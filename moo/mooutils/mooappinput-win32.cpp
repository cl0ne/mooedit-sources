/*
 *   mooapp/mooappinput-win32.c
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

# include <windows.h>
# include <io.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "mooappinput.h"
#include "mooapp-ipc.h"
#include "mooutils-misc.h"
#include "mooutils-thread.h"
#include "mooutils-debug.h"

#define INPUT_PREFIX "input-"


typedef struct {
    char *pipe_name;
    guint event_id;
} ListenerInfo;

struct InputChannel
{
    char *appname;
    char *name;
    char *pipe_name;
    GString *buffer;
    guint event_id;
};

char *
_moo_app_input_channel_get_path (InputChannel *ch)
{
    g_return_val_if_fail (ch != NULL, NULL);
    return g_strdup (ch->pipe_name);
}

const char *
_moo_app_input_channel_get_name (InputChannel *ch)
{
    g_return_val_if_fail (ch != NULL, NULL);
    return ch->name;
}

static ListenerInfo *
listener_info_new (const char *pipe_name,
                   guint       event_id)
{
    ListenerInfo *info = g_new (ListenerInfo, 1);
    info->pipe_name = g_strdup (pipe_name);
    info->event_id = event_id;
    return info;
}

static void
listener_info_free (ListenerInfo *info)
{
    if (info)
    {
        g_free (info->pipe_name);
        g_free (info);
    }
}


static gpointer
listener_main (ListenerInfo *info)
{
    HANDLE input;

    _moo_message_async ("%s: hi there", G_STRLOC);

	/* XXX unicode */
    input = CreateNamedPipeA (info->pipe_name,
                              PIPE_ACCESS_DUPLEX,
                              PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                              PIPE_UNLIMITED_INSTANCES,
                              0, 0, 200, NULL);

    if (input == INVALID_HANDLE_VALUE)
    {
        _moo_message_async ("%s: could not create input pipe", G_STRLOC);
        listener_info_free (info);
        return NULL;
    }

    _moo_message_async ("%s: opened pipe %s", G_STRLOC, info->pipe_name);

    while (TRUE)
    {
        DWORD bytes_read;
        char c;

        DisconnectNamedPipe (input);
        _moo_message_async ("%s: opening connection", G_STRLOC);

        if (!ConnectNamedPipe (input, NULL))
        {
            DWORD err = GetLastError();

            if (err != ERROR_PIPE_CONNECTED)
            {
                char *msg = g_win32_error_message (err);
                _moo_message_async ("%s: error in ConnectNamedPipe(): %s", G_STRLOC, msg);
                CloseHandle (input);
                g_free (msg);
                break;
            }
        }

        _moo_message_async ("%s: client connected", G_STRLOC);

        while (ReadFile (input, &c, 1, &bytes_read, NULL))
        {
            if (bytes_read == 1)
            {
                _moo_event_queue_push (info->event_id, GINT_TO_POINTER ((int) c), NULL);
            }
            else
            {
                _moo_message_async ("%s: client disconnected", G_STRLOC);
                break;
            }
        }
    }

    _moo_message_async ("%s: goodbye", G_STRLOC);

    CloseHandle (input);
    listener_info_free (info);
    return NULL;
}


static char *
get_pipe_name (const char *appname,
               const char *name)
{
    if (!name)
        name = _moo_get_pid_string ();
    return g_strdup_printf ("\\\\.\\pipe\\%s_in_%s",
                            appname, name);
}


static void
event_callback (GList        *events,
                InputChannel *ch)
{
    while (events)
    {
        char c = GPOINTER_TO_INT (events->data);

        if (c != 0)
            g_string_append_c (ch->buffer, c);
        else
            _moo_app_input_channel_commit (&ch->buffer);

        events = events->next;
    }
}


static gboolean
input_channel_start (InputChannel *ch)
{
    ListenerInfo *info;

    g_free (ch->pipe_name);
    ch->pipe_name = get_pipe_name (ch->appname, ch->name);
    ch->event_id = _moo_event_queue_connect ((MooEventQueueCallback) event_callback,
                                             ch, NULL);

    info = listener_info_new (ch->pipe_name, ch->event_id);

    if (!g_thread_create ((GThreadFunc) listener_main, info, FALSE, NULL))
    {
        g_critical ("could not start listener thread");
        listener_info_free (info);
        g_free (ch->pipe_name);
        ch->pipe_name = NULL;
        _moo_event_queue_disconnect (ch->event_id);
        ch->event_id = 0;
        return FALSE;
    }

    return TRUE;
}


InputChannel *
_moo_app_input_channel_new (const char *appname,
                            const char *name,
                            G_GNUC_UNUSED gboolean may_fail)
{
    InputChannel *ch;

    ch = g_slice_new0 (InputChannel);
    ch->appname = g_strdup (appname);
    ch->name = g_strdup (name);
    ch->buffer = g_string_new_len (NULL, MOO_APP_INPUT_MAX_BUFFER_SIZE);

    if (!input_channel_start (ch))
    {
        _moo_app_input_channel_free (ch);
        return NULL;
    }

    return ch;
}

void
_moo_app_input_channel_free (InputChannel *ch)
{
    if (ch->event_id)
        _moo_event_queue_disconnect (ch->event_id);
    if (ch->buffer)
        g_string_free (ch->buffer, TRUE);
    g_free (ch->pipe_name);
    g_free (ch->appname);
    g_free (ch->name);
    g_slice_free (InputChannel, ch);
}

static gboolean
write_data (HANDLE      file,
            const char *data,
            gsize       len,
            const char *pipe_name)
{
    DWORD bytes_written;

    if (!WriteFile (file, data, (DWORD) len, &bytes_written, NULL))
    {
        char *err_msg = g_win32_error_message (GetLastError ());
        g_warning ("could not write data to '%s': %s", pipe_name, err_msg);
        g_free (err_msg);
        return FALSE;
    }

    if (bytes_written < (DWORD) len)
    {
        g_warning ("written less data than requested to '%s'", pipe_name);
        return FALSE;
    }

    return TRUE;
}

gboolean
_moo_app_input_send_msg (const char *name,
                         const char *data,
                         gssize      len)
{
    char *err_msg = NULL;
    char *pipe_name;
    HANDLE pipe_handle;
    gboolean result = FALSE;

    g_return_val_if_fail (data != NULL, FALSE);

    if (len < 0)
        len = strlen (data);

    if (!len)
        return TRUE;

    if (!name)
        name = "main";

    pipe_name = get_pipe_name (MOO_PACKAGE_NAME, name);
	/* XXX unicode */
    pipe_handle = CreateFileA (pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (pipe_handle == INVALID_HANDLE_VALUE)
    {
        err_msg = g_win32_error_message (GetLastError ());
        goto out;
    }

    result = write_data (pipe_handle, data, len, pipe_name);

    if (result)
    {
        char c = 0;
        result = write_data (pipe_handle, &c, 1, pipe_name);
    }

out:
    if (pipe_handle != INVALID_HANDLE_VALUE)
        CloseHandle (pipe_handle);

    g_free (pipe_name);
    g_free (err_msg);
    return result;
}

void
_moo_app_input_broadcast (const char *header,
                          const char *data,
                          gssize      len)
{
    MOO_IMPLEMENT_ME

    g_return_if_fail (header != NULL);
    g_return_if_fail (data != NULL);

    if (len < 0)
        len = strlen (data);

    g_return_if_fail (len != 0);
}
