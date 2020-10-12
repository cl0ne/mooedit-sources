/*
 *   moolineview.c
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

/**
 * class:MooLineView: (parent MooTextView) (constructable)
 **/

#include "moolineview.h"
#include "marshals.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/mooutils-messages.h"
#include "mooutils/moocompat.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>


struct _MooLineViewPrivate {
    MooData *line_data;
    GHashTable *cursors;
    gboolean busy;
    gboolean scrolled;
    GtkTextMark *end_mark;
    GtkWidget *hscrollbar;
};

static void      moo_line_view_finalize         (GObject        *object);

static void      moo_line_view_parent_set       (GtkWidget      *widget,
                                                 GtkWidget      *old_parent);
static gboolean  moo_line_view_button_release   (GtkWidget      *widget,
                                                 GdkEventButton *event);

static void      moo_line_view_move_cursor      (GtkTextView    *text_view,
                                                 GtkMovementStep step,
                                                 gint            count,
                                                 gboolean        extend_selection);
static void      moo_line_view_populate_popup   (GtkTextView    *text_view,
                                                 GtkMenu        *menu);

static MooTextCursor
                 moo_line_view_get_text_cursor  (MooTextView    *view,
                                                 int             x,
                                                 int             y);

static void      activate                       (MooLineView    *view,
                                                 int             line);
static void      activate_current_line          (MooLineView    *view);


static GtkTextBuffer *get_buffer                (MooLineView    *view);


enum {
    ACTIVATE,
    ACTIVATE_CURRENT_LINE,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


/* MOO_TYPE_LINE_VIEW */
G_DEFINE_TYPE (MooLineView, moo_line_view, MOO_TYPE_TEXT_VIEW)


static void
moo_line_view_class_init (MooLineViewClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    GtkTextViewClass *textview_class = GTK_TEXT_VIEW_CLASS (klass);
    MooTextViewClass *mootextview_class = MOO_TEXT_VIEW_CLASS (klass);
    GtkBindingSet *binding_set;

    gobject_class->finalize = moo_line_view_finalize;

    widget_class->button_release_event = moo_line_view_button_release;
    widget_class->parent_set = moo_line_view_parent_set;

    textview_class->move_cursor = moo_line_view_move_cursor;
    textview_class->populate_popup = moo_line_view_populate_popup;

    mootextview_class->get_text_cursor = moo_line_view_get_text_cursor;

    g_type_class_add_private (klass, sizeof (MooLineViewPrivate));

    signals[ACTIVATE] =
            g_signal_new ("activate",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooLineViewClass, activate),
                          NULL, NULL,
                          _moo_marshal_VOID__INT,
                          G_TYPE_NONE, 1,
                          G_TYPE_INT);

    signals[ACTIVATE_CURRENT_LINE] =
            _moo_signal_new_cb ("activate-current-line",
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (activate_current_line),
                                NULL, NULL,
                                _moo_marshal_VOID__VOID,
                                G_TYPE_NONE, 0);

    binding_set = gtk_binding_set_by_class (klass);

    gtk_binding_entry_add_signal (binding_set, GDK_Return, (GdkModifierType) 0,
                                  "activate-current-line", 0);
}


static void
moo_line_view_init (MooLineView *view)
{
    view->priv = G_TYPE_INSTANCE_GET_PRIVATE (view, MOO_TYPE_LINE_VIEW, MooLineViewPrivate);

    view->priv->hscrollbar = NULL;
    view->priv->line_data = _moo_data_new (g_direct_hash,
                                           g_direct_equal,
                                           NULL);
    view->priv->cursors = g_hash_table_new (g_direct_hash, g_direct_equal);

    g_object_set (view,
                  "editable", FALSE,
                  "cursor-visible", FALSE,
                  "current-line-color", "grey",
                  "highlight-matching-brackets", FALSE,
                  nullptr);
}


static void
moo_line_view_finalize (GObject *object)
{
    MooLineView *view = MOO_LINE_VIEW (object);

    _moo_data_destroy (view->priv->line_data);
    g_hash_table_destroy (view->priv->cursors);

    G_OBJECT_CLASS (moo_line_view_parent_class)->finalize (object);
}


