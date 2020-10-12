/*
 *   mooutils-fs.h
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

#ifndef MOO_UTILS_FS_H
#define MOO_UTILS_FS_H

#include <mooglib/moo-glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_FILE_ERROR (_moo_file_error_quark ())

/* XXX */
#define _moo_save_file_utf8 moo_save_file_utf8

typedef enum
{
    MOO_FILE_ERROR_NONEXISTENT,
    MOO_FILE_ERROR_NOT_FOLDER,
    MOO_FILE_ERROR_BAD_FILENAME,
    MOO_FILE_ERROR_FAILED,
    MOO_FILE_ERROR_ALREADY_EXISTS,
    MOO_FILE_ERROR_ACCESS_DENIED,
    MOO_FILE_ERROR_READONLY,
    MOO_FILE_ERROR_DIFFERENT_FS,
    MOO_FILE_ERROR_NOT_IMPLEMENTED
} MooFileError;

GQuark          _moo_file_error_quark       (void) G_GNUC_CONST;
MooFileError    _moo_file_error_from_errno  (mgw_errno_t err_code);

gboolean        _moo_save_file_utf8         (const char *name,
                                             const char *text,
                                             gssize      len,
                                             GError    **error);
gboolean        _moo_remove_dir             (const char *path,
                                             gboolean    recursive,
                                             GError    **error);
gboolean        _moo_create_dir             (const char *path,
                                             GError    **error);
int             _moo_mkdir_with_parents     (const char *path,
                                             mgw_errno_t* err); /* S_IRWXU on unix */
gboolean        _moo_rename_file            (const char *path,
                                             const char *new_path,
                                             GError    **error);

char           **moo_filenames_from_locale  (char      **files);
char            *moo_filename_from_locale   (const char *file);
char           *_moo_filename_to_uri        (const char *file,
                                             GError    **error);

char           *_moo_normalize_file_path    (const char *filename);
gboolean        _moo_path_is_absolute       (const char *path);

gboolean        _moo_copy_files_ui          (GList      *filenames,
                                             const char *destdir,
                                             GtkWidget  *parent);
gboolean        _moo_move_files_ui          (GList      *filenames,
                                             const char *destdir,
                                             GtkWidget  *parent);

/*
 * C library and WinAPI functions wrappers analogous to glib/gstdio.h
 */

int             _moo_mkdir                  (const char  *path,
                                             mgw_errno_t *err); /* S_IRWXU on unix */

gboolean        _moo_glob_match_simple      (const char *pattern,
                                             const char *filename);


G_END_DECLS

#endif /* MOO_UTILS_FS_H */
