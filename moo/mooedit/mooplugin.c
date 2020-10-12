/*
 *   mooplugin.c
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
 * class:MooPlugin: (parent GObject) (moo.lua 0) (moo.private 1)
 **/

/**
 * class:MooWinPlugin: (parent GObject) (moo.lua 0) (moo.private 1)
 **/

/**
 * class:MooDocPlugin: (parent GObject) (moo.lua 0) (moo.private 1)
 **/

/**
 * boxed:MooPluginInfo: (moo.lua 0) (moo.private 1)
 **/

/**
 * boxed:MooPluginParams: (moo.lua 0) (moo.private 1)
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mooedit/mooplugin.h"
#include "mooedit/mooplugin-loader.h"
#include "mooedit/moopluginprefs-gxml.h"
#include "mooutils/mooprefsdialog.h"
#include "mooutils/moostock.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-debug.h"
#include "mooutils/moohelp.h"
#include "mooutils/mootype-macros.h"
#ifdef MOO_ENABLE_HELP
#include "moo-help-sections.h"
#endif
#include <string.h>
#include <gmodule.h>
#include <gobject/gvaluecollector.h>

#ifdef __WIN32__
#include <windows.h>
#endif

#define PLUGIN_PREFS_ENABLED "enabled"


typedef struct {
    MooEditor *editor;
    GSList *list; /* MooPlugin* */
    GHashTable *names;
    char **dirs;
    gboolean dirs_read;
    GQuark plugin_quark;
    GQuark meths_quark;
} PluginStore;

static PluginStore *plugin_store = NULL;
#define MOO_PLUGIN_QUARK plugin_store->plugin_quark
#define MOO_PLUGIN_METHS_QUARK plugin_store->meths_quark

MOO_DEFINE_BOXED_TYPE_C (MooPluginInfo, moo_plugin_info)
MOO_DEFINE_BOXED_TYPE_C (MooPluginParams, moo_plugin_params)

static void     plugin_store_init       (void);
static void     plugin_store_add        (MooPlugin      *plugin);
static void     plugin_type_cleanup     (GType           type);

static void     moo_plugin_class_init   (MooPluginClass *klass);
static void     some_plugin_class_init  (gpointer        klass);

static gboolean plugin_init             (MooPlugin      *plugin);
static void     plugin_deinit           (MooPlugin      *plugin);
static void     plugin_attach_win       (MooPlugin      *plugin,
                                         MooEditWindow  *window);
static void     plugin_detach_win       (MooPlugin      *plugin,
                                         MooEditWindow  *window);
static void     plugin_attach_doc       (MooPlugin      *plugin,
                                         MooEditWindow  *window,
                                         MooEdit        *doc);
static void     plugin_detach_doc       (MooPlugin      *plugin,
                                         MooEditWindow  *window,
                                         MooEdit        *doc);

static gboolean plugin_info_check       (const MooPluginInfo *info);

static char    *make_prefs_key          (MooPlugin      *plugin,
                                         const char     *key);
static GQuark   make_id_quark           (MooPlugin      *plugin);

static MooWinPlugin *window_get_plugin  (MooEditWindow  *window,
                                         MooPlugin      *plugin);
static MooDocPlugin *doc_get_plugin     (MooEdit        *doc,
                                         MooPlugin      *plugin);
static void     window_set_plugin       (MooEditWindow  *window,
                                         MooPlugin      *plugin,
                                         MooWinPlugin   *win_plugin);
static void     doc_set_plugin          (MooEdit        *doc,
                                         MooPlugin      *plugin,
                                         MooDocPlugin   *doc_plugin);

static gboolean moo_plugin_registered   (GType           type);


static gpointer parent_class = NULL;


GType
moo_plugin_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        static const GTypeInfo info = {
            sizeof (MooPluginClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) moo_plugin_class_init,
            (GClassFinalizeFunc) NULL,
            NULL,   /* class_data */
            sizeof (MooPlugin),
            0,      /* n_preallocs */
            NULL,
            NULL    /* value_table */
        };

        type = g_type_register_static (G_TYPE_OBJECT, "MooPlugin", &info, 0);
    }

    return type;
}


GType
moo_win_plugin_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        static const GTypeInfo info = {
            sizeof (MooWinPluginClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) some_plugin_class_init,
            (GClassFinalizeFunc) NULL,
            NULL,   /* class_data */
            sizeof (MooWinPlugin),
            0,      /* n_preallocs */
            NULL,
            NULL    /* value_table */
        };

        type = g_type_register_static (G_TYPE_OBJECT, "MooWinPlugin", &info, 0);
    }

    return type;
}


GType
moo_doc_plugin_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (type == 0))
    {
        static const GTypeInfo info = {
            sizeof (MooDocPluginClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) some_plugin_class_init,
            (GClassFinalizeFunc) NULL,
            NULL,   /* class_data */
            sizeof (MooDocPlugin),
            0,      /* n_preallocs */
            NULL,
            NULL    /* value_table */
        };

        type = g_type_register_static (G_TYPE_OBJECT, "MooDocPlugin", &info, 0);
    }

    return type;
}


static void
some_plugin_class_init (gpointer klass)
{
    parent_class = g_type_class_peek_parent (klass);
}


static void
moo_plugin_finalize (GObject *object)
{
    MooPlugin *plugin = MOO_PLUGIN (object);

    g_free (plugin->id);
    moo_plugin_info_free (plugin->info);
    moo_plugin_params_free (plugin->params);

    G_OBJECT_CLASS(parent_class)->finalize (object);
}


static void
moo_plugin_class_init (MooPluginClass *klass)
{
    some_plugin_class_init (klass);
    G_OBJECT_CLASS(klass)->finalize = moo_plugin_finalize;
}


/**
 * moo_plugin_register: (moo.lua 0) (moo.private 1)
 *
 * @id: (type const-utf8)
 * @type:
 * @info:
 * @params: (allow-none) (default NULL)
 **/
