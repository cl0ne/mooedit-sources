/*
 *   mooutils-thread.c
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

#include "config.h"
#include "mooutils/mooutils-thread.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-debug.h"
#include "mooutils/mootype-macros.h"
#include "mooutils/moolist.h"

#include <stdio.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

MOO_DEBUG_INIT(threads, FALSE)

typedef struct {
    MooEventQueueCallback callback;
    gpointer callback_data;
    GDestroyNotify notify;
    guint id;
} QueueClient;

typedef struct {
    gpointer data;
    GDestroyNotify destroy;
    guint id;
} EventData;

MOO_DEFINE_SLIST(QueueClientList, queue_client_list, QueueClient)
MOO_DEFINE_QUEUE(EventData, event_data)

typedef struct {
    QueueClientList *clients;
    guint last_id;

    MgwFd pipe_in;
    MgwFd pipe_out;
    GIOChannel *io;

    GHashTable *data;

    volatile gboolean init;
} EventQueue;


#if !GLIB_CHECK_VERSION(2,32,0)
static GStaticMutex queue_lock = G_STATIC_MUTEX_INIT;
#else
static GMutex queue_lock;
#endif
static EventQueue queue;


#if GLIB_CHECK_VERSION(2,32,0)
static GMutex *moo_mutex_new ()
{
    GMutex *mutex = g_slice_new (GMutex);
    g_mutex_init (mutex);
    return mutex;
}

static void moo_mutex_free (GMutex* mutex)
{
    if (mutex)
    {
        g_mutex_clear (mutex);
        g_slice_free (GMutex, mutex);
    }
}

static void moo_static_mutex_lock (GMutex* mutex)
{
    g_mutex_lock (mutex);
}

static void moo_static_mutex_unlock (GMutex* mutex)
{
    g_mutex_unlock (mutex);
}

static GThread *
moo_thread_create (GThreadFunc thread_func,
                   gpointer data)
{
    return g_thread_new (NULL, thread_func, data);
}
#else
static GMutex *moo_mutex_new ()
{
    return g_mutex_new ();
}

static void moo_mutex_free (GMutex* mutex)
{
    g_mutex_free (mutex);
}

static void moo_static_mutex_lock (GStaticMutex* mutex)
{
    g_static_mutex_lock (mutex);
}

static void moo_static_mutex_unlock (GStaticMutex* mutex)
{
    g_static_mutex_unlock (mutex);
}

static GThread *
moo_thread_create (GThreadFunc thread_func,
                   gpointer data)
{
    return g_thread_create (thread_func, data, FALSE, NULL);
}
#endif


static QueueClient *
get_event_client (guint id)
{
    QueueClientList *l;

    g_return_val_if_fail (queue.init, NULL);

    for (l = queue.clients; l != NULL; l = l->next)
    {
        QueueClient *s = l->data;

        if (s->id == id)
            return s;
    }

    return NULL;
}


static void
invoke_callback (gpointer        id,
                 EventDataQueue *events)
{
    EventDataList *l;
    QueueClient *client;

    moo_dmsg ("processing events for id %u", GPOINTER_TO_UINT (id));
    client = get_event_client (GPOINTER_TO_UINT (id));

    if (client)
    {
        GList *data_list = NULL;

        for (l = events->head; l != NULL; l = l->next)
        {
            EventData *data = l->data;
            data_list = g_list_prepend (data_list, data->data);
        }

        data_list = g_list_reverse (data_list);

        client->callback (data_list, client->callback_data);

        g_list_free (data_list);
    }

    for (l = events->head; l != NULL; l = l->next)
    {
        EventData *data = l->data;

        if (data->destroy)
            data->destroy (data->data);

        g_free (data);
    }

    event_data_queue_free_links (events);
}


static gboolean
got_data (GIOChannel *io)
{
    GHashTable *data;
    char buf[1];

    moo_static_mutex_lock (&queue_lock);
    data = queue.data;
    queue.data = NULL;
    g_io_channel_read_chars (io, buf, 1, NULL, NULL);
    moo_static_mutex_unlock (&queue_lock);

    g_hash_table_foreach (data, (GHFunc) invoke_callback, NULL);
    g_hash_table_destroy (data);

    return TRUE;
}


void
_moo_event_queue_do_events (guint event_id)
{
    EventDataQueue *events = NULL;

    g_return_if_fail (queue.init);

    moo_static_mutex_lock (&queue_lock);

    if (queue.data)
    {
        events = (EventDataQueue*) g_hash_table_lookup (queue.data, GUINT_TO_POINTER (event_id));

        if (events)
            g_hash_table_remove (queue.data, GUINT_TO_POINTER (event_id));
    }

    moo_static_mutex_unlock (&queue_lock);

    if (events)
        invoke_callback (GUINT_TO_POINTER (event_id), events);
}


static void
init_queue (void)
{
    if (g_atomic_int_get (&queue.init))
        return;

    moo_static_mutex_lock (&queue_lock);

    if (!queue.init)
    {
        MgwFd fds[2];
        GSource *source;

        if (mgw_pipe (fds) != 0)
        {
            mgw_perror ("pipe");
            goto out;
        }

        queue.clients = NULL;

        queue.pipe_in = fds[1];
        queue.pipe_out = fds[0];

#ifdef __WIN32__
        queue.io = mgw_io_channel_win32_new_fd (queue.pipe_out);
#else
        queue.io = mgw_io_channel_unix_new (queue.pipe_out);
#endif

        if (!queue.io)
        {
            g_critical ("g_io_channel_new failed");
            goto out;
        }

        source = g_io_create_watch (queue.io, G_IO_IN);
        g_source_set_callback (source, (GSourceFunc) got_data, NULL, NULL);
        g_source_set_can_recurse (source, TRUE);
        g_source_attach (source, NULL);

        queue.data = NULL;

        queue.init = TRUE;
    }

out:
    moo_static_mutex_unlock (&queue_lock);
}


guint
_moo_event_queue_connect (MooEventQueueCallback callback,
                          gpointer              data,
                          GDestroyNotify        notify)
{
    QueueClient *client;

    g_return_val_if_fail (callback != NULL, 0);

    init_queue ();

    client = g_new0 (QueueClient, 1);
    client->callback = callback;
    client->callback_data = data;
    client->notify = notify;

    moo_static_mutex_lock (&queue_lock);
    client->id = ++queue.last_id;
    queue.clients = queue_client_list_prepend (queue.clients, client);
    moo_static_mutex_unlock (&queue_lock);

    return client->id;
}


void
_moo_event_queue_disconnect (guint event_id)
{
    QueueClient *client;

    g_return_if_fail (event_id != 0);
    g_return_if_fail (queue.init);

    moo_static_mutex_lock (&queue_lock);

    client = get_event_client (event_id);

    if (!client)
        g_warning ("no client with id %d", event_id);
    else
        queue.clients = queue_client_list_remove (queue.clients, client);

    moo_static_mutex_unlock (&queue_lock);

    if (client && client->notify)
        client->notify (client->callback_data);

    g_free (client);
}


/* called from a thread */
void
_moo_event_queue_push (guint          event_id,
                       gpointer       data,
                       GDestroyNotify data_destroy)
{
    char c = 'd';
    EventData *event_data;
    EventDataQueue *events;

    event_data = g_new (EventData, 1);
    event_data->data = data;
    event_data->destroy = data_destroy;
    event_data->id = event_id;

    moo_static_mutex_lock (&queue_lock);

    if (!queue.data)
    {
        seriously_ignore_return_value (mgw_write (queue.pipe_in, &c, 1));
        queue.data = g_hash_table_new (g_direct_hash, g_direct_equal);
    }

    events = (EventDataQueue*) g_hash_table_lookup (queue.data, GUINT_TO_POINTER (event_id));

    if (!events)
    {
        events = event_data_queue_new ();
        g_hash_table_insert (queue.data, GUINT_TO_POINTER (event_id), events);
    }

    event_data_queue_push_tail (events, event_data);

    moo_static_mutex_unlock (&queue_lock);
}


