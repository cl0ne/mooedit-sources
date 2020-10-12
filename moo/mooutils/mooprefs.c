/*
 *   mooprefs.c
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

#include "mooutils/mooprefs.h"
#include "marshals.h"
#include "mooutils/moomarkup.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/mootype-macros.h"
#include <string.h>
#include <gobject/gvaluecollector.h>
#include <mooglib/moo-glib.h>

#define MOO_PREFS_ELEMENT "moo-prefs"
#define PROP_VERSION "version"
#define MOO_PREFS_VERSION "1.0"

#define PREFS_ROOT "Prefs"
#undef MOO_PREFS_DEBUG_READWRITE

#define MOO_PREFS_SYS -1

typedef struct
{
    GHashTable      *data; /* char* -> Item* */
    MooMarkupDoc    *xml_rc;
    MooMarkupDoc    *xml_state;
    gboolean         rc_modified;
} PrefsStore;

typedef struct {
    GType  type;
    GValue value;
    GValue default_value;
    guint  prefs_kind : 1;
    guint  overridden : 1;
} PrefsItem;

static PrefsItem    *prefs_get_item     (const char     *key);

static PrefsItem    *item_new           (GType           type,
                                         const GValue   *value,
                                         const GValue   *default_value,
                                         MooPrefsKind    prefs_kind);
static void          item_free          (PrefsItem      *item);
static gboolean      item_set_type      (PrefsItem      *item,
                                         GType           type);
static gboolean      item_set           (PrefsItem      *item,
                                         const GValue   *value);
static gboolean      item_set_default   (PrefsItem      *item,
                                         const GValue   *value);

static void          moo_prefs_set_modified (gboolean    modified);


static PrefsStore *
prefs_instance (void)
{
    static PrefsStore prefs;

    if (G_UNLIKELY (!prefs.data))
        prefs.data =
            g_hash_table_new_full (g_str_hash, g_str_equal,
                                   (GDestroyNotify) g_free,
                                   (GDestroyNotify) item_free);

    return &prefs;
}


char *
moo_prefs_make_key (const char *first_comp,
                    ...)
{
    char *key;
    va_list var_args;

    g_return_val_if_fail (first_comp != NULL, NULL);

    va_start (var_args, first_comp);
    key = moo_prefs_make_keyv (first_comp, var_args);
    va_end (var_args);

    return key;
}


char *
moo_prefs_make_keyv (const char *first_comp,
                     va_list     var_args)
{
    const char *comp;
    GString *key = NULL;

    g_return_val_if_fail (first_comp != NULL, NULL);

    comp = first_comp;

    while (comp)
    {
        if (!key)
        {
            key = g_string_new (comp);
        }
        else
        {
            g_string_append_c (key, '/');
            g_string_append (key, comp);
        }

        comp = va_arg (var_args, const char*);
    }

    return g_string_free (key, FALSE);
}


static MooMarkupDoc *
create_markup_doc (void)
{
    MooMarkupDoc *doc = moo_markup_doc_new ("Prefs");
    MooMarkupNode *root = moo_markup_create_root_element (doc, MOO_PREFS_ELEMENT);
    moo_markup_set_prop (root, PROP_VERSION, MOO_PREFS_VERSION);
    return doc;
}

MooMarkupNode *
moo_prefs_get_markup (MooPrefsKind prefs_kind)
{
    PrefsStore *prefs = prefs_instance ();

    if (!prefs->xml_rc)
        prefs->xml_rc = create_markup_doc ();
    if (!prefs->xml_state)
        prefs->xml_state = create_markup_doc ();

    switch (prefs_kind)
    {
        case MOO_PREFS_RC:
            return moo_markup_get_root_element (prefs->xml_rc, MOO_PREFS_ELEMENT);
        case MOO_PREFS_STATE:
            return moo_markup_get_root_element (prefs->xml_state, MOO_PREFS_ELEMENT);
    }

    g_return_val_if_reached (NULL);
}


