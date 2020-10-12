/*
 *   moopane.c
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
 * class:MooPane: (parent GtkObject) (moo.lua 0) (moo.private 1)
 **/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "marshals.h"
#include "moopaned.h"
#include "moo-pixbufs.h"

#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#define SPACING_IN_BUTTON 4
#define OPEN_PANE_TIMEOUT 200

#include "mooutils-misc.h"
#include "moocompat.h"
#include "moohelp.h"
#include "mooutils-gobject.h"
#include "mooi18n.h"

struct _MooPane {
    GtkObject base;

    char         *id;
    MooPaned     *parent;
    GtkWidget    *child;

    GtkWidget    *child_holder;
    MooPaneLabel *label;
    GtkWidget    *frame;
    GtkWidget    *handle;
    GtkWidget    *small_handle;
    GtkWidget    *button;
    GtkWidget    *label_widget;
    GtkWidget    *icon_widget;
    GtkWidget    *sticky_button;
    GtkWidget    *detach_button;
    GtkWidget    *close_button;

    GtkWidget    *frame_label_embedded;
    GtkWidget    *frame_label_window;
    char         *frame_label_text;
    gboolean      frame_label_markup;

    /* XXX weak pointer */
    gpointer      focus_child;

    GtkWidget    *window;
    GtkWidget    *keep_on_top_button;
    GtkWidget    *window_child_holder;

    MooPaneParams *params;

    guint         open_timeout;
    guint         button_highlight : 1;
    guint         drag_dest_enabled : 1;

    guint         detachable : 1;
    guint         removable : 1;

    guint         params_changed_blocked : 1;
};

struct _MooPaneClass {
    GtkObjectClass base_class;
    gboolean (*remove) (MooPane *pane);
};

G_DEFINE_TYPE (MooPane, moo_pane, GTK_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_ID,
    PROP_LABEL,
    PROP_PARAMS,
    PROP_DETACHABLE,
    PROP_REMOVABLE
};

enum {
    REMOVE,
    NUM_SIGNALS
};

static guint signals[NUM_SIGNALS];


static void
set_pane_window_icon_and_title (MooPane *pane)
{
    if (pane->window && pane->label)
    {
        if (pane->label->icon_pixbuf)
            gtk_window_set_icon (GTK_WINDOW (pane->window), pane->label->icon_pixbuf);
        else if (pane->label->icon_stock_id)
            _moo_window_set_icon_from_stock (GTK_WINDOW (pane->window), pane->label->icon_stock_id);

        if (pane->label->window_title)
            gtk_window_set_title (GTK_WINDOW (pane->window), pane->label->window_title);
        else
            gtk_window_set_title (GTK_WINDOW (pane->window), pane->label->label);
    }
}

static void
update_label_widgets (MooPane *pane)
{
    if (pane->label && pane->label_widget)
    {
        gtk_label_set_text (GTK_LABEL (pane->label_widget), pane->label->label);
        g_object_set (pane->label_widget, "visible", pane->label->label != NULL, NULL);
    }

    if (pane->label && pane->icon_widget)
    {
        if (pane->label->icon_pixbuf)
            gtk_image_set_from_pixbuf (GTK_IMAGE (pane->icon_widget),
                                       pane->label->icon_pixbuf);
        else if (pane->label->icon_stock_id)
            gtk_image_set_from_stock (GTK_IMAGE (pane->icon_widget),
                                      pane->label->icon_stock_id,
                                      GTK_ICON_SIZE_MENU);

        g_object_set (pane->icon_widget, "visible",
                      pane->label->icon_pixbuf || pane->label->icon_stock_id,
                      NULL);
    }

    set_pane_window_icon_and_title (pane);
}

/**
 * moo_pane_set_label:
 **/
void
moo_pane_set_label (MooPane      *pane,
                    MooPaneLabel *label)
{
    MooPaneLabel *tmp;

    g_return_if_fail (MOO_IS_PANE (pane));
    g_return_if_fail (label != NULL);

    tmp = pane->label;
    pane->label = moo_pane_label_copy (label);
    moo_pane_label_free (tmp);

    update_label_widgets (pane);

    g_object_notify (G_OBJECT (pane), "label");
}


/**
 * moo_pane_get_params:
 **/
MooPaneParams *
moo_pane_get_params (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), NULL);
    return moo_pane_params_copy (pane->params);
}

/**
 * moo_pane_get_label:
 **/
MooPaneLabel *
moo_pane_get_label (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), NULL);
    return moo_pane_label_copy (pane->label);
}


/**
 * moo_pane_set_params:
 **/
void
moo_pane_set_params (MooPane       *pane,
                     MooPaneParams *params)
{
    MooPaneParams *old_params;

    g_return_if_fail (MOO_IS_PANE (pane));
    g_return_if_fail (params != NULL);

    old_params = pane->params;
    pane->params = moo_pane_params_copy (params);

    if (old_params->detached != params->detached)
    {
        pane->params->detached = old_params->detached;

        if (old_params->detached)
            moo_paned_attach_pane (pane->parent, pane);
        else
            moo_paned_detach_pane (pane->parent, pane);
    }

    moo_pane_params_free (old_params);

    g_object_notify (G_OBJECT (pane), "params");
}


/**
 * moo_pane_set_detachable:
 **/
void
moo_pane_set_detachable (MooPane  *pane,
                         gboolean  detachable)
{
    g_return_if_fail (MOO_IS_PANE (pane));

    if (detachable == pane->detachable)
        return;

    pane->detachable = detachable != 0;

    if (pane->params->detached && !detachable)
        moo_paned_attach_pane (pane->parent, pane);

    if (pane->detach_button)
        g_object_set (pane->detach_button, "visible", pane->detachable, NULL);

    g_object_notify (G_OBJECT (pane), "detachable");
}


/**
 * moo_pane_set_removable:
 **/
