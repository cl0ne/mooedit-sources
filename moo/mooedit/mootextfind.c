/*
 *   mootextfind.c
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

#include "mooedit/mootextfind.h"
#include "mooedit/mootextview.h"
#include "mooedit/mooeditdialogs.h"
#include "mooedit/mootextsearch-private.h"
#include "mooedit/mooeditprefs.h"
#include "mooedit/mooedit-enums.h"
#include "mooutils/moohistorycombo.h"
#include "mooutils/mooentry.h"
#include "mooutils/moodialogs.h"
#include "mooutils/mooi18n.h"
#include "mooutils/moohelp.h"
#include "mooutils/moocompat.h"
#include "mooedit/mootextfind-gxml.h"
#include "mooedit/mootextgotoline-gxml.h"
#ifdef MOO_ENABLE_HELP
#include "moo-help-sections.h"
#endif
#include <gtk/gtk.h>
#include <glib/gprintf.h>


#define REGEX_FREE(re) G_STMT_START {   \
    if (re)                             \
        _moo_regex_unref (re);          \
    (re) = NULL;                        \
} G_STMT_END

static char *last_search;
static MooRegex *last_regex;
static MooFindFlags last_search_flags;
static MooHistoryList *search_history;
static MooHistoryList *replace_history;
static gboolean last_replace_empty;


static void         init_find_history       (void);

static GObject     *moo_find_constructor    (GType           type,
                                             guint           n_props,
                                             GObjectConstructParam *props);
static void         moo_find_set_property   (GObject        *object,
                                             guint           prop_id,
                                             const GValue   *value,
                                             GParamSpec     *pspec);
static void         moo_find_get_property   (GObject        *object,
                                             guint           prop_id,
                                             GValue         *value,
                                             GParamSpec     *pspec);
static void      moo_find_finalize          (GObject        *object);

static void      moo_find_set_flags         (MooFind        *find,
                                             MooFindFlags    flags);
static MooFindFlags moo_find_get_flags      (MooFind        *find);
/* returned string/regex must be freed/unrefed */
static char     *moo_find_get_text          (MooFind        *find);
static MooRegex *moo_find_get_regex         (MooFind        *find);
static char     *moo_find_get_replacement   (MooFind        *find);


G_DEFINE_TYPE(MooFind, moo_find, GTK_TYPE_DIALOG)


enum {
    PROP_0,
    PROP_REPLACE
};


static void
init_find_history (void)
{
    static gboolean been_here = FALSE;

    if (!been_here)
    {
        been_here = TRUE;

        search_history = moo_history_list_get ("MooFind");
        moo_history_list_set_max_entries (search_history, 20);
        replace_history = moo_history_list_get ("MooReplace");
        moo_history_list_set_max_entries (replace_history, 20);
        last_search = moo_history_list_get_last_item (search_history);

        moo_prefs_create_key (moo_edit_setting (MOO_EDIT_PREFS_SEARCH_FLAGS), MOO_PREFS_STATE,
                                                G_TYPE_INT, MOO_FIND_CASELESS);
        last_search_flags = moo_prefs_get_int (moo_edit_setting (MOO_EDIT_PREFS_SEARCH_FLAGS));
    }
}


static void
moo_find_class_init (MooFindClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->constructor = moo_find_constructor;
    gobject_class->set_property = moo_find_set_property;
    gobject_class->get_property = moo_find_get_property;
    gobject_class->finalize = moo_find_finalize;

    init_find_history ();

    g_object_class_install_property (gobject_class,
                                     PROP_REPLACE,
                                     g_param_spec_boolean ("replace",
                                             "replace",
                                             "replace",
                                             TRUE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
}


static void
moo_find_init (MooFind *find)
{
    MooCombo *search, *replace;

    find->xml = moo_find_box_xml_new ();
    g_return_if_fail (find->xml != NULL);

    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(find)->vbox),
                       GTK_WIDGET (find->xml->MooFindBox));
    gtk_dialog_set_has_separator (GTK_DIALOG (find), FALSE);

    search = MOO_COMBO (find->xml->search_entry);
    replace = MOO_COMBO (find->xml->replace_entry);
    moo_entry_set_use_special_chars_menu (MOO_ENTRY (search->entry), TRUE);
    moo_entry_set_use_special_chars_menu (MOO_ENTRY (replace->entry), TRUE);

    moo_history_combo_set_list (find->xml->search_entry, search_history);
    moo_history_combo_set_list (find->xml->replace_entry, replace_history);
}


