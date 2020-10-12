/*
 *   moocommand-exe.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "moocommand-exe.h"
#include "mooedit/mooeditor.h"
#include "mooedit/mooedit-script.h"
#include "../support/moocmdview.h"
#include "../support/mooeditwindowoutput.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-script.h"
#include "mooutils/mooutils-debug.h"
#include "mooutils/moospawn.h"
#include "mooutils/mootype-macros.h"
#include "plugins/usertools/mooedittools-exe-gxml.h"
#include <gtk/gtk.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef __WIN32__
#include <io.h>
#endif

#ifndef __WIN32__
#define RUN_CMD_FLAGS ((GSpawnFlags)0)
#define SCRIPT_EXTENSION ".sh"
#else
#define RUN_CMD_FLAGS G_SPAWN_SEARCH_PATH
#define SCRIPT_EXTENSION ".bat"
#endif

#define MOO_COMMAND_EXE_MAX_INPUT       (MOO_COMMAND_EXE_INPUT_DOC_COPY + 1)
#define MOO_COMMAND_EXE_MAX_OUTPUT      (MOO_COMMAND_EXE_OUTPUT_NEW_DOC + 1)
#define MOO_COMMAND_EXE_INPUT_DEFAULT   MOO_COMMAND_EXE_INPUT_NONE
#define MOO_COMMAND_EXE_OUTPUT_DEFAULT  MOO_COMMAND_EXE_OUTPUT_NONE

typedef enum
{
    MOO_COMMAND_EXE_INPUT_NONE,
    MOO_COMMAND_EXE_INPUT_LINES,
    MOO_COMMAND_EXE_INPUT_SELECTION,
    MOO_COMMAND_EXE_INPUT_DOC,
    MOO_COMMAND_EXE_INPUT_DOC_COPY,
} MooCommandExeInput;

typedef enum
{
    MOO_COMMAND_EXE_OUTPUT_NONE,
    MOO_COMMAND_EXE_OUTPUT_NONE_ASYNC,
#ifdef __WIN32__
    MOO_COMMAND_EXE_OUTPUT_CONSOLE,
#endif
    MOO_COMMAND_EXE_OUTPUT_PANE,
    MOO_COMMAND_EXE_OUTPUT_INSERT,
    MOO_COMMAND_EXE_OUTPUT_NEW_DOC
} MooCommandExeOutput;

static const char *output_strings[] = {
    "none",
    "async",
#ifdef __WIN32__
    "console",
#endif
    "pane",
    "insert",
    "new-doc"
};

static const char *output_names[] = {
    /* Translators: this is an action for output of a shell command, remove the part before and including | */
    N_("Output|None"),
    /* Translators: this is an action for output of a shell command, remove the part before and including | */
    N_("Output|None, asynchronous"),
#ifdef __WIN32__
    /* Translators: this is an action for output of a shell command, remove the part before and including | */
    N_("Output|Console"),
#endif
    /* Translators: this is an action for output of a shell command, remove the part before and including | */
    N_("Output|Output pane"),
    /* Translators: this is an action for output of a shell command, remove the part before and including | */
    N_("Output|Insert into the document"),
    /* Translators: this is an action for output of a shell command, remove the part before and including | */
    N_("Output|New document")
};

enum {
    COLUMN_NAME,
    COLUMN_ID
};

enum {
    KEY_INPUT,
    KEY_OUTPUT,
    KEY_FILTER,
    N_KEYS
};

static const char *data_keys[N_KEYS+1] = {
    "input", "output", "filter", NULL
};

struct _MooCommandExePrivate {
    char *cmd_line;
    MooCommandExeInput input;
    MooCommandExeOutput output;
    char *filter;
};

G_DEFINE_TYPE (MooCommandExe, _moo_command_exe, MOO_TYPE_COMMAND)

typedef MooCommandFactory MooCommandFactoryExe;
typedef MooCommandFactoryClass MooCommandFactoryExeClass;
MOO_DEFINE_TYPE_STATIC (MooCommandFactoryExe, _moo_command_factory_exe, MOO_TYPE_COMMAND_FACTORY)


static void
_moo_command_exe_init (MooCommandExe *cmd)
{
    cmd->priv = G_TYPE_INSTANCE_GET_PRIVATE (cmd,
                                             MOO_TYPE_COMMAND_EXE,
                                             MooCommandExePrivate);
}


