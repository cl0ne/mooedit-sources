/*
 *   mooprefspage.c
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
 * class:MooPrefsPage: (parent GtkVBox) (constructable) (moo.private 1)
 **/

#include "mooutils/mooprefspage.h"
#include "mooutils/moostock.h"
#include "mooutils/mooprefs.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/moofontsel.h"
#include "mooutils/mooi18n.h"
#include "mooutils/moocompat.h"
#include "marshals.h"
#include <string.h>

struct _MooPrefsPagePrivate {
    GdkPixbuf   *icon;
    char        *icon_stock_id;
    GSList      *widgets;
    GSList      *children;

    MooPrefsPageInitUi init_ui;
    MooPrefsPageInit init;
    MooPrefsPageApply apply;
    gboolean ui_initialized;
};

static void moo_prefs_page_finalize     (GObject        *object);

static void moo_prefs_page_set_property (GObject        *object,
                                         guint           prop_id,
                                         const GValue   *value,
                                         GParamSpec     *pspec);
static void moo_prefs_page_get_property (GObject        *object,
                                         guint           prop_id,
                                         GValue         *value,
                                         GParamSpec     *pspec);

static void moo_prefs_page_init_sig     (MooPrefsPage   *page);
static void moo_prefs_page_apply        (MooPrefsPage   *page);


enum {
    PROP_0,
    PROP_LABEL,
    PROP_ICON,
    PROP_ICON_STOCK_ID,
    PROP_AUTO_APPLY
};

enum {
    APPLY,
    INIT,
    LAST_SIGNAL
};

static G_GNUC_UNUSED guint signals[LAST_SIGNAL];

/* MOO_TYPE_PREFS_PAGE */
G_DEFINE_TYPE (MooPrefsPage, moo_prefs_page, GTK_TYPE_VBOX)

