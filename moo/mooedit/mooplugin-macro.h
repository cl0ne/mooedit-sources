/*
 *   mooplugin-macro.h
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

#ifndef MOO_PLUGIN_MACRO_H
#define MOO_PLUGIN_MACRO_H

#include <mooedit/mooplugin.h>


#define MOO_PLUGIN_INIT_FUNC_DECL   \
G_MODULE_EXPORT gboolean            \
MOO_PLUGIN_INIT_FUNC (GType *type)

#define MOO_MODULE_INIT_FUNC_DECL   \
G_MODULE_EXPORT gboolean            \
MOO_MODULE_INIT_FUNC (void)


#define MOO_PLUGIN_DEFINE_INFO(plugin_name__,name__,                        \
                               description__,author__,version__)            \
                                                                            \
static const MooPluginInfo plugin_name__##_plugin_info =                    \
    {(char*) name__, (char*) description__,                                 \
     (char*) author__, (char*) version__};


#define MOO_PLUGIN_DEFINE_FULL(Name__,name__,                               \
                               class_init_code__,instance_init_code__,      \
                               attach_win__,detach_win__,                   \
                               attach_doc__,detach_doc__,                   \
                               prefs_page_func__,                           \
                               WIN_PLUGIN_TYPE__,DOC_PLUGIN_TYPE__)         \
                                                                            \
static gpointer name__##_plugin_parent_class;                               \
static gboolean name__##_plugin_init (Name__##Plugin *plugin);              \
static void name__##_plugin_deinit (Name__##Plugin *plugin);                \
                                                                            \
typedef struct {                                                            \
    MooPluginClass parent_class;                                            \
} Name__##PluginClass;                                                      \
                                                                            \
static void                                                                 \
name__##_plugin_class_init (MooPluginClass *klass)                          \
{                                                                           \
    name__##_plugin_parent_class = g_type_class_peek_parent (klass);        \
                                                                            \
    klass->init = (MooPluginInitFunc) name__##_plugin_init;                 \
    klass->deinit = (MooPluginDeinitFunc) name__##_plugin_deinit;           \
    klass->attach_win = attach_win__;                                       \
    klass->detach_win = detach_win__;                                       \
    klass->attach_doc = attach_doc__;                                       \
    klass->detach_doc = detach_doc__;                                       \
    klass->create_prefs_page = prefs_page_func__;                           \
                                                                            \
    {                                                                       \
        class_init_code__ ;                                                 \
    }                                                                       \
}                                                                           \
                                                                            \
static void                                                                 \
name__##_plugin_instance_init (MooPlugin *plugin)                           \
{                                                                           \
    plugin->win_plugin_type = WIN_PLUGIN_TYPE__;                            \
    plugin->doc_plugin_type = DOC_PLUGIN_TYPE__;                            \
                                                                            \
    {                                                                       \
        instance_init_code__ ;                                              \
    }                                                                       \
}                                                                           \
                                                                            \
static GType name__##_plugin_get_type (void) G_GNUC_CONST;                  \
                                                                            \
static GType                                                                \
name__##_plugin_get_type (void)                                             \
{                                                                           \
    static GType type__ = 0;                                                \
                                                                            \
    if (G_UNLIKELY (type__ == 0))                                           \
    {                                                                       \
        static const GTypeInfo info__ = {                                   \
            sizeof (Name__##PluginClass),                                   \
            (GBaseInitFunc) NULL,                                           \
            (GBaseFinalizeFunc) NULL,                                       \
            (GClassInitFunc) name__##_plugin_class_init,                    \
            (GClassFinalizeFunc) NULL,                                      \
            NULL,   /* class_data */                                        \
            sizeof (Name__##Plugin),                                        \
            0,      /* n_preallocs */                                       \
            (GInstanceInitFunc) name__##_plugin_instance_init,              \
            NULL    /* value_table */                                       \
        };                                                                  \
                                                                            \
        type__ = g_type_register_static (MOO_TYPE_PLUGIN,                   \
                                         #Name__ "Plugin", &info__,         \
                                         (GTypeFlags) 0);                   \
    }                                                                       \
                                                                            \
    return type__;                                                          \
}

#define MOO_PLUGIN_DEFINE(Name__,name__,                                    \
                          attach_win__,detach_win__,                        \
                          attach_doc__,detach_doc__,                        \
                          prefs_page_func__,                                \
                          WIN_PLUGIN_TYPE__,DOC_PLUGIN_TYPE__)              \
