/*
 *   moopython-loader.h
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

#ifndef MOO_PYTHON_LOADER_H
#define MOO_PYTHON_LOADER_H

#include <mooedit/mooplugin-loader.h>

G_BEGIN_DECLS


#define MOO_PYTHON_PLUGIN_LOADER_ID "Python"
MooPluginLoader *_moo_python_get_plugin_loader (void);

#define _moo_python_plugin_loader_free g_free


G_END_DECLS

#endif /* MOO_PYTHON_LOADER_H */
