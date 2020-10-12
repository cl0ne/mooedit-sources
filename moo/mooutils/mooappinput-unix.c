/*
 *   mooapp/mooappinput-unix.c
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

#define MOO_DO_NOT_MANGLE_GLIB_FUNCTIONS
#include <mooglib/moo-glib.h>
MGW_ERROR_IF_NOT_SHARED_LIBC

#include "mooutils/mooappinput-priv.h"

# include <sys/socket.h>
# include <sys/un.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "mooapp-ipc.h"
#include "mooutils-misc.h"
#include "mooutils-thread.h"
#include "mooutils-debug.h"

#define INPUT_PREFIX "in-"


static const char *
get_display_name (void)
{
    static char *name;

#ifdef GDK_WINDOWING_X11
    static gboolean been_here;
    if (!been_here)
    {
        GdkDisplay *display;
        const char *display_name;

        been_here = TRUE;

        if ((display = gdk_display_get_default ()))
        {
            display_name = gdk_display_get_name (display);
        }
        else
        {
            display_name = gdk_get_display_arg_name ();
            if (!display_name || !display_name[0])
                display_name = g_getenv ("DISPLAY");
        }

        if (display_name && display_name[0])
        {
            char *colon, *dot;

            if ((colon = strchr (display_name, ':')) &&
                (dot = strrchr (display_name, '.')) &&
                dot > colon)
                    name = g_strndup (display_name, dot - display_name);
            else
                name = g_strdup (display_name);

            if (name[0] == ':')
            {
                if (name[1])
                {
                    char *tmp = g_strdup (name + 1);
                    g_free (name);
                    name = tmp;
                }
                else
                {
                    g_free (name);
                    name = NULL;
                }
            }

            if (name)
                g_strcanon (name,
                            G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS,
                            '-');
        }
    }
#endif

    return name;
}

static const char *
get_user_name (void)
{
    static char *user_name;

    if (!user_name)
        user_name = g_strcanon (g_strdup (g_get_user_name ()),
                                G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS,
                                '-');

    return user_name;
}

static char *
get_pipe_dir (const char *appname)
{
    const char *display_name;
    const char *user_name;
    char *name;

    g_return_val_if_fail (appname != NULL, NULL);

    display_name = get_display_name ();
    user_name = get_user_name ();

    if (display_name)
        name = g_strdup_printf ("%s/%s-%s-%s", g_get_tmp_dir (), appname, user_name, display_name);
    else
        name = g_strdup_printf ("%s/%s-%s", g_get_tmp_dir (), appname, user_name);

    return name;
}

static char *
get_pipe_path (const char *pipe_dir,
               const char *name)
{
    return g_strdup_printf ("%s/" INPUT_PREFIX "%s",
                            pipe_dir, name);
}

static gboolean
input_channel_start_io (int           fd,
                        GIOFunc       io_func,
                        gpointer      data,
                        GIOChannel  **io_channel,
                        guint        *io_watch)
{
    GSource *source;

    *io_channel = g_io_channel_unix_new (fd);
    g_io_channel_set_encoding (*io_channel, NULL, NULL);

    *io_watch = g_io_add_watch (*io_channel,
                                G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR,
                                io_func, data);

    source = g_main_context_find_source_by_id (NULL, *io_watch);
    g_source_set_can_recurse (source, TRUE);

    return TRUE;
}


static gboolean do_send (const char *filename,
                         const char *iheader,
                         const char *data,
                         gssize      data_len);

static gboolean
try_send (const char *pipe_dir_name,
          const char *name,
          const char *iheader,
          const char *data,
          gssize      data_len)
{
    char *filename = NULL;
    gboolean result = FALSE;

    g_return_val_if_fail (name && name[0], FALSE);

    filename = get_pipe_path (pipe_dir_name, name);
    moo_dmsg ("try_send: sending data to `%s'", filename);

    if (!g_file_test (filename, G_FILE_TEST_EXISTS))
    {
        moo_dmsg ("try_send: file %s doesn't exist", filename);
        goto out;
    }

    result = do_send (filename, iheader, data, data_len);

out:
    g_free (filename);
    return result;
}

gboolean
_moo_app_input_send_msg (const char *name,
                         const char *data,
                         gssize      len)
{
    const char *entry;
    GDir *pipe_dir = NULL;
    char *pipe_dir_name;
    gboolean success = FALSE;

    g_return_val_if_fail (data != NULL, FALSE);

    moo_dmsg ("_moo_app_input_send_msg: sending data to %s", name ? name : "NONE");

    pipe_dir_name = get_pipe_dir (MOO_PACKAGE_NAME);
    g_return_val_if_fail (pipe_dir_name != NULL, FALSE);

    if (name)
    {
        success = try_send (pipe_dir_name, name, NULL, data, len);
        goto out;
    }

    success = try_send (pipe_dir_name, MOO_APP_INPUT_NAME_DEFAULT, NULL, data, len);
    if (success)
        goto out;

    pipe_dir = g_dir_open (pipe_dir_name, 0, NULL);

    if (!pipe_dir)
        goto out;

    while ((entry = g_dir_read_name (pipe_dir)))
    {
        if (!strncmp (entry, INPUT_PREFIX, strlen (INPUT_PREFIX)))
        {
            name = entry + strlen (INPUT_PREFIX);

            if (try_send (pipe_dir_name, name, NULL, data, len))
            {
                success = TRUE;
                goto out;
            }
        }
    }

out:
    if (pipe_dir)
        g_dir_close (pipe_dir);
    g_free (pipe_dir_name);
    return success;
}

void
_moo_app_input_broadcast (const char *header,
                          const char *data,
                          gssize      len)
{
    const char *entry;
    GDir *pipe_dir = NULL;
    char *pipe_dir_name;

    g_return_if_fail (data != NULL);

    moo_dmsg ("_moo_app_input_broadcast");

    if (!_moo_app_input_instance)
        return;

    pipe_dir_name = get_pipe_dir (_moo_app_input_instance->appname);
    g_return_if_fail (pipe_dir_name != NULL);

    pipe_dir = g_dir_open (pipe_dir_name, 0, NULL);

    while (pipe_dir && (entry = g_dir_read_name (pipe_dir)))
    {
        if (!strncmp (entry, INPUT_PREFIX, strlen (INPUT_PREFIX)))
        {
            GSList *l;
            gboolean my_name = FALSE;
            const char *name = entry + strlen (INPUT_PREFIX);

            for (l = _moo_app_input_instance->pipes; !my_name && l != NULL; l = l->next)
            {
                InputChannel *ch = l->data;
                const char *ch_name = _moo_app_input_channel_get_name (ch);
                if (ch_name && strcmp (ch_name, name) == 0)
                    my_name = TRUE;
            }

            if (!my_name)
                try_send (pipe_dir_name, name, header, data, len);
        }
    }

    if (pipe_dir)
        g_dir_close (pipe_dir);
    g_free (pipe_dir_name);
}

static gboolean
do_write (int         fd,
          const char *data,
          gsize       data_len)
{
    while (data_len > 0)
    {
        ssize_t n;

        errno = 0;
        n = write (fd, data, data_len);

        if (n < 0)
        {
            if (errno != EAGAIN && errno != EINTR)
            {
                g_warning ("in write: %s", g_strerror (errno));
                return FALSE;
            }
        }
        else
        {
            data += n;
            data_len -= n;
        }
    }

    return TRUE;
}

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

typedef struct {
    int fd;
    GIOChannel *io;
    guint io_watch;
    GString *buffer; /* messages are zero-terminated */
    InputChannel *ch;
} Connection;

