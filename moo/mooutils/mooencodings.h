/*
 *   mooencodings.h
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

#ifndef MOO_ENCODINGS_H
#define MOO_ENCODINGS_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_ENCODING_AUTO           "auto"
#define MOO_ENCODING_UTF8           "UTF-8"

#define MOO_ENCODING_UTF16          "UTF-16"
#define MOO_ENCODING_UTF16_BOM      "UTF-16-BOM"
#define MOO_ENCODING_UTF16LE        "UTF-16LE"
#define MOO_ENCODING_UTF16BE        "UTF-16BE"
#define MOO_ENCODING_UTF16LE_BOM    "UTF-16LE-BOM"
#define MOO_ENCODING_UTF16BE_BOM    "UTF-16BE-BOM"

#define MOO_ENCODING_UTF32          "UTF-32"
#define MOO_ENCODING_UTF32_BOM      "UTF-32-BOM"
#define MOO_ENCODING_UTF32LE        "UTF-32LE"
#define MOO_ENCODING_UTF32BE        "UTF-32BE"
#define MOO_ENCODING_UTF32LE_BOM    "UTF-32LE-BOM"
#define MOO_ENCODING_UTF32BE_BOM    "UTF-32BE-BOM"

typedef enum {
    MOO_ENCODING_COMBO_OPEN,
    MOO_ENCODING_COMBO_SAVE
} MooEncodingComboType;

void         _moo_encodings_combo_init      (GtkComboBox            *combo,
                                             MooEncodingComboType    type,
					     gboolean		     use_separators);
void         _moo_encodings_combo_set_enc   (GtkComboBox            *combo,
                                             const char             *enc,
                                             MooEncodingComboType    type);
const char  *_moo_encodings_combo_get_enc   (GtkComboBox            *combo,
                                             MooEncodingComboType    type);

void         _moo_encodings_attach_combo    (GtkWidget              *dialog,
                                             GtkWidget              *box,
                                             gboolean                save_mode,
                                             const char             *encoding);
const char  *_moo_encodings_combo_get       (GtkWidget              *dialog,
                                             gboolean                save_mode);

typedef void (*MooEncodingsMenuFunc)        (const char             *encoding,
                                             gpointer                data);
GtkAction   *_moo_encodings_menu_action_new (const char             *id,
                                             const char             *label,
                                             MooEncodingsMenuFunc    func,
                                             gpointer                data);
void         _moo_encodings_menu_action_set_current (GtkAction      *action,
                                             const char             *enc);

const char  *_moo_encoding_locale           (void);
gboolean     _moo_encodings_equal           (const char             *enc1,
                                             const char             *enc2);
const char  *_moo_encoding_get_display_name (const char             *enc);


G_END_DECLS

#endif /* MOO_ENCODINGS_H */
