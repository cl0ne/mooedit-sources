/*
 *   moofile.h
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

#ifndef MOO_FILE_H
#define MOO_FILE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


typedef struct _MooFile   MooFile;
typedef struct _MooFolder MooFolder;

/* should be ordered TODO why? */
typedef enum {
    MOO_FILE_HAS_STAT       = 1 << 1,
    MOO_FILE_HAS_MIME_TYPE  = 1 << 2,
    MOO_FILE_HAS_ICON       = 1 << 3,
    MOO_FILE_ALL_FLAGS      = (1 << 4) - 1
} MooFileFlags;

typedef enum {
    MOO_FILE_INFO_EXISTS         = 1 << 0,
    MOO_FILE_INFO_IS_DIR         = 1 << 1,
    MOO_FILE_INFO_IS_HIDDEN      = 1 << 2,
    MOO_FILE_INFO_IS_LINK        = 1 << 3,
    MOO_FILE_INFO_IS_BLOCK_DEV   = 1 << 4,
    MOO_FILE_INFO_IS_CHAR_DEV    = 1 << 5,
    MOO_FILE_INFO_IS_FIFO        = 1 << 6,
    MOO_FILE_INFO_IS_SOCKET      = 1 << 7,
    MOO_FILE_INFO_IS_LOCKED      = 1 << 8   /* no read premissions */
} MooFileInfo;

#define MOO_FILE_INFO_IS_SPECIAL (MOO_FILE_INFO_IS_LINK | MOO_FILE_INFO_IS_BLOCK_DEV |  \
                                  MOO_FILE_INFO_IS_CHAR_DEV | MOO_FILE_INFO_IS_FIFO |   \
                                  MOO_FILE_INFO_IS_SOCKET)

#define MOO_FILE_EXISTS(file)           (_moo_file_test (file, MOO_FILE_INFO_EXISTS))
#define MOO_FILE_IS_DIR(file)           (_moo_file_test (file, MOO_FILE_INFO_IS_DIR))
#define MOO_FILE_IS_SPECIAL(file)       (_moo_file_test (file, MOO_FILE_INFO_IS_SPECIAL))
#define MOO_FILE_IS_LINK(file)          (_moo_file_test (file, MOO_FILE_INFO_IS_LINK))
#define MOO_FILE_IS_BROKEN_LINK(file)   (!MOO_FILE_EXISTS (file) && MOO_FILE_IS_LINK (file))
#define MOO_FILE_IS_LOCKED(file)        (_moo_file_test (file, MOO_FILE_INFO_IS_LOCKED))
#define MOO_FILE_IS_HIDDEN(file)        (_moo_file_test (file, MOO_FILE_INFO_IS_HIDDEN))


gboolean     _moo_file_test             (const MooFile  *file,
                                         MooFileInfo     test);
const char  *_moo_file_get_mime_type    (const MooFile  *file);
const char  *_moo_file_display_name     (const MooFile  *file);


G_END_DECLS

#endif /* MOO_FILE_H */