static GObject*
moo_find_constructor (GType           type,
                      guint           n_props,
                      GObjectConstructParam *props)
{
    GObject *object;
    MooFind *find;
    gboolean use_replace;
    const char *stock_id, *title;

    object = G_OBJECT_CLASS (moo_find_parent_class)->constructor (type, n_props, props);
    find = MOO_FIND (object);

    if (find->replace)
    {
        use_replace = TRUE;
        stock_id = GTK_STOCK_FIND_AND_REPLACE;
        title = C_("Dialog title", "Replace");
    }
    else
    {
        use_replace = FALSE;
        stock_id = GTK_STOCK_FIND;
        title = C_("Dialog title", "Find");
    }

    gtk_widget_set_sensitive (GTK_WIDGET (find->xml->backwards), !use_replace);
    g_object_set (find->xml->replace_frame, "visible", use_replace, NULL);
    g_object_set (find->xml->dont_prompt, "visible", use_replace, NULL);

    gtk_window_set_title (GTK_WINDOW (find), title);
    gtk_dialog_add_buttons (GTK_DIALOG (find),
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            stock_id, GTK_RESPONSE_OK,
                            NULL);
    gtk_dialog_set_alternative_button_order (GTK_DIALOG (find),
                                             GTK_RESPONSE_OK,
                                             GTK_RESPONSE_CANCEL,
                                             -1);
    gtk_dialog_set_default_response (GTK_DIALOG (find), GTK_RESPONSE_OK);

#ifdef MOO_ENABLE_HELP
    moo_help_set_id (GTK_WIDGET (find), find->replace ?
                                HELP_SECTION_DIALOG_REPLACE : HELP_SECTION_DIALOG_FIND);
#endif
    moo_help_connect_keys (GTK_WIDGET (find));

    return object;
}


