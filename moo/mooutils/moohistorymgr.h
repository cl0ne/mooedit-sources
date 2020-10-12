/*
 *   moohistorymgr.h
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

#ifndef MOO_HISTORY_MGR_H
#define MOO_HISTORY_MGR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MOO_HISTORY_MGR_PARSE_ERROR (moo_history_mgr_parse_error_quark ())

enum {
    MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT
};

#define MOO_TYPE_HISTORY_MGR             (moo_history_mgr_get_type ())
#define MOO_HISTORY_MGR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOO_TYPE_HISTORY_MGR, MooHistoryMgr))
#define MOO_HISTORY_MGR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_HISTORY_MGR, MooHistoryMgrClass))
#define MOO_IS_HISTORY_MGR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOO_TYPE_HISTORY_MGR))
#define MOO_IS_HISTORY_MGR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_HISTORY_MGR))
#define MOO_HISTORY_MGR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_HISTORY_MGR, MooHistoryMgrClass))

typedef struct _MooHistoryItem MooHistoryItem;
typedef struct _MooHistoryMgr MooHistoryMgr;
typedef struct _MooHistoryMgrClass MooHistoryMgrClass;
typedef struct _MooHistoryMgrPrivate MooHistoryMgrPrivate;

struct _MooHistoryMgr {
    GObject base;
    MooHistoryMgrPrivate *priv;
};

struct _MooHistoryMgrClass {
    GObjectClass base_class;
};

typedef void (*MooHistoryCallback)              (GSList   *items,
                                                 gpointer  data);

GQuark      moo_history_mgr_parse_error_quark   (void) G_GNUC_CONST;
GType       moo_history_mgr_get_type            (void) G_GNUC_CONST;

void        moo_history_mgr_add_file            (MooHistoryMgr  *mgr,
                                                 MooHistoryItem *item);
void        moo_history_mgr_update_file         (MooHistoryMgr  *mgr,
                                                 MooHistoryItem *item);
void        moo_history_mgr_add_uri             (MooHistoryMgr  *mgr,
                                                 const char     *uri);
void        moo_history_mgr_remove_uri          (MooHistoryMgr  *mgr,
                                                 const char     *uri);
MooHistoryItem  *moo_history_mgr_find_uri       (MooHistoryMgr  *mgr,
                                                 const char     *uri);

void        moo_history_mgr_shutdown            (MooHistoryMgr  *mgr);

guint       moo_history_mgr_get_n_items         (MooHistoryMgr  *mgr);

GtkWidget  *moo_history_mgr_create_menu         (MooHistoryMgr  *mgr,
                                                 MooHistoryCallback callback,
                                                 gpointer        data,
                                                 GDestroyNotify  notify);
GtkWidget  *moo_history_mgr_create_dialog       (MooHistoryMgr  *mgr,
                                                 MooHistoryCallback callback,
                                                 gpointer        data,
                                                 GDestroyNotify  notify);

MooHistoryItem *moo_history_item_new            (const char     *uri,
                                                 const char     *first_key,
                                                 ...);
MooHistoryItem *moo_history_item_copy           (MooHistoryItem *item);
void            moo_history_item_free           (MooHistoryItem *item);
void            moo_history_item_set            (MooHistoryItem *item,
                                                 const char     *key,
                                                 const char     *value);
const char     *moo_history_item_get            (MooHistoryItem *item,
                                                 const char     *key);
const char     *moo_history_item_get_uri        (MooHistoryItem *item);
void            moo_history_item_foreach        (MooHistoryItem *item,
                                                 GDataForeachFunc func,
                                                 gpointer        user_data);

char          *_moo_history_mgr_get_filename    (MooHistoryMgr  *mgr);

G_END_DECLS

#endif /* MOO_HISTORY_MGR_H */
