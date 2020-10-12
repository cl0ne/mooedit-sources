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

/**
 * class:MooHistoryMgr: (parent GObject) (moo.private 1)
 **/

#include "mooutils/moohistorymgr.h"
#include "mooutils/moofileicon.h"
#include "mooutils/mooapp-ipc.h"
#include "mooutils/moofilewatch.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils-treeview.h"
#include "mooutils/mooprefs.h"
#include "mooutils/moomarkup.h"
#include "mooutils/mooutils-thread.h"
#include "mooutils/moolist.h"
#include "mooutils/mootype-macros.h"
#include "marshals.h"
#include <stdarg.h>
#include <string.h>

#define N_MENU_ITEMS 10
#define MAX_ITEM_NUMBER 5000

#define IPC_ID "MooHistoryMgr"

MOO_DEFINE_DLIST (WidgetList, widget_list, GtkWidget)
MOO_DEFINE_QUEUE (MooHistoryItem, moo_history_item)
MOO_DEFINE_QUARK (moo-history-mgr-parse-error, moo_history_mgr_parse_error_quark)

struct _MooHistoryMgrPrivate {
    char *filename;
    char *basename;
    char *name;
    char *ipc_id;

    guint save_idle;

    guint update_widgets_idle;
    WidgetList *widgets;

    MooHistoryItemQueue *files;
    GHashTable *hash;
    guint loaded : 1;
};

typedef struct {
    MooHistoryCallback callback;
    gpointer data;
    GDestroyNotify notify;
} CallbackData;

struct _MooHistoryItem {
    char *uri;
    GData *data;
    MooFileIcon *icon;
};

typedef enum {
    UPDATE_ITEM_UPDATE,
    UPDATE_ITEM_REMOVE,
    UPDATE_ITEM_ADD
} UpdateType;

static GObject     *moo_history_mgr_constructor (GType           type,
                                                 guint           n_props,
                                                 GObjectConstructParam *props);
static void         moo_history_mgr_dispose     (GObject        *object);
static void         moo_history_mgr_set_property(GObject        *object,
                                                 guint           property_id,
                                                 const GValue   *value,
                                                 GParamSpec     *pspec);
static void         moo_history_mgr_get_property(GObject        *object,
                                                 guint           property_id,
                                                 GValue         *value,
                                                 GParamSpec     *pspec);

static const char  *get_filename                (MooHistoryMgr   *mgr);
static const char  *get_basename                (MooHistoryMgr   *mgr);

static void         ensure_files                (MooHistoryMgr   *mgr);
static void         schedule_save               (MooHistoryMgr   *mgr);
static void         moo_history_mgr_save        (MooHistoryMgr   *mgr);

static void         populate_menu               (MooHistoryMgr   *mgr,
                                                 GtkWidget      *menu);
static void         schedule_update_widgets     (MooHistoryMgr   *mgr);

static void         moo_history_item_format     (MooHistoryItem  *item,
                                                 GString        *buffer);
static gboolean     moo_history_item_equal      (MooHistoryItem  *item1,
                                                 MooHistoryItem  *item2);
static MooFileIcon *moo_history_item_get_icon   (MooHistoryItem  *item);
static char        *uri_get_basename            (const char     *uri);
static char        *uri_get_display_name        (const char     *uri);

static void         ipc_callback                (GObject        *obj,
                                                 const char     *data,
                                                 gsize           len);
static void         ipc_notify_add_file         (MooHistoryMgr   *mgr,
                                                 MooHistoryItem  *item);
static void         ipc_notify_update_file      (MooHistoryMgr   *mgr,
                                                 MooHistoryItem  *item);
static void         ipc_notify_remove_file      (MooHistoryMgr   *mgr,
                                                 MooHistoryItem  *item);

G_DEFINE_TYPE (MooHistoryMgr, moo_history_mgr, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_NAME,
    PROP_EMPTY
};

enum {
    CHANGED,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

static void
moo_history_mgr_class_init (MooHistoryMgrClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (MooHistoryMgrPrivate));

    object_class->constructor = moo_history_mgr_constructor;
    object_class->set_property = moo_history_mgr_set_property;
    object_class->get_property = moo_history_mgr_get_property;
    object_class->dispose = moo_history_mgr_dispose;

    g_object_class_install_property (object_class, PROP_NAME,
        g_param_spec_string ("name", "name", "name",
                             NULL, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property (object_class, PROP_EMPTY,
        g_param_spec_boolean ("empty", "empty", "empty",
                              TRUE, G_PARAM_READABLE));

    signals[CHANGED] =
        _moo_signal_new_cb ("changed",
                            MOO_TYPE_HISTORY_MGR,
                            G_SIGNAL_RUN_LAST,
                            G_CALLBACK (schedule_update_widgets),
                            NULL, NULL,
                            _moo_marshal_VOID__VOID,
                            G_TYPE_NONE, 0);
}

static void
moo_history_mgr_init (MooHistoryMgr *mgr)
{
    mgr->priv = G_TYPE_INSTANCE_GET_PRIVATE (mgr, MOO_TYPE_HISTORY_MGR, MooHistoryMgrPrivate);
    mgr->priv->filename = NULL;
    mgr->priv->basename = NULL;
    mgr->priv->files = moo_history_item_queue_new ();
    mgr->priv->hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                             g_free, NULL);
    mgr->priv->loaded = FALSE;
}

static GObject *
moo_history_mgr_constructor (GType           type,
                             guint           n_props,
                             GObjectConstructParam *props)
{
    GObject *object;
    MooHistoryMgr *mgr;

    object = G_OBJECT_CLASS (moo_history_mgr_parent_class)->
                constructor (type, n_props, props);
    mgr = MOO_HISTORY_MGR (object);

    if (mgr->priv->name)
    {
        mgr->priv->ipc_id = g_strdup_printf (IPC_ID "/%s", mgr->priv->name);
        moo_ipc_register_client (G_OBJECT (mgr), mgr->priv->ipc_id, ipc_callback);
    }

    return object;
}

static void
moo_history_mgr_dispose (GObject *object)
{
    MooHistoryMgr *mgr = MOO_HISTORY_MGR (object);

    if (mgr->priv)
    {
        moo_history_mgr_shutdown (mgr);

        if (mgr->priv->files)
        {
            moo_history_item_queue_foreach (mgr->priv->files, (MooHistoryItemListFunc) moo_history_item_free, NULL);
            moo_history_item_queue_free_links (mgr->priv->files);
            g_hash_table_destroy (mgr->priv->hash);
        }

        g_free (mgr->priv->name);
        g_free (mgr->priv->filename);
        g_free (mgr->priv->basename);

        mgr->priv = NULL;
    }

    G_OBJECT_CLASS (moo_history_mgr_parent_class)->dispose (object);
}

void
moo_history_mgr_shutdown (MooHistoryMgr *mgr)
{
    g_return_if_fail (MOO_IS_HISTORY_MGR (mgr));

    if (!mgr->priv)
        return;

    if (mgr->priv->ipc_id)
    {
        moo_ipc_unregister_client (G_OBJECT (mgr), mgr->priv->ipc_id);
        g_free (mgr->priv->ipc_id);
        mgr->priv->ipc_id = NULL;
    }

    if (mgr->priv->save_idle)
    {
        g_source_remove (mgr->priv->save_idle);
        mgr->priv->save_idle = 0;
        moo_history_mgr_save (mgr);
    }

    if (mgr->priv->update_widgets_idle)
    {
        g_source_remove (mgr->priv->update_widgets_idle);
        mgr->priv->update_widgets_idle = 0;
    }

    while (mgr->priv->widgets)
        gtk_widget_destroy (mgr->priv->widgets->data);
}

static void
moo_history_mgr_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
    MooHistoryMgr *mgr = MOO_HISTORY_MGR (object);

