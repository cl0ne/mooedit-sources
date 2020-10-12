/*
 *   mooplugin-loader.h
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

#ifndef MOO_PLUGIN_LOADER_H
#define MOO_PLUGIN_LOADER_H

#include <mooedit/mooplugin.h>

G_BEGIN_DECLS


typedef struct MooPluginLoader MooPluginLoader;

typedef void (*MooLoadModuleFunc) (const char      *module_file,
                                   const char      *ini_file,
                                   gpointer         data);
typedef void (*MooLoadPluginFunc) (const char      *plugin_file,
                                   const char      *plugin_id,
                                   MooPluginInfo   *info,
                                   MooPluginParams *params,
                                   const char      *ini_file,
                                   gpointer         data);


struct MooPluginLoader
{
    MooLoadModuleFunc load_module;
    MooLoadPluginFunc load_plugin;
    gpointer data;
};

void             moo_plugin_loader_register (const MooPluginLoader  *loader,
                                             const char             *type);
MooPluginLoader *moo_plugin_loader_lookup   (const char             *type);

void             _moo_plugin_load           (const char             *dir,
                                             const char             *ini_file);
void             _moo_plugin_finish_load    (void);


G_END_DECLS

#endif /* MOO_PLUGIN_LOADER_H */
