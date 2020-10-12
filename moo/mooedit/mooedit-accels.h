/*
 *   mooedit-accels.h
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

#ifndef MOO_EDIT_ACCELS_H
#define MOO_EDIT_ACCELS_H

#include <mooutils/mooaccel.h>

#define MOO_EDIT_ACCEL_NEW          MOO_ACCEL_NEW
#define MOO_EDIT_ACCEL_OPEN         MOO_ACCEL_OPEN
#define MOO_EDIT_ACCEL_SAVE         MOO_ACCEL_SAVE
#define MOO_EDIT_ACCEL_SAVE_AS      MOO_ACCEL_SAVE_AS
#define MOO_EDIT_ACCEL_CLOSE        MOO_ACCEL_CLOSE
#define MOO_EDIT_ACCEL_PAGE_SETUP   MOO_ACCEL_PAGE_SETUP
#define MOO_EDIT_ACCEL_PRINT        MOO_ACCEL_PRINT

#define MOO_EDIT_ACCEL_NEW_WINDOW MOO_ACCEL_CTRL "<Shift>N"
#define MOO_EDIT_ACCEL_FIND MOO_ACCEL_CTRL "F"
#define MOO_EDIT_ACCEL_REPLACE MOO_ACCEL_CTRL "R"
#define MOO_EDIT_ACCEL_OPEN_RECENT_DIALOG MOO_ACCEL_CTRL "<Shift>O"

#ifndef GDK_WINDOWING_QUARTZ

#define MOO_EDIT_ACCEL_FIND_NEXT "F3"
#define MOO_EDIT_ACCEL_FIND_PREV "<Shift>F3"
#define MOO_EDIT_ACCEL_FIND_CURRENT "F4"
#define MOO_EDIT_ACCEL_FIND_CURRENT_BACK "<Shift>F4"
#define MOO_EDIT_ACCEL_GOTO_LINE "<Ctrl>G"
#define MOO_EDIT_ACCEL_FIND_IN_FILES "<Ctrl><Shift>F"

#define MOO_EDIT_ACCEL_FOCUS_DOC "<Alt>C"
#define MOO_EDIT_ACCEL_MOVE_TO_SPLIT_NOTEBOOK "<Alt>L"
#define MOO_EDIT_ACCEL_FOCUS_SPLIT_NOTEBOOK "<Ctrl>L"
#define MOO_EDIT_ACCEL_SWITCH_TO_TAB "<Alt>"
#define MOO_EDIT_ACCEL_PREV_TAB "<Alt>Left"
#define MOO_EDIT_ACCEL_NEXT_TAB "<Alt>Right"
#define MOO_EDIT_ACCEL_PREV_TAB_IN_VIEW "<Alt>O"
#define MOO_EDIT_ACCEL_NEXT_TAB_IN_VIEW "<Alt>P"

#else /* GDK_WINDOWING_QUARTZ */

#define MOO_EDIT_ACCEL_FIND_NEXT "<Meta>G"
#define MOO_EDIT_ACCEL_FIND_PREV "<Meta><Shift>G"
#define MOO_EDIT_ACCEL_FIND_CURRENT "F4" /* XXX */
#define MOO_EDIT_ACCEL_FIND_CURRENT_BACK "<Shift>F4" /* XXX */
#define MOO_EDIT_ACCEL_GOTO_LINE "<Meta>L"
#define MOO_EDIT_ACCEL_FIND_IN_FILES "<Ctrl><Shift>F"

#define MOO_EDIT_ACCEL_FOCUS_DOC "<Meta>J"
#define MOO_EDIT_ACCEL_MOVE_TO_SPLIT_NOTEBOOK ""
#define MOO_EDIT_ACCEL_FOCUS_SPLIT_NOTEBOOK ""
#define MOO_EDIT_ACCEL_SWITCH_TO_TAB "<Meta>"
#define MOO_EDIT_ACCEL_PREV_TAB "<Meta>Left"
#define MOO_EDIT_ACCEL_NEXT_TAB "<Meta>Right"
#define MOO_EDIT_ACCEL_PREV_TAB_IN_VIEW "<Meta>O"
#define MOO_EDIT_ACCEL_NEXT_TAB_IN_VIEW "<Meta>P"

#endif /* GDK_WINDOWING_QUARTZ */

#define MOO_EDIT_ACCEL_BOOKMARK MOO_ACCEL_CTRL "B"
#define MOO_EDIT_ACCEL_NEXT_BOOKMARK "<Alt>Down" /* XXX */
#define MOO_EDIT_ACCEL_PREV_BOOKMARK "<Alt>Up" /* XXX */

/* no such shortcut on Mac */
#define MOO_EDIT_ACCEL_RELOAD "F5"
/* XXX Shift-Command-W is Close File on Mac */
#define MOO_EDIT_ACCEL_CLOSE_ALL MOO_ACCEL_CTRL "<Shift>W"

#define MOO_EDIT_ACCEL_STOP "<Ctrl>Cancel"

#define MOO_EDIT_ACCEL_COMPLETE "<Ctrl>Space"

#endif /* MOO_EDIT_ACCELS_H */