void
moo_prefs_create_key (const char   *key,
                      MooPrefsKind  prefs_kind,
                      GType         value_type,
                      ...)
{
    va_list args;
    GValue default_value;
    char *error = NULL;

    g_return_if_fail (key != NULL);
    g_return_if_fail (prefs_kind < 2);
    g_return_if_fail (_moo_value_type_supported (value_type));

    default_value.g_type = 0;
    g_value_init (&default_value, value_type);

    va_start (args, value_type);
    G_VALUE_COLLECT (&default_value, args, 0, &error);
    va_end (args);

    if (error)
    {
        g_warning ("could not read value: %s", error);
        g_free (error);
        return;
    }

    moo_prefs_new_key (key, value_type, &default_value, prefs_kind);
    g_value_unset (&default_value);
}


GType
moo_prefs_get_key_type (const char *key)
{
    PrefsItem *item = prefs_get_item (key);
    g_return_val_if_fail (item != NULL, G_TYPE_NONE);
    return item->type;
}


gboolean
moo_prefs_key_registered (const char *key)
{
    g_return_val_if_fail (key != NULL, FALSE);
    return prefs_get_item (key) != NULL;
}


const GValue *
moo_prefs_get (const char *key)
{
    const GValue *val;
    PrefsItem *item;

    g_return_val_if_fail (key != NULL, NULL);

    item = prefs_get_item (key);
    val = item ? &item->value : NULL;

    if (!item)
        g_warning ("key '%s' not registered", key);

    return val;
}


const GValue *
moo_prefs_get_default (const char *key)
{
    PrefsItem *item;

    g_return_val_if_fail (key != NULL, NULL);

    item = prefs_get_item (key);
    g_return_val_if_fail (item != NULL, NULL);

    return &item->default_value;
}


void
moo_prefs_set (const char     *key,
               const GValue   *value)
{
    PrefsItem *item;

    g_return_if_fail (key != NULL);
    g_return_if_fail (G_IS_VALUE (value));

    item = prefs_get_item (key);

    if (!item)
    {
        g_warning ("key '%s' not registered", key);
        return;
    }

    if (item_set (item, value))
        if (item->prefs_kind == MOO_PREFS_RC)
            moo_prefs_set_modified (TRUE);
}


void
moo_prefs_set_default (const char     *key,
                       const GValue   *value)
{
    PrefsItem *item;

    g_return_if_fail (key != NULL);
    g_return_if_fail (G_IS_VALUE (value));

    item = prefs_get_item (key);
    g_return_if_fail (item != NULL);

    item_set_default (item, value);
}


static void
moo_prefs_set_modified (gboolean modified)
{
    PrefsStore *prefs = prefs_instance ();
    prefs->rc_modified = modified;
}


static void
prepend_key (const char *key,
             PrefsItem  *item,
             gpointer    pdata)
{
    struct {
        GSList *list;
        MooPrefsKind prefs_kind;
    } *data = pdata;

    if (data->prefs_kind == item->prefs_kind)
        data->list = g_slist_prepend (data->list, g_strdup (key));
}

GSList *
moo_prefs_list_keys (MooPrefsKind prefs_kind)
{
    PrefsStore *prefs = prefs_instance ();

    struct {
        GSList *list;
        MooPrefsKind prefs_kind;
    } data;

    data.list = NULL;
    data.prefs_kind = prefs_kind;
    g_hash_table_foreach (prefs->data, (GHFunc) prepend_key, &data);

    return g_slist_sort (data.list, (GCompareFunc) strcmp);
}


/***************************************************************************/
/* Prefs
 */


void
moo_prefs_new_key (const char   *key,
                   GType         value_type,
                   const GValue *default_value,
                   MooPrefsKind  prefs_kind)
{
    PrefsItem *item;
    PrefsStore *prefs;

    g_return_if_fail (key && key[0]);
    g_return_if_fail (g_utf8_validate (key, -1, NULL));
    g_return_if_fail (G_IS_VALUE (default_value));
    g_return_if_fail (G_VALUE_TYPE (default_value) == value_type);

    switch (value_type)
    {
        case G_TYPE_STRING:
        case G_TYPE_BOOLEAN:
        case G_TYPE_INT:
            break;
        default:
            g_warning ("bad type '%s'", g_type_name (value_type));
            return;
    }

    prefs = prefs_instance ();
    item = prefs_get_item (key);

    if (!item)
    {
        item = item_new (value_type, default_value, default_value, prefs_kind);
        g_hash_table_insert (prefs->data, g_strdup (key), item);
    }
    else
    {
        item_set_type (item, value_type);

        if (!item->overridden)
            item_set_default (item, default_value);

        if (!item->overridden &&
            item->prefs_kind == MOO_PREFS_RC &&
            item->prefs_kind != prefs_kind)
                moo_prefs_set_modified (TRUE);

        item->prefs_kind = prefs_kind;
    }
}


