/*
 *   mooactionfactory.h
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

#ifndef MOO_ACTION_FACTORY_H
#define MOO_ACTION_FACTORY_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_ACTION_FACTORY              (moo_action_factory_get_type ())
#define MOO_ACTION_FACTORY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_ACTION_FACTORY, MooActionFactory))
#define MOO_ACTION_FACTORY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_ACTION_FACTORY, MooActionFactoryClass))
#define MOO_IS_ACTION_FACTORY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_ACTION_FACTORY))
#define MOO_IS_ACTION_FACTORY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_ACTION_FACTORY))
#define MOO_ACTION_FACTORY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_ACTION_FACTORY, MooActionFactoryClass))


typedef struct _MooActionFactory        MooActionFactory;
typedef struct _MooActionFactoryClass   MooActionFactoryClass;

typedef GtkAction* (*MooActionFactoryFunc) (gpointer          data,
                                            MooActionFactory *factory);

struct _MooActionFactory
{
    GObject      object;

    MooActionFactoryFunc factory_func;
    gpointer             factory_func_data;

    GType       action_type;
    guint       n_props;
    GParameter *props;
};

struct _MooActionFactoryClass
{
    GObjectClass parent_class;
};


GType               moo_action_factory_get_type     (void) G_GNUC_CONST;

MooActionFactory   *moo_action_factory_new_func     (MooActionFactoryFunc factory_func,
                                                     gpointer            data);
MooActionFactory   *moo_action_factory_new_a        (GType               object_type,
                                                     GParameter         *params,
                                                     guint               n_params);

GtkAction          *moo_action_factory_create_action(MooActionFactory   *factory,
                                                     gpointer            data,
                                                     const char         *additional_prop_name,
                                                     ...) G_GNUC_NULL_TERMINATED;


GtkAction          *moo_action_group_add_action     (GtkActionGroup     *group,
                                                     const char         *name,
                                                     const char         *first_prop_name,
                                                     ...) G_GNUC_NULL_TERMINATED;


G_END_DECLS

#endif /* MOO_ACTION_FACTORY_H */