    switch (prop_id)
    {
        case PROP_NAME:
            MOO_ASSIGN_STRING (mgr->priv->name, g_value_get_string (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
moo_history_mgr_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
    MooHistoryMgr *mgr = MOO_HISTORY_MGR (object);

    switch (prop_id)
    {
        case PROP_NAME:
            g_value_set_string (value, mgr->priv->name);
            break;

        case PROP_EMPTY:
            g_value_set_boolean (value, moo_history_mgr_get_n_items (mgr) == 0);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static const char *
get_basename (MooHistoryMgr *mgr)
{
    if (!mgr->priv->basename)
    {
        if (mgr->priv->name)
        {
            char *name = g_ascii_strdown (mgr->priv->name, -1);
            mgr->priv->basename = g_strdup_printf ("recent-files-%s.xml", name);
            g_free (name);
        }
        else
        {
            mgr->priv->basename = g_strdup ("recent-files.xml");
        }
    }

    return mgr->priv->basename;
}

static const char *
get_filename (MooHistoryMgr *mgr)
{
    if (!mgr->priv->filename)
        mgr->priv->filename = moo_get_user_cache_file (get_basename (mgr));
    return mgr->priv->filename;
}

char *
_moo_history_mgr_get_filename (MooHistoryMgr *mgr)
{
    g_return_val_if_fail (MOO_IS_HISTORY_MGR (mgr), NULL);
    return g_strdup (get_filename (mgr));
}


/*****************************************************************/
/* Loading and saving
 */

#define ELM_ROOT "md-recent-files"
#define ELM_UPDATE "md-recent-files-update"
#define ELM_ITEM "item"
#define ELM_DATA "data"
#define PROP_VERSION "version"
#define PROP_VERSION_VALUE "1.0"
#define PROP_URI "uri"
#define PROP_KEY "key"
#define PROP_VALUE "value"
#define PROP_TYPE "type"

#define ELM_RECENT_ITEMS "recent-items"

static void
add_file (MooHistoryMgr  *mgr,
          MooHistoryItem *item)
{
    const char *uri;

    uri = moo_history_item_get_uri (item);

    if (g_hash_table_lookup (mgr->priv->hash, uri) != NULL)
    {
        g_critical ("duplicated uri '%s'", uri);
        moo_history_item_free (item);
        return;
    }

    moo_history_item_queue_push_tail (mgr->priv->files, item);
    g_hash_table_insert (mgr->priv->hash, g_strdup (uri),
                         mgr->priv->files->tail);
}

static gboolean
parse_element (const char     *filename,
               MooMarkupNode  *elm,
               MooHistoryItem **item_p)
{
    const char *uri;
    MooHistoryItem *item;
    MooMarkupNode *child;

    if (strcmp (elm->name, ELM_ITEM) != 0)
    {
        g_critical ("in file '%s': invalid element '%s'",
                    filename ? filename : "NONE", elm->name);
        return FALSE;
    }

    if (!(uri = moo_markup_get_prop (elm, PROP_URI)))
    {
        g_critical ("in file '%s': attribute '%s' missing",
                    filename ? filename : "NONE", PROP_URI);
        return FALSE;
    }

    item = moo_history_item_new (uri, NULL);
    g_return_val_if_fail (item != NULL, FALSE);

    for (child = elm->children; child != NULL; child = child->next)
    {
        const char *key, *value;

        if (!MOO_MARKUP_IS_ELEMENT (child))
            continue;

        if (strcmp (child->name, ELM_DATA) != 0)
        {
            g_critical ("in file '%s': invalid element '%s'",
                        filename ? filename : "NONE", child->name);
            continue;
        }

        key = moo_markup_get_prop (child, PROP_KEY);
        value = moo_markup_get_prop (child, PROP_VALUE);

        if (!key || !key[0])
        {
            g_critical ("in file '%s': attribute '%s' missing",
                        filename ? filename : "NONE", PROP_KEY);
            continue;
        }
        else if (!value || !value[0])
        {
            g_critical ("in file '%s': attribute '%s' missing",
                        filename ? filename : "NONE", PROP_VALUE);
            continue;
        }

        moo_history_item_set (item, key, value);
    }

    *item_p = item;
    return TRUE;
}

typedef enum {
    ELEMENT_NONE = 0,
    ELEMENT_ROOT,
    ELEMENT_ITEM,
    ELEMENT_DATA
} Element;

typedef struct {
    gboolean seen_root;
    Element element;
    MooHistoryItem *item;
    MooHistoryMgr *mgr;
} ParserData;

static void
start_element_root (const gchar  *element_name,
                    const gchar **attribute_names,
                    const gchar **attribute_values,
                    ParserData   *data,
                    GError      **error)
{
    gboolean seen_version = FALSE;
    const char **p, **v;

    if (data->seen_root)
    {
        g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                     MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                     "invalid element '%s'", element_name);
        return;
    }

    if (strcmp (element_name, ELM_ROOT) != 0)
    {
        g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                     MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                     "invalid element '%s'", element_name);
        return;
    }

    for (p = attribute_names, v = attribute_values; p && *p; p++, v++)
    {
        if (seen_version || strcmp (*p, PROP_VERSION) != 0)
        {
            g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                         MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                         "invalid attribute '%s'", *p);
            return;
        }

        if (strcmp (*v, PROP_VERSION_VALUE) != 0)
        {
            g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                         MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                         "invalid version value '%s'", *v);
            return;
        }

        seen_version = TRUE;
    }

    if (!seen_version)
    {
        g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                     MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                     "version attribute missing");
        return;
    }