gboolean
moo_plugin_register (const char            *id,
                     GType                  type,
                     const MooPluginInfo   *info,
                     const MooPluginParams *params)
{
    MooPluginClass *klass;
    MooPlugin *plugin;
    char *prefs_key;
    MooEditWindowArray *windows;
    guint i;

    g_return_val_if_fail (id != NULL && id[0] != 0, FALSE);
    g_return_val_if_fail (g_utf8_validate (id, -1, NULL), FALSE);

    g_return_val_if_fail (g_type_is_a (type, MOO_TYPE_PLUGIN), FALSE);

    klass = g_type_class_ref (type);
    g_return_val_if_fail (klass != NULL, FALSE);

    if (moo_plugin_registered (type))
    {
        g_warning ("plugin '%s' already registered",
                   g_type_name (type));
        return FALSE;
    }

    if (!plugin_info_check (info))
    {
        g_warning ("invalid info in plugin '%s'",
                   g_type_name (type));
        return FALSE;
    }

    plugin = MOO_PLUGIN (g_object_new (type, (const char*) NULL));
    plugin->id = g_strdup (id);
    plugin->info = moo_plugin_info_copy ((MooPluginInfo*) info);
    plugin->params = params ? moo_plugin_params_copy ((MooPluginParams*) params) :
                              moo_plugin_params_new (TRUE, TRUE);

    if (moo_plugin_lookup (moo_plugin_id (plugin)))
    {
        _moo_message ("plugin with id %s already registered",
                      moo_plugin_id (plugin));
        g_object_unref (plugin);
        return FALSE;
    }

    plugin->id_quark = make_id_quark (plugin);

    prefs_key = make_prefs_key (plugin, PLUGIN_PREFS_ENABLED);
    moo_prefs_new_key_bool (prefs_key, moo_plugin_enabled (plugin));
    plugin->params->enabled = moo_prefs_get_bool (prefs_key);
    g_free (prefs_key);

    if (!plugin_init (plugin))
    {
        g_object_unref (plugin);
        return FALSE;
    }

    plugin_store_add (plugin);

    windows = moo_editor_get_windows (plugin_store->editor);
    for (i = 0; i < windows->n_elms; ++i)
        plugin_attach_win (plugin, windows->elms[i]);
    moo_edit_window_array_free (windows);

    return TRUE;
}


static gboolean
plugin_init (MooPlugin *plugin)
{
    MooPluginClass *klass;

    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), FALSE);

    if (!moo_plugin_enabled (plugin) || plugin->initialized)
        return TRUE;

    klass = MOO_PLUGIN_GET_CLASS (plugin);

    if (!klass->init || klass->init (plugin))
    {
        plugin->initialized = TRUE;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


static void
plugin_deinit (MooPlugin *plugin)
{
    MooPluginClass *klass;

    g_return_if_fail (MOO_IS_PLUGIN (plugin));

    if (!plugin->initialized)
        return;

    klass = MOO_PLUGIN_GET_CLASS (plugin);

    if (klass->deinit)
        klass->deinit (plugin);

    plugin->initialized = FALSE;
}


static void
plugin_attach_win (MooPlugin      *plugin,
                   MooEditWindow  *window)
{
    MooPluginClass *klass;
    MooWinPluginClass *wklass;
    MooWinPlugin *win_plugin;
    GType wtype;
    MooEditArray *docs;
    guint i;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_PLUGIN (plugin));

    if (!moo_plugin_enabled (plugin))
        return;

    klass = MOO_PLUGIN_GET_CLASS (plugin);

    if (klass->attach_win)
        klass->attach_win (plugin, window);

    wtype = plugin->win_plugin_type;

    if (wtype && g_type_is_a (wtype, MOO_TYPE_WIN_PLUGIN))
    {
        win_plugin = MOO_WIN_PLUGIN (g_object_new (wtype, (const char*) NULL));
        g_return_if_fail (win_plugin != NULL);

        win_plugin->plugin = plugin;
        win_plugin->window = window;
        wklass = MOO_WIN_PLUGIN_GET_CLASS (win_plugin);

        if (!wklass->create || wklass->create (win_plugin))
            window_set_plugin (window, plugin, win_plugin);
        else
            g_object_unref (win_plugin);
    }

    docs = moo_edit_window_get_docs (window);
    for (i = 0; i < docs->n_elms; ++i)
        plugin_attach_doc (plugin, window, docs->elms[i]);
    moo_edit_array_free (docs);
}


static void
plugin_detach_win (MooPlugin      *plugin,
                   MooEditWindow  *window)
{
    MooPluginClass *klass;
    MooWinPluginClass *wklass;
    MooWinPlugin *win_plugin;
    MooEditArray *docs;
    guint i;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_PLUGIN (plugin));

    if (!moo_plugin_enabled (plugin))
        return;

    docs = moo_edit_window_get_docs (window);
    for (i = 0; i < docs->n_elms; ++i)
        plugin_detach_doc (plugin, window, docs->elms[i]);
    moo_edit_array_free (docs);

    win_plugin = window_get_plugin (window, plugin);

    if (win_plugin)
    {
        wklass = MOO_WIN_PLUGIN_GET_CLASS (win_plugin);

        if (wklass->destroy)
            wklass->destroy (win_plugin);

        g_object_unref (win_plugin);
        window_set_plugin (window, plugin, NULL);
    }

    klass = MOO_PLUGIN_GET_CLASS (plugin);

    if (klass->detach_win)
        klass->detach_win (plugin, window);
}


