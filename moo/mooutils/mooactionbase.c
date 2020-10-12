/*
 *   mooactionbase.c
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

#include "mooutils/mooactionbase-private.h"
#include "mooutils/mooaction-private.h"
#include "mooutils/mooactiongroup.h"
#include "mooutils/mooaccel.h"
#include "marshals.h"
#include <gtk/gtk.h>
#include <string.h>


static void proxy_set_use_underline       (GtkWidget *proxy,
                                           gboolean   use_underline);


enum {
    MOO_ACTION_BASE_PROPS(MOO_ACTION_BASE)
};


static void
class_init (gpointer g_iface)
{
    g_object_interface_install_property (g_iface,
        g_param_spec_string ("display-name", "display-name", "display-name",
                             NULL, (GParamFlags) G_PARAM_READWRITE));
    g_object_interface_install_property (g_iface,
        g_param_spec_string ("default-accel", "default-accel", "default-accel",
                             NULL, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
    g_object_interface_install_property (g_iface,
        g_param_spec_boolean ("connect-accel", "connect-accel", "connect-accel",
                              FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
    g_object_interface_install_property (g_iface,
        g_param_spec_boolean ("no-accel", "no-accel", "no-accel",
                              FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
    g_object_interface_install_property (g_iface,
        g_param_spec_boolean ("force-accel-label", "force-accel-label", "force-accel-label",
                              FALSE, (GParamFlags) G_PARAM_READWRITE));
    g_object_interface_install_property (g_iface,
        g_param_spec_boolean ("accel-editable", "accel-editable", "accel-editable",
                              TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
    g_object_interface_install_property (g_iface,
        g_param_spec_boolean ("dead", "dead", "dead",
                              FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
    g_object_interface_install_property (g_iface,
        g_param_spec_boolean ("active", "active", "active",
                              TRUE, (GParamFlags) G_PARAM_WRITABLE));
    g_object_interface_install_property (g_iface,
        g_param_spec_boolean ("has-submenu", "has-submenu", "has-submenu",
                              FALSE, (GParamFlags) G_PARAM_READWRITE));
    g_object_interface_install_property (g_iface,
        g_param_spec_boolean ("use-underline", "use-underline", "use-underline",
                              TRUE, (GParamFlags) G_PARAM_READWRITE));

    g_signal_new ("connect-proxy",
                  MOO_TYPE_ACTION_BASE,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MooActionBaseClass, connect_proxy),
                  NULL, NULL,
                  _moo_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, GTK_TYPE_WIDGET);

    g_signal_new ("disconnect-proxy",
                  MOO_TYPE_ACTION_BASE,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MooActionBaseClass, disconnect_proxy),
                  NULL, NULL,
                  _moo_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
}


void
_moo_action_base_init_class (GObjectClass *klass)
{
    g_object_class_install_property (klass, MOO_ACTION_BASE_PROP_DISPLAY_NAME,
        g_param_spec_string ("display-name", "display-name", "display-name",
                             NULL, (GParamFlags) G_PARAM_READWRITE));
    g_object_class_install_property (klass, MOO_ACTION_BASE_PROP_DEFAULT_ACCEL,
        g_param_spec_string ("default-accel", "default-accel", "default-accel",
                             NULL, (GParamFlags) G_PARAM_READWRITE));
    g_object_class_install_property (klass, MOO_ACTION_BASE_PROP_CONNECT_ACCEL,
        g_param_spec_boolean ("connect-accel", "connect-accel", "connect-accel",
                              FALSE, (GParamFlags) G_PARAM_READWRITE));
    g_object_class_install_property (klass, MOO_ACTION_BASE_PROP_NO_ACCEL,
        g_param_spec_boolean ("no-accel", "no-accel", "no-accel",
                              FALSE, (GParamFlags) G_PARAM_READWRITE));
    g_object_class_install_property (klass, MOO_ACTION_BASE_PROP_ACCEL_EDITABLE,
        g_param_spec_boolean ("accel-editable", "accel-editable", "accel-editable",
                              TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property (klass, MOO_ACTION_BASE_PROP_FORCE_ACCEL_LABEL,
        g_param_spec_boolean ("force-accel-label", "force-accel-label", "force-accel-label",
                              FALSE, (GParamFlags) G_PARAM_READWRITE));
    g_object_class_install_property (klass, MOO_ACTION_BASE_PROP_DEAD,
        g_param_spec_boolean ("dead", "dead", "dead",
                              FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));
    g_object_class_install_property (klass, MOO_ACTION_BASE_PROP_ACTIVE,
        g_param_spec_boolean ("active", "active", "active",
                              TRUE, G_PARAM_WRITABLE));
    g_object_class_install_property (klass, MOO_ACTION_BASE_PROP_HAS_SUBMENU,
        g_param_spec_boolean ("has-submenu", "has-submenu", "has-submenu",
                              FALSE, (GParamFlags) G_PARAM_READWRITE));
    g_object_class_install_property (klass, MOO_ACTION_BASE_PROP_USE_UNDERLINE,
        g_param_spec_boolean ("use-underline", "use-underline", "use-underline",
                              TRUE, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_override_property (klass,
                                      MOO_ACTION_BASE_PROP_LABEL,
                                      "label");
    g_object_class_override_property (klass,
                                      MOO_ACTION_BASE_PROP_TOOLTIP,
                                      "tooltip");
}


GType
moo_action_base_get_type (void)
{
    static GType type;

    if (G_UNLIKELY (!type))
    {
        static const GTypeInfo info = {
            sizeof (MooActionBaseClass), /* class_size */
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            0,
            0, /* n_preallocs */
            NULL,
            NULL
        };

        type = g_type_register_static (G_TYPE_INTERFACE,
                                       "MooActionBase",
                                       &info, (GTypeFlags) 0);
        g_type_interface_add_prerequisite (type, GTK_TYPE_ACTION);
    }

    return type;
}