static void
moo_command_exe_finalize (GObject *object)
{
    MooCommandExe *cmd = MOO_COMMAND_EXE (object);

    g_free (cmd->priv->filter);
    g_free (cmd->priv->cmd_line);

    G_OBJECT_CLASS (_moo_command_exe_parent_class)->finalize (object);
}


static char *
get_lines (MooEdit  *doc,
           gboolean  select_them)
{
    GtkTextBuffer *buffer;
    GtkTextIter start, end;

    buffer = moo_edit_get_buffer (doc);
    gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

    gtk_text_iter_set_line_offset (&start, 0);

    if (!gtk_text_iter_starts_line (&end) || gtk_text_iter_equal (&start, &end))
        gtk_text_iter_forward_line (&end);

    if (select_them)
        gtk_text_buffer_select_range (buffer, &end, &start);

    return gtk_text_buffer_get_slice (buffer, &start, &end, TRUE);
}

static char *
save_input_file (MooEdit *doc)
{
    MooSaveInfo *info;
    GError *error = NULL;
    char *file;

    file = moo_tempnam (NULL);
    info = moo_save_info_new (file, moo_edit_get_encoding (doc));

    if (!moo_edit_save_copy (doc, info, &error))
    {
        g_error_free (error);
        g_free (file);
        file = NULL;
    }

    moo_save_info_free (info);
    return file;
}

static char *
get_input (MooCommandExe       *cmd,
           MooCommandContext   *ctx,
           gboolean             select_it)
{
    MooEdit *doc = moo_command_context_get_doc (ctx);

    g_return_val_if_fail (cmd->priv->input == MOO_COMMAND_EXE_INPUT_NONE || doc != NULL, NULL);

    switch (cmd->priv->input)
    {
        case MOO_COMMAND_EXE_INPUT_NONE:
        case MOO_COMMAND_EXE_INPUT_DOC_COPY:
            return NULL;
        case MOO_COMMAND_EXE_INPUT_LINES:
            return get_lines (doc, select_it);
        case MOO_COMMAND_EXE_INPUT_SELECTION:
            return moo_edit_get_selected_text (doc);
        case MOO_COMMAND_EXE_INPUT_DOC:
            return moo_edit_get_text (doc, NULL, NULL);
    }

    g_return_val_if_reached (NULL);
}


static char *
save_temp (const char *data,
           gssize      length)
{
    GError *error = NULL;
    char *filename;

    filename = moo_tempnam (NULL);

    /* XXX */
    if (!_moo_save_file_utf8 (filename, data, length, &error))
    {
        g_critical ("%s", error->message);
        g_error_free (error);
        g_free (filename);
        return NULL;
    }

    return filename;
}

static char *
make_cmd (const char *base_cmd_line,
          const char *input)
{
    char *cmd_line = NULL;
    char *tmp_file = NULL;
    gsize input_len;

    input_len = input ? strlen (input) : 0;

    if (!input_len)
        return g_strdup (base_cmd_line);

    tmp_file = save_temp (input, input_len);

    if (tmp_file)
    {
        cmd_line = g_strdup_printf ("( %s ) < '%s' ; rm '%s'",
                                    base_cmd_line, tmp_file, tmp_file);
        g_free (tmp_file);
    }

    return cmd_line;
}

static char *
make_cmd_line (MooCommandExe     *cmd,
               MooCommandContext *ctx,
               gboolean           select_input)
{
    char *input;
    char *cmd_line;

    input = get_input (cmd, ctx, select_input);
    cmd_line = make_cmd (cmd->priv->cmd_line, input);

    g_free (input);
    return cmd_line;
}

static char **
make_argv (const char  *cmd_line,
           G_GNUC_UNUSED GError **error)
{
    char **argv = NULL;

#ifndef __WIN32__
    argv = g_new (char*, 4);
    argv[0] = g_strdup ("/bin/sh");
    argv[1] = g_strdup ("-c");
    argv[2] = g_strdup (cmd_line);
    argv[3] = NULL;
#else
    argv = _moo_win32_lame_parse_cmd_line (cmd_line, error);
#endif

    return argv;
}