static void
plugin_attach_doc (MooPlugin      *plugin,
                   MooEditWindow  *window,
                   MooEdit        *doc)
{
    MooPluginClass *klass;
    MooDocPluginClass *dklass;
    MooDocPlugin *doc_plugin;
    GType dtype;

    g_return_if_fail (!window || MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (MOO_IS_PLUGIN (plugin));

    if (!moo_plugin_enabled (plugin))
        return;

    plugin->docs = g_slist_prepend (plugin->docs, doc);

    klass = MOO_PLUGIN_GET_CLASS (plugin);

    if (klass->attach_doc)
        klass->attach_doc (plugin, doc, window);

    dtype = plugin->doc_plugin_type;

    if (dtype && g_type_is_a (dtype, MOO_TYPE_DOC_PLUGIN))
    {
        doc_plugin = MOO_DOC_PLUGIN (g_object_new (dtype, (const char*) NULL));
        g_return_if_fail (doc_plugin != NULL);

        doc_plugin->plugin = plugin;
        doc_plugin->window = window;
        doc_plugin->doc = doc;
        dklass = MOO_DOC_PLUGIN_GET_CLASS (doc_plugin);

        if (!dklass->create || dklass->create (doc_plugin))
            doc_set_plugin (doc, plugin, doc_plugin);
        else
            g_object_unref (doc_plugin);
    }
}


static void
plugin_detach_doc (MooPlugin      *plugin,
                   MooEditWindow  *window,
                   MooEdit        *doc)
{
    MooPluginClass *klass;
    MooDocPluginClass *dklass;
    MooDocPlugin *doc_plugin;

    g_return_if_fail (MOO_IS_PLUGIN (plugin));
    g_return_if_fail (!window || MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT (doc));

    if (!moo_plugin_enabled (plugin))
        return;

    if (!g_slist_find (plugin->docs, doc))
        return;

    plugin->docs = g_slist_remove (plugin->docs, doc);

    doc_plugin = doc_get_plugin (doc, plugin);

    if (doc_plugin)
    {
        dklass = MOO_DOC_PLUGIN_GET_CLASS (doc_plugin);

        if (dklass->destroy)
            dklass->destroy (doc_plugin);

        g_object_unref (doc_plugin);
        doc_set_plugin (doc, plugin, NULL);
    }

    klass = MOO_PLUGIN_GET_CLASS (plugin);

    if (klass->detach_doc)
        klass->detach_doc (plugin, doc, window);
}


static MooWinPlugin*
window_get_plugin (MooEditWindow  *window,
                   MooPlugin      *plugin)
{
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), NULL);
    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), NULL);
    return g_object_get_qdata (G_OBJECT (window), plugin->id_quark);
}


static MooDocPlugin*
doc_get_plugin (MooEdit        *doc,
                MooPlugin      *plugin)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), NULL);
    return g_object_get_qdata (G_OBJECT (doc), plugin->id_quark);
}


static void
window_set_plugin (MooEditWindow  *window,
                   MooPlugin      *plugin,
                   MooWinPlugin   *win_plugin)
{
    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_PLUGIN (plugin));
    g_return_if_fail (!win_plugin || MOO_IS_WIN_PLUGIN (win_plugin));
    g_object_set_qdata (G_OBJECT (window), plugin->id_quark, win_plugin);
}


static void
doc_set_plugin (MooEdit        *doc,
                MooPlugin      *plugin,
                MooDocPlugin   *doc_plugin)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (MOO_IS_PLUGIN (plugin));
    g_return_if_fail (!doc_plugin || MOO_IS_DOC_PLUGIN (doc_plugin));
    g_object_set_qdata (G_OBJECT (doc), plugin->id_quark, doc_plugin);
}


static void
plugin_store_init (void)
{
    if (G_UNLIKELY (!plugin_store))
    {
        static PluginStore store;

        store.editor = moo_editor_instance ();
        store.list = NULL;
        store.names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
        store.plugin_quark = g_quark_from_static_string ("moo-plugin");
        store.meths_quark = g_quark_from_static_string ("moo-plugin-methods");

        plugin_store = &store;
    }
}


static void
plugin_store_add (MooPlugin *plugin)
{
    g_return_if_fail (MOO_IS_PLUGIN (plugin));

    plugin_store_init ();

    g_return_if_fail (!g_hash_table_lookup (plugin_store->names, moo_plugin_id (plugin)));

    plugin_store->list = g_slist_append (plugin_store->list, plugin);
    g_hash_table_insert (plugin_store->names,
                         g_strdup (moo_plugin_id (plugin)),
                         plugin);

    g_type_set_qdata (G_OBJECT_TYPE (plugin), MOO_PLUGIN_QUARK, plugin);
}


static void
plugin_store_remove (MooPlugin *plugin)
{
    g_return_if_fail (plugin_store != NULL);
    g_return_if_fail (MOO_IS_PLUGIN (plugin));
    plugin_store->list = g_slist_remove (plugin_store->list, plugin);
    g_hash_table_remove (plugin_store->names, moo_plugin_id (plugin));
    g_type_set_qdata (G_OBJECT_TYPE (plugin), MOO_PLUGIN_QUARK, NULL);
    plugin_type_cleanup (G_OBJECT_TYPE (plugin));
}


static MooPlugin *
moo_plugin_get (GType type)
{
    g_return_val_if_fail (g_type_is_a (type, MOO_TYPE_PLUGIN), NULL);
    plugin_store_init ();
    return g_type_get_qdata (type, MOO_PLUGIN_QUARK);
}


static gboolean
moo_plugin_registered (GType type)
{
    return moo_plugin_get (type) != NULL;
}


gpointer
moo_win_plugin_lookup (const char     *plugin_id,
                       MooEditWindow  *window)
{
    MooPlugin *plugin;

    g_return_val_if_fail (plugin_id != NULL, NULL);
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), NULL);

    plugin = moo_plugin_lookup (plugin_id);
    return plugin ? window_get_plugin (window, plugin) : NULL;
}


gpointer
moo_doc_plugin_lookup (const char     *plugin_id,
                       MooEdit        *doc)
{
    MooPlugin *plugin;

    g_return_val_if_fail (plugin_id != NULL, NULL);
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);

    plugin = moo_plugin_lookup (plugin_id);
    return plugin ? doc_get_plugin (doc, plugin) : NULL;
}


/**
 * moo_win_plugin_get_window:
 **/
MooEditWindow *
moo_win_plugin_get_window (MooWinPlugin *wplugin)
{
    g_return_val_if_fail (MOO_IS_WIN_PLUGIN (wplugin), NULL);
    return wplugin->window;
}

/**
 * moo_win_plugin_get_plugin:
 **/
