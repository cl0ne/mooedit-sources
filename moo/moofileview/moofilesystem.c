/*
 *   moofilesystem.c
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

#include <config.h>

#include "moofilesystem.h"
#include "moofolder-private.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils-mem.h"
#include "mooutils/mooutils.h"
#include "marshals.h"
#include <gio/gio.h>
#include <stdio.h>
#ifndef __WIN32__
#include <sys/wait.h>
#else
#include <windows.h>
#include <io.h>
#include <shellapi.h>
#endif

#if 0 && MOO_DEBUG
#define DEBUG_MESSAGE g_message
#else
static void G_GNUC_PRINTF(1,2) DEBUG_MESSAGE (G_GNUC_UNUSED const char *format, ...)
{
}
#endif

#define BROKEN_NAME "<" "????" ">"
#define FOLDERS_CACHE_SIZE 10

typedef struct {
    GQueue *queue;
    GHashTable *paths;
} FoldersCache;

struct _MooFileSystemPrivate {
    GHashTable *folders;
    MooFileWatch *fam;
    FoldersCache cache;
    guint debug_timeout;
};

static MooFileSystem *fs_instance = NULL;


static void         moo_file_system_dispose (GObject        *object);
static char        *_moo_file_system_normalize_path (MooFileSystem *fs,
                                             const char     *path,
                                             gboolean        is_folder,
                                             GError        **error);

static MooFolder   *get_folder              (MooFileSystem  *fs,
                                             const char     *path,
                                             MooFileFlags    wanted,
                                             GError        **error);
static gboolean     create_folder           (MooFileSystem  *fs,
                                             const char     *path,
                                             GError        **error);
static MooFolder   *get_parent_folder       (MooFileSystem  *fs,
                                             MooFolder      *folder,
                                             MooFileFlags    flags);
static gboolean     delete_file             (MooFileSystem  *fs,
                                             const char     *path,
                                             MooDeleteFileFlags flags,
                                             GError        **error);

static gboolean     move_file_unix          (MooFileSystem  *fs,
                                             const char     *old_path,
                                             const char     *new_path,
                                             GError        **error);

#ifndef __WIN32__
static MooFolder   *get_root_folder_unix    (MooFileSystem  *fs,
                                             MooFileFlags    wanted);
static char        *normalize_path_unix     (MooFileSystem  *fs,
                                             const char     *path,
                                             gboolean        is_folder,
                                             GError        **error);
static char        *make_path_unix          (MooFileSystem  *fs,
                                             const char     *base_path,
                                             const char     *display_name,
                                             GError        **error);
static gboolean     parse_path_unix         (MooFileSystem  *fs,
                                             const char     *path_utf8,
                                             char          **dirname,
                                             char          **display_dirname,
                                             char          **display_basename,
                                             GError        **error);
static char        *get_absolute_path_unix  (MooFileSystem  *fs,
                                             const char     *display_name,
                                             const char     *current_dir);

#else /* __WIN32__ */

static MooFolder   *get_root_folder_win32   (MooFileSystem  *fs,
                                             MooFileFlags    wanted);
#if 0
static gboolean     move_file_win32         (MooFileSystem  *fs,
                                             const char     *old_path,
                                             const char     *new_path,
                                             GError        **error);
#endif
static char        *normalize_path_win32    (MooFileSystem  *fs,
                                             const char     *path,
                                             gboolean        is_folder,
                                             GError        **error);
static char        *make_path_win32         (MooFileSystem  *fs,
                                             const char     *base_path,
                                             const char     *display_name,
                                             GError        **error);
static gboolean     parse_path_win32        (MooFileSystem  *fs,
                                             const char     *path_utf8,
                                             char          **dirname,
                                             char          **display_dirname,
                                             char          **display_basename,
                                             GError        **error);
static char        *get_absolute_path_win32 (MooFileSystem  *fs,
                                             const char     *display_name,
                                             const char     *current_dir);
#endif /* __WIN32__ */


/* MOO_TYPE_FILE_SYSTEM */
G_DEFINE_TYPE (MooFileSystem, _moo_file_system, G_TYPE_OBJECT)