static void
moo_line_view_parent_set (GtkWidget *widget,
                          GtkWidget *old_parent)
{
    MooLineView *view = MOO_LINE_VIEW (widget);

    view->priv->hscrollbar = NULL;

    if (widget->parent && GTK_IS_SCROLLED_WINDOW (widget->parent))
        view->priv->hscrollbar = GTK_SCROLLED_WINDOW(widget->parent)->hscrollbar;

    if (GTK_WIDGET_CLASS (moo_line_view_parent_class)->parent_set)
        GTK_WIDGET_CLASS (moo_line_view_parent_class)->parent_set (widget, old_parent);
}


GtkWidget *
moo_line_view_new (void)
{
    return GTK_WIDGET (g_object_new (MOO_TYPE_LINE_VIEW, (const char*) NULL));
}


/**
 * moo_line_view_clear:
 */
void
moo_line_view_clear (MooLineView *view)
{
    GtkTextBuffer *buffer;
    GtkTextIter start, end;

    g_return_if_fail (MOO_IS_LINE_VIEW (view));

    buffer = get_buffer (view);
    gtk_text_buffer_get_bounds (buffer, &start, &end);
    gtk_text_buffer_delete (buffer, &start, &end);

    _moo_data_clear (view->priv->line_data);
    g_hash_table_remove_all (view->priv->cursors);
}


static GtkTextBuffer*
get_buffer (MooLineView *view)
{
    return gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
}


static gboolean
moo_line_view_button_release (GtkWidget      *widget,
                              GdkEventButton *event)
{
    GtkTextView *textview = GTK_TEXT_VIEW (widget);
    MooLineView *view = MOO_LINE_VIEW (widget);
    int buffer_x, buffer_y;
    GtkTextIter iter;
    gboolean result;

    result = GTK_WIDGET_CLASS(moo_line_view_parent_class)->button_release_event (widget, event);

    if (gtk_text_view_get_window_type (textview, event->window) == GTK_TEXT_WINDOW_TEXT &&
        !moo_text_view_has_selection (MOO_TEXT_VIEW (widget)))
    {
        gtk_text_view_window_to_buffer_coords (textview,
                                               GTK_TEXT_WINDOW_TEXT,
                                               (int) event->x, (int) event->y,
                                               &buffer_x, &buffer_y);
        /* XXX */
        gtk_text_view_get_line_at_y (textview, &iter, buffer_y, NULL);
        activate (view, gtk_text_iter_get_line (&iter));
    }

    return result;
}


static void
activate (MooLineView *view,
          int          line)
{
    g_signal_emit (view, signals[ACTIVATE], 0, line);
}


static int
get_current_line (MooLineView *view)
{
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
    gtk_text_buffer_get_iter_at_mark (buffer, &iter, gtk_text_buffer_get_insert (buffer));
    return gtk_text_iter_get_line (&iter);
}


static void
activate_current_line (MooLineView *view)
{
    activate (view, get_current_line (view));
}


static int
get_visible_height (GtkTextView *text_view)
{
    GdkRectangle rect;
    GtkTextIter iter;
    int start, end;

    gtk_text_view_get_visible_rect (text_view, &rect);
    gtk_text_view_get_line_at_y (text_view, &iter, rect.y, NULL);
    start = gtk_text_iter_get_line (&iter);
    gtk_text_view_get_line_at_y (text_view, &iter, rect.y + rect.height - 1, NULL);
    end = gtk_text_iter_get_line (&iter);

    return end - start + 1;
}