static void
moo_find_set_property (GObject        *object,
                       guint           prop_id,
                       const GValue   *value,
                       GParamSpec     *pspec)
{
    MooFind *find = MOO_FIND (object);

    switch (prop_id)
    {
        case PROP_REPLACE:
            find->replace = g_value_get_boolean (value) ? TRUE : FALSE;
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_find_get_property (GObject        *object,
                       guint           prop_id,
                       GValue         *value,
                       GParamSpec     *pspec)
{
    MooFind *find = MOO_FIND (object);

    switch (prop_id)
    {
        case PROP_REPLACE:
            g_value_set_boolean (value, find->replace ? TRUE : FALSE);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_find_finalize (GObject *object)
{
    MooFind *find = MOO_FIND (object);

    REGEX_FREE (find->regex);

    G_OBJECT_CLASS(moo_find_parent_class)->finalize (object);
}


static GtkWidget *
moo_find_new (gboolean replace)
{
    return GTK_WIDGET (g_object_new (MOO_TYPE_FIND, "replace", replace, (const char*) NULL));
}


static inline gboolean
is_word (char *p)
{
    return *p && (*p == '_' || g_unichar_isalnum (g_utf8_get_char (p)));
}

static char *
get_word_at_iter (GtkTextBuffer *buffer,
                  GtkTextIter   *start,
                  GtkTextIter   *end)
{
    char *word = NULL, *cursor, *line, *word_start = NULL, *word_end = NULL;
    int cursor_offset;

    cursor_offset = gtk_text_iter_get_line_offset (start);
    *end = *start;
    gtk_text_iter_set_line_offset (start, 0);
    if (!gtk_text_iter_ends_line (end))
        gtk_text_iter_forward_to_line_end (end);

    line = gtk_text_buffer_get_slice (buffer, start, end, TRUE);
    cursor = g_utf8_offset_to_pointer (line, cursor_offset);

    if (cursor > line)
    {
        char *p = g_utf8_prev_char (cursor);

        while (p >= line && is_word (p))
        {
            word_start = p;
            if (p == line)
                break;
            p = g_utf8_prev_char (p);
        }
    }

    for (word_end = cursor; is_word (word_end); word_end = g_utf8_next_char (word_end)) ;

    if (word_start)
    {
        word = g_strndup (word_start, word_end - word_start);
        gtk_text_iter_set_line_offset (start, g_utf8_pointer_to_offset (line, word_start));
        gtk_text_iter_set_line_offset (end, g_utf8_pointer_to_offset (line, word_end));
    }
    else if (word_end > cursor)
    {
        word = g_strndup (cursor, word_end - cursor);
        gtk_text_iter_set_line_offset (start, cursor_offset);
        gtk_text_iter_set_line_offset (end, g_utf8_pointer_to_offset (line, word_end));
    }
    else
    {
        gtk_text_iter_set_line_offset (start, cursor_offset);
        *end = *start;
    }

    g_free (line);
    return word;
}

static char *
get_search_term (GtkTextView *view,
                 gboolean     at_cursor,
                 GtkTextIter *start_p,
                 GtkTextIter *end_p)
{
    GtkTextBuffer *buffer;
    GtkTextIter start, end;

    buffer = gtk_text_view_get_buffer (view);

    if (gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
        if (gtk_text_iter_get_line (&start) == gtk_text_iter_get_line (&end))
        {
            if (start_p)
                *start_p = start;
            if (end_p)
                *end_p = end;
            return gtk_text_buffer_get_slice (buffer, &start, &end, TRUE);
        }
        else
        {
            return NULL;
        }
    }

    if (at_cursor)
    {
        char *word = get_word_at_iter (buffer, &start, &end);

        if (word)
        {
            if (start_p)
                *start_p = start;
            if (end_p)
                *end_p = end;
        }

        return word;
    }

    return NULL;
}


static void
moo_find_setup (MooFind        *find,
                GtkTextView    *view)
{
    GtkTextBuffer *buffer;
    GtkTextIter sel_start, sel_end;
    char *search_term;
    gboolean has_selection;

    g_return_if_fail (MOO_IS_FIND (find));
    g_return_if_fail (GTK_IS_TEXT_VIEW (view));

    moo_window_set_parent (GTK_WIDGET (find), GTK_WIDGET (view));

    buffer = gtk_text_view_get_buffer (view);

    search_term = get_search_term (view, TRUE, NULL, NULL);

    if (search_term && *search_term)
        gtk_entry_set_text (GTK_ENTRY (MOO_COMBO (find->xml->search_entry)->entry),
                            search_term);
    else if (last_search)
        gtk_entry_set_text (GTK_ENTRY (MOO_COMBO (find->xml->search_entry)->entry),
                            last_search);

    if (find->replace)
    {
        const char *replace_with = "";
        char *freeme = NULL;

        if (!last_replace_empty)
            replace_with = freeme = moo_history_list_get_last_item (replace_history);

        if (replace_with)
            gtk_entry_set_text (GTK_ENTRY (MOO_COMBO (find->xml->replace_entry)->entry),
                                replace_with);

        g_free (freeme);
    }

    has_selection = gtk_text_buffer_get_selection_bounds (buffer, &sel_start, &sel_end);

    if (!has_selection)
        gtk_widget_set_sensitive (GTK_WIDGET (find->xml->selected), FALSE);

    if (find->replace && has_selection &&
        gtk_text_iter_get_line (&sel_start) != gtk_text_iter_get_line (&sel_end))
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find->xml->selected), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find->xml->selected), FALSE);

    moo_entry_clear_undo (MOO_ENTRY (MOO_COMBO (find->xml->search_entry)->entry));

    moo_find_set_flags (find, last_search_flags);

    g_free (search_term);
}


G_GNUC_PRINTF(3, 0) static void
print_messagev (MooFindMsgFunc func,
                gpointer       data,
                const char    *format,
                va_list        args)
{
    char *msg = NULL;
    g_vasprintf (&msg, format, args);
    g_return_if_fail (msg != NULL);
    func (msg, data);
    g_free (msg);
}

static void G_GNUC_PRINTF(3,4)
print_message (MooFindMsgFunc func,
               gpointer       data,
               const char    *format,
               ...)
{
    va_list args;

    if (func)
    {
        g_return_if_fail (format != NULL);
        va_start (args, format);
        print_messagev (func, data, format, args);
        va_end (args);
    }
}


static gboolean
moo_find_run (MooFind        *find,
              MooFindMsgFunc  msg_func,
              gpointer        data)
{
    g_return_val_if_fail (MOO_IS_FIND (find), FALSE);

    REGEX_FREE (find->regex);

    while (TRUE)
    {
        const char *search_for, *replace_with;
        MooFindFlags flags;

        if (gtk_dialog_run (GTK_DIALOG (find)) != GTK_RESPONSE_OK)
            return FALSE;

        search_for = moo_combo_entry_get_text (MOO_COMBO (find->xml->search_entry));
        replace_with = moo_combo_entry_get_text (MOO_COMBO (find->xml->replace_entry));

        if (!search_for[0])
        {
            print_message (msg_func, data, _("No search term entered"));
            return FALSE;
        }

        flags = moo_find_get_flags (find);

        if (flags & MOO_FIND_REGEX)
        {
            MooRegex *regex;
            GRegexCompileFlags re_flags = 0;
            GError *error = NULL;

            if (flags & MOO_FIND_CASELESS)
                re_flags |= G_REGEX_CASELESS;

            regex = _moo_regex_compile (search_for, re_flags | G_REGEX_OPTIMIZE, 0, &error);

            if (!regex)
            {
                _moo_text_regex_error_dialog (GTK_WIDGET (find), error);
                g_error_free (error);
                continue;
            }

            find->regex = regex;
        }

        if (find->replace && !(flags & MOO_FIND_REPL_LITERAL))
        {
            GError *error = NULL;

            if (!g_regex_check_replacement (replace_with, NULL, &error))
            {
                _moo_text_regex_error_dialog (GTK_WIDGET (find), error);
                g_error_free (error);
                REGEX_FREE (find->regex);
                continue;
            }
        }

        last_search_flags = flags;
        moo_prefs_set_int (moo_edit_setting (MOO_EDIT_PREFS_SEARCH_FLAGS), flags);
        g_free (last_search);
        last_search = g_strdup (search_for);
        REGEX_FREE (last_regex);
        last_regex = find->regex ? _moo_regex_ref (find->regex) : NULL;

        moo_history_list_add (search_history, search_for);

        if (find->replace)
        {
            if (replace_with[0])
                moo_history_list_add (replace_history, replace_with);
            last_replace_empty = !replace_with[0];
        }

        return TRUE;
    }
}


static MooFindFlags
moo_find_get_flags (MooFind *find)
{
    MooFindFlags flags = 0;

    g_return_val_if_fail (MOO_IS_FIND (find), 0);

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find->xml->regex)))
        flags |= MOO_FIND_REGEX;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find->xml->repl_literal)))
        flags |= MOO_FIND_REPL_LITERAL;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find->xml->whole_words)))
        flags |= MOO_FIND_WHOLE_WORDS;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find->xml->from_cursor)))
        flags |= MOO_FIND_FROM_CURSOR;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find->xml->backwards)))
        flags |= MOO_FIND_BACKWARDS;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find->xml->selected)))
        flags |= MOO_FIND_IN_SELECTED;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find->xml->dont_prompt)))
        flags |= MOO_FIND_DONT_PROMPT;

    if (!(flags & MOO_FIND_REGEX))
        flags |= MOO_FIND_REPL_LITERAL;

    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (find->xml->case_sensitive)))
        flags |= MOO_FIND_CASELESS;

    return flags;
}


