/*
 *   moocommanddisplay.c
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

#include "moocommanddisplay.h"
#include "moocommand-private.h"
#include "mooutils/mooutils-misc.h"
#include <string.h>


typedef struct {
    MooCommandData *data;
    MooCommandFactory *factory;
    gboolean changed;
} Data;

struct _MooCommandDisplay {
    MooTreeHelper base;

    GtkComboBox *factory_combo;
    GtkNotebook *notebook;

    Data *data;
    int n_factories;
    int active;
    int original;
};

struct _MooCommandDisplayClass {
    MooTreeHelperClass base_class;
};


G_DEFINE_TYPE (MooCommandDisplay, _moo_command_display, MOO_TYPE_TREE_HELPER)


static void     combo_changed   (MooCommandDisplay *display);


static void
block_combo (MooCommandDisplay *display)
{
    g_signal_handlers_block_by_func (display->factory_combo,
                                     (gpointer) combo_changed,
                                     display);
}


static void
unblock_combo (MooCommandDisplay *display)
{
    g_signal_handlers_unblock_by_func (display->factory_combo,
                                       (gpointer) combo_changed,
                                       display);
}


static void
combo_changed (MooCommandDisplay *display)
{
    GtkWidget *widget;
    int index;
    MooCommandFactory *factory;

    index = gtk_combo_box_get_active (display->factory_combo);
    g_return_if_fail (index >= 0);

    if (index == display->active)
        return;

    if (display->active >= 0)
    {
        widget = gtk_notebook_get_nth_page (display->notebook, display->active);
        if (_moo_command_factory_save_data (display->data[display->active].factory, widget,
                                            display->data[display->active].data))
            display->data[display->active].changed = TRUE;
    }

    display->active = index;
    factory = display->data[index].factory;
    g_return_if_fail (factory != NULL);

    if (!display->data[index].data)
        display->data[index].data = moo_command_data_new (factory->n_keys);

    gtk_notebook_set_current_page (display->notebook, index);
    widget = gtk_notebook_get_nth_page (display->notebook, index);
    _moo_command_factory_load_data (factory, widget, display->data[index].data);
}


static void
combo_set_active (MooCommandDisplay  *display,
                  int                 index)
{
    block_combo (display);
    gtk_combo_box_set_active (display->factory_combo, index);
    unblock_combo (display);

    if (index < 0)
    {
        gtk_widget_hide (GTK_WIDGET (display->notebook));
    }
    else
    {
        gtk_notebook_set_current_page (display->notebook, index);
        gtk_widget_show (GTK_WIDGET (display->notebook));
    }
}


static int
combo_find_factory (MooCommandDisplay *display,
                    MooCommandFactory *factory)
{
    int i;

    g_return_val_if_fail (!factory || MOO_IS_COMMAND_FACTORY (factory), -1);

    if (!factory)
        return -1;

    for (i = 0; i < display->n_factories; ++i)
        if (display->data[i].factory == factory)
            return i;

    g_return_val_if_reached (-1);
}


void
_moo_command_display_set (MooCommandDisplay  *display,
                          MooCommandFactory  *factory,
                          MooCommandData     *data)
{
    int index;
    GtkWidget *widget;

    g_return_if_fail (MOO_IS_COMMAND_DISPLAY (display));
    g_return_if_fail (!factory || MOO_IS_COMMAND_FACTORY (factory));
    g_return_if_fail (!factory == !data);

    for (index = 0; index < display->n_factories; ++index)
    {
        display->data[index].changed = FALSE;
        if (display->data[index].data)
            moo_command_data_unref (display->data[index].data);
        display->data[index].data = NULL;
    }

    index = combo_find_factory (display, factory);

    display->active = index;
    display->original = index;
    combo_set_active (display, index);

    if (index >= 0)
    {
        display->data[index].data = moo_command_data_ref (data);
        widget = gtk_notebook_get_nth_page (display->notebook, index);
        _moo_command_factory_load_data (factory, widget, data);
    }
}


gboolean
_moo_command_display_get (MooCommandDisplay  *display,
                          MooCommandFactory **factory_p,
                          MooCommandData    **data_p)
{
    GtkWidget *widget;
    Data *data;

    g_return_val_if_fail (MOO_IS_COMMAND_DISPLAY (display), FALSE);

    if (display->active < 0)
        return FALSE;

    data = &display->data[display->active];
    widget = gtk_notebook_get_nth_page (display->notebook, display->active);

    if (_moo_command_factory_save_data (data->factory, widget, data->data))
        data->changed = TRUE;

    if (display->active == display->original && !data->changed)
        return FALSE;

    *factory_p = data->factory;
    *data_p = data->data;
    data->changed = FALSE;
    display->original = display->active;

    return TRUE;
}


static void
init_factory_combo (MooCommandDisplay *display,
                    GtkComboBox       *combo,
                    GtkNotebook       *notebook)
{
    GtkListStore *store;
    GSList *factories;
    GtkCellRenderer *cell;

    g_return_if_fail (MOO_IS_COMMAND_DISPLAY (display));
    g_return_if_fail (GTK_IS_COMBO_BOX (combo));
    g_return_if_fail (GTK_IS_NOTEBOOK (notebook));

    display->factory_combo = combo;
    display->notebook = notebook;

    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (display->factory_combo), cell, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (display->factory_combo), cell, "text", 0, nullptr);

    store = gtk_list_store_new (1, G_TYPE_STRING);

    factories = moo_command_list_factories ();
    display->active = -1;
    display->original = -1;
    display->n_factories = 0;
    display->data = g_new0 (Data, g_slist_length (factories));

    while (factories)
    {
        GtkTreeIter iter;
        MooCommandFactory *cmd_factory;
        GtkWidget *widget;

        cmd_factory = (MooCommandFactory*) factories->data;
        widget = _moo_command_factory_create_widget (cmd_factory);

        if (widget)
        {
            gtk_widget_show (widget);
            gtk_notebook_append_page (display->notebook, widget, NULL);

            gtk_list_store_append (store, &iter);
            gtk_list_store_set (store, &iter, 0, cmd_factory->display_name, -1);

            display->data[display->n_factories].factory = cmd_factory;
            display->n_factories++;
        }

        factories = g_slist_delete_link (factories, factories);
    }

    gtk_combo_box_set_model (display->factory_combo, GTK_TREE_MODEL (store));

    g_signal_connect_swapped (display->factory_combo, "changed",
                              G_CALLBACK (combo_changed), display);

    g_object_unref (store);
}


MooCommandDisplay *
_moo_command_display_new (GtkComboBox        *factory_combo,
                          GtkNotebook        *notebook,
                          GtkWidget          *treeview,
                          GtkWidget          *new_btn,
                          GtkWidget          *delete_btn,
                          GtkWidget          *up_btn,
                          GtkWidget          *down_btn)
{
    MooCommandDisplay *display;

    display = (MooCommandDisplay*) g_object_new (MOO_TYPE_COMMAND_DISPLAY, (const char*) NULL);
    _moo_tree_helper_connect (MOO_TREE_HELPER (display), treeview,
                              new_btn, delete_btn, up_btn, down_btn);
    init_factory_combo (display, factory_combo, notebook);

    g_object_ref_sink (display);
    return display;
}


static void
moo_command_display_dispose (GObject *object)
{
    MooCommandDisplay *display = MOO_COMMAND_DISPLAY (object);

    if (display->data)
    {
        int i;

        for (i = 0; i < display->n_factories; ++i)
            if (display->data[i].data)
                moo_command_data_unref (display->data[i].data);

        g_free (display->data);
        display->data = NULL;
    }

    G_OBJECT_CLASS (_moo_command_display_parent_class)->dispose (object);
}


static void
_moo_command_display_init (G_GNUC_UNUSED MooCommandDisplay *display)
{
}


static void
_moo_command_display_class_init (MooCommandDisplayClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = moo_command_display_dispose;
}