    data->seen_root = TRUE;
    data->element = ELEMENT_ROOT;
}

static void
start_element_item (const gchar  *element_name,
                    const gchar **attribute_names,
                    const gchar **attribute_values,
                    ParserData   *data,
                    GError      **error)
{
    MooHistoryItem *item = NULL;
    const char **p, **v;

    if (strcmp (element_name, ELM_ITEM) != 0)
    {
        g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                     MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                     "invalid element '%s'",
                     element_name);
        return;
    }

    data->element = ELEMENT_ITEM;

    for (p = attribute_names, v = attribute_values; p && *p; p++, v++)
    {
        if (strcmp (*p, PROP_URI) != 0)
        {
            g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                         MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                         "invalid attribute '%s'", *v);
            return;
        }

        if (item != NULL)
        {
            g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                         MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                         "duplicate attribute '%s'", *v);
            return;
        }

        item = moo_history_item_new (*v, NULL);
        g_return_if_fail (item != NULL);
    }

    if (!item)
    {
        g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                     MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                     "missing attribute '%s'", PROP_URI);
        return;
    }

    data->item = item;
}

static void
start_element_data (const gchar  *element_name,
                    const gchar **attribute_names,
                    const gchar **attribute_values,
                    ParserData   *data,
                    GError      **error)
{
    const char **p, **v;
    const char *key = NULL;
    const char *value = NULL;

    g_return_if_fail (data->item != NULL);

    if (strcmp (element_name, ELM_DATA) != 0)
    {
        g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                     MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                     "invalid element '%s'",
                     element_name);
        return;
    }

    data->element = ELEMENT_DATA;

    for (p = attribute_names, v = attribute_values; p && *p; p++, v++)
    {
        if (strcmp (*p, PROP_KEY) == 0)
        {
            key = *v;
        }
        else if (strcmp (*p, PROP_VALUE) == 0)
        {
            value = *v;
        }
        else
        {
            g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                         MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                         "invalid attribute '%s'", *v);
            return;
        }
    }

    if (!key || !key[0])
    {
        g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                     MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                     "missing attribute '%s'", PROP_KEY);
        return;
    }

    if (!value)
    {
        g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                     MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                     "missing attribute '%s'", PROP_VALUE);
        return;
    }

    moo_history_item_set (data->item, key, value);
}

static void
parser_start_element (G_GNUC_UNUSED GMarkupParseContext *context,
                      const gchar         *element_name,
                      const gchar        **attribute_names,
                      const gchar        **attribute_values,
                      ParserData          *data,
                      GError             **error)
{
    switch (data->element)
    {
        case ELEMENT_NONE:
            start_element_root (element_name, attribute_names,
                                attribute_values, data, error);
            break;
        case ELEMENT_ROOT:
            start_element_item (element_name, attribute_names,
                                attribute_values, data, error);
            break;
        case ELEMENT_ITEM:
            start_element_data (element_name, attribute_names,
                                attribute_values, data, error);
            break;
        case ELEMENT_DATA:
            g_set_error (error, MOO_HISTORY_MGR_PARSE_ERROR,
                         MOO_HISTORY_MGR_PARSE_ERROR_INVALID_CONTENT,
                         "invalid element '%s'", element_name);
            break;
    }
}

static void
parser_end_element (G_GNUC_UNUSED GMarkupParseContext *context,
                    G_GNUC_UNUSED const gchar *element_name,
                    ParserData *data,
                    G_GNUC_UNUSED GError **error)
{
    switch (data->element)
    {
        case ELEMENT_ROOT:
            data->element = ELEMENT_NONE;
            break;
        case ELEMENT_ITEM:
            data->element = ELEMENT_ROOT;
            if (data->item)
            {
                add_file (data->mgr, data->item);
                data->item = NULL;
            }
            break;
        case ELEMENT_DATA:
            data->element = ELEMENT_ITEM;
            break;
        case ELEMENT_NONE:
            g_assert_not_reached ();
            break;
    }
}

static void
load_file (MooHistoryMgr *mgr)
{
    const char *filename;
    GMarkupParser parser = {0};
    ParserData data = {0};
    GError *error = NULL;

    mgr->priv->loaded = TRUE;

    filename = get_filename (mgr);
    g_return_if_fail (filename != NULL);

    if (!g_file_test (filename, G_FILE_TEST_EXISTS))
        return;

    parser.start_element = (MooMarkupStartElementFunc) parser_start_element;
    parser.end_element = (MooMarkupEndElementFunc) parser_end_element;

    data.seen_root = FALSE;
    data.element = ELEMENT_NONE;
    data.item = NULL;
    data.mgr = mgr;

    if (!moo_parse_markup_file (filename, &parser, &data, &error))
    {
        g_critical ("could not load file '%s': %s",
                    filename, moo_error_message (error));
        g_error_free (error);
    }

    if (data.item)
        moo_history_item_free (data.item);
}