/****************************************************************************/
/* Messages
 */

#if GLIB_CHECK_VERSION(2,32,0)
static GMutex message_lock;
#else
static GStaticMutex message_lock = G_STATIC_MUTEX_INIT;
#endif
static volatile int message_event_id = 0;
static volatile int print_event_id = 0;

static void
message_callback (GList *events, G_GNUC_UNUSED gpointer data)
{
    g_assert (moo_is_main_thread ());

    while (events)
    {
        moo_message_noloc ("%s", (char*) events->data);
        events = events->next;
    }
}

static void
print_callback (GList *events, G_GNUC_UNUSED gpointer data)
{
    g_assert (moo_is_main_thread ());

    while (events)
    {
        g_print ("%s", (char*) events->data);
        events = events->next;
    }
}

static void
init_message_queue (void)
{
    moo_static_mutex_lock (&message_lock);

    if (!message_event_id)
        message_event_id = _moo_event_queue_connect (message_callback, NULL, NULL);
    if (!print_event_id)
        print_event_id = _moo_event_queue_connect (print_callback, NULL, NULL);

    moo_static_mutex_unlock (&message_lock);
}

void
_moo_print_async (const char *format,
                  ...)
{
    char *msg;
    va_list args;

    va_start (args, format);
    msg = g_strdup_vprintf (format, args);
    va_end (args);

    if (msg)
    {
        init_message_queue ();
        _moo_event_queue_push (print_event_id, msg, g_free);
    }
}