MooPlugin *
moo_win_plugin_get_plugin (MooWinPlugin *wplugin)
{
    g_return_val_if_fail (MOO_IS_WIN_PLUGIN (wplugin), NULL);
    return wplugin->plugin;
}

/**
 * moo_doc_plugin_get_window:
 **/
MooEditWindow *
moo_doc_plugin_get_window (MooDocPlugin *dplugin)
{
    g_return_val_if_fail (MOO_IS_DOC_PLUGIN (dplugin), NULL);
    return dplugin->window;
}

/**
 * moo_doc_plugin_get_doc:
 **/
MooEdit *
moo_doc_plugin_get_doc (MooDocPlugin *dplugin)
{
    g_return_val_if_fail (MOO_IS_DOC_PLUGIN (dplugin), NULL);
    return dplugin->doc;
}

/**
 * moo_doc_plugin_get_plugin:
 **/
MooPlugin *
moo_doc_plugin_get_plugin (MooDocPlugin *dplugin)
{
    g_return_val_if_fail (MOO_IS_DOC_PLUGIN (dplugin), NULL);
    return dplugin->plugin;
}


static gboolean
plugin_info_check (const MooPluginInfo *info)
{
    return info &&
            info->name && g_utf8_validate (info->name, -1, NULL) &&
            info->description && g_utf8_validate (info->description, -1, NULL);
}


static char*
make_prefs_key (MooPlugin      *plugin,
                const char     *key)
{
    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), NULL);
    g_return_val_if_fail (key != NULL, NULL);

    return moo_prefs_make_key (MOO_PLUGIN_PREFS_ROOT,
                               moo_plugin_id (plugin),
                               PLUGIN_PREFS_ENABLED,
                               NULL);
}


static GQuark
make_id_quark (MooPlugin      *plugin)
{
    char *string;
    GQuark quark;

    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), 0);

    string = g_strdup_printf ("MooPlugin::%s", moo_plugin_id (plugin));
    quark = g_quark_from_string (string);

    g_free (string);
    return quark;
}


gboolean
moo_plugin_initialized (MooPlugin *plugin)
{
    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), FALSE);
    return plugin->initialized;
}


gboolean
moo_plugin_enabled (MooPlugin *plugin)
{
    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), FALSE);
    return plugin->params->enabled;
}


static gboolean
plugin_enable (MooPlugin  *plugin)
{
    MooEditWindowArray *windows;
    guint i;

    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), FALSE);

    if (moo_plugin_enabled (plugin))
        return TRUE;

    g_assert (!plugin->initialized);

    plugin->params->enabled = TRUE;

    if (!plugin_init (plugin))
    {
        plugin->params->enabled = FALSE;
        return FALSE;
    }

    windows = moo_editor_get_windows (plugin_store->editor);
    for (i = 0; i < windows->n_elms; ++i)
        plugin_attach_win (plugin, windows->elms[i]);
    moo_edit_window_array_free (windows);

    return TRUE;
}


static void
plugin_disable (MooPlugin  *plugin)
{
    MooEditWindowArray *windows;
    guint i;

    g_return_if_fail (MOO_IS_PLUGIN (plugin));

    if (!moo_plugin_enabled (plugin))
        return;

    g_assert (plugin->initialized);

    windows = moo_editor_get_windows (plugin_store->editor);
    for (i = 0; i < windows->n_elms; ++i)
        plugin_detach_win (plugin, windows->elms[i]);
    moo_edit_window_array_free (windows);

    plugin_deinit (plugin);
    plugin->params->enabled = FALSE;
}


gboolean
moo_plugin_set_enabled (MooPlugin  *plugin,
                        gboolean    enabled)
{
    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), FALSE);

    if (enabled)
    {
        return plugin_enable (plugin);
    }
    else
    {
        plugin_disable (plugin);
        return TRUE;
    }
}


static void
moo_plugin_unregister (GType type)
{
    MooPlugin *plugin = moo_plugin_get (type);
    g_return_if_fail (plugin != NULL);
    moo_plugin_set_enabled (plugin, FALSE);
    plugin_store_remove (plugin);
    g_object_unref (plugin);
}


void
_moo_module_version (guint *major,
                     guint *minor)
{
    if (major)
        *major = MOO_MODULE_MAJOR_VERSION;
    if (minor)
        *minor = MOO_MODULE_MINOR_VERSION;
}

gboolean
moo_module_check_version (guint major,
                          guint minor)
{
    return major == MOO_MODULE_MAJOR_VERSION &&
            minor <= MOO_MODULE_MINOR_VERSION;
}


gpointer
moo_plugin_lookup (const char *plugin_id)
{
    g_return_val_if_fail (plugin_id != NULL, NULL);
    plugin_store_init ();
    return g_hash_table_lookup (plugin_store->names, plugin_id);
}


GSList*
moo_list_plugins (void)
{
    plugin_store_init ();
    return g_slist_copy (plugin_store->list);
}


#define DEFINE_GETTER(what)                                 \
const char*                                                 \
moo_plugin_##what (MooPlugin *plugin)                       \
{                                                           \
    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), NULL);    \
    return plugin->info->what;                              \
}

DEFINE_GETTER(name)
DEFINE_GETTER(description)
DEFINE_GETTER(version)
DEFINE_GETTER(author)

#undef DEFINE_GETTER

const char *
moo_plugin_id (MooPlugin *plugin)
{
    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), NULL);
    return plugin->id;
}


static void
moo_plugin_read_dir (const char *path)
{
    GDir *dir;
    const char *name;

    g_return_if_fail (path != NULL);

    dir = g_dir_open (path, 0, NULL);

    if (!dir)
        return;

    while ((name = g_dir_read_name (dir)))
    {
        if (g_str_has_suffix (name, ".ini"))
        {
            char *tmp = g_strdup (name);
            _moo_plugin_load (path, tmp);
            g_free (tmp);
        }
    }

    g_dir_close (dir);
}


char **
moo_plugin_get_dirs (void)
{
    plugin_store_init ();
    return g_strdupv (plugin_store->dirs);
}


