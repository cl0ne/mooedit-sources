/*
 *   moofolder.c
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

#include "moofileview/moofilesystem.h"
#include "moofileview/moofolder-private.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils-misc.h"
#include "marshals.h"
#include <mooglib/moo-glib.h>
#include <mooglib/moo-stat.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <time.h>
#include <gtk/gtk.h>

#define NORMAL_PRIORITY         G_PRIORITY_DEFAULT_IDLE
#define NORMAL_TIMEOUT          0.04
#define BACKGROUND_PRIORITY     G_PRIORITY_LOW
#define BACKGROUND_TIMEOUT      0.001

#define TIMER_CLEAR(timer)  \
G_STMT_START {              \
    g_timer_start (timer);  \
    g_timer_stop (timer);   \
} G_STMT_END

#if 0
#define PRINT_TIMES g_print
#else
static void G_GNUC_PRINTF(1,2) PRINT_TIMES (G_GNUC_UNUSED const char *format, ...)
{
}
#endif

static void     moo_folder_dispose          (GObject        *object);

static void     moo_folder_deleted          (MooFolderImpl  *impl);
static gboolean moo_folder_do_reload        (MooFolderImpl  *impl);

static void     stop_populate               (MooFolderImpl  *impl);

static void     files_list_free             (GSList        **list);

static gboolean get_icons_a_bit             (MooFolderImpl  *impl);
static gboolean get_stat_a_bit              (MooFolderImpl  *impl);
static double   get_names                   (MooFolderImpl  *impl);

#define FILE_PATH(folder,file)  g_build_filename (folder->impl->path, file->name, NULL)

static void start_monitor                   (MooFolderImpl  *impl);
static void stop_monitor                    (MooFolderImpl  *impl);

static GSList *hash_table_to_file_list (GHashTable *files);
static void diff_hash_tables    (GHashTable *table1,
                                 GHashTable *table2,
                                 GSList    **only_1,
                                 GSList    **only_2);


/* MOO_TYPE_FOLDER */
G_DEFINE_TYPE (MooFolder, _moo_folder, G_TYPE_OBJECT)

enum {
    DELETED,
    FILES_ADDED,
    FILES_REMOVED,
    FILES_CHANGED,
    NUM_SIGNALS
};

static guint signals[NUM_SIGNALS];

static void
_moo_folder_class_init (MooFolderClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = moo_folder_dispose;

    signals[DELETED] =
            g_signal_new ("deleted",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_FIRST,
                          G_STRUCT_OFFSET (MooFolderClass, deleted),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[FILES_ADDED] =
            g_signal_new ("files-added",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_FIRST,
                          G_STRUCT_OFFSET (MooFolderClass, files_added),
                          NULL, NULL,
                          _moo_marshal_VOID__POINTER,
                          G_TYPE_NONE, 1,
                          G_TYPE_POINTER);

    signals[FILES_REMOVED] =
            g_signal_new ("files-removed",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_FIRST,
                          G_STRUCT_OFFSET (MooFolderClass, files_removed),
                          NULL, NULL,
                          _moo_marshal_VOID__POINTER,
                          G_TYPE_NONE, 1,
                          G_TYPE_POINTER);

    signals[FILES_CHANGED] =
            g_signal_new ("files-changed",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_FIRST,
                          G_STRUCT_OFFSET (MooFolderClass, files_changed),
                          NULL, NULL,
                          _moo_marshal_VOID__POINTER,
                          G_TYPE_NONE, 1,
                          G_TYPE_POINTER);
}


static void
_moo_folder_init (G_GNUC_UNUSED MooFolder *folder)
{
}


static MooFolderImpl *
moo_folder_impl_new (MooFileSystem *fs,
                     const char    *path,
                     GDir          *dir)
{
    MooFolderImpl *impl;

    impl = g_new0 (MooFolderImpl, 1);
    impl->deleted = FALSE;
    impl->done = 0;
    impl->wanted = 0;
    impl->fs = fs;
    impl->dir = dir;
    impl->files_copy = NULL;
    impl->path = g_strdup (path);
    impl->populate_func = NULL;
    impl->populate_idle_id = 0;
    impl->populate_timeout = BACKGROUND_TIMEOUT;
    impl->timer = g_timer_new ();
    g_timer_stop (impl->timer);

    impl->files = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                         (GDestroyNotify) _moo_file_unref);

    return impl;
}