struct InputChannel
{
    char *name;
    char *path;
    char *pipe_dir;
    gboolean owns_file;
    int fd;
    GIOChannel *io;
    guint io_watch;
    GSList *connections;
};

char *
_moo_app_input_channel_get_path (InputChannel *ch)
{
    g_return_val_if_fail (ch != NULL, NULL);
    return g_strdup (ch->path);
}

const char *
_moo_app_input_channel_get_name (InputChannel *ch)
{
    g_return_val_if_fail (ch != NULL, NULL);
    return ch->name;
}

static void
connection_free (Connection *conn)
{
    if (conn->io_watch)
        g_source_remove (conn->io_watch);

    if (conn->io)
    {
        g_io_channel_shutdown (conn->io, FALSE, NULL);
        g_io_channel_unref (conn->io);
    }

    if (conn->fd != -1)
        close (conn->fd);

    g_string_free (conn->buffer, TRUE);
    g_slice_free (Connection, conn);
}

static void
input_channel_shutdown (InputChannel *ch)
{
    g_slist_foreach (ch->connections, (GFunc) connection_free, NULL);
    g_slist_free (ch->connections);
    ch->connections = NULL;

    if (ch->io_watch)
    {
        g_source_remove (ch->io_watch);
        ch->io_watch = 0;
    }

    if (ch->io)
    {
        g_io_channel_shutdown (ch->io, FALSE, NULL);
        g_io_channel_unref (ch->io);
        ch->io = NULL;
    }

    if (ch->fd != -1)
    {
        close (ch->fd);
        ch->fd = -1;
    }

    if (ch->path)
    {
        if (ch->owns_file)
            unlink (ch->path);
        g_free (ch->path);
        ch->path = NULL;
    }
}