static void
prefs_new_key_from_string (const char   *key,
                           const char   *type,
                           const char   *value,
                           int           prefs_kind)
{
    PrefsItem *item;
    GValue string_val = { 0 };
    GValue real_val = { 0 };
    GType value_type;

    g_return_if_fail (key && key[0]);
    g_return_if_fail (type && type[0]);
    g_return_if_fail (g_utf8_validate (key, -1, NULL));

    if (strcmp (type, "bool") == 0)
        value_type = G_TYPE_BOOLEAN;
    else if (strcmp (type, "int") == 0)
        value_type = G_TYPE_INT;
    else if (strcmp (type, "string") == 0)
        value_type = G_TYPE_STRING;
    else
    {
        g_critical ("unknown value type '%s'", type);
        return;
    }

    g_value_init (&string_val, G_TYPE_STRING);
    g_value_set_string (&string_val, value);
    g_value_init (&real_val, value_type);
    _moo_value_convert (&string_val, &real_val);

    item = prefs_get_item (key);

    if (!item)
    {
        GValue default_val = { 0 };
        g_value_init (&default_val, value_type);
        if (prefs_kind == MOO_PREFS_SYS)
            g_value_copy (&real_val, &default_val);
        moo_prefs_new_key (key, value_type, &default_val,
                           prefs_kind == MOO_PREFS_SYS ? MOO_PREFS_RC : prefs_kind);
        item = prefs_get_item (key);
        g_value_unset (&default_val);
    }

    item_set (item, &real_val);

    if (prefs_kind == MOO_PREFS_SYS)
        item->overridden = TRUE;

    g_value_unset (&real_val);
    g_value_unset (&string_val);
}


void
moo_prefs_delete_key (const char *key)
{
    PrefsItem *item;
    PrefsStore *prefs;

    g_return_if_fail (key != NULL);

    item = prefs_get_item (key);

    if (!item)
        return;

    if (item->prefs_kind == MOO_PREFS_RC)
        moo_prefs_set_modified (TRUE);

    prefs = prefs_instance ();
    g_hash_table_remove (prefs->data, key);
}



static PrefsItem*
prefs_get_item (const char *key)
{
    PrefsStore *prefs = prefs_instance ();
    g_return_val_if_fail (key != NULL, NULL);
    return g_hash_table_lookup (prefs->data, key);
}


/***************************************************************************/
/* PrefsItem
 */

static PrefsItem*
item_new (GType           type,
          const GValue   *value,
          const GValue   *default_value,
          MooPrefsKind    prefs_kind)
{
    PrefsItem *item;

    g_return_val_if_fail (_moo_value_type_supported (type), NULL);
    g_return_val_if_fail (value && default_value, NULL);
    g_return_val_if_fail (G_VALUE_TYPE (value) == type, NULL);
    g_return_val_if_fail (G_VALUE_TYPE (default_value) == type, NULL);

    item = g_new0 (PrefsItem, 1);

    item->type = type;
    item->prefs_kind = prefs_kind;

    g_value_init (&item->value, type);
    g_value_copy (value, &item->value);
    g_value_init (&item->default_value, type);
    g_value_copy (default_value, &item->default_value);

    return item;
}


static gboolean
item_set_type (PrefsItem      *item,
               GType           type)
{
    g_return_val_if_fail (item != NULL, FALSE);
    g_return_val_if_fail (_moo_value_type_supported (type), FALSE);

    if (type != item->type)
    {
        g_critical ("oops");
        item->type = type;
        _moo_value_change_type (&item->value, type);
        _moo_value_change_type (&item->default_value, type);
        return TRUE;
    }

    return FALSE;
}


