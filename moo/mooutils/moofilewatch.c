/*
 *   moofilewatch.h
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define WANT_STAT_MONITOR

#ifdef __WIN32__
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

#ifndef __WIN32__
#include <time.h>
#endif

#include <mooglib/moo-glib.h>
#include <mooglib/moo-stat.h>
#include <sys/types.h>
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-mem.h"
#include "mooutils/moofilewatch.h"
#include "mooutils/mootype-macros.h"
#include "marshals.h"
#include "mooutils/mooutils-thread.h"
#include "mooutils/moolist.h"


#if 1
static void  G_GNUC_PRINTF(1,2) DEBUG_PRINT (G_GNUC_UNUSED const char *format, ...)
{
}
#elif defined(__WIN32__)
#define DEBUG_PRINT _moo_message_async
#else
#define DEBUG_PRINT _moo_message
#endif

typedef struct {
    guint id;
    MooFileWatch *watch;
    char *filename;

    MooFileWatchCallback callback;
    GDestroyNotify notify;
    gpointer data;

    MgwStatBuf statbuf;

    guint isdir : 1;
    guint alive : 1;
} Monitor;

struct WatchFuncs {
    gboolean (*start)           (MooFileWatch   *watch,
                                 GError        **error);
    gboolean (*shutdown)        (MooFileWatch   *watch,
                                 GError        **error);

    gboolean (*start_monitor)   (MooFileWatch   *watch,
                                 Monitor        *monitor,
                                 GError        **error);
    void     (*stop_monitor)    (MooFileWatch   *watch,
                                 Monitor        *monitor);
};

MOO_DEFINE_SLIST(MonitorList, monitor_list, Monitor)

struct _MooFileWatch {
    guint ref_count;
    guint id;
    guint stat_timeout;
    MonitorList *monitors;
    GHashTable *requests;  /* int -> Monitor* */
    guint alive : 1;
};

typedef enum
{
    MOO_FILE_WATCH_ERROR_CLOSED,
    MOO_FILE_WATCH_ERROR_FAILED,
    MOO_FILE_WATCH_ERROR_NOT_IMPLEMENTED,
    MOO_FILE_WATCH_ERROR_TOO_MANY,
    MOO_FILE_WATCH_ERROR_NOT_DIR,
    MOO_FILE_WATCH_ERROR_IS_DIR,
    MOO_FILE_WATCH_ERROR_NONEXISTENT,
    MOO_FILE_WATCH_ERROR_BAD_FILENAME,
    MOO_FILE_WATCH_ERROR_ACCESS_DENIED
} MooFileWatchError;

MOO_DEFINE_BOXED_TYPE_R (MooFileWatch, moo_file_watch)

#define MOO_FILE_WATCH_ERROR (moo_file_watch_error_quark ())
MOO_DEFINE_QUARK_STATIC (moo-file-watch-error, moo_file_watch_error_quark)

#ifdef WANT_STAT_MONITOR
static gboolean watch_stat_start            (MooFileWatch   *watch,
                                             GError        **error);
static gboolean watch_stat_shutdown         (MooFileWatch   *watch,
                                             GError        **error);
static gboolean watch_stat_start_monitor    (MooFileWatch   *watch,
                                             Monitor        *monitor,
                                             GError        **error);
#endif

#ifdef __WIN32__
static gboolean watch_win32_start           (MooFileWatch   *watch,
                                             GError        **error);
static gboolean watch_win32_shutdown        (MooFileWatch   *watch,
                                             GError        **error);
static gboolean watch_win32_start_monitor   (MooFileWatch   *watch,
                                             Monitor        *monitor,
                                             GError        **error);
static void     watch_win32_stop_monitor    (MooFileWatch   *watch,
                                             Monitor        *monitor);
#endif /* __WIN32__ */

static Monitor *monitor_new                 (MooFileWatch   *watch,
                                             const char     *filename,
                                             MooFileWatchCallback callback,
                                             gpointer        data,
                                             GDestroyNotify  notify);
static void     monitor_free                (Monitor        *monitor);


static struct WatchFuncs watch_funcs = {
#if defined(__WIN32__)
    watch_win32_start,
    watch_win32_shutdown,
    watch_win32_start_monitor,
    watch_win32_stop_monitor
#else
    watch_stat_start,
    watch_stat_shutdown,
    watch_stat_start_monitor,
    NULL
#endif
};


