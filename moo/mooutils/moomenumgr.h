/*
 *   moomenumgr.h
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

#ifndef MOO_MENU_MGR_H
#define MOO_MENU_MGR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MOO_TYPE_MENU_ITEM_FLAGS       (moo_menu_item_flags_get_type ())

#define MOO_TYPE_MENU_MGR              (moo_menu_mgr_get_type ())
#define MOO_MENU_MGR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_MENU_MGR, MooMenuMgr))
#define MOO_MENU_MGR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_MENU_MGR, MooMenuMgrClass))
#define MOO_IS_MENU_MGR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_MENU_MGR))
#define MOO_IS_MENU_MGR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_MENU_MGR))
#define MOO_MENU_MGR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_MENU_MGR, MooMenuMgrClass))


typedef struct _MooMenuMgr        MooMenuMgr;
typedef struct _MooMenuMgrPrivate MooMenuMgrPrivate;
typedef struct _MooMenuMgrClass   MooMenuMgrClass;

typedef enum {
    MOO_MENU_ITEM_FLAGS_NONE        = 0,
    MOO_MENU_ITEM_TOGGLE            = 1 << 0,
    MOO_MENU_ITEM_RADIO             = 1 << 1,
    MOO_MENU_ITEM_ACTIVATABLE       = 1 << 2,
    MOO_MENU_ITEM_SEPARATOR         = 1 << 3,
    MOO_MENU_ITEM_USE_MNEMONIC      = 1 << 4,
    MOO_MENU_ITEM_DONT_USE_MNEMONIC = 1 << 5
} MooMenuItemFlags;

struct _MooMenuMgr
{
    GObject base;
    MooMenuMgrPrivate *priv;
};

struct _MooMenuMgrClass
{
    GObjectClass base_class;

    void (*radio_set_active) (MooMenuMgr    *mgr,
                              gpointer       item_data);
    void (*toggle_set_active)(MooMenuMgr    *mgr,
                              gpointer       item_data,
                              gboolean       active);
    void (*item_activated)   (MooMenuMgr    *mgr,
                              gpointer       item_data,
                              gpointer       menu_data);
};


GType       moo_menu_mgr_get_type           (void) G_GNUC_CONST;
GType       moo_menu_item_flags_get_type    (void) G_GNUC_CONST;

MooMenuMgr *moo_menu_mgr_new                (void);

int         moo_menu_mgr_insert             (MooMenuMgr         *mgr,
                                             const char         *parent_id,
                                             int                 position,
                                             const char         *item_id,
                                             const char         *label,
                                             const char         *tip,
                                             MooMenuItemFlags    flags,
                                             gpointer            data,
                                             GDestroyNotify      destroy);
int         moo_menu_mgr_append             (MooMenuMgr         *mgr,
                                             const char         *parent_id,
                                             const char         *item_id,
                                             const char         *label,
                                             const char         *tip,
                                             MooMenuItemFlags    flags,
                                             gpointer            data,
                                             GDestroyNotify      destroy);
int         moo_menu_mgr_insert_separator   (MooMenuMgr         *mgr,
                                             const char         *parent_id,
                                             int                 position);

void        moo_menu_mgr_remove             (MooMenuMgr         *mgr,
                                             const char         *parent_id,
                                             guint               position);

void        moo_menu_mgr_set_active         (MooMenuMgr         *mgr,
                                             const char         *item_id,
                                             gboolean            active);

GtkWidget  *moo_menu_mgr_create_item        (MooMenuMgr         *mgr,
                                             const char         *label,
                                             MooMenuItemFlags    flags,
                                             gpointer            user_data,
                                             GDestroyNotify      destroy);

void        moo_menu_mgr_set_use_mnemonic   (MooMenuMgr         *mgr,
                                             gboolean            use);
void        moo_menu_mgr_set_show_tooltips  (MooMenuMgr         *mgr,
                                             gboolean            show);


G_END_DECLS

#endif /* MOO_MENU_MGR_H */
