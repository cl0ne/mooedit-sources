/*
 *   mooactiongroup.c
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

#include "mooutils/mooactiongroup.h"
#include "mooutils/mooutils-misc.h"


G_DEFINE_TYPE (MooActionGroup, _moo_action_group, GTK_TYPE_ACTION_GROUP)


const char *
_moo_action_group_get_display_name (MooActionGroup *group)
{
    const char *name;

    g_return_val_if_fail (MOO_IS_ACTION_GROUP (group), NULL);

    name = group->display_name;

    if (!name)
        name = gtk_action_group_get_name (GTK_ACTION_GROUP (group));

    if (!name)
        name = "Actions";

    return name;
}


void
_moo_action_group_set_display_name (MooActionGroup *group,
                                    const char     *display_name)
{
    g_return_if_fail (MOO_IS_ACTION_GROUP (group));
    MOO_ASSIGN_STRING (group->display_name, display_name);
}


static void
_moo_action_group_init (G_GNUC_UNUSED MooActionGroup *group)
{
}


static void
moo_action_group_finalize (GObject *object)
{
    MooActionGroup *group = MOO_ACTION_GROUP (object);

    g_free (group->display_name);

    G_OBJECT_CLASS (_moo_action_group_parent_class)->finalize (object);
}


static void
_moo_action_group_class_init (MooActionGroupClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = moo_action_group_finalize;
}


MooActionGroup *
_moo_action_group_new (MooActionCollection *collection,
                       const char          *name,
                       const char          *display_name)
{
    MooActionGroup *group = MOO_ACTION_GROUP (g_object_new (MOO_TYPE_ACTION_GROUP, "name", name, (const char*) NULL));
    group->display_name = g_strdup (display_name);
    group->collection = collection;
    return group;
}


MooActionCollection *
_moo_action_group_get_collection (MooActionGroup *group)
{
    g_return_val_if_fail (MOO_IS_ACTION_GROUP (group), NULL);
    return group->collection;
}


void
_moo_action_group_set_collection (MooActionGroup      *group,
                                  MooActionCollection *collection)
{
    g_return_if_fail (MOO_IS_ACTION_GROUP (group));
    g_return_if_fail (!collection || MOO_IS_ACTION_COLLECTION (collection));
    group->collection = collection;
}


#if 0
// void
// moo_action_group_set_name (MooActionGroup *group,
//                                 const char          *name)
// {
//     g_return_if_fail (MOO_IS_ACTION_GROUP (group));
//     action_group_set_display_name (group->priv->default_group, name);
// }
//
//
// const char *
// moo_action_group_get_name (MooActionGroup *group)
// {
//     g_return_val_if_fail (MOO_IS_ACTION_GROUP (group), NULL);
//     return action_group_get_display_name (group->priv->default_group);
// }
//
//
// static GtkActionGroup *
// get_group (MooActionGroup *group,
//            const char          *name)
// {
//     if (name)
//         return g_hash_table_lookup (group->priv->groups, name);
//     else
//         return group->priv->default_group;
// }
//
//
// GtkActionGroup *
// moo_action_group_add_group (MooActionGroup *group,
//                                  const char          *name,
//                                  const char          *display_name)
// {
//     MooActionGroup *group;
//
//     g_return_val_if_fail (MOO_IS_ACTION_GROUP (group), NULL);
//     g_return_val_if_fail (name != NULL, NULL);
//     g_return_val_if_fail (get_group (group, name) == NULL, NULL);
//
//     group = _moo_action_group_new (name, display_name);
//     g_hash_table_insert (group->priv->groups, g_strdup (name), group);
//     group->priv->groups_list = g_slist_prepend (group->priv->groups_list, group);
//
//     return GTK_ACTION_GROUP (group);
// }
//
//
// void
// moo_action_group_remove_group (MooActionGroup *group,
//                                     GtkActionGroup      *group)
// {
//     const char *name;
//
//     g_return_if_fail (MOO_IS_ACTION_GROUP (group));
//     g_return_if_fail (GTK_IS_ACTION_GROUP (group));
//
//     name = gtk_action_group_get_name (group);
//     g_return_if_fail (name != NULL);
//     g_return_if_fail (group == get_group (group, name));
//
//     g_hash_table_remove (group->priv->groups, name);
//     group->priv->groups_list = g_slist_remove (group->priv->groups_list, group);
// }
//
//
// GtkActionGroup *
// moo_action_group_get_group (MooActionGroup *group,
//                                  const char          *name)
// {
//     g_return_val_if_fail (MOO_IS_ACTION_GROUP (group), NULL);
//     return get_group (group, name);
// }
//
//
// const GSList *
// moo_action_group_get_groups (MooActionGroup *group)
// {
//     g_return_val_if_fail (MOO_IS_ACTION_GROUP (group), NULL);
//     return group->priv->groups_list;
// }
//
//
// GtkAction *
// moo_action_group_get_action (MooActionGroup *group,
//                                   const char          *name)
// {
//     GSList *l;
//
//     g_return_val_if_fail (MOO_IS_ACTION_GROUP (group), NULL);
//     g_return_val_if_fail (name != NULL, NULL);
//
//     for (l = group->priv->groups_list; l != NULL; l = l->next)
//     {
//         GtkActionGroup *group = l->data;
//         GtkAction *action = gtk_action_group_get_action (group, name);
//         if (name)
//             return action;
//     }
//
//     return NULL;
// }
//
//
// void
// moo_action_group_remove_action (MooActionGroup *group,
//                                      GtkAction           *action)
// {
//     GtkActionGroup *group = NULL;
//
//     g_return_if_fail (MOO_IS_ACTION_GROUP (group));
//     g_return_if_fail (GTK_IS_ACTION (action));
//
//     g_object_get (action, "action-group", &group, NULL);
//     g_return_if_fail (group != NULL);
//     g_return_if_fail (g_slist_find (group->priv->groups_list, group) != NULL);
//
//     gtk_action_group_remove_action (group, action);
// }
#endif
