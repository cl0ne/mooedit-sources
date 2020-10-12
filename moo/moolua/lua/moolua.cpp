/*
 *   moolua.cpp
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

/* This is not a part of the Lua distribution */

#include "config.h"
#include "moolua.h"
#include "mooutils/moospawn.h"
#include "mooutils/mooutils.h"
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef __WIN32__
#include <io.h>
#endif
#include <mooglib/moo-glib.h>

void
lua_push_utf8string (lua_State  *L,
                     const char *s,
                     int         len)
{
    if (!g_utf8_validate (s, len, NULL))
        luaL_error (L, "string is not valid UTF-8");

    if (len < 0)
        len = (int) strlen (s);

    lua_pushlstring (L, s, len);
}

void
lua_take_utf8string (lua_State *L,
                     char      *s)
{
    if (!g_utf8_validate (s, -1, NULL))
    {
        g_free (s);
        luaL_error (L, "string is not valid UTF-8");
    }

    lua_pushstring (L, s);
    g_free (s);
}

const char *
lua_check_utf8string (lua_State  *L,
                      int         numArg,
                      size_t     *len)
{
    const char *s;

    s = luaL_checklstring (L, numArg, len);

    if (!g_utf8_validate (s, -1, NULL))
        luaL_argerror (L, numArg, "string is not valid UTF-8");

    return s;
}


enum {
    my_F_OK = 0,
    my_R_OK = 1 << 0,
    my_W_OK = 1 << 1,
    my_X_OK = 1 << 2
};

static int
cfunc__access (lua_State *L)
{
    const char *filename = luaL_checkstring (L, 1);
    int mode = luaL_checkint (L, 2);

    mgw_access_mode_t m = { MGW_F_OK };
    if (mode & my_R_OK)
        m.value = mgw_access_mode_value_t (m.value | MGW_R_OK);
    if (mode & my_W_OK)
        m.value = mgw_access_mode_value_t (m.value | MGW_W_OK);
    if (mode & my_X_OK)
        m.value = mgw_access_mode_value_t (m.value | MGW_X_OK);

    lua_pushboolean (L, mgw_access (filename, m) == 0);
    return 1;
}

static int
cfunc__execute (lua_State *L)
{
    char **argv = 0;
    const char *command = luaL_checkstring (L, 1);
    GError *error = NULL;
    int exit_status;

#ifdef __WIN32__
    argv = _moo_win32_lame_parse_cmd_line (command, &error);
#else
    g_shell_parse_argv (command, NULL, &argv, &error);
#endif

    if (!argv)
    {
        lua_pushinteger (L, 2);
        g_message ("could not parse command line '%s': %s", command, error->message);
        g_error_free (error);
        return 1;
    }

    if (!g_spawn_sync (NULL, argv, NULL,
                       (GSpawnFlags) (G_SPAWN_SEARCH_PATH | MOO_SPAWN_WIN32_HIDDEN_CONSOLE),
                       NULL, NULL, NULL, NULL,
                       &exit_status, &error))
    {
        lua_pushinteger (L, 2);
        g_message ("could not run command '%s': %s", command, error->message);
        g_error_free (error);
        g_strfreev (argv);
        return 1;
    }

    g_strfreev (argv);
    lua_pushinteger (L, exit_status);
    return 1;
}

static const luaL_Reg moo_utils_funcs[] = {
  { "_access", cfunc__access },
  { "_execute", cfunc__execute },
  { NULL, NULL }
};

int
luaopen_moo_utils (lua_State *L)
{
    luaL_register (L, "_moo_utils", moo_utils_funcs);

    lua_pushinteger (L, my_F_OK);
    lua_setfield (L, -2, "F_OK");
    lua_pushinteger (L, my_R_OK);
    lua_setfield (L, -2, "R_OK");
    lua_pushinteger (L, my_W_OK);
    lua_setfield (L, -2, "W_OK");
    lua_pushinteger (L, my_X_OK);
    lua_setfield (L, -2, "X_OK");

    return 1;
}


void
lua_addpath (lua_State  *L,
             char      **dirs,
             unsigned    n_dirs)
{
    guint i;
    GString *new_path;
    const char *path;

    if (!n_dirs)
        return;

    lua_getglobal (L, "package"); /* push package */
    if (!lua_istable (L, -1))
    {
        lua_pop (L, 1);
        g_critical ("%s: package variable missing or not a table", G_STRFUNC);
        return;
    }

    lua_getfield (L, -1, "path"); /* push package.path */
    if (!lua_isstring (L, -1))
    {
        lua_pop (L, 2);
        g_critical ("%s: package.path is missing or not a string", G_STRFUNC);
        return;
    }

    path = lua_tostring (L, -1);
    new_path = g_string_new (NULL);

    for (i = 0; i < n_dirs; ++i)
        g_string_append_printf (new_path, "%s/?.lua;", dirs[i]);

    g_string_append (new_path, path);

    lua_pushstring (L, new_path->str);
    lua_setfield (L, -3, "path"); /* pops the string */
    lua_pop (L, 2); /* pop package.path and package */

    g_string_free (new_path, TRUE);
}

void
moo_lua_add_user_path (lua_State *L)
{
    char **dirs;
    dirs = moo_get_data_subdirs ("lua");
    lua_addpath (L, dirs, g_strv_length (dirs));
    g_strfreev (dirs);
}