void
moo_plugin_read_dirs (void)
{
    char **d, **dirs;

    plugin_store_init ();

    if (plugin_store->dirs_read)
        return;

    plugin_store->dirs_read = TRUE;

    dirs = moo_get_data_and_lib_subdirs (MOO_PLUGIN_DIR_BASENAME);
    g_strfreev (plugin_store->dirs);
    plugin_store->dirs = _moo_strv_reverse (dirs);

    for (d = plugin_store->dirs; d && *d; ++d)
        moo_plugin_read_dir (*d);

    _moo_plugin_finish_load ();
}


void
moo_plugin_shutdown (void)
{
    GSList *list;

    if (!plugin_store || !plugin_store->dirs_read)
        return;

    list = g_slist_copy (plugin_store->list);
    g_slist_foreach (list, (GFunc) g_object_ref, NULL);

    while (list)
    {
        moo_plugin_unregister (G_OBJECT_TYPE (list->data));
        g_object_unref (list->data);
        list = g_slist_delete_link (list, list);
    }

    plugin_store->dirs_read = FALSE;
    plugin_store->editor = NULL;

    if (plugin_store->list)
    {
        g_critical ("could not unregister all plugins");
        g_slist_free (plugin_store->list);
        plugin_store->list = NULL;
    }

    g_hash_table_destroy (plugin_store->names);
    plugin_store->names = NULL;

    g_strfreev (plugin_store->dirs);
    plugin_store->dirs = NULL;

    plugin_store = NULL;
}


void
_moo_window_attach_plugins (MooEditWindow *window)
{
    GSList *l;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));

    plugin_store_init ();

    for (l = plugin_store->list; l != NULL; l = l->next)
        plugin_attach_win (l->data, window);
}


void
_moo_window_detach_plugins (MooEditWindow *window)
{
    GSList *l;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));

    plugin_store_init ();

    for (l = plugin_store->list; l != NULL; l = l->next)
        plugin_detach_win (l->data, window);
}


void
_moo_doc_attach_plugins (MooEditWindow *window,
                         MooEdit       *doc)
{
    GSList *l;

    g_return_if_fail (!window || MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT (doc));

    plugin_store_init ();

    for (l = plugin_store->list; l != NULL; l = l->next)
        plugin_attach_doc (l->data, window, doc);
}


void
_moo_doc_detach_plugins (MooEditWindow *window,
                         MooEdit       *doc)
{
    GSList *l;

    g_return_if_fail (!window || MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT (doc));

    plugin_store_init ();

    for (l = plugin_store->list; l != NULL; l = l->next)
        plugin_detach_doc (l->data, window, doc);
}


static gboolean
moo_plugin_visible (MooPlugin *plugin)
{
    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), FALSE);
    return plugin->params->visible ? TRUE : FALSE;
}


/**
 * moo_plugin_set_info:
 */
void
moo_plugin_set_info (MooPlugin     *plugin,
                     MooPluginInfo *info)
{
    g_return_if_fail (MOO_IS_PLUGIN (plugin));

    if (plugin->info != info)
    {
        moo_plugin_info_free (plugin->info);
        plugin->info = moo_plugin_info_copy (info);
    }
}


/**
 * moo_plugin_set_doc_plugin_type:
 */
void
moo_plugin_set_doc_plugin_type (MooPlugin   *plugin,
                                GType        type)
{
    g_return_if_fail (MOO_IS_PLUGIN (plugin));
    g_return_if_fail (g_type_is_a (type, MOO_TYPE_DOC_PLUGIN));
    plugin->doc_plugin_type = type;
}


/**
 * moo_plugin_set_win_plugin_type:
 */
void
moo_plugin_set_win_plugin_type (MooPlugin *plugin,
                                GType      type)
{
    g_return_if_fail (MOO_IS_PLUGIN (plugin));
    g_return_if_fail (g_type_is_a (type, MOO_TYPE_WIN_PLUGIN));
    plugin->win_plugin_type = type;
}


/**
 * moo_plugin_info_new: (constructor-of MooPluginInfo)
 *
 * @name: (type const-utf8)
 * @description: (type const-utf8) (allow-none) (default NULL)
 * @author: (type const-utf8) (allow-none) (default NULL)
 * @version: (type const-utf8) (allow-none) (default NULL)
 */
MooPluginInfo *
moo_plugin_info_new (const char     *name,
                     const char     *description,
                     const char     *author,
                     const char     *version)
{
    MooPluginInfo *info;

    g_return_val_if_fail (name != NULL, NULL);

    info = g_new0 (MooPluginInfo, 1);
    info->name = g_strdup (name);
    info->description = g_strdup (MOO_NZS (description));
    info->author = g_strdup (MOO_NZS (author));
    info->version = g_strdup (MOO_NZS (version));

    return info;
}


MooPluginInfo *
moo_plugin_info_copy (MooPluginInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);
    return moo_plugin_info_new (info->name, info->description,
                                info->author, info->version);
}


void
moo_plugin_info_free (MooPluginInfo *info)
{
    if (info)
    {
        g_free (info->name);
        g_free (info->description);
        g_free (info->author);
        g_free (info->version);
        g_free (info);
    }
}


/**
 * moo_plugin_params_new: (constructor-of MooPluginParams)
 *
 * @enabled: (default TRUE)
 * @visible: (default TRUE)
 */
MooPluginParams *
moo_plugin_params_new (gboolean enabled,
                       gboolean visible)
{
    MooPluginParams *params = g_new0 (MooPluginParams, 1);
    params->enabled = enabled != 0;
    params->visible = visible != 0;
    return params;
}


MooPluginParams *
moo_plugin_params_copy (MooPluginParams *params)
{
    g_return_val_if_fail (params != NULL, NULL);
    return moo_plugin_params_new (params->enabled, params->visible);
}


void
moo_plugin_params_free (MooPluginParams *params)
{
    g_free (params);
}


/***************************************************************************/
/* Preferences dialog
 */

enum {
    COLUMN_ENABLED,
    COLUMN_PLUGIN_ID,
    COLUMN_PLUGIN_NAME,
    N_COLUMNS
};