static gboolean
parse_update_item (MooMarkupDoc   *xml,
                   MooHistoryItem **item,
                   UpdateType     *type)
{
    const char *version;
    const char *update_type_string;
    MooMarkupNode *root, *child;

    if (!(root = moo_markup_get_root_element (xml, ELM_UPDATE)))
    {
        g_critical ("missing element %s", ELM_UPDATE);
        return FALSE;
    }

    if (!(version = moo_markup_get_prop (root, PROP_VERSION)) ||
        strcmp (version, PROP_VERSION_VALUE) != 0)
    {
        g_critical ("invalid version value '%s'",
                    version ? version : "(null)");
        return FALSE;
    }

    if (!(update_type_string = moo_markup_get_prop (root, PROP_TYPE)))
    {
        g_critical ("attribute '%s' missing", PROP_TYPE);
        return FALSE;
    }

    if (strcmp (update_type_string, "add") == 0)
        *type = UPDATE_ITEM_ADD;
    else if (strcmp (update_type_string, "remove") == 0)
        *type = UPDATE_ITEM_REMOVE;
    else if (strcmp (update_type_string, "update") == 0)
        *type = UPDATE_ITEM_UPDATE;
    else
    {
        g_critical ("invalid value '%s' for attribute '%s'",
                    update_type_string, PROP_TYPE);
        return FALSE;
    }

    for (child = root->children; child != NULL; child = child->next)
        if (MOO_MARKUP_IS_ELEMENT (child))
            return parse_element (NULL, child, item);

    g_critical ("element '%s' missing", ELM_ITEM);
    return FALSE;
}

static void
ensure_files (MooHistoryMgr *mgr)
{
    if (!mgr->priv->loaded)
        load_file (mgr);
}


static gboolean
save_in_idle (MooHistoryMgr *mgr)
{
    mgr->priv->save_idle = 0;
    moo_history_mgr_save (mgr);
    return FALSE;
}

static void
schedule_save (MooHistoryMgr *mgr)
{
    if (!mgr->priv->save_idle)
        mgr->priv->save_idle = g_idle_add ((GSourceFunc) save_in_idle, mgr);
}

