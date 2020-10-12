/*
 *   mooeditfileinfo.h
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

#pragma once

#include <gio/gio.h>
#include <mooedit/mooedittypes.h>
#include <mooutils/mooutils-cpp.h>

G_BEGIN_DECLS

#define MOO_TYPE_OPEN_INFO                       (moo_open_info_get_type ())
#define MOO_OPEN_INFO(object)                    (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_OPEN_INFO, MooOpenInfo))
#define MOO_OPEN_INFO_CLASS(klass)               (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_OPEN_INFO, MooOpenInfoClass))
#define MOO_IS_OPEN_INFO(object)                 (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_OPEN_INFO))
#define MOO_IS_OPEN_INFO_CLASS(klass)            (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_OPEN_INFO))
#define MOO_OPEN_INFO_GET_CLASS(obj)             (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_OPEN_INFO, MooOpenInfoClass))

#define MOO_TYPE_SAVE_INFO                       (moo_save_info_get_type ())
#define MOO_SAVE_INFO(object)                    (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_SAVE_INFO, MooSaveInfo))
#define MOO_SAVE_INFO_CLASS(klass)               (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_SAVE_INFO, MooSaveInfoClass))
#define MOO_IS_SAVE_INFO(object)                 (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_SAVE_INFO))
#define MOO_IS_SAVE_INFO_CLASS(klass)            (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_SAVE_INFO))
#define MOO_SAVE_INFO_GET_CLASS(obj)             (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_SAVE_INFO, MooSaveInfoClass))

#define MOO_TYPE_RELOAD_INFO                     (moo_reload_info_get_type ())
#define MOO_RELOAD_INFO(object)                  (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_RELOAD_INFO, MooReloadInfo))
#define MOO_RELOAD_INFO_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_RELOAD_INFO, MooReloadInfoClass))
#define MOO_IS_RELOAD_INFO(object)               (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_RELOAD_INFO))
#define MOO_IS_RELOAD_INFO_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_RELOAD_INFO))
#define MOO_RELOAD_INFO_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_RELOAD_INFO, MooReloadInfoClass))

typedef struct MooOpenInfoClass MooOpenInfoClass;
typedef struct MooSaveInfoClass MooSaveInfoClass;
typedef struct MooReloadInfoClass MooReloadInfoClass;

GType                moo_open_info_get_type     (void) G_GNUC_CONST;
GType                moo_reload_info_get_type   (void) G_GNUC_CONST;
GType                moo_save_info_get_type     (void) G_GNUC_CONST;

MooOpenInfo         *moo_open_info_new          (const char         *path,
                                                 const char         *encoding,
                                                 int                 line,
                                                 MooOpenFlags        flags);
MooOpenInfo         *moo_open_info_new_file     (GFile              *file,
                                                 const char         *encoding,
                                                 int                 line,
                                                 MooOpenFlags        flags);
MooOpenInfo         *moo_open_info_new_uri      (const char         *uri,
                                                 const char         *encoding,
                                                 int                 line,
                                                 MooOpenFlags        flags);
MooOpenInfo         *moo_open_info_dup          (MooOpenInfo        *info);
void                 moo_open_info_free         (MooOpenInfo        *info);

char                *moo_open_info_get_filename (MooOpenInfo        *info);
char                *moo_open_info_get_uri      (MooOpenInfo        *info);
GFile               *moo_open_info_get_file     (MooOpenInfo        *info);
char                *moo_open_info_get_uri      (MooOpenInfo        *info);
const char          *moo_open_info_get_encoding (MooOpenInfo        *info);
void                 moo_open_info_set_encoding (MooOpenInfo        *info,
                                                 const char         *encoding);
int                  moo_open_info_get_line     (MooOpenInfo        *info);
void                 moo_open_info_set_line     (MooOpenInfo        *info,
                                                 int                 line);
MooOpenFlags         moo_open_info_get_flags    (MooOpenInfo        *info);
void                 moo_open_info_set_flags    (MooOpenInfo        *info,
                                                 MooOpenFlags        flags);
void                 moo_open_info_add_flags    (MooOpenInfo        *info,
                                                 MooOpenFlags        flags);

MooReloadInfo       *moo_reload_info_new        (const char         *encoding,
                                                 int                 line);
MooReloadInfo       *moo_reload_info_dup        (MooReloadInfo      *info);
void                 moo_reload_info_free       (MooReloadInfo      *info);

int                  moo_reload_info_get_line   (MooReloadInfo      *info);
void                 moo_reload_info_set_line   (MooReloadInfo      *info,
                                                 int                 line);

MooSaveInfo         *moo_save_info_new          (const char         *path,
                                                 const char         *encoding);
MooSaveInfo         *moo_save_info_new_file     (GFile              *file,
                                                 const char         *encoding);
MooSaveInfo         *moo_save_info_new_uri      (const char         *uri,
                                                 const char         *encoding);
MooSaveInfo         *moo_save_info_dup          (MooSaveInfo        *info);
void                 moo_save_info_free         (MooSaveInfo        *info);

G_END_DECLS

#ifdef __cplusplus

MOO_DEFINE_GOBJ_TRAITS(MooOpenInfo, MOO_TYPE_OPEN_INFO);
MOO_DEFINE_GOBJ_TRAITS(MooSaveInfo, MOO_TYPE_SAVE_INFO);
MOO_DEFINE_GOBJ_TRAITS(MooReloadInfo, MOO_TYPE_RELOAD_INFO);

#endif // __cplusplus
