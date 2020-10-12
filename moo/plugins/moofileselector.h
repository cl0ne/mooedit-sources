/*
 *   moofileselector.h
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

#pragma once

#include "mooedit/mooplugin.h"
#include "moofileview/moofileview-impl.h"

#define MOO_FILE_SELECTOR_PLUGIN_ID "FileSelector"

#define MOO_TYPE_FILE_SELECTOR              (_moo_file_selector_get_type ())
#define MOO_FILE_SELECTOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_FILE_SELECTOR, MooFileSelector))
#define MOO_FILE_SELECTOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_FILE_SELECTOR, MooFileSelectorClass))
#define MOO_IS_FILE_SELECTOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_FILE_SELECTOR))
#define MOO_IS_FILE_SELECTOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_FILE_SELECTOR))
#define MOO_FILE_SELECTOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_FILE_SELECTOR, MooFileSelectorClass))


typedef struct _MooFileSelector      MooFileSelector;
typedef struct _MooFileSelectorClass MooFileSelectorClass;

struct _MooFileSelector
{
    MooFileView base;

    MooEditWindow *window;

    GtkTargetList *targets;
    gboolean waiting_for_tab;
};

struct _MooFileSelectorClass
{
    MooFileViewClass base_class;
};


GType       _moo_file_selector_get_type     (void) G_GNUC_CONST;
GtkWidget  *_moo_file_selector_prefs_page   (MooPlugin  *plugin);
void        _moo_file_selector_update_tools (MooPlugin  *plugin);


#define moo_file_selector_plugin_get_widget(window, filesel)                    \
G_STMT_START {                                                                  \
    gpointer result__ = NULL;                                                   \
    gpointer plugin__ = moo_plugin_lookup (MOO_FILE_SELECTOR_PLUGIN_ID);        \
                                                                                \
    if (plugin__)                                                               \
    {                                                                           \
        moo_plugin_call_method (plugin__, "get-widget", window, &result__);     \
        filesel = result__;                                                     \
        if (result__)                                                           \
            g_object_unref (result__);                                          \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        g_critical ("plugin %s is not registered", MOO_FILE_SELECTOR_PLUGIN_ID);\
        filesel = NULL;                                                         \
    }                                                                           \
} G_STMT_END