static guint
get_new_monitor_id (void)
{
    static guint id = 0;

    if (!++id)
        ++id;

    return id;
}

static guint
get_new_watch_id (void)
{
    static guint id = 0;

    if (!++id)
        ++id;

    return id;
}


MooFileWatch *
moo_file_watch_ref (MooFileWatch *watch)
{
    g_return_val_if_fail (watch != NULL, watch);
    watch->ref_count++;
    return watch;
}


void
moo_file_watch_unref (MooFileWatch *watch)
{
    GError *error = NULL;

    g_return_if_fail (watch != NULL);

    if (--watch->ref_count)
        return;

    if (watch->alive)
    {
        g_warning ("finalizing open watch");

        if (!moo_file_watch_close (watch, &error))
        {
            g_warning ("error in moo_file_watch_close(): %s",
                       moo_error_message (error));
            g_error_free (error);
        }
    }

    g_hash_table_destroy (watch->requests);
    g_free (watch);
}


MooFileWatch *
moo_file_watch_new (GError **error)
{
    MooFileWatch *watch;

    watch = g_new0 (MooFileWatch, 1);

    watch->requests = g_hash_table_new (g_direct_hash, g_direct_equal);

    watch->id = get_new_watch_id ();
    watch->ref_count = 1;

    if (!watch_funcs.start (watch, error))
    {
        moo_file_watch_unref (watch);
        return NULL;
    }

    watch->alive = TRUE;
    return watch;
}


static MooFileEvent *
moo_file_event_new (const char      *filename,
                    guint            monitor_id,
                    MooFileEventCode code)
{
    MooFileEvent *event;

    event = g_new0 (MooFileEvent, 1);
    event->filename = g_strdup (filename);
    event->monitor_id = monitor_id;
    event->code = code;
    event->error = NULL;

    return event;
}

static MooFileEvent *
moo_file_event_copy (MooFileEvent *event)
{
    MooFileEvent *copy;

    copy = moo_file_event_new (event->filename,
                               event->monitor_id,
                               event->code);

    if (event->error)
        copy->error = g_error_copy (event->error);

    return copy;
}

static void
moo_file_event_free (MooFileEvent *event)
{
    if (event)
    {
        if (event->error)
            g_error_free (event->error);
        g_free (event->filename);
        g_free (event);
    }
}

MOO_DEFINE_BOXED_TYPE_C (MooFileEvent, moo_file_event)


gboolean
moo_file_watch_close (MooFileWatch   *watch,
                      GError        **error)
{
    MonitorList *monitors;

    g_return_val_if_fail (watch != NULL, FALSE);

    if (!watch->alive)
        return TRUE;

    watch->alive = FALSE;
    monitors = watch->monitors;
    watch->monitors = NULL;

    while (monitors)
    {
        Monitor *mon = monitors->data;

        if (watch_funcs.stop_monitor)
            watch_funcs.stop_monitor (watch, mon);

        monitor_free (mon);
        monitors = monitor_list_delete_link (monitors, monitors);
    }

    return watch_funcs.shutdown (watch, error);
}


guint
moo_file_watch_create_monitor (MooFileWatch   *watch,
                               const char     *filename,
                               MooFileWatchCallback callback,
                               gpointer        data,
                               GDestroyNotify  notify,
                               GError        **error)
{
    Monitor *monitor;

    g_return_val_if_fail (watch != NULL, 0);
    g_return_val_if_fail (filename != NULL, 0);
    g_return_val_if_fail (callback != NULL, 0);

    if (!watch->alive)
    {
        g_set_error (error, MOO_FILE_WATCH_ERROR,
                     MOO_FILE_WATCH_ERROR_CLOSED,
                     "MooFileWatch %u closed",
                     watch->id);
        return 0;
    }

    monitor = monitor_new (watch, filename, callback, data, notify);

    if (!watch_funcs.start_monitor (watch, monitor, error))
    {
        monitor_free (monitor);
        return 0;
    }

    monitor->alive = TRUE;
    watch->monitors = monitor_list_prepend (watch->monitors, monitor);
    g_hash_table_insert (watch->requests, GUINT_TO_POINTER (monitor->id), monitor);

    DEBUG_PRINT ("created monitor %d for '%s'", monitor->id, monitor->filename);

    return monitor->id;
}