static void
folder_shutdown (MooFolderImpl *impl)
{
    stop_populate (impl);
    stop_monitor (impl);
    files_list_free (&impl->files_copy);

    if (impl->reload_idle)
        g_source_remove (impl->reload_idle);
    impl->reload_idle = 0;

    if (impl->files)
        g_hash_table_destroy (impl->files);
    impl->files = NULL;

    if (impl->dir)
        g_dir_close (impl->dir);
    impl->dir = NULL;

    if (impl->populate_idle_id)
        g_source_remove (impl->populate_idle_id);
    impl->populate_idle_id = 0;

    if (impl->timer)
        g_timer_destroy (impl->timer);
    impl->timer = NULL;
}


void
_moo_folder_impl_free (MooFolderImpl *impl)
{
    g_return_if_fail (impl != NULL);
    folder_shutdown (impl);
    g_free (impl->path);
    g_free (impl);
}


static void
moo_folder_dispose (GObject *object)
{
    MooFolder *folder = MOO_FOLDER (object);

    if (folder->impl)
    {
        MooFileSystem *fs = folder->impl->fs;
        folder->impl->proxy = NULL;
        _moo_file_system_folder_finalized (fs, folder);
        folder->impl = NULL;
        g_object_unref (fs);
    }

    G_OBJECT_CLASS (_moo_folder_parent_class)->finalize (object);
}


static void
add_file_size (G_GNUC_UNUSED const char *filename,
               MooFile *file,
               gsize *mem)
{
    *mem += sizeof *file;
#define STRING_SIZE(s) ((s) ? (strlen (s) + 1) : 0)
    *mem += STRING_SIZE (file->name);
    *mem += STRING_SIZE (file->link_target);
    *mem += STRING_SIZE (file->display_name);
    *mem += STRING_SIZE (file->case_display_name);
    *mem += _moo_collation_key_size (file->collation_key);
#undef STRING_SIZE
}

gsize
_moo_folder_mem_usage (MooFolder *folder)
{
    gsize mem = 0;
    mem += sizeof (MooFolderImpl);
    g_hash_table_foreach (folder->impl->files, (GHFunc) add_file_size, &mem);
    return mem;
}


MooFolder *
_moo_folder_new_with_impl (MooFolderImpl *impl)
{
    MooFolder *folder = MOO_FOLDER (g_object_new (MOO_TYPE_FOLDER, (const char*) NULL));
    folder->impl = impl;
    impl->proxy = folder;
    g_object_ref (impl->fs);
    return folder;
}


MooFolder *
_moo_folder_new (MooFileSystem  *fs,
                 const char     *path,
                 MooFileFlags    wanted,
                 GError        **error)
{
    GDir *dir;
    MooFolderImpl *impl;
    MooFolder *folder;
    GError *file_error = NULL;

    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), NULL);
    g_return_val_if_fail (path != NULL, NULL);

    dir = g_dir_open (path, 0, &file_error);

    if (!dir)
    {
        if (file_error->domain != G_FILE_ERROR)
        {
            g_set_error (error, MOO_FILE_ERROR,
                         MOO_FILE_ERROR_FAILED,
                         "%s", file_error->message);
        }
        else switch (file_error->code)
        {
            case G_FILE_ERROR_NOENT:
                g_set_error (error, MOO_FILE_ERROR,
                             MOO_FILE_ERROR_NONEXISTENT,
                             "%s", file_error->message);
                break;
            case G_FILE_ERROR_NOTDIR:
                g_set_error (error, MOO_FILE_ERROR,
                             MOO_FILE_ERROR_NOT_FOLDER,
                             "%s", file_error->message);
                break;
            case G_FILE_ERROR_NAMETOOLONG:
            case G_FILE_ERROR_LOOP:
                g_set_error (error, MOO_FILE_ERROR,
                             MOO_FILE_ERROR_BAD_FILENAME,
                             "%s", file_error->message);
                break;
            default:
                g_set_error (error, MOO_FILE_ERROR,
                             MOO_FILE_ERROR_FAILED,
                             "%s", file_error->message);
                break;
        }

        g_error_free (file_error);
        return NULL;
    }

    impl = moo_folder_impl_new (fs, path, dir);

    folder = _moo_folder_new_with_impl (impl);
    get_names (folder->impl);
    _moo_folder_set_wanted (folder, wanted, TRUE);

    return folder;
}


