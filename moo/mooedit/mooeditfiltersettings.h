/*
 *   mooeditfiltersettings.h
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

#ifndef MOO_EDIT_FILTER_SETTINGS_H
#define MOO_EDIT_FILTER_SETTINGS_H

#include <mooedit/mooedit.h>

G_BEGIN_DECLS


typedef struct MooEditFilter MooEditFilter;

typedef enum {
    MOO_EDIT_FILTER_CONFIG,
    MOO_EDIT_FILTER_ACTION
} MooEditFilterKind;

MooEditFilter  *_moo_edit_filter_new                    (const char         *string,
                                                         MooEditFilterKind   kind);
MooEditFilter  *_moo_edit_filter_new_full               (const char         *string,
                                                         MooEditFilterKind   kind,
                                                         GError            **error);
gboolean        _moo_edit_filter_valid                  (MooEditFilter      *filter);
void            _moo_edit_filter_free                   (MooEditFilter      *filter);
gboolean        _moo_edit_filter_match                  (MooEditFilter      *filter,
                                                         MooEdit            *doc);

void            _moo_edit_filter_settings_load          (void);

GSList         *_moo_edit_filter_settings_get_strings   (void);
void            _moo_edit_filter_settings_set_strings   (GSList             *strings);

char           *_moo_edit_filter_settings_get_for_doc   (MooEdit            *doc);


G_END_DECLS

#endif /* MOO_EDIT_FILTER_SETTINGS_H */