void
moo_file_watch_cancel_monitor (MooFileWatch *watch,
                               guint         monitor_id)
{
    Monitor *monitor;

    g_return_if_fail (watch != NULL);

    monitor = (Monitor*) g_hash_table_lookup (watch->requests,
                                              GUINT_TO_POINTER (monitor_id));
    g_return_if_fail (monitor != NULL);

    watch->monitors = monitor_list_remove (watch->monitors, monitor);
    g_hash_table_remove (watch->requests, GUINT_TO_POINTER (monitor->id));

    if (monitor->alive)
        DEBUG_PRINT ("stopping monitor %d for '%s'",
                     monitor->id, monitor->filename);
    else
        DEBUG_PRINT ("stopping dead monitor %d for '%s'",
                     monitor->id, monitor->filename);

    if (monitor->alive && watch_funcs.stop_monitor)
        watch_funcs.stop_monitor (watch, monitor);

    monitor_free (monitor);
}


static void
moo_file_watch_emit_event (MooFileWatch *watch,
                           MooFileEvent *event,
                           Monitor      *monitor)
{
    moo_file_watch_ref (watch);

    if (monitor->alive || event->code == MOO_FILE_EVENT_ERROR)
    {
        static const char *names[] = {
            "changed", "created", "deleted", "error"
        };
        DEBUG_PRINT ("emitting event %s for %s", names[event->code], event->filename);
        monitor->callback (watch, event, monitor->data);
    }

    moo_file_watch_unref (watch);
}


static Monitor *
monitor_new (MooFileWatch   *watch,
             const char     *filename,
             MooFileWatchCallback callback,
             gpointer        data,
             GDestroyNotify  notify)
{
    Monitor *mon;

    mon = g_new0 (Monitor, 1);
    mon->watch = watch;
    mon->filename = g_strdup (filename);

    mon->callback = callback;
    mon->notify = notify;
    mon->data = data;

    return mon;
}


static void
monitor_free (Monitor *monitor)
{
    if (monitor)
    {
        if (monitor->notify)
            monitor->notify (monitor->data);
        g_free (monitor->filename);
        g_free (monitor);
    }
}


/*****************************************************************************/
/* stat()
 */

#ifdef WANT_STAT_MONITOR

#define MOO_STAT_PRIORITY   G_PRIORITY_DEFAULT
#define MOO_STAT_TIMEOUT    500

static MooFileWatchError errno_to_file_error    (mgw_errno_t     code);
static gboolean do_stat                         (MooFileWatch   *watch);


static gboolean
watch_stat_start (MooFileWatch *watch,
                  G_GNUC_UNUSED GError **error)
{
    watch->stat_timeout =
            g_timeout_add_full (MOO_STAT_PRIORITY,
                                MOO_STAT_TIMEOUT,
                                (GSourceFunc) do_stat,
                                watch, NULL);
    return TRUE;
}


static gboolean
watch_stat_shutdown (MooFileWatch *watch,
                     G_GNUC_UNUSED GError **error)
{
    if (watch->stat_timeout)
        g_source_remove (watch->stat_timeout);
    watch->stat_timeout = 0;
    return TRUE;
}


static gboolean
watch_stat_start_monitor (MooFileWatch   *watch,
                          Monitor        *monitor,
                          GError        **error)
{
    MgwStatBuf buf;
    mgw_errno_t err;

    g_return_val_if_fail (watch != NULL, FALSE);
    g_return_val_if_fail (monitor->filename != NULL, FALSE);

    if (mgw_stat (monitor->filename, &buf, &err) != 0)
    {
        g_set_error (error, MOO_FILE_WATCH_ERROR,
                     errno_to_file_error (err),
                     "stat: %s", mgw_strerror (err));
        return FALSE;
    }

    monitor->isdir = buf.isdir;

#ifdef __WIN32__
    if (monitor->isdir) /* it's fatal on windows */
    {
        char *display_name = g_filename_display_name (monitor->filename);
        g_set_error (error, MOO_FILE_WATCH_ERROR,
                     MOO_FILE_WATCH_ERROR_FAILED,
                     "%s is a directory", display_name);
        g_free (display_name);
        return FALSE;
    }
#endif

    monitor->id = get_new_monitor_id ();
    monitor->statbuf = buf;

    return TRUE;
}