static void
moo_folder_deleted (MooFolderImpl *impl)
{
    folder_shutdown (impl);
    impl->deleted = TRUE;
    _moo_file_system_folder_deleted (impl->fs, impl);
}


void
_moo_folder_set_wanted (MooFolder      *folder,
                        MooFileFlags    wanted,
                        gboolean        bit_now)
{
    Stage wanted_stage = STAGE_NAMES;

    g_return_if_fail (MOO_IS_FOLDER (folder));
    g_return_if_fail (!folder->impl->deleted);

    if (wanted & MOO_FILE_HAS_ICON)
        wanted_stage = STAGE_MIME_TYPE;
    else if (wanted & MOO_FILE_HAS_MIME_TYPE)
        wanted_stage = STAGE_MIME_TYPE;
    else if (wanted & MOO_FILE_HAS_STAT)
        wanted_stage = STAGE_STAT;

    if (wanted_stage <= folder->impl->done)
        return;

    if (folder->impl->wanted > folder->impl->done)
    {
        g_assert (folder->impl->populate_idle_id != 0);
        folder->impl->wanted = MAX (folder->impl->wanted, wanted_stage);
        return;
    }

    folder->impl->wanted = wanted_stage;

    if (folder->impl->wanted_bg != 0)
    {
        g_assert (folder->impl->populate_idle_id != 0);
        g_assert (folder->impl->populate_func != NULL);
        g_assert (folder->impl->populate_priority != 0);
        g_source_remove (folder->impl->populate_idle_id);
    }
    else
    {
        g_assert (folder->impl->populate_idle_id == 0);

        switch (folder->impl->done)
        {
            case STAGE_NAMES:
                g_assert (folder->impl->dir == NULL);
                folder->impl->populate_func = (GSourceFunc) get_stat_a_bit;
                break;
            case STAGE_STAT:
                g_assert (folder->impl->dir == NULL);
                folder->impl->populate_func = (GSourceFunc) get_icons_a_bit;
                break;
            default:
                g_assert_not_reached ();
        }
    }

    folder->impl->wanted_bg = STAGE_MIME_TYPE;
    folder->impl->populate_timeout = NORMAL_TIMEOUT;
    folder->impl->populate_priority = NORMAL_PRIORITY;

    TIMER_CLEAR (folder->impl->timer);

    g_object_ref (folder);

    if (!bit_now ||
        folder->impl->populate_func (folder->impl))
    {
        folder->impl->populate_idle_id =
                g_timeout_add_full (folder->impl->populate_priority,
                                    folder->impl->populate_timeout,
                                    folder->impl->populate_func,
                                    folder->impl, NULL);
    }

    g_object_unref (folder);
}


static void
stop_populate (MooFolderImpl *impl)
{
    if (impl->populate_idle_id)
        g_source_remove (impl->populate_idle_id);
    impl->populate_idle_id = 0;
    impl->populate_func = NULL;
    if (impl->dir)
        g_dir_close (impl->dir);
    impl->dir = NULL;
    files_list_free (&impl->files_copy);
}


static void
folder_emit_deleted (MooFolderImpl *impl)
{
    MooFolder *folder;

    /* moo_folder_deleted may call impl_free() */
    folder = impl->proxy;
    moo_folder_deleted (impl);

    if (folder)
        g_signal_emit (folder, signals[DELETED], 0);
}


static void
folder_emit_files (MooFolderImpl *impl,
                   guint          sig,
                   GSList        *files)
{
    if (files && impl->proxy)
        g_signal_emit (impl->proxy, signals[sig], 0, files);
}


GSList *
_moo_folder_list_files (MooFolder *folder)
{
    g_return_val_if_fail (MOO_IS_FOLDER (folder), NULL);
    g_return_val_if_fail (!folder->impl->deleted, NULL);
    return hash_table_to_file_list (folder->impl->files);
}


