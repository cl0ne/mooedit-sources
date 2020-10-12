/*
 *   mooplugin-loader.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "mooedit/mooplugin-loader.h"
#include "mooedit/mooplugin.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-debug.h"
#include "mooutils/mooi18n.h"
#include <string.h>
#include <stdlib.h>
#include <gmodule.h>

#ifdef __WIN32__
#include <windows.h>
#endif

#define GROUP_MODULE    "module"
#define GROUP_PLUGIN    "plugin"
#define KEY_LOADER      "type"
#define KEY_FILE        "file"
#define KEY_ID          "id"
#define KEY_NAME        "name"
#define KEY_DESCRIPTION "description"
#define KEY_AUTHOR      "author"
#define KEY_VERSION     "version"
#define KEY_ENABLED     "enabled"
#define KEY_VISIBLE     "visible"


typedef struct {
    char            *ini_file;
    char            *loader;
    char            *file;
    char            *plugin_id;
    MooPluginInfo   *plugin_info;
    MooPluginParams *plugin_params;
} ModuleInfo;


static GHashTable *registered_loaders;
static GSList *waiting_list;

static void init_loaders                    (void);
GType       _moo_c_plugin_loader_get_type   (void);
static void module_info_free                (ModuleInfo *info);


static void
moo_plugin_loader_load (const MooPluginLoader *loader,
                        ModuleInfo            *module_info)
{
    if (module_info->plugin_id)
    {
        g_return_if_fail (loader->load_plugin != NULL);
        loader->load_plugin (module_info->file,
                             module_info->plugin_id,
                             module_info->plugin_info,
                             module_info->plugin_params,
                             module_info->ini_file,
                             loader->data);
    }
    else
    {
        g_return_if_fail (loader->load_module != NULL);
        loader->load_module (module_info->file,
                             module_info->ini_file,
                             loader->data);
    }
}


MooPluginLoader *
moo_plugin_loader_lookup (const char *id)
{
    g_return_val_if_fail (id != NULL, NULL);

    if (registered_loaders)
        return g_hash_table_lookup (registered_loaders, id);
    else
        return NULL;
}


static void
moo_plugin_loader_add (const MooPluginLoader *loader,
                       const char            *type)
{
    MooPluginLoader *copy;

    g_return_if_fail (loader != NULL);
    g_return_if_fail (type != NULL);
    g_return_if_fail (registered_loaders != NULL);

    copy = g_memdup (loader, sizeof (MooPluginLoader));
    g_hash_table_insert (registered_loaders, g_strdup (type), copy);
}


void
moo_plugin_loader_register (const MooPluginLoader *loader,
                            const char            *type)
{
    GSList *open_now = NULL, *hold = NULL;
    GSList *l;

    g_return_if_fail (loader != NULL);
    g_return_if_fail (type != NULL);

    init_loaders ();
    g_return_if_fail (!g_hash_table_lookup (registered_loaders, type));

    moo_plugin_loader_add (loader, type);

    for (l = waiting_list; l != NULL; l = l->next)
    {
        ModuleInfo *info = l->data;

        if (!strcmp (info->loader, type))
            open_now = g_slist_prepend (open_now, info);
        else
            hold = g_slist_prepend (hold, info);
    }

    if (open_now)
    {
        open_now = g_slist_reverse (open_now);
        g_slist_free (waiting_list);
        waiting_list = g_slist_reverse (hold);
    }
    else
    {
        g_slist_free (hold);
    }

    while (open_now)
    {
        moo_plugin_loader_load (loader, open_now->data);
        module_info_free (open_now->data);
        open_now = g_slist_delete_link (open_now, open_now);
    }
}


static gboolean
parse_plugin_info (GKeyFile         *key_file,
                   const char       *plugin_id,
                   MooPluginInfo   **info_p,
                   MooPluginParams **params_p)
{
    MooPluginInfo *info;
    MooPluginParams *params;
    char *name;
    char *description;
    char *author;
    char *version;
    gboolean enabled = TRUE;
    gboolean visible = TRUE;

    name = g_key_file_get_locale_string (key_file, GROUP_PLUGIN, KEY_NAME, NULL, NULL);
    description = g_key_file_get_locale_string (key_file, GROUP_PLUGIN, KEY_DESCRIPTION, NULL, NULL);
    author = g_key_file_get_locale_string (key_file, GROUP_PLUGIN, KEY_AUTHOR, NULL, NULL);
    version = g_key_file_get_locale_string (key_file, GROUP_PLUGIN, KEY_VERSION, NULL, NULL);

    if (g_key_file_has_key (key_file, GROUP_PLUGIN, KEY_ENABLED, NULL))
        enabled = g_key_file_get_boolean (key_file, GROUP_PLUGIN, KEY_ENABLED, NULL);
    if (g_key_file_has_key (key_file, GROUP_PLUGIN, KEY_VISIBLE, NULL))
        visible = g_key_file_get_boolean (key_file, GROUP_PLUGIN, KEY_VISIBLE, NULL);

    info = moo_plugin_info_new (name ? name : plugin_id, description, author, version);
    params = moo_plugin_params_new (enabled, visible);

    *info_p = info;
    *params_p = params;

    g_free (name);
    g_free (description);
    g_free (author);
    g_free (version);

    return TRUE;
}


static gboolean
check_version (const char *version,
               const char *ini_file_path)
{
    char *dot;
    char *end = NULL;
    long major, minor;

    dot = strchr (version, '.');

    if (!dot || dot == version)
        goto invalid;

    major = strtol (version, &end, 10);

    if (end != dot)
        goto invalid;

    minor = strtol (dot + 1, &end, 10);

    if (*end != 0)
        goto invalid;

    if (!moo_module_check_version (major, minor))
    {
        guint current_major, current_minor;
        _moo_module_version (&current_major, &current_minor);
        /* Translators: remove the part before and including | */
        moo_message_noloc (Q_("console message|ignoring file '%s', module version '%s' is not "
                           "compatible with current version %u.%u"),
                     ini_file_path, version, current_major, current_minor);
        return FALSE;
    }

    return TRUE;

