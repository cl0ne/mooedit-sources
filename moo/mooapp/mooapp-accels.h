/*
 *   mooapp-accels.h
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

#ifndef MOO_APP_ACCELS_H
#define MOO_APP_ACCELS_H

#include <gtk/gtk.h>

#ifndef GDK_WINDOWING_QUARTZ

#define MOO_APP_ACCEL_HELP "F1"
#define MOO_APP_ACCEL_QUIT "<Ctrl>Q"

#else /* GDK_WINDOWING_QUARTZ */

#define MOO_APP_ACCEL_HELP "<Meta>question"
#define MOO_APP_ACCEL_QUIT "<Meta>Q"

#endif /* GDK_WINDOWING_QUARTZ */

#endif /* MOO_APP_ACCELS_H */