static double
get_names (MooFolderImpl *impl)
{
    GTimer *timer;
    GSList *added = NULL;
    const char *name;
    MooFile *file;
    double elapsed;

    g_assert (impl->path != NULL);
    g_assert (impl->dir != NULL);
    g_assert (g_hash_table_size (impl->files) == 0);

    timer = g_timer_new ();

    file = _moo_file_new (impl->path, "..");
    file->flags = MOO_FILE_HAS_MIME_TYPE | MOO_FILE_HAS_ICON;
    file->info = MOO_FILE_INFO_EXISTS | MOO_FILE_INFO_IS_DIR;
    file->icon = _moo_file_get_icon_type (file, impl->path);

    g_hash_table_insert (impl->files, g_strdup (".."), file);
    added = g_slist_prepend (added, file);

    for (name = g_dir_read_name (impl->dir);
         name != NULL;
         name = g_dir_read_name (impl->dir))
    {
        file = _moo_file_new (impl->path, name);

        if (file)
        {
            file->icon = _moo_file_icon_blank ();
            g_hash_table_insert (impl->files, g_strdup (name), file);
            added = g_slist_prepend (added, file);
            _moo_file_stat (file, impl->path);
        }
        else
        {
            g_critical ("_moo_file_new() failed for '%s'", name);
        }
    }

    folder_emit_files (impl, FILES_ADDED, added);
    g_slist_free (added);

    elapsed = impl->debug.names_timer = g_timer_elapsed (timer, NULL);
    g_timer_destroy (timer);

    g_dir_close (impl->dir);
    impl->dir = NULL;

    impl->done = STAGE_NAMES;

    PRINT_TIMES ("names folder %s: %f sec\n",
                 impl->path,
                 impl->debug.names_timer);

    return elapsed;
}


static gboolean
get_stat_a_bit (MooFolderImpl *impl)
{
    gboolean done = FALSE;
    double elapsed;

    g_assert (impl->dir == NULL);
    g_assert (impl->done == STAGE_NAMES);
    g_assert (impl->path != NULL);

    elapsed = g_timer_elapsed (impl->timer, NULL);
    g_timer_continue (impl->timer);

    if (!impl->files_copy)
        impl->files_copy = hash_table_to_file_list (impl->files);
    if (!impl->files_copy)
        done = TRUE;

    while (!done)
    {
        GSList *changed = impl->files_copy;
        MooFile *file = changed->data;
        impl->files_copy = g_slist_remove_link (impl->files_copy, impl->files_copy);

        if (!(file->flags & MOO_FILE_HAS_STAT))
        {
            _moo_file_stat (file, impl->path);
            folder_emit_files (impl, FILES_CHANGED, changed);
        }
        else
        {
            _moo_file_unref (file);
        }

        g_slist_free_1 (changed);

        if (!impl->files_copy)
            done = TRUE;

        if (g_timer_elapsed (impl->timer, NULL) > impl->populate_timeout)
            break;
    }

    elapsed = g_timer_elapsed (impl->timer, NULL) - elapsed;
    impl->debug.stat_timer += elapsed;
    impl->debug.stat_counter += 1;
    g_timer_stop (impl->timer);

    if (!done)
    {
        TIMER_CLEAR (impl->timer);
        return TRUE;
    }
    else
    {
        g_assert (impl->files_copy == NULL);
        impl->populate_idle_id = 0;

        PRINT_TIMES ("stat folder %s: %d iterations, %f sec\n",
                     impl->path,
                     impl->debug.stat_counter,
                     impl->debug.stat_timer);

        impl->done = STAGE_STAT;

        if (impl->wanted >= STAGE_MIME_TYPE || impl->wanted_bg >= STAGE_MIME_TYPE)
        {
            if (impl->wanted >= STAGE_MIME_TYPE)
            {
                impl->populate_priority = NORMAL_PRIORITY;
                impl->populate_timeout = NORMAL_TIMEOUT;
            }
            else if (impl->wanted_bg >= STAGE_MIME_TYPE)
            {
                impl->populate_priority = BACKGROUND_PRIORITY;
                impl->populate_timeout = BACKGROUND_TIMEOUT;
            }

            if (impl->populate_idle_id)
                g_source_remove (impl->populate_idle_id);
            impl->populate_idle_id = 0;
            impl->populate_func = (GSourceFunc) get_icons_a_bit;

            if (g_timer_elapsed (impl->timer, NULL) < impl->populate_timeout)
            {
                /* in this case we may block for as much as twice TIMEOUT, but usually
                   it allows stat and loading icons in one iteration */
                TIMER_CLEAR (impl->timer);
                if (impl->populate_func (impl))
                    impl->populate_idle_id =
                            g_timeout_add_full (impl->populate_priority,
                                                impl->populate_timeout,
                                                impl->populate_func,
                                                impl, NULL);
            }
            else
            {
                TIMER_CLEAR (impl->timer);
                impl->populate_idle_id =
                        g_timeout_add_full (impl->populate_priority,
                                            impl->populate_timeout,
                                            impl->populate_func,
                                            impl, NULL);
            }
        }
        else
        {
            impl->populate_func = NULL;
            impl->populate_priority = 0;
            impl->populate_timeout = 0;
        }

        return FALSE;
    }
}


