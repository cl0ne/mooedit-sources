/*
 *   moocmdview.c
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

/**
 * class:MooCmdView: (parent MooLineView) (constructable)
 **/

#include "moocmdview.h"
#include "mooedit/mooeditwindow.h"
#include "marshals.h"
#include "mooutils/moospawn.h"
#include "mooutils/mooutils-misc.h"
#include "plugins/usertools/moocommand.h"

#ifndef __WIN32__
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#endif /* !__WIN32__ */


struct _MooCmdViewPrivate {
    MooCmd *cmd;

    GtkTextTag *error_tag;
    GtkTextTag *message_tag;
    GtkTextTag *stdout_tag;
    GtkTextTag *stderr_tag;

    MooEditWindow *window;
    MooOutputFilter *filter;
};

static void      moo_cmd_view_destroy       (GtkObject  *object);
static GObject  *moo_cmd_view_constructor   (GType                  type,
                                             guint                  n_construct_properties,
                                             GObjectConstructParam *construct_param);

static void     moo_cmd_view_abort_and_disconnect (MooCmdView *view);

static gboolean moo_cmd_view_abort_real     (MooCmdView *view);
static gboolean moo_cmd_view_cmd_exit       (MooCmdView *view,
                                             int         status);
static gboolean moo_cmd_view_stdout_line    (MooCmdView *view,
                                             const char *line);
static gboolean moo_cmd_view_stderr_line    (MooCmdView *view,
                                             const char *line);



enum {
    ABORT,
    CMD_EXIT,
    OUTPUT_LINE,
    STDOUT_LINE,
    STDERR_LINE,
    JOB_STARTED,
    JOB_FINISHED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


/* MOO_TYPE_CMD_VIEW */
G_DEFINE_TYPE (MooCmdView, moo_cmd_view, MOO_TYPE_LINE_VIEW)


static void
moo_cmd_view_class_init (MooCmdViewClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);

    gobject_class->constructor = moo_cmd_view_constructor;

    gtkobject_class->destroy = moo_cmd_view_destroy;

    klass->abort = moo_cmd_view_abort_real;
    klass->cmd_exit = moo_cmd_view_cmd_exit;
    klass->stdout_line = moo_cmd_view_stdout_line;
    klass->stderr_line = moo_cmd_view_stderr_line;

    g_type_class_add_private (klass, sizeof (MooCmdViewPrivate));

    signals[ABORT] =
            g_signal_new ("abort",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                          G_STRUCT_OFFSET (MooCmdViewClass, abort),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOL__VOID,
                          G_TYPE_BOOLEAN, 0);

    signals[CMD_EXIT] =
            g_signal_new ("cmd-exit",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooCmdViewClass, cmd_exit),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOL__INT,
                          G_TYPE_BOOLEAN, 1,
                          G_TYPE_INT);

    signals[STDOUT_LINE] =
            g_signal_new ("stdout-line",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooCmdViewClass, stdout_line),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOL__STRING,
                          G_TYPE_BOOLEAN, 1,
                          G_TYPE_STRING);

    signals[STDERR_LINE] =
            g_signal_new ("stderr-line",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooCmdViewClass, stderr_line),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOL__STRING,
                          G_TYPE_BOOLEAN, 1,
                          G_TYPE_STRING);

    signals[JOB_STARTED] =
            g_signal_new ("job-started",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooCmdViewClass, job_started),
                          NULL, NULL,
                          _moo_marshal_VOID__STRING,
                          G_TYPE_NONE, 1,
                          G_TYPE_STRING);

    signals[JOB_FINISHED] =
            g_signal_new ("job-finished",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooCmdViewClass, job_finished),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);
}


static void
moo_cmd_view_init (MooCmdView *view)
{
    view->priv = G_TYPE_INSTANCE_GET_PRIVATE (view, MOO_TYPE_CMD_VIEW, MooCmdViewPrivate);
}


