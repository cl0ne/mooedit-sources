/*
 *   ctags-view.c
 *
 *   Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
 *   Copyright (C) 2008      by Christian Dywan <christian@twotoasts.de>
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

#include "ctags-view.h"
#include "ctags-doc.h"
#include "plugins/mooplugin-builtin.h"
#include "marshals.h"
#include <mooutils/mooutils-gobject.h>
#include <mooutils/mooutils-treeview.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (MooCtagsView, _moo_ctags_view, GTK_TYPE_TREE_VIEW)

struct _MooCtagsViewPrivate {
    guint nothing;
};


static void
moo_ctags_view_cursor_changed (GtkTreeView *treeview)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    MooCtagsEntry *entry;

    if (GTK_TREE_VIEW_CLASS(_moo_ctags_view_parent_class)->cursor_changed)
        GTK_TREE_VIEW_CLASS(_moo_ctags_view_parent_class)->cursor_changed (treeview);

    selection = gtk_tree_view_get_selection (treeview);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;

    gtk_tree_model_get (model, &iter, MOO_CTAGS_VIEW_COLUMN_ENTRY, &entry, -1);

    if (entry)
    {
        g_signal_emit_by_name (treeview, "activate-entry", entry);
        _moo_ctags_entry_unref (entry);
    }

}

static void
_moo_ctags_view_class_init (MooCtagsViewClass *klass)
{
    GtkTreeViewClass *treeview_class = GTK_TREE_VIEW_CLASS (klass);

    treeview_class->cursor_changed = moo_ctags_view_cursor_changed;

    g_signal_new ("activate-entry",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _moo_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  MOO_TYPE_CTAGS_ENTRY);

    g_type_class_add_private (klass, sizeof (MooCtagsViewPrivate));
}


static void
data_func (G_GNUC_UNUSED GtkTreeViewColumn *column,
           GtkCellRenderer   *cell,
           GtkTreeModel      *model,
           GtkTreeIter       *iter)
{
    MooCtagsEntry *entry = NULL;
    char *label = NULL;

    gtk_tree_model_get (model, iter,
                        MOO_CTAGS_VIEW_COLUMN_ENTRY, &entry,
                        MOO_CTAGS_VIEW_COLUMN_LABEL, &label,
                        -1);

    if (entry)
    {
        /*if (entry->signature)
            markup = g_strdup_printf ("%s %s", entry->name, entry->signature);
        else
            markup = g_strdup (entry->name);
        /*/
        g_object_set (cell, "text", entry->name, NULL);
    }
    else
    {
        g_object_set (cell, "markup", label, NULL);
    }

    g_free (label);
    _moo_ctags_entry_unref (entry);
}

static gboolean
tree_view_search_equal_func (GtkTreeModel *model,
                             G_GNUC_UNUSED int column,
                             const char   *key,
                             GtkTreeIter  *iter)
{
    MooCtagsEntry *entry = NULL;
    const char *compare_with = NULL;
    gboolean retval = TRUE;

    gtk_tree_model_get (model, iter,
                        MOO_CTAGS_VIEW_COLUMN_ENTRY, &entry,
                        -1);

    if (entry)
        compare_with = entry->name;

    if (compare_with)
        retval = !_moo_str_semicase_compare (compare_with, key);

    _moo_ctags_entry_unref (entry);

    return retval;
}

static void
_moo_ctags_view_init (MooCtagsView *view)
{
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;

    view->priv = G_TYPE_INSTANCE_GET_PRIVATE (view, MOO_TYPE_CTAGS_VIEW,
                                              MooCtagsViewPrivate);

    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
    _moo_tree_view_setup_expander (GTK_TREE_VIEW (view), column);

    gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (view),
                                         (GtkTreeViewSearchEqualFunc) tree_view_search_equal_func,
                                         NULL, NULL);

    cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, cell, TRUE);
    gtk_tree_view_column_set_cell_data_func (column, cell,
                                             (GtkTreeCellDataFunc) data_func,
                                             NULL, NULL);
}


GtkWidget *
_moo_ctags_view_new (void)
{
    return g_object_new (MOO_TYPE_CTAGS_VIEW, (const char*) NULL);
}