void
moo_pane_set_removable (MooPane  *pane,
                        gboolean  removable)
{
    g_return_if_fail (MOO_IS_PANE (pane));

    if (removable == pane->removable)
        return;

    pane->removable = removable != 0;

    if (pane->close_button)
        g_object_set (pane->close_button, "visible", pane->removable, NULL);

    g_object_notify (G_OBJECT (pane), "removable");
}


/**
 * moo_pane_get_detachable:
 **/
gboolean
moo_pane_get_detachable (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), FALSE);
    return pane->detachable;
}

/**
 * moo_pane_get_removable:
 **/
gboolean
moo_pane_get_removable (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), FALSE);
    return pane->removable;
}


static void
moo_pane_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
    MooPane *pane = MOO_PANE (object);

    switch (prop_id)
    {
        case PROP_LABEL:
            moo_pane_set_label (pane, g_value_get_boxed (value));
            break;
        case PROP_PARAMS:
            moo_pane_set_params (pane, g_value_get_boxed (value));
            break;
        case PROP_DETACHABLE:
            moo_pane_set_detachable (pane, g_value_get_boolean (value));
            break;
        case PROP_REMOVABLE:
            moo_pane_set_removable (pane, g_value_get_boolean (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
moo_pane_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
    MooPane *pane = MOO_PANE (object);

    switch (prop_id)
    {
        case PROP_ID:
            g_value_set_string (value, pane->id);
            break;
        case PROP_LABEL:
            g_value_set_boxed (value, pane->label);
            break;
        case PROP_PARAMS:
            g_value_set_boxed (value, pane->params);
            break;
        case PROP_DETACHABLE:
            g_value_set_boolean (value, pane->detachable != 0);
            break;
        case PROP_REMOVABLE:
            g_value_set_boolean (value, pane->removable != 0);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
moo_pane_init (MooPane *pane)
{
    pane->detachable = TRUE;
    pane->removable = TRUE;
    pane->params = moo_pane_params_new (NULL, FALSE, FALSE, FALSE);

    pane->id = NULL;
    pane->label = NULL;
    pane->child = NULL;

    pane->child_holder = NULL;
    pane->frame = NULL;
    pane->handle = NULL;
    pane->small_handle = NULL;
    pane->button = NULL;
    pane->label_widget = NULL;
    pane->icon_widget = NULL;
    pane->sticky_button = NULL;
    pane->detach_button = NULL;
    pane->close_button = NULL;
    pane->focus_child = NULL;

    pane->window = NULL;
    pane->keep_on_top_button = NULL;
    pane->window_child_holder = NULL;
}

static void
moo_pane_finalize (GObject *object)
{
    MooPane *pane = MOO_PANE (object);

    g_free (pane->id);
    moo_pane_label_free (pane->label);
    moo_pane_params_free (pane->params);

    G_OBJECT_CLASS (moo_pane_parent_class)->finalize (object);
}

static void
moo_pane_dispose (GObject *object)
{
    MooPane *pane = MOO_PANE (object);

    if (pane->child)
    {
        GtkWidget *tmp = pane->child;
        pane->child = NULL;
        g_object_unref (tmp);
    }

    if (pane->frame)
    {
        gtk_widget_unparent (pane->frame);
        pane->frame = NULL;
    }

    if (pane->window)
    {
        gtk_widget_destroy (pane->window);
        pane->window = NULL;
    }

    if (pane->open_timeout)
    {
        g_source_remove (pane->open_timeout);
        pane->open_timeout = 0;
    }

    G_OBJECT_CLASS (moo_pane_parent_class)->dispose (object);
}

static void
moo_pane_class_init (MooPaneClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = moo_pane_set_property;
    gobject_class->get_property = moo_pane_get_property;
    gobject_class->finalize = moo_pane_finalize;
    gobject_class->dispose = moo_pane_dispose;

    g_object_class_install_property (gobject_class, PROP_ID,
        g_param_spec_string ("id", "id", "id",
                             NULL, G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_LABEL,
        g_param_spec_boxed ("label", "label", "label",
                            MOO_TYPE_PANE_LABEL, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_PARAMS,
        g_param_spec_boxed ("params", "params", "params",
                            MOO_TYPE_PANE_PARAMS, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_DETACHABLE,
        g_param_spec_boolean ("detachable", "detachable", "detachable",
                              TRUE, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_REMOVABLE,
        g_param_spec_boolean ("removable", "removable", "removable",
                              TRUE, (GParamFlags) G_PARAM_READWRITE));

    signals[REMOVE] =
            g_signal_new ("remove",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooPaneClass, remove),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOL__VOID,
                          G_TYPE_BOOLEAN, 0);
}


static void
close_button_clicked (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    if (pane->parent)
        _moo_pane_try_remove (pane);
}

static void
hide_button_clicked (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    if (pane->parent)
        moo_paned_hide_pane (pane->parent);
}

static void
attach_button_clicked (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    if (pane->parent)
        _moo_paned_attach_pane (pane->parent, pane);
}

static void
detach_button_clicked (MooPane *pane)
{
    moo_paned_detach_pane (pane->parent, pane);
}

static void
sticky_button_toggled (GtkToggleButton *button,
                       MooPane         *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    moo_paned_set_sticky_pane (pane->parent, gtk_toggle_button_get_active (button));
}


static void
update_sticky_button (MooPane *pane)
{
    if (pane->parent)
    {
        gboolean sticky, active;
        g_object_get (pane->parent, "sticky-pane", &sticky, NULL);
        g_object_get (pane->sticky_button, "active", &active, NULL);
        if (active != sticky)
            g_object_set (pane->sticky_button, "active", sticky, NULL);
    }
}


/**
 * moo_pane_set_frame_markup:
 *
 * @pane:
 * @markup: (type const-utf8) (allow-none):
 **/
void
moo_pane_set_frame_markup (MooPane    *pane,
                           const char *text)
{
    char *tmp;

    g_return_if_fail (MOO_IS_PANE (pane));

    if (!text)
    {
        moo_pane_set_frame_text (pane, NULL);
        return;
    }

    tmp = pane->frame_label_text;
    pane->frame_label_text = g_strdup (text);
    pane->frame_label_markup = TRUE;
    g_free (tmp);

    if (pane->frame_label_embedded)
        gtk_label_set_markup (GTK_LABEL (pane->frame_label_embedded), text);
    if (pane->frame_label_window)
        gtk_label_set_markup (GTK_LABEL (pane->frame_label_window), text);
}

/**
 * moo_pane_set_frame_text:
 *
 * @pane:
 * @text: (type const-utf8) (allow-none):
 **/
void
moo_pane_set_frame_text (MooPane    *pane,
                         const char *text)
{
    char *tmp;

    g_return_if_fail (MOO_IS_PANE (pane));

    tmp = pane->frame_label_text;
    pane->frame_label_text = g_strdup (text);
    pane->frame_label_markup = FALSE;
    g_free (tmp);

    if (pane->frame_label_embedded)
        gtk_label_set_text (GTK_LABEL (pane->frame_label_embedded), text);
    if (pane->frame_label_window)
        gtk_label_set_text (GTK_LABEL (pane->frame_label_window), text);
}


static GtkWidget *
create_button (MooPane      *pane,
               GtkWidget    *toolbar,
               const char   *tip,
               gboolean      toggle,
               int           padding,
               MooSmallIcon  icon)
{
    GtkWidget *button;
    GtkWidget *icon_widget;

    if (toggle)
        button = gtk_toggle_button_new ();
    else
        button = gtk_button_new ();

    g_object_set_data (G_OBJECT (button), "moo-pane", pane);
    gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
    _moo_widget_set_tooltip (button, tip);

    icon_widget = _moo_create_small_icon (icon);
    gtk_container_add (GTK_CONTAINER (button), icon_widget);
    gtk_box_pack_end (GTK_BOX (toolbar), button, FALSE, FALSE, padding);

    gtk_widget_show_all (button);
    return button;
}

static GtkWidget *
create_frame_widget (MooPane        *pane,
                     MooPanePosition position,
                     gboolean        embedded)
{
    GtkWidget *vbox, *toolbar, *separator, *handle, *table, *child_holder;
    GtkWidget *handle_hbox, *frame_label;

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);

    toolbar = gtk_hbox_new (FALSE, 0);

    handle = gtk_event_box_new ();
    gtk_widget_show (handle);
    gtk_box_pack_start (GTK_BOX (toolbar), handle, TRUE, TRUE, 3);
    pane->handle = handle;

    handle_hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (handle_hbox);
    gtk_container_add (GTK_CONTAINER (pane->handle), handle_hbox);

    frame_label = gtk_label_new (NULL);
    gtk_widget_show (frame_label);
    gtk_box_pack_start (GTK_BOX (handle_hbox), frame_label, TRUE, TRUE, 0);
    gtk_misc_set_alignment (GTK_MISC (frame_label), .0, .5);
    gtk_misc_set_padding (GTK_MISC (frame_label), 6, 0);
    gtk_label_set_ellipsize (GTK_LABEL (frame_label), PANGO_ELLIPSIZE_END);
    if (pane->frame_label_markup)
        gtk_label_set_markup (GTK_LABEL (frame_label), pane->frame_label_text);
    else
        gtk_label_set_text (GTK_LABEL (frame_label), pane->frame_label_text);
    if (embedded)
        pane->frame_label_embedded = frame_label;
    else
        pane->frame_label_window = frame_label;

    pane->small_handle = gtk_event_box_new ();
    gtk_widget_show (pane->small_handle);
    gtk_box_pack_start (GTK_BOX (handle_hbox), pane->small_handle, TRUE, TRUE, 0);

    if (embedded)
    {
        GtkWidget *hide_button;

        pane->close_button = create_button (pane, toolbar,
                                            _("Remove pane"), FALSE, 3,
                                            MOO_SMALL_ICON_CLOSE);
        g_object_set_data (G_OBJECT (pane->close_button), "moo-pane", pane);
        g_signal_connect_swapped (pane->close_button, "clicked",
                                  G_CALLBACK (close_button_clicked),
                                  pane);
        if (!pane->removable)
            gtk_widget_hide (pane->close_button);

        hide_button = create_button (pane, toolbar,
                                     _("Hide pane"), FALSE, 0,
                                     MOO_SMALL_ICON_HIDE);

        pane->sticky_button = create_button (pane, toolbar,
                                             _("Sticky"), TRUE, 0,
                                             MOO_SMALL_ICON_STICKY);

        pane->detach_button = create_button (pane, toolbar,
                                             _("Detach pane"), FALSE, 0,
                                             MOO_SMALL_ICON_DETACH);

        g_signal_connect_swapped (hide_button, "clicked",
                                  G_CALLBACK (hide_button_clicked), pane);
        g_signal_connect_swapped (pane->detach_button, "clicked",
                                  G_CALLBACK (detach_button_clicked), pane);
    }
    else
    {
        GtkWidget *attach_button;

        attach_button = create_button (pane, toolbar,
                                       _("Attach"), FALSE, 0,
                                       MOO_SMALL_ICON_ATTACH);

        pane->keep_on_top_button = create_button (pane, toolbar,
                                                  _("Keep on top"), TRUE, 0,
                                                  MOO_SMALL_ICON_KEEP_ON_TOP);

        g_object_set_data (G_OBJECT (attach_button), "moo-pane", pane);
        g_signal_connect_swapped (attach_button, "clicked",
                                  G_CALLBACK (attach_button_clicked), pane);
    }

    gtk_widget_show (toolbar);
    gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);

    separator = gtk_hseparator_new ();
    gtk_widget_show (separator);
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 0);

    child_holder = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (child_holder);
    gtk_box_pack_start (GTK_BOX (vbox), child_holder, TRUE, TRUE, 0);
    if (embedded)
        pane->child_holder = child_holder;
    else
        pane->window_child_holder = child_holder;

    table = gtk_table_new (2, 2, FALSE);

    switch (position)
    {
        case MOO_PANE_POS_LEFT:
        case MOO_PANE_POS_RIGHT:
            separator = gtk_vseparator_new ();
            break;
        case MOO_PANE_POS_TOP:
        case MOO_PANE_POS_BOTTOM:
            separator = gtk_hseparator_new ();
            break;
    }

    gtk_widget_show (separator);

    switch (position)
    {
        case MOO_PANE_POS_LEFT:
            gtk_table_attach (GTK_TABLE (table), separator,
                              0, 1, 0, 1,
                              0, GTK_FILL, 0, 0);
            gtk_table_attach (GTK_TABLE (table), vbox,
                              1, 2, 0, 1,
                              GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
            break;
        case MOO_PANE_POS_TOP:
            gtk_table_attach (GTK_TABLE (table), separator,
                              0, 1, 0, 1,
                              0, GTK_FILL, 0, 0);
            gtk_table_attach (GTK_TABLE (table), vbox,
                              0, 1, 1, 2,
                              GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
            break;
        case MOO_PANE_POS_RIGHT:
            gtk_table_attach (GTK_TABLE (table), separator,
                              1, 2, 0, 1,
                              0, GTK_FILL, 0, 0);
            gtk_table_attach (GTK_TABLE (table), vbox,
                              0, 1, 0, 1,
                              GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
            break;
        case MOO_PANE_POS_BOTTOM:
            gtk_table_attach (GTK_TABLE (table), separator,
                              0, 1, 1, 2,
                              0, GTK_FILL, 0, 0);
            gtk_table_attach (GTK_TABLE (table), vbox,
                              0, 1, 0, 1,
                              GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
            break;
    }

    return table;
}

static GtkWidget *
create_label_widget (MooPanePosition position,
                     GtkWidget     **label_widget,
                     GtkWidget     **icon_widget)
{
    GtkWidget *box = NULL;

    g_return_val_if_fail (position < 4, NULL);

    *label_widget = gtk_label_new (NULL);

    switch (position)
    {
        case MOO_PANE_POS_LEFT:
            gtk_label_set_angle (GTK_LABEL (*label_widget), 90);
            break;
        case MOO_PANE_POS_RIGHT:
            gtk_label_set_angle (GTK_LABEL (*label_widget), 270);
            break;
        default:
            break;
    }

    *icon_widget = gtk_image_new ();

    switch (position)
    {
        case MOO_PANE_POS_LEFT:
        case MOO_PANE_POS_RIGHT:
            box = gtk_vbox_new (FALSE, SPACING_IN_BUTTON);
            break;
        default:
            box = gtk_hbox_new (FALSE, SPACING_IN_BUTTON);
            break;
    }

    switch (position)
    {
        case MOO_PANE_POS_LEFT:
            gtk_box_pack_start (GTK_BOX (box), *label_widget, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX (box), *icon_widget, FALSE, FALSE, 0);
            break;
        default:
            gtk_box_pack_start (GTK_BOX (box), *icon_widget, FALSE, FALSE, 0);
            gtk_box_pack_start (GTK_BOX (box), *label_widget, FALSE, FALSE, 0);
            break;
    }

    gtk_widget_show (box);
    return box;
}


static void
paned_enable_detaching_notify (MooPane *pane)
{
    gboolean enable;
    g_object_get (pane->parent, "enable-detaching", &enable, NULL);
    g_object_set (pane->detach_button, "visible", enable && pane->detachable, NULL);
}

static void
paned_sticky_pane_notify (MooPane *pane)
{
    update_sticky_button (pane);
}


static void
button_drag_leave (GtkWidget      *button,
                   G_GNUC_UNUSED GdkDragContext *context,
                   G_GNUC_UNUSED guint time,
                   MooPane        *pane)
{
    if (pane->open_timeout)
        g_source_remove (pane->open_timeout);
    pane->open_timeout = 0;
    if (pane->button_highlight)
        gtk_drag_unhighlight (button);
    pane->button_highlight = FALSE;
}

static gboolean
drag_open_pane (MooPane *pane)
{
    moo_pane_open (pane);

    if (pane->button_highlight)
        gtk_drag_unhighlight (pane->button);
    pane->button_highlight = FALSE;

    pane->open_timeout = 0;
    return FALSE;
}

static gboolean
button_drag_motion (GtkWidget      *button,
                    GdkDragContext *context,
                    G_GNUC_UNUSED int x,
                    G_GNUC_UNUSED int y,
                    guint           time,
                    MooPane        *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), FALSE);

    if (moo_paned_is_open (pane->parent) &&
        moo_paned_get_open_pane (pane->parent) == pane)
    {
        goto out;
    }

    if (pane->params->detached)
        goto out;

    if (!pane->button_highlight)
    {
        gtk_drag_highlight (button);
        pane->button_highlight = TRUE;
    }

    if (!pane->open_timeout)
        pane->open_timeout = g_timeout_add (OPEN_PANE_TIMEOUT,
                                            (GSourceFunc) drag_open_pane,
                                            pane);

    gdk_drag_status (context, 0, time);

    return TRUE;

out:
    if (pane->button_highlight)
        gtk_drag_unhighlight (button);
    pane->button_highlight = FALSE;
    if (pane->open_timeout)
        g_source_remove (pane->open_timeout);
    pane->open_timeout = 0;

    gdk_drag_status (context, 0, time);

    return TRUE;
}

static void
setup_button_dnd (MooPane *pane)
{
    gtk_drag_dest_set (pane->button, 0, NULL, 0, GDK_ACTION_COPY | GDK_ACTION_MOVE);
    g_signal_connect (pane->button, "drag-motion",
                      G_CALLBACK (button_drag_motion), pane);
    g_signal_connect (pane->button, "drag-leave",
                      G_CALLBACK (button_drag_leave), pane);
}

/**
 * moo_pane_set_drag_dest:
 **/
void
moo_pane_set_drag_dest (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));

    moo_pane_unset_drag_dest (pane);

    pane->drag_dest_enabled = TRUE;

    if (pane->button)
        setup_button_dnd (pane);
}

/**
 * moo_pane_unset_drag_dest:
 **/
void
moo_pane_unset_drag_dest (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));

    if (pane->drag_dest_enabled && pane->button)
    {
        gtk_drag_dest_unset (pane->button);
        g_signal_handlers_disconnect_by_func (pane->button,
                                              (gpointer) button_drag_motion,
                                              pane);
        g_signal_handlers_disconnect_by_func (pane->button,
                                              (gpointer) button_drag_leave,
                                              pane);
    }

    if (pane->open_timeout)
        g_source_remove (pane->open_timeout);
    pane->open_timeout = 0;

    if (pane->button_highlight)
    {
        gtk_drag_unhighlight (pane->button);
        pane->button_highlight = FALSE;
    }
}


static void
create_widgets (MooPane         *pane,
                MooPanePosition  position,
                GdkWindow       *pane_window)
{
    GtkWidget *label;

    pane->frame = create_frame_widget (pane, position, TRUE);
    update_sticky_button (pane);

    gtk_widget_set_parent_window (pane->frame, pane_window);
    gtk_widget_set_parent (pane->frame, GTK_WIDGET (pane->parent));

    gtk_box_pack_start (GTK_BOX (pane->child_holder), pane->child, TRUE, TRUE, 0);

    pane->button = gtk_toggle_button_new ();
    gtk_widget_show (pane->button);
    gtk_button_set_focus_on_click (GTK_BUTTON (pane->button), FALSE);

    label = create_label_widget (position,
                                 &pane->label_widget,
                                 &pane->icon_widget);
    gtk_container_add (GTK_CONTAINER (pane->button), label);
    gtk_widget_show (label);
    update_label_widgets (pane);

    if (pane->drag_dest_enabled)
        setup_button_dnd (pane);

    g_object_set_data (G_OBJECT (pane->button), "moo-pane", pane);
    g_object_set_data (G_OBJECT (pane->child), "moo-pane", pane);
    g_object_set_data (G_OBJECT (pane->frame), "moo-pane", pane);
    g_object_set_data (G_OBJECT (pane->handle), "moo-pane", pane);

    g_signal_connect (pane->sticky_button, "toggled",
                      G_CALLBACK (sticky_button_toggled), pane);
}

MooPane *
_moo_pane_new (GtkWidget    *child,
               MooPaneLabel *label)
{
    MooPane *pane;

    g_return_val_if_fail (GTK_IS_WIDGET (child), NULL);

    pane = g_object_new (MOO_TYPE_PANE, (const char*) NULL);
    pane->child = g_object_ref (child);
    gtk_widget_show (pane->child);
    g_object_set_data (G_OBJECT (pane->child), "moo-pane", pane);

    if (label)
        moo_pane_set_label (pane, label);

    return pane;
}

/**
 * moo_pane_get_id:
 *
 * Returns: (type const-utf8)
 **/
const char *
moo_pane_get_id (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), NULL);
    return pane->id;
}