static void
moo_history_mgr_save (MooHistoryMgr *mgr)
{
    const char *filename;
    GError *error = NULL;
    MooFileWriter *writer;

    g_return_if_fail (MOO_IS_HISTORY_MGR (mgr));

    if (!mgr->priv->files)
        return;

    filename = get_filename (mgr);

    if (!mgr->priv->files->length)
    {
        mgw_errno_t err;
        mgw_unlink (filename, &err);
        return;
    }

    if ((writer = moo_config_writer_new (filename, FALSE, &error)))
    {
        GString *string;
        MooHistoryItemList *l;

        moo_file_writer_write (writer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n", -1);
        moo_file_writer_write (writer, "<" ELM_ROOT " " PROP_VERSION "=\"" PROP_VERSION_VALUE "\">\n", -1);

        string = g_string_new (NULL);

        for (l = mgr->priv->files->head; l != NULL; l = l->next)
        {
            MooHistoryItem *item = l->data;
            g_string_truncate (string, 0);
            moo_history_item_format (item, string);
            if (!moo_file_writer_write (writer, string->str, -1))
                break;
        }

        g_string_free (string, TRUE);

        moo_file_writer_write (writer, "</" ELM_ROOT ">\n", -1);
        moo_file_writer_close (writer, &error);
    }

    if (error)
    {
        g_critical ("could not save file '%s': %s",
                    filename, moo_error_message (error));
        g_error_free (error);
    }
}


static char *
format_for_update (MooHistoryItem *item,
                   UpdateType     type)
{
    GString *buffer;
    const char *update_types[3] = {"update", "remove", "add"};

    g_return_val_if_fail (type < 3, NULL);

    buffer = g_string_new (NULL);
    g_string_append_printf (buffer, "<%s %s=\"%s\" %s=\"%s\">\n",
                            ELM_UPDATE, PROP_VERSION, PROP_VERSION_VALUE,
                            PROP_TYPE, update_types[type]);

    moo_history_item_format (item, buffer);

    g_string_append (buffer, "</" ELM_UPDATE ">\n");

    return g_string_free (buffer, FALSE);
}


guint
moo_history_mgr_get_n_items (MooHistoryMgr *mgr)
{
    g_return_val_if_fail (MOO_IS_HISTORY_MGR (mgr), 0);
    ensure_files (mgr);
    return mgr->priv->files->length;
}


void
moo_history_mgr_add_uri (MooHistoryMgr *mgr,
                        const char   *uri)
{
    MooHistoryItem *freeme = NULL;
    MooHistoryItem *item;

    g_return_if_fail (MOO_IS_HISTORY_MGR (mgr));
    g_return_if_fail (uri && uri[0]);

    if (!(item = moo_history_mgr_find_uri (mgr, uri)))
    {
        freeme = moo_history_item_new (uri, NULL);
        item = freeme;
    }

    moo_history_mgr_add_file (mgr, item);

    moo_history_item_free (freeme);
}

static void
moo_history_mgr_add_file_real (MooHistoryMgr  *mgr,
                              MooHistoryItem *item,
                              gboolean       notify)
{
    const char *uri;
    MooHistoryItemList *link;
    MooHistoryItem *new_item = NULL;

    g_return_if_fail (MOO_IS_HISTORY_MGR (mgr));
    g_return_if_fail (item != NULL);

    ensure_files (mgr);

    uri = moo_history_item_get_uri (item);
    link = (MooHistoryItemList*) g_hash_table_lookup (mgr->priv->hash, uri);

    if (!link)
    {
        MooHistoryItem *copy = moo_history_item_copy (item);
        moo_history_item_queue_push_head (mgr->priv->files, copy);
        new_item = copy;
        g_hash_table_insert (mgr->priv->hash, g_strdup (uri),
                             mgr->priv->files->head);
    }
    else if (link != mgr->priv->files->head ||
            !moo_history_item_equal (item, link->data))
    {
        MooHistoryItem *tmp = link->data;

        moo_history_item_queue_unlink (mgr->priv->files, link);
        moo_history_item_queue_push_head_link (mgr->priv->files, link);

        new_item = link->data = moo_history_item_copy (item);

        moo_history_item_free (tmp);
    }

    if (new_item)
    {
        g_signal_emit (mgr, signals[CHANGED], 0);

        if (notify)
        {
            schedule_save (mgr);
            ipc_notify_add_file (mgr, new_item);
        }

        if (mgr->priv->files->length == 1)
            g_object_notify (G_OBJECT (mgr), "empty");
    }

    if (mgr->priv->files->length > MAX_ITEM_NUMBER)
        moo_history_mgr_remove_uri (mgr,
            moo_history_item_get_uri (mgr->priv->files->tail->data));
}

void
moo_history_mgr_add_file (MooHistoryMgr  *mgr,
                         MooHistoryItem *item)
{
    moo_history_mgr_add_file_real (mgr, item, TRUE);
}

static void
moo_history_mgr_update_file_real (MooHistoryMgr  *mgr,
                                 MooHistoryItem *file,
                                 gboolean       notify)
{
    const char *uri;
    MooHistoryItemList *link;

    g_return_if_fail (MOO_IS_HISTORY_MGR (mgr));
    g_return_if_fail (file != NULL);

    ensure_files (mgr);

    uri = moo_history_item_get_uri (file);
    link = (MooHistoryItemList*) g_hash_table_lookup (mgr->priv->hash, uri);

    if (!link)
    {
        moo_history_mgr_add_file (mgr, file);
    }
    else if (!moo_history_item_equal (link->data, file))
    {
        MooHistoryItem *tmp = link->data;
        link->data = moo_history_item_copy (file);
        moo_history_item_free (tmp);
        g_signal_emit (mgr, signals[CHANGED], 0);

        if (notify)
        {
            schedule_save (mgr);
            ipc_notify_update_file (mgr, link->data);
        }
    }
}

void
moo_history_mgr_update_file (MooHistoryMgr  *mgr,
                            MooHistoryItem *file)
{
    moo_history_mgr_update_file_real (mgr, file, TRUE);
}

MooHistoryItem *
moo_history_mgr_find_uri (MooHistoryMgr *mgr,
                         const char   *uri)
{
    MooHistoryItemList *link;

    g_return_val_if_fail (MOO_IS_HISTORY_MGR (mgr), NULL);
    g_return_val_if_fail (uri != NULL, NULL);

    ensure_files (mgr);

    link = (MooHistoryItemList*) g_hash_table_lookup (mgr->priv->hash, uri);

    return link ? link->data : NULL;
}

static void
moo_history_mgr_remove_uri_real (MooHistoryMgr *mgr,
                                const char   *uri,
                                gboolean      notify)
{
    MooHistoryItemList *link;
    MooHistoryItem *item;

    g_return_if_fail (MOO_IS_HISTORY_MGR (mgr));
    g_return_if_fail (uri != NULL);

    ensure_files (mgr);

    link = (MooHistoryItemList*) g_hash_table_lookup (mgr->priv->hash, uri);

    if (!link)
        return;

    item = link->data;

    g_hash_table_remove (mgr->priv->hash, uri);
    moo_history_item_queue_delete_link (mgr->priv->files, link);

    g_signal_emit (mgr, signals[CHANGED], 0);

    if (notify)
    {
        schedule_save (mgr);
        ipc_notify_remove_file (mgr, item);
    }

    if (mgr->priv->files->length == 0)
        g_object_notify (G_OBJECT (mgr), "empty");

    moo_history_item_free (item);
}

void
moo_history_mgr_remove_uri (MooHistoryMgr *mgr,
                           const char   *uri)
{
    moo_history_mgr_remove_uri_real (mgr, uri, TRUE);
}


static void
ipc_callback (GObject    *obj,
              const char *data,
              gsize       len)
{
    MooHistoryMgr *mgr;
    MooMarkupDoc *xml;
    GError *error = NULL;
    MooHistoryItem *item;
    UpdateType type;

    g_return_if_fail (MOO_IS_HISTORY_MGR (obj));

    mgr = MOO_HISTORY_MGR (obj);
    ensure_files (mgr);

    xml = moo_markup_parse_memory (data, len, &error);

    if (!xml)
    {
        g_critical ("got invalid data: %.*s", (int) len, data);
        return;
    }

#if 0
    g_print ("%s: got data: %.*s\n", G_STRLOC, (int) len, data);
#endif

    if (parse_update_item (xml, &item, &type))
    {
        switch (type)
        {
            case UPDATE_ITEM_UPDATE:
                moo_history_mgr_update_file_real (mgr, item, FALSE);
                break;
            case UPDATE_ITEM_ADD:
                moo_history_mgr_add_file_real (mgr, item, FALSE);
                break;
            case UPDATE_ITEM_REMOVE:
                moo_history_mgr_remove_uri_real (mgr, moo_history_item_get_uri (item), FALSE);
                break;
        }

        moo_history_item_free (item);
    }

    moo_markup_doc_unref (xml);
}

static void
ipc_notify (MooHistoryMgr  *mgr,
            MooHistoryItem *item,
            UpdateType     type)
{
    if (mgr->priv->ipc_id)
    {
        char *string = format_for_update (item, type);
        moo_ipc_send (G_OBJECT (mgr), mgr->priv->ipc_id, string, -1);
        g_free (string);
    }
}

static void
ipc_notify_add_file (MooHistoryMgr  *mgr,
                     MooHistoryItem *item)
{
    ipc_notify (mgr, item, UPDATE_ITEM_ADD);
}

static void
ipc_notify_update_file (MooHistoryMgr  *mgr,
                        MooHistoryItem *item)
{
    ipc_notify (mgr, item, UPDATE_ITEM_UPDATE);
}

static void
ipc_notify_remove_file (MooHistoryMgr  *mgr,
                        MooHistoryItem *item)
{
    ipc_notify (mgr, item, UPDATE_ITEM_REMOVE);
}


/*****************************************************************/
/* Menu
 */

static void
callback_data_free (CallbackData *data)
{
    if (data)
    {
        if (data->notify)
            data->notify (data->data);
        g_slice_free (CallbackData, data);
    }
}

static void
view_destroyed (GtkWidget    *widget,
                MooHistoryMgr *mgr)
{
    g_object_set_data (G_OBJECT (widget), "moo-history-mgr-callback-data", NULL);
    mgr->priv->widgets = widget_list_remove (mgr->priv->widgets, widget);
}

static void
update_menu (MooHistoryMgr *mgr,
             GtkWidget    *menu)
{
    WidgetList *children;

    children = widget_list_from_glist (gtk_container_get_children (GTK_CONTAINER (menu)));

    while (children)
    {
        GtkWidget *item = children->data;

        if (g_object_get_data (G_OBJECT (item), "moo-history-menu-item-file"))
            gtk_widget_destroy (item);

        children = widget_list_delete_link (children, children);
    }

    populate_menu (mgr, menu);
}

GtkWidget *
moo_history_mgr_create_menu (MooHistoryMgr   *mgr,
                            MooHistoryCallback callback,
                            gpointer        data,
                            GDestroyNotify  notify)
{
    GtkWidget *menu;
    CallbackData *cb_data;

    g_return_val_if_fail (MOO_IS_HISTORY_MGR (mgr), NULL);
    g_return_val_if_fail (callback != NULL, NULL);

    menu = gtk_menu_new ();
    gtk_widget_show (menu);
    g_signal_connect (menu, "destroy", G_CALLBACK (view_destroyed), mgr);

    cb_data = g_slice_new0 (CallbackData);
    cb_data->callback = callback;
    cb_data->data = data;
    cb_data->notify = notify;
    g_object_set_data_full (G_OBJECT (menu), "moo-history-mgr-callback-data",
                            cb_data, (GDestroyNotify) callback_data_free);

    populate_menu (mgr, menu);
    mgr->priv->widgets = widget_list_prepend (mgr->priv->widgets, menu);

    return menu;
}

static void
menu_item_activated (GtkWidget *menu_item)
{
    GtkWidget *parent = menu_item->parent;
    CallbackData *data;
    MooHistoryItem *item;
    GSList *list;

    g_return_if_fail (parent != NULL);

    data = (CallbackData*) g_object_get_data (G_OBJECT (parent), "moo-history-mgr-callback-data");
    item = (MooHistoryItem*) g_object_get_data (G_OBJECT (menu_item), "moo-history-menu-item-file");
    g_return_if_fail (data && item);

    list = g_slist_prepend (NULL, moo_history_item_copy (item));
    data->callback (list, data->data);
    moo_history_item_free ((MooHistoryItem*) list->data);
    g_slist_free (list);
}

static void
populate_menu (MooHistoryMgr *mgr,
               GtkWidget    *menu)
{
    guint n_items, i;
    MooHistoryItemList *l;

    ensure_files (mgr);

    n_items = MIN (mgr->priv->files->length, N_MENU_ITEMS);

    for (i = 0, l = mgr->priv->files->head; i < n_items; i++, l = l->next)
    {
        GtkWidget *item, *image;
        MooHistoryItem *hist_item = l->data;
        char *display_name, *display_basename;
        GdkPixbuf *pixbuf;

        display_basename = uri_get_basename (hist_item->uri);
        display_name = uri_get_display_name (hist_item->uri);

        item = gtk_image_menu_item_new_with_label (display_basename);
        _moo_widget_set_tooltip (item, display_name);
        gtk_widget_show (item);
        gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, i);

        /* XXX */
        pixbuf = moo_file_icon_get_pixbuf (moo_history_item_get_icon (hist_item),
                                           GTK_WIDGET (item),
                                           GTK_ICON_SIZE_MENU);
        image = gtk_image_new_from_pixbuf (pixbuf);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);

        g_object_set_data_full (G_OBJECT (item), "moo-history-menu-item-file",
                                moo_history_item_copy (hist_item),
                                (GDestroyNotify) moo_history_item_free);
        g_signal_connect (item, "activate", G_CALLBACK (menu_item_activated), NULL);

        g_free (display_basename);
        g_free (display_name);
    }
}