static void
moo_find_set_flags (MooFind        *find,
                    MooFindFlags    flags)
{
    g_return_if_fail (MOO_IS_FIND (find));

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find->xml->regex),
                                  (flags & MOO_FIND_REGEX) ? TRUE : FALSE);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find->xml->repl_literal),
                                  (flags & MOO_FIND_REPL_LITERAL) && (flags & MOO_FIND_REGEX));

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find->xml->whole_words),
                                  (flags & MOO_FIND_WHOLE_WORDS) ? TRUE : FALSE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find->xml->from_cursor),
                                  (flags & MOO_FIND_FROM_CURSOR) ? TRUE : FALSE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find->xml->backwards),
                                  (flags & MOO_FIND_BACKWARDS) ? TRUE : FALSE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find->xml->dont_prompt),
                                  (flags & MOO_FIND_DONT_PROMPT) ? TRUE : FALSE);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (find->xml->case_sensitive),
                                  (flags & MOO_FIND_CASELESS) ? FALSE : TRUE);
}


static char *
moo_find_get_text (MooFind *find)
{
    g_return_val_if_fail (MOO_IS_FIND (find), NULL);
    return g_strdup (moo_combo_entry_get_text (MOO_COMBO (find->xml->search_entry)));
}


static MooRegex *
moo_find_get_regex (MooFind *find)
{
    g_return_val_if_fail (MOO_IS_FIND (find), NULL);
    return find->regex ? _moo_regex_ref (find->regex) : NULL;
}


static char *
moo_find_get_replacement (MooFind *find)
{
    g_return_val_if_fail (MOO_IS_FIND (find), NULL);
    return g_strdup (moo_combo_entry_get_text (MOO_COMBO (find->xml->replace_entry)));
}


static gboolean
do_find (const GtkTextIter *start,
         const GtkTextIter *end,
         MooFindFlags       flags,
         MooRegex          *regex,
         const char        *text,
         GtkTextIter       *match_start,
         GtkTextIter       *match_end)
{
    if (regex)
    {
        if (flags & MOO_FIND_BACKWARDS)
            return _moo_text_search_regex_backward (start, end, regex,
                                                    match_start, match_end,
                                                    NULL, NULL, NULL, NULL);
        else
            return _moo_text_search_regex_forward (start, end, regex,
                                                   match_start, match_end,
                                                   NULL, NULL, NULL, NULL);
    }
    else
    {
        MooTextSearchFlags search_flags = 0;

        if (flags & MOO_FIND_CASELESS)
            search_flags |= MOO_TEXT_SEARCH_CASELESS;
        if (flags & MOO_FIND_WHOLE_WORDS)
            search_flags |= MOO_TEXT_SEARCH_WHOLE_WORDS;

        if (flags & MOO_FIND_BACKWARDS)
            return moo_text_search_backward (start, text, search_flags,
                                             match_start, match_end, end);
        else
            return moo_text_search_forward (start, text, search_flags,
                                            match_start, match_end, end);
    }
}


static void
get_search_bounds (GtkTextBuffer *buffer,
                   MooFindFlags   flags,
                   GtkTextIter   *start,
                   GtkTextIter   *end)
{
    if (flags & MOO_FIND_IN_SELECTED)
    {
        gtk_text_buffer_get_selection_bounds (buffer, start, end);
    }
    else if (flags & MOO_FIND_FROM_CURSOR)
    {
        gtk_text_buffer_get_iter_at_mark (buffer, start,
                                          gtk_text_buffer_get_insert (buffer));
        if (flags & MOO_FIND_BACKWARDS)
            gtk_text_buffer_get_start_iter (buffer, end);
        else
            gtk_text_buffer_get_end_iter (buffer, end);
    }
    else if (flags & MOO_FIND_BACKWARDS)
    {
        gtk_text_buffer_get_end_iter (buffer, start);
        gtk_text_buffer_get_start_iter (buffer, end);
    }
    else
    {
        gtk_text_buffer_get_start_iter (buffer, start);
        gtk_text_buffer_get_end_iter (buffer, end);
    }
}

static gboolean
get_search_bounds2 (GtkTextBuffer *buffer,
                    MooFindFlags   flags,
                    GtkTextIter   *start,
                    GtkTextIter   *end)
{
    if (flags & MOO_FIND_IN_SELECTED)
    {
        return FALSE;
    }
    else if (flags & MOO_FIND_FROM_CURSOR)
    {
        gtk_text_buffer_get_iter_at_mark (buffer, end,
                                          gtk_text_buffer_get_insert (buffer));

        if (flags & MOO_FIND_BACKWARDS)
            gtk_text_buffer_get_end_iter (buffer, start);
        else
            gtk_text_buffer_get_start_iter (buffer, start);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


static void
scroll_to_found (GtkTextView *view)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);
    gtk_text_view_scroll_to_mark (view, gtk_text_buffer_get_insert (buffer),
                                  0.1, FALSE, 0, 0);
}