void
_moo_pane_set_id (MooPane    *pane,
                  const char *id)
{
    char *tmp;

    g_return_if_fail (MOO_IS_PANE (pane));

    tmp = pane->id;
    pane->id = g_strdup (id);
    g_free (tmp);
}

void
_moo_pane_set_parent (MooPane   *pane,
                      gpointer   parent,
                      GdkWindow *pane_window)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    g_return_if_fail (MOO_IS_PANED (parent));
    g_return_if_fail (pane->parent == NULL || pane->parent == parent);
    g_return_if_fail (pane->child != NULL || pane->parent == parent);

    if (pane->parent == parent)
    {
        gtk_widget_set_parent_window (pane->frame, pane_window);
    }
    else
    {
        pane->parent = parent;
        create_widgets (pane, _moo_paned_get_position (parent), pane_window);

        g_signal_connect_swapped (parent, "notify::enable-detaching",
                                  G_CALLBACK (paned_enable_detaching_notify),
                                  pane);
        g_signal_connect_swapped (parent, "notify::sticky-pane",
                                  G_CALLBACK (paned_sticky_pane_notify),
                                  pane);
    }
}


void
_moo_pane_size_request (MooPane        *pane,
                        GtkRequisition *req)
{
    g_return_if_fail (MOO_IS_PANE (pane) && pane->frame != NULL);
    gtk_widget_size_request (pane->frame, req);
}