static gboolean
get_icons_a_bit (MooFolderImpl *impl)
{
    gboolean done = FALSE;
    double elapsed;

    g_assert (impl->dir == NULL);
    g_assert (impl->done == STAGE_STAT);
    g_assert (impl->path != NULL);

    elapsed = g_timer_elapsed (impl->timer, NULL);
    g_timer_continue (impl->timer);

    if (!impl->files_copy)
        impl->files_copy = hash_table_to_file_list (impl->files);
    if (!impl->files_copy)
        done = TRUE;

    while (!done)
    {
        GSList *changed = impl->files_copy;
        MooFile *file = changed->data;

        impl->files_copy = g_slist_remove_link (impl->files_copy, changed);

#ifndef __WIN32__
        if (file->info & MOO_FILE_INFO_EXISTS &&
            !(file->flags & MOO_FILE_HAS_MIME_TYPE))
        {
            char *path = g_build_filename (impl->path, file->name, NULL);

            _moo_file_find_mime_type (file, path);

            file->flags |= MOO_FILE_HAS_ICON;
            file->icon = _moo_file_get_icon_type (file, impl->path);
            folder_emit_files (impl, FILES_CHANGED, changed);
            g_free (path);
        }
#endif

        _moo_file_free_statbuf (file);
        _moo_file_unref (file);
        g_slist_free (changed);

        if (!impl->files_copy)
            done = TRUE;

        if (g_timer_elapsed (impl->timer, NULL) > impl->populate_timeout)
            break;
    }

    elapsed = g_timer_elapsed (impl->timer, NULL) - elapsed;
    impl->debug.icons_timer += elapsed;
    impl->debug.icons_counter += 1;
    TIMER_CLEAR (impl->timer);

    if (done)
    {
        PRINT_TIMES ("icons folder %s: %d iterations, %f sec\n",
                     impl->path,
                     impl->debug.icons_counter,
                     impl->debug.icons_timer);

        g_assert (impl->files_copy == NULL);
        impl->populate_idle_id = 0;
        impl->done = STAGE_MIME_TYPE;
        impl->populate_func = NULL;
        impl->populate_priority = 0;
        impl->populate_timeout = 0;

        start_monitor (impl);
    }

    return !done;
}


#if 0
MooFile *
_moo_folder_get_file (MooFolder  *folder,
                      const char *basename)
{
    g_return_val_if_fail (MOO_IS_FOLDER (folder), NULL);
    g_return_val_if_fail (!folder->impl->deleted, NULL);
    g_return_val_if_fail (basename != NULL, NULL);
    return g_hash_table_lookup (folder->impl->files, basename);
}
#endif


char *
_moo_folder_get_file_path (MooFolder *folder,
                           MooFile   *file)
{
    g_return_val_if_fail (MOO_IS_FOLDER (folder), NULL);
    g_return_val_if_fail (file != NULL, NULL);
    return FILE_PATH (folder, file);
}


char *
_moo_folder_get_file_uri (MooFolder *folder,
                          MooFile   *file)
{
    char *path, *uri;

    g_return_val_if_fail (MOO_IS_FOLDER (folder), NULL);
    g_return_val_if_fail (file != NULL, NULL);

    path = FILE_PATH (folder, file);
    g_return_val_if_fail (path != NULL, NULL);

    uri = g_filename_to_uri (path, NULL, NULL);

    g_free (path);
    return uri;
}


