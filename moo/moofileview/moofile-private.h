/*
 *   moofile-private.h
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

#ifndef MOO_FILE_PRIVATE_H
#define MOO_FILE_PRIVATE_H

#include "moofileview/moofile.h"
#include <sys/types.h>

G_BEGIN_DECLS


#define MOO_TYPE_FILE               (_moo_file_get_type ())

#ifdef __WIN32__
/* FILETIME */
typedef guint64 MooFileTime;
#else
/* time_t */
/* XXX it's not time_t! */
typedef GTime MooFileTime;
#endif

typedef gint64 MooFileSize;
typedef struct MooCollationKey MooCollationKey;

struct _MooFile
{
    char               *name;
    char               *link_target;
    char               *display_name; /* normalized */
    char               *case_display_name;
    MooCollationKey    *collation_key;
    MooFileInfo         info;
    MooFileFlags        flags;
    guint8              icon;
    const char         *mime_type;
    int                 ref_count;
    struct MgwStatBuf  *statbuf;
};


GType        _moo_file_get_type         (void) G_GNUC_CONST;

MooFile     *_moo_file_new              (const char     *dirname,
                                         const char     *basename);

MooFile     *_moo_file_ref              (MooFile        *file);
void         _moo_file_unref            (MooFile        *file);

MooFileInfo  _moo_file_get_info         (const MooFile  *file);

const char  *_moo_file_name             (const MooFile  *file);

/* returned pixbuf is owned by icon cache */
GdkPixbuf   *_moo_file_get_icon         (const MooFile  *file,
                                         GtkWidget      *widget,
                                         GtkIconSize     size);

gsize        _moo_collation_key_size    (MooCollationKey *key);
int          _moo_collation_key_cmp     (const MooCollationKey *key1,
                                         const MooCollationKey *key2);
const MooCollationKey *_moo_file_collation_key (const MooFile *file);

const char  *_moo_file_case_display_name(const MooFile  *file);

#ifndef __WIN32__
const char  *_moo_file_link_get_target  (const MooFile  *file);
#endif

guint8       _moo_file_icon_blank       (void);
guint8       _moo_file_get_icon_type    (MooFile        *file,
                                         const char     *dirname);
void         _moo_file_stat             (MooFile        *file,
                                         const char     *dirname);
void         _moo_file_free_statbuf     (MooFile        *file);
void         _moo_file_find_mime_type   (MooFile        *file,
                                         const char     *path);


G_END_DECLS

#endif /* MOO_FILE_PRIVATE_H */
