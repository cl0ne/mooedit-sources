/*
 *   mootreeview.c
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

#include "moofileview/mootreeview.h"
#include "marshals.h"
#include "mooutils/mooutils-gobject.h"
#include <gtk/gtk.h>


typedef MooTreeViewChild Child;


static void     moo_tree_view_finalize      (GObject            *object);
static void     moo_tree_view_set_property  (GObject            *object,
                                             guint               prop_id,
                                             const GValue       *value,
                                             GParamSpec         *pspec);
static void     moo_tree_view_get_property  (GObject            *object,
                                             guint               prop_id,
                                             GValue             *value,
                                             GParamSpec         *pspec);

static void     _moo_tree_view_set_model    (MooTreeView        *view,
                                             GtkTreeModel       *model);

static void     disconnect_child            (MooTreeView        *view);
static Child   *find_child                  (MooTreeView        *view,
                                             GtkWidget          *real_view);

static Child   *child_new                   (MooTreeView        *view,
                                             GtkWidget          *real_view);
static void     child_free                  (Child              *child);
static void     child_set_model             (Child              *child,
                                             GtkTreeModel       *model);

static void     child_row_activated         (Child              *child,
                                             const GtkTreePath  *path);
static void     child_selection_changed     (Child              *child);
static gboolean child_key_press             (Child              *child,
                                             GdkEventKey        *event);
static gboolean child_button_press          (Child              *child,
                                             GdkEventButton     *event);


/* MOO_TYPE_TREE_VIEW */
G_DEFINE_TYPE (MooTreeView, _moo_tree_view, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_MODEL
};

enum {
    BUTTON_PRESS_EVENT,
    KEY_PRESS_EVENT,
    ROW_ACTIVATED,
    SELECTION_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


static void
_moo_tree_view_class_init (MooTreeViewClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = moo_tree_view_finalize;
    gobject_class->set_property = moo_tree_view_set_property;
    gobject_class->get_property = moo_tree_view_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_MODEL,
                                     g_param_spec_object ("model",
                                             "model",
                                             "model",
                                             GTK_TYPE_TREE_MODEL,
                                             (GParamFlags) G_PARAM_READWRITE));

    signals[BUTTON_PRESS_EVENT] =
            g_signal_new ("button-press-event",
                          G_TYPE_FROM_CLASS (gobject_class),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTreeViewClass, button_press_event),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOLEAN__OBJECT_BOXED,
                          G_TYPE_BOOLEAN, 2,
                          GTK_TYPE_WIDGET,
                          GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

    signals[KEY_PRESS_EVENT] =
            g_signal_new ("key-press-event",
                          G_TYPE_FROM_CLASS (gobject_class),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTreeViewClass, key_press_event),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOLEAN__OBJECT_BOXED,
                          G_TYPE_BOOLEAN, 2,
                          GTK_TYPE_WIDGET,
                          GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

    signals[ROW_ACTIVATED] =
            g_signal_new ("row-activated",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTreeViewClass, row_activated),
                          NULL, NULL,
                          _moo_marshal_VOID__BOXED,
                          G_TYPE_NONE, 1,
                          GTK_TYPE_TREE_PATH | G_SIGNAL_TYPE_STATIC_SCOPE);

    signals[SELECTION_CHANGED] =
            g_signal_new ("selection-changed",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTreeViewClass, selection_changed),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);
}


static void
_moo_tree_view_init (G_GNUC_UNUSED MooTreeView *view)
{
}


static void
moo_tree_view_finalize  (GObject      *object)
{
    MooTreeView *view = MOO_TREE_VIEW (object);

    if (view->model)
        g_object_unref (view->model);

    if (view->active)
        disconnect_child (view);

    g_slist_foreach (view->children, (GFunc) child_free, NULL);
    g_slist_free (view->children);

    G_OBJECT_CLASS (_moo_tree_view_parent_class)->finalize (object);
}


MooTreeView *
_moo_tree_view_new (GtkTreeModel *model)
{
    g_return_val_if_fail (!model || GTK_IS_TREE_MODEL (model), NULL);
    return MOO_TREE_VIEW (g_object_new (MOO_TYPE_TREE_VIEW, "model", model, (const char*) NULL));
}


