/*
 *   mooluaplugin.cpp
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

#include "config.h"
#include "plugins/mooplugin-builtin.h"
#include "mooedit/mooplugin-macro.h"
#include "mooedit/mooplugin-loader.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooutils-messages.h"
#include "moolua/lua-module-init.h"
#include "moolua/lua-plugin-init.h"
#include "medit-lua.h"
#include "mooutils/moolist.h"

#define MOO_LUA_PLUGIN_ID "MooLua"

struct MooLuaModule {
    lua_State *L;
};

MOO_DEFINE_SLIST(ModuleList, module_list, MooLuaModule)

struct MooLuaPlugin {
    MooPlugin parent;
    ModuleList *modules;
};

static MooLuaModule *
moo_lua_module_load (const char *filename)
{
    lua_State *L = medit_lua_new ();

    if (!L)
        return NULL;

    if (!medit_lua_do_string (L, LUA_MODULE_INIT))
    {
        medit_lua_free (L);
        return NULL;
    }

    if (!medit_lua_do_file (L, filename))
    {
        medit_lua_free (L);
        return NULL;
    }

    MooLuaModule *mod = g_new0 (MooLuaModule, 1);
    mod->L = L;
    return mod;
}

static void
moo_lua_module_unload (MooLuaModule *mod)
{
    g_return_if_fail (mod != NULL);
    if (mod->L)
        medit_lua_free (mod->L);
    g_free (mod);
}

static gboolean
moo_lua_plugin_init (MooLuaPlugin *plugin)
{
    plugin->modules = NULL;
    return TRUE;
}

static void
moo_lua_plugin_deinit (MooLuaPlugin *plugin)
{
    while (plugin->modules)
    {
        MooLuaModule *mod = plugin->modules->data;
        plugin->modules = module_list_delete_link (plugin->modules, plugin->modules);
        moo_lua_module_unload (mod);
    }
}

static void
moo_lua_plugin_load_module (MooLuaPlugin *plugin,
                            const char   *module_file)
{
    if (MooLuaModule *mod = moo_lua_module_load (module_file))
        plugin->modules = module_list_prepend (plugin->modules, mod);
}


static void
load_lua_module (const char *module_file,
                 G_GNUC_UNUSED const char *ini_file,
                 G_GNUC_UNUSED gpointer data)
{
    if (MooLuaPlugin *plugin = (MooLuaPlugin*) moo_plugin_lookup (MOO_LUA_PLUGIN_ID))
        moo_lua_plugin_load_module (plugin, module_file);
}


MOO_PLUGIN_DEFINE_INFO (moo_lua,
                        N_("Lua"), N_("Lua support"),
                        "Yevgen Muntyan <" MOO_EMAIL ">",
                        MOO_VERSION)
MOO_PLUGIN_DEFINE (MooLua, moo_lua,
                   NULL, NULL, NULL, NULL, NULL,
                   0, 0)


gboolean
_moo_lua_plugin_init (void)
{
    MooPluginLoader loader = { load_lua_module, NULL, NULL };
    moo_plugin_loader_register (&loader, "Lua");
    MooPluginParams params = { TRUE, TRUE };
    return moo_plugin_register (MOO_LUA_PLUGIN_ID,
                                moo_lua_plugin_get_type (),
                                &moo_lua_plugin_info,
                                &params);
}