void
_moo_pane_size_allocate (MooPane       *pane,
                         GtkAllocation *allocation)
{
    g_return_if_fail (MOO_IS_PANE (pane) && pane->frame != NULL);
    gtk_widget_size_allocate (pane->frame, allocation);
}

void
_moo_pane_get_size_request (MooPane        *pane,
                            GtkRequisition *req)
{
    g_return_if_fail (MOO_IS_PANE (pane) && pane->frame != NULL);
    gtk_widget_get_child_requisition (pane->frame, req);
}

GtkWidget *
_moo_pane_get_frame (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), NULL);
    return pane->frame;
}

GtkWidget *
_moo_pane_get_focus_child (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), NULL);
    return pane->focus_child;
}

GtkWidget *
_moo_pane_get_button (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), NULL);
    return pane->button;
}

void
_moo_pane_get_handle (MooPane    *pane,
                      GtkWidget **big,
                      GtkWidget **small_)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    *big = pane->handle;
    *small_ = pane->small_handle;
}

GtkWidget *
_moo_pane_get_window (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), NULL);
    return pane->window;
}

/**
 * moo_pane_get_child:
 **/
GtkWidget *
moo_pane_get_child (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), NULL);
    return pane->child;
}

gpointer
_moo_pane_get_parent (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), NULL);
    return pane->parent;
}


