/*
 *   moohistorylist.c
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

/**
 * class:MooHistoryList: (parent GObject) (constructable) (moo.private 1)
 **/

#include "marshals.h"
#include "mooutils/moohistorylist.h"
#include "mooutils/mooprefs.h"
#include "mooutils/mootype-macros.h"
#include <gtk/gtk.h>
#include <string.h>

#define MAX_NUM_HISTORY_ITEMS 10

typedef MooHistoryListItem Item;

struct _MooHistoryListPrivate {
    GtkTreeModel *model;
    GtkListStore *store;
    guint num_user;
    guint num_builtin;
    gboolean has_separator;
    gboolean use_separator;
    guint max_items;

    MooMenuMgr *mgr;
    gboolean prefs_loaded;
    gboolean loading;
    char *user_id;

    MooHistoryDisplayFunc display_func;
    gpointer display_data;
    MooHistoryDisplayFunc tip_func;
    gpointer tip_data;
    MooHistoryCompareFunc compare_func;
    gpointer compare_data;
    gboolean allow_empty;
};

MOO_DEFINE_BOXED_TYPE_C (MooHistoryListItem, moo_history_list_item)

static GHashTable *named_lists;

static void     moo_history_list_class_init     (MooHistoryListClass *klass);
static void     moo_history_list_init           (MooHistoryList     *list);
static void     moo_history_list_finalize       (GObject            *object);
static void     moo_history_list_set_property   (GObject            *object,
                                                 guint               prop_id,
                                                 const GValue       *value,
                                                 GParamSpec         *pspec);
static void     moo_history_list_get_property   (GObject            *object,
                                                 guint               prop_id,
                                                 GValue             *value,
                                                 GParamSpec         *pspec);

static char    *list_get_display                (MooHistoryList     *list,
                                                 const char         *item);
static gboolean default_compare_func            (const char         *text,
                                                 Item              *entry,
                                                 gpointer            data);

static void     _list_remove                    (MooHistoryList     *list,
                                                 GtkTreeIter        *iter);
static void     _list_insert                    (MooHistoryList     *list,
                                                 int                 index,
                                                 const Item         *entry);
static void     _list_move_on_top               (MooHistoryList     *list,
                                                 GtkTreeIter        *iter);
static void     _list_delete_last               (MooHistoryList     *list);

static void     list_save_recent                (MooHistoryList     *list);

static void     menu_item_activated             (MooHistoryList     *list,
                                                 MooHistoryListItem *entry,
                                                 gpointer            menu_data);


/* MOO_TYPE_HISTORY_LIST */
G_DEFINE_TYPE (MooHistoryList, moo_history_list, G_TYPE_OBJECT)

enum {
    ACTIVATE_ITEM,
    CHANGED,
    NUM_SIGNALS
};

enum {
    PROP_0,
    PROP_USER_ID,
    PROP_MAX_ITEMS,
    PROP_EMPTY
};

static guint signals[NUM_SIGNALS];

static void
moo_history_list_class_init (MooHistoryListClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = moo_history_list_finalize;
    gobject_class->set_property = moo_history_list_set_property;
    gobject_class->get_property = moo_history_list_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_USER_ID,
                                     g_param_spec_string ("user-id",
                                             "user-id",
                                             "user-id",
                                             NULL,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property (gobject_class,
                                     PROP_MAX_ITEMS,
                                     g_param_spec_uint ("max-items",
                                             "max-items",
                                             "max-items",
                                             1,
                                             G_MAXUINT,
                                             MAX_NUM_HISTORY_ITEMS,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_EMPTY,
                                     g_param_spec_boolean ("empty",
                                             "empty",
                                             "empty",
                                             TRUE,
                                             G_PARAM_READABLE));

    signals[ACTIVATE_ITEM] =
            g_signal_new ("activate-item",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooHistoryListClass, activate_item),
                          NULL, NULL,
                          _moo_marshal_VOID__BOXED_POINTER,
                          G_TYPE_NONE, 2,
                          MOO_TYPE_HISTORY_ITEM,
                          G_TYPE_POINTER);

    signals[CHANGED] =
            g_signal_new ("changed",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooHistoryListClass, changed),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);
}