static char *
strip_underscore (const char *label)
{
    char *stripped, *underscore;

    g_return_val_if_fail (label != NULL, NULL);

    stripped = g_strdup (label);
    underscore = strchr (stripped, '_');

    if (underscore)
        memmove (underscore, underscore + 1, strlen (underscore + 1) + 1);

    return stripped;
}


static void
set_string (gpointer    object,
            const char *id,
            const char *data)
{
    g_object_set_data_full (G_OBJECT (object), id, g_strdup (data), g_free);
}

static const char *
get_string (gpointer    object,
            const char *id)
{
    return (const char*) g_object_get_data (G_OBJECT (object), id);
}


static void
set_bool (gpointer    object,
          const char *id,
          gboolean    value)
{
    g_object_set_data (G_OBJECT (object), id, GINT_TO_POINTER (value));
}

static gboolean
get_bool (gpointer    object,
          const char *id)
{
    return g_object_get_data (G_OBJECT (object), id) != NULL;
}


static void
moo_action_base_set_display_name (MooActionBase *ab,
                                  const char    *name)
{
    GtkStockItem stock_item;
    char *freeme = NULL;

    g_return_if_fail (MOO_IS_ACTION_BASE (ab));
    g_return_if_fail (name != NULL);

    if (gtk_stock_lookup (name, &stock_item))
    {
        freeme = strip_underscore (stock_item.label);
        name = freeme;
    }

    set_string (ab, "moo-action-display-name", name);
    g_object_notify (G_OBJECT (ab), "display-name");

    g_free (freeme);
}

const char *
_moo_action_get_display_name (gpointer action)
{
    const char *display_name;

    g_return_val_if_fail (MOO_IS_ACTION_BASE (action), NULL);

    display_name = get_string (action, "moo-action-display-name");

    if (!display_name)
        display_name = gtk_action_get_name (GTK_ACTION (action));

    return display_name;
}


static void
moo_action_base_set_default_accel (MooActionBase *ab,
                                   const char    *accel)
{
    g_return_if_fail (MOO_IS_ACTION_BASE (ab));

    if (accel && !accel[0])
        accel = NULL;

    set_string (ab, "moo-action-default-accel", accel);
    g_object_notify (G_OBJECT (ab), "default-accel");
}

static const char *
moo_action_base_get_default_accel (MooActionBase *ab)
{
    const char *accel;

    g_return_val_if_fail (MOO_IS_ACTION_BASE (ab), "");

    accel = get_string (ab, "moo-action-default-accel");

    if (!accel)
        accel = "";

    return accel;
}