static gboolean
do_stat (MooFileWatch *watch)
{
    MonitorList *lm;
    GSList *list = NULL, *lid;
    GSList *to_remove = NULL;
    gboolean result = TRUE;

    g_return_val_if_fail (watch != NULL, FALSE);

    moo_file_watch_ref (watch);

    if (!watch->monitors)
        goto out;

    for (lm = watch->monitors; lm != NULL; lm = lm->next)
    {
        Monitor *m = lm->data;
        list = g_slist_prepend (list, GUINT_TO_POINTER (m->id));
    }

    /* Order of list is correct now, watch->monitors is last-added-first */
    for (lid = list; lid != NULL; lid = lid->next)
    {
        gboolean do_emit = FALSE;
        MooFileEvent event;
        Monitor *monitor;
        mgw_time_t old;
        mgw_errno_t err;

        monitor = (Monitor*) g_hash_table_lookup (watch->requests, lid->data);

        if (!monitor || !monitor->alive)
            continue;

        old = monitor->statbuf.mtime;

        event.monitor_id = monitor->id;
        event.filename = monitor->filename;
        event.error = NULL;

        if (mgw_stat (monitor->filename, &monitor->statbuf, &err) != 0)
        {
            if (err.value == MGW_ENOENT)
            {
                event.code = MOO_FILE_EVENT_DELETED;
                to_remove = g_slist_prepend (to_remove, GUINT_TO_POINTER (monitor->id));
            }
            else
            {
                event.code = MOO_FILE_EVENT_ERROR;
                g_set_error (&event.error, MOO_FILE_WATCH_ERROR,
                             errno_to_file_error (err),
                             "stat failed: %s",
                             mgw_strerror (err));
                monitor->alive = FALSE;
            }

            do_emit = TRUE;
        }
        else if (monitor->statbuf.mtime.value > old.value)
        {
            event.code = MOO_FILE_EVENT_CHANGED;
            do_emit = TRUE;
        }

        if (do_emit)
            moo_file_watch_emit_event (watch, &event, monitor);

        if (event.error)
            g_error_free (event.error);
    }

    for (lid = to_remove; lid != NULL; lid = lid->next)
        if (g_hash_table_lookup (watch->requests, GUINT_TO_POINTER (lid->data)))
            moo_file_watch_cancel_monitor (watch, GPOINTER_TO_UINT (lid->data));

    g_slist_free (to_remove);
    g_slist_free (list);

out:
    moo_file_watch_unref (watch);
    return result;
}


static MooFileWatchError
errno_to_file_error (mgw_errno_t code)
{
    MooFileWatchError fcode = MOO_FILE_WATCH_ERROR_FAILED;

    switch (code.value)
    {
#ifdef EACCESS
        case MGW_EACCES:
            fcode = MOO_FILE_WATCH_ERROR_ACCESS_DENIED;
            break;
#endif
        case MGW_ENAMETOOLONG:
            fcode = MOO_FILE_WATCH_ERROR_BAD_FILENAME;
            break;
        case MGW_ENOENT:
            fcode = MOO_FILE_WATCH_ERROR_NONEXISTENT;
            break;
        case MGW_ENOTDIR:
            fcode = MOO_FILE_WATCH_ERROR_NOT_DIR;
            break;

        default:
            break;
    }

    return fcode;
}


#endif /* WANT_STAT_MONITOR */

/*****************************************************************************/
/* win32
 */
#ifdef __WIN32__

typedef struct {
    guint watch_id;
    guint request;
    char *path;
} FAMThreadWatch;

typedef struct {
    GMutex *lock;
    GAsyncQueue *incoming;
    HANDLE events[MAXIMUM_WAIT_OBJECTS];
    FAMThreadWatch watches[MAXIMUM_WAIT_OBJECTS];
    guint n_events;
    gboolean running;
} FAMThread;

typedef enum {
    COMMAND_ADD_PATH,
    COMMAND_REMOVE_PATH
} FAMThreadCommandType;

typedef struct {
    FAMThreadCommandType type;
    char *path;
    guint watch_id;
    guint request;
} FAMThreadCommand;

typedef struct {
    FAMThread thread_data;
    guint event_id;
    gboolean running;
    GSList *watches;
} FAMWin32;

typedef struct {
    gboolean error;
    guint watch_id;
    guint request;
    guint code;
    char *msg;
} FAMEvent;

static FAMWin32 *fam;