MooFolder *
_moo_folder_get_parent (MooFolder      *folder,
                        MooFileFlags    wanted)
{
    g_return_val_if_fail (MOO_IS_FOLDER (folder), NULL);
    g_return_val_if_fail (!folder->impl->deleted, NULL);
    return _moo_file_system_get_parent_folder (folder->impl->fs,
                                               folder, wanted);
}


MooFileSystem *
_moo_folder_get_file_system (MooFolder *folder)
{
    g_return_val_if_fail (MOO_IS_FOLDER (folder), NULL);
    return folder->impl->fs;
}


/*****************************************************************************/
/* Monitoring
 */

static void file_deleted    (MooFolderImpl  *folder,
                             const char     *name);
static void file_changed    (MooFolderImpl  *folder,
                             const char     *name);
static void file_created    (MooFolderImpl  *folder,
                             const char     *name);

static void
fam_callback (MooFileWatch *watch,
              MooFileEvent *event,
              gpointer      data)
{
    MooFolderImpl *impl = data;

    g_return_if_fail (watch == impl->fam);
    g_return_if_fail (event->monitor_id == impl->fam_request);

    switch (event->code)
    {
        case MOO_FILE_EVENT_CHANGED:
            file_changed (impl, event->filename);
            break;
        case MOO_FILE_EVENT_CREATED:
            file_created (impl, event->filename);
            break;
        case MOO_FILE_EVENT_DELETED:
            file_deleted (impl, event->filename);
            break;
        case MOO_FILE_EVENT_ERROR:
            stop_monitor (impl);
            file_changed (impl, impl->path);
            break;
    }
}


static void
start_monitor (MooFolderImpl *impl)
{
    GError *error = NULL;

    g_return_if_fail (!impl->deleted);
    g_return_if_fail (impl->fam_request == 0);
    impl->fam = _moo_file_system_get_file_watch (impl->fs);
    g_return_if_fail (impl->fam != NULL);

    impl->fam_request =
        moo_file_watch_create_monitor (impl->fam, impl->path,
                                       fam_callback, impl,
                                       NULL, &error);

    if (!impl->fam_request)
    {
        g_warning ("moo_file_watch_create_monitor failed for path '%s': %s",
                   impl->path, moo_error_message (error));
        g_error_free (error);
        return;
    }
}


static void
stop_monitor (MooFolderImpl *impl)
{
    if (impl->fam_request)
    {
        moo_file_watch_cancel_monitor (impl->fam, impl->fam_request);
        impl->fam = NULL;
        impl->fam_request = 0;
    }
}


static void
file_deleted (MooFolderImpl *impl,
              const char    *name)
{
    MooFile *file;
    GSList *list;

    g_return_if_fail (!impl->deleted);

    if (!strcmp (name, impl->path))
    {
        folder_emit_deleted (impl);
        return;
    }

    file = g_hash_table_lookup (impl->files, name);
    if (!file) return;

    _moo_file_ref (file);
    g_hash_table_remove (impl->files, name);

    list = g_slist_append (NULL, file);
    folder_emit_files (impl, FILES_REMOVED, list);

    g_slist_free (list);
    _moo_file_unref (file);
}


void
_moo_folder_reload (MooFolder *folder)
{
    g_return_if_fail (MOO_IS_FOLDER (folder));

    if (folder->impl->reload_idle)
        g_source_remove (folder->impl->reload_idle);
    folder->impl->reload_idle = 0;

    moo_folder_do_reload (folder->impl);
}


static void
file_changed (MooFolderImpl *impl,
              const char    *name)
{
    g_return_if_fail (!impl->deleted);

    if (!strcmp (name, impl->path))
    {
        if (!impl->reload_idle)
            impl->reload_idle = g_idle_add ((GSourceFunc) moo_folder_do_reload, impl);
    }
}