void
_moo_pane_params_changed (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    if (!pane->params_changed_blocked)
        g_object_notify (G_OBJECT (pane), "params");
}

void
_moo_pane_freeze_params (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    pane->params_changed_blocked = TRUE;
}

void
_moo_pane_thaw_params (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    pane->params_changed_blocked = FALSE;
}

gboolean
_moo_pane_get_detached (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), FALSE);
    return pane->params->detached;
}


void
_moo_pane_unparent (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));

    if (pane->parent)
    {
        g_signal_handlers_disconnect_by_func (pane->parent, (gpointer) paned_enable_detaching_notify, pane);
        g_signal_handlers_disconnect_by_func (pane->parent, (gpointer) paned_sticky_pane_notify, pane);

        pane->parent = NULL;

        gtk_container_remove (GTK_CONTAINER (pane->child_holder), pane->child);

        gtk_widget_unparent (pane->frame);

        pane->child_holder = NULL;
        pane->frame = NULL;
        pane->handle = NULL;
        pane->small_handle = NULL;
        pane->button = NULL;
        pane->label_widget = NULL;
        pane->icon_widget = NULL;
        pane->sticky_button = NULL;
        pane->detach_button = NULL;
        pane->close_button = NULL;

        if (pane->window)
            gtk_widget_destroy (pane->window);

        pane->window = NULL;
        pane->keep_on_top_button = NULL;
        pane->window_child_holder = NULL;

        pane->focus_child = NULL;
    }
}