void
_moo_message_async (const char *format,
                    ...)
{
    char *msg;
    va_list args;

    va_start (args, format);
    msg = g_strdup_vprintf (format, args);
    va_end (args);

    if (msg)
    {
        init_message_queue ();
        _moo_event_queue_push (message_event_id, msg, g_free);
    }
}


struct MooAsyncJob {
    GObject base;

    MooAsyncJobCallback callback;
    gpointer data;
    GDestroyNotify data_notify;

    GThread *thread;
    GMutex *mutex;
    guint cancelled : 1;
};

typedef struct {
    GObjectClass base_class;
} MooAsyncJobClass;


MOO_DEFINE_TYPE_STATIC (MooAsyncJob, moo_async_job, G_TYPE_OBJECT)

static void
moo_async_job_dispose (GObject *object)
{
    MooAsyncJob *job = (MooAsyncJob*) object;

    if (job->data_notify)
    {
        GDestroyNotify notify = job->data_notify;
        job->data_notify = NULL;
        notify (job->data);
        job->data = NULL;
    }

    g_assert (!job->thread);

    if (job->mutex)
    {
        moo_mutex_free (job->mutex);
        job->mutex = NULL;
    }

    G_OBJECT_CLASS (moo_async_job_parent_class)->dispose (object);
}

static void
moo_async_job_class_init (MooAsyncJobClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = moo_async_job_dispose;
}

static void
moo_async_job_init (MooAsyncJob *job)
{
    job->callback = NULL;
    job->data = NULL;
    job->data_notify = NULL;

    job->thread = NULL;
    job->mutex = moo_mutex_new ();
    job->cancelled = FALSE;
}

MooAsyncJob *
moo_async_job_new (MooAsyncJobCallback callback,
                   gpointer            data,
                   GDestroyNotify      data_notify)
{
    MooAsyncJob *job;

    g_return_val_if_fail (callback != NULL, NULL);

    job = (MooAsyncJob*) g_object_new (moo_async_job_get_type (), (const char*) NULL);
    job->callback = callback;
    job->data = data;
    job->data_notify = data_notify;

    return job;
}

static gpointer
moo_async_job_thread_func (MooAsyncJob *job)
{
    gboolean proceed = TRUE;

    while (proceed)
    {
        g_mutex_lock (job->mutex);

        if (job->cancelled)
        {
            _moo_print_async ("%s: job cancelled\n", G_STRFUNC);
            g_mutex_unlock (job->mutex);
            break;
        }

        g_mutex_unlock (job->mutex);

        proceed = job->callback (job->data);

        if (!proceed)
            _moo_print_async ("%s: job finished\n", G_STRFUNC);

        if (proceed)
            g_usleep (1000);
    }

    g_mutex_lock (job->mutex);

    if (job->data_notify)
    {
        GDestroyNotify notify = job->data_notify;
        job->data_notify = NULL;
        notify (job->data);
    }

    job->thread = NULL;

    g_mutex_unlock (job->mutex);

    g_object_unref (job);
    return NULL;
}

void
moo_async_job_start (MooAsyncJob *job)
{
    GError *error = NULL;

    g_return_if_fail (job != NULL);
    g_return_if_fail (job->thread == NULL);

    g_mutex_lock (job->mutex);

    job->thread = moo_thread_create ((GThreadFunc) moo_async_job_thread_func,
                                     g_object_ref (job));
    if (!job->thread)
    {
        g_critical ("could not start thread: %s", moo_error_message (error));
        g_error_free (error);
        goto out;
    }

out:
    g_mutex_unlock (job->mutex);
}

void
moo_async_job_cancel (MooAsyncJob *job)
{
    g_return_if_fail (job != NULL);
    g_mutex_lock (job->mutex);
    job->cancelled = TRUE;
    g_mutex_unlock (job->mutex);
}