static void
moo_history_list_init (MooHistoryList *list)
{
    list->priv = g_new0 (MooHistoryListPrivate, 1);
    list->priv->max_items = MAX_NUM_HISTORY_ITEMS;
    list->priv->store = gtk_list_store_new (1, MOO_TYPE_HISTORY_ITEM);
    list->priv->model = GTK_TREE_MODEL (list->priv->store);
    list->priv->use_separator = TRUE;

    list->priv->mgr = moo_menu_mgr_new ();
    moo_menu_mgr_set_use_mnemonic (list->priv->mgr, FALSE);
    g_signal_connect_swapped (list->priv->mgr, "item-activated",
                              G_CALLBACK (menu_item_activated), list);

    list->priv->compare_func = default_compare_func;
}


static void moo_history_list_set_property (GObject            *object,
                                           guint               prop_id,
                                           const GValue       *value,
                                           GParamSpec         *pspec)
{
    MooHistoryList *list = MOO_HISTORY_LIST (object);

    switch (prop_id)
    {
        case PROP_USER_ID:
            g_free (list->priv->user_id);
            list->priv->user_id = g_strdup (g_value_get_string (value));
            g_object_notify (object, "user-id");
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void moo_history_list_get_property (GObject            *object,
                                           guint               prop_id,
                                           GValue             *value,
                                           GParamSpec         *pspec)
{
    MooHistoryList *list = MOO_HISTORY_LIST (object);

    switch (prop_id)
    {
        case PROP_USER_ID:
            g_value_set_string (value, list->priv->user_id);
            break;

        case PROP_EMPTY:
            g_value_set_boolean (value, moo_history_list_is_empty (list));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_history_list_finalize (GObject *object)
{
    MooHistoryList *list = MOO_HISTORY_LIST (object);

    g_object_unref (list->priv->store);
    g_free (list->priv->user_id);
    g_object_unref (list->priv->mgr);

    g_free (list->priv);

    G_OBJECT_CLASS (moo_history_list_parent_class)->finalize (object);
}


MooHistoryListItem *
moo_history_list_get_item (MooHistoryList *list,
                           GtkTreeIter    *iter)
{
    MooHistoryListItem *item = NULL;
    gtk_tree_model_get (list->priv->model, iter, 0, &item, -1);
    return item;
}


gboolean
moo_history_list_find (MooHistoryList *list,
                       const char     *text,
                       GtkTreeIter    *iter)
{
    g_return_val_if_fail (MOO_IS_HISTORY_LIST (list), FALSE);
    g_return_val_if_fail (iter != NULL, FALSE);
    g_return_val_if_fail (text != NULL, FALSE);

    if (gtk_tree_model_get_iter_first (list->priv->model, iter))
    {
        do
        {
            Item *entry = moo_history_list_get_item (list, iter);

            if (entry && list->priv->compare_func (text, entry, list->priv->compare_data))
            {
                moo_history_list_item_free (entry);
                return TRUE;
            }

            moo_history_list_item_free (entry);
        }
        while (gtk_tree_model_iter_next (list->priv->model, iter));
    }

    return FALSE;
}


static gboolean
default_compare_func (const char *text,
                      Item       *item,
                      G_GNUC_UNUSED gpointer data)
{
    g_return_val_if_fail (text != NULL, FALSE);
    g_return_val_if_fail (item != NULL, FALSE);
    return !strcmp (text, item->data);
}


Item*
moo_history_list_item_new (const char     *data,
                           const char     *display,
                           gboolean        builtin)
{
    Item *item;

    g_return_val_if_fail (data != NULL, NULL);
    g_return_val_if_fail (display != NULL, NULL);
    g_return_val_if_fail (g_utf8_validate (display, -1, NULL), NULL);

    item = g_new0 (Item, 1);
    item->data = g_strdup (data);
    item->display = g_strdup (display);
    item->builtin = builtin != 0;

    return item;
}


Item*
moo_history_list_item_copy (const Item *item)
{
    g_return_val_if_fail (item != NULL, NULL);
    return moo_history_list_item_new (item->data,
                                      item->display,
                                      item->builtin);
}


void
moo_history_list_item_free (Item *item)
{
    if (item)
    {
        g_free (item->data);
        g_free (item->display);
        g_free (item);
    }
}


static char*
list_get_display (MooHistoryList     *list,
                  const char         *item)
{
    g_return_val_if_fail (item != NULL, NULL);

    if (list->priv->display_func)
    {
        return list->priv->display_func (item, list->priv->display_data);
    }
    else
    {
        g_return_val_if_fail (g_utf8_validate (item, -1, NULL), NULL);
        return g_strdup (item);
    }
}


static char*
list_get_tip (MooHistoryList     *list,
              const char         *item)
{
    g_return_val_if_fail (item != NULL, NULL);

    if (list->priv->tip_func)
        return list->priv->tip_func (item, list->priv->tip_data);
    else
        return NULL;
}


void
moo_history_list_add_builtin (MooHistoryList *list,
                              const char     *item,
                              const char     *display_item)
{
    Item *new_item;
    GtkTreeIter iter;

    g_return_if_fail (MOO_IS_HISTORY_LIST (list));

    new_item = moo_history_list_item_new (item, display_item, TRUE);
    g_return_if_fail (new_item != NULL);

    if (moo_history_list_find (list, item, &iter))
    {
        Item *old = moo_history_list_get_item (list, &iter);
        if (!old->builtin)
            _list_remove (list, &iter);
        moo_history_list_item_free (old);
    }

    _list_insert (list, list->priv->num_builtin++, new_item);

    if (!list->priv->has_separator &&
         list->priv->use_separator &&
         list->priv->num_user)
    {
        _list_insert (list, list->priv->num_builtin, NULL);
        list->priv->has_separator = TRUE;
    }

    g_signal_emit (list, signals[CHANGED], 0);
    g_object_notify (G_OBJECT (list), "empty");
}


void
moo_history_list_add (MooHistoryList *list,
                      const char     *entry)
{
    char *display;

    g_return_if_fail (MOO_IS_HISTORY_LIST (list));
    g_return_if_fail (entry != NULL);

    display = list_get_display (list, entry);
    g_return_if_fail (display != NULL);

    moo_history_list_add_full (list, entry, display);

    g_free (display);
}


void
moo_history_list_add_filename (MooHistoryList *list,
                               const char     *filename)
{
    char *display;

    g_return_if_fail (MOO_IS_HISTORY_LIST (list));
    g_return_if_fail (filename != NULL);

    if (list->priv->display_func)
        display = list->priv->display_func (filename, list->priv->display_data);
    else
        display = g_filename_display_name (filename);

    g_return_if_fail (display != NULL);

    moo_history_list_add_full (list, filename, display);

    g_free (display);
}


void
moo_history_list_add_full (MooHistoryList *list,
                           const char     *entry,
                           const char     *display_entry)
{
    Item *new_entry;
    GtkTreeIter iter;
    int index;
    gboolean was_empty;

    g_return_if_fail (MOO_IS_HISTORY_LIST (list));
    g_return_if_fail (entry != NULL);
    g_return_if_fail (display_entry != NULL);

    if (!list->priv->allow_empty && !entry[0])
        return;

    was_empty = moo_history_list_is_empty (list);

    _moo_history_list_load (list);

    if (moo_history_list_find (list, entry, &iter))
    {
        Item *old = moo_history_list_get_item (list, &iter);

        if (!old->builtin)
        {
            _list_move_on_top (list, &iter);
            list_save_recent (list);
            g_signal_emit (list, signals[CHANGED], 0);
        }

        moo_history_list_item_free (old);
        return;
    }

    new_entry = moo_history_list_item_new (entry, display_entry, FALSE);
    g_return_if_fail (new_entry != NULL);

    if (list->priv->num_builtin &&
        !list->priv->has_separator &&
        list->priv->use_separator)
    {
        _list_insert (list, list->priv->num_builtin, NULL);
        list->priv->has_separator = TRUE;
    }

    index = list->priv->num_builtin + (list->priv->has_separator ? 1 : 0);
    _list_insert (list, index, new_entry);
    list->priv->num_user++;

    if (list->priv->num_user > list->priv->max_items)
    {
        _list_delete_last (list);
        list->priv->num_user--;
    }

    list_save_recent (list);

    g_signal_emit (list, signals[CHANGED], 0);

    if (was_empty)
        g_object_notify (G_OBJECT (list), "empty");

    moo_history_list_item_free (new_entry);
}


char *
moo_history_list_get_last_item (MooHistoryList *list)
{
    GtkTreeIter iter;
    Item *item = NULL;
    char *data;

    g_return_val_if_fail (MOO_IS_HISTORY_LIST (list), NULL);

    _moo_history_list_load (list);

    if (!gtk_tree_model_iter_children (list->priv->model, &iter, NULL))
        return NULL;

    gtk_tree_model_get (list->priv->model, &iter, 0, &item, -1);

    if (!item)
        return NULL;

    data = item->data;
    item->data = NULL;
    moo_history_list_item_free (item);
    return data;
}


void
moo_history_list_remove (MooHistoryList *list,
                         const char     *entry)
{
    Item *item;
    GtkTreeIter iter;
    int separator;

    g_return_if_fail (MOO_IS_HISTORY_LIST (list));
    g_return_if_fail (entry != NULL);

    if (!moo_history_list_find (list, entry, &iter))
        return;

    item = moo_history_list_get_item (list, &iter);
    _list_remove (list, &iter);

    separator = list->priv->num_builtin;

    if (item->builtin)
        list->priv->num_builtin--;
    else
        list->priv->num_user--;

    if (list->priv->has_separator &&
        (!list->priv->num_builtin || !list->priv->num_user))
    {
        gtk_tree_model_iter_nth_child (list->priv->model, &iter, NULL, separator);
        _list_remove (list, &iter);
        list->priv->has_separator = FALSE;
    }

    list_save_recent (list);

    if (moo_history_list_is_empty (list))
        g_object_notify (G_OBJECT (list), "empty");

    moo_history_list_item_free (item);
    return;
}


void
moo_history_list_set_display_func (MooHistoryList *list,
                                   MooHistoryDisplayFunc func,
                                   gpointer        data)
{
    g_return_if_fail (MOO_IS_HISTORY_LIST (list));
    list->priv->display_func = func;
    list->priv->display_data = data;
}


void
moo_history_list_set_tip_func (MooHistoryList *list,
                               MooHistoryDisplayFunc func,
                               gpointer        data)
{
    g_return_if_fail (MOO_IS_HISTORY_LIST (list));
    list->priv->tip_func = func;
    list->priv->tip_data = data;
}


void
moo_history_list_set_compare_func  (MooHistoryList *list,
                                    MooHistoryCompareFunc func,
                                    gpointer        data)
{
    g_return_if_fail (MOO_IS_HISTORY_LIST (list));

    if (func)
    {
        list->priv->compare_func = func;
        list->priv->compare_data = data;
    }
    else
    {
        list->priv->compare_func = default_compare_func;
        list->priv->compare_data = NULL;
    }
}


char *
moo_history_list_display_basename (const char     *filename,
                                   G_GNUC_UNUSED gpointer data)
{
    char *basename, *display;

    g_return_val_if_fail (filename != NULL, NULL);

    basename = g_path_get_basename (filename);
    g_return_val_if_fail (basename != NULL, NULL);

    display = g_filename_display_name (basename);

    g_free (basename);
    return display;
}


char *
moo_history_list_display_filename (const char *filename,
                                   G_GNUC_UNUSED gpointer data)
{
    g_return_val_if_fail (filename != NULL, NULL);
    return g_filename_display_name (filename);
}


gboolean
moo_history_list_is_empty (MooHistoryList *list)
{
    g_return_val_if_fail (MOO_IS_HISTORY_LIST (list), TRUE);
    return (list->priv->num_builtin + list->priv->num_user) ? FALSE : TRUE;
}


guint
moo_history_list_n_user_entries (MooHistoryList *list)
{
    g_return_val_if_fail (MOO_IS_HISTORY_LIST (list), 0);
    return list->priv->num_user;
}


guint
moo_history_list_get_max_entries (MooHistoryList *list)
{
    g_return_val_if_fail (MOO_IS_HISTORY_LIST (list), 0);
    return list->priv->max_items;
}


void
moo_history_list_set_max_entries (MooHistoryList *list,
                                  guint           num)
{
    g_return_if_fail (MOO_IS_HISTORY_LIST (list));
    g_return_if_fail (num > 0);
    list->priv->max_items = num;
}


static void
add_named_list (const char     *user_id,
                MooHistoryList *list)
{
    if (!named_lists)
        named_lists = g_hash_table_new_full (g_str_hash, g_str_equal,
                                             g_free, g_object_unref);
    g_hash_table_insert (named_lists, g_strdup (user_id), g_object_ref (list));
}

MooHistoryList *
moo_history_list_new (const char *user_id)
{
    MooHistoryList *list;

    if (user_id && named_lists)
        g_return_val_if_fail (g_hash_table_lookup (named_lists, user_id) == NULL, NULL);

    list = MOO_HISTORY_LIST (g_object_new (MOO_TYPE_HISTORY_LIST,
                                           "user-id", user_id,
                                           (const char*) NULL));
    if (user_id)
    {
        add_named_list (user_id, list);
        _moo_history_list_load (list);
    }

    return list;
}

MooHistoryList *
moo_history_list_get (const char *user_id)
{
    MooHistoryList *list;

    g_return_val_if_fail (user_id != NULL, NULL);

    if (!named_lists)
        named_lists = g_hash_table_new_full (g_str_hash, g_str_equal,
                                             g_free, g_object_unref);

    list = (MooHistoryList*) g_hash_table_lookup (named_lists, user_id);

    if (!list)
    {
        list = moo_history_list_new (user_id);
        g_object_unref (list);
    }

    return list;
}


GtkTreeModel*
moo_history_list_get_model (MooHistoryList *list)
{
    g_return_val_if_fail (MOO_IS_HISTORY_LIST (list), NULL);
    return list->priv->model;
}


/**************************************************************************/
/* Menus
 */

static void
menu_item_activated (MooHistoryList     *list,
                     MooHistoryListItem *entry,
                     gpointer            menu_data)
{
    g_signal_emit (list, signals[ACTIVATE_ITEM], 0, entry, menu_data);
}


MooMenuMgr*
moo_history_list_get_menu_mgr (MooHistoryList *list)
{
    g_return_val_if_fail (MOO_IS_HISTORY_LIST (list), NULL);
    _moo_history_list_load (list);
    return list->priv->mgr;
}


static void
_list_remove (MooHistoryList     *list,
              GtkTreeIter        *iter)
{
    GtkTreePath *path;
    int index;

    path = gtk_tree_model_get_path (list->priv->model, iter);
    index = gtk_tree_path_get_indices (path)[0];
    gtk_tree_path_free (path);

    moo_menu_mgr_remove (list->priv->mgr, NULL, index);
    gtk_list_store_remove (list->priv->store, iter);
}


static void
_list_insert (MooHistoryList     *list,
              int                 index,
              const Item         *entry)
{
    GtkTreeIter iter;
    char *tip = NULL;

    gtk_list_store_insert (list->priv->store, &iter, index);
    gtk_list_store_set (list->priv->store, &iter, 0, entry, -1);

    if (entry)
    {
        tip = list_get_tip (list, entry->data);
        moo_menu_mgr_insert (list->priv->mgr,
                             NULL, index, NULL,
                             entry->display, tip, MOO_MENU_ITEM_ACTIVATABLE,
                             moo_history_list_item_copy (entry),
                             (GDestroyNotify) moo_history_list_item_free);
    }
    else
    {
        moo_menu_mgr_insert_separator (list->priv->mgr, NULL, index);
    }

    g_free (tip);
}


static void
_list_move_on_top (MooHistoryList     *list,
                   GtkTreeIter        *iter)
{
    GtkTreePath *path;
    int old_index, new_index;
    Item *entry;
    char *tip;

    path = gtk_tree_model_get_path (list->priv->model, iter);
    old_index = gtk_tree_path_get_indices (path)[0];
    gtk_tree_path_free (path);

    new_index = list->priv->num_builtin + (list->priv->has_separator ? 1 : 0);

    if (old_index == new_index)
        return;

    entry = moo_history_list_get_item (list, iter);

    moo_menu_mgr_remove (list->priv->mgr, NULL, old_index);
    tip = list_get_tip (list, entry->data);

    if (entry)
        moo_menu_mgr_insert (list->priv->mgr,
                             NULL, new_index, NULL,
                             entry->display, tip, MOO_MENU_ITEM_ACTIVATABLE,
                             moo_history_list_item_copy (entry),
                             (GDestroyNotify) moo_history_list_item_free);
    else
        moo_menu_mgr_insert_separator (list->priv->mgr, NULL, new_index);

    gtk_list_store_remove (list->priv->store, iter);
    gtk_list_store_insert (list->priv->store, iter, new_index);
    gtk_list_store_set (list->priv->store, iter, 0, entry, -1);
    moo_history_list_item_free (entry);
    g_free (tip);
}


static void
_list_delete_last (MooHistoryList *list)
{
    GtkTreeIter iter;
    int n = gtk_tree_model_iter_n_children (list->priv->model, NULL);
    g_return_if_fail (n > 0);
    gtk_tree_model_iter_nth_child (list->priv->model, &iter, NULL, n-1);
    _list_remove (list, &iter);
}


/***************************************************************************/
/* Loading and saving
 */

#define ELEMENT_RECENT_ITEMS    "recent-items"
#define ELEMENT_ITEM            "item"

/* TODO: this all is broken with non-utf8 text */

void
_moo_history_list_load (MooHistoryList *list)
{
    MooMarkupNode *xml;
    MooMarkupNode *root, *node;
    char *root_path;

    g_return_if_fail (MOO_IS_HISTORY_LIST (list));

    if (!list->priv->user_id)
        return;

    if (list->priv->prefs_loaded)
        return;
    else
        list->priv->prefs_loaded = TRUE;

    xml = moo_prefs_get_markup (MOO_PREFS_STATE);
    g_return_if_fail (xml != NULL);

    root_path = g_strdup_printf ("%s/" ELEMENT_RECENT_ITEMS, list->priv->user_id);
    root = moo_markup_get_element (xml, root_path);
    g_free (root_path);

    if (!root)
        return;

    list->priv->loading = TRUE;

    for (node = root->last; node != NULL; node = node->prev)
    {
        if (!MOO_MARKUP_IS_ELEMENT (node))
            continue;

        if (!strcmp (node->name, ELEMENT_ITEM))
        {
            const char *entry = moo_markup_get_content (node);

            if (!entry || !entry[0])
            {
                g_warning ("empty recent entry");
                continue;
            }

            moo_history_list_add (list, entry);
        }
        else
        {
            g_warning ("invalid '%s' element", node->name);
        }
    }

    list->priv->loading = FALSE;
}


static gboolean
save_one (GtkTreeModel  *model,
          G_GNUC_UNUSED GtkTreePath *path,
          GtkTreeIter   *iter,
          MooMarkupNode *root)
{
    Item *item = NULL;

    /* XXX 0 */
    gtk_tree_model_get (model, iter, 0, &item, -1);

    if (!item)
        return FALSE;

    if (!item->builtin)
        moo_markup_create_text_element (root, ELEMENT_ITEM, item->data);

    moo_history_list_item_free (item);
    return FALSE;
}

static void
list_save_recent (MooHistoryList *list)
{
    MooMarkupNode *xml;
    MooMarkupNode *root;
    char *root_path;

    if (!list->priv->user_id)
        return;

    if (list->priv->loading)
        return;

    xml = moo_prefs_get_markup (MOO_PREFS_STATE);
    g_return_if_fail (xml != NULL);

    root_path = g_strdup_printf ("%s/" ELEMENT_RECENT_ITEMS, list->priv->user_id);
    root = moo_markup_get_element (xml, root_path);

    if (root)
        moo_markup_delete_node (root);

    if (!moo_history_list_n_user_entries (list))
    {
        g_free (root_path);
        return;
    }

    root = moo_markup_create_element (xml, root_path);
    g_return_if_fail (root != NULL);

    gtk_tree_model_foreach (GTK_TREE_MODEL (list->priv->store),
                            (GtkTreeModelForeachFunc) save_one,
                            root);

    g_free (root_path);
}

#undef ELEMENT_RECENT_FILES
#undef ELEMENT_ENTRY
