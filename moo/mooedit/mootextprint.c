/*
 *   mootextprint.c
 *
 *   Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
 *
 *   This file is part of medit.  medit is free software; you can
 *   redistribute it and/or modify it under the terms of the
 *   GNU Lesser General Public License as published by the
 *   Free Software Foundation; either version 2.1 of the License,
 *   or (at your option) any later version.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with medit.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mooedit/mootextprint-private.h"
#include "mooedit/mooedit.h"
#include "mooedit/mooedit-impl.h"
#include "mooedit/mooeditview-impl.h"
#include "mooedit/mooeditprefs.h"
#include "mooedit/mootext-private.h"
#include "mooutils/moodialogs.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-debug.h"
#include "mooutils/mootype-macros.h"
#include "mooedit/mooprint-gxml.h"
#include "mooglib/moo-time.h"
#include <sys/types.h>
#include <string.h>

#ifdef __WIN32__
#include <windows.h>
#include <cairo-win32.h>
#endif

MOO_DEBUG_INIT (printing, FALSE)

#define SEPARATOR_POINTS 1.

#define PREFS_FONT                      MOO_EDIT_PREFS_PREFIX "/print/font"
#define PREFS_USE_STYLES                MOO_EDIT_PREFS_PREFIX "/print/use_styles"
#define PREFS_USE_CUSTOM_FONT           MOO_EDIT_PREFS_PREFIX "/print/use_font"
#define PREFS_WRAP                      MOO_EDIT_PREFS_PREFIX "/print/wrap"
#define PREFS_ELLIPSIZE                 MOO_EDIT_PREFS_PREFIX "/print/ellipsize"
#define PREFS_LINE_NUMBERS              MOO_EDIT_PREFS_PREFIX "/print/line_numbers"
#define PREFS_LINE_NUMBERS_STEP         MOO_EDIT_PREFS_PREFIX "/print/line_numbers_step"

#define PREFS_PRINT_HEADER              MOO_EDIT_PREFS_PREFIX "/print/header/print"
#define PREFS_PRINT_HEADER_SEPARATOR    MOO_EDIT_PREFS_PREFIX "/print/header/separator"
#define PREFS_HEADER_LEFT               MOO_EDIT_PREFS_PREFIX "/print/header/left"
#define PREFS_HEADER_CENTER             MOO_EDIT_PREFS_PREFIX "/print/header/center"
#define PREFS_HEADER_RIGHT              MOO_EDIT_PREFS_PREFIX "/print/header/right"
#define PREFS_PRINT_FOOTER              MOO_EDIT_PREFS_PREFIX "/print/footer/print"
#define PREFS_PRINT_FOOTER_SEPARATOR    MOO_EDIT_PREFS_PREFIX "/print/footer/separator"
#define PREFS_FOOTER_LEFT               MOO_EDIT_PREFS_PREFIX "/print/footer/left"
#define PREFS_FOOTER_CENTER             MOO_EDIT_PREFS_PREFIX "/print/footer/center"
#define PREFS_FOOTER_RIGHT              MOO_EDIT_PREFS_PREFIX "/print/footer/right"

#define PRINT_SETTINGS_FILE             "printsettings.ini"


typedef struct {
    double x;
    double y;
    double width;
    double height;
    double text_x;
    double text_width;
    double ln_margin;
    double ln_space;
} Page;

struct MooPrintOperationPrivate {
    GtkWindow *parent;
    GtkTextView *doc;
    GtkTextBuffer *buffer;

    /* print settings */
    int first_line;
    int last_line;          /* -1 to print everything after first_line */
    MooPrintSettings *settings;
    char *filename;
    char *basename;
    gpointer tm; /* struct tm * */

    /* aux stuff */
    GArray *pages;          /* offsets */
    PangoLayout *layout;
    double ln_height;
    PangoLayout *ln_layout;

    Page page;              /* text area */
};


#define SET_FLAG(flags, f, val)                     \
G_STMT_START {                                      \
    if (val)                                        \
        (flags) |= (f);                             \
    else                                            \
        (flags) &= (~(f));                          \
} G_STMT_END

#define GET_OPTION(op, opt) (((op)->priv->settings->flags & (opt)) != 0)


typedef struct HFFormat HFFormat;
static HFFormat *hf_format_parse    (const char         *strformat);
static void      hf_format_free     (HFFormat           *format);
static char     *hf_format_eval     (HFFormat           *format,
                                     struct tm          *tm,
                                     int                 page,
                                     int                 total_pages,
                                     const char         *filename,
                                     const char         *basename);


static GtkPageSetup *_global_page_setup;
static GtkPrintSettings *_global_print_settings;

static void moo_print_init_prefs    (void);

static void moo_print_operation_finalize    (GObject            *object);
static void moo_print_operation_set_property(GObject            *object,
                                             guint               prop_id,
                                             const GValue       *value,
                                             GParamSpec         *pspec);
static void moo_print_operation_get_property(GObject            *object,
                                             guint               prop_id,
                                             GValue             *value,
                                             GParamSpec         *pspec);

static void moo_print_operation_begin_print (GtkPrintOperation  *operation,
                                             GtkPrintContext    *context);
static void moo_print_operation_draw_page   (GtkPrintOperation  *operation,
                                             GtkPrintContext    *context,
                                             int                 page);
static void moo_print_operation_end_print   (GtkPrintOperation  *operation,
                                             GtkPrintContext    *context);
static void moo_print_operation_status_changed (GtkPrintOperation *operation);
static GtkWidget *moo_print_operation_create_custom_widget (GtkPrintOperation *operation);
static void moo_print_operation_custom_widget_apply (GtkPrintOperation *operation,
                                                     GtkWidget *widget);
static void moo_print_operation_load_prefs  (MooPrintOperation  *print);
static void update_progress                 (GtkPrintOperation  *operation,
                                             int                 page);

static void moo_print_operation_set_doc     (MooPrintOperation  *print,
                                             GtkTextView        *doc);
static void moo_print_operation_set_buffer  (MooPrintOperation  *print,
                                             GtkTextBuffer      *buffer);

static void moo_print_operation_set_settings (MooPrintOperation *op,
                                              MooPrintSettings  *settings);

static PangoLayout *create_layout           (GtkPrintContext    *context);
static void fill_layout                     (MooPrintOperation  *op,
                                             PangoLayout        *layout,
                                             const GtkTextIter  *start,
                                             const GtkTextIter  *end,
                                             gboolean            get_styles);

static GtkPageSetup     *get_global_page_setup      (void);
static GtkPrintSettings *get_global_print_settings  (void);
static void              set_global_page_setup      (GtkPageSetup *page_setup);
static void              set_global_print_settings  (GtkPrintSettings *print_settings);


G_DEFINE_TYPE(MooPrintOperation, _moo_print_operation, GTK_TYPE_PRINT_OPERATION)

enum {
    PROP_0,
    PROP_DOC,
    PROP_BUFFER,
    PROP_SETTINGS
};


static MooPrintHeaderFooter *
moo_print_header_footer_new (void)
{
    int i;
    MooPrintHeaderFooter *hf = g_new0 (MooPrintHeaderFooter, 1);

    hf->font = NULL;
    hf->separator = TRUE;
    hf->layout = NULL;

    for (i = 0; i < 3; ++i)
    {
        hf->format[i] = NULL;
        hf->parsed_format[i] = NULL;
    }

    return hf;
}


static MooPrintHeaderFooter *
moo_print_header_footer_copy (MooPrintHeaderFooter *hf)
{
    int i;
    MooPrintHeaderFooter *copy;

    g_return_val_if_fail (hf != NULL, NULL);

    copy = moo_print_header_footer_new ();
    copy->separator = hf->separator;

    if (hf->font)
        copy->font = pango_font_description_copy (hf->font);

    for (i = 0; i < 3; ++i)
        copy->format[i] = g_strdup (hf->format[i]);

    return copy;
}


static void
moo_print_header_footer_free (MooPrintHeaderFooter *hf)
{
    int i;

    if (hf->font)
        g_object_unref (hf->font);
    if (hf->layout)
        g_object_unref (hf->layout);

    for (i = 0; i < 3; ++i)
    {
        g_free (hf->format[i]);
        hf_format_free (hf->parsed_format[i]);
    }

    g_free (hf);
}


static void
moo_print_header_footer_parse_format (MooPrintHeaderFooter *hf)
{
    int i;

    for (i = 0; i < 3; ++i)
        if (hf->format[i] && !hf->parsed_format[i])
            hf->parsed_format[i] = hf_format_parse (hf->format[i]);
}


