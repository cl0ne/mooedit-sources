/*
 *   moopane.h
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

#ifndef MOO_PANE_H
#define MOO_PANE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_PANE               (moo_pane_get_type ())
#define MOO_PANE(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_PANE, MooPane))
#define MOO_PANE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_PANE, MooPaneClass))
#define MOO_IS_PANE(object)         (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_PANE))
#define MOO_IS_PANE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_PANE))
#define MOO_PANE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_PANE, MooPaneClass))

#define MOO_TYPE_PANE_LABEL         (moo_pane_label_get_type ())
#define MOO_TYPE_PANE_PARAMS        (moo_pane_params_get_type ())

typedef struct _MooPane          MooPane;
typedef struct _MooPaneClass     MooPaneClass;
typedef struct _MooPaneLabel     MooPaneLabel;
typedef struct _MooPaneParams    MooPaneParams;

struct _MooPaneLabel {
    char *icon_stock_id;
    GdkPixbuf *icon_pixbuf;
    char *label;
    char *window_title;
};

struct _MooPaneParams
{
    GdkRectangle window_position;
    guint detached : 1;
    guint maximized : 1;
    guint keep_on_top : 1;
};


GType           moo_pane_get_type           (void) G_GNUC_CONST;
GType           moo_pane_label_get_type     (void) G_GNUC_CONST;
GType           moo_pane_params_get_type    (void) G_GNUC_CONST;

const char     *moo_pane_get_id             (MooPane        *pane);

void            moo_pane_set_label          (MooPane        *pane,
                                             MooPaneLabel   *label);
/* result must be freed with moo_pane_label_free() */
MooPaneLabel   *moo_pane_get_label          (MooPane        *pane);
void            moo_pane_set_frame_markup   (MooPane        *pane,
                                             const char     *markup);
void            moo_pane_set_frame_text     (MooPane        *pane,
                                             const char     *text);
void            moo_pane_set_params         (MooPane        *pane,
                                             MooPaneParams  *params);
/* result must be freed with moo_pane_params_free() */
MooPaneParams  *moo_pane_get_params         (MooPane        *pane);
void            moo_pane_set_detachable     (MooPane        *pane,
                                             gboolean        detachable);
gboolean        moo_pane_get_detachable     (MooPane        *pane);
void            moo_pane_set_removable      (MooPane        *pane,
                                             gboolean        removable);
gboolean        moo_pane_get_removable      (MooPane        *pane);

GtkWidget      *moo_pane_get_child          (MooPane        *pane);
int             moo_pane_get_index          (MooPane        *pane);

void            moo_pane_open               (MooPane        *pane);
void            moo_pane_present            (MooPane        *pane);
void            moo_pane_attach             (MooPane        *pane);
void            moo_pane_detach             (MooPane        *pane);

void            moo_pane_set_drag_dest      (MooPane        *pane);
void            moo_pane_unset_drag_dest    (MooPane        *pane);

MooPaneParams  *moo_pane_params_new         (GdkRectangle   *window_position,
                                             gboolean        detached,
                                             gboolean        maximized,
                                             gboolean        keep_on_top);
MooPaneParams  *moo_pane_params_copy        (MooPaneParams  *params);
void            moo_pane_params_free        (MooPaneParams  *params);

MooPaneLabel   *moo_pane_label_new          (const char     *icon_stock_id,
                                             GdkPixbuf      *pixbuf,
                                             const char     *label,
                                             const char     *window_title);
MooPaneLabel   *moo_pane_label_copy         (MooPaneLabel   *label);
void            moo_pane_label_free         (MooPaneLabel   *label);

MooPane        *_moo_pane_new               (GtkWidget      *child,
                                             MooPaneLabel   *label);
void            _moo_pane_set_id            (MooPane        *pane,
                                             const char     *id);

gpointer        _moo_pane_get_parent        (MooPane        *pane);
GtkWidget      *_moo_pane_get_frame         (MooPane        *pane);
void            _moo_pane_update_focus_child (MooPane       *pane);
GtkWidget      *_moo_pane_get_focus_child   (MooPane        *pane);
GtkWidget      *_moo_pane_get_button        (MooPane        *pane);
void            _moo_pane_get_handle        (MooPane        *pane,
                                             GtkWidget     **big,
                                             GtkWidget     **small_);
GtkWidget      *_moo_pane_get_window        (MooPane        *pane);

void            _moo_pane_params_changed    (MooPane        *pane);
void            _moo_pane_freeze_params     (MooPane        *pane);
void            _moo_pane_thaw_params       (MooPane        *pane);
void            _moo_pane_size_request      (MooPane        *pane,
                                             GtkRequisition *req);
void            _moo_pane_get_size_request  (MooPane        *pane,
                                             GtkRequisition *req);
void            _moo_pane_size_allocate     (MooPane        *pane,
                                             GtkAllocation  *allocation);

gboolean        _moo_pane_get_detached      (MooPane        *pane);
void            _moo_pane_attach            (MooPane        *pane);
void            _moo_pane_detach            (MooPane        *pane);
void            _moo_pane_set_parent        (MooPane        *pane,
                                             gpointer        parent,
                                             GdkWindow      *window);
void            _moo_pane_unparent          (MooPane        *pane);
void            _moo_pane_try_remove        (MooPane        *pane);

typedef enum {
    MOO_SMALL_ICON_HIDE,
    MOO_SMALL_ICON_STICKY,
    MOO_SMALL_ICON_CLOSE,
    MOO_SMALL_ICON_DETACH,
    MOO_SMALL_ICON_ATTACH,
    MOO_SMALL_ICON_KEEP_ON_TOP
} MooSmallIcon;

GtkWidget      *_moo_create_small_icon      (MooSmallIcon    icon);
GtkWidget      *_moo_create_arrow_icon      (GtkArrowType    arrow_type);


G_END_DECLS

#endif /* MOO_PANE_H */
