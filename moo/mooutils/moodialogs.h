/*
 *   moodialogs.h
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

#ifndef MOOUTILS_DIALOGS_H
#define MOOUTILS_DIALOGS_H

#include <gtk/gtk.h>
#include "mooutils/moofiledialog.h"

G_BEGIN_DECLS


/**
 * enum:MooSaveChangesResponse
 *
 * @MOO_SAVE_CHANGES_RESPONSE_CANCEL: cancel current operation, don't save
 * @MOO_SAVE_CHANGES_RESPONSE_SAVE: do save the file
 * @MOO_SAVE_CHANGES_RESPONSE_DONT_SAVE: do not save the file and continue
 *
 * Values returned by moo_save_changes_dialog().
 **/
typedef enum {
    MOO_SAVE_CHANGES_RESPONSE_CANCEL,
    MOO_SAVE_CHANGES_RESPONSE_SAVE,
    MOO_SAVE_CHANGES_RESPONSE_DONT_SAVE
} MooSaveChangesResponse;

gboolean    moo_overwrite_file_dialog       (const char         *display_name,
                                             const char         *display_dirname,
                                             GtkWidget          *parent);
MooSaveChangesResponse
            moo_save_changes_dialog         (const char         *display_name,
                                             GtkWidget          *parent);

void        moo_position_window_at_pointer  (GtkWidget          *window,
                                             GtkWidget          *parent);
void        moo_window_set_parent           (GtkWidget          *window,
                                             GtkWidget          *parent);

gboolean    moo_question_dialog             (const char         *text,
                                             const char         *secondary_text,
                                             GtkWidget          *parent,
                                             GtkResponseType     default_response);
void        moo_error_dialog                (const char         *text,
                                             const char         *secondary_text,
                                             GtkWidget          *parent);
void        moo_info_dialog                 (const char         *text,
                                             const char         *secondary_text,
                                             GtkWidget          *parent);
void        moo_warning_dialog              (const char         *text,
                                             const char         *secondary_text,
                                             GtkWidget          *parent);

void        _moo_window_set_remember_size   (GtkWindow          *window,
                                             const char         *prefs_key,
                                             int                 default_width,
                                             int                 default_height,
                                             gboolean            remember_position);


G_END_DECLS

#endif /* MOOUTILS_DIALOGS_H */