void
_moo_action_set_no_accel (gpointer action,
                          gboolean no_accel)
{
    g_return_if_fail (MOO_IS_ACTION_BASE (action));
    set_bool (action, "moo-action-no-accel", no_accel);
    g_object_notify (G_OBJECT (action), "no-accel");
}

gboolean
_moo_action_get_no_accel (gpointer action)
{
    g_return_val_if_fail (MOO_IS_ACTION_BASE (action), FALSE);
    return get_bool (action, "moo-action-no-accel");
}


static void
moo_action_base_set_connect_accel (gpointer action,
                                   gboolean connect)
{
    g_return_if_fail (MOO_IS_ACTION_BASE (action));
    set_bool (action, "moo-action-connect-accel", connect);
    g_object_notify (G_OBJECT (action), "connect-accel");
}

gboolean
_moo_action_get_connect_accel (gpointer action)
{
    g_return_val_if_fail (MOO_IS_ACTION_BASE (action), FALSE);
    return get_bool (action, "moo-action-connect-accel");
}


static void
moo_action_base_set_accel_editable (gpointer action,
                                    gboolean editable)
{
    g_return_if_fail (MOO_IS_ACTION_BASE (action));
    set_bool (action, "moo-action-accel-editable", editable);
    g_object_notify (G_OBJECT (action), "accel-editable");
}

gboolean
_moo_action_get_accel_editable (gpointer action)
{
    g_return_val_if_fail (MOO_IS_ACTION_BASE (action), FALSE);
    return get_bool (action, "moo-action-accel-editable");
}


static void
moo_action_base_set_force_accel_label (MooActionBase *ab,
                                       gboolean       force)
{
    g_return_if_fail (MOO_IS_ACTION_BASE (ab));
    set_bool (ab, "moo-action-force-accel-label", force);
    g_object_notify (G_OBJECT (ab), "force-accel-label");
}

static gboolean
moo_action_base_get_force_accel_label (MooActionBase *ab)
{
    g_return_val_if_fail (MOO_IS_ACTION_BASE (ab), FALSE);
    return get_bool (ab, "moo-action-force-accel-label");
}


static void
moo_action_base_set_dead (MooActionBase *ab, gboolean dead)
{
    g_return_if_fail (MOO_IS_ACTION_BASE (ab));
    set_bool (ab, "moo-action-dead", dead);
    g_object_notify (G_OBJECT (ab), "dead");
}

gboolean
_moo_action_get_dead (gpointer action)
{
    g_return_val_if_fail (MOO_IS_ACTION_BASE (action), FALSE);
    return get_bool (action, "moo-action-dead");
}


static void
moo_action_base_set_active (MooActionBase *ab, gboolean active)
{
    g_return_if_fail (MOO_IS_ACTION_BASE (ab));
    g_object_set (ab, "visible", active, "sensitive", active, NULL);
}


static void
moo_action_base_set_has_submenu (MooActionBase *ab, gboolean has_submenu)
{
    g_return_if_fail (MOO_IS_ACTION_BASE (ab));
    set_bool (ab, "moo-action-has-submenu", has_submenu);
    g_object_notify (G_OBJECT (ab), "has-submenu");
}

gboolean
_moo_action_get_has_submenu (gpointer action)
{
    g_return_val_if_fail (MOO_IS_ACTION_BASE (action), FALSE);
    return get_bool (action, "moo-action-has-submenu");
}


static void
sync_proxies_use_underline (gpointer action,
                            gboolean use_underline)
{
    GSList *proxies;

    proxies = g_slist_copy (gtk_action_get_proxies (GTK_ACTION (action)));
    g_slist_foreach (proxies, (GFunc) g_object_ref, NULL);

    while (proxies)
    {
        proxy_set_use_underline (GTK_WIDGET (proxies->data), use_underline);
        g_object_unref (proxies->data);
        proxies = g_slist_delete_link (proxies, proxies);
    }
}

static void
moo_action_base_set_use_underline (gpointer action,
                                   gboolean use_underline)
{
    g_return_if_fail (MOO_IS_ACTION_BASE (action));
    set_bool (action, "moo-action-use-underline", use_underline);
    sync_proxies_use_underline (action, use_underline);
    g_object_notify (G_OBJECT (action), "use-underline");
}

