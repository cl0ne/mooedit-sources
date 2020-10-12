/*
 *   mootreeview.h
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

#ifndef MOO_TREE_VIEW_H
#define MOO_TREE_VIEW_H

#include <gtk/gtk.h>
#include "moofileview/mooiconview.h"

G_BEGIN_DECLS


#define MOO_TYPE_TREE_VIEW              (_moo_tree_view_get_type ())
#define MOO_TREE_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_TREE_VIEW, MooTreeView))
#define MOO_TREE_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_TREE_VIEW, MooTreeViewClass))
#define MOO_IS_TREE_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_TREE_VIEW))
#define MOO_IS_TREE_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_TREE_VIEW))
#define MOO_TREE_VIEW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_TREE_VIEW, MooTreeViewClass))


typedef struct _MooTreeView         MooTreeView;
typedef struct _MooTreeViewClass    MooTreeViewClass;
typedef struct _MooTreeViewChild    MooTreeViewChild;
typedef struct _MooTreeViewTree     MooTreeViewTree;
typedef struct _MooTreeViewIcon     MooTreeViewIcon;

typedef enum {
    MOO_TREE_VIEW_TREE,
    MOO_TREE_VIEW_ICON
} MooTreeViewChildType;

struct _MooTreeViewTree
{
    GtkTreeView *view;
    GtkTreeSelection *selection;
};

struct _MooTreeViewIcon
{
    MooIconView *view;
};

struct _MooTreeViewChild
{
    MooTreeViewChildType type;
    MooTreeView *parent;
    GtkWidget *widget;
    union {
        MooTreeViewTree tree;
        MooTreeViewIcon icon;
    } u;
};

struct _MooTreeView
{
    GObject object;
    GSList *children;
    MooTreeViewChild *active;
    GtkTreeModel *model;
};

struct _MooTreeViewClass
{
    GObjectClass object_class;

    gboolean (* button_press_event) (MooTreeView        *view,
                                     GtkWidget          *widget,
                                     GdkEventButton     *event);
    gboolean (* key_press_event)    (MooTreeView        *view,
                                     GtkWidget          *widget,
                                     GdkEventKey        *event);

    void     (* row_activated)      (MooTreeView        *view,
                                     const GtkTreePath  *path);
    void     (* selection_changed)  (MooTreeView        *view);
};


typedef void (*MooTreeViewForeachFunc)  (GtkTreeModel       *model,
                                         GtkTreePath        *path,
                                         GtkTreeIter        *iter,
                                         gpointer            data);


GType           _moo_tree_view_get_type             (void) G_GNUC_CONST;

MooTreeView    *_moo_tree_view_new                  (GtkTreeModel   *model);

void            _moo_tree_view_add                  (MooTreeView    *view,
                                                     GtkWidget      *real_view);
void            _moo_tree_view_set_active           (MooTreeView    *view,
                                                     GtkWidget      *real_view);

GtkTreeModel   *_moo_tree_view_get_model            (gpointer        view);

gboolean        _moo_tree_view_selection_is_empty   (MooTreeView    *view);
gboolean        _moo_tree_view_path_is_selected     (MooTreeView    *view,
                                                     GtkTreePath    *path);
void            _moo_tree_view_unselect_all         (MooTreeView    *view);
GList          *_moo_tree_view_get_selected_rows    (MooTreeView    *view);
GtkTreePath    *_moo_tree_view_get_selected_path    (MooTreeView    *view);
void            _moo_tree_view_selected_foreach     (MooTreeView    *view,
                                                     MooTreeViewForeachFunc func,
                                                     gpointer        data);

/* window coordinates */
gboolean        _moo_tree_view_get_path_at_pos      (gpointer        view,
                                                     int             x,
                                                     int             y,
                                                     GtkTreePath   **path);

void            _moo_tree_view_widget_to_abs_coords (gpointer        view,
                                                     int             wx,
                                                     int             wy,
                                                     int            *absx,
                                                     int            *absy);

void            _moo_tree_view_set_cursor           (MooTreeView    *view,
                                                     GtkTreePath    *path,
                                                     gboolean        start_editing);
void            _moo_tree_view_scroll_to_cell       (MooTreeView    *view,
                                                     GtkTreePath    *path);

void            _moo_tree_view_set_drag_dest_row    (gpointer        view,
                                                     GtkTreePath    *path);


G_END_DECLS

#endif /* MOO_TREE_VIEW_H */
