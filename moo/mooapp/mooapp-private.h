/*
 *   mooapp/mooapp-private.h
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

#ifndef MOO_APP_PRIVATE_H
#define MOO_APP_PRIVATE_H

#include "mooapp.h"

G_BEGIN_DECLS

typedef enum
{
    CMD_SCRIPT = 1,
    CMD_OPEN_FILES,
    CMD_LAST
} MooAppCmdCode;

/* I is taken by the IPC thing */
#define CMD_SCRIPT_S        "e"
#define CMD_OPEN_FILES_S    "F"

static const char *moo_app_cmd_chars =
    "\0"
    CMD_SCRIPT_S
    CMD_OPEN_FILES_S
;


G_END_DECLS

#endif /* MOO_APP_PRIVATE_H */