/****************************************************************************/
/* Watch thread
 */

static void
fam_event_free (FAMEvent *event)
{
    if (event)
    {
        g_free (event->msg);
        g_free (event);
    }
}

static void
fam_thread_event (guint       code,
                  gboolean    error,
                  const char *msg,
                  guint       watch_id,
                  guint       request)
{
    FAMEvent *event = g_new0 (FAMEvent, 1);
    event->error = error;
    event->code = code;
    event->watch_id = watch_id;
    event->request = request;
    event->msg = g_strdup (msg);
    _moo_event_queue_push (fam->event_id, event, (GDestroyNotify) fam_event_free);
}

static void
fam_thread_add_path (FAMThread  *thr,
                     const char *path,
                     guint       watch_id,
                     guint       request)
{
    gunichar2 *win_path;

    if (thr->n_events >= MAXIMUM_WAIT_OBJECTS)
    {
        g_critical ("too many folders watched");
        return;
    }

    win_path = g_utf8_to_utf16 (path, -1, NULL, NULL, NULL);

    if (!win_path)
    {
        char *msg = g_strdup_printf ("Could not convert filename '%s' to UTF16",
                                     path);
        fam_thread_event (MOO_FILE_WATCH_ERROR_BAD_FILENAME, TRUE, msg, watch_id, request);
        g_free (msg);
        return;
    }

    thr->events[thr->n_events] =
        FindFirstChangeNotificationW ((const WCHAR*) win_path, FALSE,
                                      FILE_NOTIFY_CHANGE_FILE_NAME |
                                      FILE_NOTIFY_CHANGE_DIR_NAME);

    if (thr->events[thr->n_events] == INVALID_HANDLE_VALUE)
    {
        char *err = g_win32_error_message (GetLastError ());
        char *msg = g_strdup_printf ("Could not convert start watch for '%s': %s", path, err);
        fam_thread_event (MOO_FILE_WATCH_ERROR_FAILED, TRUE, msg, watch_id, request);
        g_free (msg);
        g_free (err);
        return;
    }

    thr->watches[thr->n_events].watch_id = watch_id;
    thr->watches[thr->n_events].request = request;
    thr->watches[thr->n_events].path = g_strdup (path);

    thr->n_events += 1;
}

static void
fam_thread_remove_path (FAMThread  *thr,
                        guint       watch_id,
                        guint       request)
{
    guint i;

    for (i = 1; i < thr->n_events; ++i)
    {
        if (thr->watches[i].watch_id == watch_id &&
            thr->watches[i].request == request)
        {
            FindCloseChangeNotification (thr->events[i]);
            g_free (thr->watches[i].path);

            if (i < thr->n_events - 1)
            {
                MOO_ELMMOVE (thr->watches + i,
                             thr->watches + i + 1,
                             thr->n_events - i - 1);
                MOO_ELMMOVE (thr->events + i,
                             thr->events + i + 1,
                             thr->n_events - i - 1);
            }

            thr->n_events -= 1;

            return;
        }
    }

    g_critical ("can't remove watch %d, request %d",
                watch_id, request);
}

static void
fam_thread_check_dir (FAMThread *thr,
                      guint      idx)
{
    MgwStatBuf buf;
    mgw_errno_t err;

    if (mgw_stat (thr->watches[idx].path, &buf, &err) != 0 && err.value == MGW_ENOENT)
    {
        fam_thread_event (MOO_FILE_EVENT_DELETED,
                          FALSE, NULL,
                          thr->watches[idx].watch_id,
                          thr->watches[idx].request);
        fam_thread_remove_path (thr,
                                thr->watches[idx].watch_id,
                                thr->watches[idx].request);
    }
    else
    {
        fam_thread_event (MOO_FILE_EVENT_CHANGED,
                          FALSE, NULL,
                          thr->watches[idx].watch_id,
                          thr->watches[idx].request);

        if (!FindNextChangeNotification (thr->events[idx]))
        {
            char *err = g_win32_error_message (GetLastError ());
            char *msg = g_strdup_printf ("Error in FindNextChangeNotification: %s", err);

            fam_thread_event (MOO_FILE_WATCH_ERROR_FAILED, TRUE, msg,
                              thr->watches[idx].watch_id,
                              thr->watches[idx].request);
            fam_thread_remove_path (thr,
                                    thr->watches[idx].watch_id,
                                    thr->watches[idx].request);

            g_free (msg);
            g_free (err);
        }
    }
}

