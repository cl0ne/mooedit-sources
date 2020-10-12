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

/* Files and directory monitor. Uses stat().
   On win32 does FindFirstChangeNotification and ReadDirectoryChangesW. */

#ifndef MOO_FILE_WATCH_H
#define MOO_FILE_WATCH_H

#include <glib-object.h>

G_BEGIN_DECLS


#define MOO_TYPE_FILE_WATCH         (moo_file_watch_get_type ())
#define MOO_TYPE_FILE_EVENT         (moo_file_event_get_type ())

typedef enum {
    MOO_FILE_EVENT_CHANGED,
    MOO_FILE_EVENT_CREATED,
    MOO_FILE_EVENT_DELETED,
    MOO_FILE_EVENT_ERROR
} MooFileEventCode;

struct _MooFileEvent {
    MooFileEventCode code;
    guint            monitor_id;
    char            *filename;
    GError          *error;
};

typedef struct _MooFileWatch          MooFileWatch;
typedef struct _MooFileEvent          MooFileEvent;

typedef void (*MooFileWatchCallback) (MooFileWatch *watch,
                                      MooFileEvent *event,
                                      gpointer      user_data);


GType           moo_file_watch_get_type             (void) G_GNUC_CONST;
GType           moo_file_event_get_type             (void) G_GNUC_CONST;

/* FAMOpen */
MooFileWatch   *moo_file_watch_new                  (GError        **error);

MooFileWatch   *moo_file_watch_ref                  (MooFileWatch   *watch);
void            moo_file_watch_unref                (MooFileWatch   *watch);

/* FAMClose */
gboolean        moo_file_watch_close                (MooFileWatch   *watch,
                                                     GError        **error);

/* FAMMonitorDirectory, FAMMonitorFile */
guint           moo_file_watch_create_monitor       (MooFileWatch   *watch,
                                                     const char     *filename,
                                                     MooFileWatchCallback callback,
                                                     gpointer        data,
                                                     GDestroyNotify  notify,
                                                     GError        **error);
/* FAMCancelMonitor */
void            moo_file_watch_cancel_monitor       (MooFileWatch   *watch,
                                                     guint           monitor_id);


G_END_DECLS

#endif /* MOO_FILE_WATCH_H */