static void
_moo_file_system_class_init (MooFileSystemClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = moo_file_system_dispose;

    klass->get_folder = get_folder;
    klass->create_folder = create_folder;
    klass->get_parent_folder = get_parent_folder;
    klass->delete_file = delete_file;

#ifdef __WIN32__
    klass->get_root_folder = get_root_folder_win32;
    klass->move_file = move_file_unix;
    klass->normalize_path = normalize_path_win32;
    klass->make_path = make_path_win32;
    klass->parse_path = parse_path_win32;
    klass->get_absolute_path = get_absolute_path_win32;
#else /* !__WIN32__ */
    klass->get_root_folder = get_root_folder_unix;
    klass->move_file = move_file_unix;
    klass->normalize_path = normalize_path_unix;
    klass->make_path = make_path_unix;
    klass->parse_path = parse_path_unix;
    klass->get_absolute_path = get_absolute_path_unix;
#endif /* !__WIN32__ */
}


static void
add_folder_cache (MooFileSystem *fs,
                  MooFolderImpl *impl)
{
    FoldersCache *cache = &fs->priv->cache;

    DEBUG_MESSAGE ("%s: adding folder %s to cache", G_STRFUNC, impl->path);
    g_queue_push_head (cache->queue, impl);
    g_hash_table_insert (cache->paths, impl->path, impl);

    if (cache->queue->length > FOLDERS_CACHE_SIZE)
    {
        MooFolderImpl *old = g_queue_pop_tail (cache->queue);
        g_hash_table_remove (cache->paths, old->path);
        DEBUG_MESSAGE ("%s: removing folder %s from cache", G_STRFUNC, old->path);
        _moo_folder_impl_free (old);
    }
}


void
_moo_file_system_folder_finalized (MooFileSystem *fs,
                                   MooFolder     *folder)
{
    MooFolderImpl *impl;

    g_return_if_fail (MOO_IS_FILE_SYSTEM (fs));
    g_return_if_fail (MOO_IS_FOLDER (folder));

    impl = folder->impl;
    folder->impl = NULL;
    impl->proxy = NULL;

    g_hash_table_remove (fs->priv->folders, impl->path);

    if (!impl->deleted)
    {
        add_folder_cache (fs, impl);
    }
    else
    {
        DEBUG_MESSAGE ("%s: folder %s deleted, freeing", G_STRFUNC, impl->path);
        _moo_folder_impl_free (impl);
    }
}


static void
calc_mem_hash_cb (G_GNUC_UNUSED const char *name,
                  MooFolder *folder,
                  gsize *mem)
{
    mem[0] += _moo_folder_mem_usage (folder);
    mem[1] += g_hash_table_size (folder->impl->files);
}

static gboolean
debug_timeout (MooFileSystem *fs)
{
    gsize mem[2] = {0, 0};
    g_hash_table_foreach (fs->priv->folders, (GHFunc) calc_mem_hash_cb, mem);
    g_print ("%" G_GSIZE_FORMAT " bytes in %" G_GSIZE_FORMAT " files\n", mem[0], mem[1]);
    return TRUE;
}


static void
_moo_file_system_init (MooFileSystem *fs)
{
    fs->priv = g_new0 (MooFileSystemPrivate, 1);
    fs->priv->folders = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    fs->priv->cache.queue = g_queue_new ();
    fs->priv->cache.paths = g_hash_table_new (g_str_hash, g_str_equal);

    if (0)
        fs->priv->debug_timeout = g_timeout_add (5000, (GSourceFunc) debug_timeout, fs);
}


static void
moo_file_system_dispose (GObject *object)
{
    MooFileSystem *fs = MOO_FILE_SYSTEM (object);

    if (fs->priv)
    {
        g_hash_table_destroy (fs->priv->folders);
        g_hash_table_destroy (fs->priv->cache.paths);
        g_queue_foreach (fs->priv->cache.queue, (GFunc) _moo_folder_impl_free, NULL);
        g_queue_free (fs->priv->cache.queue);

        if (fs->priv->fam)
        {
            moo_file_watch_close (fs->priv->fam, NULL);
            moo_file_watch_unref (fs->priv->fam);
        }

        if (fs->priv->debug_timeout)
            g_source_remove (fs->priv->debug_timeout);

        g_free (fs->priv);
        fs->priv = NULL;
    }

    G_OBJECT_CLASS (_moo_file_system_parent_class)->finalize (object);
}