static void
fam_thread_do_command (FAMThread *thr)
{
    FAMThreadCommand *cmd;
    GSList *list = NULL;

    DEBUG_PRINT ("fam_thread_do_command start");

    g_mutex_lock (thr->lock);
    while ((cmd = (FAMThreadCommand*) g_async_queue_try_pop (thr->incoming)))
        list = g_slist_prepend (list, cmd);
    ResetEvent (thr->events[0]);
    g_mutex_unlock (thr->lock);

    list = g_slist_reverse (list);

    while (list)
    {
        cmd = (FAMThreadCommand*) list->data;

        switch (cmd->type)
        {
            case COMMAND_ADD_PATH:
                fam_thread_add_path (thr, cmd->path, cmd->watch_id, cmd->request);
                break;

            case COMMAND_REMOVE_PATH:
                fam_thread_remove_path (thr, cmd->watch_id, cmd->request);
                break;
        }

        list = g_slist_delete_link (list, list);
        g_free (cmd->path);
        g_free (cmd);
    }

    DEBUG_PRINT ("fam_thread_do_command end");
}


static gpointer
fam_thread_main (FAMThread *thr)
{
    while (TRUE)
    {
        int ret;

        DEBUG_PRINT ("calling WaitForMultipleObjects for %d handles", thr->n_events);
        ret = WaitForMultipleObjects (thr->n_events, thr->events, FALSE, INFINITE);
        DEBUG_PRINT ("WaitForMultipleObjects returned %d", ret);

        if (ret == (int) WAIT_FAILED)
        {
            char *msg = g_win32_error_message (GetLastError ());
            g_critical ("%s", msg);
            g_free (msg);
            break;
        }

        if (ret == WAIT_OBJECT_0)
            fam_thread_do_command (thr);
        else if ((int) WAIT_OBJECT_0 < ret && ret < (int) thr->n_events)
            fam_thread_check_dir (thr, ret - WAIT_OBJECT_0);
        else
        {
            g_critical ("oops");
            break;
        }
    }

    /* XXX cleanup */
    return NULL;
}

static void
fam_thread_command (FAMThreadCommandType type,
                    const char          *filename,
                    guint                watch_id,
                    guint                request)
{
    FAMThreadCommand *cmd;

    cmd = g_new0 (FAMThreadCommand, 1);
    cmd->type = type;
    cmd->path = g_strdup (filename);
    cmd->watch_id = watch_id;
    cmd->request = request;

    g_mutex_lock (fam->thread_data.lock);
    g_async_queue_push (fam->thread_data.incoming, cmd);
    SetEvent (fam->thread_data.events[0]);
    g_mutex_unlock (fam->thread_data.lock);
}

/****************************************************************************/
/* Monitors
 */

static MooFileWatch *
find_watch (guint id)
{
    GSList *l;

    for (l = fam->watches; l != NULL; l = l->next)
    {
        MooFileWatch *watch = (MooFileWatch*) l->data;

        if (watch->id == id)
            return watch;
    }

    return NULL;
}

static void
do_event (FAMEvent *event)
{
    MooFileWatch *watch;
    Monitor *monitor;
    MooFileEvent watch_event;

    watch = find_watch (event->watch_id);

    if (!watch)
    {
        DEBUG_PRINT ("got event for dead watch %d", event->watch_id);
        return;
    }

    monitor = (Monitor*)
        g_hash_table_lookup (watch->requests, GUINT_TO_POINTER (event->request));

    if (!monitor)
    {
        DEBUG_PRINT ("got event for dead monitor %d", event->request);
        return;
    }

    watch_event.monitor_id = event->request;
    watch_event.filename = monitor->filename;
    watch_event.error = NULL;

    if (event->error)
    {
        watch_event.code = MOO_FILE_EVENT_ERROR;
        g_set_error (&watch_event.error, MOO_FILE_WATCH_ERROR,
                     event->code,
                     event->msg ? event->msg : "FAILED");
        DEBUG_PRINT ("got error for watch %d: %s", event->watch_id, watch_event.error->message);
    }
    else
    {
        DEBUG_PRINT ("got event for filename %s", monitor->filename);
        watch_event.code = (MooFileEventCode) event->code;
    }

    moo_file_watch_emit_event (watch, &watch_event, monitor);

    if (watch_event.error)
        g_error_free (watch_event.error);
}