static void
run_in_pane (MooEditWindow     *window,
             MooEdit           *doc,
             const char        *filter_id,
             const char        *cmd_line,
             const char        *display_cmd_line,
             const char        *working_dir,
             char             **envp)
{
    GtkWidget *output;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (cmd_line != NULL);

    output = moo_edit_window_get_output (window);
    g_return_if_fail (output != NULL);

    /* XXX */
    if (!moo_cmd_view_running (MOO_CMD_VIEW (output)))
    {
        MooOutputFilter *filter = NULL;

        moo_line_view_clear (MOO_LINE_VIEW (output));
        moo_edit_window_present_output (window);
        gtk_widget_grab_focus (output);

        if (!filter_id)
            filter_id = "default";

        filter = moo_command_filter_create (filter_id);

        if (filter)
        {
            char *fn = doc ? moo_edit_get_filename (doc) : NULL;
            moo_output_filter_set_active_file (filter, fn);
            g_free (fn);
        }

        moo_cmd_view_set_filter (MOO_CMD_VIEW (output), filter);

        moo_cmd_view_run_command_full (MOO_CMD_VIEW (output),
                                       cmd_line, display_cmd_line,
                                       working_dir, envp, "Command");

        if (filter)
            g_object_unref (filter);
    }
}

static char *
get_working_dir_for_doc (MooEditWindow *window,
                         MooEdit       *doc)
{
    char *filename = NULL;
    char *dirname = NULL;

    if (!doc && window)
        doc = moo_edit_window_get_active_doc (window);

    if (doc)
        filename = moo_edit_get_filename (doc);

    if (filename)
        dirname = g_path_get_dirname (filename);

    g_free (filename);
    return dirname;
}

void
_moo_edit_run_in_pane (const char     *cmd_line,
                       const char     *working_dir,
                       char          **envp,
                       MooEditWindow  *window,
                       MooEdit        *doc)
{
    char *freeme = NULL;

    if (!working_dir || !working_dir[0])
        working_dir = freeme = get_working_dir_for_doc (window, doc);

    run_in_pane (window, NULL, NULL, cmd_line, NULL, working_dir, envp);

    g_free (freeme);
}

static void
run_command_in_pane (MooCommandExe     *cmd,
                     MooCommandContext *ctx,
                     const char        *working_dir,
                     char             **envp)
{
    MooEdit *doc;
    MooEditWindow *window;
    char *cmd_line;

    doc = moo_command_context_get_doc (ctx);
    window = moo_command_context_get_window (ctx);

    g_return_if_fail (cmd->priv->input == MOO_COMMAND_EXE_INPUT_NONE || doc != NULL);
    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));

    cmd_line = make_cmd_line (cmd, ctx, FALSE);
    g_return_if_fail (cmd_line != NULL);

    run_in_pane (window, doc, cmd->priv->filter, cmd_line,
                 cmd->priv->cmd_line, working_dir, envp);

    g_free (cmd_line);
}


static void
insert_text (MooEdit    *doc,
             const char *text,
             gboolean    replace_doc)
{
    GtkTextBuffer *buffer;
    GtkTextIter start, end;

    buffer = moo_edit_get_buffer (doc);

    gtk_text_buffer_begin_user_action (buffer);

    if (replace_doc)
        gtk_text_buffer_get_bounds (buffer, &start, &end);
    else
        gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

    gtk_text_buffer_delete (buffer, &start, &end);
    gtk_text_buffer_insert (buffer, &start, text, -1);

    gtk_text_buffer_end_user_action (buffer);
}


static void
get_extension (const char *string,
               char      **base,
               char      **ext)
{
    char *dot;

    g_return_if_fail (string != NULL);

    dot = strrchr ((char*) string, '.');

    if (dot)
    {
        *base = g_strndup (string, dot - string);
        *ext = g_strdup (dot);
    }
    else
    {
        *base = g_strdup (string);
        *ext = g_strdup ("");
    }
}

