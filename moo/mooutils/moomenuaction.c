/*
 *   moomenuaction.c
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
 * class:MooMenuAction: (parent MooAction) (moo.private 1)
 **/

#include "mooutils/moomenuaction.h"
#include "mooutils/mooaction-private.h"
#include "marshals.h"
#include <gtk/gtk.h>
#include <string.h>


static void moo_menu_action_class_init      (MooMenuActionClass *klass);

static void moo_menu_action_init            (MooMenuAction      *action);
static void moo_menu_action_finalize        (GObject            *object);

static void moo_menu_action_set_property    (GObject            *object,
                                             guint               prop_id,
                                             const GValue       *value,
                                             GParamSpec         *pspec);
static void moo_menu_action_get_property    (GObject            *object,
                                             guint               prop_id,
                                             GValue             *value,
                                             GParamSpec         *pspec);

static GtkWidget *moo_menu_action_create_menu_item (GtkAction   *action);

static void data_destroyed                  (MooMenuAction      *action,
                                             gpointer            data);

enum {
    PROP_0,
    PROP_MENU_MGR,
    PROP_MENU_FUNC
};


/* MOO_TYPE_MENU_ACTION */
G_DEFINE_TYPE (MooMenuAction, moo_menu_action, MOO_TYPE_ACTION)


static void
moo_menu_action_class_init (MooMenuActionClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkActionClass *action_class = GTK_ACTION_CLASS (klass);

    gobject_class->set_property = moo_menu_action_set_property;
    gobject_class->get_property = moo_menu_action_get_property;
    gobject_class->finalize = moo_menu_action_finalize;

    action_class->create_menu_item = moo_menu_action_create_menu_item;

    g_object_class_install_property (gobject_class,
                                     PROP_MENU_MGR,
                                     g_param_spec_object ("menu-mgr",
                                             "menu-mgr",
                                             "menu-mgr",
                                             MOO_TYPE_MENU_MGR,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_MENU_FUNC,
                                     g_param_spec_pointer ("menu-func",
                                             "menu-func",
                                             "menu-func",
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));
}


static void
moo_menu_action_init (MooMenuAction *action)
{
    action->mgr = NULL;
    action->func = NULL;
    _moo_action_set_no_accel (GTK_ACTION (action), TRUE);
}


static void
moo_menu_action_get_property (GObject        *object,
                              guint           prop_id,
                              GValue         *value,
                              GParamSpec     *pspec)
{
    MooMenuAction *action = MOO_MENU_ACTION (object);

    switch (prop_id)
    {
        case PROP_MENU_MGR:
            g_value_set_object (value, moo_menu_action_get_mgr (action));
            break;

        case PROP_MENU_FUNC:
            g_value_set_pointer (value, (gpointer) action->func);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
moo_menu_action_set_property (GObject        *object,
                              guint           prop_id,
                              const GValue   *value,
                              GParamSpec     *pspec)
{
    MooMenuAction *action = MOO_MENU_ACTION (object);

    switch (prop_id)
    {
        case PROP_MENU_MGR:
            moo_menu_action_set_mgr (action, (MooMenuMgr*) g_value_get_object (value));
            break;

        case PROP_MENU_FUNC:
            moo_menu_action_set_func (action, (MooMenuFunc) g_value_get_pointer (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static GtkWidget *
moo_menu_action_create_menu_item (GtkAction *action)
{
    MooMenuAction *menu_action;
    GtkWidget *item;
    gpointer data = NULL;
    GDestroyNotify destroy = NULL;
    char *label = NULL;

    menu_action = MOO_MENU_ACTION (action);

    if (menu_action->func)
        return menu_action->func (GTK_ACTION (menu_action));

    if (menu_action->data && menu_action->is_object)
    {
        data = g_object_ref (menu_action->data);
        destroy = g_object_unref;
    }
    else
    {
        data = menu_action->data;
        destroy = NULL;
    }

    g_object_get (action, "label", &label, NULL);

    item = moo_menu_mgr_create_item (moo_menu_action_get_mgr (menu_action),
                                     label, (MooMenuItemFlags) 0, data, destroy);

    g_free (label);
    return item;
}


GtkAction *
moo_menu_action_new (const char *id,
                     const char *label)
{
    return GTK_ACTION (g_object_new (MOO_TYPE_MENU_ACTION,
                                     "name", id, "label", label,
                                     (const char*) NULL));
}


MooMenuMgr *
moo_menu_action_get_mgr (MooMenuAction *action)
{
    g_return_val_if_fail (MOO_IS_MENU_ACTION (action), NULL);

    if (!action->func && !action->mgr)
        action->mgr = moo_menu_mgr_new ();

    return action->mgr;
}


void
moo_menu_action_set_mgr (MooMenuAction *action,
                         MooMenuMgr    *mgr)
{
    g_return_if_fail (MOO_IS_MENU_ACTION (action));
    g_return_if_fail (!mgr || MOO_IS_MENU_MGR (mgr));

    if (mgr != action->mgr)
    {
        if (action->mgr)
            g_object_unref (action->mgr);

        if (mgr)
        {
            action->mgr = MOO_MENU_MGR (g_object_ref (mgr));
            action->func = NULL;
        }
        else
        {
            action->mgr = NULL;
        }

        g_object_freeze_notify (G_OBJECT (action));
        g_object_notify (G_OBJECT (action), "menu-func");
        g_object_notify (G_OBJECT (action), "menu-mgr");
        g_object_thaw_notify (G_OBJECT (action));
    }
}


static void
moo_menu_action_finalize (GObject *object)
{
    MooMenuAction *action = MOO_MENU_ACTION (object);

    if (action->data && action->is_object)
        g_object_weak_unref (G_OBJECT (action->data), (GWeakNotify) data_destroyed, action);

    G_OBJECT_CLASS(moo_menu_action_parent_class)->finalize (object);
}


static void
data_destroyed (MooMenuAction      *action,
                gpointer            data)
{
    g_return_if_fail (action->data == data);
    action->data = NULL;
}


void
moo_menu_action_set_menu_data (MooMenuAction  *action,
                               gpointer        data,
                               gboolean        is_object)
{
    g_return_if_fail (MOO_IS_MENU_ACTION (action));

    if (action->data && action->is_object)
        g_object_weak_unref (G_OBJECT (action->data), (GWeakNotify) data_destroyed, action);

    action->data = data;
    action->is_object = is_object;

    if (action->data && action->is_object)
        g_object_weak_ref (G_OBJECT (action->data), (GWeakNotify) data_destroyed, action);
}


void
moo_menu_action_set_func (MooMenuAction *action,
                          MooMenuFunc    func)
{
    g_return_if_fail (MOO_IS_MENU_ACTION (action));

    if (func != action->func)
    {
        if (action->mgr)
        {
            g_object_unref (action->mgr);
            action->mgr = NULL;
        }

        action->func = func;

        g_object_freeze_notify (G_OBJECT (action));
        g_object_notify (G_OBJECT (action), "menu-func");
        g_object_notify (G_OBJECT (action), "menu-mgr");
        g_object_thaw_notify (G_OBJECT (action));
    }
}
