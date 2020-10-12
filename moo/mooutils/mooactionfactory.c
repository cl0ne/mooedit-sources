/*
 *   mooactionfactory.c
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

#include "mooutils/mooactionfactory.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/mooaction.h"
#include "mooutils/mooactionbase.h"
#include <gobject/gvaluecollector.h>
#include <string.h>


static MooActionFactory *moo_action_factory_new_valist  (GType       action_type,
                                                         const char *first_prop_name,
                                                         va_list     var_args);


G_DEFINE_TYPE(MooActionFactory, moo_action_factory, G_TYPE_OBJECT)


static void
moo_action_factory_dispose (GObject *object)
{
    MooActionFactory *factory = MOO_ACTION_FACTORY (object);

    if (factory->props)
    {
        _moo_param_array_free (factory->props, factory->n_props);
        factory->props = NULL;
        factory->n_props = 0;
    }

    G_OBJECT_CLASS(moo_action_factory_parent_class)->dispose (object);
}


static void
moo_action_factory_class_init (MooActionFactoryClass *klass)
{
    G_OBJECT_CLASS(klass)->dispose = moo_action_factory_dispose;
}


static void
moo_action_factory_init (MooActionFactory *factory)
{
    factory->action_type = MOO_TYPE_ACTION;
}


static GtkAction *
moo_action_new_valist (GType       action_type,
                       const char *name,
                       const char *first_prop_name,
                       va_list     var_args)
{
    MooActionFactory *factory;
    GtkAction *action;

    g_return_val_if_fail (g_type_is_a (action_type, MOO_TYPE_ACTION_BASE), NULL);

    factory = moo_action_factory_new_valist (action_type, first_prop_name, var_args);
    action = moo_action_factory_create_action (factory, NULL, "name", name, NULL);

    g_object_unref (factory);
    return action;
}


GtkAction *
moo_action_group_add_action (GtkActionGroup *group,
                             const char     *name,
                             const char     *first_prop_name,
                             ...)
{
    GtkAction *action;
    GType action_type = MOO_TYPE_ACTION;
    va_list var_args;

    g_return_val_if_fail (GTK_IS_ACTION_GROUP (group), NULL);

    va_start (var_args, first_prop_name);

    if (first_prop_name &&
        (!strcmp (first_prop_name, "action-type::") || !strcmp (first_prop_name, "action_type::")))
    {
        action_type = va_arg (var_args, GType);
        g_return_val_if_fail (g_type_is_a (action_type, MOO_TYPE_ACTION_BASE), NULL);
        first_prop_name = va_arg (var_args, char*);
    }

    action = moo_action_new_valist (action_type, name, first_prop_name, var_args);

    va_end (var_args);

    g_return_val_if_fail (action != NULL, NULL);

    gtk_action_group_add_action (group, action);
    g_object_unref (action);

    return action;
}


static gboolean
collect_valist (GType        type,
                GParameter **props_p,
                guint       *n_props_p,
                const char  *first_prop_name,
                va_list      var_args)
{
    GObjectClass *klass;
    GArray *props;
    const char *prop_name;

    g_return_val_if_fail (first_prop_name != NULL, FALSE);

    klass = G_OBJECT_CLASS (g_type_class_ref (type));
    g_return_val_if_fail (klass != NULL, FALSE);

    props = g_array_new (FALSE, TRUE, sizeof (GParameter));
    prop_name = first_prop_name;

    while (prop_name)
    {
        char *error = NULL;
        GParameter param;
        GParamSpec *pspec;

        pspec = g_object_class_find_property (klass, prop_name);

        if (!pspec)
        {
            g_warning ("could not find property '%s' for class '%s'",
                       prop_name, g_type_name (type));

            _moo_param_array_free ((GParameter*) props->data, props->len);
            g_array_free (props, FALSE);

            g_type_class_unref (klass);
            return FALSE;
        }

        param.name = g_strdup (prop_name);
        param.value.g_type = 0;
        g_value_init (&param.value, G_PARAM_SPEC_VALUE_TYPE (pspec));
        G_VALUE_COLLECT (&param.value, var_args, 0, &error);

        if (error)
        {
            g_critical ("%s", error);
            g_free (error);
            g_value_unset (&param.value);
            g_free ((char*)param.name);

            _moo_param_array_free ((GParameter*) props->data, props->len);
            g_array_free (props, FALSE);

            g_type_class_unref (klass);
            return FALSE;
        }

        g_array_append_val (props, param);

        prop_name = va_arg (var_args, char*);
    }

    g_type_class_unref (klass);

    *n_props_p = props->len;
    *props_p = (GParameter*) g_array_free (props, FALSE);

    return TRUE;
}


static MooActionFactory *
moo_action_factory_new_valist (GType       action_type,
                               const char *first_prop_name,
                               va_list     var_args)
{
    MooActionFactory *factory;

    g_return_val_if_fail (g_type_is_a (action_type, MOO_TYPE_ACTION_BASE), NULL);

    factory = MOO_ACTION_FACTORY (g_object_new (MOO_TYPE_ACTION_FACTORY, (const char*) NULL));

    factory->action_type = action_type;

    if (!collect_valist (action_type,
                         &factory->props, &factory->n_props,
                         first_prop_name, var_args))
    {
        g_object_unref (factory);
        return NULL;
    }

    return factory;
}


static void
array_add_parameter (GArray     *array,
                     GParameter *param)
{
    GParameter p;
    p.name = g_strdup (param->name);
    p.value.g_type = 0;
    g_value_init (&p.value, G_VALUE_TYPE (&param->value));
    g_value_copy (&param->value, &p.value);
    g_array_append_val (array, p);
}


static GParameter *
param_array_concatenate (GParameter *props1,
                         guint       n_props1,
                         GParameter *props2,
                         guint       n_props2,
                         guint      *n_props)
{
    GArray *array;
    guint i;

    array = g_array_new (FALSE, TRUE, sizeof (GParameter));

    for (i = 0; i < n_props1; ++i)
        array_add_parameter (array, &props1[i]);
    for (i = 0; i < n_props2; ++i)
        array_add_parameter (array, &props2[i]);

    *n_props = array->len;
    return (GParameter*) g_array_free (array, FALSE);
}


GtkAction *
moo_action_factory_create_action (MooActionFactory   *factory,
                                  gpointer            data,
                                  const char         *prop_name,
                                  ...)
{
    GObject *object;
    GParameter *props, *add_props;
    guint n_props, n_add_props;
    va_list var_args;
    gboolean success;

    g_return_val_if_fail (MOO_IS_ACTION_FACTORY (factory), NULL);

    if (factory->factory_func)
    {
        if (factory->factory_func_data)
            data = factory->factory_func_data;
        return factory->factory_func (data, factory);
    }

    if (!prop_name)
        return GTK_ACTION (g_object_newv (factory->action_type,
                                          factory->n_props,
                                          factory->props));

    va_start (var_args, prop_name);

    success = collect_valist (factory->action_type,
                              &add_props, &n_add_props,
                              prop_name, var_args);

    va_end (var_args);

    if (!success)
        return NULL;

    props = param_array_concatenate (factory->props,
                                     factory->n_props,
                                     add_props,
                                     n_add_props,
                                     &n_props);

    object = G_OBJECT (g_object_newv (factory->action_type, n_props, props));

    _moo_param_array_free (props, n_props);
    _moo_param_array_free (add_props, n_add_props);

    return GTK_ACTION (object);
}


#if 0
MooActionFactory *
moo_action_factory_new (GType       action_type,
                        const char *first_prop_name,
                        ...)
{
    MooActionFactory *factory;
    va_list var_args;

    g_return_val_if_fail (g_type_is_a (action_type, MOO_TYPE_ACTION_BASE), NULL);

    va_start (var_args, first_prop_name);
    factory = moo_action_factory_new_valist (action_type, first_prop_name, var_args);
    va_end (var_args);

    return factory;
}
#endif


MooActionFactory *
moo_action_factory_new_a (GType       action_type,
                          GParameter *params,
                          guint       n_params)
{
    MooActionFactory *factory;
    GObjectClass *klass;
    GArray *props;
    guint i;

    g_return_val_if_fail (g_type_is_a (action_type, MOO_TYPE_ACTION_BASE), NULL);

    klass = G_OBJECT_CLASS (g_type_class_ref (action_type));
    props = g_array_new (FALSE, TRUE, sizeof (GParameter));

    for (i = 0; i < n_params; ++i)
    {
        GParameter param;
        GParamSpec *pspec;
        const char *prop_name = params[i].name;

        pspec = g_object_class_find_property (klass, prop_name);

        if (!pspec)
        {
            g_warning ("could not find property '%s' for class '%s'",
                       prop_name, g_type_name (action_type));

            _moo_param_array_free ((GParameter*) props->data, props->len);
            g_array_free (props, FALSE);

            g_type_class_unref (klass);
            return NULL;
        }

        param.name = g_strdup (prop_name);
        param.value.g_type = 0;
        g_value_init (&param.value, G_PARAM_SPEC_VALUE_TYPE (pspec));
        g_value_copy (&params[i].value, &param.value);

        g_array_append_val (props, param);
    }

    g_type_class_unref (klass);

    factory = MOO_ACTION_FACTORY (g_object_new (MOO_TYPE_ACTION_FACTORY, (const char*) NULL));
    factory->action_type = action_type;

    factory->n_props = props->len;
    factory->props = (GParameter*) g_array_free (props, FALSE);

    return factory;
}


MooActionFactory*
moo_action_factory_new_func (MooActionFactoryFunc factory_func,
                             gpointer             data)
{
    MooActionFactory *factory;

    g_return_val_if_fail (factory_func != NULL, NULL);

    factory = MOO_ACTION_FACTORY (g_object_new (MOO_TYPE_ACTION_FACTORY, (const char*) NULL));
    factory->factory_func = factory_func;
    factory->factory_func_data = data;

    return factory;
}
