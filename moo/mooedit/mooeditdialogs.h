/*
 *   mooeditdialogs.h
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

#ifndef MOO_EDIT_DIALOGS_H
#define MOO_EDIT_DIALOGS_H

#include "mooutils/moodialogs.h"
#include "mooedit/mooedittypes.h"
#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum {
    MOO_EDIT_TRY_ENCODING_RESPONSE_TRY_ANOTHER,
    MOO_EDIT_TRY_ENCODING_RESPONSE_CANCEL
} MooEditTryEncodingResponse;

MooSaveInfo                    *_moo_edit_save_as_dialog                (MooEdit        *doc,
                                                                         const char     *display_basename);
MooOpenInfoArray               *_moo_edit_open_dialog                   (GtkWidget      *widget,
                                                                         MooEdit        *current_doc);

MooSaveChangesResponse          _moo_edit_save_changes_dialog           (MooEdit        *doc);
MooSaveChangesResponse          _moo_edit_save_multiple_changes_dialog  (MooEditArray   *docs,
                                                                         MooEditArray   *to_save);

gboolean                        _moo_edit_reload_modified_dialog        (MooEdit        *doc);
gboolean                        _moo_edit_overwrite_modified_dialog     (MooEdit        *doc);

void                            _moo_edit_save_error_dialog             (MooEdit        *doc,
                                                                         GFile          *file,
                                                                         GError         *error);
gboolean                        _moo_edit_save_error_enc_dialog         (MooEdit        *doc,
                                                                         GFile          *file,
                                                                         const char     *encoding);
void                            _moo_edit_open_error_dialog             (GtkWidget      *widget,
                                                                         GFile          *file,
                                                                         GError         *error);
void                            _moo_edit_reload_error_dialog           (MooEdit        *doc,
                                                                         GError         *error);
MooEditTryEncodingResponse      _moo_edit_try_encoding_dialog           (GFile          *file,
                                                                         const char     *encoding,
                                                                         char          **new_encoding);

gboolean                        _moo_text_search_from_start_dialog      (GtkWidget      *parent,
                                                                         gboolean        backwards);
void                            _moo_text_regex_error_dialog            (GtkWidget      *parent,
                                                                         GError         *error);

gboolean                        _moo_text_replace_from_start_dialog     (GtkWidget      *parent,
                                                                         int             replaced);
GtkWidget                      *_moo_text_prompt_on_replace_dialog      (GtkWidget      *parent);

G_END_DECLS

#endif /* MOO_EDIT_DIALOGS_H */
