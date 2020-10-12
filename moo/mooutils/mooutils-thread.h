/*
 *   mooutils-thread.h
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

#ifndef MOO_UTILS_WIN32_H
#define MOO_UTILS_WIN32_H

#include <mooglib/moo-glib.h>

G_BEGIN_DECLS


typedef void (*MooEventQueueCallback)   (GList   *events,
                                         gpointer data);

typedef struct MooAsyncJob MooAsyncJob;
typedef gboolean (*MooAsyncJobCallback) (gpointer data);

MooAsyncJob *moo_async_job_new          (MooAsyncJobCallback    callback,
                                         gpointer               data,
                                         GDestroyNotify         data_notify);
void         moo_async_job_start        (MooAsyncJob           *job);
void         moo_async_job_cancel       (MooAsyncJob           *job);

guint       _moo_event_queue_connect    (MooEventQueueCallback  callback,
                                         gpointer               data,
                                         GDestroyNotify         notify);
void        _moo_event_queue_disconnect (guint                  event_id);

void        _moo_event_queue_do_events  (guint                  event_id);

/* called from a thread */
void        _moo_event_queue_push       (guint                  event_id,
                                         gpointer               data,
                                         GDestroyNotify         data_destroy);

void        _moo_print_async            (const char            *format,
                                         ...) G_GNUC_PRINTF(1,2);
void        _moo_message_async          (const char            *format,
                                         ...) G_GNUC_PRINTF(1,2);


G_END_DECLS

#endif /* MOO_UTILS_WIN32_H */