void
moo_text_view_run_find (GtkTextView    *view,
                        MooFindMsgFunc  msg_func,
                        gpointer        data)
{
    GtkWidget *find;
    MooFindFlags flags;
    char *text;
    MooRegex *regex;
    GtkTextIter start, end;
    GtkTextIter match_start, match_end;
    GtkTextBuffer *buffer;
    gboolean found;
    gboolean wrapped = FALSE;

    g_return_if_fail (GTK_IS_TEXT_VIEW (view));

    find = moo_find_new (FALSE);
    moo_find_setup (MOO_FIND (find), view);

    if (!moo_find_run (MOO_FIND (find), msg_func, data))
    {
        gtk_widget_destroy (find);
        return;
    }

    flags = moo_find_get_flags (MOO_FIND (find));
    text = moo_find_get_text (MOO_FIND (find));
    regex = moo_find_get_regex (MOO_FIND (find));

    gtk_widget_destroy (find);

    buffer = gtk_text_view_get_buffer (view);

    get_search_bounds (buffer, flags, &start, &end);

    found = do_find (&start, &end, flags, regex, text, &match_start, &match_end);

    if (!found && get_search_bounds2 (buffer, flags, &start, &end))
    {
        found = do_find (&start, &end, flags, regex, text, &match_start, &match_end);
        wrapped = TRUE;
    }

    if (found)
    {
        gtk_text_buffer_select_range (buffer, &match_end, &match_start);
        scroll_to_found (view);

        if (wrapped)
        {
            if (flags & MOO_FIND_BACKWARDS)
                print_message (msg_func, data, _("Search hit top, continuing at bottom"));
            else
                print_message (msg_func, data, _("Search hit bottom, continuing at top"));
        }
    }
    else
    {
        GtkTextIter insert;
        gtk_text_buffer_get_iter_at_mark (buffer, &insert,
                                          gtk_text_buffer_get_insert (buffer));
        gtk_text_buffer_place_cursor (buffer, &insert);
        print_message (msg_func, data, _("Pattern not found: %s"), text);
    }

    g_free (text);
    REGEX_FREE (regex);
}


void
moo_text_view_run_find_current_word (GtkTextView    *view,
                                     gboolean        forward,
                                     MooFindMsgFunc  msg_func,
                                     gpointer        data)
{
    GtkTextBuffer *buffer;
    GtkTextIter word_start, word_end;
    GtkTextIter start, end;
    GtkTextIter match_start, match_end;
    gboolean found;
    gboolean wrapped = FALSE;
    char *search_term;
    MooFindFlags flags;

    g_return_if_fail (GTK_IS_TEXT_VIEW (view));

    init_find_history ();

    search_term = get_search_term (view, TRUE, &word_start, &word_end);

    if (!search_term)
    {
        moo_text_view_run_find (view, msg_func, data);
        return;
    }

    buffer = gtk_text_view_get_buffer (view);
    flags = last_search_flags & ~(MOO_FIND_REGEX | MOO_FIND_IN_SELECTED |
                                  MOO_FIND_BACKWARDS | MOO_FIND_FROM_CURSOR);

    g_free (last_search);
    last_search = search_term;
    REGEX_FREE (last_regex);
    moo_history_list_add (search_history, search_term);

    if (forward)
    {
        start = word_end;
        gtk_text_buffer_get_end_iter (buffer, &end);
    }
    else
    {
        start = word_start;
        gtk_text_buffer_get_start_iter (buffer, &end);
        flags |= MOO_FIND_BACKWARDS;
    }

    found = do_find (&start, &end, flags, NULL, search_term,
                      &match_start, &match_end);

    if (!found)
    {
        wrapped = TRUE;
        end = start;

        if (forward)
            gtk_text_buffer_get_start_iter (buffer, &start);
        else
            gtk_text_buffer_get_end_iter (buffer, &start);

        found = do_find (&start, &end, flags, NULL, search_term,
                          &match_start, &match_end);
    }

    if (found)
    {
        gtk_text_buffer_select_range (buffer, &match_end, &match_start);
        scroll_to_found (view);

        if (wrapped)
        {
            gtk_text_iter_order (&word_start, &word_end);
            gtk_text_iter_order (&match_start, &match_end);

            if (gtk_text_iter_equal (&word_start, &match_start) &&
                gtk_text_iter_equal (&word_end, &match_end))
            {
                print_message (msg_func, data,
                               _("Found single instance of pattern '%s'"),
                               last_search);
            }
            else
            {
                if (forward)
                    print_message (msg_func, data, _("Search hit bottom, continuing at top"));
                else
                    print_message (msg_func, data, _("Search hit top, continuing at bottom"));
            }
        }
    }
    else
    {
        GtkTextIter insert;
        gtk_text_buffer_get_iter_at_mark (buffer, &insert,
                                          gtk_text_buffer_get_insert (buffer));
        gtk_text_buffer_place_cursor (buffer, &insert);
        print_message (msg_func, data, _("Pattern not found: %s"), last_search);
    }
}


