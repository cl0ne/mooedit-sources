/*
 *   ctags-plugin.c
 *
 *   Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
 *   Copyright (C) 2008      by Christian Dywan <christian@twotoasts.de>
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
#include "mooedit/mooplugin-macro.h"
#include "plugins/mooplugin-builtin.h"
#include "mooedit/mooeditwindow.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooutils-gobject.h"
#include "ctags-view.h"
#include "ctags-doc.h"
#include <gtk/gtk.h>

#define CTAGS_PLUGIN_ID "Ctags"

typedef struct {
    MooPlugin parent;
} CtagsPlugin;

typedef struct {
    MooWinPlugin parent;
    MooCtagsView *view;
    guint update_idle;
} CtagsWindowPlugin;


static gboolean
window_plugin_update (CtagsWindowPlugin *plugin)
{
    MooEditWindow *window;
    MooEdit *doc;
    MooCtagsDocPlugin *dp;
    GtkTreeModel *model;

    plugin->update_idle = 0;

    window = MOO_WIN_PLUGIN (plugin)->window;
    doc = moo_edit_window_get_active_doc (window);

    if (!doc)
    {
        gtk_tree_view_set_model (GTK_TREE_VIEW (plugin->view), NULL);
        return FALSE;
    }

    dp = moo_doc_plugin_lookup (CTAGS_PLUGIN_ID, doc);
    g_return_val_if_fail (MOO_IS_CTAGS_DOC_PLUGIN (dp), FALSE);

    model = _moo_ctags_doc_plugin_get_store (dp);
    gtk_tree_view_set_model (GTK_TREE_VIEW (plugin->view), model);
    gtk_tree_view_expand_all (GTK_TREE_VIEW (plugin->view));

    return FALSE;
}

static void
active_doc_changed (CtagsWindowPlugin *plugin)
{
    if (!plugin->update_idle)
        plugin->update_idle =
            g_idle_add_full (G_PRIORITY_LOW,
                             (GSourceFunc) window_plugin_update,
                             plugin, NULL);
}

static void
entry_activated (CtagsWindowPlugin *plugin,
                 MooCtagsEntry     *entry)
{
    MooEditWindow *window;
    MooEditView *view;

    window = MOO_WIN_PLUGIN (plugin)->window;
    view = moo_edit_window_get_active_view (window);

    if (view && entry->line >= 0)
    {
        gtk_widget_grab_focus (GTK_WIDGET (view));
        moo_text_view_move_cursor (MOO_TEXT_VIEW (view), entry->line, -1, FALSE, FALSE);
    }
}

static gboolean
ctags_window_plugin_create (CtagsWindowPlugin *plugin)
{
    GtkWidget *swin, *view;
    MooPaneLabel *label;
    MooEditWindow *window = MOO_WIN_PLUGIN (plugin)->window;

    view = _moo_ctags_view_new ();
    g_return_val_if_fail (view != NULL, FALSE);
    plugin->view = MOO_CTAGS_VIEW (view);
    g_signal_connect_swapped (view, "activate-entry",
                              G_CALLBACK (entry_activated),
                              plugin);

    swin = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (swin), GTK_WIDGET (plugin->view));
    gtk_widget_show_all (swin);

    label = moo_pane_label_new (GTK_STOCK_INDEX, NULL,
                                /* label of Ctags plugin pane */
                                C_("window-pane", "Functions"),
                                C_("window-pane", "Functions"));
    moo_edit_window_add_pane (window, CTAGS_PLUGIN_ID,
                              swin, label, MOO_PANE_POS_RIGHT);
    moo_pane_label_free (label);

    g_signal_connect_swapped (window, "notify::active-doc",
                              G_CALLBACK (active_doc_changed),
                              plugin);
    active_doc_changed (plugin);

    return TRUE;
}

static void
ctags_window_plugin_destroy (CtagsWindowPlugin *plugin)
{
    MooEditWindow *window = MOO_WIN_PLUGIN(plugin)->window;

    moo_edit_window_remove_pane (window, CTAGS_PLUGIN_ID);

    g_signal_handlers_disconnect_by_func (window,
                                          (gpointer) active_doc_changed,
                                          plugin);
    if (plugin->update_idle)
        g_source_remove (plugin->update_idle);
    plugin->update_idle = 0;
}


static gboolean
ctags_plugin_init (G_GNUC_UNUSED CtagsPlugin *plugin)
{
    return TRUE;
}

static void
ctags_plugin_deinit (G_GNUC_UNUSED CtagsPlugin *plugin)
{
}


MOO_PLUGIN_DEFINE_INFO (ctags, "Ctags", "Shows functions in the open document",
                        "Yevgen Muntyan <emuntyan@users.sourceforge.net>\n"
                        "Christian Dywan <christian@twotoasts.de>",
                        MOO_VERSION)
MOO_WIN_PLUGIN_DEFINE (Ctags, ctags)
MOO_PLUGIN_DEFINE (Ctags, ctags,
                   NULL, NULL, NULL, NULL, NULL,
                   ctags_window_plugin_get_type (),
                   MOO_TYPE_CTAGS_DOC_PLUGIN)

gboolean
_moo_ctags_plugin_init (void)
{
    MooPluginParams params = {FALSE, TRUE};
    return moo_plugin_register (CTAGS_PLUGIN_ID,
                                ctags_plugin_get_type (),
                                &ctags_plugin_info,
                                &params);
}
