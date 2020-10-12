/*
 *   mooutils/mooappinput-common.c
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

#include "config.h"
#include "mooutils/mooappinput-priv.h"

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "mooappinput.h"
#include "mooapp-ipc.h"
#include "mooutils-misc.h"
#include "mooutils-thread.h"
#include "mooutils-debug.h"

MOO_DEBUG_INIT(input, FALSE)

MooAppInput *_moo_app_input_instance;

static void
exec_callback (char        cmd,
               const char *data,
               gsize       len)
{
    g_return_if_fail (_moo_app_input_instance && _moo_app_input_instance->callback);
    if (cmd == MOO_APP_INPUT_IPC_MAGIC_CHAR)
        _moo_ipc_dispatch (data, len);
    else
        _moo_app_input_instance->callback (cmd, data, len, _moo_app_input_instance->callback_data);
}


static MooAppInput *
moo_app_input_new (const char *name,
                   gboolean    bind_default,
                   MooAppInputCallback callback,
                   gpointer    callback_data)
{
    MooAppInput *ch;
    InputChannel *ich;

    g_return_val_if_fail (callback != NULL, NULL);

    ch = g_slice_new0 (MooAppInput);

    ch->callback = callback;
    ch->callback_data = callback_data;
    ch->pipes = NULL;
    ch->appname = g_strdup (MOO_PACKAGE_NAME);

    if ((ich = _moo_app_input_channel_new (ch->appname, _moo_get_pid_string (), FALSE)))
    {
        ch->pipes = g_slist_prepend (ch->pipes, ich);
        ch->main_path = _moo_app_input_channel_get_path (ich);
    }

    if (name && (ich = _moo_app_input_channel_new (ch->appname, name, FALSE)))
        ch->pipes = g_slist_prepend (ch->pipes, ich);

    if (bind_default && (ich = _moo_app_input_channel_new (ch->appname, MOO_APP_INPUT_NAME_DEFAULT, TRUE)))
        ch->pipes = g_slist_prepend (ch->pipes, ich);

    return ch;
}

void
_moo_app_input_start (const char     *name,
                      gboolean        bind_default,
                      MooAppInputCallback callback,
                      gpointer        callback_data)
{
    g_return_if_fail (_moo_app_input_instance == NULL);
    _moo_app_input_instance = moo_app_input_new (name, bind_default,
                                                 callback, callback_data);
}

static void
moo_app_input_free (MooAppInput *ch)
{
    g_return_if_fail (ch != NULL);

    g_slist_foreach (ch->pipes, (GFunc) _moo_app_input_channel_free, NULL);
    g_slist_free (ch->pipes);

    g_free (ch->main_path);
    g_free (ch->appname);
    g_slice_free (MooAppInput, ch);
}

void
_moo_app_input_shutdown (void)
{
    if (_moo_app_input_instance)
    {
        MooAppInput *tmp = _moo_app_input_instance;
        _moo_app_input_instance = NULL;
        moo_app_input_free (tmp);
    }
}


const char *
_moo_app_input_get_path (void)
{
    return _moo_app_input_instance ? _moo_app_input_instance->main_path : NULL;
}

gboolean
_moo_app_input_running (void)
{
    return _moo_app_input_instance != NULL;
}


void
_moo_app_input_channel_commit (GString **buffer)
{
    char buf[MOO_APP_INPUT_MAX_BUFFER_SIZE];
    GString *freeme = NULL;
    char *ptr;
    gsize len;

    if (!(*buffer)->len)
    {
        moo_dmsg ("got empty command");
        return;
    }

    if ((*buffer)->len + 1 > MOO_APP_INPUT_MAX_BUFFER_SIZE)
    {
        freeme = *buffer;
        *buffer = g_string_new_len (NULL, MOO_APP_INPUT_MAX_BUFFER_SIZE);
        ptr = freeme->str;
        len = freeme->len;
    }
    else
    {
        memcpy (buf, (*buffer)->str, (*buffer)->len + 1);
        ptr = buf;
        len = (*buffer)->len;
        g_string_truncate (*buffer, 0);
    }

    if (0)
        g_print ("%s: commit %c\n%s\n-----\n", G_STRLOC, ptr[0], ptr + 1);

    exec_callback (ptr[0], ptr + 1, len - 1);

    if (freeme)
        g_string_free (freeme, TRUE);
}