void
moo_text_view_run_find_next (GtkTextView    *view,
                             MooFindMsgFunc  msg_func,
                             gpointer        data)
{
    GtkTextBuffer *buffer;
    GtkTextIter sel_start, sel_end;
    GtkTextIter start, end;
    GtkTextIter match_start, match_end;
    gboolean found;
    gboolean has_selection;
    gboolean wrapped = FALSE;

    g_return_if_fail (GTK_IS_TEXT_VIEW (view));

    init_find_history ();

    if (!last_search)
    {
        moo_text_view_run_find (view, msg_func, data);
        return;
    }

    buffer = gtk_text_view_get_buffer (view);

    has_selection = gtk_text_buffer_get_selection_bounds (buffer, &sel_start, &sel_end);

    start = sel_end;
    gtk_text_buffer_get_end_iter (buffer, &end);

    found = do_find (&start, &end, last_search_flags & ~MOO_FIND_BACKWARDS,
                     last_regex, last_search, &match_start, &match_end);

    if (found && !has_selection &&
        gtk_text_iter_equal (&match_start, &match_end) &&
        gtk_text_iter_equal (&match_start, &start))
    {
        if (!gtk_text_iter_forward_char (&start))
        {
            found = FALSE;
        }
        else
        {
            found = do_find (&start, &end, last_search_flags & ~MOO_FIND_BACKWARDS,
                              last_regex, last_search, &match_start, &match_end);;
        }
    }

    if (!found && !gtk_text_iter_is_start (&sel_start))
    {
        wrapped = TRUE;
        gtk_text_buffer_get_start_iter (buffer, &start);
        end = sel_start;
        found = do_find (&start, &end, last_search_flags & ~MOO_FIND_BACKWARDS,
                          last_regex, last_search, &match_start, &match_end);
    }

    if (found)
    {
        gtk_text_buffer_select_range (buffer, &match_end, &match_start);
        scroll_to_found (view);
        if (wrapped)
            print_message (msg_func, data,
                           _("Search hit bottom, continuing at top"));
    }
    else
    {
        GtkTextIter insert;
        gtk_text_buffer_get_iter_at_mark (buffer, &insert,
                                          gtk_text_buffer_get_insert (buffer));
        gtk_text_buffer_place_cursor (buffer, &insert);
        print_message (msg_func, data, _("Pattern not found: %s"), last_search);
    }
}


void
moo_text_view_run_find_prev (GtkTextView    *view,
                             MooFindMsgFunc  msg_func,
                             gpointer        data)
{
    GtkTextBuffer *buffer;
    GtkTextIter sel_start, sel_end;
    GtkTextIter start, end;
    GtkTextIter match_start, match_end;
    gboolean found;
    gboolean has_selection;
    gboolean wrapped = FALSE;

    g_return_if_fail (GTK_IS_TEXT_VIEW (view));

    init_find_history ();

    if (!last_search)
    {
        moo_text_view_run_find (view, msg_func, data);
        return;
    }

    buffer = gtk_text_view_get_buffer (view);

    has_selection = gtk_text_buffer_get_selection_bounds (buffer, &sel_start, &sel_end);

    start = sel_start;
    gtk_text_buffer_get_start_iter (buffer, &end);

    found = do_find (&start, &end, last_search_flags | MOO_FIND_BACKWARDS,
                     last_regex, last_search, &match_start, &match_end);

    if (found && !has_selection &&
        gtk_text_iter_equal (&match_start, &match_end) &&
        gtk_text_iter_equal (&match_start, &start))
    {
        if (!gtk_text_iter_backward_char (&start))
        {
            found = FALSE;
        }
        else
        {
            found = do_find (&start, &end, last_search_flags | MOO_FIND_BACKWARDS,
                             last_regex, last_search, &match_start, &match_end);;
        }
    }

    if (!found && !gtk_text_iter_is_end (&sel_end))
    {
        gtk_text_buffer_get_end_iter (buffer, &start);
        end = sel_end;
        found = do_find (&start, &end, last_search_flags | MOO_FIND_BACKWARDS,
                         last_regex, last_search, &match_start, &match_end);
        wrapped = TRUE;
    }

    if (found)
    {
        gtk_text_buffer_select_range (buffer, &match_start, &match_end);
        scroll_to_found (view);

        if (wrapped)
            print_message (msg_func, data,
                           _("Search hit top, continuing at bottom"));
    }
    else
    {
        GtkTextIter insert;
        gtk_text_buffer_get_iter_at_mark (buffer, &insert,
                                          gtk_text_buffer_get_insert (buffer));
        gtk_text_buffer_place_cursor (buffer, &insert);
        print_message (msg_func, data, _("Pattern not found: %s"), last_search);
    }
}


