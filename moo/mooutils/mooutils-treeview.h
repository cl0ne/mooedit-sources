/*
 *   mooutils-treeview.h
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

#ifndef MOO_UTILS_TREE_VIEW_H
#define MOO_UTILS_TREE_VIEW_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_TREE_HELPER              (_moo_tree_helper_get_type ())
#define MOO_TREE_HELPER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_TREE_HELPER, MooTreeHelper))
#define MOO_TREE_HELPER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_TREE_HELPER, MooTreeHelperClass))
#define MOO_IS_TREE_HELPER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_TREE_HELPER))
#define MOO_IS_TREE_HELPER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_TREE_HELPER))
#define MOO_TREE_HELPER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_TREE_HELPER, MooTreeHelperClass))

typedef struct _MooTreeHelper MooTreeHelper;
typedef struct _MooTreeHelperClass MooTreeHelperClass;

struct _MooTreeHelper {
    GtkObject parent;

    gboolean modified;
    int type;
    gpointer widget;

    GtkWidget *new_btn;
    GtkWidget *delete_btn;
    GtkWidget *up_btn;
    GtkWidget *down_btn;
};

struct _MooTreeHelperClass {
    GtkObjectClass parent_class;

    gboolean    (*new_row)          (MooTreeHelper  *helper,
                                     GtkTreeModel   *model,
                                     GtkTreePath    *path);
    gboolean    (*delete_row)       (MooTreeHelper  *helper,
                                     GtkTreeModel   *model,
                                     GtkTreePath    *path);
    gboolean    (*move_row)         (MooTreeHelper  *helper,
                                     GtkTreeModel   *model,
                                     GtkTreePath    *old_path,
                                     GtkTreePath    *new_path);

    void        (*update_widgets)   (MooTreeHelper  *helper,
                                     GtkTreeModel   *model,
                                     GtkTreePath    *path,
                                     GtkTreeIter    *iter);
    void        (*update_model)     (MooTreeHelper  *helper,
                                     GtkTreeModel   *model,
                                     GtkTreePath    *path,
                                     GtkTreeIter    *iter);
};


GType            _moo_tree_helper_get_type          (void) G_GNUC_CONST;

MooTreeHelper   *_moo_tree_helper_new               (GtkWidget          *treeview_or_combo,
                                                     GtkWidget          *new_btn,
                                                     GtkWidget          *delete_btn,
                                                     GtkWidget          *up_btn,
                                                     GtkWidget          *down_btn);
void             _moo_tree_helper_connect           (MooTreeHelper      *helper,
                                                     GtkWidget          *treeview_or_combo,
                                                     GtkWidget          *new_btn,
                                                     GtkWidget          *delete_btn,
                                                     GtkWidget          *up_btn,
                                                     GtkWidget          *down_btn);
void             _moo_tree_helper_update_model      (MooTreeHelper      *helper,
                                                     GtkTreeModel       *model,
                                                     GtkTreePath        *path);
void             _moo_tree_helper_update_widgets    (MooTreeHelper      *helper);

void             _moo_tree_helper_set_modified      (MooTreeHelper      *helper,
                                                     gboolean            modified);
gboolean         _moo_tree_helper_get_modified      (MooTreeHelper      *helper);

gboolean         _moo_tree_helper_set               (MooTreeHelper      *helper,
                                                     GtkTreeIter        *iter,
                                                     ...);

void             _moo_tree_view_select_first        (GtkTreeView        *tree_view);
void             _moo_combo_box_select_first        (GtkComboBox        *combo);

void             _moo_tree_view_setup_expander      (GtkTreeView        *tree_view,
                                                     GtkTreeViewColumn  *column);


G_END_DECLS

#endif /* MOO_UTILS_TREE_VIEW_H */
