/*
 *   moomenu.c
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

#include "mooutils/moomenu.h"
#include "marshals.h"
#include "mooutils/mooutils-misc.h"
#include <gdk/gdkkeysyms.h>

static gboolean moo_menu_key_press_event    (GtkWidget      *widget,
                                             GdkEventKey    *event);
static gboolean moo_menu_key_release_event  (GtkWidget      *widget,
                                             GdkEventKey    *event);

enum {
    ALTERNATE_TOGGLED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE (MooMenu, moo_menu, GTK_TYPE_MENU)

static void
moo_menu_class_init (MooMenuClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    widget_class->key_press_event = moo_menu_key_press_event;
    widget_class->key_release_event = moo_menu_key_release_event;

    signals[ALTERNATE_TOGGLED] =
            g_signal_new ("alternate-toggled",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooMenuClass, alternate_toggled),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);
}

static void
moo_menu_init (G_GNUC_UNUSED MooMenu *menu)
{
}

static void
moo_menu_key_event (GtkWidget   *widget,
                    GdkEventKey *event)
{
    switch (event->keyval)
    {
        case GDK_Shift_L:
        case GDK_Shift_R:
        case GDK_Shift_Lock:
            g_signal_emit (widget, signals[ALTERNATE_TOGGLED], 0);
            break;
    }
}

static gboolean
moo_menu_key_press_event (GtkWidget   *widget,
                          GdkEventKey *event)
{
    moo_menu_key_event (widget, event);

    if (GTK_WIDGET_CLASS (moo_menu_parent_class)->key_press_event)
        return GTK_WIDGET_CLASS (moo_menu_parent_class)->key_press_event (widget, event);
    else
        return FALSE;
}

static gboolean
moo_menu_key_release_event (GtkWidget   *widget,
                            GdkEventKey *event)
{
    moo_menu_key_event (widget, event);

    if (GTK_WIDGET_CLASS (moo_menu_parent_class)->key_release_event)
        return GTK_WIDGET_CLASS (moo_menu_parent_class)->key_release_event (widget, event);
    else
        return FALSE;
}

GtkWidget *
moo_menu_new (void)
{
    return GTK_WIDGET (g_object_new (MOO_TYPE_MENU, (const char*) NULL));
}
