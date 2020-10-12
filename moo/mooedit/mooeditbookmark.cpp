/*
 *   mooedit-bookmarks.c
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
 * class:MooEditBookmark: (parent MooLineMark) (moo.private 1)
 **/

#include "mooedit/mooeditbookmark.h"
#include "mooedit/mooedit-private.h"
#include "mooedit/mootextbuffer.h"
#include "mooutils/moostock.h"
#include "mooutils/mooutils-misc.h"


G_DEFINE_TYPE (MooEditBookmark, moo_edit_bookmark, MOO_TYPE_LINE_MARK)


static void        disconnect_bookmark (MooEditBookmark *bk);
static const char *get_bookmark_color  (MooEdit         *doc);


static void
moo_edit_bookmark_finalize (GObject *object)
{
    G_OBJECT_CLASS(moo_edit_bookmark_parent_class)->finalize (object);
}


static void
moo_edit_bookmark_class_init (MooEditBookmarkClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = moo_edit_bookmark_finalize;
}


static void
moo_edit_bookmark_init (MooEditBookmark *bk)
{
    g_object_set (bk, "visible", TRUE, (char*) 0);
}


static void
bookmarks_changed (G_GNUC_UNUSED MooEdit *edit)
{
}


static MooTextBuffer *
get_moo_buffer (MooEdit *edit)
{
    return MOO_TEXT_BUFFER (moo_edit_get_buffer (edit));
}

static guint
get_line_count (MooEdit *edit)
{
    return gtk_text_buffer_get_line_count (moo_edit_get_buffer (edit));
}


void
moo_edit_set_enable_bookmarks (MooEdit  *edit,
                               gboolean  enable)
{
    g_return_if_fail (MOO_IS_EDIT (edit));

    enable = enable != 0;

    if ((bool) enable != edit->priv->enable_bookmarks)
    {
        edit->priv->enable_bookmarks = enable;

        if (!enable)
            _moo_edit_delete_bookmarks (edit, FALSE);

        g_object_notify (G_OBJECT (edit), "enable-bookmarks");
    }
}


gboolean
moo_edit_get_enable_bookmarks (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), FALSE);
    return edit->priv->enable_bookmarks;
}


static int
cmp_bookmarks (MooLineMark *a,
               MooLineMark *b)
{
    int line_a = moo_line_mark_get_line (a);
    int line_b = moo_line_mark_get_line (b);
    return line_a < line_b ? -1 : (line_a > line_b ? 1 : 0);
}

static gboolean
update_bookmarks (MooEdit *edit)
{
    GSList *deleted, *dup, *old, *new_, *l;

    edit->priv->update_bookmarks_idle = 0;
    old = edit->priv->bookmarks;
    edit->priv->bookmarks = NULL;

    for (deleted = NULL, new_ = NULL, l = old; l != NULL; l = l->next)
        if (moo_line_mark_get_deleted (MOO_LINE_MARK (l->data)))
            deleted = g_slist_prepend (deleted, l->data);
        else
            new_ = g_slist_prepend (new_, l->data);

    g_slist_foreach (deleted, (GFunc) disconnect_bookmark, NULL);
    g_slist_foreach (deleted, (GFunc) g_object_unref, NULL);
    g_slist_free (deleted);

    new_ = g_slist_sort (new_, (GCompareFunc) cmp_bookmarks);
    old = new_;
    new_ = NULL;
    dup = NULL;

    for (l = old; l != NULL; l = l->next)
        if (new_ && moo_line_mark_get_line (MOO_LINE_MARK (new_->data)) == moo_line_mark_get_line (MOO_LINE_MARK (l->data)))
            dup = g_slist_prepend (dup, l->data);
        else
            new_ = g_slist_prepend (new_, l->data);

    while (dup)
    {
        disconnect_bookmark (MOO_EDIT_BOOKMARK (dup->data));
        moo_text_buffer_delete_line_mark (get_moo_buffer (edit), MOO_LINE_MARK (dup->data));
        g_object_unref (dup->data);
        dup = g_slist_delete_link (dup, dup);
    }

    edit->priv->bookmarks = g_slist_reverse (new_);

    return FALSE;
}


static void
update_bookmarks_now (MooEdit *edit)
{
    if (edit->priv->update_bookmarks_idle)
    {
        g_source_remove (edit->priv->update_bookmarks_idle);
        edit->priv->update_bookmarks_idle = 0;
        update_bookmarks (edit);
    }
}


const GSList *
moo_edit_list_bookmarks (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), NULL);
    update_bookmarks_now (edit);
    return edit->priv->bookmarks;
}


void
moo_edit_toggle_bookmark (MooEdit *edit,
                          guint    line)
{
    MooEditBookmark *bk;

    g_return_if_fail (MOO_IS_EDIT (edit));
    g_return_if_fail (line < get_line_count (edit));

    bk = moo_edit_get_bookmark_at_line (edit, line);

    if (bk)
        moo_edit_remove_bookmark (edit, bk);
    else
        moo_edit_add_bookmark (edit, line, 0);
}


