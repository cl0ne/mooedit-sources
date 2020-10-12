/*
 *   moocommand-script.h
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

#ifndef MOO_COMMAND_SCRIPT_H
#define MOO_COMMAND_SCRIPT_H

#include "plugins/usertools/moocommand.h"

G_BEGIN_DECLS


#define MOO_TYPE_COMMAND_SCRIPT             (_moo_command_script_get_type ())
#define MOO_COMMAND_SCRIPT(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_COMMAND_SCRIPT, MooCommandScript))
#define MOO_COMMAND_SCRIPT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_COMMAND_SCRIPT, MooCommandScriptClass))
#define MOO_IS_COMMAND_SCRIPT(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_COMMAND_SCRIPT))
#define MOO_IS_COMMAND_SCRIPT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_COMMAND_SCRIPT))
#define MOO_COMMAND_SCRIPT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_COMMAND_SCRIPT, MooCommandScriptClass))

typedef struct _MooCommandScriptPrivate MooCommandScriptPrivate;

typedef enum {
    MOO_SCRIPT_LUA,
    MOO_SCRIPT_PYTHON
} MooScriptType;

struct MooCommandScript : public MooCommand {
    MooScriptType type;
    char *code;
};

struct MooCommandScriptClass : public MooCommandClass {
};


GType _moo_command_script_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* MOO_COMMAND_SCRIPT_H */