static void
file_created (MooFolderImpl *impl,
              const char    *name)
{
    MooFile *file;
    GSList *list;

    g_return_if_fail (!impl->deleted);

    file = _moo_file_new (impl->path, name);
    g_return_if_fail (file != NULL);

    file->icon = _moo_file_get_icon_type (file, impl->path);
    _moo_file_stat (file, impl->path);

#ifndef __WIN32__
    if (file->info & MOO_FILE_INFO_EXISTS &&
        !(file->flags & MOO_FILE_HAS_MIME_TYPE))
    {
        char *path = g_build_filename (impl->path, file->name, NULL);

        _moo_file_find_mime_type (file, path);

        file->flags |= MOO_FILE_HAS_ICON;
        file->icon = _moo_file_get_icon_type (file, impl->path);
        g_free (path);
    }
#endif

    _moo_file_free_statbuf (file);

    g_hash_table_insert (impl->files, g_strdup (name), file);
    list = g_slist_append (NULL, file);
    folder_emit_files (impl, FILES_ADDED, list);

    g_slist_free (list);
}


void
_moo_folder_check_exists (MooFolder  *folder,
                          const char *name)
{
    MooFile *file;
    char *path;
    gboolean exists;

    g_return_if_fail (MOO_IS_FOLDER (folder));
    g_return_if_fail (name != NULL);

    file = g_hash_table_lookup (folder->impl->files, name);
    path = g_build_filename (folder->impl->path, name, NULL);
    exists = g_file_test (path, G_FILE_TEST_EXISTS);

    if (exists && !file)
        file_created (folder->impl, name);
    else if (!exists && file)
        file_deleted (folder->impl, name);

    g_free (path);
}


/* TODO */
static gboolean
moo_folder_do_reload (MooFolderImpl *impl)
{
    GHashTable *files;
    GDir *dir;
    GError *error = NULL;
    const char *name;
    GSList *new = NULL, *deleted = NULL, *l;

    g_return_val_if_fail (!impl->deleted, FALSE);
    impl->reload_idle = 0;

    dir = g_dir_open (impl->path, 0, &error);

    if (!dir)
    {
        g_warning ("could not open directory %s: %s", impl->path, moo_error_message (error));
        g_error_free (error);
        folder_emit_deleted (impl);
        return FALSE;
    }

    files = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    while ((name = g_dir_read_name (dir)))
        g_hash_table_insert (files, g_strdup (name), NULL);

    diff_hash_tables (files, impl->files, &new, &deleted);

    for (l = new; l != NULL; l = l->next)
        file_created (impl, l->data);

    for (l = deleted; l != NULL; l = l->next)
        file_deleted (impl, l->data);

    g_slist_foreach (new, (GFunc) g_free, NULL);
    g_slist_foreach (deleted, (GFunc) g_free, NULL);
    g_slist_free (new);
    g_slist_free (deleted);
    g_hash_table_destroy (files);
    g_dir_close (dir);
    return FALSE;
}


/* XXX */
static char *
moo_file_get_type_string (MooFile *file)
{
    g_return_val_if_fail (MOO_FILE_EXISTS (file), NULL);

    if (MOO_FILE_IS_DIR (file))
        return g_strdup ("folder");
    else if (file->mime_type)
        return g_strdup (file->mime_type);
    else
        return g_strdup ("file");
}


/* XXX */
static char *
get_size_string (MgwStatBuf *statbuf)
{
    g_return_val_if_fail (statbuf != NULL, NULL);
    return g_strdup_printf ("%" G_GUINT64_FORMAT, statbuf->size);
}


/* XXX */
static char *
moo_file_get_mtime_string (MooFile *file)
{
    static char buf[1024];

    if (!MOO_FILE_EXISTS (file))
        return NULL;

#ifdef __WIN32__
    if (MOO_FILE_IS_DIR (file))
        return NULL;
#endif

    g_return_val_if_fail (file->statbuf != NULL, NULL);

    if (strftime (buf, 1024, "%x %X", mgw_localtime (&file->statbuf->mtime)))
        return g_strdup (buf);
    else
        return NULL;
}