static gboolean
read_input (G_GNUC_UNUSED GIOChannel *source,
            G_GNUC_UNUSED GIOCondition condition,
            Connection *conn)
{
    char c;
    int n;
    gboolean do_commit = FALSE;

    errno = 0;

    while ((n = read (conn->fd, &c, 1)) > 0)
    {
        if (c == 0)
        {
            do_commit = TRUE;
            break;
        }

        g_string_append_c (conn->buffer, c);
    }

    if (n <= 0)
    {
        if (n < 0)
            moo_dmsg ("%s", g_strerror (errno));
        else
            moo_dmsg ("EOF");
        goto remove;
    }
    else
    {
        moo_dmsg ("got bytes: '%s'", conn->buffer->str);
    }

    if (do_commit)
        _moo_app_input_channel_commit (&conn->buffer);

    if (!do_commit && (condition & (G_IO_ERR | G_IO_HUP)))
    {
        moo_dmsg ("%s", (condition & G_IO_ERR) ? "G_IO_ERR" : "G_IO_HUP");
        goto remove;
    }

    return TRUE;

remove:
    if (conn->ch)
        conn->ch->connections = g_slist_remove (conn->ch->connections, conn);
    connection_free (conn);
    return FALSE;
}

static gboolean
accept_connection (G_GNUC_UNUSED GIOChannel *source,
                   GIOCondition  condition,
                   InputChannel *ch)
{
    Connection *conn;
    socklen_t dummy;

    if (condition & G_IO_ERR)
    {
        input_channel_shutdown (ch);
        return FALSE;
    }

    conn = g_slice_new0 (Connection);
    conn->ch = ch;
    conn->buffer = g_string_new_len (NULL, MOO_APP_INPUT_MAX_BUFFER_SIZE);

    conn->fd = accept (ch->fd, NULL, &dummy);

    if (conn->fd == -1)
    {
        g_warning ("in accept: %s", g_strerror (errno));
        g_slice_free (Connection, conn);
        return TRUE;
    }

    if (!input_channel_start_io (conn->fd, (GIOFunc) read_input, conn,
                                 &conn->io, &conn->io_watch))
    {
        close (conn->fd);
        g_slice_free (Connection, conn);
        return TRUE;
    }

    ch->connections = g_slist_prepend (ch->connections, conn);
    return TRUE;
}

