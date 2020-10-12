/*
 *   moocmdview.h
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

#ifndef __MOO_CMD_VIEW__
#define __MOO_CMD_VIEW__

#include "moolineview.h"
#include "moooutputfilter.h"

G_BEGIN_DECLS


#define MOO_TYPE_CMD_VIEW              (moo_cmd_view_get_type ())
#define MOO_CMD_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_CMD_VIEW, MooCmdView))
#define MOO_CMD_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_CMD_VIEW, MooCmdViewClass))
#define MOO_IS_CMD_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_CMD_VIEW))
#define MOO_IS_CMD_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_CMD_VIEW))
#define MOO_CMD_VIEW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_CMD_VIEW, MooCmdViewClass))

#define MOO_CMD_VIEW_MESSAGE "message"
#define MOO_CMD_VIEW_ERROR   "error"
#define MOO_CMD_VIEW_STDOUT  "stdout"
#define MOO_CMD_VIEW_STDERR  "stderr"

typedef struct _MooCmdViewPrivate  MooCmdViewPrivate;

struct MooCmdView
{
    MooLineView parent;
    MooCmdViewPrivate *priv;
};

struct MooCmdViewClass
{
    MooLineViewClass parent_class;

    void     (*job_started) (MooCmdView *view,
                             const char *job_name);
    void     (*job_finished)(MooCmdView *view);

    /* action signal */
    gboolean (*abort)       (MooCmdView *view);

    gboolean (*cmd_exit)    (MooCmdView *view,
                             int         status);
    gboolean (*stdout_line) (MooCmdView *view,
                             const char *line);
    gboolean (*stderr_line) (MooCmdView *view,
                             const char *line);
};


GType       moo_cmd_view_get_type           (void) G_GNUC_CONST;

GtkWidget  *moo_cmd_view_new                (void);

void        moo_cmd_view_set_filter         (MooCmdView         *view,
                                             MooOutputFilter    *filter);
void        moo_cmd_view_set_filter_by_id   (MooCmdView         *view,
                                             const char         *id);
void        moo_cmd_view_add_filter_dirs    (MooCmdView         *view,
                                             char              **dirs);

void        moo_cmd_view_write_with_filter  (MooCmdView         *view,
                                             const char         *text,
                                             gboolean            error);

gboolean    moo_cmd_view_run_command        (MooCmdView         *view,
                                             const char         *cmd,
                                             const char         *working_dir,
                                             const char         *job_name);
gboolean    moo_cmd_view_run_command_full   (MooCmdView         *view,
                                             const char         *cmd,
                                             const char         *display_cmd,
                                             const char         *working_dir,
                                             char              **envp,
                                             const char         *job_name);

void        moo_cmd_view_abort              (MooCmdView         *view);
gboolean    moo_cmd_view_running            (MooCmdView         *view);


G_END_DECLS

#endif /* __MOO_CMD_VIEW__ */