enum {
    COLUMN_PIXBUF,
    COLUMN_NAME,
    COLUMN_TOOLTIP,
    COLUMN_URI,
    N_COLUMNS
};

static void
open_selected (GtkTreeView *tree_view)
{
    CallbackData *data;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    MooHistoryMgr *mgr;
    GList *selected;
    GSList *items;

    mgr = (MooHistoryMgr*) g_object_get_data (G_OBJECT (tree_view), "moo-history-mgr");
    g_return_if_fail (MOO_IS_HISTORY_MGR (mgr));

    data = (CallbackData*) g_object_get_data (G_OBJECT (tree_view), "moo-history-mgr-callback-data");
    g_return_if_fail (data != NULL);

    selection = gtk_tree_view_get_selection (tree_view);
    selected = gtk_tree_selection_get_selected_rows (selection, &model);

    for (items = NULL; selected != NULL; )
    {
        char *uri = NULL;
        MooHistoryItem *item;
        GtkTreePath *path = (GtkTreePath*) selected->data;

        gtk_tree_model_get_iter (model, &iter, path);
        gtk_tree_model_get (model, &iter, COLUMN_URI, &uri, -1);
        item = moo_history_mgr_find_uri (mgr, uri);

        if (item)
            items = g_slist_prepend (items, moo_history_item_copy (item));

        g_free (uri);
        gtk_tree_path_free (path);
        selected = g_list_delete_link (selected, selected);
    }

    items = g_slist_reverse (items);

    if (items)
        data->callback (items, data->data);

    g_slist_foreach (items, (GFunc) moo_history_item_free, NULL);
    g_slist_free (items);
}

static GtkWidget *
create_tree_view (void)
{
    GtkWidget *tree_view;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;
    GtkListStore *store;

    tree_view = gtk_tree_view_new ();
    store = gtk_list_store_new (N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING,
                                G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (store));
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);
    gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (tree_view),
                                      COLUMN_TOOLTIP);
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)),
                                 GTK_SELECTION_MULTIPLE);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

    cell = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (column, cell, FALSE);
    gtk_tree_view_column_set_attributes (column, cell, "pixbuf", COLUMN_PIXBUF, NULL);

    cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, cell, TRUE);
    gtk_tree_view_column_set_attributes (column, cell, "text", COLUMN_NAME, NULL);

    return tree_view;
}

#define FIRST_CHUNK_SIZE 100
#define CHUNK_SIZE 100
#define LOADING_TIMEOUT 40
#define LOADING_PRIORITY G_PRIORITY_DEFAULT_IDLE

typedef struct {
    MooHistoryItemList *items;
    guint idle;
    GtkWidget *tree_view;
    GtkListStore *store;
} IdleLoader;

static void
cancel_idle_loading (GtkWidget *tree_view)
{
    g_object_set_data (G_OBJECT (tree_view),
                       "moo-history-mgr-idle-loader",
                       NULL);
}

static void
idle_loader_free (IdleLoader *data)
{
    if (data->idle)
        g_source_remove (data->idle);
    moo_history_item_list_foreach (data->items, (MooHistoryItemListFunc) moo_history_item_free, NULL);
    moo_history_item_list_free_links (data->items);
    g_free (data);
}