MooFileSystem *
_moo_file_system_create (void)
{
    if (!fs_instance)
    {
        fs_instance = MOO_FILE_SYSTEM (g_object_new (MOO_TYPE_FILE_SYSTEM, (const char*) NULL));
        g_object_weak_ref (G_OBJECT (fs_instance),
                           (GWeakNotify) g_nullify_pointer, &fs_instance);
        return fs_instance;
    }
    else
    {
        return g_object_ref (fs_instance);
    }
}


MooFolder *
_moo_file_system_get_root_folder (MooFileSystem  *fs,
                                  MooFileFlags    wanted)
{
    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), NULL);
    return MOO_FILE_SYSTEM_GET_CLASS(fs)->get_root_folder (fs, wanted);
}


MooFolder *
_moo_file_system_get_folder (MooFileSystem  *fs,
                             const char     *path,
                             MooFileFlags    wanted,
                             GError        **error)
{
    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), NULL);
    return MOO_FILE_SYSTEM_GET_CLASS(fs)->get_folder (fs, path, wanted, error);
}


MooFolder *
_moo_file_system_get_parent_folder (MooFileSystem  *fs,
                                    MooFolder      *folder,
                                    MooFileFlags    wanted)
{
    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), NULL);
    g_return_val_if_fail (MOO_IS_FOLDER (folder), NULL);
    return MOO_FILE_SYSTEM_GET_CLASS(fs)->get_parent_folder (fs, folder, wanted);
}


gboolean
_moo_file_system_create_folder (MooFileSystem  *fs,
                                const char     *path,
                                GError        **error)
{
    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), FALSE);
    g_return_val_if_fail (path != NULL, FALSE);
    return MOO_FILE_SYSTEM_GET_CLASS(fs)->create_folder (fs, path, error);
}


gboolean
_moo_file_system_delete_file (MooFileSystem       *fs,
                              const char          *path,
                              MooDeleteFileFlags   flags,
                              GError             **error)
{
    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), FALSE);
    g_return_val_if_fail (path != NULL, FALSE);
    return MOO_FILE_SYSTEM_GET_CLASS(fs)->delete_file (fs, path, flags, error);
}


gboolean
_moo_file_system_move_file (MooFileSystem  *fs,
                            const char     *old_path,
                            const char     *new_path,
                            GError        **error)
{
    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), FALSE);
    g_return_val_if_fail (old_path && new_path, FALSE);
    return MOO_FILE_SYSTEM_GET_CLASS(fs)->move_file (fs, old_path, new_path, error);
}


char *
_moo_file_system_make_path (MooFileSystem  *fs,
                            const char     *base_path,
                            const char     *display_name,
                            GError        **error)
{
    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), NULL);
    g_return_val_if_fail (base_path != NULL && display_name != NULL, NULL);
    return MOO_FILE_SYSTEM_GET_CLASS(fs)->make_path (fs, base_path, display_name, error);
}


static char *
_moo_file_system_normalize_path (MooFileSystem  *fs,
                                 const char     *path,
                                 gboolean        is_folder,
                                 GError        **error)
{
    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), NULL);
    g_return_val_if_fail (path != NULL, NULL);
    return MOO_FILE_SYSTEM_GET_CLASS(fs)->normalize_path (fs, path, is_folder, error);
}


gboolean
_moo_file_system_parse_path (MooFileSystem  *fs,
                             const char     *path_utf8,
                             char          **dirname,
                             char          **display_dirname,
                             char          **display_basename,
                             GError        **error)
{
    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), FALSE);
    g_return_val_if_fail (path_utf8 != NULL, FALSE);
    return MOO_FILE_SYSTEM_GET_CLASS(fs)->parse_path (fs, path_utf8, dirname,
                                                      display_dirname, display_basename,
                                                      error);
}


char *
_moo_file_system_get_absolute_path (MooFileSystem  *fs,
                                    const char     *display_name,
                                    const char     *current_dir)
{
    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), NULL);
    g_return_val_if_fail (display_name != NULL, NULL);
    return MOO_FILE_SYSTEM_GET_CLASS(fs)->get_absolute_path (fs, display_name, current_dir);
}