static MooPrintSettings *
moo_print_settings_new_default (void)
{
    MooPrintSettings *settings = g_new0 (MooPrintSettings, 1);
    settings->flags = MOO_PRINT_HEADER | MOO_PRINT_FOOTER;
    settings->wrap_mode = PANGO_WRAP_WORD_CHAR;
    settings->header = moo_print_header_footer_new ();
    settings->footer = moo_print_header_footer_new ();
    settings->ln_step = 1;
    return settings;
}


static MooPrintSettings *
_moo_print_settings_copy (MooPrintSettings *settings)
{
    MooPrintSettings *copy;

    g_return_val_if_fail (settings != NULL, NULL);

    copy = g_memdup (settings, sizeof (MooPrintSettings));

    copy->font = g_strdup (settings->font);
    copy->ln_font = g_strdup (settings->ln_font);
    copy->header = moo_print_header_footer_copy (settings->header);
    copy->footer = moo_print_header_footer_copy (settings->footer);

    return copy;
}

static void
_moo_print_settings_free (MooPrintSettings *settings)
{
    if (settings)
    {
        g_free (settings->font);
        g_free (settings->ln_font);
        moo_print_header_footer_free (settings->header);
        moo_print_header_footer_free (settings->footer);
        g_free (settings);
    }
}

MOO_DEFINE_BOXED_TYPE_C (MooPrintSettings, _moo_print_settings)


static void
moo_print_operation_set_settings (MooPrintOperation *op,
                                  MooPrintSettings  *settings)
{
    g_return_if_fail (MOO_IS_PRINT_OPERATION (op));

    if (op->priv->settings != settings)
    {
        _moo_print_settings_free (op->priv->settings);

        if (settings)
            op->priv->settings = _moo_print_settings_copy (settings);
        else
            op->priv->settings = moo_print_settings_new_default ();

        moo_print_header_footer_parse_format (op->priv->settings->header);
        moo_print_header_footer_parse_format (op->priv->settings->footer);

        g_object_notify (G_OBJECT (op), "settings");
    }
}


static void
moo_print_operation_finalize (GObject *object)
{
    MooPrintOperation *op = MOO_PRINT_OPERATION (object);

    if (op->priv->doc)
        g_object_unref (op->priv->doc);
    if (op->priv->buffer)
        g_object_unref (op->priv->buffer);

    _moo_print_settings_free (op->priv->settings);
    g_free (op->priv->filename);
    g_free (op->priv->basename);
    g_free (op->priv->tm);
    g_free (op->priv);

    moo_dmsg ("moo_print_operation_finalize");

    G_OBJECT_CLASS(_moo_print_operation_parent_class)->finalize (object);
}


