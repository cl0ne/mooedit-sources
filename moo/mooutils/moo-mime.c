/*
 *   moo-mime.c
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

#include "mooutils/moo-mime.h"
#include "xdgmime/xdgmime.h"
#include "mooutils/mooutils-fs.h"
#include <mooglib/moo-stat.h>
#include <string.h>

G_LOCK_DEFINE (moo_mime);

const char *
moo_mime_type_unknown (void)
{
    return XDG_MIME_TYPE_UNKNOWN;
}

static const char *
mime_type_intern (const char *mime)
{
    static GHashTable *hash;
    const char *interned;

    if (mime == XDG_MIME_TYPE_UNKNOWN || mime == NULL)
        return XDG_MIME_TYPE_UNKNOWN;

    if (G_UNLIKELY (!hash))
        hash = g_hash_table_new (g_str_hash, g_str_equal);

    if (G_UNLIKELY (!(interned = (const char*) g_hash_table_lookup (hash, mime))))
    {
        char *copy = g_strdup (mime);
        g_hash_table_insert (hash, copy, copy);
        interned = copy;
    }

    return interned;
}

const char *
moo_get_mime_type_for_file (const char *filename,
                            MgwStatBuf *statbuf)
{
    const char *mime;
    char *filename_utf8 = NULL;
    gboolean is_regular = statbuf ? statbuf->isreg : FALSE;

    if (filename)
        filename_utf8 = g_filename_display_name (filename);

    G_LOCK (moo_mime);
    mime = mime_type_intern (xdg_mime_get_mime_type_for_file (filename_utf8, statbuf ? &is_regular : NULL));
    G_UNLOCK (moo_mime);

    g_free (filename_utf8);
    return mime;
}

const char *
moo_get_mime_type_for_filename (const char *filename)
{
    const char *mime;
    char *filename_utf8 = NULL;

    if (filename)
        filename_utf8 = g_filename_display_name (filename);

    G_LOCK (moo_mime);
    mime = mime_type_intern (xdg_mime_get_mime_type_from_file_name (filename_utf8));
    G_UNLOCK (moo_mime);

    g_free (filename_utf8);
    return mime;
}

gboolean
moo_mime_type_is_subclass (const char *mime_type,
                           const char *base)
{
    gboolean ret;
    G_LOCK (moo_mime);
    ret = xdg_mime_mime_type_subclass (mime_type, base);
    G_UNLOCK (moo_mime);
    return ret;
}

const char **
moo_mime_type_list_parents (const char *mime_type)
{
    char **p;
    char **ret;

    G_LOCK (moo_mime);

    ret = xdg_mime_list_mime_parents (mime_type);
    for (p = ret; p && *p; ++p)
        *p = (char*) mime_type_intern (*p);

    G_UNLOCK (moo_mime);

    return (const char**) ret;
}

void
moo_mime_shutdown (void)
{
    G_LOCK (moo_mime);
    xdg_mime_shutdown ();
    G_UNLOCK (moo_mime);
}


/* should not contain duplicates */
const char * const *
_moo_get_mime_data_dirs (void)
{
    /* It is called from inside xdgmime, i.e. when mime_lock is held */
    static char **data_dirs = NULL;

    if (!data_dirs)
    {
        guint i, n_dirs;
        GSList *dirs = NULL;
        const char* const *sys_dirs;

        sys_dirs = g_get_system_data_dirs ();

#ifndef __WIN32__
        dirs = g_slist_prepend (NULL, _moo_normalize_file_path (g_get_user_data_dir ()));
#endif

        for (i = 0; sys_dirs[i] && sys_dirs[i][0]; ++i)
        {
            char *norm = _moo_normalize_file_path (sys_dirs[i]);

            if (norm && !g_slist_find_custom (dirs, norm, (GCompareFunc) strcmp))
                dirs = g_slist_prepend (dirs, norm);
            else
                g_free (norm);
        }

        dirs = g_slist_reverse (dirs);
        n_dirs = g_slist_length (dirs);
        data_dirs = g_new (char*, n_dirs + 1);

        for (i = 0; i < n_dirs; ++i)
        {
            data_dirs[i] = (char*) dirs->data;
            dirs = g_slist_delete_link (dirs, dirs);
        }

        data_dirs[n_dirs] = NULL;
    }

    return (const char* const*) data_dirs;
}