MooFileWatch *
_moo_file_system_get_file_watch (MooFileSystem *fs)
{
    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), NULL);

    if (!fs->priv->fam)
    {
        GError *error = NULL;
        fs->priv->fam = moo_file_watch_new (&error);
        if (!fs->priv->fam)
        {
            g_warning ("moo_fam_open failed: %s", moo_error_message (error));
            g_error_free (error);
        }
    }

    return fs->priv->fam;
}


/* TODO what's this? */
void
_moo_file_system_folder_deleted (MooFileSystem *fs,
                                 MooFolderImpl *impl)
{
    if (impl->proxy)
    {
        g_hash_table_remove (fs->priv->folders, impl->path);
        DEBUG_MESSAGE ("%s: folder %s deleted, proxy alive", G_STRFUNC, impl->path);
    }
    else
    {
        DEBUG_MESSAGE ("%s: cached folder %s deleted, freeing", G_STRFUNC, impl->path);
        g_hash_table_remove (fs->priv->cache.paths, impl->path);
        g_queue_remove (fs->priv->cache.queue, impl);
        _moo_folder_impl_free (impl);
    }
}


static MooFolder *
get_folder (MooFileSystem  *fs,
            const char     *path,
            MooFileFlags    wanted,
            GError        **error)
{
    MooFolder *folder;
    MooFolderImpl *impl;
    char *norm_path = NULL;

    g_return_val_if_fail (path != NULL, NULL);

#ifdef __WIN32__
    if (!*path)
        return get_root_folder_win32 (fs, wanted);
#endif /* __WIN32__ */

    /* XXX check the caller */
    if (!_moo_path_is_absolute (path))
    {
        g_set_error (error, MOO_FILE_ERROR,
                     MOO_FILE_ERROR_BAD_FILENAME,
                     "folder path '%s' is not absolute",
                     path);
        return NULL;
    }

    norm_path = _moo_file_system_normalize_path (fs, path, TRUE, error);

    if (!norm_path)
        return NULL;

    folder = g_hash_table_lookup (fs->priv->folders, norm_path);

    if (folder)
    {
        _moo_folder_set_wanted (folder, wanted, TRUE);
        g_object_ref (folder);
        goto out;
    }

    impl = g_hash_table_lookup (fs->priv->cache.paths, norm_path);

    if (impl)
    {
        g_hash_table_remove (fs->priv->cache.paths, impl->path);
        g_queue_remove (fs->priv->cache.queue, impl);
        folder = _moo_folder_new_with_impl (impl);
        g_hash_table_insert (fs->priv->folders, norm_path, folder);
        norm_path = NULL;
        _moo_folder_set_wanted (folder, wanted, TRUE);
        goto out;
    }

    if (!g_file_test (norm_path, G_FILE_TEST_EXISTS))
    {
        g_set_error (error, MOO_FILE_ERROR,
                     MOO_FILE_ERROR_NONEXISTENT,
                     "'%s' does not exist", norm_path);
        folder = NULL;
        goto out;
    }

    if (!g_file_test (norm_path, G_FILE_TEST_IS_DIR))
    {
        g_set_error (error, MOO_FILE_ERROR,
                     MOO_FILE_ERROR_NOT_FOLDER,
                     "'%s' is not a folder", norm_path);
        folder = NULL;
        goto out;
    }

    folder = _moo_folder_new (fs, norm_path, wanted, error);

    if (folder)
    {
        g_hash_table_insert (fs->priv->folders, norm_path, folder);
        norm_path = NULL;
    }

out:
    g_free (norm_path);
    return folder;
}


/* TODO */
static gboolean
create_folder (G_GNUC_UNUSED MooFileSystem *fs,
               const char     *path,
               GError        **error)
{
    mgw_errno_t err;

    g_return_val_if_fail (path != NULL, FALSE);

    /* XXX check the caller */
    if (!_moo_path_is_absolute (path))
    {
        g_set_error (error, MOO_FILE_ERROR,
                     MOO_FILE_ERROR_BAD_FILENAME,
                     "folder path '%s' is not absolute",
                     path);
        return FALSE;
    }

    /* TODO mkdir must (?) adjust permissions according to umask */
#ifndef __WIN32__
    if (mgw_mkdir (path, S_IRWXU | S_IRWXG | S_IRWXO, &err))
#else
    if (_moo_mkdir (path, &err))
#endif
    {
        g_set_error (error, MOO_FILE_ERROR,
                     _moo_file_error_from_errno (err),
                     "%s", mgw_strerror (err));
        return FALSE;
    }

    return TRUE;
}


