/*
 *   moofileview-accels.h
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

#ifndef MOO_FILE_VIEW_ACCELS_H
#define MOO_FILE_VIEW_ACCELS_H

#include <mooutils/mooaccel.h>

#define MOO_FILE_VIEW_ACCEL_CUT MOO_ACCEL_CUT
#define MOO_FILE_VIEW_ACCEL_COPY MOO_ACCEL_COPY
#define MOO_FILE_VIEW_ACCEL_PASTE MOO_ACCEL_PASTE

#ifndef GDK_WINDOWING_QUARTZ

#define MOO_FILE_VIEW_ACCEL_GO_UP "<Alt>Up"
#define MOO_FILE_VIEW_ACCEL_GO_BACK "<Alt>Left"
#define MOO_FILE_VIEW_ACCEL_GO_FORWARD "<Alt>Right"
#define MOO_FILE_VIEW_ACCEL_GO_HOME "<Alt>Home"
#define MOO_FILE_VIEW_ACCEL_DELETE "<Alt>Delete"
#define MOO_FILE_VIEW_ACCEL_SHOW_HIDDEN "<Alt><Shift>H"
#define MOO_FILE_VIEW_ACCEL_PROPERTIES "<Alt>Return"

#define MOO_FILE_VIEW_BINDING_GO_UP GDK_Up, GDK_MOD1_MASK
#define MOO_FILE_VIEW_BINDING_GO_UP_KP GDK_KP_Up, GDK_MOD1_MASK
#define MOO_FILE_VIEW_BINDING_GO_BACK GDK_Left, GDK_MOD1_MASK
#define MOO_FILE_VIEW_BINDING_GO_BACK_KP GDK_KP_Left, GDK_MOD1_MASK
#define MOO_FILE_VIEW_BINDING_GO_FORWARD GDK_Right, GDK_MOD1_MASK
#define MOO_FILE_VIEW_BINDING_GO_FORWARD_KP GDK_KP_Right, GDK_MOD1_MASK
#define MOO_FILE_VIEW_BINDING_GO_HOME GDK_Home, GDK_MOD1_MASK
#define MOO_FILE_VIEW_BINDING_GO_HOME_KP GDK_KP_Home, GDK_MOD1_MASK
#define MOO_FILE_VIEW_BINDING_DELETE GDK_Delete, GDK_MOD1_MASK
#define MOO_FILE_VIEW_BINDING_PROPERTIES GDK_Return, GDK_MOD1_MASK

#else /* GDK_WINDOWING_QUARTZ */

#define MOO_FILE_VIEW_ACCEL_GO_UP "<Meta>Up"
#define MOO_FILE_VIEW_ACCEL_GO_BACK "<Meta>["
#define MOO_FILE_VIEW_ACCEL_GO_FORWARD "<Meta>]"
#define MOO_FILE_VIEW_ACCEL_GO_HOME "<Meta><Shift>H"
#define MOO_FILE_VIEW_ACCEL_DELETE "<Meta>Backspace"
#define MOO_FILE_VIEW_ACCEL_SHOW_HIDDEN NULL
#define MOO_FILE_VIEW_ACCEL_PROPERTIES "<Meta>I"

#define MOO_FILE_VIEW_BINDING_GO_UP GDK_Up, GDK_META_MASK
#define MOO_FILE_VIEW_BINDING_GO_UP_KP GDK_KP_Up, GDK_META_MASK
#define MOO_FILE_VIEW_BINDING_GO_BACK GDK_bracketleft, GDK_META_MASK
#define MOO_FILE_VIEW_BINDING_GO_FORWARD GDK_bracketright, GDK_META_MASK
#define MOO_FILE_VIEW_BINDING_GO_HOME GDK_h, GDK_META_MASK | GDK_SHIFT_MASK
#define MOO_FILE_VIEW_BINDING_DELETE GDK_BackSpace, GDK_META_MASK
#define MOO_FILE_VIEW_BINDING_PROPERTIES GDK_i, GDK_META_MASK

#endif /* GDK_WINDOWING_QUARTZ */

#endif /* MOO_FILE_VIEW_ACCELS_H */
