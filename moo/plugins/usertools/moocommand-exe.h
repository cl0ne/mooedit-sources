/*
 *   moocommand-exe.h
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

#ifndef MOO_COMMAND_EXE_H
#define MOO_COMMAND_EXE_H

#include "plugins/usertools/moocommand.h"
#include <mooedit/mooeditwindow.h>

G_BEGIN_DECLS


#define MOO_TYPE_COMMAND_EXE            (_moo_command_exe_get_type ())
#define MOO_COMMAND_EXE(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_COMMAND_EXE, MooCommandExe))
#define MOO_COMMAND_EXE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_COMMAND_EXE, MooCommandExeClass))
#define MOO_IS_COMMAND_EXE(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_COMMAND_EXE))
#define MOO_IS_COMMAND_EXE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_COMMAND_EXE))
#define MOO_COMMAND_EXE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_COMMAND_EXE, MooCommandExeClass))

typedef struct _MooCommandExePrivate MooCommandExePrivate;

struct MooCommandExe : public MooCommand {
    MooCommandExePrivate *priv;
};

struct MooCommandExeClass : public MooCommandClass {
};


GType   _moo_command_exe_get_type   (void) G_GNUC_CONST;

void    _moo_edit_run_in_pane       (const char     *cmd_line,
                                     const char     *working_dir,
                                     char          **envp,
                                     MooEditWindow  *window,
                                     MooEdit        *doc);
void    _moo_edit_run_async         (const char     *cmd_line,
                                     const char     *working_dir,
                                     char          **envp,
                                     MooEditWindow  *window,
                                     MooEdit        *doc);
void    _moo_edit_run_sync          (const char     *cmd_line,
                                     const char     *working_dir,
                                     char          **envp,
                                     MooEditWindow  *window,
                                     MooEdit        *doc,
                                     const char     *input,
                                     int            *exit_status,
                                     char          **output,
                                     char          **output_err);


G_END_DECLS

#endif /* MOO_COMMAND_EXE_H */