/***************************************************************************/
/* common methods
 */

/* folder may be deleted, but this function returns parent
   folder anyway, if that exists */
static MooFolder *
get_parent_folder (MooFileSystem  *fs,
                   MooFolder      *folder,
                   MooFileFlags    wanted)
{
    char *parent_path;
    MooFolder *parent;

    g_return_val_if_fail (MOO_IS_FILE_SYSTEM (fs), NULL);
    g_return_val_if_fail (MOO_IS_FOLDER (folder), NULL);
    g_return_val_if_fail (_moo_folder_get_file_system (folder) == fs, NULL);

    parent_path = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "..",
                                   _moo_folder_get_path (folder));

    parent = _moo_file_system_get_folder (fs, parent_path, wanted, NULL);

    g_free (parent_path);
    return parent;
}


static char *
normalize_path (const char *path)
{
    GPtrArray *comps;
    gboolean first_slash;
    char **pieces, **p;
    char *normpath;

    g_return_val_if_fail (path != NULL, NULL);

    first_slash = (path[0] == G_DIR_SEPARATOR);

    pieces = g_strsplit (path, G_DIR_SEPARATOR_S, 0);
    g_return_val_if_fail (pieces != NULL, NULL);

    comps = g_ptr_array_new ();

    for (p = pieces; *p != NULL; ++p)
    {
        char *s = *p;
        gboolean push = TRUE;
        gboolean pop = FALSE;

        if (!strcmp (s, "") || !strcmp (s, "."))
        {
            push = FALSE;
        }
        else if (!strcmp (s, ".."))
        {
            if (!comps->len && first_slash)
            {
                push = FALSE;
            }
            else if (comps->len)
            {
                push = FALSE;
                pop = TRUE;
            }
        }

        if (pop)
        {
            g_free (comps->pdata[comps->len - 1]);
            g_ptr_array_remove_index (comps, comps->len - 1);
        }

        if (push)
            g_ptr_array_add (comps, g_strdup (s));
    }

    g_ptr_array_add (comps, NULL);

    if (comps->len == 1)
    {
        if (first_slash)
            normpath = g_strdup (G_DIR_SEPARATOR_S);
        else
            normpath = g_strdup (".");
    }
    else
    {
        normpath = g_strjoinv (G_DIR_SEPARATOR_S, (char**) comps->pdata);

        if (first_slash)
        {
            gsize len = strlen (normpath);
            normpath = g_renew (char, normpath, len + 2);
            memmove (normpath + 1, normpath, len + 1);
            normpath[0] = G_DIR_SEPARATOR;
        }
    }

    g_strfreev (pieces);
    g_strfreev ((char**) comps->pdata);
    g_ptr_array_free (comps, FALSE);

    return normpath;
}


#if defined(__WIN32__) && 0

#ifndef FOF_NORECURSION
#define FOF_NORECURSION 0x1000
#endif

static gboolean
delete_file_win32 (const char     *path,
                   gboolean        recursive,
                   GError        **error)
{
    gboolean success;
    SHFILEOPSTRUCT op = {0};
    gunichar2 *path_utf16;
    long len;

    if (!(path_utf16 = g_utf8_to_utf16 (path, -1, NULL, &len, error)))
        return FALSE;

    path_utf16 = g_renew (gunichar2, path_utf16, len + 2);
    path_utf16[len + 1] = 0;

    op.wFunc = FO_DELETE;
    op.pFrom = path_utf16;
    op.fFlags = FOF_ALLOWUNDO;
    if (!recursive)
        op.fFlags |= FOF_NORECURSION;

    success = SHFileOperation (&op) == 0;

    if (!success)
        g_set_error (error, MOO_FILE_ERROR,
                     MOO_FILE_ERROR_FAILED,
                     "Could not delete '%s'",
                     path);

    g_free (path_utf16);
    return success;
}

#endif

