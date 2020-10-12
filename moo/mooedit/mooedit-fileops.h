/*
 *   mooedit-fileops.h
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

#ifndef MOO_EDIT_FILE_OPS_H
#define MOO_EDIT_FILE_OPS_H

#include "mooedit/mooedit.h"
#include <gio/gio.h>

G_BEGIN_DECLS


const char *_moo_get_default_encodings (void);

typedef enum {
    MOO_EDIT_SAVE_FLAGS_NONE = 0,
    MOO_EDIT_SAVE_BACKUP = 1 << 0
} MooEditSaveFlags;

#define MOO_EDIT_FILE_ERROR (_moo_edit_file_error_quark ())

enum {
    MOO_EDIT_FILE_ERROR_ENCODING = 1,
    MOO_EDIT_FILE_ERROR_FAILED,
    MOO_EDIT_FILE_ERROR_NOT_IMPLEMENTED,
    MOO_EDIT_FILE_ERROR_NOENT,
    MOO_EDIT_FILE_ERROR_CANCELLED
};

GQuark           _moo_edit_file_error_quark     (void) G_GNUC_CONST;

gboolean         _moo_is_file_error_cancelled   (GError         *error);

gboolean         _moo_edit_file_is_new          (GFile          *file);
gboolean         _moo_edit_load_file            (MooEdit        *edit,
                                                 GFile          *file,
                                                 const char     *encoding,
                                                 const char     *cached_encoding,
                                                 GError        **error);
gboolean         _moo_edit_reload_file          (MooEdit        *edit,
                                                 const char     *encoding,
                                                 GError        **error);
gboolean         _moo_edit_save_file            (MooEdit        *edit,
                                                 GFile          *floc,
                                                 const char     *encoding,
                                                 MooEditSaveFlags flags,
                                                 GError        **error);
gboolean         _moo_edit_save_file_copy       (MooEdit        *edit,
                                                 GFile          *file,
                                                 const char     *encoding,
                                                 MooEditSaveFlags flags,
                                                 GError        **error);


G_END_DECLS

#ifdef __cplusplus

#include "mooutils/mooutils-cpp.h"

MOO_DEFINE_FLAGS(MooEditSaveFlags)

#endif // __cplusplus

#endif /* MOO_EDIT_FILE_OPS_H */
