/*
 *   mooplugin-builtin.h
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

#ifndef MOO_EDIT_PLUGINS_H
#define MOO_EDIT_PLUGINS_H

#include <mooglib/moo-glib.h>

G_BEGIN_DECLS


void         moo_plugin_init                (void);

#ifndef __WIN32__
gboolean    _moo_ctags_plugin_init          (void);
#endif

gboolean    _moo_find_plugin_init           (void);
gboolean    _moo_file_selector_plugin_init  (void);
gboolean    _moo_project_plugin_init        (void);
gboolean    _moo_file_list_plugin_init      (void);
gboolean    _moo_user_tools_plugin_init     (void);

gboolean    _moo_python_plugin_init         (void);
gboolean    _moo_lua_plugin_init            (void);

/* implemented in moofilelist.c */
gboolean    _moo_str_semicase_compare       (const char *string,
                                             const char *key);


G_END_DECLS

#endif /* MOO_EDIT_PLUGINS_H */