static void
item_free (PrefsItem *item)
{
    if (item)
    {
        item->type = 0;
        g_value_unset (&item->value);
        g_value_unset (&item->default_value);
        g_free (item);
    }
}


static gboolean
item_set (PrefsItem      *item,
          const GValue   *value)
{
    g_return_val_if_fail (item != NULL, FALSE);
    g_return_val_if_fail (G_IS_VALUE (value), FALSE);
    g_return_val_if_fail (item->type == G_VALUE_TYPE (value), FALSE);

    if (!_moo_value_equal (value, &item->value))
    {
        g_value_copy (value, &item->value);
        return TRUE;
    }

    return FALSE;
}


static gboolean
item_set_default (PrefsItem      *item,
                  const GValue   *value)
{
    g_return_val_if_fail (item != NULL, FALSE);
    g_return_val_if_fail (G_IS_VALUE (value), FALSE);
    g_return_val_if_fail (item->type == G_VALUE_TYPE (value), FALSE);

    if (!_moo_value_equal (value, &item->default_value))
    {
        g_value_copy (value, &item->default_value);
        return TRUE;
    }

    return FALSE;
}


/***************************************************************************/
/* Loading abd saving
 */

static void
process_item (MooMarkupElement *elm,
              int               prefs_kind)
{
    const char *name;
    const char *type;

    if (strcmp (elm->name, "item") != 0)
    {
        g_critical ("bad element '%s'", elm->name);
        return;
    }

    name = moo_markup_get_prop (MOO_MARKUP_NODE (elm), "name");
    type = moo_markup_get_prop (MOO_MARKUP_NODE (elm), "type");
    g_return_if_fail (name && *name && type && *type);

    prefs_new_key_from_string (name, type, elm->content, prefs_kind);

#ifdef MOO_PREFS_DEBUG_READWRITE
    g_print ("key: '%s', type: '%s', val: '%s'\n", name, type, elm->content);
#endif
}

static gboolean
load_file (const char  *file,
           int          prefs_kind,
           GError     **error)
{
    MooMarkupDoc *xml;
    MooMarkupNode *root;
    PrefsStore *prefs;
    MooMarkupDoc **target = NULL;
    const char *version;

    prefs = prefs_instance ();

    switch (prefs_kind)
    {
        case MOO_PREFS_RC:
            target = &prefs->xml_rc;
            break;
        case MOO_PREFS_STATE:
            target = &prefs->xml_state;
            break;
        case MOO_PREFS_SYS:
            target = NULL;
            break;
        default:
            g_assert_not_reached ();
    }

    if (!g_file_test (file, G_FILE_TEST_EXISTS))
        return TRUE;

    xml = moo_markup_parse_file (file, error);

    if (!xml)
        return FALSE;

    root = moo_markup_get_root_element (xml, MOO_PREFS_ELEMENT);

    if (!root)
    {
        g_warning ("element '%s' missing in file %s", MOO_PREFS_ELEMENT, file);
        moo_markup_doc_unref (xml);
        return FALSE;
    }

    version = moo_markup_get_prop (root, PROP_VERSION);
    if (!version || strcmp (version, MOO_PREFS_VERSION) != 0)
    {
        g_warning ("invalid version '%s'in file %s", version, file);
        moo_markup_doc_unref (xml);
        return FALSE;
    }

    if (target)
    {
        if (*target)
        {
            g_warning ("implement me");
            moo_markup_doc_unref (*target);
        }

        *target = moo_markup_doc_ref (xml);
    }

    if (prefs_kind != MOO_PREFS_STATE)
    {
        _moo_markup_set_track_modified (xml, TRUE);
        _moo_markup_set_modified (xml, FALSE);
    }

    root = moo_markup_get_element (root, PREFS_ROOT);

    if (root)
    {
        MooMarkupNode *child;
        for (child = root->children; child != NULL; child = child->next)
            if (child->type == MOO_MARKUP_ELEMENT_NODE)
                process_item (MOO_MARKUP_ELEMENT (child), prefs_kind);
    }

    moo_markup_doc_unref (xml);
    return TRUE;
}


