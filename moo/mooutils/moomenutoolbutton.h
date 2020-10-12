/*
 *   moomenutoolbutton.h
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

#ifndef MOO_MENU_TOOL_BUTTON_H
#define MOO_MENU_TOOL_BUTTON_H

#include <gtk/gtk.h>


G_BEGIN_DECLS

#define MOO_TYPE_MENU_TOOL_BUTTON             (moo_menu_tool_button_get_type ())
#define MOO_MENU_TOOL_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOO_TYPE_MENU_TOOL_BUTTON, MooMenuToolButton))
#define MOO_MENU_TOOL_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_MENU_TOOL_BUTTON, MooMenuToolButtonClass))
#define MOO_IS_MENU_TOOL_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOO_TYPE_MENU_TOOL_BUTTON))
#define MOO_IS_MENU_TOOL_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_MENU_TOOL_BUTTON))
#define MOO_MENU_TOOL_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_MENU_TOOL_BUTTON, MooMenuToolButtonClass))

typedef struct _MooMenuToolButton        MooMenuToolButton;
typedef struct _MooMenuToolButtonClass   MooMenuToolButtonClass;

struct _MooMenuToolButton {
    GtkToggleToolButton parent;
    GtkWidget *menu;
};

struct _MooMenuToolButtonClass {
    GtkToggleToolButtonClass parent_class;
};


GType        moo_menu_tool_button_get_type  (void) G_GNUC_CONST;
GtkWidget   *moo_menu_tool_button_new       (void);

void         moo_menu_tool_button_set_menu  (MooMenuToolButton  *button,
                                             GtkWidget          *menu);
GtkWidget   *moo_menu_tool_button_get_menu  (MooMenuToolButton  *button);


G_END_DECLS


#endif /* MOO_MENU_TOOL_BUTTON_H */
