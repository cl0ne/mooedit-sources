/*
 *   moolinklabel.c
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

#include "moolinklabel.h"
#include "marshals.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/moocompat.h"
#include <gtk/gtk.h>


struct _MooLinkLabelPrivate {
    char *text;
    char *url;
    GdkWindow *window;
};


G_DEFINE_TYPE (MooLinkLabel, _moo_link_label, GTK_TYPE_LABEL)

enum {
    PROP_0,
    PROP_TEXT,
    PROP_URL
};

enum {
    ACTIVATE,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};


static void
_moo_link_label_init (MooLinkLabel *label)
{
    label->priv = g_new0 (MooLinkLabelPrivate, 1);
}


static void
set_cursor (GtkWidget *widget,
            gboolean   hand)
{
    MooLinkLabel *label = MOO_LINK_LABEL (widget);

    if (!GTK_WIDGET_REALIZED (widget))
        return;

    if (hand)
    {
        GdkCursor *cursor = gdk_cursor_new (GDK_HAND2);
        gdk_window_set_cursor (label->priv->window, cursor);
        gdk_cursor_unref (cursor);
    }
    else
    {
        gdk_window_set_cursor (label->priv->window, NULL);
    }
}


static void
moo_link_label_realize (GtkWidget *widget)
{
    GdkWindowAttr attributes;
    gint attributes_mask;
    MooLinkLabel *label = MOO_LINK_LABEL (widget);

    GTK_WIDGET_CLASS(_moo_link_label_parent_class)->realize (widget);

    widget = GTK_WIDGET (label);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass = GDK_INPUT_ONLY;
    attributes.override_redirect = TRUE;
    attributes.event_mask = gtk_widget_get_events (widget) |
                                            GDK_BUTTON_PRESS_MASK |
                                            GDK_BUTTON_RELEASE_MASK |
                                            GDK_BUTTON_MOTION_MASK;

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_NOREDIR;

    label->priv->window = gdk_window_new (widget->window, &attributes, attributes_mask);
    gdk_window_set_user_data (label->priv->window, widget);

    set_cursor (widget, label->priv->url && label->priv->text);
}


static void
moo_link_label_unrealize (GtkWidget *widget)
{
    MooLinkLabel *label = MOO_LINK_LABEL (widget);

    gdk_window_set_user_data (label->priv->window, NULL);
    gdk_window_destroy (label->priv->window);
    label->priv->window = NULL;

    GTK_WIDGET_CLASS(_moo_link_label_parent_class)->unrealize (widget);
}


static void
moo_link_label_size_allocate (GtkWidget     *widget,
                              GtkAllocation *allocation)
{
    MooLinkLabel *label = MOO_LINK_LABEL (widget);

    GTK_WIDGET_CLASS(_moo_link_label_parent_class)->size_allocate (widget, allocation);

    if (label->priv->window)
        gdk_window_move_resize (label->priv->window,
                                allocation->x,
                                allocation->y,
                                allocation->width,
                                allocation->height);
}


static void
moo_link_label_map (GtkWidget *widget)
{
    MooLinkLabel *label = MOO_LINK_LABEL (widget);

    GTK_WIDGET_CLASS(_moo_link_label_parent_class)->map (widget);

    if (label->priv->window)
        gdk_window_show (label->priv->window);
}


static void
moo_link_label_unmap (GtkWidget *widget)
{
    MooLinkLabel *label = MOO_LINK_LABEL (widget);

    if (label->priv->window)
        gdk_window_hide (label->priv->window);

    GTK_WIDGET_CLASS(_moo_link_label_parent_class)->unmap (widget);
}


static void
moo_link_label_activate (G_GNUC_UNUSED MooLinkLabel *label,
                         const char   *url)
{
    moo_open_url (url);
}


static void
open_activated (MooLinkLabel *label)
{
    g_return_if_fail (label->priv->url != NULL);
    g_signal_emit (label, signals[ACTIVATE], 0, label->priv->url);
}

static void
copy_activated (MooLinkLabel *label)
{
    GtkClipboard *clipboard;

    g_return_if_fail (label->priv->url != NULL);

    clipboard = gtk_widget_get_clipboard (GTK_WIDGET (label), GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text (clipboard, label->priv->url, -1);
}

static gboolean
moo_link_label_button_press (GtkWidget      *widget,
                             GdkEventButton *event)
{
    MooLinkLabel *label = MOO_LINK_LABEL (widget);
    GtkWidget *menu, *item, *image;

    if (event->button == 1)
    {
        if (label->priv->url)
            g_signal_emit (label, signals[ACTIVATE], 0, label->priv->url);
        return TRUE;
    }

    if (event->button != 3 || !label->priv->url)
        return FALSE;

    menu = gtk_menu_new ();
    g_object_ref_sink (menu);

    item = gtk_image_menu_item_new_with_label ("Copy Link");
    image = gtk_image_new_from_stock (GTK_STOCK_COPY, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
    g_signal_connect_swapped (item, "activate", G_CALLBACK (copy_activated), label);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    gtk_widget_show_all (item);

    item = gtk_image_menu_item_new_with_label ("Open Link");
    image = gtk_image_new_from_stock (GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
    g_signal_connect_swapped (item, "activate", G_CALLBACK (open_activated), label);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    gtk_widget_show_all (item);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
                    event->button, event->time);
    g_object_unref (menu);

    return TRUE;
}


static void
moo_link_label_finalize (GObject *object)
{
    MooLinkLabel *label = MOO_LINK_LABEL (object);

    g_free (label->priv->url);
    g_free (label->priv->text);
    g_free (label->priv);

    G_OBJECT_CLASS(_moo_link_label_parent_class)->finalize (object);
}


static void
moo_link_label_set_property (GObject        *object,
                             guint           param_id,
                             const GValue   *value,
                             GParamSpec     *pspec)
{
    MooLinkLabel *label = MOO_LINK_LABEL (object);

    switch (param_id)
    {
        case PROP_TEXT:
            _moo_link_label_set_text (label, g_value_get_string (value));
            break;

        case PROP_URL:
            _moo_link_label_set_url (label, g_value_get_string (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    }
}


static void
moo_link_label_get_property (GObject        *object,
                             guint           param_id,
                             GValue         *value,
                             GParamSpec     *pspec)
{
    MooLinkLabel *label = MOO_LINK_LABEL (object);

    switch (param_id)
    {
        case PROP_TEXT:
            g_value_set_string (value, label->priv->text);
            break;

        case PROP_URL:
            g_value_set_string (value, label->priv->url);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    }
}


static void
_moo_link_label_class_init (MooLinkLabelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->finalize = moo_link_label_finalize;
    object_class->set_property = moo_link_label_set_property;
    object_class->get_property = moo_link_label_get_property;

    widget_class->button_press_event = moo_link_label_button_press;
    widget_class->realize = moo_link_label_realize;
    widget_class->unrealize = moo_link_label_unrealize;
    widget_class->size_allocate = moo_link_label_size_allocate;
    widget_class->map = moo_link_label_map;
    widget_class->unmap = moo_link_label_unmap;

    klass->activate = moo_link_label_activate;

    g_object_class_install_property (object_class,
                                     PROP_TEXT,
                                     g_param_spec_string ("text",
                                             "text",
                                             "text",
                                             NULL,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_URL,
                                     g_param_spec_string ("url",
                                             "url",
                                             "url",
                                             NULL,
                                             (GParamFlags) G_PARAM_READWRITE));

    signals[ACTIVATE] =
            g_signal_new ("activate",
                          G_TYPE_FROM_CLASS (object_class),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooLinkLabelClass, activate),
                          NULL, NULL,
                          _moo_marshal_VOID__STRING,
                          G_TYPE_NONE, 1,
                          G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE);
}


static void
moo_link_label_update (MooLinkLabel *label)
{
    if (!label->priv->text || !label->priv->url)
    {
        gtk_label_set_text (GTK_LABEL (label), label->priv->text ? label->priv->text : "");
        set_cursor (GTK_WIDGET (label), FALSE);
    }
    else
    {
        char *markup;
        const char *color = NULL;
        GValue val;
        val.g_type = 0;
        g_value_init (&val, GDK_TYPE_COLOR);
        gtk_widget_ensure_style (GTK_WIDGET (label));
        gtk_widget_style_get_property (GTK_WIDGET (label), "link-color", &val);
        color = _moo_value_convert_to_string (&val);
        g_value_unset (&val);
        if (!color)
            color = "#0000EE";
        markup = g_markup_printf_escaped ("<span foreground=\"%s\">%s</span>",
                                          color, label->priv->text);
        gtk_label_set_markup (GTK_LABEL (label), markup);
        set_cursor (GTK_WIDGET (label), TRUE);
        g_free (markup);
    }
}


void
_moo_link_label_set_url (MooLinkLabel   *label,
                         const char     *url)
{
    g_return_if_fail (MOO_IS_LINK_LABEL (label));
    g_free (label->priv->url);
    label->priv->url = url && *url ? g_strdup (url) : NULL;
    moo_link_label_update (label);
    g_object_notify (G_OBJECT (label), "url");
}


void
_moo_link_label_set_text (MooLinkLabel   *label,
                          const char     *text)
{
    g_return_if_fail (MOO_IS_LINK_LABEL (label));
    g_free (label->priv->text);
    label->priv->text = text && *text ? g_strdup (text) : NULL;
    moo_link_label_update (label);
    g_object_notify (G_OBJECT (label), "text");
}