static void
event_callback (GList *events)
{
    GList *trimmed = NULL;

    while (events)
    {
        GList *l;
        FAMEvent *event = (FAMEvent*) events->data;
        gboolean found = FALSE;

        event = (FAMEvent*) events->data;
        events = events->next;

        for (l = trimmed; l != NULL; l = l->next)
        {
            FAMEvent *old_event = (FAMEvent*) l->data;

            if (old_event->watch_id == event->watch_id &&
                old_event->request == event->request)
            {
                found = TRUE;

                if (!old_event->error && event->error)
                    l->data = event;

                break;
            }
        }

        if (!found)
            trimmed = g_list_prepend (trimmed, event);
    }

    trimmed = g_list_reverse (trimmed);

    while (trimmed)
    {
        do_event ((FAMEvent*) trimmed->data);
        trimmed = g_list_delete_link (trimmed, trimmed);
    }
}

static gboolean
fam_win32_init (void)
{
    if (!fam)
    {
        GError *error = NULL;

        fam = g_new0 (FAMWin32, 1);
        fam->running = FALSE;

        fam->thread_data.n_events = 1;
        fam->thread_data.events[0] = CreateEvent (NULL, TRUE, FALSE, NULL);

        if (!fam->thread_data.events[0])
        {
            char *msg = g_win32_error_message (GetLastError ());
            g_critical ("could not create incoming event: %s", msg);
            g_free (msg);
            return FALSE;
        }

        fam->thread_data.incoming = g_async_queue_new ();
        fam->thread_data.lock = g_mutex_new ();
        fam->event_id = _moo_event_queue_connect ((MooEventQueueCallback) event_callback,
                                                  NULL, NULL);

        if (!g_thread_create ((GThreadFunc) fam_thread_main, &fam->thread_data, FALSE, &error))
        {
            g_critical ("could not start watch thread: %s", moo_error_message (error));
            g_error_free (error);
        }
        else
        {
            fam->running = TRUE;
            DEBUG_PRINT ("initialized folder watch");
        }
    }

    return fam->running;
}

static gboolean
watch_win32_start (MooFileWatch   *watch,
                   GError        **error)
{
    if (!watch_stat_start (watch, error))
        return FALSE;

    if (!fam_win32_init ())
    {
        g_set_error (error, MOO_FILE_WATCH_ERROR,
                     MOO_FILE_WATCH_ERROR_FAILED,
                     "Could not initialize folder watch thread");
        watch_stat_shutdown (watch, NULL);
        return FALSE;
    }

    fam->watches = g_slist_prepend (fam->watches, watch);
    DEBUG_PRINT ("started watch %d", watch->id);

    return TRUE;
}


static gboolean
watch_win32_shutdown (MooFileWatch   *watch,
                      GError        **error)
{
    return watch_stat_shutdown (watch, error);
}


static gboolean
watch_win32_start_monitor (MooFileWatch   *watch,
                           Monitor        *monitor,
                           GError        **error)
{
    MgwStatBuf buf;
    mgw_errno_t err;

    g_return_val_if_fail (watch != NULL, FALSE);
    g_return_val_if_fail (monitor->filename != NULL, FALSE);

    if (mgw_stat (monitor->filename, &buf, &err) != 0)
    {
        g_set_error (error, MOO_FILE_WATCH_ERROR,
                     errno_to_file_error (err),
                     "stat: %s", mgw_strerror (err));
        return FALSE;
    }

    monitor->isdir = buf.isdir;

    if (!monitor->isdir)
        return watch_stat_start_monitor (watch, monitor, error);

    DEBUG_PRINT ("created monitor for '%s'", monitor->filename);

    monitor->id = get_new_monitor_id ();

    fam_thread_command (COMMAND_ADD_PATH, monitor->filename, watch->id, monitor->id);

    return TRUE;
}


static void
watch_win32_stop_monitor (MooFileWatch *watch,
                          Monitor      *monitor)
{
    g_return_if_fail (monitor != NULL);

    if (monitor->isdir)
    {
        DEBUG_PRINT ("removing monitor for '%s'", monitor->filename);
        fam_thread_command (COMMAND_REMOVE_PATH, monitor->filename,
                            watch->id, monitor->id);
    }
}
#endif /* __WIN32__ */