invalid:
    /* Translators: remove the part before and including | */
    moo_warning_noloc (Q_("console message|invalid module version '%s' in file '%s'"),
                       version, ini_file_path);
    return FALSE;
}


static ModuleInfo *
parse_ini_file (const char *dir,
                const char *ini_file)
{
    GKeyFile *key_file;
    GError *error = NULL;
    char *ini_file_path;
    char *file = NULL, *loader = NULL, *id = NULL, *version = NULL;
    MooPluginInfo *info = NULL;
    MooPluginParams *params = NULL;
    ModuleInfo *module_info = NULL;

    ini_file_path = g_build_filename (dir, ini_file, NULL);
    key_file = g_key_file_new ();

    if (!g_key_file_load_from_file (key_file, ini_file_path, 0, &error))
    {
        g_warning ("error parsing plugin ini file '%s': %s", ini_file_path, error->message);
        goto out;
    }

    if (!g_key_file_has_group (key_file, GROUP_MODULE))
    {
        g_warning ("plugin ini file '%s' does not have '" GROUP_MODULE "' group", ini_file_path);
        goto out;
    }

    if (!(version = g_key_file_get_string (key_file, GROUP_MODULE, KEY_VERSION, &error)))
    {
        g_warning ("plugin ini file '%s' does not specify version of module system", ini_file_path);
        goto out;
    }

    if (!check_version (version, ini_file_path))
        goto out;

    if (!(loader = g_key_file_get_string (key_file, GROUP_MODULE, KEY_LOADER, &error)))
    {
        g_warning ("plugin ini file '%s' does not specify module type", ini_file_path);
        goto out;
    }

    if (!(file = g_key_file_get_string (key_file, GROUP_MODULE, KEY_FILE, &error)))
    {
        g_warning ("plugin ini file '%s' does not specify module file", ini_file_path);
        goto out;
    }

    if (!g_path_is_absolute (file))
    {
        char *tmp = file;
        file = g_build_filename (dir, file, NULL);
        g_free (tmp);
    }

    if (g_key_file_has_group (key_file, GROUP_PLUGIN))
    {
        if (!(id = g_key_file_get_string (key_file, GROUP_PLUGIN, KEY_ID, NULL)))
        {
            g_warning ("plugin ini file '%s' does not specify plugin id", ini_file_path);
            goto out;
        }

        if (!parse_plugin_info (key_file, id, &info, &params))
            goto out;
    }

    module_info = g_new0 (ModuleInfo, 1);
    module_info->loader = loader;
    module_info->file = g_build_path (dir, file, NULL);
    module_info->plugin_id = id;
    module_info->plugin_info = info;
    module_info->plugin_params = params;
    module_info->ini_file = ini_file_path;
    ini_file_path = NULL;

out:
    if (error)
        g_error_free (error);
    g_free (ini_file_path);
    g_key_file_free (key_file);
    g_free (file);
    g_free (version);

    if (!module_info)
    {
        g_free (loader);
        g_free (id);
        moo_plugin_info_free (info);
        moo_plugin_params_free (params);
    }

    return module_info;
}