static gboolean
delete_file (G_GNUC_UNUSED MooFileSystem *fs,
             const char          *path,
             MooDeleteFileFlags   flags,
             GError             **error)
{
    gboolean isdir;
    mgw_errno_t err;

    g_return_val_if_fail (path != NULL, FALSE);
    g_return_val_if_fail (_moo_path_is_absolute (path), FALSE);
    g_return_val_if_fail (!error || !*error, FALSE);

    if (flags & MOO_DELETE_TO_TRASH)
    {
        GFile *file = g_file_new_for_path (path);
        gboolean retval = g_file_trash (file, NULL, error);
        g_object_unref (file);
        return retval;
    }

#if defined(__WIN32__) && 0
    return delete_file_win32 (path, (flags & MOO_DELETE_RECURSIVE) != 0, error);
#endif

    if (g_file_test (path, G_FILE_TEST_IS_SYMLINK))
        isdir = FALSE;
    else
        isdir = g_file_test (path, G_FILE_TEST_IS_DIR);

    if (isdir)
        return _moo_remove_dir (path, (flags & MOO_DELETE_RECURSIVE) != 0, error);

    if (mgw_remove (path, &err) != 0)
    {
        char *path_utf8 = g_filename_display_name (path);
        g_set_error (error, MOO_FILE_ERROR,
                     _moo_file_error_from_errno (err),
                     "Could not delete file '%s': %s",
                     path_utf8 ? path_utf8 : BROKEN_NAME,
                     mgw_strerror (err));
        g_free (path_utf8);
        return FALSE;
    }

    return TRUE;
}


/***************************************************************************/
/* UNIX methods
 */
static gboolean
move_file_unix (G_GNUC_UNUSED MooFileSystem *fs,
                const char     *old_path,
                const char     *new_path,
                GError        **error)
{
    mgw_errno_t err;

    g_return_val_if_fail (old_path && new_path, FALSE);
    g_return_val_if_fail (_moo_path_is_absolute (old_path), FALSE);
    g_return_val_if_fail (_moo_path_is_absolute (new_path), FALSE);

    /* XXX */
    if (mgw_rename (old_path, new_path, &err))
    {
        g_set_error (error, MOO_FILE_ERROR,
                     _moo_file_error_from_errno (err),
                     "%s", mgw_strerror (err));
        return FALSE;
    }

    return TRUE;
}


#ifndef __WIN32__

static MooFolder *
get_root_folder_unix (MooFileSystem  *fs,
                      MooFileFlags    wanted)
{
    return _moo_file_system_get_folder (fs, "/", wanted, NULL);
}


static char *
make_path_unix (G_GNUC_UNUSED MooFileSystem *fs,
                const char     *base_path,
                const char     *display_name,
                GError        **error)
{
    GError *error_here = NULL;
    char *path, *name;

    g_return_val_if_fail (base_path != NULL, NULL);
    g_return_val_if_fail (display_name != NULL, NULL);

    /* XXX check the caller */
    if (!_moo_path_is_absolute (base_path))
    {
        g_set_error (error, MOO_FILE_ERROR,
                     MOO_FILE_ERROR_BAD_FILENAME,
                     "path '%s' is not absolute",
                     base_path);
        return NULL;
    }

    name = g_filename_from_utf8 (display_name, -1, NULL, NULL, &error_here);

    if (error_here)
    {
        g_set_error (error, MOO_FILE_ERROR,
                     MOO_FILE_ERROR_BAD_FILENAME,
                     "Could not convert '%s' to filename encoding: %s",
                     display_name, error_here->message);
        g_free (name);
        g_error_free (error_here);
        return NULL;
    }

    path = g_strdup_printf ("%s/%s", base_path, name);
    g_free (name);
    return path;
}


/* XXX make sure error is set TODO: error checking, etc. */
static char *
normalize_path_unix (G_GNUC_UNUSED MooFileSystem *fs,
                     const char     *path,
                     gboolean        is_folder,
                     G_GNUC_UNUSED GError **error)
{
    guint len;
    char *normpath;

    g_return_val_if_fail (path != NULL, NULL);

    normpath = normalize_path (path);

    if (!is_folder)
        return normpath;

    len = strlen (normpath);
    g_return_val_if_fail (len > 0, normpath);

    if (normpath[len-1] != G_DIR_SEPARATOR)
    {
        normpath = g_renew (char, normpath, len + 2);
        normpath[len] = G_DIR_SEPARATOR;
        normpath[len+1] = 0;
    }

#if 0
    g_print ("path: '%s'\nnormpath: '%s'\n",
             path, normpath);
#endif

    return normpath;
}