static void
create_environment (MooCommandExe     *cmd,
                    MooCommandContext *ctx,
                    char             **working_dir,
                    char            ***envp)
{
    GPtrArray *array;
    MooEdit *doc = moo_command_context_get_doc (ctx);

    array = g_ptr_array_new ();

    if (doc && !moo_edit_is_untitled (doc))
    {
        char *filename, *basename;
        char *dirname, *base = NULL, *extension = NULL;

        filename = moo_edit_get_filename (doc);
        basename = filename ? g_path_get_basename (filename) : NULL;
        dirname = g_path_get_dirname (filename);
        get_extension (basename, &base, &extension);

        g_ptr_array_add (array, g_strdup_printf ("DOC=%s", basename));
        g_ptr_array_add (array, g_strdup_printf ("DOC_DIR=%s", dirname));
        g_ptr_array_add (array, g_strdup_printf ("DOC_BASE=%s", base));
        g_ptr_array_add (array, g_strdup_printf ("DOC_EXT=%s", extension));
        g_ptr_array_add (array, g_strdup_printf ("DOC_PATH=%s", filename));

        g_free (basename);
        g_free (dirname);
        g_free (base);
        g_free (extension);
        g_free (filename);
    }

    if (doc)
    {
        MooEditView *view = moo_edit_get_view (doc);
        int line = moo_text_view_get_cursor_line (GTK_TEXT_VIEW (view));
        g_ptr_array_add (array, g_strdup_printf ("LINE0=%d", line));
        g_ptr_array_add (array, g_strdup_printf ("LINE=%d", line + 1));
    }

    {
        char *data_dir = moo_get_user_data_dir ();
        char *temp_dir = moo_tempdir ();
        g_ptr_array_add (array, g_strdup_printf ("DATA_DIR=%s", data_dir));
        g_ptr_array_add (array, g_strdup_printf ("TEMP_DIR=%s", temp_dir));
        g_free (data_dir);
        g_free (temp_dir);
    }

    g_ptr_array_add (array, g_strdup_printf ("APP_PID=%s", _moo_get_pid_string ()));
    g_ptr_array_add (array, g_strdup_printf ("MEDIT_PID=%s", _moo_get_pid_string ()));

    if (cmd->priv->input == MOO_COMMAND_EXE_INPUT_DOC_COPY)
    {
        char *input_file = save_input_file (doc);
        if (input_file)
            g_ptr_array_add (array, g_strdup_printf ("INPUT_FILE=%s", input_file));
        g_free (input_file);
    }

    g_ptr_array_add (array, NULL);
    *envp = (char**) g_ptr_array_free (array, FALSE);
    *working_dir = NULL;

    if (doc && !moo_edit_is_untitled (doc))
    {
        char *filename = moo_edit_get_filename (doc);
        *working_dir = g_path_get_dirname (filename);
        g_free (filename);
    }
}


static gboolean
run_sync (const char  *base_cmd_line,
          const char  *working_dir,
          char       **envp,
          const char  *input,
          int         *exit_status,
          char       **output,
          char       **output_err)
{
    GError *error = NULL;
    gboolean result = FALSE;
    GSpawnFlags flags = (GSpawnFlags) (RUN_CMD_FLAGS | MOO_SPAWN_WIN32_HIDDEN_CONSOLE);
    char **argv;
    char **real_env;
    char *cmd_line;

    g_return_val_if_fail (base_cmd_line != NULL, FALSE);

    cmd_line = make_cmd (base_cmd_line, input);
    argv = make_argv (cmd_line, &error);

    if (argv)
    {
        real_env = _moo_env_add (envp);
        result = g_spawn_sync (working_dir, argv, real_env, flags,
                               NULL, NULL, output, output_err, exit_status,
                               &error);
        g_strfreev (real_env);
    }

    if (!result)
    {
        g_message ("%s: could not run command: %s (command line was '%s')",
                   G_STRFUNC, error->message, cmd_line);
        g_error_free (error);
    }

    g_strfreev (argv);
    g_free (cmd_line);
    return result;
}

void
_moo_edit_run_sync (const char    *cmd_line,
                    const char    *working_dir,
                    char         **envp,
                    MooEditWindow *window,
                    MooEdit       *doc,
                    const char    *input,
                    int           *exit_status,
                    char         **output,
                    char         **output_err)
{
    char *freeme = NULL;

    if (!working_dir || !working_dir[0])
        working_dir = freeme = get_working_dir_for_doc (window, doc);

    run_sync (cmd_line, working_dir, envp, input, exit_status, output, output_err);

    g_free (freeme);
}

static gboolean
run_command (MooCommandExe     *cmd,
             MooCommandContext *ctx,
             const char        *working_dir,
             char             **envp,
             char             **output,
             gboolean           select_input)
{
    gboolean result;
    char *input;

    input = get_input (cmd, ctx, select_input);
    result = run_sync (cmd->priv->cmd_line, working_dir, envp,
                       input, NULL, output, NULL);

    g_free (input);
    return result;
}