MooEditBookmark *
moo_edit_get_bookmark_at_line (MooEdit *edit,
                               guint    line)
{
    GSList *list, *l;
    MooEditBookmark *bk;

    g_return_val_if_fail (MOO_IS_EDIT (edit), NULL);
    g_return_val_if_fail (line < get_line_count (edit), NULL);

    bk = NULL;
    list = moo_text_buffer_get_line_marks_at_line (get_moo_buffer (edit), line);

    for (l = list; l != NULL; l = l->next)
    {
        if (MOO_IS_EDIT_BOOKMARK (l->data) && g_slist_find (edit->priv->bookmarks, l->data))
        {
            bk = MOO_EDIT_BOOKMARK (l->data);
            break;
        }
    }

    g_slist_free (list);
    return bk;
}

void
moo_edit_remove_bookmark (MooEdit         *edit,
                          MooEditBookmark *bookmark)
{
    g_return_if_fail (MOO_IS_EDIT (edit));
    g_return_if_fail (MOO_IS_EDIT_BOOKMARK (bookmark));
    g_return_if_fail (g_slist_find (edit->priv->bookmarks, bookmark));

    disconnect_bookmark (bookmark);
    edit->priv->bookmarks = g_slist_remove (edit->priv->bookmarks, bookmark);
    moo_text_buffer_delete_line_mark (get_moo_buffer (edit), MOO_LINE_MARK (bookmark));

    g_object_unref (bookmark);
    bookmarks_changed (edit);
}


