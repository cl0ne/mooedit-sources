/*
 *   mooiconview.h
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

#ifndef MOO_ICON_VIEW_H
#define MOO_ICON_VIEW_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_ICON_VIEW              (_moo_icon_view_get_type ())
#define MOO_ICON_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_ICON_VIEW, MooIconView))
#define MOO_ICON_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_ICON_VIEW, MooIconViewClass))
#define MOO_IS_ICON_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_ICON_VIEW))
#define MOO_IS_ICON_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_ICON_VIEW))
#define MOO_ICON_VIEW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_ICON_VIEW, MooIconViewClass))


typedef struct _MooIconView        MooIconView;
typedef struct _MooIconViewPrivate MooIconViewPrivate;
typedef struct _MooIconViewClass   MooIconViewClass;

struct _MooIconView
{
    GtkVBox             vbox;
    MooIconViewPrivate *priv;
};

struct _MooIconViewClass
{
    GtkVBoxClass        vbox_class;

    void    (*set_scroll_adjustments)   (GtkWidget          *widget,
                                         GtkAdjustment      *hadjustment,
                                         GtkAdjustment      *vadjustment);

    void    (*row_activated)            (MooIconView        *iconview,
                                         const GtkTreePath  *path);
    void    (*selection_changed)        (MooIconView        *iconview);
    void    (*cursor_moved)             (MooIconView        *iconview,
                                         const GtkTreePath  *path);
};

typedef enum {
    MOO_ICON_VIEW_CELL_PIXBUF,
    MOO_ICON_VIEW_CELL_TEXT
} MooIconViewCell;


typedef void (*MooIconCellDataFunc)     (MooIconView        *view,
                                         GtkCellRenderer    *cell,
                                         GtkTreeModel       *model,
                                         GtkTreeIter        *iter,
                                         gpointer            data);
typedef void (*MooIconViewForeachFunc)  (GtkTreeModel       *model,
                                         GtkTreePath        *path,
                                         GtkTreeIter        *iter,
                                         gpointer            data);


GType         _moo_icon_view_get_type         (void) G_GNUC_CONST;

GtkWidget    *_moo_icon_view_new              (GtkTreeModel *model);

GtkTreeModel *_moo_icon_view_get_model        (MooIconView    *view);
void          _moo_icon_view_set_model        (MooIconView    *view,
                                               GtkTreeModel   *model);

GtkCellRenderer *_moo_icon_view_get_cell      (MooIconView    *view,
                                               MooIconViewCell cell_type);

void          _moo_icon_view_set_cell_data_func(MooIconView    *view,
                                               MooIconViewCell cell,
                                               MooIconCellDataFunc func,
                                               gpointer        func_data,
                                               GDestroyNotify  destroy);

/* TreeView-like selection and cursor interface */
void        _moo_icon_view_set_selection_mode   (MooIconView        *view,
                                                 GtkSelectionMode    mode);
gboolean    _moo_icon_view_get_selected         (MooIconView        *view,
                                                 GtkTreeIter        *iter);
GtkTreePath *_moo_icon_view_get_selected_path   (MooIconView        *view);
void        _moo_icon_view_selected_foreach     (MooIconView        *view,
                                                 MooIconViewForeachFunc func,
                                                 gpointer data);
GList*      _moo_icon_view_get_selected_rows    (MooIconView        *view);
gboolean    _moo_icon_view_path_is_selected     (MooIconView        *view,
                                                 GtkTreePath        *path);
void        _moo_icon_view_select_all           (MooIconView        *view);
void        _moo_icon_view_unselect_all         (MooIconView        *view);

void        _moo_icon_view_scroll_to_cell       (MooIconView        *view,
                                                 GtkTreePath        *path);
void        _moo_icon_view_set_cursor           (MooIconView        *view,
                                                 GtkTreePath        *path,
                                                 gboolean            start_editing);

void        _moo_icon_view_widget_to_abs_coords (MooIconView        *view,
                                                 int                 wx,
                                                 int                 wy,
                                                 int                *absx,
                                                 int                *absy);
void        _moo_icon_view_abs_to_widget_coords (MooIconView        *view,
                                                 int                 absx,
                                                 int                 absy,
                                                 int                *wx,
                                                 int                *wy);

gboolean    _moo_icon_view_get_path_at_pos      (MooIconView        *view,
                                                 int                 x,
                                                 int                 y,
                                                 GtkTreePath       **path,
                                                 MooIconViewCell    *cell,
                                                 int                *cell_x,
                                                 int                *cell_y);

void        _moo_icon_view_enable_drag_source   (MooIconView        *view,
                                                 GdkModifierType     start_button_mask,
                                                 GtkTargetEntry     *targets,
                                                 gint                n_targets,
                                                 GdkDragAction       actions);

void        _moo_icon_view_enable_drag_dest     (MooIconView        *view,
                                                 GtkTargetEntry     *targets,
                                                 gint                n_targets,
                                                 GdkDragAction       actions);
void        _moo_icon_view_set_dest_targets     (MooIconView        *view,
                                                 GtkTargetList      *targets);
void        _moo_icon_view_set_drag_dest_row    (MooIconView        *view,
                                                 GtkTreePath        *path);
GtkTreePath *_moo_icon_view_get_drag_dest_row   (MooIconView        *view);


G_END_DECLS

#endif /* MOO_ICON_VIEW_H */
