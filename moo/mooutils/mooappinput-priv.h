/*
 *   mooutils/mooappinput-priv.h
 *
 *   Copyright (C) 2004-2015 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
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

#pragma once

#include "mooutils/mooappinput.h"

G_BEGIN_DECLS

typedef struct MooAppInput MooAppInput;
typedef struct InputChannel InputChannel;

struct MooAppInput
{
    GSList *pipes;
    char *appname;
    char *main_path;
    MooAppInputCallback callback;
    gpointer callback_data;
};

#define MOO_APP_INPUT_NAME_DEFAULT "main"
#define MOO_APP_INPUT_IPC_MAGIC_CHAR 'I'
#define MOO_APP_INPUT_MAX_BUFFER_SIZE 4096

extern MooAppInput *_moo_app_input_instance;

InputChannel *_moo_app_input_channel_new        (const char     *appname,
                                                 const char     *name,
                                                 gboolean        may_fail);
void          _moo_app_input_channel_free       (InputChannel   *ch);
char         *_moo_app_input_channel_get_path   (InputChannel   *ch);
const char   *_moo_app_input_channel_get_name   (InputChannel   *ch);

void          _moo_app_input_channel_commit     (GString **buffer);

G_END_DECLS