static GtkWidget *
find_focus (GtkWidget *widget)
{
    GtkWidget *focus_child, *window;

    if (!widget)
        return NULL;

    window = gtk_widget_get_toplevel (widget);

    if (!GTK_IS_WINDOW (window))
        return NULL;

    focus_child = gtk_window_get_focus (GTK_WINDOW (window));

    if (focus_child && gtk_widget_is_ancestor (focus_child, widget))
        return focus_child;
    else
        return NULL;
}

void
_moo_pane_update_focus_child (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));

    if (pane->focus_child)
        g_object_remove_weak_pointer (pane->focus_child, &pane->focus_child);

    pane->focus_child = find_focus (pane->child);

    if (pane->focus_child)
        g_object_add_weak_pointer (pane->focus_child, &pane->focus_child);
}


static gboolean
pane_window_delete_event (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), FALSE);
    moo_paned_attach_pane (pane->parent, pane);
    return TRUE;
}

static void
keep_on_top_button_toggled (GtkToggleButton *button,
                            MooPane         *pane)
{
    gboolean active;

    g_return_if_fail (MOO_IS_PANE (pane));

    active = gtk_toggle_button_get_active (button);
    pane->params->keep_on_top = active;

    if (pane->params->keep_on_top)
    {
        GtkWidget *parent = gtk_widget_get_toplevel (GTK_WIDGET (pane->parent));

        if (GTK_IS_WINDOW (parent))
            gtk_window_set_transient_for (GTK_WINDOW (pane->window),
                                          GTK_WINDOW (parent));
    }
    else
    {
        gtk_window_set_transient_for (GTK_WINDOW (pane->window), NULL);
    }

    _moo_pane_params_changed (pane);
}

static gboolean
pane_window_configure (GtkWidget         *window,
                       GdkEventConfigure *event,
                       MooPane           *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), FALSE);
    g_return_val_if_fail (pane->window == window, FALSE);

    pane->params->window_position.x = event->x;
    pane->params->window_position.y = event->y;
    pane->params->window_position.width = event->width;
    pane->params->window_position.height = event->height;

    _moo_pane_params_changed (pane);
    return FALSE;
}

static void
create_pane_window (MooPane *pane)
{
    int width = -1;
    int height = -1;
    GtkWidget *frame;
    GtkWindow *window;

    if (pane->window)
        return;

    pane->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    window = GTK_WINDOW (pane->window);

    moo_help_connect_keys (pane->window);

    set_pane_window_icon_and_title (pane);
    gtk_window_set_type_hint (GTK_WINDOW (pane->window),
                              GDK_WINDOW_TYPE_HINT_UTILITY);

    switch (_moo_paned_get_position (pane->parent))
    {
        case MOO_PANE_POS_LEFT:
        case MOO_PANE_POS_RIGHT:
            width = moo_paned_get_pane_size (pane->parent);
            height = GTK_WIDGET(pane->parent)->allocation.height;
            break;
        case MOO_PANE_POS_TOP:
        case MOO_PANE_POS_BOTTOM:
            height = moo_paned_get_pane_size (pane->parent);
            width = GTK_WIDGET(pane->parent)->allocation.width;
            break;
    }

    gtk_window_set_default_size (window, width, height);

    g_signal_connect_swapped (window, "delete-event",
                              G_CALLBACK (pane_window_delete_event), pane);

    frame = create_frame_widget (pane, _moo_paned_get_position (pane->parent), FALSE);
    gtk_widget_show (frame);
    gtk_container_add (GTK_CONTAINER (pane->window), frame);

    g_object_set_data (G_OBJECT (pane->window), "moo-pane", pane);
    g_object_set_data (G_OBJECT (pane->keep_on_top_button), "moo-pane", pane);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pane->keep_on_top_button),
                                  pane->params->keep_on_top);
    g_signal_connect (pane->keep_on_top_button, "toggled",
                      G_CALLBACK (keep_on_top_button_toggled), pane);
    g_signal_connect (pane->window, "configure-event",
                      G_CALLBACK (pane_window_configure), pane);
}

/* FIXME use gtk_widget_reparent(), it does work now */
static void
reparent (GtkWidget *widget,
          GtkWidget *old_container,
          GtkWidget *new_container)
{
    g_object_ref (widget);
    gtk_container_remove (GTK_CONTAINER (old_container), widget);
    gtk_container_add (GTK_CONTAINER (new_container), widget);
    g_object_unref (widget);
}