/* XXX must set error */
static gboolean
parse_path_unix (MooFileSystem  *fs,
                 const char     *path_utf8,
                 char          **dirname_p,
                 char          **display_dirname_p,
                 char          **display_basename_p,
                 GError        **error)
{
    const char *separator;
    char *dirname = NULL, *norm_dirname = NULL;
    char *display_dirname = NULL, *display_basename = NULL;

    g_return_val_if_fail (path_utf8 && path_utf8[0], FALSE);

    /* XXX check the caller */
    if (!_moo_path_is_absolute (path_utf8))
    {
        g_set_error (error, MOO_FILE_ERROR,
                     MOO_FILE_ERROR_BAD_FILENAME,
                     "path '%s' is not absolute",
                     path_utf8);
        return FALSE;
    }

    if (!strcmp (path_utf8, "/"))
    {
        display_dirname = g_strdup ("/");
        display_basename = g_strdup ("");
        norm_dirname = g_strdup ("/");
        goto success;
    }

    separator = strrchr (path_utf8, '/');
    g_return_val_if_fail (separator != NULL, FALSE);

    display_dirname = g_strndup (path_utf8, separator - path_utf8 + 1);
    display_basename = g_strdup (separator + 1);
    dirname = g_filename_from_utf8 (display_dirname, -1, NULL, NULL, error);

    if (!dirname)
        goto error;

    norm_dirname = _moo_file_system_normalize_path (fs, dirname, TRUE, error);

    if (!norm_dirname)
        goto error;
    else
        goto success;

    /* no fallthrough */
    g_assert_not_reached ();

error:
    g_free (dirname);
    g_free (norm_dirname);
    g_free (display_dirname);
    g_free (display_basename);
    return FALSE;

success:
    g_clear_error (error);
    g_free (dirname);
    *dirname_p = norm_dirname;
    *display_dirname_p = display_dirname;
    *display_basename_p = display_basename;
    return TRUE;
}


/* XXX unicode */
static char *
get_absolute_path_unix (G_GNUC_UNUSED MooFileSystem *fs,
                        const char     *short_name,
                        const char     *current_dir)
{
    g_return_val_if_fail (short_name && short_name[0], NULL);

    if (short_name[0] == '~')
    {
        const char *home = g_get_home_dir ();
        g_return_val_if_fail (home != NULL, NULL);

        if (short_name[1])
            return g_build_filename (home, short_name + 1, NULL);
        else
            return g_strdup (home);
    }

    if (_moo_path_is_absolute (short_name))
        return g_strdup (short_name);

    if (current_dir)
        return g_build_filename (current_dir, short_name, NULL);

    return NULL;
}

#endif /* !__WIN32__ */


/***************************************************************************/
/* Win32 methods
 */
#ifdef __WIN32__

static MooFolder *
get_root_folder_win32 (MooFileSystem  *fs,
                       MooFileFlags    wanted)
{
    MOO_IMPLEMENT_ME
    return _moo_file_system_get_folder (fs, "c:\\", wanted, NULL);
}


#if 0
static gboolean
move_file_win32 (G_GNUC_UNUSED MooFileSystem  *fs,
                 G_GNUC_UNUSED const char *old_path,
                 G_GNUC_UNUSED const char *new_path,
                 GError        **error)
{
    MOO_IMPLEMENT_ME
    g_set_error (error, MOO_FILE_ERROR,
                 MOO_FILE_ERROR_NOT_IMPLEMENTED,
                 "Renaming files is not implemented on win32");
    return FALSE;
}
#endif


static void
splitdrive (const char *fullpath,
            char      **drive,
            char      **path)
{
    if (fullpath[0] && fullpath[1] == ':')
    {
        *drive = g_strndup (fullpath, 2);
        *path = g_strdup (fullpath + 2);
    }
    else
    {
        *drive = NULL;
        *path = g_strdup (fullpath);
    }
}