static gboolean
run_async (const char     *cmd_line,
           const char     *working_dir,
           char          **envp,
           MooEditWindow  *window)
{
    GError *error = NULL;
    gboolean result = FALSE;
    char **real_env;
    GdkScreen *screen = NULL;
    GSpawnFlags flags = RUN_CMD_FLAGS;
    char **argv;

    if (!cmd_line)
        return FALSE;

    if (window && gtk_widget_has_screen (GTK_WIDGET (window)))
        screen = gtk_widget_get_screen (GTK_WIDGET (window));

    argv = make_argv (cmd_line, &error);

    if (argv)
    {
        _moo_message ("Launching:\n%s\n", cmd_line);
        real_env = _moo_env_add (envp);

        if (screen)
        {
            result = gdk_spawn_on_screen (screen, working_dir, (char**) argv, real_env,
                                          flags, NULL, NULL, NULL, &error);
        }
        else
        {
            result = g_spawn_async (working_dir, (char**) argv, real_env,
                                    flags, NULL, NULL, NULL, &error);
        }

        g_strfreev (real_env);
    }

    if (!result)
    {
        g_message ("%s: could not run command: %s (command line was '%s')",
                   G_STRFUNC, error->message, cmd_line);
        g_error_free (error);
    }

    g_strfreev (argv);
    return result;
}

void
_moo_edit_run_async (const char     *cmd_line,
                     const char     *working_dir,
                     char          **envp,
                     MooEditWindow  *window,
                     MooEdit        *doc)
{
    char *freeme = NULL;

    if (!working_dir || !working_dir[0])
        working_dir = freeme = get_working_dir_for_doc (window, doc);

    run_async (cmd_line, working_dir, envp, window);

    g_free (freeme);
}

static gboolean
run_command_async (MooCommandExe     *cmd,
                   MooCommandContext *ctx,
                   const char        *working_dir,
                   char             **envp)
{
    gboolean result;
    char *cmd_line;
    MooEditWindow *window;

    cmd_line = make_cmd_line (cmd, ctx, FALSE);

    if (!cmd_line)
        return FALSE;

    window = moo_command_context_get_window (ctx);
    result = run_async (cmd_line, working_dir, envp, window);

    g_free (cmd_line);
    return result;
}


static void
moo_command_exe_run (MooCommand        *cmd_base,
                     MooCommandContext *ctx)
{
    MooEdit *doc;
    MooCommandExe *cmd = MOO_COMMAND_EXE (cmd_base);
    char **envp;
    char *output = NULL;
    char *working_dir;

    create_environment (cmd, ctx, &working_dir, &envp);

    switch (cmd->priv->output)
    {
        case MOO_COMMAND_EXE_OUTPUT_PANE:
            run_command_in_pane (cmd, ctx, working_dir, envp);
            goto out;
        case MOO_COMMAND_EXE_OUTPUT_NONE:
            run_command (cmd, ctx, working_dir, envp, NULL, FALSE);
            goto out;
        case MOO_COMMAND_EXE_OUTPUT_NONE_ASYNC:
            run_command_async (cmd, ctx, working_dir, envp);
            goto out;
#ifdef __WIN32__
        case MOO_COMMAND_EXE_OUTPUT_CONSOLE:
            run_command_async (cmd, ctx, working_dir, envp);
            goto out;
#endif
        default:
            break;
    }

    if (!run_command (cmd, ctx, working_dir, envp, &output,
                      cmd->priv->output == MOO_COMMAND_EXE_OUTPUT_INSERT))
        goto out;

    if (cmd->priv->output == MOO_COMMAND_EXE_OUTPUT_INSERT)
    {
        doc = moo_command_context_get_doc (ctx);
        g_return_if_fail (MOO_IS_EDIT (doc));
    }
    else
    {
        MooEditWindow *window = moo_command_context_get_window (ctx);
        doc = moo_editor_new_doc (moo_editor_instance (), window);
        g_return_if_fail (MOO_IS_EDIT (doc));
    }

    insert_text (doc, output, cmd->priv->input == MOO_COMMAND_EXE_INPUT_DOC);

out:
    g_free (working_dir);
    g_strfreev (envp);
    g_free (output);
}


