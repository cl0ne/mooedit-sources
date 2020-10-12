/*
 *   mooeditconfig.h
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

#ifndef MOO_EDIT_CONFIG_H
#define MOO_EDIT_CONFIG_H

#include <glib-object.h>
#include <mooedit/mooedit-enums.h>

G_BEGIN_DECLS


#define MOO_TYPE_EDIT_CONFIG              (moo_edit_config_get_type ())
#define MOO_EDIT_CONFIG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_EDIT_CONFIG, MooEditConfig))
#define MOO_EDIT_CONFIG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_EDIT_CONFIG, MooEditConfigClass))
#define MOO_IS_EDIT_CONFIG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_EDIT_CONFIG))
#define MOO_IS_EDIT_CONFIG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_EDIT_CONFIG))
#define MOO_EDIT_CONFIG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_EDIT_CONFIG, MooEditConfigClass))

typedef struct MooEditConfig         MooEditConfig;
typedef struct MooEditConfigPrivate  MooEditConfigPrivate;
typedef struct MooEditConfigClass    MooEditConfigClass;

struct MooEditConfig
{
    GObject object;
    MooEditConfigPrivate *priv;
};

struct MooEditConfigClass
{
    GObjectClass object_class;
};


GType           moo_edit_config_get_type        (void) G_GNUC_CONST;

MooEditConfig  *moo_edit_config_new             (void);

const char     *moo_edit_config_get_string      (MooEditConfig  *config,
                                                 const char     *setting);
guint           moo_edit_config_get_uint        (MooEditConfig  *config,
                                                 const char     *setting);
gboolean        moo_edit_config_get_bool        (MooEditConfig  *config,
                                                 const char     *setting);

void            moo_edit_config_set             (MooEditConfig  *config,
                                                 MooEditConfigSource source,
                                                 const char     *first_setting,
                                                 ...) G_GNUC_NULL_TERMINATED; /* setting, value, ... */
void            moo_edit_config_get             (MooEditConfig  *config,
                                                 const char     *first_setting,
                                                 ...) G_GNUC_NULL_TERMINATED; /* alias for g_object_get() */

void            moo_edit_config_set_global      (MooEditConfigSource source,
                                                 const char     *first_setting,
                                                 ...) G_GNUC_NULL_TERMINATED; /* setting, value, ... */

void            moo_edit_config_parse_one       (MooEditConfig  *config,
                                                 const char     *setting_name,
                                                 const char     *value,
                                                 MooEditConfigSource source);
void            moo_edit_config_parse           (MooEditConfig  *config,
                                                 const char     *string,
                                                 MooEditConfigSource source);

guint           moo_edit_config_install_setting (GParamSpec     *pspec);
void            moo_edit_config_install_alias   (const char     *name,
                                                 const char     *alias);

guint           moo_edit_config_get_setting_id  (GParamSpec     *pspec);
GParamSpec     *moo_edit_config_get_spec        (guint           id);
GParamSpec     *moo_edit_config_lookup_spec     (const char     *name,
                                                 guint          *id,
                                                 gboolean        try_alias);

void            moo_edit_config_unset_by_source (MooEditConfig  *config,
                                                 MooEditConfigSource source);

gboolean        moo_edit_config_parse_bool      (const char     *string,
                                                 gboolean       *value);


G_END_DECLS

#endif /* MOO_EDIT_CONFIG_H */