static void
module_info_free (ModuleInfo *module_info)
{
    g_free (module_info->ini_file);
    g_free (module_info->loader);
    g_free (module_info->file);
    g_free (module_info->plugin_id);
    moo_plugin_info_free (module_info->plugin_info);
    moo_plugin_params_free (module_info->plugin_params);
    g_free (module_info);
}


void
_moo_plugin_load (const char *dir,
                  const char *ini_file)
{
    ModuleInfo *module_info;
    MooPluginLoader *loader;

    g_return_if_fail (dir != NULL && ini_file != NULL);

    init_loaders ();

    module_info = parse_ini_file (dir, ini_file);

    if (module_info)
    {
        loader = moo_plugin_loader_lookup (module_info->loader);

        if (!loader)
        {
            waiting_list = g_slist_append (waiting_list, module_info);
        }
        else
        {
            moo_plugin_loader_load (loader, module_info);
            module_info_free (module_info);
        }
    }
}


void
_moo_plugin_finish_load (void)
{
    while (waiting_list)
    {
        ModuleInfo *info = waiting_list->data;
        _moo_message ("unknown module type '%s' in file %s",
                      info->loader, info->ini_file);
        module_info_free (info);
        waiting_list = g_slist_delete_link (waiting_list, waiting_list);
    }
}


static GModule *
module_open (const char *path)
{
    GModule *module;

    moo_disable_win32_error_message ();
    module = g_module_open (path, G_MODULE_BIND_LAZY);
    moo_enable_win32_error_message ();

    if (!module)
        g_warning ("could not open module '%s': %s", path, g_module_error ());

    return module;
}


static void
load_c_module (const char *module_file,
               G_GNUC_UNUSED const char *ini_file,
               G_GNUC_UNUSED gpointer data)
{
    gpointer init_func_ptr;
    MooModuleInitFunc init_func;
    GModule *module;

    module = module_open (module_file);

    if (!module)
        return;

    if (g_module_symbol (module, MOO_MODULE_INIT_FUNC_NAME, &init_func_ptr) &&
        (init_func = init_func_ptr) && init_func ())
    {
        g_module_make_resident (module);
    }

    g_module_close (module);
}


static void
load_c_plugin (const char      *plugin_file,
               const char      *plugin_id,
               MooPluginInfo   *info,
               MooPluginParams *params,
               G_GNUC_UNUSED const char *ini_file,
               G_GNUC_UNUSED gpointer data)
{
    gpointer init_func_ptr;
    MooPluginModuleInitFunc init_func;
    GModule *module;
    GType type = 0;

    module = module_open (plugin_file);

    if (!module)
        return;

    if (!g_module_symbol (module, MOO_PLUGIN_INIT_FUNC_NAME, &init_func_ptr) ||
        !(init_func = init_func_ptr) ||
        !init_func (&type))
    {
        g_module_close (module);
        return;
    }

    g_return_if_fail (g_type_is_a (type, MOO_TYPE_PLUGIN));

    if (!moo_plugin_register (plugin_id, type, info, params))
    {
        g_module_close (module);
        return;
    }

    g_module_make_resident (module);
    g_module_close (module);
}


static void
init_loaders (void)
{
    MooPluginLoader loader = {load_c_module, load_c_plugin, NULL};

    if (registered_loaders)
        return;

    registered_loaders = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                g_free, g_free);

    moo_plugin_loader_add (&loader, "C");
}
