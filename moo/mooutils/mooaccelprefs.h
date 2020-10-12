/*
 *   mooaccelprefs.c
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

#ifndef MOO_ACCEL_PREFS_H
#define MOO_ACCEL_PREFS_H

#include <mooutils/mooactioncollection.h>

G_BEGIN_DECLS


gboolean _moo_accel_prefs_dialog_run (MooActionCollection *coll,
                                      GtkWidget           *parent);


G_END_DECLS

#endif /* MOO_ACCEL_PREFS_H */