static gboolean
moo_command_exe_check_sensitive (MooCommand *cmd_base,
                                 MooEdit    *doc)
{
    MooCommandExe *cmd = MOO_COMMAND_EXE (cmd_base);
    MooCommandOptions options;

    options = cmd_base->options;

    switch (cmd->priv->input)
    {
        case MOO_COMMAND_EXE_INPUT_NONE:
            break;
        case MOO_COMMAND_EXE_INPUT_LINES:
        case MOO_COMMAND_EXE_INPUT_SELECTION:
        case MOO_COMMAND_EXE_INPUT_DOC:
        case MOO_COMMAND_EXE_INPUT_DOC_COPY:
            options |= MOO_COMMAND_NEED_DOC;
            break;
    }

    switch (cmd->priv->output)
    {
        case MOO_COMMAND_EXE_OUTPUT_NONE:
        case MOO_COMMAND_EXE_OUTPUT_NONE_ASYNC:
#ifdef __WIN32__
        case MOO_COMMAND_EXE_OUTPUT_CONSOLE:
#endif
        case MOO_COMMAND_EXE_OUTPUT_NEW_DOC:
        case MOO_COMMAND_EXE_OUTPUT_PANE:
            break;
        case MOO_COMMAND_EXE_OUTPUT_INSERT:
            options |= MOO_COMMAND_NEED_DOC;
            break;
    }

    moo_command_set_options (cmd_base, options);

    return MOO_COMMAND_CLASS (_moo_command_exe_parent_class)->check_sensitive (cmd_base, doc);
}


static void
_moo_command_exe_class_init (MooCommandExeClass *klass)
{
    MooCommandFactory *factory;

    G_OBJECT_CLASS(klass)->finalize = moo_command_exe_finalize;
    MOO_COMMAND_CLASS(klass)->run = moo_command_exe_run;
    MOO_COMMAND_CLASS(klass)->check_sensitive = moo_command_exe_check_sensitive;

    g_type_class_add_private (klass, sizeof (MooCommandExePrivate));

    factory = (MooCommandFactory*) g_object_new (_moo_command_factory_exe_get_type (), (const char*) NULL);
    moo_command_factory_register ("exe", _("Shell command"),
                                  factory, (char**) data_keys,
                                  SCRIPT_EXTENSION);
    g_object_unref (factory);
}


static void
_moo_command_factory_exe_init (G_GNUC_UNUSED MooCommandFactoryExe *factory)
{
}


static MooCommand *
_moo_command_exe_new (const char         *cmd_line,
                      MooCommandOptions   options,
                      MooCommandExeInput  input,
                      MooCommandExeOutput output,
                      const char         *filter)
{
    MooCommandExe *cmd;

    g_return_val_if_fail (cmd_line && cmd_line[0], NULL);
    g_return_val_if_fail (input < MOO_COMMAND_EXE_MAX_INPUT, NULL);
    g_return_val_if_fail (output < MOO_COMMAND_EXE_MAX_OUTPUT, NULL);

    cmd = (MooCommandExe*) g_object_new (MOO_TYPE_COMMAND_EXE, "options", options, (const char*) NULL);

    cmd->priv->cmd_line = g_strdup (cmd_line);
    cmd->priv->input = input;
    cmd->priv->output = output;
    cmd->priv->filter = g_strdup (filter);

    return MOO_COMMAND (cmd);
}


static gboolean
parse_input (const char *string,
             int        *input)
{
    *input = MOO_COMMAND_EXE_INPUT_DEFAULT;

    if (!string || !string[0])
        return TRUE;

    if (!strcmp (string, "none"))
        *input = MOO_COMMAND_EXE_INPUT_NONE;
    else if (!strcmp (string, "lines"))
        *input = MOO_COMMAND_EXE_INPUT_LINES;
    else if (!strcmp (string, "selection"))
        *input = MOO_COMMAND_EXE_INPUT_SELECTION;
    else if (!strcmp (string, "doc"))
        *input = MOO_COMMAND_EXE_INPUT_DOC;
    else if (!strcmp (string, "doc-copy"))
        *input = MOO_COMMAND_EXE_INPUT_DOC_COPY;
    else
    {
        g_warning ("unknown input type %s", string);
        return FALSE;
    }

    return TRUE;
}