gboolean
moo_prefs_load (char          **sys_files,
                const char     *file_rc,
                const char     *file_state,
                GError        **error)
{
    moo_prefs_set_modified (FALSE);

    for (; sys_files && *sys_files; ++sys_files)
        if (!load_file (*sys_files, MOO_PREFS_SYS, error))
            return FALSE;

    if (file_rc && !load_file (file_rc, MOO_PREFS_RC, error))
        return FALSE;

    if (file_state && !load_file (file_state, MOO_PREFS_STATE, error))
        return FALSE;

    return TRUE;
}


typedef struct {
    GPtrArray *keys;
    MooPrefsKind prefs_kind;
} CollectItemsData;

static void
collect_item (const char       *key,
              PrefsItem        *item,
              CollectItemsData *data)
{
    g_return_if_fail (key && *key && item);
    g_return_if_fail (_moo_value_type_supported (item->type));

    if (item->prefs_kind != data->prefs_kind)
        return;

    if (_moo_value_equal (&item->value, &item->default_value))
    {
#ifdef MOO_PREFS_DEBUG_READWRITE
        g_print ("skipping '%s'\n", key);
#endif
        return;
    }

    g_ptr_array_add (data->keys, (gpointer) key);
}

static void
write_item (const char    *key,
            MooMarkupNode *root)
{
    const char *string = NULL;
    const char *type;
    MooMarkupNode *elm;
    PrefsItem *item;

    item = prefs_get_item (key);
    g_return_if_fail (item != NULL);

    string = _moo_value_convert_to_string (&item->value);

    if (!string)
        string = "";

    switch (item->type)
    {
        case G_TYPE_INT:
            type = "int";
            break;
        case G_TYPE_STRING:
            type = "string";
            break;
        case G_TYPE_BOOLEAN:
            type = "bool";
            break;
        default:
            g_return_if_reached ();
    }

    elm = moo_markup_create_text_element (root, "item", string);
    moo_markup_set_prop (elm, "name", key);
    moo_markup_set_prop (elm, "type", type);

#ifdef MOO_PREFS_DEBUG_READWRITE
    g_print ("writing key: '%s', type: '%s', val: '%s'\n", key, type, string);
#endif
}

static int
compare_strings (gconstpointer a, gconstpointer b)
{
    return strcmp (*(char**)a, *(char**)b);
}

static void
sync_xml (MooPrefsKind prefs_kind)
{
    PrefsStore *prefs = prefs_instance ();
    MooMarkupDoc *xml;
    MooMarkupDoc **xml_ptr = NULL;
    MooMarkupNode *root;
    CollectItemsData data;

    switch (prefs_kind)
    {
        case MOO_PREFS_RC:
            xml_ptr = &prefs->xml_rc;
            break;
        case MOO_PREFS_STATE:
            xml_ptr = &prefs->xml_state;
            break;
        default:
            g_return_if_reached ();
            break;
    }

    if (!*xml_ptr)
        *xml_ptr = create_markup_doc ();

    xml = *xml_ptr;
    root = moo_markup_get_element (MOO_MARKUP_NODE (xml), MOO_PREFS_ELEMENT "/" PREFS_ROOT);

    if (root)
        moo_markup_delete_node (root);

    data.prefs_kind = prefs_kind;
    data.keys = g_ptr_array_new ();

    g_hash_table_foreach (prefs->data,
                          (GHFunc) collect_item,
                          &data);

    if (data.keys->len > 0)
    {
        guint i;

        g_ptr_array_sort (data.keys, compare_strings);

        root = moo_markup_create_element (MOO_MARKUP_NODE (xml),
                                          MOO_PREFS_ELEMENT "/" PREFS_ROOT);

        for (i = 0; i < data.keys->len; ++i)
            write_item (data.keys->pdata[i], root);
    }

    g_ptr_array_free (data.keys, TRUE);
}


static gboolean
check_modified (MooPrefsKind prefs_kind)
{
    PrefsStore *prefs = prefs_instance ();
    MooMarkupDoc *xml = prefs->xml_rc;

    if (prefs_kind != MOO_PREFS_RC)
        return TRUE;

    if (xml && _moo_markup_get_modified (xml))
        return TRUE;
    else
        return prefs->rc_modified;
}

