/*
 *   moolua.h
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

#ifndef MOO_LUA_H
#define MOO_LUA_H

#ifdef __cplusplus

#include "lualib.h"
#include "lauxlib.h"

#define L_RETURN_BOOL(val)      \
G_STMT_START {                  \
    lua_pushboolean (L, val);   \
    return 1;                   \
} G_STMT_END

void         lua_addpath            (lua_State   *L,
                                     char      **dirs,
                                     unsigned    n_dirs);

int          luaopen_unicode        (lua_State  *L);
int          luaopen_moo_utils      (lua_State  *L);

void         lua_take_utf8string    (lua_State  *L,
                                     char       *s);
void         lua_push_utf8string    (lua_State  *L,
                                     const char *s,
                                     int         len);
const char  *lua_check_utf8string   (lua_State  *L,
                                     int         numArg,
                                     size_t     *len);

void         moo_lua_add_user_path  (lua_State  *L);

#endif /* __cplusplus */

#endif /* MOO_LUA_H */
