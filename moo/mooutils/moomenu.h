/*
 *   moomenu.h
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

#ifndef MOO_MENU_H
#define MOO_MENU_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MOO_TYPE_MENU              (moo_menu_get_type ())
#define MOO_MENU(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_MENU, MooMenu))
#define MOO_MENU_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_MENU, MooMenuClass))
#define MOO_IS_MENU(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_MENU))
#define MOO_IS_MENU_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_MENU))
#define MOO_MENU_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_MENU, MooMenuClass))

typedef struct _MooMenu      MooMenu;
typedef struct _MooMenuClass MooMenuClass;

struct _MooMenu
{
    GtkMenu base;
};

struct _MooMenuClass
{
    GtkMenuClass base_class;

    void (*alternate_toggled) (MooMenu *menu);
};

GType        moo_menu_get_type  (void) G_GNUC_CONST;
GtkWidget   *moo_menu_new       (void);

G_END_DECLS

#endif /* MOO_MENU_H */