static gboolean
save_file (const char    *file,
           MooPrefsKind   prefs_kind,
           GError       **error)
{
    MooMarkupDoc *xml = NULL;
    MooMarkupNode *node;
    gboolean empty;
    PrefsStore *prefs = prefs_instance ();
    MooFileWriter *writer;

    if (!check_modified (prefs_kind))
        return TRUE;

    sync_xml (prefs_kind);

    switch (prefs_kind)
    {
        case MOO_PREFS_RC:
            xml = prefs->xml_rc;
            break;
        case MOO_PREFS_STATE:
            xml = prefs->xml_state;
            break;
    }

    g_return_val_if_fail (xml != NULL, FALSE);

    empty = TRUE;
    for (node = xml->children; empty && node != NULL; node = node->next)
        if (MOO_MARKUP_IS_ELEMENT (node))
            empty = FALSE;

    if (empty)
    {
        if (g_file_test (file, G_FILE_TEST_EXISTS))
        {
            mgw_errno_t err;
            if (mgw_unlink (file, &err) != 0)
                g_critical ("%s", mgw_strerror (err));
        }
        return TRUE;
    }

    if ((writer = moo_config_writer_new (file, FALSE, error)))
    {
        moo_markup_write_pretty (xml, writer, 2);
        return moo_file_writer_close (writer, error);
    }

    return FALSE;
}


gboolean
moo_prefs_save (const char  *file_rc,
                const char  *file_state,
                GError     **error)
{
    PrefsStore *prefs = prefs_instance ();

    g_return_val_if_fail (file_rc != NULL, FALSE);
    g_return_val_if_fail (file_state != NULL, FALSE);

    if (save_file (file_rc, MOO_PREFS_RC, error))
    {
        if (prefs->xml_rc)
            _moo_markup_set_modified (prefs->xml_rc, FALSE);
        moo_prefs_set_modified (FALSE);
    }
    else
    {
        return FALSE;
    }

    return save_file (file_state, MOO_PREFS_STATE, error);
}


/***************************************************************************/
/* Helpers
 */

/**
 * moo_prefs_new_key_bool:
 *
 * @key: (type const-utf8)
 * @default_val: (default FALSE):
 */
void
moo_prefs_new_key_bool (const char *key,
                        gboolean    default_val)
{
    GValue val;

    g_return_if_fail (key != NULL);

    val.g_type = 0;
    g_value_init (&val, G_TYPE_BOOLEAN);
    g_value_set_boolean (&val, default_val);

    moo_prefs_new_key (key, G_TYPE_BOOLEAN, &val, MOO_PREFS_RC);
}


/**
 * moo_prefs_new_key_int:
 *
 * @key: (type const-utf8)
 * @default_val: (default 0):
 */
void
moo_prefs_new_key_int (const char *key,
                       int         default_val)
{
    GValue val;

    g_return_if_fail (key != NULL);

    val.g_type = 0;
    g_value_init (&val, G_TYPE_INT);
    g_value_set_int (&val, default_val);

    moo_prefs_new_key (key, G_TYPE_INT, &val, MOO_PREFS_RC);
}


/**
 * moo_prefs_new_key_string:
 *
 * @key: (type const-utf8)
 * @default_val: (type const-cstring) (allow-none) (default NULL):
 **/
void
moo_prefs_new_key_string (const char *key,
                          const char *default_val)
{
    static GValue val;

    g_return_if_fail (key != NULL);

    val.g_type = 0;
    g_value_init (&val, G_TYPE_STRING);

    g_value_set_static_string (&val, default_val);
    moo_prefs_new_key (key, G_TYPE_STRING, &val, MOO_PREFS_RC);

    g_value_unset (&val);
}


/**
 * moo_prefs_get_string:
 *
 * @key: (type const-utf8)
 *
 * Returns: (type const-cstring)
 */
const char *
moo_prefs_get_string (const char *key)
{
    const GValue *val;

    val = moo_prefs_get (key);
    g_return_val_if_fail (val != NULL, NULL);

    return g_value_get_string (val);
}


/**
 * moo_prefs_get_filename:
 *
 * @key: (type const-utf8)
 *
 * Returns: (type const-filename)
 */
