/*
 *   moolinemark.c
 *
 *   Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
 *   Copyright (C) 2008 by Daniel Poelzleithner <mooedit@poelzi.org>
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

/**
 * class:MooLineMark: (parent GObject) (constructable) (moo.private 1)
 **/

#include "mooedit/mootext-private.h"
#include "mooedit/mootextbuffer.h"
#include "marshals.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/moocompat.h"


struct MooLineMarkPrivate {
    GdkColor background;

    char *stock_id;
    GdkPixbuf *pixbuf;

    GtkWidget *widget;

    char *markup;

    MooTextBuffer *buffer;
    LineBuffer *line_buf;
    Line *line;
    int line_no;
    guint stamp;

    MooFold *fold;

    guint background_set : 1;
    guint visible : 1;
    guint realized : 1;
    guint pretty : 1;
};


static void     moo_line_mark_finalize      (GObject        *object);

static void     moo_line_mark_set_property  (GObject        *object,
                                             guint           prop_id,
                                             const GValue   *value,
                                             GParamSpec     *pspec);
static void     moo_line_mark_get_property  (GObject        *object,
                                             guint           prop_id,
                                             GValue         *value,
                                             GParamSpec     *pspec);

static void     moo_line_mark_changed       (MooLineMark    *mark);
static void     update_pixbuf               (MooLineMark    *mark);

static void     moo_line_mark_deleted_real  (MooLineMark    *mark);


