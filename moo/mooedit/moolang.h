/*
 *   moolang.h
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

#ifndef MOO_LANG_H
#define MOO_LANG_H

#include <glib-object.h>

G_BEGIN_DECLS


#define MOO_TYPE_LANG              (moo_lang_get_type ())
#define MOO_LANG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_LANG, MooLang))
#define MOO_IS_LANG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_LANG))

#define MOO_LANG_NONE       "none"
#define MOO_LANG_NONE_NAME  "None"

typedef struct MooLang MooLang;

GType        moo_lang_get_type                  (void) G_GNUC_CONST;

/* accepts NULL */
const char  *_moo_lang_id                       (MooLang    *lang);
const char  *_moo_lang_display_name             (MooLang    *lang);

/* result must be freed together with content */
GSList      *_moo_lang_get_globs                (MooLang    *lang);
/* result must be freed together with content */
GSList      *_moo_lang_get_mime_types           (MooLang    *lang);

const char  *_moo_lang_get_line_comment         (MooLang    *lang);
const char  *_moo_lang_get_block_comment_start  (MooLang    *lang);
const char  *_moo_lang_get_block_comment_end    (MooLang    *lang);
const char  *_moo_lang_get_brackets             (MooLang    *lang);
const char  *_moo_lang_get_section              (MooLang    *lang);
gboolean     _moo_lang_get_hidden               (MooLang    *lang);

char        *_moo_lang_id_from_name             (const char *whatever);
GSList      *_moo_lang_parse_string_list        (const char *string);


G_END_DECLS

#endif /* MOO_LANG_H */