static void
add_entry (MooHistoryItem *item,
           GtkListStore  *store,
           GtkWidget     *tree_view)
{
    char *display_name, *display_basename;
    GdkPixbuf *pixbuf;
    GtkTreeIter iter;

    display_basename = uri_get_basename (item->uri);
    display_name = uri_get_display_name (item->uri);
    pixbuf = moo_file_icon_get_pixbuf (moo_history_item_get_icon (item),
                                       tree_view, GTK_ICON_SIZE_MENU);

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
                        COLUMN_PIXBUF, pixbuf,
                        COLUMN_NAME, display_basename,
                        COLUMN_TOOLTIP, display_name,
                        COLUMN_URI, moo_history_item_get_uri (item),
                        -1);

    g_free (display_basename);
    g_free (display_name);
}

static gboolean
idle_loader (IdleLoader *data)
{
    int count;

    for (count = 0; data->items != NULL && count < CHUNK_SIZE; count++)
    {
        add_entry (data->items->data, data->store, data->tree_view);
        moo_history_item_free (data->items->data);
        data->items = moo_history_item_list_delete_link (data->items, data->items);
    }

    if (!data->items)
    {
        data->idle = 0;
        g_object_set_data (G_OBJECT (data->tree_view),
                           "moo-history-mgr-idle-loader",
                           NULL);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

static void
populate_tree_view (MooHistoryMgr *mgr,
                    GtkWidget    *tree_view)
{
    GtkListStore *store;
    GtkTreeModel *model;
    MooHistoryItemList *l;
    int count;

    ensure_files (mgr);

    cancel_idle_loading (tree_view);

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));
    store = GTK_LIST_STORE (model);

    for (l = mgr->priv->files->head, count = 0;
         l != NULL && count < FIRST_CHUNK_SIZE;
         l = l->next, count++)
    {
        add_entry (l->data, store, tree_view);
    }

    if (l != NULL)
    {
        IdleLoader *data = g_new0 (IdleLoader, 1);

        while (l != NULL)
        {
            data->items = moo_history_item_list_prepend (data->items, moo_history_item_copy (l->data));
            l = l->next;
        }

        data->items = moo_history_item_list_reverse (data->items);
        data->idle = g_timeout_add_full (LOADING_PRIORITY,
                                         LOADING_TIMEOUT,
                                         (GSourceFunc) idle_loader,
                                         data, NULL);
        data->tree_view = tree_view;
        data->store = store;

        g_object_set_data_full (G_OBJECT (tree_view),
                                "moo-history-mgr-idle-loader", data,
                                (GDestroyNotify) idle_loader_free);
    }
}

static void
update_tree_view (MooHistoryMgr *mgr,
                  GtkWidget    *tree_view)
{
    GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));
    gtk_list_store_clear (GTK_LIST_STORE (model));
    populate_tree_view (mgr, tree_view);
}

static GtkWidget *
moo_history_mgr_create_tree_view (MooHistoryMgr   *mgr,
                                  MooHistoryCallback callback,
                                  gpointer        data,
                                  GDestroyNotify  notify)
{
    GtkWidget *tree_view;
    CallbackData *cb_data;

    g_return_val_if_fail (MOO_IS_HISTORY_MGR (mgr), NULL);
    g_return_val_if_fail (callback != NULL, NULL);

    tree_view = create_tree_view ();
    gtk_widget_show (tree_view);
    g_signal_connect (tree_view, "destroy", G_CALLBACK (view_destroyed), mgr);

    cb_data = g_slice_new0 (CallbackData);
    cb_data->callback = callback;
    cb_data->data = data;
    cb_data->notify = notify;
    g_object_set_data_full (G_OBJECT (tree_view), "moo-history-mgr-callback-data",
                            cb_data, (GDestroyNotify) callback_data_free);
    g_object_set_data (G_OBJECT (tree_view), "moo-history-mgr", mgr);

    populate_tree_view (mgr, tree_view);
    if (mgr->priv->files->head)
        _moo_tree_view_select_first (GTK_TREE_VIEW (tree_view));
    mgr->priv->widgets = widget_list_prepend (mgr->priv->widgets, tree_view);

    return tree_view;
}

static void
dialog_response (GtkTreeView *tree_view,
                 int          response)
{
    if (response == GTK_RESPONSE_OK)
        open_selected (tree_view);
}

static void
row_activated (GtkDialog *dialog)
{
    gtk_dialog_response (dialog, GTK_RESPONSE_OK);
}

GtkWidget *
moo_history_mgr_create_dialog (MooHistoryMgr   *mgr,
                               MooHistoryCallback callback,
                               gpointer        data,
                               GDestroyNotify  notify)
{
    GtkWidget *dialog, *swin, *tree_view;

    g_return_val_if_fail (MOO_IS_HISTORY_MGR (mgr), NULL);
    g_return_val_if_fail (callback != NULL, NULL);

    dialog = gtk_dialog_new_with_buttons ("", NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_OK,
                                          NULL);
    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_OK,
                                             GTK_RESPONSE_CANCEL,
                                             -1);

    swin = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    tree_view = moo_history_mgr_create_tree_view (mgr, callback, data, notify);
    gtk_container_add (GTK_CONTAINER (swin), tree_view);
    gtk_widget_show_all (swin);

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), swin, TRUE, TRUE, 0);

    g_signal_connect_swapped (tree_view, "row-activated",
                              G_CALLBACK (row_activated), dialog);
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (dialog_response), tree_view);

    return dialog;
}


static gboolean
do_update_widgets (MooHistoryMgr *mgr)
{
    WidgetList *l;

    mgr->priv->update_widgets_idle = 0;

    for (l = mgr->priv->widgets; l != NULL; l = l->next)
    {
        GtkWidget *widget = l->data;

        if (GTK_IS_MENU (widget))
            update_menu (mgr, widget);
        else if (GTK_IS_TREE_VIEW (widget))
            update_tree_view (mgr, widget);
        else
            g_critical ("%s: oops", G_STRFUNC);
    }

    return FALSE;
}

static void
schedule_update_widgets (MooHistoryMgr *mgr)
{
    if (!mgr->priv->update_widgets_idle && mgr->priv->widgets)
        mgr->priv->update_widgets_idle =
            g_idle_add ((GSourceFunc) do_update_widgets, mgr);
}


void
moo_history_item_free (MooHistoryItem *item)
{
    if (item)
    {
        g_free (item->uri);
        g_datalist_clear (&item->data);
        moo_file_icon_free (item->icon);
        g_slice_free (MooHistoryItem, item);
    }
}