static gboolean
try_connect (const char *filename,
             int        *fdp)
{
    int fd;
    struct sockaddr_un addr;

    g_return_val_if_fail (filename != NULL, FALSE);

    if (strlen (filename) + 1 > sizeof addr.sun_path)
    {
        g_critical ("oops");
        return FALSE;
    }

    addr.sun_family = AF_UNIX;
    strcpy (addr.sun_path, filename);
    fd = socket (PF_UNIX, SOCK_STREAM, 0);

    if (fd == -1)
    {
        g_warning ("in socket for %s: %s", filename, g_strerror (errno));
        return FALSE;
    }

    errno = 0;

    if (connect (fd, (struct sockaddr *) &addr, sizeof addr) == -1)
    {
        unlink (filename);
        close (fd);
    	return FALSE;
    }

    if (fdp)
        *fdp = fd;
    else
        close (fd);

    return TRUE;
}

static gboolean
input_channel_start (InputChannel *ch,
                     gboolean      may_fail)
{
    struct sockaddr_un addr;

    mkdir (ch->pipe_dir, S_IRWXU);

    if (try_connect (ch->path, NULL))
    {
        if (!may_fail)
            g_warning ("'%s' is already in use", ch->path);
        return FALSE;
    }

    if (strlen (ch->path) + 1 > sizeof addr.sun_path)
    {
        g_critical ("oops");
        return FALSE;
    }

    addr.sun_family = AF_UNIX;
    strcpy (addr.sun_path, ch->path);

    errno = 0;

    if ((ch->fd = socket (PF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        g_warning ("in socket for %s: %s", ch->path, g_strerror (errno));
        return FALSE;
    }

    if (bind (ch->fd, (struct sockaddr*) &addr, sizeof addr) == -1)
    {
        g_warning ("in bind for %s: %s", ch->path, g_strerror (errno));
        close (ch->fd);
        ch->fd = -1;
        return FALSE;
    }

    if (listen (ch->fd, 5) == -1)
    {
        g_warning ("in listen for %s: %s", ch->path, g_strerror (errno));
        close (ch->fd);
    	ch->fd = -1;
    	return FALSE;
    }

    ch->owns_file = TRUE;

    if (!input_channel_start_io (ch->fd, (GIOFunc) accept_connection, ch,
                                 &ch->io, &ch->io_watch))
    	return FALSE;

    return TRUE;
}

InputChannel *
_moo_app_input_channel_new (const char *appname,
                            const char *name,
                            gboolean    may_fail)
{
    InputChannel *ch;

    g_return_val_if_fail (appname != NULL, NULL);
    g_return_val_if_fail (name != NULL, NULL);

    ch = g_slice_new0 (InputChannel);

    ch->name = g_strdup (name);
    ch->pipe_dir = get_pipe_dir (appname);
    ch->path = get_pipe_path (ch->pipe_dir, name);
    ch->fd = -1;
    ch->io = NULL;
    ch->io_watch = 0;

    if (!input_channel_start (ch, may_fail))
    {
        _moo_app_input_channel_free (ch);
        return NULL;
    }

    return ch;
}

void
_moo_app_input_channel_free (InputChannel *ch)
{
    input_channel_shutdown (ch);
    g_free (ch->name);
    g_free (ch->path);

    if (ch->pipe_dir)
    {
        remove (ch->pipe_dir);
        g_free (ch->pipe_dir);
    }

    g_slice_free (InputChannel, ch);
}


static gboolean
do_send (const char *filename,
         const char *iheader,
         const char *data,
         gssize      data_len)
{
    int fd;
    gboolean result = TRUE;

    g_return_val_if_fail (filename != NULL, FALSE);
    g_return_val_if_fail (data != NULL || data_len == 0, FALSE);

    if (!try_connect (filename, &fd))
        return FALSE;

    if (data_len < 0)
        data_len = strlen (data);

    if (iheader)
    {
        char c = MOO_APP_INPUT_IPC_MAGIC_CHAR;
        result = do_write (fd, &c, 1) &&
                 do_write (fd, iheader, strlen (iheader));
    }

    if (result && data_len)
        result = do_write (fd, data, data_len);

    if (result)
    {
        char c = 0;
        result = do_write (fd, &c, 1);
    }

    close (fd);
    return result;
}