static void
moo_line_view_move_cursor (GtkTextView    *text_view,
                           GtkMovementStep step,
                           gint            count,
                           gboolean        extend_selection)
{
    gboolean handle;
    MooLineView *view;
    GtkTextBuffer *buffer;
    int current_line, new_line = 0, height, total;
    GtkTextIter iter;

    switch (step)
    {
        case GTK_MOVEMENT_LOGICAL_POSITIONS:
        case GTK_MOVEMENT_VISUAL_POSITIONS:
        case GTK_MOVEMENT_WORDS:
        case GTK_MOVEMENT_PARAGRAPH_ENDS:
        case GTK_MOVEMENT_HORIZONTAL_PAGES:
            handle = FALSE;
            break;

        default:
            handle = TRUE;
    }

    if (extend_selection)
        handle = FALSE;

    if (!handle)
    {
        GTK_TEXT_VIEW_CLASS(moo_line_view_parent_class)->move_cursor (text_view, step, count, extend_selection);
        return;
    }

    view = MOO_LINE_VIEW (text_view);
    buffer = gtk_text_view_get_buffer (text_view);

    gtk_text_buffer_get_iter_at_mark (buffer, &iter, gtk_text_buffer_get_insert (buffer));
    current_line = get_current_line (view);

    height = get_visible_height (text_view);
    total = gtk_text_buffer_get_line_count (buffer);

    switch (step)
    {
        case GTK_MOVEMENT_DISPLAY_LINES:
        case GTK_MOVEMENT_PARAGRAPHS:
            new_line = current_line + count;
            break;

        case GTK_MOVEMENT_PAGES:
            new_line = current_line + count * (height - 1);
            break;

        case GTK_MOVEMENT_DISPLAY_LINE_ENDS:
        case GTK_MOVEMENT_BUFFER_ENDS:
            if (count < 0)
                new_line = 0;
            else
                new_line = total - 1;
            break;

        case GTK_MOVEMENT_LOGICAL_POSITIONS:
        case GTK_MOVEMENT_VISUAL_POSITIONS:
        case GTK_MOVEMENT_WORDS:
        case GTK_MOVEMENT_PARAGRAPH_ENDS:
        case GTK_MOVEMENT_HORIZONTAL_PAGES:
            g_return_if_reached ();
    }

    new_line = CLAMP (new_line, 0, total - 1);
    gtk_text_buffer_get_iter_at_line (buffer, &iter, new_line);
    gtk_text_buffer_place_cursor (buffer, &iter);
    gtk_text_view_scroll_to_mark (text_view,
                                  gtk_text_buffer_get_insert (buffer),
                                  0, FALSE, 0, 0);
}


static MooTextCursor
moo_line_view_get_text_cursor (MooTextView    *view,
                               int             x,
                               int             y)
{
    GtkTextIter iter;
    GtkTextView *text_view;
    int line_y, line_height;
    int line;
    MooTextCursor cursor;

    if (x < 0 || y < 0)
        return MOO_TEXT_CURSOR_ARROW;

    text_view = GTK_TEXT_VIEW (view);

    gtk_text_view_get_line_at_y (text_view, &iter, y, NULL);
    gtk_text_view_get_line_yrange (text_view, &iter, &line_y, &line_height);

    if (y >= line_y + line_height)
        return MOO_TEXT_CURSOR_ARROW;

    line = gtk_text_iter_get_line (&iter);
    cursor = (MooTextCursor) GPOINTER_TO_INT (g_hash_table_lookup (MOO_LINE_VIEW (view)->priv->cursors, GINT_TO_POINTER (line)));

    return cursor ? cursor : MOO_TEXT_CURSOR_ARROW;
}

void
moo_line_view_set_cursor (MooLineView    *view,
                          int             line,
                          MooTextCursor   cursor)
{
    g_return_if_fail (MOO_IS_LINE_VIEW (view));
    g_return_if_fail (line >= 0 && line < gtk_text_buffer_get_line_count (get_buffer (view)));

    if (cursor)
        g_hash_table_insert (view->priv->cursors, GINT_TO_POINTER (line), GINT_TO_POINTER (cursor));
    else
        g_hash_table_remove (view->priv->cursors, GINT_TO_POINTER (line));
}


void
moo_line_view_set_data (MooLineView    *view,
                        int             line,
                        gpointer        data,
                        GDestroyNotify  free_func)
{
    g_return_if_fail (MOO_IS_LINE_VIEW (view));
    g_return_if_fail (line >= 0 && line < gtk_text_buffer_get_line_count (get_buffer (view)));

    if (data)
        _moo_data_insert_ptr (view->priv->line_data,
                              GINT_TO_POINTER (line),
                              data, free_func);
    else
        _moo_data_remove (view->priv->line_data, GINT_TO_POINTER (line));
}


gpointer
moo_line_view_get_data (MooLineView    *view,
                        int             line)
{
    g_return_val_if_fail (MOO_IS_LINE_VIEW (view), NULL);
    g_return_val_if_fail (line >= 0, NULL);
    return _moo_data_get_ptr (view->priv->line_data,
                              GINT_TO_POINTER (line));
}