void
_moo_pane_detach (MooPane *pane)
{
    gboolean visible;

    g_return_if_fail (MOO_IS_PANE (pane));

    if (pane->params->detached)
        return;

    pane->params->detached = TRUE;

    create_pane_window (pane);
    reparent (pane->child, pane->child_holder, pane->window_child_holder);

    if (pane->params->keep_on_top)
    {
        GtkWidget *parent = gtk_widget_get_toplevel (GTK_WIDGET (pane->parent));
        if (GTK_IS_WINDOW (parent))
            gtk_window_set_transient_for (GTK_WINDOW (pane->window),
                                          GTK_WINDOW (parent));
    }
    else
    {
        gtk_window_set_transient_for (GTK_WINDOW (pane->window), NULL);
    }

    if (pane->focus_child)
        gtk_widget_grab_focus (pane->focus_child);
    else
        gtk_widget_child_focus (pane->child, GTK_DIR_TAB_FORWARD);

    g_object_get (pane->window, "visible", &visible, NULL);

    if (!visible &&
        pane->params->window_position.width > 0 &&
        pane->params->window_position.height > 0)
    {
        gtk_window_move (GTK_WINDOW (pane->window),
                         pane->params->window_position.x,
                         pane->params->window_position.y);
        gtk_window_set_default_size (GTK_WINDOW (pane->window),
                                     pane->params->window_position.width,
                                     pane->params->window_position.height);
    }

    gtk_window_present (GTK_WINDOW (pane->window));
    _moo_pane_params_changed (pane);
}


void
_moo_pane_attach (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));

    if (!pane->params->detached)
        return;

    pane->params->detached = FALSE;

    if (pane->focus_child)
        g_object_remove_weak_pointer (pane->focus_child, &pane->focus_child);
    pane->focus_child = find_focus (pane->child);
    if (pane->focus_child)
        g_object_add_weak_pointer (pane->focus_child, &pane->focus_child);

    reparent (pane->child, pane->window_child_holder, pane->child_holder);

    gtk_widget_hide (pane->window);
    _moo_pane_params_changed (pane);
}


void
_moo_pane_try_remove (MooPane *pane)
{
    gboolean ret;

    g_return_if_fail (MOO_IS_PANE (pane));
    g_return_if_fail (pane->parent != NULL);

    g_object_ref (pane);

    g_signal_emit (pane, signals[REMOVE], 0, &ret);

    if (!ret && pane->parent && pane->child)
        moo_paned_remove_pane (pane->parent, pane->child);

    g_object_unref (pane);
}


/**
 * moo_pane_open:
 **/
void
moo_pane_open (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    g_return_if_fail (pane->parent != NULL);
    moo_paned_open_pane (pane->parent, pane);
}

/**
 * moo_pane_present:
 **/
void
moo_pane_present (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    g_return_if_fail (pane->parent != NULL);
    moo_paned_present_pane (pane->parent, pane);
}

/**
 * moo_pane_attach:
 **/
void
moo_pane_attach (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    g_return_if_fail (pane->parent != NULL);
    moo_paned_attach_pane (pane->parent, pane);
}

/**
 * moo_pane_detach:
 **/
void
moo_pane_detach (MooPane *pane)
{
    g_return_if_fail (MOO_IS_PANE (pane));
    g_return_if_fail (pane->parent != NULL);
    moo_paned_detach_pane (pane->parent, pane);
}


/**
 * moo_pane_get_index:
 **/
int
moo_pane_get_index (MooPane *pane)
{
    g_return_val_if_fail (MOO_IS_PANE (pane), -1);
    if (pane->parent)
        return moo_paned_get_pane_num (pane->parent, pane->child);
    else
        return -1;
}


typedef enum {
    ICON_PIXBUFS,
    ICON_ARROW_UP,
    ICON_ARROW_DOWN,
    ICON_ARROW_LEFT,
    ICON_ARROW_RIGHT
} IconType;

typedef struct {
    GtkWidget base;
    GdkPixbuf **pixbufs;
    const guchar *data;
    IconType type;
} MooIconWidget;

typedef struct {
    GtkWidgetClass base_class;
} MooIconWidgetClass;

GType _moo_icon_widget_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (MooIconWidget, _moo_icon_widget, GTK_TYPE_WIDGET)

static void
_moo_icon_widget_init (MooIconWidget *icon)
{
    GTK_WIDGET_SET_NO_WINDOW (icon);
    icon->pixbufs = NULL;
    icon->data = NULL;
    icon->type = ICON_PIXBUFS;
}

static void
free_pixbufs (MooIconWidget *icon)
{
    if (icon->pixbufs)
    {
        int i;
        for (i = 0; i < 5 /* magic */; ++i)
            g_object_unref (icon->pixbufs[i]);
        g_free (icon->pixbufs);
        icon->pixbufs = NULL;
    }
}

static void
moo_icon_widget_style_set (GtkWidget *widget,
                           GtkStyle  *old_style)
{
    GTK_WIDGET_CLASS (_moo_icon_widget_parent_class)->style_set (widget, old_style);
    free_pixbufs ((MooIconWidget*) widget);
}

static void
moo_icon_widget_dispose (GObject *object)
{
    free_pixbufs ((MooIconWidget*) object);
    G_OBJECT_CLASS (_moo_icon_widget_parent_class)->dispose (object);
}