static int
do_replace_silent (GtkTextIter       *start,
                   GtkTextIter       *end,
                   MooFindFlags       flags,
                   const char        *text,
                   MooRegex          *regex,
                   const char        *replacement)
{
    MooTextSearchFlags search_flags = 0;

    if (flags & MOO_FIND_CASELESS)
        search_flags |= MOO_TEXT_SEARCH_CASELESS;
    if (flags & MOO_FIND_WHOLE_WORDS)
        search_flags |= MOO_TEXT_SEARCH_WHOLE_WORDS;

    if (regex)
        return _moo_text_replace_regex_all (start, end, regex, replacement,
                                            flags & MOO_FIND_REPL_LITERAL);
    else
        return moo_text_replace_all (start, end, text, replacement, search_flags);
}


static void
replaced_n_message (guint          n,
                    MooFindMsgFunc msg_func,
                    gpointer       data)
{
    if (!n)
    {
        print_message (msg_func, data, _("No replacement made"));
    }
    else
    {
        print_message (msg_func, data,
                       dngettext (GETTEXT_PACKAGE,
                                  "%u replacement made",
                                  "%u replacements made",
                                  n),
                       n);
    }
}


static void
run_replace_silent (GtkTextView   *view,
                    const char    *text,
                    MooRegex      *regex,
                    const char    *replacement,
                    MooFindFlags   flags,
                    MooFindMsgFunc msg_func,
                    gpointer       data)
{
    GtkTextIter start, end;
    GtkTextBuffer *buffer;
    int replaced;

    buffer = gtk_text_view_get_buffer (view);
    get_search_bounds (buffer, flags, &start, &end);

    replaced = do_replace_silent (&start, &end, flags, text, regex, replacement);

    if (get_search_bounds2 (buffer, flags, &start, &end) &&
        _moo_text_search_from_start_dialog (GTK_WIDGET (view), replaced))
    {
        int replaced2 = do_replace_silent (&start, &end, flags, text, regex, replacement);
        replaced_n_message (replaced2, msg_func, data);
    }
    else
    {
        replaced_n_message (replaced, msg_func, data);
    }
}


static MooTextReplaceResponse
replace_func (G_GNUC_UNUSED const char *text,
              G_GNUC_UNUSED MooRegex *regex,
              G_GNUC_UNUSED const char *replacement,
              const GtkTextIter  *to_replace_start,
              const GtkTextIter  *to_replace_end,
              gpointer            user_data)
{
    GtkTextBuffer *buffer;
    int response;

    struct {
        GtkTextView *view;
        GtkWidget *dialog;
        MooTextReplaceResponse response;
    } *data = user_data;

    buffer = gtk_text_view_get_buffer (data->view);
    gtk_text_buffer_select_range (buffer, to_replace_end, to_replace_start);
    scroll_to_found (data->view);

    if (!data->dialog)
        data->dialog = _moo_text_prompt_on_replace_dialog (GTK_WIDGET (data->view));

    response = gtk_dialog_run (GTK_DIALOG (data->dialog));

    switch (response)
    {
        case MOO_TEXT_REPLACE_SKIP:
        case MOO_TEXT_REPLACE_DO_REPLACE:
        case MOO_TEXT_REPLACE_ALL:
            data->response = response;
            break;

        default:
            data->response = MOO_TEXT_REPLACE_STOP;
            break;
    }

    return data->response;
}


static MooTextReplaceResponse
do_replace_interactive (GtkTextView       *view,
                        GtkTextIter       *start,
                        GtkTextIter       *end,
                        MooFindFlags       flags,
                        const char        *text,
                        MooRegex          *regex,
                        const char        *replacement,
                        int               *replaced)
{
    MooTextSearchFlags search_flags = 0;

    struct {
        GtkTextView *view;
        GtkWidget *dialog;
        MooTextReplaceResponse response;
    } data;

    if (flags & MOO_FIND_CASELESS)
        search_flags |= MOO_TEXT_SEARCH_CASELESS;
    if (flags & MOO_FIND_WHOLE_WORDS)
        search_flags |= MOO_TEXT_SEARCH_WHOLE_WORDS;

    data.view = view;
    data.dialog = NULL;
    data.response = MOO_TEXT_REPLACE_DO_REPLACE;

    if (regex)
        *replaced = _moo_text_replace_regex_all_interactive (start, end, regex, replacement,
                                                             flags & MOO_FIND_REPL_LITERAL,
                                                             replace_func, &data);
    else
        *replaced = _moo_text_replace_all_interactive (start, end, text, replacement,
                                                       search_flags, replace_func, &data);

    if (data.dialog)
        gtk_widget_destroy (data.dialog);

    return data.response;
}


static void
run_replace_interactive (GtkTextView   *view,
                         const char    *text,
                         MooRegex      *regex,
                         const char    *replacement,
                         MooFindFlags   flags,
                         MooFindMsgFunc msg_func,
                         gpointer       data)
{
    GtkTextIter start, end;
    GtkTextBuffer *buffer;
    MooTextReplaceResponse result;
    int replaced;

    buffer = gtk_text_view_get_buffer (view);
    get_search_bounds (buffer, flags, &start, &end);

    result = do_replace_interactive (view, &start, &end, flags, text, regex, replacement, &replaced);

    if (result != MOO_TEXT_REPLACE_STOP && get_search_bounds2 (buffer, flags, &start, &end))
    {
        int replaced2 = replaced;

        if (_moo_text_search_from_start_dialog (GTK_WIDGET (view), replaced))
            do_replace_interactive (view, &start, &end, flags, text, regex, replacement, &replaced2);

        replaced_n_message (replaced2, msg_func, data);
    }
    else
    {
        replaced_n_message (replaced, msg_func, data);
    }
}