GtkTextTag*
moo_line_view_create_tag (MooLineView    *view,
                          const char     *tag_name,
                          const char     *first_property_name,
                          ...)
{
    GtkTextTag *tag;
    GtkTextBuffer *buffer;
    va_list list;

    g_return_val_if_fail (MOO_IS_LINE_VIEW (view), NULL);

    buffer = get_buffer (view);
    tag = gtk_text_buffer_create_tag (buffer, tag_name, NULL);
    g_return_val_if_fail (tag != NULL, NULL);

    if (first_property_name)
    {
        va_start (list, first_property_name);
        g_object_set_valist (G_OBJECT (tag), first_property_name, list);
        va_end (list);
    }

    return tag;
}


GtkTextTag*
moo_line_view_lookup_tag (MooLineView    *view,
                          const char     *tag_name)
{
    GtkTextTagTable *table;
    GtkTextBuffer *buffer;

    g_return_val_if_fail (MOO_IS_LINE_VIEW (view), NULL);
    g_return_val_if_fail (tag_name != NULL, NULL);

    buffer = get_buffer (view);
    table = gtk_text_buffer_get_tag_table (buffer);

    return gtk_text_tag_table_lookup (table, tag_name);
}


int
moo_line_view_write_line (MooLineView    *view,
                          const char     *text,
                          int             len,
                          GtkTextTag     *tag)
{
    int line;

    g_return_val_if_fail (MOO_IS_LINE_VIEW (view), -1);
    g_return_val_if_fail (text != NULL, -1);

    line = moo_line_view_start_line (view);
    g_return_val_if_fail (line >= 0, -1);

    moo_line_view_write (view, text, len, tag);
    moo_line_view_end_line (view);

    return line;
}


#if 0
static void
check_if_scrolled (MooLineView *view)
{
    GdkRectangle rect;
    GtkTextIter iter;
    int line;

    if (!GTK_WIDGET_REALIZED (view))
    {
        view->priv->scrolled = FALSE;
        return;
    }

    gtk_text_view_get_visible_rect (GTK_TEXT_VIEW (view), &rect);
    gtk_text_view_get_line_at_y (GTK_TEXT_VIEW (view), &iter,
                                 rect.y + rect.height - 1, NULL);
    line = gtk_text_iter_get_line (&iter);

    if (line + 2 < gtk_text_buffer_get_line_count (get_buffer (view)))
        view->priv->scrolled = TRUE;
    else
        view->priv->scrolled = FALSE;
}
#else
static void
check_if_scrolled (MooLineView *view)
{
    int delta;
    GtkAdjustment *adj = GTK_TEXT_VIEW (view)->vadjustment;

    delta = 10;

    if (view->priv->hscrollbar && GTK_WIDGET_VISIBLE (view->priv->hscrollbar))
    {
        int space;
        gtk_widget_style_get (GTK_WIDGET (view)->parent, "scrollbar-spacing", &space, nullptr);
        delta = MAX (delta - 1, space + view->priv->hscrollbar->allocation.height) + 1;
    }

    view->priv->scrolled = adj && GTK_WIDGET_REALIZED (view) &&
                           ABS (adj->value - (adj->upper - adj->page_size)) > delta;
}
#endif


int
moo_line_view_start_line (MooLineView *view)
{
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    g_return_val_if_fail (MOO_IS_LINE_VIEW (view), -1);
    g_return_val_if_fail (!view->priv->busy, -1);

    view->priv->busy = TRUE;

    buffer = get_buffer (view);
    gtk_text_buffer_get_end_iter (buffer, &iter);

    check_if_scrolled (view);

    if (!gtk_text_iter_starts_line (&iter))
        gtk_text_buffer_insert (buffer, &iter, "\n", 1);

    return gtk_text_iter_get_line (&iter);
}


void
moo_line_view_write (MooLineView    *view,
                     const char     *text,
                     int             len,
                     GtkTextTag     *tag)
{
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    g_return_if_fail (MOO_IS_LINE_VIEW (view));
    g_return_if_fail (text != NULL);
    g_return_if_fail (view->priv->busy);

    buffer = get_buffer (view);
    gtk_text_buffer_get_end_iter (buffer, &iter);

    if (g_utf8_validate (text, len, NULL))
    {
        gtk_text_buffer_insert_with_tags (buffer, &iter, text, len, tag, nullptr);
    }
    else
    {
        char *text_utf8 = g_locale_to_utf8 (text, len, NULL, NULL, NULL);

        if (text_utf8)
            gtk_text_buffer_insert_with_tags (buffer, &iter, text_utf8, -1, tag, nullptr);
        else
            g_warning ("could not convert '%s' to utf8", text);

        g_free (text_utf8);
    }
}