enum {
    CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


enum {
    PROP_0,
    PROP_BACKGROUND,
    PROP_BACKGROUND_GDK,
    PROP_BACKGROUND_SET,
    PROP_PIXBUF,
    PROP_STOCK_ID,
    PROP_MARKUP,
    PROP_VISIBLE,
    PROP_FOLD
};


/* MOO_TYPE_LINE_MARK */
G_DEFINE_TYPE (MooLineMark, moo_line_mark, G_TYPE_OBJECT)


static void
moo_line_mark_class_init (MooLineMarkClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = moo_line_mark_set_property;
    gobject_class->get_property = moo_line_mark_get_property;
    gobject_class->finalize = moo_line_mark_finalize;

    klass->deleted = moo_line_mark_deleted_real;

    g_object_class_install_property (gobject_class,
                                     PROP_BACKGROUND,
                                     g_param_spec_string ("background",
                                             "background",
                                             "background",
                                             NULL,
                                             G_PARAM_WRITABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_BACKGROUND_GDK,
                                     g_param_spec_boxed ("background-gdk",
                                             "background-gdk",
                                             "background-gdk",
                                             GDK_TYPE_COLOR,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_BACKGROUND_SET,
                                     g_param_spec_boolean ("background-set",
                                             "background-set",
                                             "background-set",
                                             FALSE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_MARKUP,
                                     g_param_spec_string ("markup",
                                             "markup",
                                             "markup",
                                             NULL,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_VISIBLE,
                                     g_param_spec_boolean ("visible",
                                             "visible",
                                             "visible",
                                             FALSE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_PIXBUF,
                                     g_param_spec_object ("pixbuf",
                                             "pixbuf",
                                             "pixbuf",
                                             GDK_TYPE_PIXBUF,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_STOCK_ID,
                                     g_param_spec_string ("stock-id",
                                             "stock-id",
                                             "stock-id",
                                             NULL,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_FOLD,
                                     g_param_spec_object ("fold",
                                             "fold",
                                             "fold",
                                             MOO_TYPE_FOLD,
                                             G_PARAM_READABLE));

    signals[CHANGED] =
            g_signal_new ("changed",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooLineMarkClass, changed),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);
}


static void
moo_line_mark_init (MooLineMark *mark)
{
    mark->priv = g_new0 (MooLineMarkPrivate, 1);
    mark->priv->line_no = -1;
    gdk_color_parse ("0xFFF", &mark->priv->background);
}


static void
moo_line_mark_finalize (GObject *object)
{
    MooLineMark *mark = MOO_LINE_MARK (object);

    if (mark->priv->pixbuf)
        g_object_unref (mark->priv->pixbuf);

    g_free (mark->priv->stock_id);
    g_free (mark->priv->markup);
    g_free (mark->priv);

    G_OBJECT_CLASS (moo_line_mark_parent_class)->finalize (object);
}


static void
moo_line_mark_set_property (GObject        *object,
                            guint           prop_id,
                            const GValue   *value,
                            GParamSpec     *pspec)
{
    MooLineMark *mark = MOO_LINE_MARK (object);

    switch (prop_id)
    {
        case PROP_BACKGROUND_GDK:
            moo_line_mark_set_background_gdk (mark, g_value_get_boxed (value));
            break;

        case PROP_BACKGROUND:
            moo_line_mark_set_background (mark, g_value_get_string (value));
            break;

        case PROP_BACKGROUND_SET:
            mark->priv->background_set = g_value_get_boolean (value) != 0;
            g_object_notify (object, "background-set");
            moo_line_mark_changed (mark);
            break;

        case PROP_MARKUP:
            moo_line_mark_set_markup (mark, g_value_get_string (value));
            break;

        case PROP_VISIBLE:
            mark->priv->visible = g_value_get_boolean (value) != 0;
            g_object_notify (object, "visible");
            break;

        case PROP_PIXBUF:
            moo_line_mark_set_pixbuf (mark, g_value_get_object (value));
            break;

        case PROP_STOCK_ID:
            moo_line_mark_set_stock_id (mark, g_value_get_string (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_line_mark_get_property (GObject        *object,
                            guint           prop_id,
                            GValue         *value,
                            GParamSpec     *pspec)
{
    MooLineMark *mark = MOO_LINE_MARK (object);

    switch (prop_id)
    {
        case PROP_BACKGROUND_GDK:
            g_value_set_boxed (value, &mark->priv->background);
            break;

        case PROP_BACKGROUND_SET:
            g_value_set_boolean (value, mark->priv->background_set != 0);
            break;

        case PROP_MARKUP:
            g_value_set_string (value, mark->priv->markup);
            break;

        case PROP_VISIBLE:
            g_value_set_boolean (value, mark->priv->visible != 0);
            break;

        case PROP_PIXBUF:
            g_value_set_object (value, mark->priv->pixbuf);
            break;

        case PROP_STOCK_ID:
            g_value_set_string (value, mark->priv->stock_id);
            break;

        case PROP_FOLD:
            g_value_set_object (value, mark->priv->fold);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


void
moo_line_mark_set_background_gdk (MooLineMark    *mark,
                                  const GdkColor *color)
{
    gboolean changed = FALSE, notify_set = FALSE, notify_bg = FALSE;

    g_return_if_fail (MOO_IS_LINE_MARK (mark));

    if (color)
    {
        if (!mark->priv->background_set)
        {
            mark->priv->background_set = TRUE;
            notify_set = TRUE;
        }

        changed = TRUE;
        notify_bg = TRUE;

        mark->priv->background = *color;
    }
    else
    {
        if (mark->priv->background_set)
        {
            mark->priv->background_set = FALSE;
            notify_set = TRUE;
            notify_bg = TRUE;
            changed = TRUE;
        }
    }

    if (notify_set || notify_bg)
    {
        g_object_freeze_notify (G_OBJECT (mark));

        if (notify_set)
            g_object_notify (G_OBJECT (mark), "background-set");

        if (notify_bg)
        {
            g_object_notify (G_OBJECT (mark), "background");
            g_object_notify (G_OBJECT (mark), "background-gdk");
        }

        g_object_thaw_notify (G_OBJECT (mark));
    }

    if (changed)
        moo_line_mark_changed (mark);
}


void
moo_line_mark_set_background (MooLineMark    *mark,
                              const char     *color)
{
    GdkColor gdk_color;

    g_return_if_fail (MOO_IS_LINE_MARK (mark));

    if (color)
    {
        if (gdk_color_parse (color, &gdk_color))
            moo_line_mark_set_background_gdk (mark, &gdk_color);
        else
            g_warning ("could not parse color '%s'", color);
    }
    else
    {
        moo_line_mark_set_background_gdk (mark, NULL);
    }
}


static void
moo_line_mark_changed (MooLineMark *mark)
{
    g_signal_emit (mark, signals[CHANGED], 0);
}


void
moo_line_mark_set_markup (MooLineMark    *mark,
                          const char     *markup)
{
    g_return_if_fail (MOO_IS_LINE_MARK (mark));

    if (!markup || !markup[0])
        markup = NULL;

    MOO_ASSIGN_STRING (mark->priv->markup, markup);
}


const char *
moo_line_mark_get_markup (MooLineMark *mark)
{
    g_return_val_if_fail (MOO_IS_LINE_MARK (mark), NULL);
    return mark->priv->markup;
}


int
moo_line_mark_get_line (MooLineMark *mark)
{
    g_return_val_if_fail (MOO_IS_LINE_MARK (mark), -1);
    g_return_val_if_fail (mark->priv->line != NULL, -1);

    if (_moo_line_buffer_get_stamp (mark->priv->line_buf) != mark->priv->stamp)
    {
        mark->priv->line_no =
                _moo_line_buffer_get_line_index (mark->priv->line_buf, mark->priv->line);
        mark->priv->stamp = _moo_line_buffer_get_stamp (mark->priv->line_buf);
    }

    return mark->priv->line_no;
}


void
_moo_line_mark_set_line (MooLineMark    *mark,
                         Line           *line,
                         int             line_no,
                         guint           stamp)
{
    g_assert (MOO_IS_LINE_MARK (mark));
    g_assert (!line || mark->priv->buffer != NULL);

    mark->priv->line = line;
    mark->priv->line_no = line_no;
    mark->priv->stamp = stamp;
}


Line*
_moo_line_mark_get_line (MooLineMark *mark)
{
    g_assert (MOO_IS_LINE_MARK (mark));
    return mark->priv->line;
}


void
_moo_line_mark_set_buffer (MooLineMark    *mark,
                           MooTextBuffer  *buffer,
                           LineBuffer     *line_buf)
{
    g_assert (MOO_IS_LINE_MARK (mark));
    g_assert (!buffer || mark->priv->buffer == NULL);

    mark->priv->buffer = buffer;
    mark->priv->line_buf = line_buf;

    if (!buffer)
        _moo_line_mark_set_line (mark, NULL, -1, 0);
}


MooTextBuffer *
moo_line_mark_get_buffer (MooLineMark *mark)
{
    g_return_val_if_fail (MOO_IS_LINE_MARK (mark), NULL);
    return mark->priv->buffer;
}


static void
moo_line_mark_deleted_real (MooLineMark *mark)
{
    _moo_line_mark_set_buffer (mark, NULL, NULL);
}


void
_moo_line_mark_deleted (MooLineMark *mark)
{
    g_assert (MOO_IS_LINE_MARK (mark));
    MOO_LINE_MARK_GET_CLASS (mark)->deleted (mark);
}


gboolean
moo_line_mark_get_deleted (MooLineMark *mark)
{
    g_return_val_if_fail (MOO_IS_LINE_MARK (mark), TRUE);
    return mark->priv->buffer == NULL;
}


gboolean
moo_line_mark_get_visible (MooLineMark *mark)
{
    g_return_val_if_fail (MOO_IS_LINE_MARK (mark), FALSE);
    return mark->priv->visible != 0;
}


void
moo_line_mark_set_stock_id (MooLineMark *mark,
                            const char  *stock_id)
{
    g_return_if_fail (MOO_IS_LINE_MARK (mark));

    if (stock_id != mark->priv->stock_id)
    {
        if (mark->priv->pixbuf)
            g_object_unref (mark->priv->pixbuf);
        mark->priv->pixbuf = NULL;
        g_free (mark->priv->stock_id);

        mark->priv->stock_id = g_strdup (stock_id);

        update_pixbuf (mark);
        g_signal_emit (mark, signals[CHANGED], 0);
    }
}


void
moo_line_mark_set_pixbuf (MooLineMark    *mark,
                          GdkPixbuf      *pixbuf)
{
    g_return_if_fail (MOO_IS_LINE_MARK (mark));
    g_return_if_fail (!pixbuf || GDK_IS_PIXBUF (pixbuf));

    if (pixbuf != mark->priv->pixbuf)
    {
        if (mark->priv->pixbuf)
            g_object_unref (mark->priv->pixbuf);
        mark->priv->pixbuf = NULL;
        g_free (mark->priv->stock_id);
        mark->priv->stock_id = NULL;

        mark->priv->pixbuf = g_object_ref (pixbuf);

        g_signal_emit (mark, signals[CHANGED], 0);
    }
}


const char *
moo_line_mark_get_stock_id (MooLineMark *mark)
{
    g_return_val_if_fail (MOO_IS_LINE_MARK (mark), NULL);
    return mark->priv->stock_id;
}


GdkPixbuf *
moo_line_mark_get_pixbuf (MooLineMark *mark)
{
    g_return_val_if_fail (MOO_IS_LINE_MARK (mark), NULL);
    return mark->priv->pixbuf;
}


static void
update_pixbuf (MooLineMark *mark)
{
    GHashTable *cache;
    GdkPixbuf *pixbuf;

    if (!mark->priv->realized || !mark->priv->stock_id)
        return;

    g_assert (mark->priv->widget != NULL);
    g_return_if_fail (GTK_IS_WIDGET (mark->priv->widget));
    g_return_if_fail (GTK_WIDGET_REALIZED (mark->priv->widget));

    cache = g_object_get_data (G_OBJECT (mark->priv->widget),
                               "moo-line-mark-icons");

    if (!cache)
    {
        cache = g_hash_table_new_full (g_str_hash, g_str_equal,
                                       g_free, g_object_unref);
        g_object_set_data_full (G_OBJECT (mark->priv->widget),
                                "moo-line-mark-icons", cache,
                                (GDestroyNotify) g_hash_table_destroy);
    }

    pixbuf = g_hash_table_lookup (cache, mark->priv->stock_id);

    if (!pixbuf)
    {
        pixbuf = gtk_widget_render_icon (mark->priv->widget,
                                         mark->priv->stock_id,
                                         GTK_ICON_SIZE_MENU,
                                         NULL);
        g_return_if_fail (pixbuf != NULL);
        g_hash_table_insert (cache, g_strdup (mark->priv->stock_id), pixbuf);
    }

    mark->priv->pixbuf = g_object_ref (pixbuf);
}


void
_moo_line_mark_realize (MooLineMark *mark,
                        GtkWidget   *widget)
{
    g_assert (MOO_IS_LINE_MARK (mark));
    g_assert (GTK_IS_WIDGET (widget));
    g_assert (GTK_WIDGET_REALIZED (widget));
    g_assert (!mark->priv->realized);

    mark->priv->realized = TRUE;
    mark->priv->widget = widget;

    update_pixbuf (mark);
}


void
_moo_line_mark_unrealize (MooLineMark *mark)
{
    g_assert (MOO_IS_LINE_MARK (mark));
    g_assert (mark->priv->realized);

    mark->priv->realized = FALSE;
    mark->priv->widget = NULL;

    if (mark->priv->pixbuf && mark->priv->stock_id)
    {
        g_object_unref (mark->priv->pixbuf);
        mark->priv->pixbuf = NULL;
    }
}


const GdkColor *
moo_line_mark_get_background(MooLineMark *mark)
{
    g_return_val_if_fail (MOO_IS_LINE_MARK (mark), NULL);
    return mark->priv->background_set ? &mark->priv->background : NULL;
}


void
_moo_line_mark_set_pretty (MooLineMark *mark,
                           gboolean     pretty)
{
    g_assert (MOO_IS_LINE_MARK (mark));
    mark->priv->pretty = pretty != 0;
}


gboolean
_moo_line_mark_get_pretty (MooLineMark *mark)
{
    g_assert (MOO_IS_LINE_MARK (mark));
    return mark->priv->pretty != 0;
}


void
_moo_line_mark_set_fold (MooLineMark *mark,
                         MooFold     *fold)
{
    g_assert (MOO_IS_LINE_MARK (mark));
    g_assert (!fold || MOO_IS_FOLD (fold));
    mark->priv->fold = fold;
}


MooFold *
_moo_line_mark_get_fold (MooLineMark *mark)
{
    g_assert (MOO_IS_LINE_MARK (mark));
    return mark->priv->fold;
}