static void
moo_print_operation_set_property (GObject            *object,
                                  guint               prop_id,
                                  const GValue       *value,
                                  GParamSpec         *pspec)
{
    MooPrintOperation *op = MOO_PRINT_OPERATION (object);

    switch (prop_id)
    {
        case PROP_DOC:
            moo_print_operation_set_doc (op, g_value_get_object (value));
            break;

        case PROP_BUFFER:
            moo_print_operation_set_buffer (op, g_value_get_object (value));
            break;

        case PROP_SETTINGS:
            moo_print_operation_set_settings (op, g_value_get_boxed (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
moo_print_operation_get_property (GObject            *object,
                                  guint               prop_id,
                                  GValue             *value,
                                  GParamSpec         *pspec)
{
    MooPrintOperation *op = MOO_PRINT_OPERATION (object);

    switch (prop_id)
    {
        case PROP_DOC:
            g_value_set_object (value, op->priv->doc);
            break;

        case PROP_BUFFER:
            g_value_set_object (value, op->priv->buffer);
            break;

        case PROP_SETTINGS:
            g_value_set_boxed (value, op->priv->settings);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
_moo_print_operation_class_init (MooPrintOperationClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkPrintOperationClass *print_class = GTK_PRINT_OPERATION_CLASS (klass);

    object_class->finalize = moo_print_operation_finalize;
    object_class->set_property = moo_print_operation_set_property;
    object_class->get_property = moo_print_operation_get_property;

    print_class->begin_print = moo_print_operation_begin_print;
    print_class->draw_page = moo_print_operation_draw_page;
    print_class->end_print = moo_print_operation_end_print;
    print_class->create_custom_widget = moo_print_operation_create_custom_widget;
    print_class->custom_widget_apply = moo_print_operation_custom_widget_apply;
    print_class->status_changed = moo_print_operation_status_changed;

    g_object_class_install_property (object_class,
                                     PROP_DOC,
                                     g_param_spec_object ("doc",
                                             "doc",
                                             "doc",
                                             GTK_TYPE_TEXT_VIEW,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_BUFFER,
                                     g_param_spec_object ("buffer",
                                             "buffer",
                                             "buffer",
                                             GTK_TYPE_TEXT_BUFFER,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_SETTINGS,
                                     g_param_spec_boxed ("settings",
                                             "settings",
                                             "settings",
                                             MOO_TYPE_PRINT_SETTINGS,
                                             (GParamFlags) G_PARAM_READWRITE));
}


static void
_moo_print_operation_init (MooPrintOperation *op)
{
    op->priv = g_new0 (MooPrintOperationPrivate, 1);

    moo_dmsg ("_moo_print_operation_init");

    gtk_print_operation_set_print_settings (GTK_PRINT_OPERATION (op),
                                            get_global_print_settings ());
    gtk_print_operation_set_default_page_setup (GTK_PRINT_OPERATION (op),
                                                get_global_page_setup ());

    op->priv->first_line = 0;
    op->priv->last_line = -1;

    op->priv->settings = moo_print_settings_new_default ();
}


static void
moo_print_operation_set_doc (MooPrintOperation *op,
                             GtkTextView       *doc)
{
    g_return_if_fail (MOO_IS_PRINT_OPERATION (op));
    g_return_if_fail (!doc || GTK_IS_TEXT_VIEW (doc));

    if (op->priv->doc == doc)
        return;

    if (op->priv->doc)
        g_object_unref (op->priv->doc);
    if (op->priv->buffer)
        g_object_unref (op->priv->buffer);

    op->priv->doc = doc;

    if (op->priv->doc)
    {
        g_object_ref (op->priv->doc);
        op->priv->buffer = gtk_text_view_get_buffer (op->priv->doc);
        g_object_ref (op->priv->buffer);
    }
    else
    {
        op->priv->buffer = NULL;
    }

    g_object_freeze_notify (G_OBJECT (op));
    g_object_notify (G_OBJECT (op), "doc");
    g_object_notify (G_OBJECT (op), "buffer");
    g_object_thaw_notify (G_OBJECT (op));
}


static void
moo_print_operation_set_buffer (MooPrintOperation *op,
                                GtkTextBuffer     *buffer)
{
    g_return_if_fail (MOO_IS_PRINT_OPERATION (op));
    g_return_if_fail (!buffer || GTK_IS_TEXT_BUFFER (buffer));

    if (op->priv->buffer == buffer)
        return;

    if (op->priv->doc)
        g_object_unref (op->priv->doc);
    if (op->priv->buffer)
        g_object_unref (op->priv->buffer);

    op->priv->doc = NULL;
    op->priv->buffer = buffer;

    if (op->priv->buffer)
        g_object_ref (op->priv->buffer);

    g_object_notify (G_OBJECT (op), "buffer");
}


void
_moo_edit_page_setup (GtkWidget *parent)
{
    GtkPageSetup *new_page_setup;
    GtkWindow *parent_window = NULL;

    g_return_if_fail (!parent || GTK_IS_WIDGET (parent));

    moo_dmsg ("_moo_edit_page_setup");

    if (parent)
        parent = gtk_widget_get_toplevel (parent);

    if (parent && GTK_IS_WINDOW (parent))
        parent_window = GTK_WINDOW (parent);

    new_page_setup = gtk_print_run_page_setup_dialog (parent_window,
                                                      get_global_page_setup (),
                                                      get_global_print_settings ());

    set_global_page_setup (new_page_setup);
}


static void
get_layout_size (PangoLayout *layout,
                 double      *width,
                 double      *height)
{
    PangoRectangle rect;

    pango_layout_get_extents (layout, NULL, &rect);

    if (width)
        *width = (double) rect.width / (double) PANGO_SCALE;

    if (height)
        *height = (double) rect.height / (double) PANGO_SCALE;
}

static void
get_layout_line_size (PangoLayoutLine *line,
                      double          *width,
                      double          *height)
{
    PangoRectangle rect;

    pango_layout_line_get_extents (line, NULL, &rect);

    if (width)
        *width = (double) rect.width / (double) PANGO_SCALE;

    if (height)
        *height = (double) rect.height / (double) PANGO_SCALE;
}


static void
header_footer_get_size (MooPrintHeaderFooter *hf,
                        MooPrintOperation    *print,
                        GtkPrintContext      *context,
                        PangoFontDescription *default_font,
                        MooPrintFlags         flag)
{
    PangoFontDescription *font = default_font;

    hf->do_print = GET_OPTION (print, flag) &&
            (hf->parsed_format[0] || hf->parsed_format[1] || hf->parsed_format[2]);

    if (hf->layout)
        g_object_unref (hf->layout);
    hf->layout = NULL;

    if (!hf->do_print)
        return;

    if (hf->font)
        font = hf->font;

    hf->layout = create_layout (context);
    pango_layout_set_text (hf->layout, "AAAyyy", -1);

    if (font)
        pango_layout_set_font_description (hf->layout, font);

    get_layout_size (hf->layout, NULL, &hf->text_height);

    if (hf->separator)
        hf->separator_height = SEPARATOR_POINTS / gtk_print_context_get_dpi_y (context);

    hf->separator_before = hf->text_height / 2;
    hf->separator_after = hf->text_height / 2;
}


static gsize
get_n_digits (guint n)
{
    gsize d = 1;
    while (n /= 10)
        d++;
    return d;
}

static void
create_ln_layout (MooPrintOperation    *op,
                  GtkPrintContext      *context,
                  PangoFontDescription *default_font)
{
    PangoFontDescription *freeme = NULL;
    PangoFontDescription *font = default_font;
    char *string;
    gsize n_digits;

    if (op->priv->ln_layout)
        g_object_unref (op->priv->ln_layout);

    op->priv->ln_layout = create_layout (context);

    if (op->priv->settings->ln_font)
        freeme = font = pango_font_description_from_string (op->priv->settings->ln_font);

    if (font)
        pango_layout_set_font_description (op->priv->ln_layout, font);

    n_digits = get_n_digits (op->priv->last_line);
    string = g_strnfill (n_digits, '9');
    pango_layout_set_text (op->priv->ln_layout, string, -1);
    get_layout_size (op->priv->ln_layout, &op->priv->page.ln_margin, &op->priv->ln_height);
    op->priv->page.ln_space = 3.;

    op->priv->page.text_x = op->priv->page.x + op->priv->page.ln_margin + op->priv->page.ln_space;
    op->priv->page.text_width = op->priv->page.width - op->priv->page.text_x;

    pango_layout_set_alignment (op->priv->ln_layout, PANGO_ALIGN_RIGHT);
    pango_layout_set_width (op->priv->ln_layout, (int) op->priv->page.ln_margin);

    if (freeme)
        pango_font_description_free (freeme);
    g_free (string);
}


static void
moo_print_operation_calc_page_size (MooPrintOperation    *op,
                                    GtkPrintContext      *context,
                                    PangoFontDescription *default_font)
{
    Page *page = &op->priv->page;
    MooPrintSettings *settings = op->priv->settings;
    MooPrintHeaderFooter *header = settings->header;
    MooPrintHeaderFooter *footer = settings->footer;

    page->x = 0.;
    page->y = 0.;
    page->width = gtk_print_context_get_width (context);
    page->height = gtk_print_context_get_height (context);

    page->ln_margin = 0.;
    page->ln_space = 0.;
    page->text_x = page->x;
    page->text_width = page->width;

    moo_dmsg ("page size: %f, %f - %f in, %f in",
              page->width, page->height,
              page->width / gtk_print_context_get_dpi_x (context),
              page->height / gtk_print_context_get_dpi_y (context));

    header_footer_get_size (settings->header, op, context,
                            default_font, MOO_PRINT_HEADER);
    header_footer_get_size (settings->footer, op, context,
                            default_font, MOO_PRINT_FOOTER);

    if (header->do_print)
    {
        double delta = header->text_height + header->separator_before +
                       header->separator_after + header->separator_height;
        page->y += delta;
        page->height -= delta;
    }

    if (footer->do_print)
    {
        double delta = footer->text_height + footer->separator_before +
                    footer->separator_after + footer->separator_height;
        page->height -= delta;
    }

    if (page->height < 0)
    {
        g_critical ("page messed up");
        page->y = 0.;
        page->height = gtk_print_context_get_height (context);
    }

    moo_dmsg ("printable area size: %f, %f - %f in, %f in",
              page->width, page->height,
              page->width / gtk_print_context_get_dpi_x (context),
              page->height / gtk_print_context_get_dpi_y (context));

    if (GET_OPTION (op, MOO_PRINT_LINE_NUMBERS))
        create_ln_layout (op, context, default_font);
}


static gboolean
line_number_displayed (MooPrintOperation *op,
                       int                line_no)
{
    return GET_OPTION (op, MOO_PRINT_LINE_NUMBERS) &&
           ((line_no+1) % op->priv->settings->ln_step) == 0;
}


static void
moo_print_operation_paginate (MooPrintOperation *op)
{
    GtkTextIter iter, print_end;
    double page_height;
    gboolean use_styles;
    int line_no;
    int offset;

    moo_dmsg ("moo_print_operation_paginate");
    moo_dmsg ("page height: %f", op->priv->page.height);

    if (op->priv->pages)
        g_array_free (op->priv->pages, TRUE);

    op->priv->pages = g_array_new (FALSE, FALSE, sizeof (int));
    gtk_text_buffer_get_iter_at_line (op->priv->buffer, &iter,
                                      op->priv->first_line);
    gtk_text_buffer_get_iter_at_line (op->priv->buffer, &print_end,
                                      op->priv->last_line);
    gtk_text_iter_forward_line (&print_end);
    offset = gtk_text_iter_get_offset (&iter);
    g_array_append_val (op->priv->pages, offset);
    page_height = 0;
    line_no = op->priv->first_line;

    use_styles = GET_OPTION (op, MOO_PRINT_USE_STYLES);

    if (use_styles && MOO_IS_TEXT_BUFFER (op->priv->buffer))
        _moo_text_buffer_update_highlight (MOO_TEXT_BUFFER (op->priv->buffer),
                                           &iter, &print_end, TRUE);

    while (gtk_text_iter_compare (&iter, &print_end) < 0)
    {
        GtkTextIter end;
        double line_height;

        end = iter;

        if (!gtk_text_iter_ends_line (&end))
            gtk_text_iter_forward_to_line_end (&end);

        fill_layout (op, op->priv->layout, &iter, &end,
                     GET_OPTION (op, MOO_PRINT_USE_STYLES));

        get_layout_size (op->priv->layout, NULL, &line_height);

        if (line_number_displayed (op, line_no))
            line_height = MAX (line_height, op->priv->ln_height);

#define EPS (.1)
        if (page_height > EPS && page_height + line_height > op->priv->page.height + EPS)
        {
            gboolean part = FALSE;

            if (GET_OPTION (op, MOO_PRINT_WRAP) && pango_layout_get_line_count (op->priv->layout) > 1)
            {
                double part_height = 0;
                PangoLayoutIter *layout_iter;
                gboolean is_first_line = TRUE;

                layout_iter = pango_layout_get_iter (op->priv->layout);

                do
                {
                    PangoLayoutLine *layout_line;
                    double layout_line_height;

                    layout_line = pango_layout_iter_get_line (layout_iter);
                    get_layout_line_size (layout_line, NULL, &layout_line_height);

                    if (is_first_line && line_number_displayed (op, line_no))
                        layout_line_height = MAX (layout_line_height, op->priv->ln_height);

                    if (page_height + part_height + layout_line_height > op->priv->page.height + EPS)
                        break;

                    is_first_line = FALSE;
                    part_height += layout_line_height;
                    part = TRUE;
                }
                while (pango_layout_iter_next_line (layout_iter));

                if (part)
                {
                    int index = pango_layout_iter_get_index (layout_iter);
                    iter = end;
                    gtk_text_iter_set_line_index (&iter, index);
                    line_height = 0;
                }

                pango_layout_iter_free (layout_iter);
            }

            offset = gtk_text_iter_get_offset (&iter);
            g_array_append_val (op->priv->pages, offset);
            page_height = line_height;

            if (!part)
                gtk_text_iter_forward_line (&iter);
        }
        else
        {
            page_height += line_height;
            gtk_text_iter_forward_line (&iter);
        }
    }
#undef EPS

    gtk_print_operation_set_n_pages (GTK_PRINT_OPERATION (op), op->priv->pages->len);

    moo_dmsg ("moo_print_operation_paginate done");
}


static void
set_tabs (MooPrintOperation *op,
          PangoLayout       *layout)
{
    PangoTabArray *tabs;
    int tab_width;
    char *string = NULL;

    if (MOO_IS_TEXT_VIEW (op->priv->doc))
    {
        guint n_spaces;

        g_object_get (op->priv->doc, "tab-width", &n_spaces, NULL);

        if (n_spaces != 8)
            string = g_strnfill (n_spaces, ' ');
    }

    if (!string)
        return;

    pango_layout_set_text (layout, string, -1);
    pango_layout_get_size (layout, &tab_width, NULL);

    tabs = pango_tab_array_new (2, FALSE);
    pango_tab_array_set_tab (tabs, 0, PANGO_TAB_LEFT, 0);
    pango_tab_array_set_tab (tabs, 1, PANGO_TAB_LEFT, tab_width);
    pango_layout_set_tabs (layout, tabs);

    pango_tab_array_free (tabs);
    g_free (string);
}

static void
moo_print_operation_begin_print (GtkPrintOperation *operation,
                                 GtkPrintContext   *context)
{
    MooPrintOperation *op = MOO_PRINT_OPERATION (operation);
    MooPrintSettings *settings;
    PangoFontDescription *font = NULL;
    GTimer *timer;
    mgw_time_t t;
    mgw_errno_t err;

    g_return_if_fail (op->priv->buffer != NULL);
    g_return_if_fail (op->priv->first_line >= 0);
    g_return_if_fail (op->priv->last_line < 0 || op->priv->last_line >= op->priv->first_line);
    g_return_if_fail (op->priv->first_line < gtk_text_buffer_get_line_count (op->priv->buffer));
    g_return_if_fail (op->priv->last_line < gtk_text_buffer_get_line_count (op->priv->buffer));

    moo_dmsg ("moo_print_operation_begin_print");

    moo_print_operation_load_prefs (op);
    settings = op->priv->settings;

    timer = g_timer_new ();

    if (MOO_IS_EDIT_VIEW (op->priv->doc))
    {
        MooEdit *doc = moo_edit_view_get_doc (MOO_EDIT_VIEW (op->priv->doc));
        _moo_edit_set_state (doc,
                             MOO_EDIT_STATE_PRINTING,
                             "Printing",
                             (GDestroyNotify) gtk_print_operation_cancel,
                             op);
    }

    if (op->priv->last_line < 0)
        op->priv->last_line = gtk_text_buffer_get_line_count (op->priv->buffer) - 1;

    if (settings->font)
        font = pango_font_description_from_string (settings->font);

    if (!font && op->priv->doc)
    {
        PangoContext *widget_ctx;

        widget_ctx = gtk_widget_get_pango_context (GTK_WIDGET (op->priv->doc));

        if (widget_ctx)
        {
            font = pango_context_get_font_description (widget_ctx);

            if (font)
                font = pango_font_description_copy_static (font);
        }
    }

    moo_print_operation_calc_page_size (op, context, font);

    if (op->priv->layout)
        g_object_unref (op->priv->layout);

    op->priv->layout = create_layout (context);

    if (GET_OPTION (op, MOO_PRINT_WRAP))
    {
        pango_layout_set_width (op->priv->layout, (int) (op->priv->page.text_width * PANGO_SCALE));
        pango_layout_set_wrap (op->priv->layout, settings->wrap_mode);
    }
    else if (GET_OPTION (op, MOO_PRINT_ELLIPSIZE))
    {
        pango_layout_set_width (op->priv->layout, (int) (op->priv->page.text_width * PANGO_SCALE));
        pango_layout_set_ellipsize (op->priv->layout, PANGO_ELLIPSIZE_END);
    }

    if (font)
    {
        pango_layout_set_font_description (op->priv->layout, font);
        pango_font_description_free (font);
    }

    set_tabs (op, op->priv->layout);

    moo_print_operation_paginate (op);

    moo_dmsg ("begin_print: %u pages in %f s", op->priv->pages->len,
              g_timer_elapsed (timer, NULL));
    g_timer_destroy (timer);

    if (!op->priv->tm)
        op->priv->tm = g_new (struct tm, 1);

    mgw_time (&t, &err);

    if (mgw_errno_is_set (err) || !mgw_localtime_r (&t, op->priv->tm, &err))
    {
        g_critical ("time: %s", mgw_strerror (err));
        g_free (op->priv->tm);
        op->priv->tm = NULL;
    }

    moo_dmsg ("moo_print_operation_begin_print done");
}


static gboolean
ignore_tag (MooPrintOperation *op,
            GtkTextTag        *tag)
{
    if (MOO_IS_TEXT_BUFFER (op->priv->buffer) &&
        _moo_text_buffer_is_bracket_tag (MOO_TEXT_BUFFER (op->priv->buffer), tag))
            return TRUE;

    return FALSE;
}

static GSList *
get_iter_attrs (MooPrintOperation *op,
                GtkTextIter       *iter,
                const GtkTextIter *limit)
{
    GSList *attrs = NULL, *tags;
    PangoAttribute *bg = NULL, *fg = NULL, *style = NULL, *ul = NULL;
    PangoAttribute *weight = NULL, *st = NULL;

    tags = gtk_text_iter_get_tags (iter);
    gtk_text_iter_forward_to_tag_toggle (iter, NULL);

    if (gtk_text_iter_compare (iter, limit) > 0)
        *iter = *limit;

    while (tags)
    {
        GtkTextTag *tag;
        gboolean bg_set, fg_set, style_set, ul_set, weight_set, st_set;

        tag = tags->data;
        tags = g_slist_delete_link (tags, tags);

        if (ignore_tag (op, tag))
            continue;

        g_object_get (tag,
                      "background-set", &bg_set,
                      "foreground-set", &fg_set,
                      "style-set", &style_set,
                      "underline-set", &ul_set,
                      "weight-set", &weight_set,
                      "strikethrough-set", &st_set,
                      NULL);

        if (bg_set)
        {
            GdkColor *color = NULL;
            if (bg) pango_attribute_destroy (bg);
            g_object_get (tag, "background-gdk", &color, NULL);
            bg = pango_attr_background_new (color->red, color->green, color->blue);
            gdk_color_free (color);
        }

        if (fg_set)
        {
            GdkColor *color = NULL;
            if (fg) pango_attribute_destroy (fg);
            g_object_get (tag, "foreground-gdk", &color, NULL);
            fg = pango_attr_foreground_new (color->red, color->green, color->blue);
            gdk_color_free (color);
        }

        if (style_set)
        {
            PangoStyle style_value;
            if (style) pango_attribute_destroy (style);
            g_object_get (tag, "style", &style_value, NULL);
            style = pango_attr_style_new (style_value);
        }

        if (ul_set)
        {
            PangoUnderline underline;
            if (ul) pango_attribute_destroy (ul);
            g_object_get (tag, "underline", &underline, NULL);
            ul = pango_attr_underline_new (underline);
        }

        if (weight_set)
        {
            PangoWeight weight_value;
            if (weight) pango_attribute_destroy (weight);
            g_object_get (tag, "weight", &weight_value, NULL);
            weight = pango_attr_weight_new (weight_value);
        }

        if (st_set)
        {
            gboolean strikethrough;
            if (st) pango_attribute_destroy (st);
            g_object_get (tag, "strikethrough", &strikethrough, NULL);
            st = pango_attr_strikethrough_new (strikethrough);
        }
    }

    if (bg)
        attrs = g_slist_prepend (attrs, bg);
    if (fg)
        attrs = g_slist_prepend (attrs, fg);
    if (style)
        attrs = g_slist_prepend (attrs, style);
    if (ul)
        attrs = g_slist_prepend (attrs, ul);
    if (weight)
        attrs = g_slist_prepend (attrs, weight);
    if (st)
        attrs = g_slist_prepend (attrs, st);

    return attrs;
}


static PangoLayout *
create_layout (GtkPrintContext *context)
{
    PangoLayout *layout;
    PangoContext *pango_context;
    cairo_font_options_t *options;

    g_return_val_if_fail (GTK_IS_PRINT_CONTEXT (context), NULL);

    pango_context = gtk_print_context_create_pango_context (context);
    pango_cairo_update_context (gtk_print_context_get_cairo_context (context), pango_context);

    options = cairo_font_options_copy (pango_cairo_context_get_font_options (pango_context));
    cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_OFF);
    cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_NONE);
    pango_cairo_context_set_font_options (pango_context, options);

    layout = pango_layout_new (pango_context);

    cairo_font_options_destroy (options);
    g_object_unref (pango_context);
    return layout;
}


static void
fill_layout (MooPrintOperation *op,
             PangoLayout       *layout,
             const GtkTextIter *start,
             const GtkTextIter *end,
             gboolean           get_styles)
{
    char *text;
    PangoAttrList *attr_list;
    GtkTextIter segm_start, segm_end;
    int start_index;

    text = gtk_text_iter_get_slice (start, end);
    pango_layout_set_text (layout, text, -1);
    g_free (text);

    if (!get_styles)
        return;

    attr_list = NULL;
    segm_start = *start;
    start_index = gtk_text_iter_get_line_index (start);
#if 0
    moo_dmsg ("line %d, start at %d\n", gtk_text_iter_get_line (start), start_index);
#endif

    while (gtk_text_iter_compare (&segm_start, end) < 0)
    {
        GSList *attrs;

        segm_end = segm_start;
        attrs = get_iter_attrs (op, &segm_end, end);

        if (attrs)
        {
            int si, ei;

            si = gtk_text_iter_get_line_index (&segm_start) - start_index;
            ei = gtk_text_iter_get_line_index (&segm_end) - start_index;

            while (attrs)
            {
                PangoAttribute *a = attrs->data;

                a->start_index = si;
                a->end_index = ei;

                if (!attr_list)
                    attr_list = pango_attr_list_new ();

                pango_attr_list_insert (attr_list, a);

                attrs = g_slist_delete_link (attrs, attrs);
            }
        }

        segm_start = segm_end;
    }

    pango_layout_set_attributes (layout, attr_list);

    if (attr_list)
        pango_attr_list_unref (attr_list);
}


static void
print_header_footer (MooPrintOperation    *op,
                     cairo_t              *cr,
                     MooPrintHeaderFooter *hf,
                     int                   page_no,
                     gboolean              header)
{
    double y;
    char *text;
    int total_pages;
    Page *page = &op->priv->page;

    if (header)
        y = 0;
    else
        y = page->y + page->height + hf->separator_before +
            hf->separator_after + hf->separator_height;

    g_object_get (op, "n-pages", &total_pages, NULL);

    if (hf->parsed_format[0] &&
        (text = hf_format_eval (hf->parsed_format[0], op->priv->tm, page_no, total_pages,
                                op->priv->filename, op->priv->basename)))
    {
        pango_layout_set_text (hf->layout, text, -1);
        cairo_move_to (cr, page->x, y);
        pango_cairo_show_layout (cr, hf->layout);
        g_free (text);
    }

    if (hf->parsed_format[1] &&
        (text = hf_format_eval (hf->parsed_format[1], op->priv->tm, page_no, total_pages,
                                op->priv->filename, op->priv->basename)))
    {
        double w;
        pango_layout_set_text (hf->layout, text, -1);
        get_layout_size (hf->layout, &w, NULL);
        cairo_move_to (cr, page->x + page->width/2. - w/2., y);
        pango_cairo_show_layout (cr, hf->layout);
        g_free (text);
    }

    if (hf->parsed_format[2] &&
        (text = hf_format_eval (hf->parsed_format[2], op->priv->tm, page_no, total_pages,
                                op->priv->filename, op->priv->basename)))
    {
        double w;
        pango_layout_set_text (hf->layout, text, -1);
        get_layout_size (hf->layout, &w, NULL);
        cairo_move_to (cr, page->x + page->width - w, y);
        pango_cairo_show_layout (cr, hf->layout);
        g_free (text);
    }

    if (hf->separator)
    {
        if (header)
            y = hf->text_height + hf->separator_before + hf->separator_height/2.;
        else
            y = page->y + page->height + hf->separator_after + hf->separator_height/2.;

        cairo_move_to (cr, page->x, y);
        cairo_set_line_width (cr, hf->separator_height);
        cairo_line_to (cr, page->x + page->width, y);
        cairo_stroke (cr);
    }
}


static void
print_line_number (MooPrintOperation *op,
                   int                line_no,
                   double             offset,
                   PangoLayout       *ln_layout,
                   PangoLayout       *text_layout,
                   double            *text_offset,
                   double            *line_height,
                   cairo_t           *cr)
{
    char *string;
    PangoLayoutIter *ln_iter, *text_iter;
    double text_baseline, ln_baseline;
    double ln_offset;
    double ln_height, text_height;

    string = g_strdup_printf ("%d", line_no+1);
    pango_layout_set_text (ln_layout, string, -1);

    text_iter = pango_layout_get_iter (text_layout);
    text_baseline = (double) pango_layout_iter_get_baseline (text_iter) / (double) PANGO_SCALE;
    get_layout_size (text_layout, NULL, &text_height);
    ln_iter = pango_layout_get_iter (ln_layout);
    ln_baseline = (double) pango_layout_iter_get_baseline (ln_iter) / (double) PANGO_SCALE;
    get_layout_size (ln_layout, NULL, &ln_height);

    if (text_baseline > ln_baseline)
    {
        ln_offset = offset + text_baseline - ln_baseline;
        *text_offset = offset;
        *line_height = MAX (text_height, text_baseline + ln_height - ln_baseline);
    }
    else
    {
        ln_offset = offset;
        *text_offset = offset + ln_baseline - text_baseline;
        *line_height = MAX (ln_height, ln_baseline + text_height - text_baseline);
    }

    cairo_move_to (cr, op->priv->page.x + op->priv->page.ln_margin, ln_offset);
    pango_cairo_show_layout (cr, ln_layout);

    pango_layout_iter_free (ln_iter);
    pango_layout_iter_free (text_iter);
    g_free (string);
}

static void
print_page (MooPrintOperation *op,
            const GtkTextIter *start,
            const GtkTextIter *end,
            int                page,
            cairo_t           *cr)
{
    MooPrintSettings *settings = op->priv->settings;
    GtkTextIter line_start, line_end;
    double offset;
    int line_no;

    moo_dmsg ("print_page %d", page);

    cairo_set_source_rgb (cr, 0., 0., 0.);

    if (settings->header->do_print)
        print_header_footer (op, cr, settings->header, page, TRUE);
    if (settings->footer->do_print)
        print_header_footer (op, cr, settings->footer, page, FALSE);

    line_start = *start;
    offset = op->priv->page.y;
    line_no = gtk_text_iter_get_line (start);

    while (gtk_text_iter_compare (&line_start, end) < 0)
    {
        double line_height;
        gboolean print_line_no;

        if (gtk_text_iter_ends_line (&line_start))
        {
            pango_layout_set_text (op->priv->layout, "", 0);
            pango_layout_set_attributes (op->priv->layout, NULL);
        }
        else
        {
            line_end = line_start;
            gtk_text_iter_forward_to_line_end (&line_end);

            if (gtk_text_iter_compare (&line_end, end) > 0)
                line_end = *end;

            fill_layout (op, op->priv->layout, &line_start, &line_end,
                         GET_OPTION (op, MOO_PRINT_USE_STYLES));
        }

        print_line_no = gtk_text_iter_starts_line (&line_start) &&
                        line_number_displayed (op, line_no);

        if (!print_line_no)
        {
            get_layout_size (op->priv->layout, NULL, &line_height);
            cairo_move_to (cr, op->priv->page.text_x, offset);
        }
        else
        {
            double text_offset;
            print_line_number (op, line_no, offset,
                               op->priv->ln_layout, op->priv->layout,
                               &text_offset, &line_height, cr);
            cairo_move_to (cr, op->priv->page.text_x, text_offset);
        }

        pango_cairo_show_layout (cr, op->priv->layout);
        offset += line_height;

        gtk_text_iter_forward_line (&line_start);
        line_no += 1;
    }

    moo_dmsg ("print_page done");
}


static void
moo_print_operation_draw_page (GtkPrintOperation *operation,
                               GtkPrintContext   *context,
                               int                page)
{
    cairo_t *cr;
    int offset;
    GtkTextIter start, end;
    MooPrintOperation *op = MOO_PRINT_OPERATION (operation);
    GTimer *timer;

    g_return_if_fail (op->priv->buffer != NULL);
    g_return_if_fail (op->priv->pages != NULL);
    g_return_if_fail (op->priv->layout != NULL);
    g_return_if_fail (page < (int) op->priv->pages->len);

    update_progress (operation, page);

    cr = gtk_print_context_get_cairo_context (context);

    offset = g_array_index (op->priv->pages, int, page);
    gtk_text_buffer_get_iter_at_offset (op->priv->buffer, &start, offset);

    if (page + 1 < (int) op->priv->pages->len)
    {
        offset = g_array_index (op->priv->pages, int, page + 1);
        gtk_text_buffer_get_iter_at_offset (op->priv->buffer, &end, offset);
    }
    else
    {
        gtk_text_buffer_get_end_iter (op->priv->buffer, &end);
    }

#if 0 && defined(__WIN32__)
    if (page == 0)
    {
        HDC dc = cairo_win32_surface_get_dc (cairo_get_target (cr));

        if (dc)
        {
            int dpi_x = GetDeviceCaps (dc, LOGPIXELSX);
            int dpi_y = GetDeviceCaps (dc, LOGPIXELSY);
            g_print ("dpi: %d, %d\n", dpi_x, dpi_y);
        }
    }
#endif

#if 0 && defined(MOO_DEBUG) && !defined(__WIN32__)
    cairo_save (cr);
    cairo_set_line_width (cr, 1.);
    cairo_set_source_rgb (cr, 1., 0., 0.);
    cairo_rectangle (cr,
                     op->priv->page.x,
                     op->priv->page.y,
                     op->priv->page.width,
                     op->priv->page.height);
    cairo_stroke (cr);
    cairo_set_source_rgb (cr, 0., 1., 0.);
    cairo_rectangle (cr, 0, 0,
                     gtk_print_context_get_width (context),
                     gtk_print_context_get_height (context));
    cairo_stroke (cr);
    cairo_restore (cr);
#endif

    timer = g_timer_new ();

    pango_cairo_update_layout (cr, op->priv->layout);
    if (op->priv->settings->header->layout)
        pango_cairo_update_layout (cr, op->priv->settings->header->layout);
    if (op->priv->settings->footer->layout)
        pango_cairo_update_layout (cr, op->priv->settings->footer->layout);
    print_page (op, &start, &end, page, cr);

    moo_dmsg ("page %d: %f s", page, g_timer_elapsed (timer, NULL));
    g_timer_destroy (timer);
}


static void
moo_print_operation_end_print (GtkPrintOperation  *operation,
                               G_GNUC_UNUSED GtkPrintContext *context)
{
    MooPrintOperation *op = MOO_PRINT_OPERATION (operation);

    g_return_if_fail (op->priv->buffer != NULL);

    moo_dmsg ("moo_print_operation_end_print");

    if (MOO_IS_EDIT_VIEW (op->priv->doc))
        _moo_edit_set_state (moo_edit_view_get_doc (MOO_EDIT_VIEW (op->priv->doc)),
                             MOO_EDIT_STATE_NORMAL,
                             NULL, NULL, NULL);

    g_object_unref (op->priv->layout);
    op->priv->layout = NULL;
    g_array_free (op->priv->pages, TRUE);
    op->priv->pages = NULL;
}


static void
update_progress (GtkPrintOperation *operation,
                 int                page)
{
    char *text = NULL;
    MooPrintOperation *op = MOO_PRINT_OPERATION (operation);
    GtkPrintStatus status;
    MooEditWindow *window;
    MooEditView *view;
    MooEdit *doc;

    if (!MOO_IS_EDIT_VIEW (op->priv->doc))
        return;

    view = MOO_EDIT_VIEW (op->priv->doc);
    doc = moo_edit_view_get_doc (view);
    window = moo_edit_view_get_window (view);
    status = gtk_print_operation_get_status (operation);

    if (status == GTK_PRINT_STATUS_FINISHED)
    {
        text = g_strdup ("Finished printing");
    }
    else if (status == GTK_PRINT_STATUS_GENERATING_DATA && page >= 0)
    {
        int n_pages;
        g_object_get (op, "n-pages", &n_pages, NULL);
        text = g_strdup_printf ("Printing page %d of %d", page + 1, n_pages);
    }
    else
    {
        text = g_strdup (gtk_print_operation_get_status_string (operation));
    }

    if (window)
        moo_window_message (MOO_WINDOW (window), NULL);

    if (_moo_edit_get_state (doc) == MOO_EDIT_STATE_PRINTING)
        _moo_edit_set_progress_text (doc, text);
    else if (window)
        moo_window_message (MOO_WINDOW (window), text);

    g_free (text);
}


static void
moo_print_operation_status_changed (GtkPrintOperation *op)
{
    update_progress (op, -1);
}


static void
moo_print_operation_set_filename (MooPrintOperation *op,
                                  const char        *filename,
                                  const char        *basename)
{
    g_return_if_fail (MOO_IS_PRINT_OPERATION (op));
    MOO_ASSIGN_STRING (op->priv->filename, filename);
    MOO_ASSIGN_STRING (op->priv->basename, basename);
}


static MooPrintSettings *
get_settings_from_prefs (void)
{
    MooPrintSettings *settings;
    MooPrintHeaderFooter *hf;

    moo_print_init_prefs ();
    settings = moo_print_settings_new_default ();

    hf = settings->header;
    hf->format[0] = g_strdup (moo_prefs_get_string (PREFS_HEADER_LEFT));
    hf->format[1] = g_strdup (moo_prefs_get_string (PREFS_HEADER_CENTER));
    hf->format[2] = g_strdup (moo_prefs_get_string (PREFS_HEADER_RIGHT));
    hf->separator = moo_prefs_get_bool (PREFS_PRINT_HEADER_SEPARATOR);
    SET_FLAG (settings->flags, MOO_PRINT_HEADER, moo_prefs_get_bool (PREFS_PRINT_HEADER));

    hf = settings->footer;
    hf->format[0] = g_strdup (moo_prefs_get_string (PREFS_FOOTER_LEFT));
    hf->format[1] = g_strdup (moo_prefs_get_string (PREFS_FOOTER_CENTER));
    hf->format[2] = g_strdup (moo_prefs_get_string (PREFS_FOOTER_RIGHT));
    hf->separator = moo_prefs_get_bool (PREFS_PRINT_FOOTER_SEPARATOR);
    SET_FLAG (settings->flags, MOO_PRINT_FOOTER, moo_prefs_get_bool (PREFS_PRINT_FOOTER));

    if (moo_prefs_get_bool (PREFS_USE_CUSTOM_FONT))
        settings->font = g_strdup (moo_prefs_get_string (PREFS_FONT));

    SET_FLAG (settings->flags, MOO_PRINT_USE_STYLES, moo_prefs_get_bool (PREFS_USE_STYLES));
    SET_FLAG (settings->flags, MOO_PRINT_WRAP, moo_prefs_get_bool (PREFS_WRAP));
    SET_FLAG (settings->flags, MOO_PRINT_ELLIPSIZE, moo_prefs_get_bool (PREFS_ELLIPSIZE));
    SET_FLAG (settings->flags, MOO_PRINT_LINE_NUMBERS, moo_prefs_get_bool (PREFS_LINE_NUMBERS));

    settings->ln_step = moo_prefs_get_int (PREFS_LINE_NUMBERS_STEP);

    return settings;
}


static void
moo_print_operation_load_prefs (MooPrintOperation *op)
{
    MooPrintSettings *settings = get_settings_from_prefs ();
    moo_print_operation_set_settings (op, settings);
    _moo_print_settings_free (settings);
}


static GKeyFile *
get_print_settings_file (const char *filename)
{
    GKeyFile *key_file = NULL;

    if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
        GError *error = NULL;
        key_file = g_key_file_new ();
        if (!g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, &error))
        {
            g_warning ("could not load print settings file '%s': %s",
                       filename, moo_error_message (error));
            g_error_free (error);
            g_key_file_free (key_file);
            key_file = NULL;
        }
    }

    return key_file;
}


static void
load_print_settings (void)
{
    static gboolean been_here = FALSE;
    char *file = NULL;
    GKeyFile *key_file = NULL;
    GError *error = NULL;

    if (been_here)
        goto out;

    file = moo_get_user_cache_file (PRINT_SETTINGS_FILE);
    key_file = get_print_settings_file (file);
    if (!key_file)
        goto out;

#define IGNORABLE_ERROR(err) ((err) && (err)->domain == GTK_PRINT_ERROR \
                                && (err)->code == GTK_PRINT_ERROR_INVALID_FILE)

    _global_page_setup = gtk_page_setup_new_from_key_file (key_file, NULL, &error);
    if (!_global_page_setup)
    {
        if (!IGNORABLE_ERROR(error))
            g_warning ("could not load page setup from file '%s': %s",
                       file, moo_error_message (error));
        g_error_free (error);
        error = NULL;
    }

    _global_print_settings = gtk_print_settings_new_from_key_file (key_file, NULL, &error);
    if (!_global_print_settings)
    {
        if (!IGNORABLE_ERROR(error))
            g_warning ("could not load print settings from file '%s': %s",
                       file, moo_error_message (error));
        g_error_free (error);
        error = NULL;
    }

#undef IGNORABLE_ERROR

out:
    if (key_file)
        g_key_file_free (key_file);
    g_free (file);
}


static GtkPageSetup *
get_global_page_setup (void)
{
    load_print_settings ();
    return _global_page_setup;
}


static GtkPrintSettings *
get_global_print_settings (void)
{
    load_print_settings ();
    return _global_print_settings;
}


static GKeyFile *
get_key_file_for_saving (void)
{
    char *file = NULL;
    GKeyFile *key_file = NULL;

    file = moo_get_user_cache_file (PRINT_SETTINGS_FILE);
    key_file = get_print_settings_file (file);
    if (!key_file)
        key_file = g_key_file_new ();

    g_free (file);
    return key_file;
}


static void
save_print_settings_file (GKeyFile* key_file)
{
    char *file = NULL;
    char *contents = NULL;
    GError *error = NULL;

    file = moo_get_user_cache_file (PRINT_SETTINGS_FILE);
    contents = g_key_file_to_data (key_file, NULL, NULL);

    if (contents && !moo_save_config_file (file, contents, -1, &error))
    {
        g_warning ("Could not save print settings file '%s': %s", file, moo_error_message (error));
        g_error_free (error);
        error = NULL;
    }

    g_free (contents);
    g_free (file);
}


static void
save_page_setup (GtkPageSetup *page_setup)
{
    GKeyFile* key_file = get_key_file_for_saving ();
    gtk_page_setup_to_key_file (page_setup, key_file, NULL);
    save_print_settings_file (key_file);
    g_key_file_free (key_file);
}


static void
save_print_settings (GtkPrintSettings *print_settings)
{
    GKeyFile* key_file = get_key_file_for_saving ();
    gtk_print_settings_to_key_file (print_settings, key_file, NULL);
    save_print_settings_file (key_file);
    g_key_file_free (key_file);
}


static void
set_global_page_setup (GtkPageSetup *page_setup)
{
    MOO_ASSIGN_OBJ (_global_page_setup, page_setup);
    if (page_setup)
        save_page_setup (page_setup);
}


static void
set_global_print_settings (GtkPrintSettings *print_settings)
{
    MOO_ASSIGN_OBJ (_global_print_settings, print_settings);
    if (print_settings)
        save_print_settings (print_settings);
}


static void
do_print_operation (GtkTextView            *view,
                    GtkWidget              *parent,
                    GtkPrintOperationAction action,
                    const char             *exp_filename)
{
    MooPrintOperation *op;
    GtkPrintOperationResult res;
    GError *error = NULL;
    GtkWidget *error_dialog;

    g_return_if_fail (GTK_IS_TEXT_VIEW (view));
    g_return_if_fail (!exp_filename || action == GTK_PRINT_OPERATION_ACTION_EXPORT);

    op = MOO_PRINT_OPERATION (g_object_new (MOO_TYPE_PRINT_OPERATION,
                                            "doc", view,
                                            "export-filename", exp_filename,
                                            (const char*) NULL));

    if (!parent)
        parent = GTK_WIDGET (view);

    if (parent)
        parent = gtk_widget_get_toplevel (parent);

    if (GTK_IS_WINDOW (parent))
        op->priv->parent = GTK_WINDOW (parent);

    if (MOO_IS_EDIT_VIEW (view))
    {
        MooEdit *doc = moo_edit_view_get_doc (MOO_EDIT_VIEW (view));
        const char *filename, *basename;
        filename = moo_edit_get_display_name (doc);
        basename = moo_edit_get_display_basename (doc);
        moo_print_operation_set_filename (op, filename, basename);
    }

    res = gtk_print_operation_run (GTK_PRINT_OPERATION (op),
                                   action, op->priv->parent, &error);

    switch (res)
    {
        case GTK_PRINT_OPERATION_RESULT_ERROR:
            error_dialog = gtk_message_dialog_new (op->priv->parent,
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "Error:\n%s",
                                                   moo_error_message (error));
            gtk_dialog_run (GTK_DIALOG (error_dialog));
            gtk_widget_destroy (error_dialog);
            g_error_free (error);
            break;

        case GTK_PRINT_OPERATION_RESULT_APPLY:
            set_global_print_settings (gtk_print_operation_get_print_settings (GTK_PRINT_OPERATION (op)));
            break;

        case GTK_PRINT_OPERATION_RESULT_CANCEL:
        case GTK_PRINT_OPERATION_RESULT_IN_PROGRESS:
            break;
    }

    g_object_unref (op);
}


void
_moo_edit_print (GtkTextView *view,
                 GtkWidget   *parent)
{
    do_print_operation (view, parent, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, NULL);
}


void
_moo_edit_print_preview (GtkTextView *view,
                         GtkWidget   *parent)
{
    do_print_operation (view, parent, GTK_PRINT_OPERATION_ACTION_PREVIEW, NULL);
}


void
_moo_edit_export_pdf (GtkTextView *view,
                      const char  *filename)
{
    do_print_operation (view, NULL, GTK_PRINT_OPERATION_ACTION_EXPORT, filename);
}


GtkWindow *
_moo_print_operation_get_parent (MooPrintOperation *op)
{
    g_return_val_if_fail (MOO_IS_PRINT_OPERATION (op), NULL);
    return op->priv->parent;
}


int
_moo_print_operation_get_n_pages (MooPrintOperation *op)
{
    g_return_val_if_fail (MOO_IS_PRINT_OPERATION (op), -1);
    return op->priv->pages ? (int) op->priv->pages->len : -1;
}


/*****************************************************************************/
/* Header/footer gui
 */

#define SET_TEXT(wid,setting)                                           \
G_STMT_START {                                                          \
    const char *s__ = moo_prefs_get_string (setting);                   \
    gtk_entry_set_text (xml->wid, MOO_NZS (s__));                       \
} G_STMT_END

#define GET_TEXT(wid,setting)                                           \
G_STMT_START {                                                          \
    const char *s__ = gtk_entry_get_text (xml->wid);                    \
    moo_prefs_set_string (setting, s__ && s__[0] ? s__ : NULL);         \
} G_STMT_END

#define SET_BOOL(wid,setting)                                           \
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (xml->wid),         \
                                  moo_prefs_get_bool (setting))

#define GET_BOOL(wid,setting)                                           \
    moo_prefs_set_bool (setting,                                        \
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (xml->wid)))


static void
moo_print_init_prefs (void)
{
    moo_prefs_new_key_bool (PREFS_PRINT_HEADER, TRUE);
    moo_prefs_new_key_bool (PREFS_PRINT_FOOTER, TRUE);
    moo_prefs_new_key_bool (PREFS_PRINT_HEADER_SEPARATOR, TRUE);
    moo_prefs_new_key_bool (PREFS_PRINT_FOOTER_SEPARATOR, TRUE);
    moo_prefs_new_key_string (PREFS_HEADER_LEFT, "%Ef");
    moo_prefs_new_key_string (PREFS_HEADER_CENTER, NULL);
    moo_prefs_new_key_string (PREFS_HEADER_RIGHT, "%x %X");
    moo_prefs_new_key_string (PREFS_FOOTER_LEFT, NULL);
    moo_prefs_new_key_string (PREFS_FOOTER_CENTER, "Page %Ep of %EP");
    moo_prefs_new_key_string (PREFS_FOOTER_RIGHT, NULL);

    moo_prefs_new_key_string (PREFS_FONT, NULL);
    moo_prefs_new_key_bool (PREFS_USE_STYLES, TRUE);
    moo_prefs_new_key_bool (PREFS_USE_CUSTOM_FONT, FALSE);
    moo_prefs_new_key_bool (PREFS_WRAP, TRUE);
    moo_prefs_new_key_bool (PREFS_ELLIPSIZE, FALSE);
    moo_prefs_new_key_bool (PREFS_LINE_NUMBERS, FALSE);
    moo_prefs_new_key_int (PREFS_LINE_NUMBERS_STEP, 1);
}


static void
set_options (PrintWidgetXml *xml)
{
    const char *s;

    g_return_if_fail (xml != NULL);
    moo_print_init_prefs ();

    SET_BOOL (print_header, PREFS_PRINT_HEADER);
    SET_BOOL (header_separator, PREFS_PRINT_HEADER_SEPARATOR);

    SET_TEXT (header_left, PREFS_HEADER_LEFT);
    SET_TEXT (header_center, PREFS_HEADER_CENTER);
    SET_TEXT (header_right, PREFS_HEADER_RIGHT);

    SET_BOOL (print_footer, PREFS_PRINT_FOOTER);
    SET_BOOL (footer_separator, PREFS_PRINT_FOOTER_SEPARATOR);

    SET_TEXT (footer_left, PREFS_FOOTER_LEFT);
    SET_TEXT (footer_center, PREFS_FOOTER_CENTER);
    SET_TEXT (footer_right, PREFS_FOOTER_RIGHT);

    SET_BOOL (use_styles, PREFS_USE_STYLES);
    SET_BOOL (use_custom_font, PREFS_USE_CUSTOM_FONT);
    SET_BOOL (wrap, PREFS_WRAP);
    SET_BOOL (ellipsize, PREFS_ELLIPSIZE);
    SET_BOOL (line_numbers, PREFS_LINE_NUMBERS);

    gtk_spin_button_set_value (xml->line_numbers_step,
                               moo_prefs_get_int (PREFS_LINE_NUMBERS_STEP));

    if ((s = moo_prefs_get_string (PREFS_FONT)))
        gtk_font_button_set_font_name (xml->font, s);
}


static void
get_options (PrintWidgetXml *xml)
{
    g_return_if_fail (xml != NULL);

    GET_BOOL (print_header, PREFS_PRINT_HEADER);
    GET_BOOL (header_separator, PREFS_PRINT_HEADER_SEPARATOR);
    GET_TEXT (header_left, PREFS_HEADER_LEFT);
    GET_TEXT (header_center, PREFS_HEADER_CENTER);
    GET_TEXT (header_right, PREFS_HEADER_RIGHT);

    GET_BOOL (print_footer, PREFS_PRINT_FOOTER);
    GET_BOOL (footer_separator, PREFS_PRINT_FOOTER_SEPARATOR);
    GET_TEXT (footer_left, PREFS_FOOTER_LEFT);
    GET_TEXT (footer_center, PREFS_FOOTER_CENTER);
    GET_TEXT (footer_right, PREFS_FOOTER_RIGHT);

    GET_BOOL (use_styles, PREFS_USE_STYLES);
    GET_BOOL (use_custom_font, PREFS_USE_CUSTOM_FONT);
    GET_BOOL (wrap, PREFS_WRAP);
    GET_BOOL (ellipsize, PREFS_ELLIPSIZE);
    GET_BOOL (line_numbers, PREFS_LINE_NUMBERS);

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (xml->line_numbers)))
    {
        GtkSpinButton *btn = xml->line_numbers_step;
        int step;
        gtk_spin_button_update (btn);
        step = gtk_spin_button_get_value_as_int (btn);
        if (step < 1)
        {
            step = 1;
            gtk_spin_button_set_value (btn, 1);
        }
        moo_prefs_set_int (PREFS_LINE_NUMBERS_STEP, step);
    }

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (xml->use_custom_font)))
        moo_prefs_set_string (PREFS_FONT, gtk_font_button_get_font_name (xml->font));
}