static GtkTextMark *
get_end_mark (MooLineView *view)
{
    GtkTextIter iter;
    GtkTextBuffer *buffer = get_buffer (view);

    gtk_text_buffer_get_end_iter (buffer, &iter);
    gtk_text_iter_set_line_offset (&iter, 0);

    if (!view->priv->end_mark)
        view->priv->end_mark =
                gtk_text_buffer_create_mark (buffer, NULL, &iter, FALSE);
    else
        gtk_text_buffer_move_mark (buffer, view->priv->end_mark, &iter);

    return view->priv->end_mark;
}


void
moo_line_view_end_line (MooLineView    *view)
{
    g_return_if_fail (MOO_IS_LINE_VIEW (view));
    g_return_if_fail (view->priv->busy);

    view->priv->busy = FALSE;

    if (!view->priv->scrolled)
        gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (view),
                                            get_end_mark (view));
}


static void
copy_clipboard (GtkTextView *text_view)
{
    g_signal_emit_by_name (text_view, "copy-clipboard");
}


static void
moo_line_view_populate_popup (GtkTextView *text_view,
                              GtkMenu     *menu)
{
    GtkWidget *item;
    gboolean has_selection, has_text;

    gtk_container_foreach (GTK_CONTAINER (menu),
                           (GtkCallback) gtk_widget_destroy,
                           NULL);

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_COPY, NULL);
    g_signal_connect_swapped (item, "activate",
                              G_CALLBACK (copy_clipboard), text_view);
    has_selection = moo_text_view_has_selection (MOO_TEXT_VIEW (text_view));
    gtk_widget_set_sensitive (item, has_selection);
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_SELECT_ALL, NULL);
    g_signal_connect_swapped (item, "activate",
                              G_CALLBACK (moo_text_view_select_all),
                              text_view);
    has_text = moo_text_view_has_text (MOO_TEXT_VIEW (text_view));
    gtk_widget_set_sensitive (item, has_text);
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
}


void
moo_line_view_set_line_data (MooLineView    *view,
                             int             line,
                             const GValue   *data)
{
    g_return_if_fail (MOO_IS_LINE_VIEW (view));
    g_return_if_fail (line >= 0);
    g_return_if_fail (!data || G_IS_VALUE (data));

    if (data)
        _moo_data_insert_value (view->priv->line_data,
                                GINT_TO_POINTER (line),
                                data);
    else
        _moo_data_remove (view->priv->line_data,
                          GINT_TO_POINTER (line));
}


void
moo_line_view_set_boxed (MooLineView    *view,
                         int             line,
                         GType           type,
                         gpointer        data)
{
    GValue gval;

    g_return_if_fail (MOO_IS_LINE_VIEW (view));
    g_return_if_fail (line >= 0);
    g_return_if_fail (g_type_is_a (type, G_TYPE_BOXED));

    if (!data)
    {
        moo_line_view_set_data (view, line, NULL, NULL);
        return;
    }

    gval.g_type = 0;
    g_value_init (&gval, type);
    g_value_set_boxed (&gval, data);

    moo_line_view_set_line_data (view, line, &gval);

    g_value_unset (&gval);
}


gboolean
moo_line_view_get_line_data (MooLineView    *view,
                             int             line,
                             GValue         *dest)
{
    g_return_val_if_fail (MOO_IS_LINE_VIEW (view), FALSE);
    g_return_val_if_fail (line >= 0, FALSE);
    g_return_val_if_fail (!G_IS_VALUE (dest), FALSE);
    return _moo_data_get_value (view->priv->line_data,
                                GINT_TO_POINTER (line),
                                dest);
}


/* returns a copy */
gpointer
moo_line_view_get_boxed (MooLineView    *view,
                         int             line,
                         GType           type)
{
    GValue gval;
    gpointer ret = NULL;

    g_return_val_if_fail (MOO_IS_LINE_VIEW (view), NULL);
    g_return_val_if_fail (line >= 0, NULL);
    g_return_val_if_fail (g_type_is_a (type, G_TYPE_BOXED), NULL);

    gval.g_type = 0;

    if (!moo_line_view_get_line_data (view, line, &gval))
        return NULL;

    if (G_VALUE_TYPE (&gval) == type)
        ret = g_value_dup_boxed (&gval);

    g_value_unset (&gval);

    return ret;
}
