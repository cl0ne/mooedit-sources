/*
 *   mooaccelbutton.h
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

#ifndef MOO_ACCEL_BUTTON_H
#define MOO_ACCEL_BUTTON_H

#include <gtk/gtk.h>


G_BEGIN_DECLS

#define MOO_TYPE_ACCEL_BUTTON             (_moo_accel_button_get_type ())
#define MOO_ACCEL_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOO_TYPE_ACCEL_BUTTON, MooAccelButton))
#define MOO_ACCEL_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_ACCEL_BUTTON, MooAccelButtonClass))
#define MOO_IS_ACCEL_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOO_TYPE_ACCEL_BUTTON))
#define MOO_IS_ACCEL_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_ACCEL_BUTTON))
#define MOO_ACCEL_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_ACCEL_BUTTON, MooAccelButtonClass))

typedef struct _MooAccelButton        MooAccelButton;
typedef struct _MooAccelButtonClass   MooAccelButtonClass;

struct _MooAccelButton {
    GtkButton parent;

    char *accel;
    char *title;
};

struct _MooAccelButtonClass {
    GtkButtonClass parent_class;

    void (* accel_set) (MooAccelButton  *button,
                        const char      *accel);
};


GType        _moo_accel_button_get_type         (void) G_GNUC_CONST;

const char  *_moo_accel_button_get_accel        (MooAccelButton     *button);
gboolean     _moo_accel_button_set_accel        (MooAccelButton     *button,
                                                 const char         *accel);


G_END_DECLS


#endif /* MOO_ACCEL_BUTTON_H */
