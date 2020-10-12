/*
 *   mooprefs.h
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

#ifndef MOO_PREFS_H
#define MOO_PREFS_H

#include <glib-object.h>
#include <mooutils/moomarkup.h>
#include <mooutils/mooutils-file.h>

G_BEGIN_DECLS

typedef enum {
    MOO_PREFS_RC,
    MOO_PREFS_STATE
} MooPrefsKind;

gboolean        moo_prefs_load          (char          **sys_files,
                                         const char     *file_rc,
                                         const char     *file_state,
                                         GError        **error);
gboolean        moo_prefs_save          (const char     *file_rc,
                                         const char     *file_state,
                                         GError        **error);

MooMarkupNode  *moo_prefs_get_markup    (MooPrefsKind    prefs_kind);

void            moo_prefs_new_key       (const char     *key,
                                         GType           value_type,
                                         const GValue   *default_value,
                                         MooPrefsKind    prefs_kind);
void            moo_prefs_delete_key    (const char     *key);

GType           moo_prefs_get_key_type  (const char     *key);
gboolean        moo_prefs_key_registered(const char     *key);

GSList         *moo_prefs_list_keys     (MooPrefsKind    prefs_kind);

const GValue   *moo_prefs_get           (const char     *key);
const GValue   *moo_prefs_get_default   (const char     *key);
void            moo_prefs_set           (const char     *key,
                                         const GValue   *value);
void            moo_prefs_set_default   (const char     *key,
                                         const GValue   *value);

void            moo_prefs_new_key_bool  (const char     *key,
                                         gboolean        default_val);
void            moo_prefs_new_key_int   (const char     *key,
                                         int             default_val);
void            moo_prefs_new_key_string(const char     *key,
                                         const char     *default_val);

void            moo_prefs_create_key    (const char     *key,
                                         MooPrefsKind    prefs_kind,
                                         GType           value_type,
                                         ...);

char           *moo_prefs_make_key      (const char     *first_comp,
                                         ...) G_GNUC_NULL_TERMINATED;
char           *moo_prefs_make_keyv     (const char     *first_comp,
                                         va_list         var_args);

const char     *moo_prefs_get_string    (const char     *key);
const char     *moo_prefs_get_filename  (const char     *key);
GFile          *moo_prefs_get_file      (const char     *key);
gboolean        moo_prefs_get_bool      (const char     *key);
int             moo_prefs_get_int       (const char     *key);

void            moo_prefs_set_string    (const char     *key,
                                         const char     *val);
void            moo_prefs_set_filename  (const char     *key,
                                         const char     *val);
void            moo_prefs_set_file      (const char     *key,
                                         GFile          *val);
void            moo_prefs_set_int       (const char     *key,
                                         int             val);
void            moo_prefs_set_bool      (const char     *key,
                                         gboolean        val);

G_END_DECLS

#endif /* MOO_PREFS_H */