static MooHistoryItem *
moo_history_item_new_uri (const char *uri)
{
    MooHistoryItem *item = g_slice_new (MooHistoryItem);
    item->uri = g_strdup (uri);
    item->data = NULL;
    item->icon = NULL;
    return item;
}

static MooHistoryItem *
moo_history_item_newv (const char *uri,
                       const char *first_key,
                       va_list     args)
{
    const char *key;
    MooHistoryItem *item;

    item = moo_history_item_new_uri (uri);

    for (key = first_key; key != NULL; )
    {
        const char *value = va_arg (args, const char *);
        moo_history_item_set (item, key, value);
        key = va_arg (args, const char *);
    }

    return item;
}

MooHistoryItem *
moo_history_item_new (const char *uri,
                      const char *first_key,
                      ...)
{
    va_list args;
    MooHistoryItem *item;

    g_return_val_if_fail (uri != NULL, NULL);

    va_start (args, first_key);
    item = moo_history_item_newv (uri, first_key, args);
    va_end (args);

    return item;
}

static void
copy_data (GQuark         key,
           const char    *value,
           MooHistoryItem *dest)
{
    g_datalist_id_set_data_full (&dest->data, key, g_strdup (value), g_free);
}

MooHistoryItem *
moo_history_item_copy (MooHistoryItem *item)
{
    MooHistoryItem *copy;

    if (!item)
        return NULL;

    copy = moo_history_item_new_uri (item->uri);
    g_datalist_foreach (&item->data, (GDataForeachFunc) copy_data, copy);
    copy->icon = moo_file_icon_copy (item->icon);

    return copy;
}

typedef struct {
    MooHistoryItem *item;
    gboolean equal;
} CmpData;

static void
cmp_data (GQuark      key,
          const char *value,
          CmpData    *data)
{
    const char *value2;

    if (!data->equal)
        return;

    value2 = (const char*) g_datalist_id_get_data (&data->item->data, key);
    if (!value2 || strcmp (value2, value) != 0)
        data->equal = FALSE;
}

static gboolean
moo_history_item_equal (MooHistoryItem *item1,
                        MooHistoryItem *item2)
{
    CmpData data;

    g_return_val_if_fail (item1 && item2, FALSE);

    if (item1 == item2)
        return TRUE;

    if (strcmp (item1->uri, item2->uri) != 0)
        return FALSE;

    data.equal = TRUE;

    data.item = item1;
    g_datalist_foreach (&item2->data, (GDataForeachFunc) cmp_data, &data);

    if (data.equal)
    {
        data.item = item2;
        g_datalist_foreach (&item1->data, (GDataForeachFunc) cmp_data, &data);
    }

    return data.equal;
}

void
moo_history_item_set (MooHistoryItem *item,
                      const char    *key,
                      const char    *value)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (key != NULL);

    if (value)
        g_datalist_set_data_full (&item->data, key, g_strdup (value), g_free);
    else
        g_datalist_remove_data (&item->data, key);
}

const char *
moo_history_item_get (MooHistoryItem *item,
                      const char    *key)
{
    g_return_val_if_fail (item != NULL, NULL);
    g_return_val_if_fail (key != NULL, NULL);
    return (const char*) g_datalist_get_data (&item->data, key);
}

const char *
moo_history_item_get_uri (MooHistoryItem *item)
{
    g_return_val_if_fail (item != NULL, NULL);
    return item->uri;
}

static MooFileIcon *
moo_history_item_get_icon (MooHistoryItem *item)
{
    g_return_val_if_fail (item != NULL, NULL);

    if (!item->icon)
    {
        char *display_name;

        item->icon = moo_file_icon_new ();

        display_name = uri_get_display_name (item->uri);
        /* XXX */
        moo_file_icon_for_file (item->icon, display_name);

        g_free (display_name);
    }

    return item->icon;
}

static void
format_data (GQuark      key_id,
             const char *value,
             GString    *dest)
{
    const char *key = g_quark_to_string (key_id);
    char *key_escaped = g_markup_escape_text (key, -1);
    char *value_escaped = g_markup_escape_text (value, -1);
    g_string_append_printf (dest, "    <data key=\"%s\" value=\"%s\"/>\n",
                            key_escaped, value_escaped);
    g_free (value_escaped);
    g_free (key_escaped);
}

static void
moo_history_item_format (MooHistoryItem *item,
                         GString       *dest)
{
    char *uri_escaped;

    g_return_if_fail (item != NULL);
    g_return_if_fail (dest != NULL);

    uri_escaped = g_markup_escape_text (item->uri, -1);

    if (item->data)
    {
        g_string_append_printf (dest, "  <item uri=\"%s\">\n", uri_escaped);
        g_datalist_foreach (&item->data, (GDataForeachFunc) format_data, dest);
        g_string_append (dest, "  </item>\n");
    }
    else
    {
        g_string_append_printf (dest, "  <item uri=\"%s\"/>\n", uri_escaped);
    }

    g_free (uri_escaped);
}

void
moo_history_item_foreach (MooHistoryItem    *item,
                          GDataForeachFunc  func,
                          gpointer          user_data)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (func != NULL);
    g_datalist_foreach (&item->data, func, user_data);
}


static char *
uri_get_basename (const char *uri)
{
    const char *last_slash;

    g_return_val_if_fail (uri != NULL, NULL);

    if (g_str_has_prefix (uri, "file://"))
    {
        char *filename = g_filename_from_uri (uri, NULL, NULL);

        if (filename)
        {
            char *display_name = g_filename_display_basename (filename);

            if (display_name)
            {
                g_free (filename);
                return display_name;
            }

            g_free (filename);
        }
    }

    /* XXX percent encoding */
    last_slash = strrchr (uri, '/');
    if (last_slash)
        return g_strdup (last_slash + 1);
    else
        return g_strdup (uri);
}

static char *
uri_get_display_name (const char *uri)
{
    g_return_val_if_fail (uri != NULL, NULL);

    if (g_str_has_prefix (uri, "file://"))
    {
        char *filename = g_filename_from_uri (uri, NULL, NULL);

        if (filename)
        {
            char *display_name = g_filename_display_name (filename);

            if (display_name)
            {
                g_free (filename);
                return display_name;
            }

            g_free (filename);
        }
    }

    return g_strdup (uri);
}