#if 0
void
_moo_edit_print_options_dialog (GtkWidget *parent)
{
    GtkWidget *dialog;
    PrintWidgetXml *xml;

    xml = moo_glade_xml_new_from_buf (MOO_PRINT_GLADE_XML, -1, NULL, GETTEXT_PACKAGE, NULL);
    g_return_if_fail (xml != NULL);

    dialog = moo_glade_xml_get_widget (xml, "dialog");
    g_return_if_fail (dialog != NULL);

    moo_window_set_parent (dialog, parent);

    set_options (xml);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
        get_options (xml);

    g_object_unref (xml);
    gtk_widget_destroy (dialog);
}
#endif


static GtkWidget *
moo_print_operation_create_custom_widget (G_GNUC_UNUSED GtkPrintOperation *operation)
{
    PrintWidgetXml *xml;

    xml = print_widget_xml_new ();

    moo_bind_sensitive (GTK_WIDGET (xml->wrap), GTK_WIDGET (xml->ellipsize), TRUE);
    moo_bind_sensitive (GTK_WIDGET (xml->print_header), GTK_WIDGET (xml->header_alignment), FALSE);
    moo_bind_sensitive (GTK_WIDGET (xml->print_footer), GTK_WIDGET (xml->footer_alignment), FALSE);
    moo_bind_sensitive (GTK_WIDGET (xml->use_custom_font), GTK_WIDGET (xml->font), FALSE);
    moo_bind_sensitive (GTK_WIDGET (xml->line_numbers), GTK_WIDGET (xml->line_numbers_hbox), FALSE);

    set_options (xml);
    return GTK_WIDGET (xml->PrintWidget);
}