static gboolean
parse_output (const char *string,
              int        *output)
{
    *output = MOO_COMMAND_EXE_OUTPUT_DEFAULT;

    if (!string || !string[0])
        return TRUE;

    if (!strcmp (string, "none"))
        *output = MOO_COMMAND_EXE_OUTPUT_NONE;
    else if (!strcmp (string, "async"))
        *output = MOO_COMMAND_EXE_OUTPUT_NONE_ASYNC;
#ifdef __WIN32__
    else if (!strcmp (string, "console"))
        *output = MOO_COMMAND_EXE_OUTPUT_CONSOLE;
#endif
    else if (!strcmp (string, "pane"))
        *output = MOO_COMMAND_EXE_OUTPUT_PANE;
    else if (!strcmp (string, "insert"))
        *output = MOO_COMMAND_EXE_OUTPUT_INSERT;
    else if (!strcmp (string, "new-doc") ||
             !strcmp (string, "new_doc") ||
             !strcmp (string, "new"))
        *output = MOO_COMMAND_EXE_OUTPUT_NEW_DOC;
    else
    {
        g_warning ("unknown output type %s", string);
        return FALSE;
    }

    return TRUE;
}

static MooCommand *
unx_factory_create_command (G_GNUC_UNUSED MooCommandFactory *factory,
                            MooCommandData *data,
                            const char     *options)
{
    MooCommand *cmd;
    const char *cmd_line;
    int input, output;

    cmd_line = moo_command_data_get_code (data);
    g_return_val_if_fail (cmd_line && *cmd_line, NULL);

    if (!parse_input (moo_command_data_get (data, KEY_INPUT), &input))
        return NULL;
    if (!parse_output (moo_command_data_get (data, KEY_OUTPUT), &output))
        return NULL;

    cmd = _moo_command_exe_new (cmd_line,
                                moo_parse_command_options (options),
                                (MooCommandExeInput) input, (MooCommandExeOutput) output,
                                moo_command_data_get (data, KEY_FILTER));
    g_return_val_if_fail (cmd != NULL, NULL);

    return cmd;
}


static void
init_combo (GtkComboBox *combo,
            const char **items,
            guint        n_items)
{
    GtkListStore *store;
    GtkCellRenderer *cell;
    guint i;

    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell, "text", 0, nullptr);

    store = gtk_list_store_new (1, G_TYPE_STRING);

    for (i = 0; i < n_items; ++i)
    {
        GtkTreeIter iter;
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, 0, Q_(items[i]), -1);
    }

    gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store));

    g_object_unref (store);
}

static void
init_filter_combo (GtkComboBox *combo)
{
    GtkListStore *store;
    GtkCellRenderer *cell;
    GtkTreeIter iter;
    GSList *ids;

    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
                                    "text", COLUMN_NAME, nullptr);

    store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

    ids = moo_command_filter_list ();

    while (ids)
    {
        const char *name;
        char *id;

        id = (char*) ids->data;
        ids = g_slist_delete_link (ids, ids);
        name = moo_command_filter_lookup (id);

        if (!name)
        {
            g_critical ("oops");
            continue;
        }

        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            COLUMN_ID, id,
                            COLUMN_NAME, name,
                            -1);

        g_free (id);
    }

    gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store));

    g_object_unref (store);
}

static void
set_filter_combo (GtkComboBox *combo,
                  const char  *id)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_combo_box_get_model (combo);

    if (!id)
    {
        gtk_combo_box_set_active (combo, 0);
        return;
    }

    if (gtk_tree_model_get_iter_first (model, &iter))
    {
        do {
            char *id_here;

            gtk_tree_model_get (model, &iter, COLUMN_ID, &id_here, -1);

            if (!strcmp (id_here, id))
            {
                gtk_combo_box_set_active_iter (combo, &iter);
                g_free (id_here);
                return;
            }

            g_free (id_here);
        }
        while (gtk_tree_model_iter_next (model, &iter));
    }

    g_warning ("unknown filter %s", id);
    gtk_combo_box_set_active (combo, -1);
}

static GtkWidget *
unx_factory_create_widget (G_GNUC_UNUSED MooCommandFactory *factory)
{
    ExePageXml *xml;

    const char *input_names[] = {
        /* Translators: this is a kind of input for a shell command, remove the part before and including | */
        N_("Input|None"),
        /* Translators: this is a kind of input for a shell command, remove the part before and including | */
        N_("Input|Selected lines"),
        /* Translators: this is a kind of input for a shell command, remove the part before and including | */
        N_("Input|Selection"),
        /* Translators: this is a kind of input for a shell command, remove the part before and including | */
        N_("Input|Whole document"),
        /* Translators: this is a kind of input for a shell command, remove the part before and including | */
        N_("Input|Document copy")
    };

    xml = exe_page_xml_new ();

    moo_text_view_set_font_from_string (xml->textview, "Monospace");
#ifndef __WIN32__
    moo_text_view_set_lang_by_id (xml->textview, "sh");
#else
    moo_text_view_set_lang_by_id (xml->textview, "dosbatch");
#endif

    init_combo (xml->input, input_names, G_N_ELEMENTS (input_names));
    init_combo (xml->output, output_names, G_N_ELEMENTS (output_names));
    init_filter_combo (xml->filter);

    return GTK_WIDGET (xml->ExePage);
}


