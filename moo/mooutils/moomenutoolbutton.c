/*
 *   moomenutoolbutton.c
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

/**
 * class:MooMenuToolButton: (parent GtkToggleToolButton) (moo.private 1)
 **/

#include "mooutils/moomenutoolbutton.h"
#include "mooutils/mooutils-misc.h"
#include <gtk/gtk.h>


enum {
    PROP_0,
    PROP_MENU
};


static void moo_menu_tool_button_class_init (MooMenuToolButtonClass *klass);
static void moo_menu_tool_button_init       (MooMenuToolButton      *button);

static void moo_menu_tool_button_destroy    (GtkObject              *object);
static void moo_menu_tool_button_toggled    (GtkToggleToolButton    *button);


G_DEFINE_TYPE(MooMenuToolButton, moo_menu_tool_button, GTK_TYPE_TOGGLE_TOOL_BUTTON)


static void
moo_menu_tool_button_class_init (MooMenuToolButtonClass *klass)
{
    GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
    GtkToggleToolButtonClass *toggle_class = GTK_TOGGLE_TOOL_BUTTON_CLASS (klass);
    gtkobject_class->destroy = moo_menu_tool_button_destroy;
    toggle_class->toggled = moo_menu_tool_button_toggled;
}


static void
moo_menu_tool_button_init (G_GNUC_UNUSED MooMenuToolButton *button)
{
}


static void
moo_menu_tool_button_destroy (GtkObject *object)
{
    MooMenuToolButton *button = MOO_MENU_TOOL_BUTTON (object);

    if (button->menu)
        gtk_widget_destroy (button->menu);
    button->menu = NULL;

    GTK_OBJECT_CLASS (moo_menu_tool_button_parent_class)->destroy (object);
}


GtkWidget *
moo_menu_tool_button_new (void)
{
    return GTK_WIDGET (g_object_new (MOO_TYPE_MENU_TOOL_BUTTON, (const char*) NULL));
}


static void
menu_position_func (G_GNUC_UNUSED GtkMenu *menu,
                    int               *x,
                    int               *y,
                    gboolean          *push_in,
                    GtkWidget         *button)
{
    GtkRequisition req;

    gdk_window_get_origin (button->window, x, y);
    gtk_widget_size_request (button, &req);

    *x += button->allocation.x + button->allocation.width - req.width;
    *y += button->allocation.y + button->allocation.height;

    *push_in = TRUE;
}


static void
moo_menu_tool_button_toggled (GtkToggleToolButton *button)
{
    MooMenuToolButton *menu_button = MOO_MENU_TOOL_BUTTON (button);

    if (!gtk_toggle_tool_button_get_active (button))
        return;

    if (!menu_button->menu)
        return;

    gtk_menu_popup (GTK_MENU (menu_button->menu), NULL, NULL,
                    (GtkMenuPositionFunc) menu_position_func,
                    button, 0, gtk_get_current_event_time ());
}


static void
menu_deactivated (GtkToggleToolButton *button)
{
    gtk_toggle_tool_button_set_active (button, FALSE);
}


static void
menu_destroyed (MooMenuToolButton *button)
{
    if (button->menu)
    {
        GtkWidget *menu = button->menu;
        button->menu = NULL;
        g_object_unref (menu);
    }
}


void
moo_menu_tool_button_set_menu (MooMenuToolButton  *button,
                               GtkWidget          *menu)
{
    g_return_if_fail (MOO_IS_MENU_TOOL_BUTTON (button));
    g_return_if_fail (!menu || GTK_IS_MENU (menu));

    if (button->menu == menu)
        return;

    if (button->menu)
    {
        g_signal_handlers_disconnect_by_func (button->menu,
                                              (gpointer) menu_deactivated,
                                              button);
        g_signal_handlers_disconnect_by_func (button->menu,
                                              (gpointer) menu_destroyed,
                                              button);
        g_object_unref (button->menu);
    }

    if (menu)
    {
        g_object_ref_sink (menu);
        g_signal_connect_swapped (menu, "deactivate",
                                  G_CALLBACK (menu_deactivated),
                                  button);
        g_signal_connect_swapped (menu, "destroy",
                                  G_CALLBACK (menu_destroyed),
                                  button);
    }

    button->menu = menu;
}


GtkWidget*
moo_menu_tool_button_get_menu (MooMenuToolButton  *button)
{
    g_return_val_if_fail (MOO_IS_MENU_TOOL_BUTTON (button), NULL);
    return button->menu;
}
