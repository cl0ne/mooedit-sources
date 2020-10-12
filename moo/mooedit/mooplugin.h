/*
 *   mooplugin.h
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

#ifndef MOO_PLUGIN_H
#define MOO_PLUGIN_H

#include <mooedit/mooeditor.h>

G_BEGIN_DECLS

#define MOO_PLUGIN_PREFS_ROOT  "Plugins"
#define MOO_PLUGIN_DIR_BASENAME "plugins"

#define MOO_PLUGIN_INIT_FUNC            moo_plugin_module_init
#define MOO_PLUGIN_INIT_FUNC_NAME       "moo_plugin_module_init"
#define MOO_MODULE_INIT_FUNC            moo_module_init
#define MOO_MODULE_INIT_FUNC_NAME       "moo_module_init"

#define MOO_TYPE_PLUGIN                 (moo_plugin_get_type ())
#define MOO_PLUGIN(object)              (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_PLUGIN, MooPlugin))
#define MOO_PLUGIN_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_PLUGIN, MooPluginClass))
#define MOO_IS_PLUGIN(object)           (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_PLUGIN))
#define MOO_IS_PLUGIN_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_PLUGIN))
#define MOO_PLUGIN_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_PLUGIN, MooPluginClass))

#define MOO_TYPE_WIN_PLUGIN             (moo_win_plugin_get_type ())
#define MOO_WIN_PLUGIN(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_WIN_PLUGIN, MooWinPlugin))
#define MOO_WIN_PLUGIN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_WIN_PLUGIN, MooWinPluginClass))
#define MOO_IS_WIN_PLUGIN(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_WIN_PLUGIN))
#define MOO_IS_WIN_PLUGIN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_WIN_PLUGIN))
#define MOO_WIN_PLUGIN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_WIN_PLUGIN, MooWinPluginClass))

#define MOO_TYPE_DOC_PLUGIN             (moo_doc_plugin_get_type ())
#define MOO_DOC_PLUGIN(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_DOC_PLUGIN, MooDocPlugin))
#define MOO_DOC_PLUGIN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_DOC_PLUGIN, MooDocPluginClass))
#define MOO_IS_DOC_PLUGIN(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_DOC_PLUGIN))
#define MOO_IS_DOC_PLUGIN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_DOC_PLUGIN))
#define MOO_DOC_PLUGIN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_DOC_PLUGIN, MooDocPluginClass))

#define MOO_TYPE_PLUGIN_INFO            (moo_plugin_info_get_type ())
#define MOO_TYPE_PLUGIN_PARAMS          (moo_plugin_params_get_type ())


typedef struct MooPlugin            MooPlugin;
typedef struct MooPluginInfo        MooPluginInfo;
typedef struct MooPluginParams      MooPluginParams;
typedef struct MooPluginClass       MooPluginClass;
typedef struct MooWinPlugin         MooWinPlugin;
typedef struct MooWinPluginClass    MooWinPluginClass;
typedef struct MooDocPlugin         MooDocPlugin;
typedef struct MooDocPluginClass    MooDocPluginClass;
typedef struct MooPluginMeth        MooPluginMeth;


typedef gboolean    (*MooModuleInitFunc)        (void);
typedef gboolean    (*MooPluginModuleInitFunc)  (GType          *type);

typedef gboolean    (*MooPluginInitFunc)        (MooPlugin      *plugin);
typedef void        (*MooPluginDeinitFunc)      (MooPlugin      *plugin);
typedef void        (*MooPluginAttachWinFunc)   (MooPlugin      *plugin,
                                                 MooEditWindow  *window);
typedef void        (*MooPluginDetachWinFunc)   (MooPlugin      *plugin,
                                                 MooEditWindow  *window);
typedef void        (*MooPluginAttachDocFunc)   (MooPlugin      *plugin,
                                                 MooEdit        *doc,
                                                 MooEditWindow  *window);
typedef void        (*MooPluginDetachDocFunc)   (MooPlugin      *plugin,
                                                 MooEdit        *doc,
                                                 MooEditWindow  *window);

typedef GtkWidget  *(*MooPluginPrefsPageFunc)   (MooPlugin      *plugin);

typedef gboolean    (*MooWinPluginCreateFunc)   (MooWinPlugin   *win_plugin);
typedef void        (*MooWinPluginDestroyFunc)  (MooWinPlugin   *win_plugin);
typedef gboolean    (*MooDocPluginCreateFunc)   (MooDocPlugin   *doc_plugin);
typedef void        (*MooDocPluginDestroyFunc)  (MooDocPlugin   *doc_plugin);


struct MooPluginMeth
{
    GType ptype;
    GType return_type;
    guint n_params;
    GType *param_types;
    GClosure *closure;
};


struct MooPluginParams
{
    gboolean enabled;
    gboolean visible;
};

struct MooPluginInfo
{
    char *name;
    char *description;
    char *author;
    char *version;
};

struct MooPlugin
{
    GObject parent;

    gboolean initialized;

    char *id;
    GQuark id_quark;
    MooPluginInfo *info;
    MooPluginParams *params;
    GSList *docs;
    GType win_plugin_type;
    GType doc_plugin_type;
};

struct MooWinPlugin
{
    GObject parent;
    MooEditWindow *window;
    MooPlugin *plugin;
};

struct MooDocPlugin
{
    GObject parent;
    MooEditWindow *window;
    MooEdit *doc;
    MooPlugin *plugin;
};

struct MooPluginClass
{
    GObjectClass parent_class;

    /**vtable:MooPlugin**/
    gboolean    (*init)                 (MooPlugin      *plugin);
    /**vtable:MooPlugin**/
    void        (*deinit)               (MooPlugin      *plugin);
    /**vtable:MooPlugin**/
    void        (*attach_win)           (MooPlugin      *plugin,
                                         MooEditWindow  *window);
    /**vtable:MooPlugin**/
    void        (*detach_win)           (MooPlugin      *plugin,
                                         MooEditWindow  *window);
    /**vtable:MooPlugin**/
    void        (*attach_doc)           (MooPlugin      *plugin,
                                         MooEdit        *doc,
                                         MooEditWindow  *window);
    /**vtable:MooPlugin**/
    void        (*detach_doc)           (MooPlugin      *plugin,
                                         MooEdit        *doc,
                                         MooEditWindow  *window);
    /**vtable:MooPlugin**/
    GtkWidget  *(*create_prefs_page)    (MooPlugin      *plugin);
};