static void
unx_factory_load_data (G_GNUC_UNUSED MooCommandFactory *factory,
                       GtkWidget      *page,
                       MooCommandData *data)
{
    GtkTextBuffer *buffer;
    const char *cmd_line;
    int index;
    ExePageXml *xml;

    g_return_if_fail (data != NULL);

    xml = exe_page_xml_get (page);
    g_return_if_fail (xml != NULL);

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (xml->textview));

    cmd_line = moo_command_data_get_code (data);
    gtk_text_buffer_set_text (buffer, MOO_NZS (cmd_line), -1);

    parse_input (moo_command_data_get (data, KEY_INPUT), &index);
    gtk_combo_box_set_active (xml->input, index);

    parse_output (moo_command_data_get (data, KEY_OUTPUT), &index);
    gtk_combo_box_set_active (xml->output, index);

    set_filter_combo (xml->filter, moo_command_data_get (data, KEY_FILTER));
}


static gboolean
unx_factory_save_data (G_GNUC_UNUSED MooCommandFactory *factory,
                       GtkWidget      *page,
                       MooCommandData *data)
{
    ExePageXml *xml;
    const char *cmd_line;
    char *new_cmd_line;
    gboolean changed = FALSE;
    int index, old_index;
    const char *input_strings[5] = { "none", "lines", "selection", "doc", "doc-copy" };

    xml = exe_page_xml_get (page);
    g_return_val_if_fail (xml != NULL, FALSE);

    new_cmd_line = moo_text_view_get_text (GTK_TEXT_VIEW (xml->textview));
    cmd_line = moo_command_data_get_code (data);

    if (!moo_str_equal (cmd_line, new_cmd_line))
    {
        moo_command_data_set_code (data, new_cmd_line);
        changed = TRUE;
    }

    index = gtk_combo_box_get_active (xml->input);
    parse_input (moo_command_data_get (data, KEY_INPUT), &old_index);
    g_assert (0 <= index && index < MOO_COMMAND_EXE_MAX_INPUT);
    if (index != old_index)
    {
        moo_command_data_set (data, KEY_INPUT, input_strings[index]);
        changed = TRUE;
    }

    index = gtk_combo_box_get_active (xml->output);
    parse_output (moo_command_data_get (data, KEY_OUTPUT), &old_index);
    g_assert (0 <= index && index < MOO_COMMAND_EXE_MAX_OUTPUT);
    if (index != old_index)
    {
        moo_command_data_set (data, KEY_OUTPUT, output_strings[index]);
        changed = TRUE;
    }

    if (index == MOO_COMMAND_EXE_OUTPUT_PANE)
    {
        const char *old_filter;
        char *new_filter = NULL;
        GtkComboBox *combo = xml->filter;
        GtkTreeIter iter;

        if (gtk_combo_box_get_active_iter (combo, &iter))
        {
            GtkTreeModel *model = gtk_combo_box_get_model (combo);
            gtk_tree_model_get (model, &iter, COLUMN_ID, &new_filter, -1);
        }

        old_filter = moo_command_data_get (data, KEY_FILTER);

        if (!old_filter)
            old_filter = "default";

        if (!moo_str_equal (old_filter, new_filter))
        {
            moo_command_data_set (data, KEY_FILTER, new_filter);
            changed = TRUE;
        }

        g_free (new_filter);
    }
    else
    {
        moo_command_data_set (data, KEY_FILTER, NULL);
    }

    g_free (new_cmd_line);
    return changed;
}


static void
_moo_command_factory_exe_class_init (MooCommandFactoryExeClass *klass)
{
    klass->create_command = unx_factory_create_command;
    klass->create_widget = unx_factory_create_widget;
    klass->load_data = unx_factory_load_data;
    klass->save_data = unx_factory_save_data;
}