static GObject*
moo_cmd_view_constructor (GType                  type,
                          guint                  n_props,
                          GObjectConstructParam *props)
{
    GObject *object;
    MooCmdView *view;
    GtkTextBuffer *buffer;

    object = G_OBJECT_CLASS(moo_cmd_view_parent_class)->constructor (type, n_props, props);
    view = MOO_CMD_VIEW (object);

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

    view->priv->message_tag = gtk_text_buffer_create_tag (buffer, "message", NULL);
    view->priv->error_tag = gtk_text_buffer_create_tag (buffer, "error", NULL);
    view->priv->stdout_tag = gtk_text_buffer_create_tag (buffer, "stdout", NULL);
    view->priv->stderr_tag = gtk_text_buffer_create_tag (buffer, "stderr", NULL);

    g_object_set (view->priv->error_tag, "foreground", "red", nullptr);
    g_object_set (view->priv->stderr_tag, "foreground", "red", nullptr);

    return object;
}


static void
moo_cmd_view_destroy (GtkObject *object)
{
    MooCmdView *view = MOO_CMD_VIEW (object);

    moo_cmd_view_abort_and_disconnect (view);
    moo_cmd_view_set_filter (view, NULL);

    if (GTK_OBJECT_CLASS (moo_cmd_view_parent_class)->destroy)
        GTK_OBJECT_CLASS (moo_cmd_view_parent_class)->destroy (object);
}


static void
moo_cmd_view_abort_and_disconnect (MooCmdView *view)
{
    MooCmd *cmd = view->priv->cmd;
    view->priv->cmd = NULL;

    if (cmd)
    {
        g_signal_handlers_disconnect_matched (cmd, G_SIGNAL_MATCH_DATA,
                                              0, 0, NULL, NULL, view);
        _moo_cmd_abort (cmd);
        g_object_unref (cmd);
    }
}


GtkWidget*
moo_cmd_view_new (void)
{
    return GTK_WIDGET (g_object_new (MOO_TYPE_CMD_VIEW,
                                     "highlight-current-line", TRUE,
                                     "highlight-current-line-unfocused", TRUE,
                                     (const char*) NULL));
}


void
moo_cmd_view_set_filter (MooCmdView      *view,
                         MooOutputFilter *filter)
{
    g_return_if_fail (MOO_IS_CMD_VIEW (view));
    g_return_if_fail (!filter || MOO_IS_OUTPUT_FILTER (filter));

    if (view->priv->filter == filter)
        return;

    if (view->priv->filter)
    {
        moo_output_filter_set_window (view->priv->filter, NULL);
        moo_output_filter_set_view (view->priv->filter, NULL);
        g_object_unref (view->priv->filter);
        view->priv->filter = NULL;
    }

    view->priv->filter = filter;

    if (view->priv->filter)
    {
        g_object_ref (view->priv->filter);
        moo_output_filter_set_window (view->priv->filter, view->priv->window);
        moo_output_filter_set_view (view->priv->filter, MOO_LINE_VIEW (view));
    }
}

/**
 * moo_cmd_view_set_filter_by_id:
 *
 * @view:
 * @id: (type const-utf8)
 */
void
moo_cmd_view_set_filter_by_id (MooCmdView *view,
                               const char *id)
{
    MooOutputFilter *filter = NULL;

    g_return_if_fail (MOO_IS_CMD_VIEW (view));

    if (id != NULL)
    {
        filter = moo_command_filter_create (id);
        g_return_if_fail (filter != NULL);
    }

    moo_cmd_view_set_filter (view, filter);

    if (filter)
        g_object_unref (filter);
}

void
moo_cmd_view_add_filter_dirs (MooCmdView  *view,
                              char       **dirs)
{
    g_return_if_fail (MOO_IS_CMD_VIEW (view));

    if (view->priv->filter)
        moo_output_filter_add_active_dirs (view->priv->filter, dirs);
}


static gboolean
cmd_exit_cb (MooCmd     *cmd,
             int         status,
             MooCmdView *view)
{
    gboolean result = FALSE;

    g_return_val_if_fail (cmd == view->priv->cmd, FALSE);
    g_signal_emit (view, signals[CMD_EXIT], 0, status, &result);

    g_signal_emit (view, signals[JOB_FINISHED], 0);
    g_object_unref (cmd);
    view->priv->cmd = NULL;

    return result;
}


static gboolean
stdout_line_cb (MooCmd     *cmd,
                const char *text,
                MooCmdView *view)
{
    gboolean result = FALSE;
    g_return_val_if_fail (cmd == view->priv->cmd, FALSE);
    g_signal_emit (view, signals[STDOUT_LINE], 0, text, &result);
    return result;
}