static void
moo_print_operation_custom_widget_apply (G_GNUC_UNUSED GtkPrintOperation *print,
                                         GtkWidget *widget)
{
    get_options (print_widget_xml_get (widget));
}


/*****************************************************************************/
/* Format string
 */

typedef enum {
    HF_FORMAT_TIME,
    HF_FORMAT_PAGE,
    HF_FORMAT_TOTAL_PAGES,
    HF_FORMAT_FILENAME,
    HF_FORMAT_BASENAME
} HFFormatType;

typedef struct {
    HFFormatType type;
    char *string;
} HFFormatChunk;

struct HFFormat {
    GSList *chunks;
};


static HFFormatChunk *
hf_format_chunk_new (HFFormatType type,
                     const char  *string,
                     gssize       len)
{
    HFFormatChunk *chunk = g_slice_new0 (HFFormatChunk);

    chunk->type = type;

    if (string)
        chunk->string = g_strndup (string, len >= 0 ? (gsize) len : strlen (string));

    return chunk;
}


static void
hf_format_chunk_free (HFFormatChunk *chunk)
{
    if (chunk)
    {
        g_free (chunk->string);
        g_slice_free (HFFormatChunk, chunk);
    }
}


static void
hf_format_free (HFFormat *format)
{
    if (format)
    {
        g_slist_foreach (format->chunks, (GFunc) hf_format_chunk_free, NULL);
        g_slist_free (format->chunks);
        g_slice_free (HFFormat, format);
    }
}