static void
selection_changed (GtkTreeSelection *selection,
                   PrefsPageXml     *gxml)
{
    MooPlugin *plugin = NULL;
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        char *id = NULL;
        gtk_tree_model_get (model, &iter, COLUMN_PLUGIN_ID, &id, -1);
        plugin = moo_plugin_lookup (id);
        g_free (id);
    }

    gtk_widget_set_sensitive (GTK_WIDGET (gxml->info), plugin != NULL);

    if (plugin)
    {
        gtk_label_set_text (gxml->author, moo_plugin_author (plugin));
        gtk_label_set_text (gxml->version, moo_plugin_version (plugin));
        gtk_label_set_text (gxml->description, moo_plugin_description (plugin));
    }
    else
    {
        gtk_label_set_text (gxml->author, "");
        gtk_label_set_text (gxml->version, "");
        gtk_label_set_text (gxml->description, "");
    }
}


static int
cmp_page_and_id (GObject    *page,
                 const char *id)
{
    const char *page_id = g_object_get_data (page, "moo-plugin-id");
    return page_id ? strcmp (id, page_id) : 1;
}

static void
sync_pages (MooPrefsDialog *dialog)
{
    GSList *old_plugin_pages, *plugin_pages, *plugin_ids, *l, *plugins;

    plugins = moo_list_plugins ();
    plugin_ids = NULL;

    for (l = plugins; l != NULL; l = l->next)
    {
        MooPlugin *plugin = l->data;
        plugin_ids = g_slist_append (plugin_ids, g_strdup (moo_plugin_id (plugin)));
    }

    old_plugin_pages = g_object_get_data (G_OBJECT (dialog), "moo-plugin-prefs-pages");
    plugin_pages = NULL;

    for (l = plugins; l != NULL; l = l->next)
    {
        MooPlugin *plugin = l->data;

        if (moo_plugin_enabled (plugin) &&
            MOO_PLUGIN_GET_CLASS(plugin)->create_prefs_page)
        {
            GSList *link = g_slist_find_custom (old_plugin_pages,
                                                moo_plugin_id (l->data),
                                                (GCompareFunc) cmp_page_and_id);

            if (link)
            {
                plugin_pages = g_slist_append (plugin_pages, link->data);
            }
            else
            {
                GtkWidget *plugin_page = MOO_PLUGIN_GET_CLASS(plugin)->create_prefs_page (plugin);

                if (plugin_page)
                {
                    if (!MOO_IS_PREFS_PAGE (plugin_page))
                    {
                        g_critical ("oops");
                    }
                    else
                    {
                        g_object_set_data_full (G_OBJECT (plugin_page), "moo-plugin-id",
                                                g_strdup (moo_plugin_id (plugin)),
                                                g_free);
                        plugin_pages = g_slist_append (plugin_pages, plugin_page);
                        moo_prefs_dialog_insert_page (dialog, plugin_page, -1);
                    }
                }
            }
        }
    }

    for (l = old_plugin_pages; l != NULL; l = l->next)
        if (!g_slist_find (plugin_pages, l->data))
            moo_prefs_dialog_remove_page (dialog, l->data);

    g_object_set_data_full (G_OBJECT (dialog), "moo-plugin-prefs-pages",
                            plugin_pages, (GDestroyNotify) g_slist_free);

    g_slist_foreach (plugin_ids, (GFunc) g_free, NULL);
    g_slist_free (plugin_ids);
    g_slist_free (plugins);
}


static void
prefs_page_init (MooPrefsPage *page,
                 PrefsPageXml *gxml)
{
    GtkListStore *store;
    GtkTreeModel *model;
    GSList *l, *plugins;

    model = gtk_tree_view_get_model (gxml->treeview);
    store = GTK_LIST_STORE (model);

    gtk_list_store_clear (store);
    plugins = moo_list_plugins ();

    for (l = plugins; l != NULL; l = l->next)
    {
        GtkTreeIter iter;
        MooPlugin *plugin = l->data;

        if (moo_plugin_visible (plugin))
        {
            gtk_list_store_append (store, &iter);
            gtk_list_store_set (store, &iter,
                                COLUMN_ENABLED, moo_plugin_enabled (plugin),
                                COLUMN_PLUGIN_ID, moo_plugin_id (plugin),
                                COLUMN_PLUGIN_NAME, moo_plugin_name (plugin),
                                -1);
        }
    }

    selection_changed (gtk_tree_view_get_selection (gxml->treeview), gxml);

    g_slist_free (plugins);
    sync_pages (g_object_get_data (G_OBJECT (page), "moo-plugin-prefs-dialog"));
}


static void
prefs_page_apply (MooPrefsPage *page,
                  PrefsPageXml *gxml)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    MooPrefsDialog *dialog;

    dialog = g_object_get_data (G_OBJECT (page), "moo-plugin-prefs-dialog");

    model = gtk_tree_view_get_model (gxml->treeview);

    if (gtk_tree_model_get_iter_first (model, &iter)) do
    {
        MooPlugin *plugin;
        gboolean enabled;
        char *id = NULL;
        char *prefs_key;

        gtk_tree_model_get (model, &iter,
                            COLUMN_ENABLED, &enabled,
                            COLUMN_PLUGIN_ID, &id,
                            -1);

        g_return_if_fail (id != NULL);
        plugin = moo_plugin_lookup (id);
        g_return_if_fail (plugin != NULL);

        moo_plugin_set_enabled (plugin, enabled);

        prefs_key = make_prefs_key (plugin, PLUGIN_PREFS_ENABLED);
        moo_prefs_set_bool (prefs_key, enabled);

        g_free (prefs_key);
        g_free (id);
    }
    while (gtk_tree_model_iter_next (model, &iter));

    sync_pages (dialog);
}


static void
enable_toggled (GtkCellRendererToggle *cell,
                gchar                 *tree_path,
                GtkListStore          *store)
{
    GtkTreePath *path = gtk_tree_path_new_from_string (tree_path);
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path))
        gtk_list_store_set (store, &iter,
                            COLUMN_ENABLED,
                            !gtk_cell_renderer_toggle_get_active (cell),
                            -1);
    gtk_tree_path_free (path);
}