static gboolean
stderr_line_cb (MooCmd     *cmd,
                const char *text,
                MooCmdView *view)
{
    gboolean result = FALSE;
    g_return_val_if_fail (cmd == view->priv->cmd, FALSE);
    g_signal_emit (view, signals[STDERR_LINE], 0, text, &result);
    return result;
}


/**
 * moo_cmd_view_run_command:
 *
 * @view:
 * @cmd: (type const-filename)
 * @working_dir: (type const-filename) (allow-none) (default NULL)
 * @job_name: (type const-utf8) (allow-none) (default NULL)
 */
gboolean
moo_cmd_view_run_command (MooCmdView *view,
                          const char *cmd,
                          const char *working_dir,
                          const char *job_name)
{
    return moo_cmd_view_run_command_full (view, cmd, NULL, working_dir, NULL, job_name);
}


static char *
make_display_cmd_line (const char *cmd,
                       const char *display_cmd,
                       const char *working_dir)
{
    char *display_cmd_line;

    display_cmd = display_cmd ? display_cmd : cmd;

    if (!working_dir)
    {
        display_cmd_line = g_strdup (display_cmd);
    }
    else
    {
        char *display_dir = g_filename_display_name (working_dir);
        display_cmd_line = g_strdup_printf ("[%s] %s", display_dir, display_cmd);
        g_free (display_dir);
    }

    return display_cmd_line;
}

gboolean
moo_cmd_view_run_command_full (MooCmdView  *view,
                               const char  *cmd,
                               const char  *display_cmd,
                               const char  *working_dir,
                               char       **envp,
                               const char  *job_name)
{
    GError *error = NULL;
    char **argv = NULL;
    gboolean result = FALSE;
    char *display_cmd_line;

    g_return_val_if_fail (MOO_IS_CMD_VIEW (view), FALSE);
    g_return_val_if_fail (cmd && cmd[0], FALSE);

    moo_cmd_view_abort_and_disconnect (view);

    display_cmd_line = make_display_cmd_line (cmd, display_cmd, working_dir);
    moo_line_view_write_line (MOO_LINE_VIEW (view),
                              display_cmd_line, -1,
                              view->priv->message_tag);
    g_free (display_cmd_line);

#ifdef __WIN32__
    if (!(argv = _moo_win32_lame_parse_cmd_line (cmd, &error)))
    {
        moo_line_view_write_line (MOO_LINE_VIEW (view),
                                  moo_error_message (error),
                                  -1, view->priv->error_tag);
        g_error_free (error);
        goto out;
    }
#else
    argv = g_new (char*, 4);
    argv[0] = g_strdup ("/bin/sh");
    argv[1] = g_strdup ("-c");
    argv[2] = g_strdup (cmd);
    argv[3] = NULL;
#endif

    view->priv->cmd = _moo_cmd_new (working_dir, argv, envp,
                                    (GSpawnFlags) (G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD),
                                    MOO_CMD_UTF8_OUTPUT,
                                    NULL, NULL,
                                    &error);

    if (!view->priv->cmd)
    {
        const char *message = moo_error_message (error);
        moo_line_view_write_line (MOO_LINE_VIEW (view), message, -1,
                                  view->priv->error_tag);
        if (error)
            g_error_free (error);
        goto out;
    }

    g_signal_connect (view->priv->cmd, "cmd-exit", G_CALLBACK (cmd_exit_cb), view);
    g_signal_connect (view->priv->cmd, "stdout-line", G_CALLBACK (stdout_line_cb), view);
    g_signal_connect (view->priv->cmd, "stderr-line", G_CALLBACK (stderr_line_cb), view);

    result = TRUE;
    g_signal_emit (view, signals[JOB_STARTED], 0, job_name);

    if (view->priv->filter)
        moo_output_filter_cmd_start (view->priv->filter, working_dir);

out:
    g_strfreev (argv);
    return result;
}


gboolean
moo_cmd_view_running (MooCmdView *view)
{
    g_return_val_if_fail (MOO_IS_CMD_VIEW (view), FALSE);
    return view->priv->cmd != NULL;
}


