/*
 *   mooeditaction-factory.h
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

#ifndef MOO_EDIT_ACTION_FACTORY_H
#define MOO_EDIT_ACTION_FACTORY_H

#include <mooutils/mooaction.h>
#include <mooedit/mooedit.h>

G_BEGIN_DECLS


void    moo_edit_class_new_action           (MooEditClass       *klass,
                                             const char         *id,
                                             const char         *first_prop_name,
                                             ...) G_GNUC_NULL_TERMINATED;
void    moo_edit_class_new_action_type      (MooEditClass       *klass,
                                             const char         *id,
                                             GType               type);

void    moo_edit_class_remove_action        (MooEditClass       *klass,
                                             const char         *id);

GtkActionGroup *moo_edit_get_actions        (MooEdit            *edit);
GtkAction *moo_edit_get_action_by_id        (MooEdit            *edit,
                                             const char         *action_id);


G_END_DECLS

#endif /* MOO_EDIT_ACTION_FACTORY_H */
