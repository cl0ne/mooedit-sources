/*
 *   moostock.h
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

#ifndef MOOUTILS_STOCK_H
#define MOOUTILS_STOCK_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_STOCK_TERMINAL              "moo-terminal"
#define MOO_STOCK_KEYBOARD              GTK_STOCK_SELECT_FONT
#define MOO_STOCK_MENU                  GTK_STOCK_INDEX
#define MOO_STOCK_RESTART               GTK_STOCK_REFRESH

#define MOO_STOCK_DOC_DELETED           GTK_STOCK_DIALOG_ERROR
#define MOO_STOCK_DOC_MODIFIED_ON_DISK  GTK_STOCK_DIALOG_WARNING
#define MOO_STOCK_DOC_MODIFIED          GTK_STOCK_SAVE

#define MOO_STOCK_NEW_WINDOW            "moo-new-window"

#define MOO_STOCK_FILE_SELECTOR         "moo-file-selector"
#define MOO_STOCK_FILE_BOOKMARK         "moo-file-bookmark"
#define MOO_STOCK_FOLDER                "moo-folder"
#define MOO_STOCK_FILE                  "moo-file"
#define MOO_STOCK_NEW_FOLDER            "moo-new-folder"

#define MOO_STOCK_SAVE_NONE             "moo-save-none"
#define MOO_STOCK_SAVE_SELECTED         "moo-save-selected"

#define MOO_STOCK_NEW_PROJECT           "moo-new-project"
#define MOO_STOCK_OPEN_PROJECT          "moo-open-project"
#define MOO_STOCK_CLOSE_PROJECT         "moo-close-project"
#define MOO_STOCK_PROJECT_OPTIONS       "moo-project-options"
#define MOO_STOCK_BUILD                 "moo-build"
#define MOO_STOCK_COMPILE               "moo-compile"
#define MOO_STOCK_EXECUTE               "moo-execute"

#define MOO_STOCK_FIND_IN_FILES         "moo-find-in-files"
#define MOO_STOCK_FIND_FILE             "moo-find-file"

#define MOO_STOCK_FILE_COPY             "moo-file-copy"
#define MOO_STOCK_FILE_MOVE             "moo-file-move"
#define MOO_STOCK_FILE_LINK             "moo-file-link"
#define MOO_STOCK_FILE_SAVE_AS          "moo-file-save-as"
#define MOO_STOCK_FILE_SAVE_COPY        "moo-file-save-copy"

#define MOO_STOCK_EDIT_BOOKMARK         "moo-edit-bookmark"

#define MOO_STOCK_PLUGINS               GTK_STOCK_PREFERENCES


void        _moo_stock_init                 (void);


G_END_DECLS

#endif /* MOOUTILS_STOCK_H */