#define ADD_CHUNK(format,type,string,len)                           \
    format->chunks =                                                \
        g_slist_prepend (format->chunks,                            \
                         hf_format_chunk_new (type, string, len))


static HFFormat *
hf_format_parse (const char *format_string)
{
    HFFormat *format;
    const char *p, *str;

    g_return_val_if_fail (format_string != NULL, NULL);

    format = g_slice_new0 (HFFormat);
    p = str = format_string;

    while (*p && (p = strchr (p, '%')))
    {
        switch (p[1])
        {
            case 'E':
                if (p != str)
                    ADD_CHUNK (format, HF_FORMAT_TIME, str, p - str);
                switch (p[2])
                {
                    case 0:
                        g_warning ("trailing '%%E' in %s",
                                   format_string);
                        goto error;
                    case 'f':
                        ADD_CHUNK (format, HF_FORMAT_BASENAME, NULL, 0);
                        str = p = p + 3;
                        break;
                    case 'F':
                        ADD_CHUNK (format, HF_FORMAT_FILENAME, NULL, 0);
                        str = p = p + 3;
                        break;
                    case 'p':
                        ADD_CHUNK (format, HF_FORMAT_PAGE, NULL, 0);
                        str = p = p + 3;
                        break;
                    case 'P':
                        ADD_CHUNK (format, HF_FORMAT_TOTAL_PAGES, NULL, 0);
                        str = p = p + 3;
                        break;
                    default:
                        g_warning ("unknown format '%%E%c' in %s",
                                   p[2], format_string);
                        goto error;
                }
                break;
            case 0:
                g_warning ("trailing '%%' in %s",
                           format_string);
                goto error;
            default:
                p = p + 2;
        }
    }

    if (*str)
        ADD_CHUNK (format, HF_FORMAT_TIME, str, -1);

    format->chunks = g_slist_reverse (format->chunks);
    return format;

error:
    hf_format_free (format);
    return NULL;
}