MOO_PLUGIN_DEFINE_FULL(Name__,name__,                                       \
                       {},{},                                               \
                       attach_win__,detach_win__,                           \
                       attach_doc__,detach_doc__,                           \
                       prefs_page_func__,                                   \
                       WIN_PLUGIN_TYPE__,DOC_PLUGIN_TYPE__)

#define MOO_WIN_PLUGIN_DEFINE(Name__,name__)                                    \
                                                                                \
typedef struct {                                                                \
    MooWinPluginClass parent_class;                                             \
} Name__##WindowPluginClass;                                                    \
                                                                                \
static GType name__##_window_plugin_get_type (void) G_GNUC_CONST;               \
static gboolean name__##_window_plugin_create (Name__##WindowPlugin *plugin);   \
static void name__##_window_plugin_destroy (Name__##WindowPlugin *plugin);      \
                                                                                \
static gpointer name__##_window_plugin_parent_class = NULL;                     \
                                                                                \
static void                                                                     \
name__##_window_plugin_class_init (MooWinPluginClass *klass)                    \
{                                                                               \
    name__##_window_plugin_parent_class = g_type_class_peek_parent (klass);     \
    klass->create = (MooWinPluginCreateFunc) name__##_window_plugin_create;     \
    klass->destroy = (MooWinPluginDestroyFunc) name__##_window_plugin_destroy;  \
}                                                                               \
                                                                                \
static GType                                                                    \
name__##_window_plugin_get_type (void)                                          \
{                                                                               \
    static GType type__ = 0;                                                    \
                                                                                \
    if (G_UNLIKELY (type__ == 0))                                               \
    {                                                                           \
        static const GTypeInfo info__ = {                                       \
            sizeof (Name__##WindowPluginClass),                                 \
            (GBaseInitFunc) NULL,                                               \
            (GBaseFinalizeFunc) NULL,                                           \
            (GClassInitFunc) name__##_window_plugin_class_init,                 \
            (GClassFinalizeFunc) NULL,                                          \
            NULL,   /* class_data */                                            \
            sizeof (Name__##WindowPlugin),                                      \
            0,      /* n_preallocs */                                           \
            NULL,                                                               \
            NULL    /* value_table */                                           \
        };                                                                      \
                                                                                \
        type__ = g_type_register_static (MOO_TYPE_WIN_PLUGIN,                   \
                                         #Name__ "WindowPlugin",&info__,        \
                                         (GTypeFlags) 0);                       \
    }                                                                           \
                                                                                \
    return type__;                                                              \
}


#define MOO_DOC_PLUGIN_DEFINE(Name__,name__)                                \
                                                                            \
typedef struct {                                                            \
    MooDocPluginClass parent_class;                                         \
} Name__##DocPluginClass;                                                   \
                                                                            \
static GType name__##_doc_plugin_get_type (void) G_GNUC_CONST;              \
static gboolean name__##_doc_plugin_create (Name__##DocPlugin *plugin);     \
static void name__##_doc_plugin_destroy (Name__##DocPlugin *plugin);        \
                                                                            \
static gpointer name__##_doc_plugin_parent_class = NULL;                    \
                                                                            \
static void                                                                 \
name__##_doc_plugin_class_init (MooDocPluginClass *klass)                   \
{                                                                           \
    name__##_doc_plugin_parent_class = g_type_class_peek_parent (klass);    \
    klass->create = (MooDocPluginCreateFunc) name__##_doc_plugin_create;    \
    klass->destroy = (MooDocPluginDestroyFunc) name__##_doc_plugin_destroy; \
}                                                                           \
                                                                            \
static GType                                                                \
name__##_doc_plugin_get_type (void)                                         \
{                                                                           \
    static GType type__ = 0;                                                \
                                                                            \
    if (G_UNLIKELY (type__ == 0))                                           \
    {                                                                       \
        static const GTypeInfo info__ = {                                   \
            sizeof (Name__##DocPluginClass),                                \
            (GBaseInitFunc) NULL,                                           \
            (GBaseFinalizeFunc) NULL,                                       \
            (GClassInitFunc) name__##_doc_plugin_class_init,                \
            (GClassFinalizeFunc) NULL,                                      \
            NULL,   /* class_data */                                        \
            sizeof (Name__##DocPlugin),                                     \
            0,      /* n_preallocs */                                       \
            NULL,                                                           \
            NULL    /* value_table */                                       \
        };                                                                  \
                                                                            \
        type__ = g_type_register_static (MOO_TYPE_DOC_PLUGIN,               \
                                         #Name__ "DocPlugin", &info__,      \
                                         (GTypeFlags) 0);                   \
    }                                                                       \
                                                                            \
    return type__;                                                          \
}


#endif /* MOO_PLUGIN_MACRO_H */
