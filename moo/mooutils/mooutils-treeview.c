/*
 *   mooutils-treeview.c
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

#include "mooutils/mooutils-treeview.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/moocompat.h"
#include "mooutils/mootype-macros.h"
#include "marshals.h"
#include <string.h>
#include <gobject/gvaluecollector.h>


typedef enum {
    TREE_VIEW,
    COMBO_BOX
} WidgetType;

static void moo_tree_helper_new_row             (MooTreeHelper      *helper);
static void moo_tree_helper_delete_row          (MooTreeHelper      *helper);
static void moo_tree_helper_row_up              (MooTreeHelper      *helper);
static void moo_tree_helper_row_down            (MooTreeHelper      *helper);
static void moo_tree_helper_real_update_widgets (MooTreeHelper      *helper,
                                                 GtkTreeModel       *model,
                                                 GtkTreePath        *path);
static GtkTreeModel *moo_tree_helper_get_model  (MooTreeHelper      *helper);
static gboolean moo_tree_helper_get_selected    (MooTreeHelper      *helper,
                                                 GtkTreeModel      **model,
                                                 GtkTreeIter        *iter);

static gboolean tree_helper_new_row_default     (MooTreeHelper      *helper,
                                                 GtkTreeModel       *model,
                                                 GtkTreePath        *path);
static gboolean tree_helper_delete_row_default  (MooTreeHelper      *helper,
                                                 GtkTreeModel       *model,
                                                 GtkTreePath        *path);
static gboolean tree_helper_move_row_default    (MooTreeHelper      *helper,
                                                 GtkTreeModel       *model,
                                                 GtkTreePath        *old_path,
                                                 GtkTreePath        *new_path);


G_DEFINE_TYPE (MooTreeHelper, _moo_tree_helper, GTK_TYPE_OBJECT)


enum {
    NEW_ROW,
    DELETE_ROW,
    MOVE_ROW,
    UPDATE_WIDGETS,
    UPDATE_MODEL,
    TREE_NUM_SIGNALS
};

static guint tree_signals[TREE_NUM_SIGNALS];


static void
tree_selection_changed (GtkTreeSelection *selection,
                        MooTreeHelper    *helper)
{
    GtkTreeRowReference *old_row;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path, *old_path;

    old_row = g_object_get_data (G_OBJECT (selection), "moo-tree-helper-current-row");
    old_path = old_row ? gtk_tree_row_reference_get_path (old_row) : NULL;

    if (old_row && !old_path)
    {
        g_object_set_data (G_OBJECT (selection), "moo-tree-helper-current-row", NULL);
        old_row = NULL;
    }

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        path = gtk_tree_model_get_path (model, &iter);

        if (old_path && !gtk_tree_path_compare (old_path, path))
        {
            gtk_tree_path_free (old_path);
            gtk_tree_path_free (path);
            return;
        }
    }
    else
    {
        if (!old_path)
            return;

        path = NULL;
    }

    if (old_path)
        _moo_tree_helper_update_model (helper, model, old_path);

    moo_tree_helper_real_update_widgets (helper, model, path);

    if (path)
    {
        GtkTreeRowReference *row;
        row = gtk_tree_row_reference_new (model, path);
        g_object_set_data_full (G_OBJECT (selection), "moo-tree-helper-current-row", row,
                                (GDestroyNotify) gtk_tree_row_reference_free);
    }
    else
    {
        g_object_set_data (G_OBJECT (selection), "moo-tree-helper-current-row", NULL);
    }

    gtk_tree_path_free (path);
    gtk_tree_path_free (old_path);
}


static void
combo_changed (GtkComboBox   *combo,
               MooTreeHelper *helper)
{
    GtkTreeRowReference *old_row;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path, *old_path;

    g_return_if_fail (MOO_IS_TREE_HELPER (helper));
    g_return_if_fail (combo == helper->widget);

    old_row = g_object_get_data (G_OBJECT (combo), "moo-tree-helper-current-row");
    old_path = old_row ? gtk_tree_row_reference_get_path (old_row) : NULL;

    if (old_row && !old_path)
    {
        g_object_set_data (G_OBJECT (combo), "moo-tree-helper-current-row", NULL);
        old_row = NULL;
    }

    if (moo_tree_helper_get_selected (helper, &model, &iter))
    {
        path = gtk_tree_model_get_path (model, &iter);

        if (old_path && !gtk_tree_path_compare (old_path, path))
        {
            gtk_tree_path_free (old_path);
            gtk_tree_path_free (path);
            return;
        }
    }
    else
    {
        if (!old_path)
            return;

        path = NULL;
    }

    if (old_path)
        _moo_tree_helper_update_model (helper, model, old_path);

    moo_tree_helper_real_update_widgets (helper, model, path);

    if (path)
    {
        GtkTreeRowReference *row;
        row = gtk_tree_row_reference_new (model, path);
        g_object_set_data_full (G_OBJECT (combo), "moo-tree-helper-current-row", row,
                                (GDestroyNotify) gtk_tree_row_reference_free);
    }
    else
    {
        g_object_set_data (G_OBJECT (combo), "moo-tree-helper-current-row", NULL);
    }

    gtk_tree_path_free (path);
    gtk_tree_path_free (old_path);
}


static void
moo_tree_helper_destroy (GtkObject *object)
{
    MooTreeHelper *helper = MOO_TREE_HELPER (object);

    if (helper->widget)
    {
        GtkTreeSelection *selection;

        g_signal_handlers_disconnect_by_func (helper->widget,
                                              (gpointer) gtk_object_destroy,
                                              helper);

        switch (helper->type)
        {
            case TREE_VIEW:
                selection = gtk_tree_view_get_selection (helper->widget);
                g_signal_handlers_disconnect_by_func (selection,
                                                      (gpointer) tree_selection_changed,
                                                      helper);
                break;

            case COMBO_BOX:
                g_signal_handlers_disconnect_by_func (helper->widget,
                                                      (gpointer) combo_changed,
                                                      helper);
                break;
        }

        if (helper->new_btn)
            g_signal_handlers_disconnect_by_func (helper->new_btn,
                                                  (gpointer) moo_tree_helper_new_row,
                                                  helper);
        if (helper->delete_btn)
            g_signal_handlers_disconnect_by_func (helper->delete_btn,
                                                  (gpointer) moo_tree_helper_delete_row,
                                                  helper);
        if (helper->up_btn)
            g_signal_handlers_disconnect_by_func (helper->up_btn,
                                                  (gpointer) moo_tree_helper_row_up,
                                                  helper);
        if (helper->down_btn)
            g_signal_handlers_disconnect_by_func (helper->down_btn,
                                                  (gpointer) moo_tree_helper_row_down,
                                                  helper);

        helper->widget = NULL;
    }

    GTK_OBJECT_CLASS (_moo_tree_helper_parent_class)->destroy (object);
}


static gboolean
tree_helper_new_row_default (G_GNUC_UNUSED MooTreeHelper *helper,
                             GtkTreeModel  *model,
                             GtkTreePath   *path)
{
    GtkTreeIter iter;

    if (!GTK_IS_LIST_STORE (model))
        return FALSE;

    g_return_val_if_fail (path != NULL, FALSE);
    g_return_val_if_fail (gtk_tree_path_get_depth (path) == 1, FALSE);

    gtk_list_store_insert (GTK_LIST_STORE (model), &iter,
                           gtk_tree_path_get_indices(path)[0]);

    return TRUE;
}


static gboolean
tree_helper_delete_row_default (G_GNUC_UNUSED MooTreeHelper *helper,
                                GtkTreeModel  *model,
                                GtkTreePath   *path)
{
    GtkTreeIter iter;

    if (!GTK_IS_LIST_STORE (model))
        return FALSE;

    g_return_val_if_fail (path != NULL, FALSE);
    g_return_val_if_fail (gtk_tree_path_get_depth (path) == 1, FALSE);

    gtk_tree_model_get_iter (model, &iter, path);
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

    return TRUE;
}


static gboolean
tree_helper_move_row_default (G_GNUC_UNUSED MooTreeHelper *helper,
                              GtkTreeModel *model,
                              GtkTreePath  *old_path,
                              GtkTreePath  *new_path)
{
    GtkTreeIter old_iter, new_iter;
    int new, old;

    if (!GTK_IS_LIST_STORE (model))
        return FALSE;

    g_return_val_if_fail (old_path && new_path, FALSE);
    g_return_val_if_fail (gtk_tree_path_get_depth (old_path) == 1, FALSE);
    g_return_val_if_fail (gtk_tree_path_get_depth (new_path) == 1, FALSE);

    new = gtk_tree_path_get_indices(new_path)[0];
    old = gtk_tree_path_get_indices(old_path)[0];
    g_return_val_if_fail (ABS (new - old) == 1, FALSE);

    gtk_tree_model_get_iter (model, &old_iter, old_path);
    gtk_tree_model_get_iter (model, &new_iter, new_path);
    gtk_list_store_swap (GTK_LIST_STORE (model), &old_iter, &new_iter);

    return TRUE;
}


static void
_moo_tree_helper_class_init (MooTreeHelperClass *klass)
{
    GTK_OBJECT_CLASS(klass)->destroy = moo_tree_helper_destroy;

    klass->move_row = tree_helper_move_row_default;
    klass->new_row = tree_helper_new_row_default;
    klass->delete_row = tree_helper_delete_row_default;

    tree_signals[NEW_ROW] =
            g_signal_new ("new-row",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTreeHelperClass, new_row),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOL__OBJECT_BOXED,
                          G_TYPE_BOOLEAN, 2,
                          GTK_TYPE_TREE_MODEL,
                          GTK_TYPE_TREE_PATH | G_SIGNAL_TYPE_STATIC_SCOPE);

    tree_signals[DELETE_ROW] =
            g_signal_new ("delete-row",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTreeHelperClass, delete_row),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOL__OBJECT_BOXED,
                          G_TYPE_BOOLEAN, 2,
                          GTK_TYPE_TREE_MODEL,
                          GTK_TYPE_TREE_PATH | G_SIGNAL_TYPE_STATIC_SCOPE);

    tree_signals[MOVE_ROW] =
            g_signal_new ("move-row",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTreeHelperClass, move_row),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOL__OBJECT_BOXED_BOXED,
                          G_TYPE_BOOLEAN, 3,
                          GTK_TYPE_TREE_MODEL,
                          GTK_TYPE_TREE_PATH | G_SIGNAL_TYPE_STATIC_SCOPE,
                          GTK_TYPE_TREE_PATH | G_SIGNAL_TYPE_STATIC_SCOPE);

    tree_signals[UPDATE_WIDGETS] =
            g_signal_new ("update-widgets",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTreeHelperClass, update_widgets),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT_BOXED_BOXED,
                          G_TYPE_NONE, 3,
                          GTK_TYPE_TREE_MODEL,
                          GTK_TYPE_TREE_PATH | G_SIGNAL_TYPE_STATIC_SCOPE,
                          GTK_TYPE_TREE_ITER | G_SIGNAL_TYPE_STATIC_SCOPE);

    tree_signals[UPDATE_MODEL] =
            g_signal_new ("update-model",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTreeHelperClass, update_model),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT_BOXED_BOXED,
                          G_TYPE_NONE, 3,
                          GTK_TYPE_TREE_MODEL,
                          GTK_TYPE_TREE_PATH | G_SIGNAL_TYPE_STATIC_SCOPE,
                          GTK_TYPE_TREE_ITER | G_SIGNAL_TYPE_STATIC_SCOPE);
}


void
_moo_tree_helper_set_modified (MooTreeHelper *helper,
                               gboolean       modified)
{
    g_return_if_fail (MOO_IS_TREE_HELPER (helper));

#ifdef MOO_DEBUG
    if (!helper->modified && modified)
        g_message ("helper modified");
#endif

    helper->modified = modified != 0;
}


gboolean
_moo_tree_helper_get_modified (MooTreeHelper *helper)
{
    g_return_val_if_fail (MOO_IS_TREE_HELPER (helper), FALSE);
    return helper->modified;
}


void
_moo_tree_view_select_first (GtkTreeView *tree_view)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;

    g_return_if_fail (GTK_IS_TREE_VIEW (tree_view));

    selection = gtk_tree_view_get_selection (tree_view);
    model = gtk_tree_view_get_model (tree_view);

    if (gtk_tree_model_get_iter_first (model, &iter))
        gtk_tree_selection_select_iter (selection, &iter);
}


void
_moo_combo_box_select_first (GtkComboBox *combo)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    g_return_if_fail (GTK_IS_COMBO_BOX (combo));

    model = gtk_combo_box_get_model (combo);

    if (gtk_tree_model_get_iter_first (model, &iter))
        gtk_combo_box_set_active_iter (combo, &iter);
}


static void
moo_tree_helper_real_update_widgets (MooTreeHelper      *helper,
                                     GtkTreeModel       *model,
                                     GtkTreePath        *path)
{
    GtkTreeIter iter;

    if (!path)
    {
        if (helper->delete_btn)
            gtk_widget_set_sensitive (helper->delete_btn, FALSE);
        if (helper->up_btn)
            gtk_widget_set_sensitive (helper->up_btn, FALSE);
        if (helper->down_btn)
            gtk_widget_set_sensitive (helper->down_btn, FALSE);
    }
    else
    {
        int *indices;
        int n_rows;

        if (helper->delete_btn)
            gtk_widget_set_sensitive (helper->delete_btn, TRUE);

        indices = gtk_tree_path_get_indices (path);

        if (helper->up_btn)
            gtk_widget_set_sensitive (helper->up_btn, indices[0] != 0);

        n_rows = gtk_tree_model_iter_n_children (model, NULL);

        if (helper->down_btn)
            gtk_widget_set_sensitive (helper->down_btn, indices[0] != n_rows - 1);
    }

    if (path)
        gtk_tree_model_get_iter (model, &iter, path);

    g_signal_emit (helper, tree_signals[UPDATE_WIDGETS], 0,
                   model, path, path ? &iter : NULL);
}


void
_moo_tree_helper_update_model (MooTreeHelper *helper,
                               GtkTreeModel  *model,
                               GtkTreePath   *path)
{
    GtkTreePath *freeme = NULL;
    GtkTreeIter iter;

    g_return_if_fail (MOO_IS_TREE_HELPER (helper));
    g_return_if_fail (!model || GTK_IS_TREE_MODEL (model));

    if (!model)
        model = moo_tree_helper_get_model (helper);

    if (!path && moo_tree_helper_get_selected (helper, NULL, &iter))
        path = freeme = gtk_tree_model_get_path (model, &iter);

    if (path)
    {
        gtk_tree_model_get_iter (model, &iter, path);
        g_signal_emit (helper, tree_signals[UPDATE_MODEL], 0,
                       model, path, &iter);
    }

    gtk_tree_path_free (freeme);
}


static GtkTreeModel *
moo_tree_helper_get_model (MooTreeHelper *helper)
{
    switch (helper->type)
    {
        case TREE_VIEW:
            return gtk_tree_view_get_model (helper->widget);
        case COMBO_BOX:
            return gtk_combo_box_get_model (helper->widget);
    }

    g_return_val_if_reached (NULL);
}


static gboolean
moo_tree_helper_get_selected (MooTreeHelper *helper,
                              GtkTreeModel **model,
                              GtkTreeIter   *iter)
{
    GtkTreeSelection *selection;

    switch (helper->type)
    {
        case TREE_VIEW:
            selection = gtk_tree_view_get_selection (helper->widget);
            return gtk_tree_selection_get_selected (selection, model, iter);

        case COMBO_BOX:
            if (model)
                *model = gtk_combo_box_get_model (helper->widget);
            return gtk_combo_box_get_active_iter (helper->widget, iter);
    }

    g_return_val_if_reached (FALSE);
}


static int
iter_get_index (GtkTreeModel *model,
                GtkTreeIter  *iter)
{
    int index;
    GtkTreePath *path;

    path = gtk_tree_model_get_path (model, iter);

    if (!path)
        return -1;

    index = gtk_tree_path_get_indices(path)[0];

    gtk_tree_path_free (path);
    return index;
}


static void
moo_tree_helper_new_row (MooTreeHelper *helper)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GtkTreePath *path;
    int index;
    gboolean result;

    g_return_if_fail (helper->type == TREE_VIEW);

    selection = gtk_tree_view_get_selection (helper->widget);

    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        index = gtk_tree_model_iter_n_children (model, NULL);
    else
        index = iter_get_index (model, &iter) + 1;

    path = gtk_tree_path_new_from_indices (index, -1);

    g_signal_emit (helper, tree_signals[NEW_ROW], 0, model, path, &result);

    if (result && gtk_tree_model_get_iter (model, &iter, path))
    {
        gtk_tree_selection_select_iter (selection, &iter);
        helper->modified = TRUE;
    }

    gtk_tree_path_free (path);
}


static void
moo_tree_helper_delete_row (MooTreeHelper *helper)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GtkTreePath *path;
    gboolean result;

    g_return_if_fail (helper->type == TREE_VIEW);

    selection = gtk_tree_view_get_selection (helper->widget);

    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        g_critical ("oops");
        return;
    }

    path = gtk_tree_model_get_path (model, &iter);

    g_signal_emit (helper, tree_signals[DELETE_ROW], 0, model, path, &result);

    if (result)
        helper->modified = TRUE;

    if (result && (gtk_tree_model_get_iter (model, &iter, path) ||
                   (gtk_tree_path_prev (path) && gtk_tree_model_get_iter (model, &iter, path))))
        gtk_tree_selection_select_iter (selection, &iter);
    else
        _moo_tree_helper_update_widgets (helper);

    gtk_tree_path_free (path);
}


static void
moo_tree_helper_row_move (MooTreeHelper *helper,
                          gboolean       up)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path, *new_path;
    GtkTreeSelection *selection;
    gboolean result;

    g_return_if_fail (helper->type == TREE_VIEW);

    selection = gtk_tree_view_get_selection (helper->widget);

    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        g_critical ("oops");
        return;
    }

    path = gtk_tree_model_get_path (model, &iter);
    new_path = gtk_tree_path_copy (path);

    if (up && !gtk_tree_path_prev (new_path))
        g_return_if_reached ();
    else if (!up)
        gtk_tree_path_next (new_path);

    g_signal_emit (helper, tree_signals[MOVE_ROW], 0, model, path, new_path, &result);

    if (result)
    {
        helper->modified = TRUE;
        moo_tree_helper_real_update_widgets (helper, model, new_path);
    }

    gtk_tree_path_free (new_path);
    gtk_tree_path_free (path);
}


static void
moo_tree_helper_row_up (MooTreeHelper *helper)
{
    moo_tree_helper_row_move (helper, TRUE);
}


static void
moo_tree_helper_row_down (MooTreeHelper *helper)
{
    moo_tree_helper_row_move (helper, FALSE);
}


static void
_moo_tree_helper_init (G_GNUC_UNUSED MooTreeHelper *helper)
{
}


void
_moo_tree_helper_connect (MooTreeHelper *helper,
                          GtkWidget     *widget,
                          GtkWidget     *new_btn,
                          GtkWidget     *delete_btn,
                          GtkWidget     *up_btn,
                          GtkWidget     *down_btn)
{
    GtkTreeSelection *selection;

    g_return_if_fail (MOO_IS_TREE_HELPER (helper));
    g_return_if_fail (GTK_IS_TREE_VIEW (widget) || GTK_IS_COMBO_BOX (widget));
    g_return_if_fail (helper->widget == NULL);

    helper->widget = widget;

    if (GTK_IS_TREE_VIEW (widget))
        helper->type = TREE_VIEW;
    else
        helper->type = COMBO_BOX;

    helper->new_btn = new_btn;
    helper->delete_btn = delete_btn;
    helper->up_btn = up_btn;
    helper->down_btn = down_btn;

    g_signal_connect_swapped (widget, "destroy",
                              G_CALLBACK (gtk_object_destroy),
                              helper);

    switch (helper->type)
    {
        case TREE_VIEW:
            selection = gtk_tree_view_get_selection (helper->widget);
            g_signal_connect (selection, "changed",
                              G_CALLBACK (tree_selection_changed),
                              helper);
            break;

        case COMBO_BOX:
            g_signal_connect (widget, "changed",
                              G_CALLBACK (combo_changed),
                              helper);
            break;
    }

    if (new_btn)
        g_signal_connect_swapped (new_btn, "clicked",
                                  G_CALLBACK (moo_tree_helper_new_row),
                                  helper);
    if (delete_btn)
        g_signal_connect_swapped (delete_btn, "clicked",
                                  G_CALLBACK (moo_tree_helper_delete_row),
                                  helper);
    if (up_btn)
        g_signal_connect_swapped (up_btn, "clicked",
                                  G_CALLBACK (moo_tree_helper_row_up),
                                  helper);
    if (down_btn)
        g_signal_connect_swapped (down_btn, "clicked",
                                  G_CALLBACK (moo_tree_helper_row_down),
                                  helper);
}


MooTreeHelper *
_moo_tree_helper_new (GtkWidget *widget,
                      GtkWidget *new_btn,
                      GtkWidget *delete_btn,
                      GtkWidget *up_btn,
                      GtkWidget *down_btn)
{
    MooTreeHelper *helper;

    g_return_val_if_fail (GTK_IS_TREE_VIEW (widget) || GTK_IS_COMBO_BOX (widget), NULL);

    helper = g_object_new (MOO_TYPE_TREE_HELPER, (const char*) NULL);
    g_object_ref_sink (helper);

    _moo_tree_helper_connect (helper, widget, new_btn, delete_btn, up_btn, down_btn);

    return helper;
}


void
_moo_tree_helper_update_widgets (MooTreeHelper *helper)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    g_return_if_fail (MOO_IS_TREE_HELPER (helper));

    if (moo_tree_helper_get_selected (helper, &model, &iter))
    {
        GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
        moo_tree_helper_real_update_widgets (MOO_TREE_HELPER (helper), model, path);
        gtk_tree_path_free (path);
    }
    else
    {
        moo_tree_helper_real_update_widgets (MOO_TREE_HELPER (helper), NULL, NULL);
    }
}


static gboolean
set_value (GtkTreeModel  *model,
           GtkTreeIter   *iter,
           int            column,
           GValue        *value)
{
    GValue old_value;
    gboolean modified;

    old_value.g_type = 0;
    gtk_tree_model_get_value (model, iter, column, &old_value);

    modified = !_moo_value_equal (value, &old_value);

    if (GTK_IS_TREE_STORE (model))
        gtk_tree_store_set_value (GTK_TREE_STORE (model), iter, column, value);
    else if (GTK_IS_LIST_STORE (model))
        gtk_list_store_set_value (GTK_LIST_STORE (model), iter, column, value);

    g_value_unset (&old_value);
    return modified;
}

static gboolean
moo_tree_helper_set_valist (MooTreeHelper *helper,
                            GtkTreeModel  *model,
                            GtkTreeIter   *iter,
                            va_list	   var_args)
{
    int column;
    int n_columns;
    gboolean modified = FALSE;

    n_columns = gtk_tree_model_get_n_columns (model);
    column = va_arg (var_args, gint);

    while (column >= 0)
    {
        GType type;
        GValue value;
        char *error = NULL;

        value.g_type = 0;

        if (column >= n_columns)
        {
            g_warning ("invalid column number %d", column);
            break;
        }

        type = gtk_tree_model_get_column_type (model, column);
        g_value_init (&value, type);

        G_VALUE_COLLECT (&value, var_args, 0, &error);

        if (error)
        {
            g_warning ("%s", error);
            g_free (error);
            break;
        }

        modified = set_value (model, iter, column, &value) || modified;

        g_value_unset (&value);
        column = va_arg (var_args, gint);
    }

    helper->modified = helper->modified || modified;
    return modified;
}

gboolean
_moo_tree_helper_set (MooTreeHelper *helper,
                      GtkTreeIter   *iter,
                      ...)
{
      va_list args;
      GtkTreeModel *model;
      gboolean ret;

      g_return_val_if_fail (MOO_IS_TREE_HELPER (helper), FALSE);

      model = moo_tree_helper_get_model (helper);
      g_return_val_if_fail (GTK_IS_TREE_STORE (model) || GTK_IS_LIST_STORE (model), FALSE);

      va_start (args, iter);
      ret = moo_tree_helper_set_valist (helper, model, iter, args);
      va_end (args);

      return ret;
}


typedef struct {
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;
    GtkTreeModel *model;
} ExpanderData;

#define MOO_TYPE_EXPANDER_CELL              (moo_expander_cell_get_type ())
#define MOO_EXPANDER_CELL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOO_TYPE_EXPANDER_CELL, MooExpanderCell))
#define MOO_EXPANDER_CELL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_EXPANDER_CELL, MooExpanderCellClass))
#define MOO_IS_EXPANDER_CELL(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOO_TYPE_EXPANDER_CELL))
#define MOO_IS_EXPANDER_CELL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_EXPANDER_CELL))
#define MOO_EXPANDER_CELL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_EXPANDER_CELL, MooExpanderCellClass))

#define LEVEL_INDENTATION 12
#define EXPANDER_SIZE 8
#define CELL_XPAD(cell) MAX ((cell)->xpad, 1)
#define CELL_YPAD(cell) MAX ((cell)->ypad, 1)

typedef struct {
    GtkCellRenderer base;
    guint expanded : 1;
} MooExpanderCell;

typedef struct {
    GtkCellRendererClass base_class;
} MooExpanderCellClass;

MOO_DEFINE_TYPE_STATIC (MooExpanderCell, moo_expander_cell, GTK_TYPE_CELL_RENDERER)

static void
moo_expander_cell_init (MooExpanderCell *cell)
{
    cell->expanded = FALSE;
}

static void
moo_expander_cell_get_size (GtkCellRenderer      *cell,
                            GtkWidget            *widget,
                            GdkRectangle         *cell_area,
                            int                  *x_offset,
                            int                  *y_offset,
                            int                  *width_p,
                            int                  *height_p)
{
    int width, height;

    width  = CELL_XPAD (cell) * 2 + EXPANDER_SIZE;
    height = CELL_YPAD (cell) * 2 + EXPANDER_SIZE;

    if (cell_area)
    {
        if (x_offset)
        {
            float xalign = (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
                                (1.0f - cell->xalign) : cell->xalign;
            *x_offset = (int) (xalign * (cell_area->width - width));
            *x_offset = MAX (*x_offset, 0);
        }

        if (y_offset)
        {
            *y_offset = (int) (cell->yalign * (cell_area->height - height));
            *y_offset = MAX (*y_offset, 0);
        }
    }
    else
    {
        if (x_offset)
            *x_offset = 0;
        if (y_offset)
            *y_offset = 0;
    }

    if (width_p)
        *width_p = width;

    if (height_p)
        *height_p = height;
}

static void
moo_expander_cell_render (GtkCellRenderer      *cell,
                          GdkDrawable          *window,
                          GtkWidget            *widget,
                          G_GNUC_UNUSED GdkRectangle *background_area,
                          GdkRectangle         *cell_area,
                          GdkRectangle         *expose_area,
                          GtkCellRendererState  flags)
{
    MooExpanderCell *exp_cell = MOO_EXPANDER_CELL (cell);
    GdkRectangle pix_rect;
    GdkRectangle draw_rect;
    GtkStateType state;

    moo_expander_cell_get_size (cell, widget, cell_area,
                                &pix_rect.x,
                                &pix_rect.y,
                                &pix_rect.width,
                                &pix_rect.height);

    pix_rect.x += cell_area->x + CELL_XPAD (cell);
    pix_rect.y += cell_area->y + CELL_YPAD (cell);
    pix_rect.width  -= CELL_XPAD (cell) * 2;
    pix_rect.height -= CELL_YPAD (cell) * 2;

    if (!gdk_rectangle_intersect (cell_area, &pix_rect, &draw_rect) ||
        !gdk_rectangle_intersect (expose_area, &draw_rect, &draw_rect))
            return;

    state = GTK_WIDGET_STATE (widget);

    if (!cell->sensitive)
    {
        state = GTK_STATE_INSENSITIVE;
    }
    else if (flags & GTK_CELL_RENDERER_SELECTED)
    {
        if (GTK_WIDGET_HAS_FOCUS (widget))
            state = GTK_STATE_SELECTED;
        else
            state = GTK_STATE_ACTIVE;
    }
    else if (flags & GTK_CELL_RENDERER_PRELIT)
    {
        state = GTK_STATE_PRELIGHT;
    }

    gdk_draw_rectangle (window, widget->style->text_gc[state], FALSE,
                        pix_rect.x, pix_rect.y,
                        pix_rect.width, pix_rect.height);
    gdk_draw_line (window, widget->style->text_gc[state],
                   pix_rect.x + 2, pix_rect.y + pix_rect.height / 2,
                   pix_rect.x + pix_rect.width - 2, pix_rect.y + pix_rect.height / 2);
    if (!exp_cell->expanded)
        gdk_draw_line (window, widget->style->text_gc[state],
                       pix_rect.x + pix_rect.width / 2, pix_rect.y + 2,
                       pix_rect.x + pix_rect.width / 2, pix_rect.y + pix_rect.height - 2);
}

static void
moo_expander_cell_class_init (MooExpanderCellClass *klass)
{
    GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);

    cell_class->get_size = moo_expander_cell_get_size;
    cell_class->render = moo_expander_cell_render;
}

static void
moo_expander_cell_set_expanded (MooExpanderCell *cell,
                                gboolean         expanded)
{
    g_return_if_fail (MOO_IS_EXPANDER_CELL (cell));
    cell->expanded = expanded != 0;
}

static void
expander_cell_data_func (G_GNUC_UNUSED GtkTreeViewColumn *column,
                         GtkCellRenderer   *cell,
                         GtkTreeModel      *model,
                         GtkTreeIter       *iter,
                         GtkTreeView       *tree_view)
{
    gboolean has_children;
    gboolean expanded;
    GtkTreePath *path;

    has_children = gtk_tree_model_iter_has_child (model, iter);
    g_object_set (cell, "visible", has_children, NULL);

    if (!has_children)
        return;

    path = gtk_tree_model_get_path (model, iter);
    expanded = gtk_tree_view_row_expanded (tree_view, path);
    moo_expander_cell_set_expanded (MOO_EXPANDER_CELL (cell), expanded);
    gtk_tree_path_free (path);
}

static void
row_has_child_toggled (GtkTreeModel *tree_model,
                       GtkTreePath  *path,
                       GtkTreeIter  *iter)
{
    gtk_tree_model_row_changed (tree_model, path, iter);
}

static void
expander_data_disconnect_model (ExpanderData *data)
{
    if (data->model)
    {
        g_signal_handlers_disconnect_by_func (data->model,
                                              (gpointer) row_has_child_toggled,
                                              NULL);
        g_object_unref (data->model);
        data->model = NULL;
    }
}

static void
tree_view_model_changed (GtkTreeView *treeview,
                         G_GNUC_UNUSED GParamSpec *pspec,
                         ExpanderData *data)
{
    expander_data_disconnect_model (data);

    if ((data->model = gtk_tree_view_get_model (treeview)))
    {
        g_object_ref (data->model);
        g_signal_connect (data->model, "row-has-child-toggled",
                          G_CALLBACK (row_has_child_toggled), NULL);
    }
}

static void
expander_data_free (ExpanderData *data)
{
    if (data)
    {
        if (data->column)
            g_object_unref (data->column);
        if (data->cell)
            g_object_unref (data->cell);
        expander_data_disconnect_model (data);
        g_slice_free (ExpanderData, data);
    }
}

static gboolean
tree_view_button_press (GtkTreeView    *treeview,
                        GdkEventButton *event,
                        ExpanderData   *data)
{
    GtkTreePath *path = NULL;
    GtkTreeViewColumn *column;
    int cell_x, cell_y;
    int start_pos, width;
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (event->button != 1 || event->type != GDK_BUTTON_PRESS)
        goto out;

    if (!gtk_tree_view_get_path_at_pos (treeview,
                                        (int) event->x, (int) event->y,
                                        &path, &column,
                                        &cell_x, &cell_y))
        goto out;

    if (column != data->column)
        goto out;

    model = gtk_tree_view_get_model (treeview);
    if (!gtk_tree_model_get_iter (model, &iter, path) ||
        !gtk_tree_model_iter_has_child (model, &iter))
            goto out;

    gtk_tree_view_column_cell_set_cell_data (column, model, &iter, FALSE, FALSE);
    gtk_tree_view_column_cell_get_position (column, data->cell, &start_pos, &width);
    start_pos += (gtk_tree_path_get_depth (path) - 1) * LEVEL_INDENTATION + CELL_XPAD (data->cell);
    width += 2 * CELL_XPAD (data->cell);

    if (cell_x < start_pos || cell_x >= start_pos + width)
        goto out;

    if (gtk_tree_view_row_expanded (treeview, path))
        gtk_tree_view_collapse_row (treeview, path);
    else
        gtk_tree_view_expand_row (treeview, path, FALSE);

    gtk_tree_path_free (path);
    return TRUE;

out:
    if (path)
        gtk_tree_path_free (path);
    return FALSE;
}

static void
tree_view_row_activated (GtkTreeView *treeview,
                         GtkTreePath *path)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_tree_view_get_model (treeview);

    if (!gtk_tree_model_get_iter (model, &iter, path) ||
        !gtk_tree_model_iter_has_child (model, &iter))
            return;

    g_signal_stop_emission_by_name (treeview, "row-activated");

    if (gtk_tree_view_row_expanded (treeview, path))
        gtk_tree_view_collapse_row (treeview, path);
    else
        gtk_tree_view_expand_row (treeview, path, FALSE);
}

void
_moo_tree_view_setup_expander (GtkTreeView       *tree_view,
                               GtkTreeViewColumn *column)
{
    GtkCellRenderer *cell;
    ExpanderData *data;

    g_return_if_fail (GTK_IS_TREE_VIEW (tree_view));
    g_return_if_fail (GTK_IS_TREE_VIEW_COLUMN (column));

    g_object_set (tree_view,
                  "show-expanders", FALSE,
                  "level-indentation", LEVEL_INDENTATION,
                  NULL);

    cell = g_object_new (MOO_TYPE_EXPANDER_CELL, (const char*) NULL);
    gtk_tree_view_column_pack_start (column, cell, FALSE);
    gtk_tree_view_column_set_cell_data_func (column, cell,
                                             (GtkTreeCellDataFunc) expander_cell_data_func,
                                             tree_view, NULL);

    data = g_slice_new0 (ExpanderData);
    data->column = g_object_ref (column);
    data->cell = g_object_ref (cell);
    g_object_set_data_full (G_OBJECT (tree_view), "moo-tree-view-expander-data",
                            data, (GDestroyNotify) expander_data_free);

    g_signal_connect (tree_view, "button-press-event", G_CALLBACK (tree_view_button_press), data);
    g_signal_connect (tree_view, "row-activated", G_CALLBACK (tree_view_row_activated), NULL);
    g_signal_connect (tree_view, "notify::model", G_CALLBACK (tree_view_model_changed), data);
    tree_view_model_changed (tree_view, NULL, data);
}