static void
moo_tree_view_set_property (GObject        *object,
                            guint           prop_id,
                            const GValue   *value,
                            GParamSpec     *pspec)
{
    MooTreeView *view = MOO_TREE_VIEW (object);

    switch (prop_id)
    {
        case PROP_MODEL:
            _moo_tree_view_set_model (view, g_value_get_object (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
moo_tree_view_get_property (GObject        *object,
                            guint           prop_id,
                            GValue         *value,
                            GParamSpec     *pspec)
{
    MooTreeView *view = MOO_TREE_VIEW (object);

    switch (prop_id)
    {
        case PROP_MODEL:
            g_value_set_object (value, view->model);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


GtkTreeModel *
_moo_tree_view_get_model (gpointer view)
{
    if (MOO_IS_TREE_VIEW (view))
        return MOO_TREE_VIEW(view)->model;
    else if (GTK_IS_TREE_VIEW (view))
        return gtk_tree_view_get_model (view);
    else if (MOO_IS_ICON_VIEW (view))
        return _moo_icon_view_get_model (view);
    else
        g_return_val_if_reached (NULL);
}


static void
_moo_tree_view_set_model (MooTreeView  *view,
                          GtkTreeModel *model)
{
    g_return_if_fail (MOO_IS_TREE_VIEW (view));
    g_return_if_fail (!model || GTK_IS_TREE_MODEL (model));

    if (view->model == model)
        return;

    if (view->model)
        g_object_unref (view->model);
    view->model = model;
    if (view->model)
        g_object_ref (view->model);

    if (view->active)
        child_set_model (view->active, model);

    g_object_notify (G_OBJECT (view), "model");
}


void
_moo_tree_view_add (MooTreeView    *view,
                    GtkWidget      *real_view)
{
    Child *child;

    g_return_if_fail (MOO_IS_TREE_VIEW (view));
    g_return_if_fail (GTK_IS_TREE_VIEW (real_view) || MOO_IS_ICON_VIEW (real_view));
    g_return_if_fail (!find_child (view, real_view));

    child = child_new (view, real_view);
    view->children = g_slist_prepend (view->children, child);
}


void
_moo_tree_view_set_active (MooTreeView    *view,
                           GtkWidget      *real_view)
{
    Child *child;

    g_return_if_fail (MOO_IS_TREE_VIEW (view));

    child = real_view ? find_child (view, real_view) : NULL;
    g_return_if_fail (real_view == NULL || child != NULL);

    if (view->active == child)
        return;

    /* XXX selection and cursor */

    if (view->active)
        disconnect_child (view);

    view->active = child;

    if (child)
        child_set_model (child, view->model);
}


static void
disconnect_child (MooTreeView *view)
{
    if (view->active)
    {
        child_set_model (view->active, NULL);
        view->active = NULL;
    }
}


static Child*
find_child (MooTreeView    *view,
            GtkWidget      *real_view)
{
    GSList *l;

    for (l = view->children; l != NULL; l = l->next)
    {
        Child *child = l->data;
        if (child->widget == real_view)
            return child;
    }

    return NULL;
}


static Child *
child_new (MooTreeView  *view,
           GtkWidget    *widget)
{
    Child *child = g_new0 (Child, 1);

    if (GTK_IS_TREE_VIEW (widget))
    {
        child->type = MOO_TREE_VIEW_TREE;
        child->u.tree.view = GTK_TREE_VIEW (widget);
        child->u.tree.selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

        g_signal_connect_swapped (child->u.tree.selection, "changed",
                                  G_CALLBACK (child_selection_changed), child);
    }
    else if (MOO_IS_ICON_VIEW (widget))
    {
        child->type = MOO_TREE_VIEW_ICON;
        child->u.icon.view = MOO_ICON_VIEW (widget);

        g_signal_connect_swapped (widget, "selection-changed",
                                  G_CALLBACK (child_selection_changed), child);
    }
    else
    {
        g_free (child);
        g_return_val_if_reached (NULL);
    }

    g_signal_connect_swapped (widget, "row-activated",
                              G_CALLBACK (child_row_activated), child);
    g_signal_connect_swapped (widget, "key-press-event",
                              G_CALLBACK (child_key_press), child);
    g_signal_connect_swapped (widget, "button-press-event",
                              G_CALLBACK (child_button_press), child);

    child->parent = view;
    child->widget = g_object_ref (widget);
    return child;
}


static void
child_free (Child *child)
{
    if (child->widget)
    {
        g_signal_handlers_disconnect_by_func (child->widget,
                                              (gpointer) child_row_activated,
                                              child);
        g_signal_handlers_disconnect_by_func (child->widget,
                                              (gpointer) child_selection_changed,
                                              child);
        g_signal_handlers_disconnect_by_func (child->widget,
                                              (gpointer) child_key_press,
                                              child);
        g_signal_handlers_disconnect_by_func (child->widget,
                                              (gpointer) child_button_press,
                                              child);
        g_object_unref (child->widget);
    }

    g_free (child);
}


static void
child_set_model (Child          *child,
                 GtkTreeModel   *model)
{
    if (child->type == MOO_TREE_VIEW_TREE)
        gtk_tree_view_set_model (child->u.tree.view, model);
    else if (child->type == MOO_TREE_VIEW_ICON)
        _moo_icon_view_set_model (child->u.icon.view, model);
    else
        g_return_if_reached ();
}


static void
child_row_activated (Child              *child,
                     const GtkTreePath  *path)
{
    if (child->parent->active == child)
        g_signal_emit (child->parent, signals[ROW_ACTIVATED], 0, path);
}


static void
child_selection_changed (Child *child)
{
    if (child->parent->active == child)
        g_signal_emit (child->parent, signals[SELECTION_CHANGED], 0);
}


static gboolean
child_key_press (Child       *child,
                 GdkEventKey *event)
{
    if (child->parent->active == child)
    {
        gboolean result = FALSE;
        g_signal_emit (child->parent, signals[KEY_PRESS_EVENT], 0,
                       child->widget, event, &result);
        return result;
    }
    else
    {
        return FALSE;
    }
}


static gboolean
child_button_press (Child          *child,
                    GdkEventButton *event)
{
    if (child->parent->active == child)
    {
        gboolean result = FALSE;
        g_signal_emit (child->parent, signals[BUTTON_PRESS_EVENT], 0,
                       child->widget, event, &result);
        return result;
    }
    else
    {
        return FALSE;
    }
}


gboolean
_moo_tree_view_selection_is_empty (MooTreeView *view)
{
    Child *child;

    g_return_val_if_fail (MOO_IS_TREE_VIEW (view), TRUE);

    if (!(child = view->active))
        return TRUE;

    switch (child->type)
    {
        case MOO_TREE_VIEW_TREE:
            return !gtk_tree_selection_count_selected_rows (child->u.tree.selection);

        case MOO_TREE_VIEW_ICON:
            return !_moo_icon_view_get_selected (child->u.icon.view, NULL);
    }

    g_return_val_if_reached (TRUE);
}


gboolean
_moo_tree_view_path_is_selected (MooTreeView    *view,
                                 GtkTreePath    *path)
{
    Child *child;

    g_return_val_if_fail (MOO_IS_TREE_VIEW (view), FALSE);

    if (!(child = view->active))
        return FALSE;

    switch (child->type)
    {
        case MOO_TREE_VIEW_TREE:
            return gtk_tree_selection_path_is_selected (child->u.tree.selection, path);

        case MOO_TREE_VIEW_ICON:
            return _moo_icon_view_path_is_selected (child->u.icon.view, path);
    }

    g_return_val_if_reached (FALSE);
}


void
_moo_tree_view_unselect_all (MooTreeView *view)
{
    Child *child;

    g_return_if_fail (MOO_IS_TREE_VIEW (view));

    if (!(child = view->active))
        return;

    switch (child->type)
    {
        case MOO_TREE_VIEW_TREE:
            gtk_tree_selection_unselect_all (child->u.tree.selection);
            break;

        case MOO_TREE_VIEW_ICON:
            _moo_icon_view_unselect_all (child->u.icon.view);
            break;
    }
}


GList *
_moo_tree_view_get_selected_rows (MooTreeView *view)
{
    Child *child;

    g_return_val_if_fail (MOO_IS_TREE_VIEW (view), NULL);

    if (!(child = view->active))
        return NULL;

    switch (child->type)
    {
        case MOO_TREE_VIEW_TREE:
            return gtk_tree_selection_get_selected_rows (child->u.tree.selection, NULL);

        case MOO_TREE_VIEW_ICON:
            return _moo_icon_view_get_selected_rows (child->u.icon.view);
    }

    g_return_val_if_reached (NULL);
}


GtkTreePath *
_moo_tree_view_get_selected_path (MooTreeView *view)
{
    Child *child;
    GtkTreeModel *model;
    GtkTreeIter iter;

    g_return_val_if_fail (MOO_IS_TREE_VIEW (view), NULL);

    if (!(child = view->active))
        return NULL;

    switch (child->type)
    {
        case MOO_TREE_VIEW_TREE:
            if (gtk_tree_selection_get_selected (child->u.tree.selection, &model, &iter))
                return gtk_tree_model_get_path (model, &iter);
            else
                return NULL;

        case MOO_TREE_VIEW_ICON:
            return _moo_icon_view_get_selected_path (child->u.icon.view);
    }

    g_return_val_if_reached (NULL);
}


/* window coordinates */
gboolean
_moo_tree_view_get_path_at_pos (gpointer        view,
                                int             x,
                                int             y,
                                GtkTreePath   **path)
{
    if (MOO_IS_TREE_VIEW (view))
    {
        Child *child;

        if (!(child = MOO_TREE_VIEW(view)->active))
            return FALSE;

        switch (child->type)
        {
            case MOO_TREE_VIEW_TREE:
                return gtk_tree_view_get_path_at_pos (child->u.tree.view, x, y, path,
                                                      NULL, NULL, NULL);

            case MOO_TREE_VIEW_ICON:
                return _moo_icon_view_get_path_at_pos (child->u.icon.view, x, y, path,
                                                       NULL, NULL, NULL);
        }

        g_return_val_if_reached (FALSE);
    }
    else if (GTK_IS_TREE_VIEW (view))
    {
        return gtk_tree_view_get_path_at_pos (view, x, y, path,
                                              NULL, NULL, NULL);
    }
    else if (MOO_IS_ICON_VIEW (view))
    {
        return _moo_icon_view_get_path_at_pos (view, x, y, path,
                                               NULL, NULL, NULL);
    }
    else
    {
        g_return_val_if_reached (FALSE);
    }
}


void
_moo_tree_view_set_cursor (MooTreeView    *view,
                           GtkTreePath    *path,
                           gboolean        start_editing)
{
    Child *child;

    g_return_if_fail (MOO_IS_TREE_VIEW (view));

    if (!(child = view->active))
        return;

    switch (child->type)
    {
        case MOO_TREE_VIEW_TREE:
            gtk_tree_view_set_cursor (child->u.tree.view, path, NULL, start_editing);
            break;

        case MOO_TREE_VIEW_ICON:
            _moo_icon_view_set_cursor (child->u.icon.view, path, FALSE);
            break;
    }
}


void
_moo_tree_view_scroll_to_cell (MooTreeView    *view,
                               GtkTreePath    *path)
{
    Child *child;

    g_return_if_fail (MOO_IS_TREE_VIEW (view));
    g_return_if_fail (path != NULL);

    if (!(child = view->active))
        return;

    switch (child->type)
    {
        case MOO_TREE_VIEW_TREE:
            gtk_tree_view_scroll_to_cell (child->u.tree.view, path, NULL, FALSE, 0, 0);
            break;

        case MOO_TREE_VIEW_ICON:
            _moo_icon_view_scroll_to_cell (child->u.icon.view, path);
            break;
    }
}


void
_moo_tree_view_selected_foreach (MooTreeView    *view,
                                 MooTreeViewForeachFunc func,
                                 gpointer        data)
{
    Child *child;

    g_return_if_fail (MOO_IS_TREE_VIEW (view));
    g_return_if_fail (func != NULL);

    if (!(child = view->active))
        return;

    switch (child->type)
    {
        case MOO_TREE_VIEW_TREE:
            gtk_tree_selection_selected_foreach (child->u.tree.selection, func, data);
            break;

        case MOO_TREE_VIEW_ICON:
            _moo_icon_view_selected_foreach (child->u.icon.view, func, data);
            break;
    }
}


void
_moo_tree_view_set_drag_dest_row (gpointer        view,
                                  GtkTreePath    *path)
{
    if (GTK_IS_TREE_VIEW (view))
        gtk_tree_view_set_drag_dest_row (view, path,
                                         GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
    else if (MOO_IS_ICON_VIEW (view))
        _moo_icon_view_set_drag_dest_row (view, path);
    else
        g_return_if_reached ();
}


void
_moo_tree_view_widget_to_abs_coords (gpointer        view,
                                     int             wx,
                                     int             wy,
                                     int            *absx,
                                     int            *absy)
{
    if (GTK_IS_TREE_VIEW (view))
        gtk_tree_view_convert_widget_to_bin_window_coords (view, wx, wy, absx, absy);
    else if (MOO_IS_ICON_VIEW (view))
        _moo_icon_view_widget_to_abs_coords (view, wx, wy, absx, absy);
    else
        g_return_if_reached ();
}


#if 0
void
_moo_tree_view_abs_to_widget_coords (gpointer        view,
                                     int             absx,
                                     int             absy,
                                     int            *wx,
                                     int            *wy)
{
    if (GTK_IS_TREE_VIEW (view))
        gtk_tree_view_tree_to_widget_coords (view, absx, absy, wx, wy);
    else if (MOO_IS_ICON_VIEW (view))
        _moo_icon_view_abs_to_widget_coords (view, absx, absy, wx, wy);
    else
        g_return_if_reached ();
}
#endif