char **
_moo_folder_get_file_info (MooFolder      *folder,
                           MooFile        *file)
{
    GPtrArray *array;
    GSList *list;

    g_return_val_if_fail (MOO_IS_FOLDER (folder), NULL);
    g_return_val_if_fail (file != NULL, NULL);
    g_return_val_if_fail (!folder->impl->deleted, NULL);
    g_return_val_if_fail (folder->impl->files != NULL, NULL);
    g_return_val_if_fail (g_hash_table_lookup (folder->impl->files,
                                               _moo_file_name (file)) == file, NULL);

    _moo_file_stat (file, folder->impl->path);

    if (file->info & MOO_FILE_INFO_EXISTS &&
        !(file->flags & MOO_FILE_HAS_MIME_TYPE))
    {
        char *path = FILE_PATH (folder, file);

        _moo_file_find_mime_type (file, path);

        file->flags |= MOO_FILE_HAS_ICON;
        file->icon = _moo_file_get_icon_type (file, folder->impl->path);
        g_free (path);
    }

    array = g_ptr_array_new ();

    if (file->info & MOO_FILE_INFO_EXISTS)
    {
        char *type, *mtime, *location;

        g_ptr_array_add (array, g_strdup ("Type:"));
        type = moo_file_get_type_string (file);

        if (file->info & MOO_FILE_INFO_IS_LINK)
        {
            g_ptr_array_add (array, g_strdup_printf ("link to %s", type));
            g_free (type);
        }
        else
        {
            g_ptr_array_add (array, type);
        }

        location = g_filename_display_name (_moo_folder_get_path (folder));
        g_ptr_array_add (array, g_strdup ("Location:"));
        g_ptr_array_add (array, location);

        if (!(file->info & MOO_FILE_INFO_IS_DIR))
        {
            g_ptr_array_add (array, g_strdup ("Size:"));
            g_ptr_array_add (array, get_size_string (file->statbuf));
        }

        mtime = moo_file_get_mtime_string (file);

        if (mtime)
        {
            g_ptr_array_add (array, g_strdup ("Modified:"));
            g_ptr_array_add (array, mtime);
        }
    }
    else if (file->info & MOO_FILE_INFO_IS_LINK)
    {
        g_ptr_array_add (array, g_strdup ("Type:"));
        g_ptr_array_add (array, g_strdup ("broken symbolic link"));
    }

#ifndef __WIN32__
    if ((file->info & MOO_FILE_INFO_IS_LINK) &&
         _moo_file_link_get_target (file))
    {
        g_ptr_array_add (array, g_strdup ("Points to:"));
        g_ptr_array_add (array, g_strdup (_moo_file_link_get_target (file)));
    }
#endif

    list = g_slist_append (NULL, _moo_file_ref (file));
    g_object_ref (folder);
    folder_emit_files (folder->impl, FILES_CHANGED, list);
    g_object_unref (folder);
    _moo_file_unref (file);
    g_slist_free (list);

    g_ptr_array_add (array, NULL);
    return (char**) g_ptr_array_free (array, FALSE);
}


const char *
_moo_folder_get_path (MooFolder *folder)
{
    g_return_val_if_fail (MOO_IS_FOLDER (folder), NULL);
    return folder->impl->path;
}


static void
files_list_free (GSList **list)
{
    g_slist_foreach (*list, (GFunc) _moo_file_unref, NULL);
    g_slist_free (*list);
    *list = NULL;
}


static void
prepend_file (G_GNUC_UNUSED gpointer key,
              MooFile *file,
              GSList **list)
{
    *list = g_slist_prepend (*list, _moo_file_ref (file));
}

static GSList *
hash_table_to_file_list (GHashTable *files)
{
    GSList *list = NULL;
    g_return_val_if_fail (files != NULL, NULL);
    g_hash_table_foreach (files, (GHFunc) prepend_file, &list);
    return list;
}


static void
check_unique (const char *key,
              G_GNUC_UNUSED gpointer whatever,
              gpointer user_data)
{
    struct {
        GSList *list;
        GHashTable *table2;
    } *data = user_data;
    gpointer orig_key, value;

    if (!g_hash_table_lookup_extended (data->table2, key, &orig_key, &value))
        data->list = g_slist_prepend (data->list, g_strdup (key));
}

static void
get_unique (GHashTable *table1,
            GHashTable *table2,
            GSList    **only_1)
{
    struct {
        GSList *list;
        GHashTable *table2;
    } data;

    data.list = NULL;
    data.table2 = table2;
    g_hash_table_foreach (table1, (GHFunc) check_unique, &data);

    *only_1 = data.list;
}

static void
diff_hash_tables (GHashTable *table1,
                  GHashTable *table2,
                  GSList    **only_1,
                  GSList    **only_2)
{
    get_unique (table1, table2, only_1);
    get_unique (table2, table1, only_2);
}