static void
moo_prefs_page_class_init (MooPrefsPageClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = moo_prefs_page_set_property;
    gobject_class->get_property = moo_prefs_page_get_property;
    gobject_class->finalize = moo_prefs_page_finalize;

    klass->init = moo_prefs_page_init_sig;
    klass->apply = moo_prefs_page_apply;

    g_type_class_add_private (klass, sizeof (MooPrefsPagePrivate));

    g_object_class_install_property (gobject_class, PROP_LABEL,
        g_param_spec_string ("label", "label", "Label",
                             NULL, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_ICON_STOCK_ID,
        g_param_spec_string ("icon-stock-id", "icon-stock-id", "icon-stock-id",
                             NULL, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_ICON,
        g_param_spec_object ("icon", "icon", "Icon",
                             GDK_TYPE_PIXBUF, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_AUTO_APPLY,
        g_param_spec_boolean ("auto-apply", "auto-apply", "auto-apply",
                              TRUE, (GParamFlags) G_PARAM_READWRITE));

    signals[APPLY] =
        g_signal_new ("apply",
                      G_OBJECT_CLASS_TYPE (klass),
                      (GSignalFlags) (G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION),
                      G_STRUCT_OFFSET (MooPrefsPageClass, apply),
                      NULL, NULL,
                      _moo_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[INIT] =
        g_signal_new ("init",
                      G_OBJECT_CLASS_TYPE (klass),
                      (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                      G_STRUCT_OFFSET (MooPrefsPageClass, init),
                      NULL, NULL,
                      _moo_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
}

static void
moo_prefs_page_init (MooPrefsPage *page)
{
    page->priv = G_TYPE_INSTANCE_GET_PRIVATE (page,
                                              MOO_TYPE_PREFS_PAGE,
                                              MooPrefsPagePrivate);

    page->label = NULL;
    page->priv->icon = NULL;
    page->priv->icon_stock_id = NULL;
    page->priv->widgets = NULL;
    page->priv->children = NULL;
    page->auto_apply = TRUE;
}

static void
moo_prefs_page_finalize (GObject *object)
{
    MooPrefsPage *page = MOO_PREFS_PAGE (object);

    g_free (page->label);
    g_free (page->priv->icon_stock_id);
    if (page->priv->icon)
        g_object_unref (page->priv->icon);
    g_slist_free (page->priv->widgets);
    g_slist_free (page->priv->children);

    G_OBJECT_CLASS (moo_prefs_page_parent_class)->finalize (object);
}

static void
moo_prefs_page_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    MooPrefsPage *page = MOO_PREFS_PAGE (object);

    switch (prop_id)
    {
        case PROP_LABEL:
            g_free (page->label);

            if (g_value_get_string (value))
                page->label = g_markup_printf_escaped ("<b>%s</b>", g_value_get_string (value));
            else
                page->label = NULL;

            g_object_notify (G_OBJECT (page), "label");
            break;

        case PROP_ICON:
            if (page->priv->icon)
                g_object_unref (G_OBJECT (page->priv->icon));
            page->priv->icon = GDK_PIXBUF (g_value_dup_object (value));
            g_object_notify (G_OBJECT (page), "icon");
            break;

        case PROP_ICON_STOCK_ID:
            g_free (page->priv->icon_stock_id);
            page->priv->icon_stock_id = g_strdup (g_value_get_string (value));
            g_object_notify (G_OBJECT (page), "icon-stock-id");
            break;

        case PROP_AUTO_APPLY:
            page->auto_apply = g_value_get_boolean (value) != 0;
            g_object_notify (object, "auto-apply");
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
moo_prefs_page_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
    MooPrefsPage *page = MOO_PREFS_PAGE (object);

    switch (prop_id)
    {
        case PROP_LABEL:
            g_value_set_string (value, page->label);
            break;

        case PROP_ICON:
            g_value_set_object (value, page->priv->icon);
            break;

        case PROP_ICON_STOCK_ID:
            g_value_set_string (value, page->priv->icon_stock_id);
            break;

        case PROP_AUTO_APPLY:
            g_value_set_boolean (value, page->auto_apply);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


GtkWidget *
moo_prefs_page_new (const char *label,
                    const char *icon_stock_id)
{
    return g_object_new (MOO_TYPE_PREFS_PAGE,
                         "label", label,
                         "icon-stock-id", icon_stock_id,
                         (const char*) NULL);
}


void
moo_prefs_page_add_page (MooPrefsPage *page,
                         MooPrefsPage *child_page)
{
    g_return_if_fail (MOO_IS_PREFS_PAGE (page));
    g_return_if_fail (MOO_IS_PREFS_PAGE (child_page));
    page->priv->children = g_slist_prepend (page->priv->children, child_page);
}


void
moo_prefs_page_set_callbacks (MooPrefsPage      *page,
                              MooPrefsPageInitUi init_ui,
                              MooPrefsPageInit   init,
                              MooPrefsPageApply  apply)
{
    g_return_if_fail (MOO_IS_PREFS_PAGE (page));
    page->priv->init_ui = init_ui;
    page->priv->init = init;
    page->priv->apply = apply;
}


/**************************************************************************/
/* Settings
 */

static void     setting_init        (GtkWidget      *widget);
static void     setting_apply       (GtkWidget      *widget);
static gboolean setting_get_value   (GtkWidget      *widget,
                                     GValue         *value,
                                     const char     *prefs_key);
static void     setting_set_value   (GtkWidget      *widget,
                                     const GValue   *value);


static void
moo_prefs_page_init_sig (MooPrefsPage *page)
{
    if (!page->priv->ui_initialized)
    {
        page->priv->ui_initialized = TRUE;
        if (page->priv->init_ui)
            page->priv->init_ui (page);
    }

    if (page->priv->init)
        page->priv->init (page);

    g_slist_foreach (page->priv->widgets, (GFunc) setting_init, NULL);
    g_slist_foreach (page->priv->children, (GFunc) moo_prefs_page_init_sig, NULL);
}


static void
moo_prefs_page_apply (MooPrefsPage *page)
{
    g_slist_foreach (page->priv->widgets, (GFunc) setting_apply, NULL);

    if (page->priv->apply && page->priv->ui_initialized)
        page->priv->apply (page);

    g_slist_foreach (page->priv->children, (GFunc) moo_prefs_page_apply, NULL);
}


static void
setting_init (GtkWidget *widget)
{
    const GValue *value;
    const char *prefs_key = g_object_get_data (G_OBJECT (widget), "moo-prefs-key");

    g_return_if_fail (prefs_key != NULL);

    value = moo_prefs_get (prefs_key);
    g_return_if_fail (value != NULL);

    setting_set_value (widget, value);
}


static void
setting_apply (GtkWidget *widget)
{
    const char *prefs_key = g_object_get_data (G_OBJECT (widget), "moo-prefs-key");
    GtkWidget *set_or_not = g_object_get_data (G_OBJECT (widget), "moo-prefs-set-or-not");
    GValue value;
    GType type;

    g_return_if_fail (prefs_key != NULL);

    if (!GTK_WIDGET_SENSITIVE (widget))
        return;

    if (set_or_not)
    {
        gboolean unset;

        if (!GTK_WIDGET_SENSITIVE (set_or_not))
            return;

        unset = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (set_or_not));

        if (unset)
            return;
    }

    type = moo_prefs_get_key_type (prefs_key);
    g_return_if_fail (type != G_TYPE_NONE);

    value.g_type = 0;
    g_value_init (&value, type);
    if (setting_get_value (widget, &value, prefs_key))
        moo_prefs_set (prefs_key, &value);
    g_value_unset (&value);
}


static gboolean
setting_get_value (GtkWidget      *widget,
                   GValue         *value,
                   const char     *prefs_key)
{
    if (GTK_IS_SPIN_BUTTON (widget))
    {
        GtkSpinButton *spin = GTK_SPIN_BUTTON (widget);

        gtk_spin_button_update (spin);

        if (value->g_type == G_TYPE_INT)
        {
            int val = gtk_spin_button_get_value_as_int (spin);
            g_value_set_int (value, val);
            return TRUE;
        }
    }
    else if (GTK_IS_ENTRY (widget))
    {
        if (value->g_type == G_TYPE_STRING)
        {
            const char *val = gtk_entry_get_text (GTK_ENTRY (widget));

            if (!val[0])
            {
                const GValue *dflt = moo_prefs_get_default (prefs_key);

                if (!dflt || !g_value_get_string (dflt))
                    val = NULL;
            }

            g_value_set_string (value, val);
            return TRUE;
        }
    }
    else if (GTK_IS_FONT_BUTTON (widget) || MOO_IS_FONT_BUTTON (widget))
    {
        if (value->g_type == G_TYPE_STRING)
        {
            char *val = NULL;
            g_object_get (widget, "font-name", &val, NULL);
            g_value_take_string (value, val);
            return TRUE;
        }
    }
    else if (GTK_IS_TOGGLE_BUTTON (widget) && !GTK_IS_RADIO_BUTTON (widget))
    {
        if (value->g_type == G_TYPE_BOOLEAN)
        {
            gboolean val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
            g_value_set_boolean (value, val);
            return TRUE;
        }
    }

    g_warning ("could not get value of type '%s' from widget '%s'",
               g_type_name (value->g_type),
               g_type_name (G_OBJECT_TYPE (widget)));
    return FALSE;
}


static void
setting_set_value (GtkWidget    *widget,
                   const GValue *value)
{
    if (GTK_IS_SPIN_BUTTON (widget))
    {
        GtkSpinButton *spin = GTK_SPIN_BUTTON (widget);

        if (value->g_type == G_TYPE_INT)
        {
            gtk_spin_button_set_value (spin, g_value_get_int (value));
            return;
        }
    }
    else if (GTK_IS_ENTRY (widget))
    {
        if (value->g_type == G_TYPE_STRING)
        {
            const char *val = g_value_get_string (value);
            if (!val)
                val = "";
            gtk_entry_set_text (GTK_ENTRY (widget), val);
            return;
        }
    }
    else if (GTK_IS_FONT_BUTTON (widget) || MOO_IS_FONT_BUTTON (widget))
    {
        if (value->g_type == G_TYPE_STRING)
        {
            const char *val = g_value_get_string (value);
            if (!val)
                val = "";
            g_object_set (widget, "font-name", val, NULL);
            return;
        }
    }
    else if (GTK_IS_TOGGLE_BUTTON (widget) && !GTK_IS_RADIO_BUTTON (widget))
    {
        if (value->g_type == G_TYPE_BOOLEAN)
        {
            gboolean val = g_value_get_boolean (value);
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), val);
            return;
        }
    }

    g_warning ("could not set value of type '%s' to widget '%s'",
               g_type_name (value->g_type),
               g_type_name (G_OBJECT_TYPE (widget)));
}


void
moo_prefs_page_bind_setting (MooPrefsPage *page,
                             GtkWidget    *widget,
                             const char   *setting)
{
    g_return_if_fail (MOO_IS_PREFS_PAGE (page));
    g_return_if_fail (GTK_IS_WIDGET (widget));
    g_return_if_fail (setting != NULL);

    g_object_set_data_full (G_OBJECT (widget), "moo-prefs-key",
                            g_strdup (setting), g_free);

    page->priv->widgets = g_slist_prepend (page->priv->widgets, widget);
}