static guint
get_unused_bookmark_no (MooEdit *edit)
{
    guint i;
    const GSList *list;
    char used[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    list = moo_edit_list_bookmarks (edit);

    while (list)
    {
        MooEditBookmark *bk = MOO_EDIT_BOOKMARK (list->data);
        used[bk->no] = 1;
        list = list->next;
    }

    for (i = 1; i < 10; ++i)
        if (!used[i])
            return i;

    return 0;
}

void
moo_edit_add_bookmark (MooEdit *edit,
                       guint    line,
                       guint    no)
{
    guint i;
    MooEditBookmark *bk;

    g_return_if_fail (MOO_IS_EDIT (edit));
    g_return_if_fail (line < get_line_count (edit));
    g_return_if_fail (moo_edit_get_bookmark_at_line (edit, line) == NULL);

    for (i = 0; i < edit->priv->views->n_elms; ++i)
        g_object_set (edit->priv->views->elms[i], "show-line-marks", TRUE, (char*) 0);

    bk = MOO_EDIT_BOOKMARK (g_object_new (MOO_TYPE_EDIT_BOOKMARK, "background", get_bookmark_color (edit), (char*) 0));
    moo_text_buffer_add_line_mark (get_moo_buffer (edit), MOO_LINE_MARK (bk), line);
    g_object_set_data (G_OBJECT (bk), "moo-edit-bookmark", GINT_TO_POINTER (TRUE));

    if (!no)
        no = get_unused_bookmark_no (edit);

    bk->no = no;

    if (no)
    {
        char buf[32];
        g_snprintf (buf, sizeof buf, "<b>%u</b>", no);
        moo_line_mark_set_markup (MOO_LINE_MARK (bk), buf);
    }
    else
    {
        moo_line_mark_set_stock_id (MOO_LINE_MARK (bk), MOO_STOCK_EDIT_BOOKMARK);
    }

    if (!edit->priv->update_bookmarks_idle)
        edit->priv->bookmarks =
                g_slist_insert_sorted (edit->priv->bookmarks, bk,
                                       (GCompareFunc) cmp_bookmarks);
    else
        edit->priv->bookmarks = g_slist_prepend (edit->priv->bookmarks, bk);

    bookmarks_changed (edit);
}


static void
disconnect_bookmark (MooEditBookmark *bk)
{
    g_object_set_data (G_OBJECT (bk), "moo-edit-bookmark", (char*) 0);
}


void
_moo_edit_line_mark_moved (MooEdit     *edit,
                           MooLineMark *mark)
{
    if (MOO_IS_EDIT_BOOKMARK (mark) &&
        g_object_get_data (G_OBJECT (mark), "moo-edit-bookmark") &&
        !edit->priv->update_bookmarks_idle)
    {
        edit->priv->update_bookmarks_idle =
                g_idle_add ((GSourceFunc) update_bookmarks, edit);
        bookmarks_changed (edit);
    }
}


void
_moo_edit_line_mark_deleted (MooEdit     *edit,
                             MooLineMark *mark)
{
    if (MOO_IS_EDIT_BOOKMARK (mark) &&
        g_object_get_data (G_OBJECT (mark), "moo-edit-bookmark") &&
        g_slist_find (edit->priv->bookmarks, mark))
    {
        disconnect_bookmark (MOO_EDIT_BOOKMARK (mark));
        edit->priv->bookmarks = g_slist_remove (edit->priv->bookmarks, mark);
        g_object_unref (mark);
        bookmarks_changed (edit);
    }
}


GSList *
moo_edit_get_bookmarks_in_range (MooEdit *edit,
                                 int      first_line,
                                 int      last_line)
{
    GSList *all, *range, *l;

    g_return_val_if_fail (MOO_IS_EDIT (edit), NULL);
    g_return_val_if_fail (first_line >= 0, NULL);

    if (last_line < 0 || last_line >= (int) get_line_count (edit))
        last_line = get_line_count (edit) - 1;

    if (first_line > last_line)
        return NULL;

    all = (GSList*) moo_edit_list_bookmarks (edit);

    for (l = all, range = NULL; l != NULL; l = l->next)
    {
        int line = moo_line_mark_get_line (MOO_LINE_MARK (l->data));

        if (line < first_line)
            continue;
        else if (line > last_line)
            break;
        else
            range = g_slist_prepend (range, l->data);
    }

    return g_slist_reverse (range);
}


void
_moo_edit_delete_bookmarks (MooEdit *edit,
                            gboolean in_destroy)
{
    GSList *bookmarks;

    bookmarks = edit->priv->bookmarks;
    edit->priv->bookmarks = NULL;

    if (bookmarks)
    {
        while (bookmarks)
        {
            disconnect_bookmark (MOO_EDIT_BOOKMARK (bookmarks->data));

            if (!in_destroy)
                moo_text_buffer_delete_line_mark (get_moo_buffer (edit),
                                                  MOO_LINE_MARK (bookmarks->data));

            g_object_unref (bookmarks->data);
            bookmarks = g_slist_delete_link (bookmarks, bookmarks);
        }

        if (!in_destroy)
            bookmarks_changed (edit);
    }
}


MooEditBookmark *
moo_edit_get_bookmark (MooEdit *edit,
                       guint    n)
{
    const GSList *list;

    g_return_val_if_fail (MOO_IS_EDIT (edit), NULL);
    g_return_val_if_fail (n > 0 && n < 10, NULL);

    list = moo_edit_list_bookmarks (edit);

    while (list)
    {
        MooEditBookmark *bk = MOO_EDIT_BOOKMARK (list->data);

        if (bk->no == n)
            return bk;

        list = list->next;
    }

    return NULL;
}


void
moo_edit_view_goto_bookmark (MooEditView     *view,
                             MooEditBookmark *bk)
{
    int cursor;

    g_return_if_fail (MOO_IS_EDIT_VIEW (view));
    g_return_if_fail (MOO_IS_EDIT_BOOKMARK (bk));

    cursor = moo_line_mark_get_line (MOO_LINE_MARK (bk));
    moo_text_view_move_cursor (MOO_TEXT_VIEW (view),
                               cursor, 0, FALSE, FALSE);
}


char *
_moo_edit_bookmark_get_text (MooEditBookmark *bk)
{
    MooTextBuffer *buffer;
    GtkTextIter start, end;
    char *line;

    g_return_val_if_fail (MOO_IS_EDIT_BOOKMARK (bk), NULL);

    buffer = moo_line_mark_get_buffer (MOO_LINE_MARK (bk));
    gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer), &start,
                                      moo_line_mark_get_line (MOO_LINE_MARK (bk)));
    end = start;
    if (!gtk_text_iter_ends_line (&end))
        gtk_text_iter_forward_to_line_end (&end);

    line = g_strstrip (gtk_text_iter_get_slice (&start, &end));

#define MAXBOOKMARKCHARS 40
    if (g_utf8_strlen (line, -1) > MAXBOOKMARKCHARS)
    {
        char *tmp;
        * g_utf8_offset_to_pointer (line, MAXBOOKMARKCHARS - 3) = 0;
        tmp = g_strdup_printf ("%s...", line);
        g_free (line);
        line = tmp;
    }
#undef MAXBOOKMARKCHARS

    return line;
}


static const char *
get_bookmark_color (MooEdit *doc)
{
    MooEditView *view;
    MooTextStyle *style;
    MooTextStyleScheme *scheme;

    view = moo_edit_get_view (doc);
    scheme = moo_text_view_get_style_scheme (MOO_TEXT_VIEW (view));

    if (!scheme)
        return NULL;

    style = _moo_text_style_scheme_lookup_style (scheme, "bookmark");
    return style ? _moo_text_style_get_bg_color (style) : NULL;
}

void
_moo_edit_update_bookmarks_style (MooEdit *edit)
{
    const GSList *bookmarks;
    const char *color;

    color = get_bookmark_color (edit);

    bookmarks = moo_edit_list_bookmarks (edit);
    while (bookmarks)
    {
        moo_line_mark_set_background (MOO_LINE_MARK (bookmarks->data), color);
        bookmarks = bookmarks->next;
    }
}