void
moo_text_view_run_replace (GtkTextView    *view,
                           MooFindMsgFunc  msg_func,
                           gpointer        data)
{
    GtkWidget *find;
    MooFindFlags flags;
    char *text, *replacement;
    MooRegex *regex;

    g_return_if_fail (GTK_IS_TEXT_VIEW (view));

    find = moo_find_new (TRUE);
    moo_find_setup (MOO_FIND (find), view);

    if (!moo_find_run (MOO_FIND (find), msg_func, data))
    {
        gtk_widget_destroy (find);
        return;
    }

    flags = moo_find_get_flags (MOO_FIND (find));
    text = moo_find_get_text (MOO_FIND (find));
    regex = moo_find_get_regex (MOO_FIND (find));
    replacement = moo_find_get_replacement (MOO_FIND (find));
    gtk_widget_destroy (find);

    if (flags & MOO_FIND_DONT_PROMPT)
        run_replace_silent (view, text, regex, replacement,
                            flags, msg_func, data);
    else
        run_replace_interactive (view, text, regex, replacement,
                                 flags, msg_func, data);

    g_free (text);
    g_free (replacement);
    REGEX_FREE (regex);
}


/****************************************************************************/
/* goto line
 */

static void     update_spin_value   (GtkRange       *scale,
                                     GtkSpinButton  *spin);
static gboolean update_scale_value  (GtkSpinButton  *spin,
                                     GtkRange       *scale);

static void
update_spin_value (GtkRange       *scale,
                   GtkSpinButton  *spin)
{
    double value = gtk_range_get_value (scale);
    g_signal_handlers_block_matched (spin, G_SIGNAL_MATCH_FUNC, 0, 0, 0,
                                     (gpointer)update_scale_value, 0);
    gtk_spin_button_set_value (spin, value);
    g_signal_handlers_unblock_matched (spin, G_SIGNAL_MATCH_FUNC, 0, 0, 0,
                                       (gpointer)update_scale_value, 0);
}

static gboolean
update_scale_value (GtkSpinButton  *spin,
                    GtkRange       *scale)
{
    double value = gtk_spin_button_get_value (spin);
    g_signal_handlers_block_matched (scale, G_SIGNAL_MATCH_FUNC, 0, 0, 0,
                                     (gpointer)update_spin_value, 0);
    gtk_range_set_value (scale, value);
    g_signal_handlers_unblock_matched (scale, G_SIGNAL_MATCH_FUNC, 0, 0, 0,
                                       (gpointer)update_spin_value, 0);
    return FALSE;
}


void
moo_text_view_run_goto_line (GtkTextView *view)
{
    GtkDialog *dialog;
    GtkTextBuffer *buffer;
    int line_count, line;
    GtkTextIter iter;
    GotoLineDialogXml *xml;

    g_return_if_fail (GTK_IS_TEXT_VIEW (view));

    buffer = gtk_text_view_get_buffer (view);
    line_count = gtk_text_buffer_get_line_count (buffer);

    xml = goto_line_dialog_xml_new ();
    dialog = xml->GotoLineDialog;

    gtk_dialog_set_alternative_button_order (dialog,
                                             GTK_RESPONSE_OK,
                                             GTK_RESPONSE_CANCEL,
                                             -1);

    gtk_text_buffer_get_iter_at_mark (buffer, &iter,
                                      gtk_text_buffer_get_insert (buffer));
    line = gtk_text_iter_get_line (&iter);

    gtk_range_set_range (GTK_RANGE (xml->scale), 1, line_count + 1);
    gtk_range_set_value (GTK_RANGE (xml->scale), line + 1);

    gtk_entry_set_activates_default (GTK_ENTRY (xml->spin), TRUE);
    gtk_spin_button_set_range (xml->spin, 1, line_count);
    gtk_spin_button_set_value (xml->spin, line + 1);
    gtk_editable_select_region (GTK_EDITABLE (xml->spin), 0, -1);

    g_signal_connect (xml->scale, "value-changed", G_CALLBACK (update_spin_value), xml->spin);
    g_signal_connect (xml->spin, "value-changed", G_CALLBACK (update_scale_value), xml->scale);

    moo_window_set_parent (GTK_WIDGET (dialog), GTK_WIDGET (view));

    if (gtk_dialog_run (dialog) != GTK_RESPONSE_OK)
    {
        gtk_widget_destroy (GTK_WIDGET (dialog));
        return;
    }

    gtk_spin_button_update (xml->spin);
    line = gtk_spin_button_get_value (xml->spin) - 1;
    gtk_widget_destroy (GTK_WIDGET (dialog));

    if (MOO_IS_TEXT_VIEW (view))
    {
        moo_text_view_move_cursor (MOO_TEXT_VIEW (view), line, 0, FALSE, FALSE);
    }
    else
    {
        gtk_text_buffer_get_iter_at_line (buffer, &iter, line);
        gtk_text_buffer_place_cursor (buffer, &iter);
        scroll_to_found (view);
    }
}