struct MooWinPluginClass
{
    GObjectClass parent_class;

    /**vtable:MooWinPlugin**/
    gboolean (*create)  (MooWinPlugin *win_plugin);
    /**vtable:MooWinPlugin**/
    void     (*destroy) (MooWinPlugin *win_plugin);
};

struct MooDocPluginClass
{
    GObjectClass parent_class;

    /**vtable:MooDocPlugin**/
    gboolean (*create)  (MooDocPlugin *doc_plugin);
    /**vtable:MooDocPlugin**/
    void     (*destroy) (MooDocPlugin *doc_plugin);
};


GType       moo_plugin_get_type         (void) G_GNUC_CONST;
GType       moo_win_plugin_get_type     (void) G_GNUC_CONST;
GType       moo_doc_plugin_get_type     (void) G_GNUC_CONST;
GType       moo_plugin_info_get_type    (void) G_GNUC_CONST;
GType       moo_plugin_params_get_type  (void) G_GNUC_CONST;

gboolean    moo_module_check_version    (guint           major,
                                         guint           minor);
void        _moo_module_version         (guint          *major,
                                         guint          *minor);

gboolean    moo_plugin_register         (const char     *id,
                                         GType           type,
                                         const MooPluginInfo *info,
                                         const MooPluginParams *params);

gboolean    moo_plugin_initialized      (MooPlugin      *plugin);
gboolean    moo_plugin_enabled          (MooPlugin      *plugin);
gboolean    moo_plugin_set_enabled      (MooPlugin      *plugin,
                                         gboolean        enabled);