static void
eval_strftime (GString    *dest,
               const char *format,
               struct tm  *tm)
{
    gsize result;
    char buf[1024];
    char *retval;

    g_return_if_fail (format != NULL);
    g_return_if_fail (tm != NULL);

    if (!format[0])
        return;

    result = strftime (buf, 1024, format, tm);

    if (!result)
    {
        g_warning ("strftime failed");
        return;
    }

    retval = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);

    if (!retval)
    {
        g_warning ("could not convert result of strftime to UTF8");
        return;
    }

    g_string_append (dest, retval);
    g_free (retval);
}


static char *
hf_format_eval (HFFormat         *format,
                struct tm        *tm,
                int               page,
                int               total_pages,
                const char       *filename,
                const char       *basename)
{
    GString *string;
    GSList *l;

    g_return_val_if_fail (format != NULL, NULL);

    string = g_string_new (NULL);

    for (l = format->chunks; l != NULL; l = l->next)
    {
        HFFormatChunk *chunk = l->data;

        switch (chunk->type)
        {
            case HF_FORMAT_TIME:
                eval_strftime (string, chunk->string, tm);
                break;
            case HF_FORMAT_PAGE:
                g_string_append_printf (string, "%d", page + 1);
                break;
            case HF_FORMAT_TOTAL_PAGES:
                g_string_append_printf (string, "%d", total_pages);
                break;
            case HF_FORMAT_FILENAME:
                if (!filename)
                    g_critical ("filename is NULL");
                else
                    g_string_append (string, filename);
                break;
            case HF_FORMAT_BASENAME:
                if (!basename)
                    g_critical ("basename is NULL");
                else
                    g_string_append (string, basename);
                break;
        }
    }

    return g_string_free (string, FALSE);
}
