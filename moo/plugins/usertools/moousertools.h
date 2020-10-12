/*
 *   moousertools.h
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

#ifndef MOO_USER_TOOLS_H
#define MOO_USER_TOOLS_H

#include <mooutils/moouixml.h>
#include "moocommand.h"

G_BEGIN_DECLS

#define MOO_USER_TOOLS_PLUGIN_ID "UserTools"

#define MOO_TYPE_USER_TOOL_INFO (_moo_user_tool_info_get_type ())

typedef enum {
    MOO_USER_TOOL_MENU,
    MOO_USER_TOOL_CONTEXT
} MooUserToolType;

typedef enum {
    MOO_USER_TOOL_POS_END,
    MOO_USER_TOOL_POS_START
} MooUserToolPosition;

typedef struct {
    char                *id;
    char                *name;
    char                *accel;
    char                *menu;
    char                *filter;
    char                *options;
    MooUserToolPosition  position;
    MooCommandFactory   *cmd_factory;
    MooCommandData      *cmd_data;
    MooUserToolType      type;
    char                *file;
    guint                ref_count : 29;
    guint                enabled : 1;
    guint                deleted : 1;
    guint                builtin : 1;
} MooUserToolInfo;

GType _moo_user_tool_info_get_type (void) G_GNUC_CONST;

MooUserToolInfo *_moo_user_tool_info_new    (void);
void             _moo_user_tool_info_unref  (MooUserToolInfo *info);

void    _moo_edit_load_user_tools       (void);
void    _moo_edit_load_user_tools_type  (MooUserToolType         type);

typedef void (*MooToolFileParseFunc)    (MooUserToolInfo        *info,
                                         gpointer                data);

/* caller must free the list and unref() the contents */
GSList *_moo_edit_parse_user_tools      (MooUserToolType         type,
                                         gboolean                all);
void    _moo_edit_save_user_tools       (MooUserToolType         type,
                                         GSList                 *user_info);


G_END_DECLS

#endif /* MOO_USER_TOOLS_H */