void
moo_plugin_attach_prefs (GtkWidget *dialog)
{
    GtkWidget *page;
    GtkCellRenderer *cell;
    GtkListStore *store;
    GtkTreeSelection *selection;
    PrefsPageXml *gxml;

    g_return_if_fail (MOO_IS_PREFS_DIALOG (dialog));

    page = moo_prefs_page_new (C_("Prefs page label", "Plugins"), MOO_STOCK_PLUGINS);
    gxml = prefs_page_xml_new_with_root (page);
    g_object_set_data (G_OBJECT (page), "moo-plugin-prefs-xml", gxml);
    g_object_set_data (G_OBJECT (page), "moo-plugin-prefs-dialog", dialog);
    g_signal_connect (page, "init", G_CALLBACK (prefs_page_init), gxml);
    g_signal_connect (page, "apply", G_CALLBACK (prefs_page_apply), gxml);

#ifdef MOO_ENABLE_HELP
    moo_help_set_id (GTK_WIDGET (page), HELP_SECTION_PREFS_PLUGINS);
#endif

    selection = gtk_tree_view_get_selection (gxml->treeview);
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    g_signal_connect (selection, "changed", G_CALLBACK (selection_changed), gxml);

    store = gtk_list_store_new (N_COLUMNS,
                                G_TYPE_BOOLEAN,
                                G_TYPE_STRING,
                                G_TYPE_STRING);
    gtk_tree_view_set_model (gxml->treeview, GTK_TREE_MODEL (store));
    g_object_unref (store);

    cell = gtk_cell_renderer_toggle_new ();
    g_object_set (cell, "activatable", TRUE, NULL);
    g_signal_connect (cell, "toggled", G_CALLBACK (enable_toggled), store);
    /* Column label on Plugins prefs page */
    gtk_tree_view_insert_column_with_attributes (gxml->treeview, 0,
                                                 C_("plugin-prefs-column", "Enabled"), cell,
                                                 "active", COLUMN_ENABLED, NULL);

    cell = gtk_cell_renderer_text_new ();
    /* Column label on Plugins prefs page */
    gtk_tree_view_insert_column_with_attributes (gxml->treeview, 1,
                                                 C_("plugin-prefs-column", "Plugin"), cell,
                                                 "text", COLUMN_PLUGIN_NAME, NULL);

    moo_prefs_dialog_append_page (MOO_PREFS_DIALOG (dialog), GTK_WIDGET (page));
    prefs_page_init (MOO_PREFS_PAGE (page), gxml);
}


/*****************************************************************************/
/* MooPluginMeth
 */

static void
plugin_type_cleanup (GType type)
{
    gpointer meths_table;

    g_assert (g_type_is_a (type, MOO_TYPE_PLUGIN));

    meths_table = g_type_get_qdata (type, MOO_PLUGIN_METHS_QUARK);

    if (meths_table)
    {
        g_hash_table_destroy (meths_table);
        g_type_set_qdata (type, MOO_PLUGIN_METHS_QUARK, NULL);
    }
}


MooPluginMeth *
moo_plugin_lookup_method (gpointer    plugin,
                          const char *name)
{
    GHashTable *meths;
    char *norm_name;
    MooPluginMeth *m = NULL;

    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), NULL);
    g_return_val_if_fail (name != NULL, NULL);

    meths = g_type_get_qdata (G_OBJECT_TYPE (plugin), MOO_PLUGIN_METHS_QUARK);

    if (!meths)
        return NULL;

    norm_name = g_strdelimit (g_strdup (name), "_", '-');
    m = g_hash_table_lookup (meths, norm_name);
    g_free (norm_name);
    return m;
}


static void
prepend_meth_name (const char *name,
                   G_GNUC_UNUSED gpointer meth,
                   GSList **list)
{
    *list = g_slist_prepend (*list, g_strdup (name));
}

GSList *
moo_plugin_list_methods (gpointer plugin)
{
    GHashTable *meths;
    GSList *list = NULL;

    g_return_val_if_fail (MOO_IS_PLUGIN (plugin), NULL);

    meths = g_type_get_qdata (G_OBJECT_TYPE (plugin), MOO_PLUGIN_METHS_QUARK);

    if (!meths)
        return NULL;

    g_hash_table_foreach (meths, (GHFunc) prepend_meth_name, &list);
    return list;
}


void
moo_plugin_call_method (gpointer        plugin,
                        const char     *name,
                        ...)
{
    va_list args;

    g_return_if_fail (MOO_IS_PLUGIN (plugin));
    g_return_if_fail (name != NULL);

    va_start (args, name);
    moo_plugin_call_method_valist (plugin, name, args);
    va_end (args);
}


#define MAX_STACK_VALUES (16)

