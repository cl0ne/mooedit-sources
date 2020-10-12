/*
 *   moofileview-private.h
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

#include <moofileview/moofileview-impl.h>
#include <gtk/gtk.h>

#ifndef MOO_FILE_VIEW_PRIVATE_H
#define MOO_FILE_VIEW_PRIVATE_H


#define MOO_TYPE_FILE_VIEW_TYPE         (_moo_file_view_type_get_type ())

typedef enum {
    MOO_FILE_VIEW_LIST,
    MOO_FILE_VIEW_ICON,
    MOO_FILE_VIEW_BOOKMARK
} MooFileViewType;


GType       _moo_file_view_type_get_type                (void) G_GNUC_CONST;

/* returns list of MooFile* pointers, must be freed, and elements must be unref'ed */
GList      *_moo_file_view_get_files                    (MooFileView    *fileview);


#endif /* MOO_FILE_VIEW_PRIVATE_H */
