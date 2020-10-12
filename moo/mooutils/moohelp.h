/*
 *   moohelp.h
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

#ifndef MOO_HELP_H
#define MOO_HELP_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MOO_HELP_ID_CONTENTS "contents"
#define MOO_HELP_ID_INDEX    "index"

typedef gboolean (*MooHelpFunc) (GtkWidget *widget,
                                 gpointer   data);

gboolean    moo_help_open               (GtkWidget     *widget);
void        moo_help_open_any           (GtkWidget     *widget);
void        moo_help_open_id            (const char    *id,
                                         GtkWidget     *parent);

void        moo_help_set_id             (GtkWidget     *widget,
                                         const char    *id);
void        moo_help_set_func           (GtkWidget     *widget,
                                         gboolean (*func) (GtkWidget*));
void        moo_help_set_func_full      (GtkWidget     *widget,
                                         MooHelpFunc    func,
                                         gpointer       func_data,
                                         GDestroyNotify notify);

void        moo_help_connect_keys       (GtkWidget     *widget);

G_END_DECLS

#endif /* MOO_HELP_H */