#ifndef __WIN32__
static char *
get_signal_message (int sig)
{
    if (sig == SIGSEGV)
        return g_strdup ("*** Aborted. Segmentation fault ***");
    else
        return g_strdup ("*** Aborted ***");
}

static gboolean
moo_cmd_view_cmd_exit (MooCmdView *view,
                       int         status)
{
    if (view->priv->filter && moo_output_filter_cmd_exit (view->priv->filter, status))
        return TRUE;

    if (WIFEXITED (status))
    {
        guint8 exit_code = WEXITSTATUS (status);

        if (!exit_code)
        {
            moo_line_view_write_line (MOO_LINE_VIEW (view),
                                      "*** Done ***", -1,
                                      view->priv->message_tag);
        }
        else
        {
            char *msg;

            if (exit_code > 128)
                msg = get_signal_message (exit_code - 128);
            else
                msg = g_strdup_printf ("*** Exited with status %d ***",
                                       exit_code);

            moo_line_view_write_line (MOO_LINE_VIEW (view),
                                      msg, -1,
                                      view->priv->error_tag);
            g_free (msg);
        }
    }
#ifdef WCOREDUMP
    else if (WCOREDUMP (status))
    {
        moo_line_view_write_line (MOO_LINE_VIEW (view),
                                  "*** Dumped core ***", -1,
                                  view->priv->error_tag);
    }
#endif
    else if (WIFSIGNALED (status))
    {
        char *msg = get_signal_message (WTERMSIG (status));
        moo_line_view_write_line (MOO_LINE_VIEW (view), msg, -1,
                                  view->priv->error_tag);
        g_free (msg);
    }
    else
    {
        moo_line_view_write_line (MOO_LINE_VIEW (view),
                                  "*** ??? ***", -1,
                                  view->priv->error_tag);
    }

    return FALSE;
}
#else /* __WIN32__ */
static gboolean
moo_cmd_view_cmd_exit (MooCmdView *view,
                       int         status)
{
    if (view->priv->filter && moo_output_filter_cmd_exit (view->priv->filter, status))
        return TRUE;

    if (!status)
    {
        moo_line_view_write_line (MOO_LINE_VIEW (view),
                                  "*** Done ***", -1,
                                  view->priv->message_tag);
    }
    else
    {
        char *msg = g_strdup_printf ("*** Exited with status %d ***", status);
        moo_line_view_write_line (MOO_LINE_VIEW (view),
                                  msg, -1,
                                  view->priv->error_tag);
        g_free (msg);
    }

    return FALSE;
}
#endif /* __WIN32__ */


static gboolean
moo_cmd_view_abort_real (MooCmdView *view)
{
    if (view->priv->cmd)
        _moo_cmd_abort (view->priv->cmd);
    return TRUE;
}


void
moo_cmd_view_abort (MooCmdView *view)
{
    gboolean handled;
    g_return_if_fail (MOO_IS_CMD_VIEW (view));
    g_signal_emit (view, signals[ABORT], 0, &handled);
}


/**
 * moo_cmd_view_write_with_filter:
 *
 * @view:
 * @text: (type const-utf8)
 * @error: (allow-none) (default FALSE)
 */
void
moo_cmd_view_write_with_filter (MooCmdView *view,
                                const char *text,
                                gboolean    error)
{
    char **lines;
    char **p;
    
    lines = moo_splitlines (text);

    for (p = lines; p && *p; ++p)
    {
        if (error)
            moo_cmd_view_stderr_line (view, *p);
        else
            moo_cmd_view_stdout_line (view, *p);
    }

    g_strfreev (lines);
}


static gboolean
moo_cmd_view_stdout_line (MooCmdView *view,
                          const char *line)
{
    if (view->priv->filter && moo_output_filter_stdout_line (view->priv->filter, line))
        return TRUE;

    moo_line_view_write_line (MOO_LINE_VIEW (view), line, -1,
                              view->priv->stdout_tag);

    return FALSE;
}


static gboolean
moo_cmd_view_stderr_line (MooCmdView *view,
                          const char *line)
{
    if (view->priv->filter && moo_output_filter_stderr_line (view->priv->filter, line))
        return TRUE;

    moo_line_view_write_line (MOO_LINE_VIEW (view), line, -1,
                              view->priv->stderr_tag);

    return FALSE;
}