static GdkPixbuf *
get_pixbuf (MooIconWidget *icon)
{
    if (!icon->pixbufs)
    {
        int state;
        GtkWidget *widget = GTK_WIDGET (icon);

        g_return_val_if_fail (icon->data != NULL, NULL);

        icon->pixbufs = g_new0 (GdkPixbuf*, 5 /* magic */);

        for (state = 0; state < 5 /* magic */; ++state)
        {
            GdkPixbuf *pixbuf;
            guchar *pixels, *p;
            int width, height, rowstride, n_channels;
            GdkColor *color;
            int x, y;

            pixbuf = gdk_pixbuf_new_from_inline (-1, icon->data, TRUE, NULL);
            g_return_val_if_fail (pixbuf != NULL, NULL);

            icon->pixbufs[state] = pixbuf;

            pixels = gdk_pixbuf_get_pixels (pixbuf);
            width = gdk_pixbuf_get_width (pixbuf);
            height = gdk_pixbuf_get_height (pixbuf);
            n_channels = gdk_pixbuf_get_n_channels (pixbuf);
            g_assert (n_channels == 4);
            rowstride = gdk_pixbuf_get_rowstride (pixbuf);

            color = &widget->style->fg[state];

            for (x = 0; x < width; ++x)
            {
                for (y = 0; y < height; ++y)
                {
                    p = pixels + y * rowstride + x * n_channels;

                    if (p[3] != 0)
                    {
                        p[0] = color->red >> 8;
                        p[1] = color->green >> 8;
                        p[2] = color->blue >> 8;
                    }
                }
            }
        }
    }

    return icon->pixbufs[GTK_WIDGET_STATE (icon)];
}

static void
draw_pixbuf (GtkWidget      *widget,
             GdkEventExpose *event)
{
    GdkPixbuf *pixbuf;
    int pixbuf_width, pixbuf_height;
    int x, y;

    pixbuf = get_pixbuf ((MooIconWidget*) widget);
    g_return_if_fail (pixbuf != NULL);

    pixbuf_width = gdk_pixbuf_get_width (pixbuf);
    pixbuf_height = gdk_pixbuf_get_height (pixbuf);

    x = widget->allocation.x + (widget->allocation.width - pixbuf_width) / 2;
    y = widget->allocation.y + (widget->allocation.height - pixbuf_height) / 2;

    gdk_draw_pixbuf (event->window,
                     widget->style->black_gc,
                     pixbuf,
                     0, 0, x, y, pixbuf_width, pixbuf_height,
                     GDK_RGB_DITHER_NORMAL, 0, 0);
}

static void
draw_arrow (GtkWidget      *widget,
            GdkEventExpose *event)
{
    GtkArrowType arrow_type;
    int x, y, width, height;

    switch (((MooIconWidget*)widget)->type)
    {
        case ICON_ARROW_UP:
            arrow_type = GTK_ARROW_UP;
            break;
        case ICON_ARROW_DOWN:
            arrow_type = GTK_ARROW_DOWN;
            break;
        case ICON_ARROW_LEFT:
            arrow_type = GTK_ARROW_LEFT;
            break;
        case ICON_ARROW_RIGHT:
            arrow_type = GTK_ARROW_RIGHT;
            break;
        default:
            g_return_if_reached ();
    }

    width = 3 * widget->allocation.width / 4;
    height = 3 * widget->allocation.height / 4;
    x = widget->allocation.x + width / 6;
    y = widget->allocation.y + height / 6;

    gtk_paint_arrow (widget->style,
                     event->window,
                     GTK_WIDGET_STATE (widget),
                     GTK_SHADOW_IN,
                     &event->area,
                     widget,
                     NULL,
                     arrow_type,
                     TRUE,
                     x, y, width, height);
}

static gboolean
moo_icon_widget_expose_event (GtkWidget      *widget,
                              GdkEventExpose *event)
{
    MooIconWidget *icon = (MooIconWidget*) widget;

    switch (icon->type)
    {
        case ICON_PIXBUFS:
            draw_pixbuf (widget, event);
            break;
        case ICON_ARROW_UP:
        case ICON_ARROW_DOWN:
        case ICON_ARROW_LEFT:
        case ICON_ARROW_RIGHT:
            draw_arrow (widget, event);
            break;
    }

    return FALSE;
}

static void
_moo_icon_widget_class_init (MooIconWidgetClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->dispose = moo_icon_widget_dispose;

    widget_class->style_set = moo_icon_widget_style_set;
    widget_class->expose_event = moo_icon_widget_expose_event;
}

GtkWidget *
_moo_create_small_icon (MooSmallIcon icon)
{
    MooIconWidget *icon_widget;
    const guchar *data = NULL;

    switch (icon)
    {
        case MOO_SMALL_ICON_HIDE:
            data = MOO_HIDE_ICON;
            break;
        case MOO_SMALL_ICON_STICKY:
            data = MOO_STICKY_ICON;
            break;
        case MOO_SMALL_ICON_CLOSE:
            data = MOO_CLOSE_ICON;
            break;
        case MOO_SMALL_ICON_DETACH:
            data = MOO_DETACH_ICON;
            break;
        case MOO_SMALL_ICON_ATTACH:
            data = MOO_ATTACH_ICON;
            break;
        case MOO_SMALL_ICON_KEEP_ON_TOP:
            data = MOO_KEEP_ON_TOP_ICON;
            break;
    }

    g_return_val_if_fail (data != NULL, NULL);

    icon_widget = g_object_new (_moo_icon_widget_get_type (), (const char*) NULL);
    icon_widget->data = data;
    gtk_widget_set_size_request (GTK_WIDGET (icon_widget), 7, 7 /* magic */);

    return GTK_WIDGET (icon_widget);
}

GtkWidget *
_moo_create_arrow_icon (GtkArrowType arrow_type)
{
    MooIconWidget *icon_widget;
    IconType icon_type;

    switch (arrow_type)
    {
        case GTK_ARROW_UP:
            icon_type = ICON_ARROW_UP;
            break;
        case GTK_ARROW_DOWN:
            icon_type = ICON_ARROW_DOWN;
            break;
        case GTK_ARROW_LEFT:
            icon_type = ICON_ARROW_LEFT;
            break;
        case GTK_ARROW_RIGHT:
            icon_type = ICON_ARROW_RIGHT;
            break;
        default:
            g_return_val_if_reached (NULL);
    }

    icon_widget = g_object_new (_moo_icon_widget_get_type (), (const char*) NULL);
    icon_widget->type = icon_type;

    return GTK_WIDGET (icon_widget);
}
