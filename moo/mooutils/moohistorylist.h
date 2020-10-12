/*
 *   moohistorylist.h
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

#ifndef MOO_HISTORY_LIST_H
#define MOO_HISTORY_LIST_H

#include <gtk/gtk.h>
#include <mooutils/moomenumgr.h>

G_BEGIN_DECLS


#define MOO_TYPE_HISTORY_ITEM              (moo_history_list_item_get_type ())
#define MOO_TYPE_HISTORY_LIST              (moo_history_list_get_type ())
#define MOO_HISTORY_LIST(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_HISTORY_LIST, MooHistoryList))
#define MOO_HISTORY_LIST_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_HISTORY_LIST, MooHistoryListClass))
#define MOO_IS_HISTORY_LIST(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_HISTORY_LIST))
#define MOO_IS_HISTORY_LIST_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_HISTORY_LIST))
#define MOO_HISTORY_LIST_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_HISTORY_LIST, MooHistoryListClass))

typedef struct _MooHistoryListItem    MooHistoryListItem;
typedef struct _MooHistoryList        MooHistoryList;
typedef struct _MooHistoryListPrivate MooHistoryListPrivate;
typedef struct _MooHistoryListClass   MooHistoryListClass;

typedef char    *(*MooHistoryDisplayFunc) (const char         *item,
                                           gpointer            data);
typedef gboolean (*MooHistoryCompareFunc) (const char         *text,
                                           MooHistoryListItem *item,
                                           gpointer            data);

struct _MooHistoryListItem
{
    char *data;
    char *display;
    guint builtin : 1;
};

struct _MooHistoryList
{
    GObject      parent;

    MooHistoryListPrivate *priv;
};

struct _MooHistoryListClass
{
    GObjectClass parent_class;

    void (*activate_item)   (MooHistoryList     *list,
                             MooHistoryListItem *item,
                             gpointer            menu_data);

    void (*changed)         (MooHistoryList *list);
};


GType            moo_history_list_get_type          (void) G_GNUC_CONST;
GType            moo_history_list_item_get_type     (void) G_GNUC_CONST;

MooHistoryList  *moo_history_list_new               (const char     *user_id);
MooHistoryList  *moo_history_list_get               (const char     *user_id);

GtkTreeModel    *moo_history_list_get_model         (MooHistoryList *list);

void             moo_history_list_add               (MooHistoryList *list,
                                                     const char     *item);
void             moo_history_list_add_filename      (MooHistoryList *list,
                                                     const char     *filename);
void             moo_history_list_add_full          (MooHistoryList *list,
                                                     const char     *item,
                                                     const char     *display_item);

char            *moo_history_list_get_last_item     (MooHistoryList *list);

void             moo_history_list_remove            (MooHistoryList *list,
                                                     const char     *item);

void             moo_history_list_set_display_func  (MooHistoryList *list,
                                                     MooHistoryDisplayFunc func,
                                                     gpointer        data);
void             moo_history_list_set_tip_func      (MooHistoryList *list,
                                                     MooHistoryDisplayFunc func,
                                                     gpointer        data);
void             moo_history_list_set_compare_func  (MooHistoryList *list,
                                                     MooHistoryCompareFunc func,
                                                     gpointer        data);
char            *moo_history_list_display_basename  (const char     *entry,
                                                     gpointer        data);
char            *moo_history_list_display_filename  (const char     *entry,
                                                     gpointer        data);

/* must free the result */
MooHistoryListItem *moo_history_list_get_item       (MooHistoryList *list,
                                                     GtkTreeIter    *iter);
gboolean         moo_history_list_find              (MooHistoryList *list,
                                                     const char     *text,
                                                     GtkTreeIter    *iter);

gboolean         moo_history_list_is_empty          (MooHistoryList *list);
guint            moo_history_list_n_user_entries    (MooHistoryList *list);
guint            moo_history_list_get_max_entries   (MooHistoryList *list);
void             moo_history_list_set_max_entries   (MooHistoryList *list,
                                                     guint           num);

void             _moo_history_list_load             (MooHistoryList *list);

void             moo_history_list_add_builtin       (MooHistoryList *list,
                                                     const char     *item,
                                                     const char     *display_item);

MooMenuMgr      *moo_history_list_get_menu_mgr      (MooHistoryList *list);

MooHistoryListItem *moo_history_list_item_new       (const char     *data,
                                                     const char     *display,
                                                     gboolean        builtin);
MooHistoryListItem *moo_history_list_item_copy      (const MooHistoryListItem *item);
void             moo_history_list_item_free         (MooHistoryListItem *item);


G_END_DECLS

#endif /* MOO_HISTORY_LIST_H */