gpointer    moo_plugin_lookup           (const char     *plugin_id);
gpointer    moo_win_plugin_lookup       (const char     *plugin_id,
                                         MooEditWindow  *window);
gpointer    moo_doc_plugin_lookup       (const char     *plugin_id,
                                         MooEdit        *doc);

/* list of MooPlugin*; list must be freed */
GSList     *moo_list_plugins            (void);

const char *moo_plugin_id               (MooPlugin      *plugin);

const char *moo_plugin_name             (MooPlugin      *plugin);
const char *moo_plugin_description      (MooPlugin      *plugin);
const char *moo_plugin_author           (MooPlugin      *plugin);
const char *moo_plugin_version          (MooPlugin      *plugin);

char      **moo_plugin_get_dirs         (void);
void        moo_plugin_read_dirs        (void);
void        moo_plugin_shutdown         (void);

void        moo_plugin_set_info         (MooPlugin      *plugin,
                                         MooPluginInfo  *info);
void        moo_plugin_set_doc_plugin_type (MooPlugin   *plugin,
                                         GType           type);
void        moo_plugin_set_win_plugin_type (MooPlugin   *plugin,
                                         GType           type);

MooPluginInfo *moo_plugin_info_new      (const char     *name,
                                         const char     *description,
                                         const char     *author,
                                         const char     *version);
MooPluginInfo *moo_plugin_info_copy     (MooPluginInfo  *info);
void         moo_plugin_info_free       (MooPluginInfo  *info);
MooPluginParams *moo_plugin_params_new  (gboolean        enabled,
                                         gboolean        visible);
MooPluginParams *moo_plugin_params_copy (MooPluginParams *params);
void         moo_plugin_params_free     (MooPluginParams *params);

void        _moo_window_attach_plugins  (MooEditWindow  *window);
void        _moo_window_detach_plugins  (MooEditWindow  *window);
void        _moo_doc_attach_plugins     (MooEditWindow  *window,
                                         MooEdit        *doc);
void        _moo_doc_detach_plugins     (MooEditWindow  *window,
                                         MooEdit        *doc);

void         moo_plugin_attach_prefs    (GtkWidget      *prefs_dialog);


MooPluginMeth *moo_plugin_lookup_method (gpointer        plugin,
                                         const char     *name);
GSList     *moo_plugin_list_methods     (gpointer        plugin);

void        moo_plugin_call_method      (gpointer        plugin,
                                         const char     *name,
                                         ...);
void        moo_plugin_call_method_valist (gpointer      plugin,
                                         const char     *name,
                                         va_list         var_args);
void        moo_plugin_call_methodv     (const GValue   *plugin_and_args,
                                         const char     *name,
                                         GValue         *return_value);

void        moo_plugin_method_new       (const char     *name,
                                         GType           ptype,
                                         GCallback       method,
                                         GClosureMarshal c_marshaller,
                                         GType           return_type,
                                         guint           n_params,
                                         ...);
void        moo_plugin_method_newv      (const char     *name,
                                         GType           ptype,
                                         GClosure       *closure,
                                         GClosureMarshal c_marshaller,
                                         GType           return_type,
                                         guint           n_params,
                                         const GType    *param_types);
void        moo_plugin_method_new_valist(const char     *name,
                                         GType           ptype,
                                         GClosure       *closure,
                                         GClosureMarshal c_marshaller,
                                         GType           return_type,
                                         guint           n_params,
                                         va_list         args);

MooEditWindow *moo_win_plugin_get_window    (MooWinPlugin *wplugin);
MooPlugin     *moo_win_plugin_get_plugin    (MooWinPlugin *wplugin);
MooEditWindow *moo_doc_plugin_get_window    (MooDocPlugin *dplugin);
MooEdit       *moo_doc_plugin_get_doc       (MooDocPlugin *dplugin);
MooPlugin     *moo_doc_plugin_get_plugin    (MooDocPlugin *dplugin);

G_END_DECLS

#endif /* MOO_PLUGIN_H */