void
moo_plugin_call_method_valist (gpointer        plugin,
                               const char     *name,
                               va_list         var_args)
{
    MooPluginMeth *meth;
    GValue *plugin_and_args, *args, *freeme = NULL;
    GValue stack_params[MAX_STACK_VALUES];
    guint i;

    g_return_if_fail (MOO_IS_PLUGIN (plugin));
    g_return_if_fail (name != NULL);

    meth = moo_plugin_lookup_method (plugin, name);

    if (!meth)
    {
        g_warning ("plugin '%s' does not have method '%s'",
                   moo_plugin_id (plugin), name);
        return;
    }

    g_assert (meth->ptype == G_OBJECT_TYPE (plugin));

    if (meth->n_params < MAX_STACK_VALUES)
        plugin_and_args = stack_params;
    else
        plugin_and_args = freeme = g_new (GValue, meth->n_params + 1);

    args = plugin_and_args + 1;

    for (i = 0; i < meth->n_params; i++)
    {
        char *error;
        GType type = meth->param_types[i] & ~G_SIGNAL_TYPE_STATIC_SCOPE;
        gboolean static_scope = meth->param_types[i] & G_SIGNAL_TYPE_STATIC_SCOPE;

        args[i].g_type = 0;
        g_value_init (args + i, type);
        G_VALUE_COLLECT (args + i, var_args,
                         static_scope ? G_VALUE_NOCOPY_CONTENTS : 0,
                         &error);

        if (error)
        {
            g_warning ("%s", error);
            g_free (error);

            while (i--)
                g_value_unset (args + i);

            g_free (freeme);
            return;
        }
    }

    plugin_and_args->g_type = 0;
    g_value_init (plugin_and_args, G_OBJECT_TYPE (plugin));
    g_value_set_object (plugin_and_args, plugin);

    if (meth->return_type == G_TYPE_NONE)
    {
        moo_plugin_call_methodv (plugin_and_args, name, NULL);
    }
    else
    {
        GValue return_val;
        char *error = NULL;
        GType type = meth->return_type & ~G_SIGNAL_TYPE_STATIC_SCOPE;
        gboolean static_scope = meth->return_type & G_SIGNAL_TYPE_STATIC_SCOPE;

        return_val.g_type = 0;
        g_value_init (&return_val, type);

        moo_plugin_call_methodv (plugin_and_args, name, &return_val);

        G_VALUE_LCOPY (&return_val, var_args,
                       static_scope ? G_VALUE_NOCOPY_CONTENTS : 0,
                       &error);

        if (!error)
        {
            g_value_unset (&return_val);
        }
        else
        {
            g_warning ("%s", error);
            g_free (error);
        }
    }

    for (i = 0; i < meth->n_params + 1; i++)
        g_value_unset (plugin_and_args + i);

    g_free (freeme);
}


void
moo_plugin_call_methodv (const GValue *plugin_and_args,
                         const char   *name,
                         GValue       *return_val)
{
    gpointer plugin;
    MooPluginMeth *meth;

    g_return_if_fail (plugin_and_args != NULL);
    g_return_if_fail (name != NULL);

    plugin = g_value_get_object (plugin_and_args);
    g_return_if_fail (plugin != NULL);

    meth = moo_plugin_lookup_method (plugin, name);

    if (!meth)
    {
        g_warning ("plugin '%s' does not have method '%s'",
                   moo_plugin_id (plugin), name);
        return;
    }

    g_return_if_fail (meth->return_type == G_TYPE_NONE ||
                      (return_val != NULL && meth->return_type == G_VALUE_TYPE (return_val)));

    g_closure_invoke (meth->closure, return_val, meth->n_params + 1, plugin_and_args, NULL);
}


void
moo_plugin_method_new (const char     *name,
                       GType           ptype,
                       GCallback       method,
                       GClosureMarshal c_marshaller,
                       GType           return_type,
                       guint           n_params,
                       ...)
{
    va_list args;
    GClosure *closure;

    g_return_if_fail (g_type_is_a (ptype, MOO_TYPE_PLUGIN));
    g_return_if_fail (name != NULL);
    g_return_if_fail (method != NULL);
    g_return_if_fail (c_marshaller != NULL);

    closure = g_cclosure_new (method, NULL, NULL);
    g_closure_sink (g_closure_ref (closure));

    va_start (args, n_params);
    moo_plugin_method_new_valist (name, ptype, closure, c_marshaller,
                                  return_type, n_params, args);
    va_end (args);

    g_closure_unref (closure);
}


void
moo_plugin_method_new_valist (const char     *name,
                              GType           ptype,
                              GClosure       *closure,
                              GClosureMarshal c_marshaller,
                              GType           return_type,
                              guint           n_params,
                              va_list         args)
{
    GType *param_types = NULL;
    guint i;

    g_return_if_fail (g_type_is_a (ptype, MOO_TYPE_PLUGIN));
    g_return_if_fail (name != NULL);
    g_return_if_fail (closure != NULL);
    g_return_if_fail (c_marshaller != NULL);

    if (n_params)
    {
        param_types = g_new (GType, n_params);

        for (i = 0; i < n_params; ++i)
            param_types[i] = va_arg (args, GType);
    }

    moo_plugin_method_newv (name, ptype, closure, c_marshaller,
                            return_type, n_params, param_types);

    g_free (param_types);
}


static void
meth_free (MooPluginMeth *meth)
{
    if (meth)
    {
        g_free (meth->param_types);
        g_closure_invalidate (meth->closure);
        g_closure_unref (meth->closure);
        g_free (meth);
    }
}


void
moo_plugin_method_newv (const char     *name,
                        GType           ptype,
                        GClosure       *closure,
                        GClosureMarshal c_marshaller,
                        GType           return_type,
                        guint           n_params,
                        const GType    *param_types)
{
    MooPluginMeth *m;
    GHashTable *meths;
    char *norm_name;

    g_return_if_fail (g_type_is_a (ptype, MOO_TYPE_PLUGIN));
    g_return_if_fail (name != NULL);
    g_return_if_fail (closure != NULL);
    g_return_if_fail (c_marshaller != NULL);
    g_return_if_fail (!n_params || param_types);

    norm_name = g_strdelimit (g_strdup (name), "_", '-');
    meths = g_type_get_qdata (ptype, MOO_PLUGIN_METHS_QUARK);

    if (meths)
    {
        if (g_hash_table_lookup (meths, norm_name) != NULL)
        {
            g_warning ("method '%s' is already registered for type '%s'",
                       name, g_type_name (ptype));
            g_free (norm_name);
            return;
        }
    }
    else
    {
        meths = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                       (GDestroyNotify) meth_free);
        g_type_set_qdata (ptype, MOO_PLUGIN_METHS_QUARK, meths);
    }

    m = g_new0 (MooPluginMeth, 1);
    m->ptype = ptype;
    m->return_type = return_type;
    m->n_params = n_params;
    m->param_types = n_params ? g_memdup (param_types, n_params * sizeof (GType)) : NULL;
    m->closure = g_closure_ref (closure);
    g_closure_sink (closure);
    g_closure_set_marshal (closure, c_marshaller);

    g_hash_table_insert (meths, norm_name, m);
}