static char *
normalize_path_win32 (G_GNUC_UNUSED MooFileSystem *fs,
                      const char     *fullpath,
                      gboolean        isdir,
                      G_GNUC_UNUSED GError **error)
{
    char *drive, *path, *normpath;
    guint slashes;
    gboolean drive_slashes = FALSE;

    g_return_val_if_fail (fullpath != NULL, NULL);

    splitdrive (fullpath, &drive, &path);
    g_strdelimit (path, "/", '\\');

    for (slashes = 0; path[slashes] == '\\'; ++slashes) ;

    if (drive && path[0] != '\\')
    {
        char *tmp = path;
        path = g_strdup_printf ("\\%s", path);
        g_free (tmp);
    }

    if (!drive)
    {
        char *tmp = path;
        drive = g_strndup (path, slashes);
        path = g_strdup (tmp + slashes);
        g_free (tmp);
        drive_slashes = TRUE;
    }
//     else if (path[0] == '\\')
//     {
//         char *tmp;
//
//         tmp = drive;
//         drive = g_strdup_printf ("%s\\", drive);
//         g_free (tmp);
//
//         tmp = path;
//         path = g_strdup (path + slashes);
//         g_free (tmp);
//     }

    normpath = normalize_path (path);

    if (!normpath[0] && !drive)
    {
        char *tmp = normpath;
        normpath = g_strdup (".");
        g_free (tmp);
    }
    else if (drive)
    {
        char *tmp = normpath;

        if (normpath[0] == '\\' || drive_slashes)
            normpath = g_strdup_printf ("%s%s", drive, normpath);
        else
            normpath = g_strdup_printf ("%s\\%s", drive, normpath);

        g_free (tmp);
    }

    if (isdir)
    {
        gsize len = strlen (normpath);

        if (!len || normpath[len -1] != '\\')
        {
            char *tmp = normpath;
            normpath = g_strdup_printf ("%s\\", normpath);
            g_free (tmp);
        }
    }

    g_free (drive);
    g_free (path);
    return normpath;
}


static char *
make_path_win32 (G_GNUC_UNUSED MooFileSystem *fs,
                 const char     *base_path,
                 const char     *display_name,
                 G_GNUC_UNUSED GError **error)
{
    g_return_val_if_fail (_moo_path_is_absolute (base_path), NULL);
    g_return_val_if_fail (display_name != NULL, NULL);
    return g_strdup_printf ("%s\\%s", base_path, display_name);
}


static gboolean
parse_path_win32 (MooFileSystem  *fs,
                  const char     *path_utf8,
                  char          **dirname_p,
                  char          **display_dirname_p,
                  char          **display_basename_p,
                  GError        **error)
{
    MOO_IMPLEMENT_ME
    const char *separator;
    char *norm_dirname = NULL, *dirname = NULL, *basename = NULL;
    gsize len;

    g_return_val_if_fail (path_utf8 && path_utf8[0], FALSE);
    g_return_val_if_fail (_moo_path_is_absolute (path_utf8), FALSE);

    separator = strrchr (path_utf8, '\\');
    g_return_val_if_fail (separator != NULL, FALSE);

    len = strlen (path_utf8);

    if (path_utf8[len - 1] == '\\')
    {
        dirname = g_strdup (path_utf8);
        basename = g_strdup ("");
    }
    else
    {
        dirname = g_path_get_dirname (path_utf8);
        basename = g_path_get_basename (path_utf8);
    }

    norm_dirname = _moo_file_system_normalize_path (fs, dirname, TRUE, error);

    if (!norm_dirname)
        goto error;

    *dirname_p = norm_dirname;
    *display_dirname_p = dirname;
    *display_basename_p = basename;

    g_message ("parsed '%s' into '%s' and '%s'", path_utf8, dirname, basename);

    return TRUE;

error:
    g_free (dirname);
    g_free (basename);
    g_free (norm_dirname);

    g_message ("could not parse '%s'", path_utf8);

    return FALSE;
}


static char *
get_absolute_path_win32 (G_GNUC_UNUSED MooFileSystem *fs,
                         const char     *short_name,
                         const char     *current_dir)
{
    g_return_val_if_fail (short_name && short_name[0], NULL);

    if (_moo_path_is_absolute (short_name))
        return g_strdup (short_name);

    if (current_dir)
        return g_build_filename (current_dir, short_name, NULL);

    return NULL;
}

#endif /* __WIN32__ */