static gboolean
moo_action_base_get_use_underline (gpointer action)
{
    g_return_val_if_fail (MOO_IS_ACTION_BASE (action), FALSE);
    return get_bool (action, "moo-action-use-underline");
}


static void
moo_action_base_set_label (MooActionBase *ab,
                           const char    *label)
{
    GtkStockItem stock_item;

    g_return_if_fail (MOO_IS_ACTION_BASE (ab));

    if (label && gtk_stock_lookup (label, &stock_item))
        label = stock_item.label;

    g_object_set (G_OBJECT (ab), "GtkAction::label", label, NULL);

    sync_proxies_use_underline (ab, moo_action_base_get_use_underline (ab));
}


static void
moo_action_base_set_tooltip (MooActionBase *ab,
                             const char    *tooltip)
{
    GtkStockItem stock_item;
    char *freeme = NULL;

    g_return_if_fail (MOO_IS_ACTION_BASE (ab));

    if (tooltip && gtk_stock_lookup (tooltip, &stock_item))
    {
        freeme = strip_underscore (stock_item.label);
        tooltip = freeme;
    }

    g_object_set (G_OBJECT (ab), "GtkAction::tooltip", tooltip, NULL);
    g_free (freeme);
}


void
_moo_action_base_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    MooActionBase *ab = MOO_ACTION_BASE (object);

    switch (property_id)
    {
        case MOO_ACTION_BASE_PROP_DISPLAY_NAME:
            moo_action_base_set_display_name (ab, g_value_get_string (value));
            break;
        case MOO_ACTION_BASE_PROP_DEFAULT_ACCEL:
            moo_action_base_set_default_accel (ab, g_value_get_string (value));
            break;
        case MOO_ACTION_BASE_PROP_CONNECT_ACCEL:
            moo_action_base_set_connect_accel (ab, g_value_get_boolean (value));
            break;
        case MOO_ACTION_BASE_PROP_NO_ACCEL:
            _moo_action_set_no_accel (ab, g_value_get_boolean (value));
            break;
        case MOO_ACTION_BASE_PROP_ACCEL_EDITABLE:
            moo_action_base_set_accel_editable (ab, g_value_get_boolean (value));
            break;
        case MOO_ACTION_BASE_PROP_FORCE_ACCEL_LABEL:
            moo_action_base_set_force_accel_label (ab, g_value_get_boolean (value));
            break;
        case MOO_ACTION_BASE_PROP_DEAD:
            moo_action_base_set_dead (ab, g_value_get_boolean (value));
            break;
        case MOO_ACTION_BASE_PROP_ACTIVE:
            moo_action_base_set_active (ab, g_value_get_boolean (value));
            break;
        case MOO_ACTION_BASE_PROP_HAS_SUBMENU:
            moo_action_base_set_has_submenu (ab, g_value_get_boolean (value));
            break;
        case MOO_ACTION_BASE_PROP_LABEL:
            moo_action_base_set_label (ab, g_value_get_string (value));
            break;
        case MOO_ACTION_BASE_PROP_TOOLTIP:
            moo_action_base_set_tooltip (ab, g_value_get_string (value));
            break;
        case MOO_ACTION_BASE_PROP_USE_UNDERLINE:
            moo_action_base_set_use_underline (ab, g_value_get_boolean (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


void
_moo_action_base_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
    MooActionBase *ab = MOO_ACTION_BASE (object);

    switch (property_id)
    {
        case MOO_ACTION_BASE_PROP_DISPLAY_NAME:
            g_value_set_string (value, _moo_action_get_display_name (ab));
            break;
        case MOO_ACTION_BASE_PROP_DEFAULT_ACCEL:
            g_value_set_string (value, moo_action_base_get_default_accel (ab));
            break;
        case MOO_ACTION_BASE_PROP_CONNECT_ACCEL:
            g_value_set_boolean (value, _moo_action_get_connect_accel (ab));
            break;
        case MOO_ACTION_BASE_PROP_NO_ACCEL:
            g_value_set_boolean (value, _moo_action_get_no_accel (ab));
            break;
        case MOO_ACTION_BASE_PROP_ACCEL_EDITABLE:
            g_value_set_boolean (value, _moo_action_get_accel_editable (ab));
            break;
        case MOO_ACTION_BASE_PROP_FORCE_ACCEL_LABEL:
            g_value_set_boolean (value, moo_action_base_get_force_accel_label (ab));
            break;
        case MOO_ACTION_BASE_PROP_DEAD:
            g_value_set_boolean (value, _moo_action_get_dead (ab));
            break;
        case MOO_ACTION_BASE_PROP_HAS_SUBMENU:
            g_value_set_boolean (value, _moo_action_get_has_submenu (ab));
            break;
        case MOO_ACTION_BASE_PROP_USE_UNDERLINE:
            g_value_set_boolean (value, moo_action_base_get_use_underline (ab));
            break;

        case MOO_ACTION_BASE_PROP_LABEL:
            g_object_get_property (object, "GtkAction::label", value);
            break;
        case MOO_ACTION_BASE_PROP_TOOLTIP:
            g_object_get_property (object, "GtkAction::tooltip", value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


GtkActionGroup *
_moo_action_get_group (gpointer action)
{
    GtkActionGroup *group = NULL;

    g_return_val_if_fail (GTK_IS_ACTION (action), NULL);

    g_object_get (action, "action-group", &group, NULL);

    if (group)
        g_object_unref (group);

    return group;
}


char *
_moo_action_make_accel_path (gpointer action)
{
    GtkActionGroup *group = NULL;
    MooActionCollection *collection;
    const char *name, *group_name, *collection_name;

    g_return_val_if_fail (MOO_IS_ACTION_BASE (action), NULL);

    group = _moo_action_get_group (action);
    g_return_val_if_fail (MOO_IS_ACTION_GROUP (group), NULL);
    collection = _moo_action_group_get_collection (MOO_ACTION_GROUP (group));
    g_return_val_if_fail (MOO_IS_ACTION_COLLECTION (collection), NULL);

    name = gtk_action_get_name (GTK_ACTION (action));
    group_name = gtk_action_group_get_name (group);
    collection_name = moo_action_collection_get_name (collection);

    g_return_val_if_fail (collection_name != NULL, NULL);
    g_return_val_if_fail (name != NULL && name[0] != 0, NULL);

    if (group_name)
        return g_strdup_printf ("<MooAction>/%s/%s/%s", collection_name, group_name, name);
    else
        return g_strdup_printf ("<MooAction>/%s/%s", collection_name, name);
}


void
_moo_action_set_accel_path (gpointer    action,
                            const char *accel_path)
{
    g_return_if_fail (MOO_IS_ACTION_BASE (action));
    gtk_action_set_accel_path (GTK_ACTION (action), accel_path);
}


const char *
_moo_action_get_accel_path (gpointer action)
{
    g_return_val_if_fail (MOO_IS_ACTION_BASE (action), NULL);
    return gtk_action_get_accel_path (GTK_ACTION (action));
}


const char *
_moo_action_get_default_accel (gpointer action)
{
    g_return_val_if_fail (MOO_IS_ACTION_BASE (action), "");
    return moo_action_base_get_default_accel (MOO_ACTION_BASE (action));
}


void
_moo_action_base_connect_proxy (GtkAction *action,
                                GtkWidget *proxy)
{
    g_return_if_fail (MOO_IS_ACTION_BASE (action));
    g_return_if_fail (GTK_IS_WIDGET (proxy));
    proxy_set_use_underline (proxy, moo_action_base_get_use_underline (action));
}

static void
proxy_set_use_underline (GtkWidget *proxy,
                         gboolean   use_underline)
{
    g_return_if_fail (GTK_IS_WIDGET (proxy));

    if (GTK_IS_MENU_ITEM (proxy) && GTK_BIN (proxy)->child && GTK_IS_LABEL (GTK_BIN (proxy)->child))
        gtk_label_set_use_underline (GTK_LABEL (GTK_BIN (proxy)->child), use_underline);
    else if (GTK_IS_BUTTON (proxy))
        gtk_button_set_use_underline (GTK_BUTTON (proxy), use_underline);
}


void
_moo_action_base_init_instance (gpointer action)
{
    g_return_if_fail (MOO_IS_ACTION_BASE (action));
    set_bool (action, "moo-action-use-underline", TRUE);
}