const char *
moo_prefs_get_filename (const char *key)
{
    const char *utf8_val;
    static char *val = NULL;
    GError *error = NULL;

    utf8_val = moo_prefs_get_string (key);

    if (!utf8_val)
        return NULL;

    g_free (val);
    val = g_filename_from_utf8 (utf8_val, -1, NULL, NULL, &error);

    if (!val)
    {
        g_warning ("could not convert '%s' to filename encoding: %s",
                   utf8_val, moo_error_message (error));
        g_error_free (error);
    }

    return val;
}

/**
 * moo_prefs_get_file:
 *
 * @key: (type const-utf8)
 *
 * Returns: (transfer full):
 */
GFile *
moo_prefs_get_file (const char *key)
{
    const char *uri = moo_prefs_get_string (key);
    return uri ? g_file_new_for_uri (uri) : NULL;
}


/**
 * moo_prefs_get_bool:
 *
 * @key: (type const-utf8)
 */
gboolean
moo_prefs_get_bool (const char *key)
{
    const GValue *value;

    g_return_val_if_fail (key != NULL, FALSE);

    value = moo_prefs_get (key);

    g_return_val_if_fail (value != NULL, FALSE);
    g_return_val_if_fail (G_VALUE_TYPE (value) == G_TYPE_BOOLEAN, FALSE);

    return g_value_get_boolean (value);
}


/**
 * moo_prefs_get_int:
 *
 * @key: (type const-utf8)
 */
int
moo_prefs_get_int (const char *key)
{
    const GValue *value;

    g_return_val_if_fail (key != NULL, 0);

    value = moo_prefs_get (key);

    g_return_val_if_fail (value != NULL, 0);
    g_return_val_if_fail (G_VALUE_TYPE (value) == G_TYPE_INT, 0);

    return g_value_get_int (value);
}


/**
 * moo_prefs_set_string:
 *
 * @key: (type const-utf8)
 * @val: (type const-cstring) (allow-none)
 */
void
moo_prefs_set_string (const char     *key,
                      const char     *val)
{
    GValue gval;

    g_return_if_fail (key != NULL);

    gval.g_type = 0;
    g_value_init (&gval, G_TYPE_STRING);
    g_value_set_static_string (&gval, val);
    moo_prefs_set (key, &gval);
    g_value_unset (&gval);
}


/**
 * moo_prefs_set_filename:
 *
 * @key: (type const-utf8)
 * @val: (type const-filename) (allow-none)
 */
void
moo_prefs_set_filename (const char     *key,
                        const char     *val)
{
    char *utf8_val;

    g_return_if_fail (key != NULL);

    if (!val)
    {
        moo_prefs_set_string (key, NULL);
        return;
    }

    utf8_val = g_filename_display_name (val);

    if (!utf8_val)
    {
        g_warning ("could not convert '%s' to utf8", val);
        return;
    }

    moo_prefs_set_string (key, utf8_val);
    g_free (utf8_val);
}

/**
 * moo_prefs_set_file:
 *
 * @key: (type const-utf8)
 * @val: (allow-none)
 */
void
moo_prefs_set_file (const char *key,
                    GFile      *val)
{
    char *uri;

    g_return_if_fail (key != NULL);

    if (!val)
    {
        moo_prefs_set_string (key, NULL);
        return;
    }

    uri = g_file_get_uri (val);
    moo_prefs_set_string (key, uri);
    g_free (uri);
}


/**
 * moo_prefs_set_int:
 *
 * @key: (type const-utf8)
 * @val:
 */
void
moo_prefs_set_int (const char *key,
                   int         val)
{
    GValue gval = { 0 };

    g_return_if_fail (key != NULL);

    g_value_init (&gval, G_TYPE_INT);
    g_value_set_int (&gval, val);
    moo_prefs_set (key, &gval);
    g_value_unset (&gval);
}


/**
 * moo_prefs_set_bool:
 *
 * @key: (type const-utf8)
 * @val:
 */
void
moo_prefs_set_bool (const char *key,
                    gboolean    val)
{
    GValue gval = { 0 };

    g_return_if_fail (key != NULL);

    g_value_init (&gval, G_TYPE_BOOLEAN);
    g_value_set_boolean (&gval, val);
    moo_prefs_set (key, &gval);
    g_value_unset (&gval);
}
