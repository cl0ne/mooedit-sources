/*
 *   moofind.c
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

#include "mooedit/mooplugin-macro.h"
#include "mooedit/mooedit-script.h"
#include "plugins/mooplugin-builtin.h"
#include "moofileview/moofileentry.h"
#include "support/moocmdview.h"
#include "mooedit/mooedit-accels.h"
#include "mooedit/mootextbuffer.h"
#include "mooutils/moostock.h"
#include "mooutils/moohistorycombo.h"
#include "mooutils/moodialogs.h"
#include "mooutils/mooi18n.h"
#include "mooutils/moohelp.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils-misc.h"
#include "plugins/moofind-gxml.h"
#include "plugins/moogrep-gxml.h"
#ifdef MOO_ENABLE_HELP
#include "moo-help-sections.h"
#endif
#include <gtk/gtk.h>
#include <string.h>
#ifndef __WIN32__
#include <sys/wait.h>
#endif
#include <signal.h>

#define FIND_PLUGIN_ID "Find"

#define GREP_SKIP_LIST_ID "FindPlugin/grep/skip"
#define FIND_SKIP_LIST_ID "FindPlugin/find/skip"

enum {
    CMD_GREP = 1,
    CMD_FIND
};

typedef struct {
    MooPlugin parent;
    guint ui_merge_id;
} FindPlugin;

typedef struct {
    MooWinPlugin parent;

    char *current_file;
    GtkWidget *grep_dialog;
    GrepXml *grep_xml;

    GtkWidget *find_dialog;
    FindXml *find_xml;

    MooEditWindow *window;
    MooCmdView *output;
    GtkTextTag *line_number_tag;
    GtkTextTag *match_tag;
    GtkTextTag *file_tag;
    GtkTextTag *error_tag;
    GtkTextTag *message_tag;
    guint match_count;
    int group_start_line;
    int group_end_line;
    int cmd;
} FindWindowPlugin;

#define WindowStuff FindWindowPlugin


typedef struct {
    char *filename;
    int line;
} FileLinePair;

static void         do_grep                 (MooEditWindow  *window,
                                             WindowStuff    *stuff);
static void         create_grep_dialog      (MooEditWindow  *window,
                                             WindowStuff    *stuff);
static void         init_grep_dialog        (MooEditWindow  *window,
                                             WindowStuff    *stuff);
static void         execute_grep            (const char     *pattern,
                                             const char     *glob,
                                             GSList         *dirs,
                                             const char     *skip_files,
                                             gboolean        case_sensitive,
                                             WindowStuff    *stuff);

static void         do_find                 (MooEditWindow  *window,
                                             WindowStuff    *stuff);
static void         create_find_dialog      (MooEditWindow  *window,
                                             WindowStuff    *stuff);
static void         init_find_dialog        (MooEditWindow  *window,
                                             WindowStuff    *stuff);
static void         execute_find            (const char     *pattern,
                                             const char     *dir,
                                             const char     *skip_files,
                                             WindowStuff    *stuff);

static gboolean     output_activate         (WindowStuff    *stuff,
                                             int             line);
static gboolean     command_exit            (MooLineView    *view,
                                             int             status,
                                             WindowStuff    *stuff);
static gboolean     process_line            (MooLineView    *view,
                                             const char     *line,
                                             WindowStuff    *stuff);


static void
find_in_files_cb (MooEditWindow *window)
{
    WindowStuff *stuff;
    int response;

    stuff = (WindowStuff*) moo_win_plugin_lookup (FIND_PLUGIN_ID, window);
    g_return_if_fail (stuff != NULL);

    if (!stuff->grep_dialog)
    {
        create_grep_dialog (window, stuff);
        g_return_if_fail (stuff->grep_dialog != NULL);
    }

    init_grep_dialog (window, stuff);

    response = gtk_dialog_run (GTK_DIALOG (stuff->grep_dialog));
    gtk_widget_hide (stuff->grep_dialog);

    if (response == GTK_RESPONSE_OK)
        do_grep (window, stuff);
}


G_GNUC_UNUSED static void
find_file_cb (MooEditWindow *window)
{
    WindowStuff *stuff;
    int response;

    stuff = (WindowStuff*) moo_win_plugin_lookup (FIND_PLUGIN_ID, window);
    g_return_if_fail (stuff != NULL);

    if (!stuff->find_dialog)
    {
        create_find_dialog (window, stuff);
        g_return_if_fail (stuff->find_dialog != NULL);
    }

    init_find_dialog (window, stuff);

    response = gtk_dialog_run (GTK_DIALOG (stuff->find_dialog));
    gtk_widget_hide (stuff->find_dialog);

    if (response == GTK_RESPONSE_OK)
        do_find (window, stuff);
}


static void
ensure_output (WindowStuff *stuff)
{
    GtkWidget *swin;
    MooPaneLabel *label;
    MooEditWindow *window = MOO_WIN_PLUGIN (stuff)->window;

    if (stuff->output)
        return;

    label = moo_pane_label_new (MOO_STOCK_FIND_IN_FILES, NULL,
                                _("Search Results"), _("Search Results"));
    stuff->output = MOO_CMD_VIEW (
        g_object_new (MOO_TYPE_CMD_VIEW,
                      "highlight-current-line", TRUE,
                      "highlight-current-line-unfocused", TRUE,
                      (const char*) NULL));

    moo_edit_window_add_stop_client (window, G_OBJECT (stuff->output));
    g_signal_connect_swapped (stuff->output, "activate",
                              G_CALLBACK (output_activate), stuff);

    stuff->line_number_tag =
            moo_line_view_create_tag (MOO_LINE_VIEW (stuff->output),
                                      NULL, "weight", PANGO_WEIGHT_BOLD, NULL);
    stuff->match_tag =
            moo_line_view_create_tag (MOO_LINE_VIEW (stuff->output), NULL,
                                      "foreground", "#0000C0", NULL);
    stuff->file_tag =
            moo_line_view_create_tag (MOO_LINE_VIEW (stuff->output), NULL,
                                      "foreground", "#008040", NULL);
    stuff->error_tag =
            moo_text_view_lookup_tag (MOO_TEXT_VIEW (stuff->output), "error");
    g_object_set (stuff->error_tag, "foreground", "#C00000", nullptr);

    stuff->message_tag =
            moo_text_view_lookup_tag (MOO_TEXT_VIEW (stuff->output), "message");

    g_signal_connect (stuff->output, "cmd-exit",
                      G_CALLBACK (command_exit), stuff);
    g_signal_connect (stuff->output, "stdout-line",
                      G_CALLBACK (process_line), stuff);

    swin = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swin),
                                         GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (swin), GTK_WIDGET (stuff->output));
    gtk_widget_show_all (swin);

    moo_edit_window_add_pane (window, FIND_PLUGIN_ID,
                              swin, label, MOO_PANE_POS_BOTTOM);
    moo_pane_label_free (label);
}


static gboolean
find_window_plugin_create (WindowStuff *stuff)
{
    stuff->window = MOO_WIN_PLUGIN (stuff)->window;
    return TRUE;
}


static gboolean
find_plugin_init (FindPlugin *plugin)
{
    MooWindowClass *klass = (MooWindowClass*) g_type_class_ref (MOO_TYPE_EDIT_WINDOW);
    MooEditor *editor = moo_editor_instance ();
    MooUiXml *xml = moo_editor_get_ui_xml (editor);

    g_return_val_if_fail (klass != NULL, FALSE);

    moo_window_class_new_action (klass, "FindInFiles", NULL,
                                 "display-name", _("Find In Files"),
                                 "label", _("Find In Files"),
                                 "tooltip", _("Find in files"),
                                 "default-accel", MOO_EDIT_ACCEL_FIND_IN_FILES,
                                 "stock-id", MOO_STOCK_FIND_IN_FILES,
                                 "closure-callback", find_in_files_cb,
                                 nullptr);

#ifndef __WIN32__
    moo_window_class_new_action (klass, "FindFile", NULL,
                                 "display-name", _("Find File"),
                                 "label", _("Find File"),
                                 "tooltip", _("Find file"),
                                 "stock-id", MOO_STOCK_FIND_FILE,
                                 "closure-callback", find_file_cb,
                                 nullptr);
#endif

    if (xml)
    {
        plugin->ui_merge_id = moo_ui_xml_new_merge_id (xml);
        moo_ui_xml_add_item (xml, plugin->ui_merge_id,
                             "Editor/Menubar/Search",
                             "FindInFiles", "FindInFiles", -1);
#ifndef __WIN32__
        moo_ui_xml_add_item (xml, plugin->ui_merge_id,
                             "Editor/Menubar/Search",
                             "FindFile", "FindFile", -1);
#endif
    }

    g_type_class_unref (klass);
    return TRUE;
}


static void
find_plugin_deinit (FindPlugin *plugin)
{
    MooWindowClass *klass = (MooWindowClass*) g_type_class_ref (MOO_TYPE_EDIT_WINDOW);
    MooEditor *editor = moo_editor_instance ();
    MooUiXml *xml = moo_editor_get_ui_xml (editor);

    moo_window_class_remove_action (klass, "FindInFiles");
#ifndef __WIN32__
    moo_window_class_remove_action (klass, "FindFile");
#endif

    if (plugin->ui_merge_id)
        moo_ui_xml_remove_ui (xml, plugin->ui_merge_id);
    plugin->ui_merge_id = 0;

    g_type_class_unref (klass);
}


static void
pattern_entry_changed (GtkEntry  *entry,
                       GtkDialog *dialog)
{
    const char *text = gtk_entry_get_text (entry);
    gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_OK,
                                       text[0] != 0);
}


static void
setup_file_combo (MooHistoryCombo *hist_combo)
{
    GtkWidget *entry;
    MooFileEntryCompletion *completion;

    g_object_set (hist_combo, "enable-completion", FALSE, nullptr);

    entry = MOO_COMBO (hist_combo)->entry;
    completion = MOO_FILE_ENTRY_COMPLETION (
        g_object_new (MOO_TYPE_FILE_ENTRY_COMPLETION,
                      "directories-only", TRUE,
                      "case-sensitive", !moo_os_win32 (),
                      "show-hidden", FALSE,
                      (const char*) NULL));
    _moo_file_entry_completion_set_entry (completion, GTK_ENTRY (entry));
    g_object_set_data_full (G_OBJECT (entry), "find-plugin-file-completion",
                            completion, g_object_unref);
}


static void
init_skip_list (void)
{
    MooHistoryList *list = moo_history_list_get (GREP_SKIP_LIST_ID);
    g_return_if_fail (list != NULL);
    if (moo_history_list_is_empty (list))
        moo_history_list_add (list, ".svn/;.hg/;.git/;CVS/;*~;*.bak;*.orig;*.rej");
}

static void
create_grep_dialog (MooEditWindow *window,
                    WindowStuff   *stuff)
{
    GtkWidget *pattern_entry;

    init_skip_list ();

    stuff->grep_xml = grep_xml_new_empty ();
    moo_glade_xml_set_property (stuff->grep_xml->xml, "pattern_combo", "history-list-id", "FindPlugin/grep/pattern");
    moo_glade_xml_set_property (stuff->grep_xml->xml, "glob_combo", "history-list-id", "FindPlugin/grep/glob");
    moo_glade_xml_set_property (stuff->grep_xml->xml, "dir_combo", "history-list-id", "FindPlugin/grep/dir");
    moo_glade_xml_set_property (stuff->grep_xml->xml, "skip_combo", "history-list-id", GREP_SKIP_LIST_ID);
    grep_xml_build (stuff->grep_xml);

    stuff->grep_dialog = GTK_WIDGET (stuff->grep_xml->Grep);
    g_return_if_fail (stuff->grep_dialog != NULL);

    gtk_window_set_default_size (GTK_WINDOW (stuff->grep_dialog), 400, -1);
    gtk_dialog_set_default_response (GTK_DIALOG (stuff->grep_dialog),
                                     GTK_RESPONSE_OK);
    gtk_dialog_set_response_sensitive (GTK_DIALOG (stuff->grep_dialog),
                                       GTK_RESPONSE_OK, FALSE);
    moo_window_set_parent (stuff->grep_dialog, GTK_WIDGET (window));
    gtk_dialog_set_alternative_button_order (GTK_DIALOG (stuff->grep_dialog),
                                             GTK_RESPONSE_OK,
                                             GTK_RESPONSE_CANCEL,
                                             -1);

#ifdef MOO_ENABLE_HELP
    moo_help_set_id (stuff->grep_dialog, HELP_SECTION_DIALOG_FIND_IN_FILES);
#endif
    moo_help_connect_keys (stuff->grep_dialog);

    g_signal_connect (stuff->grep_dialog, "delete-event",
                      G_CALLBACK (gtk_widget_hide_on_delete), NULL);

    pattern_entry = MOO_COMBO(stuff->grep_xml->pattern_combo)->entry;
    g_signal_connect (pattern_entry, "changed",
                      G_CALLBACK (pattern_entry_changed), stuff->grep_dialog);

    setup_file_combo (stuff->grep_xml->dir_combo);

    moo_combo_set_active (MOO_COMBO (stuff->grep_xml->skip_combo), 0);
}


static void
create_find_dialog (MooEditWindow *window,
                    WindowStuff   *stuff)
{
    GtkWidget *pattern_entry;

    stuff->find_xml = find_xml_new_empty ();
    moo_glade_xml_set_property (stuff->find_xml->xml, "pattern_combo", "history-list-id", "FindPlugin/find/pattern");
    moo_glade_xml_set_property (stuff->find_xml->xml, "dir_combo", "history-list-id", "FindPlugin/find/dir");
    moo_glade_xml_set_property (stuff->find_xml->xml, "skip_combo", "history-list-id", FIND_SKIP_LIST_ID);
    find_xml_build (stuff->find_xml);

    stuff->find_dialog = GTK_WIDGET (stuff->find_xml->Find);
    g_return_if_fail (stuff->find_dialog != NULL);

    gtk_window_set_default_size (GTK_WINDOW (stuff->find_dialog), 400, -1);
    gtk_dialog_set_default_response (GTK_DIALOG (stuff->find_dialog),
                                     GTK_RESPONSE_OK);
    gtk_dialog_set_response_sensitive (GTK_DIALOG (stuff->find_dialog),
                                       GTK_RESPONSE_OK, FALSE);
    gtk_dialog_set_alternative_button_order (GTK_DIALOG (stuff->find_dialog),
                                             GTK_RESPONSE_OK,
                                             GTK_RESPONSE_CANCEL,
                                             -1);
    moo_window_set_parent (stuff->find_dialog, GTK_WIDGET (window));

#ifdef MOO_ENABLE_HELP
    moo_help_set_id (stuff->find_dialog, HELP_SECTION_DIALOG_FIND_FILE);
#endif
    moo_help_connect_keys (stuff->find_dialog);

    g_signal_connect (stuff->find_dialog, "delete-event",
                      G_CALLBACK (gtk_widget_hide_on_delete), NULL);

    pattern_entry = MOO_COMBO(stuff->find_xml->pattern_combo)->entry;
    g_signal_connect (pattern_entry, "changed",
                      G_CALLBACK (pattern_entry_changed), stuff->find_dialog);

    setup_file_combo (stuff->find_xml->dir_combo);
}


static void
init_dir_entry (MooHistoryCombo *hist_combo,
                MooEdit         *doc)
{
    GtkWidget *entry;

    g_return_if_fail (hist_combo != NULL);

    entry = MOO_COMBO (hist_combo)->entry;

    if (!gtk_entry_get_text (GTK_ENTRY (entry))[0])
        moo_combo_set_active (MOO_COMBO (hist_combo), 0);

    if (!gtk_entry_get_text (GTK_ENTRY (entry))[0])
    {
        MooFileEntryCompletion *completion;
        GFile *file;
        char *filename;

        completion = (MooFileEntryCompletion*) g_object_get_data (G_OBJECT (entry), "find-plugin-file-completion");

        file = doc ? moo_edit_get_file (doc) : NULL;
        filename = file ? g_file_get_path (file) : NULL;

        if (filename)
        {
            char *dir = g_path_get_dirname (filename);
            _moo_file_entry_completion_set_path (completion, dir);
            g_free (dir);
        }
        else
        {
            _moo_file_entry_completion_set_path (completion, g_get_home_dir ());
        }

        g_free (filename);
        g_object_unref (file);
    }

#if 0
//     moo_history_list_remove (list, "CURRENT_DOC_DIR");
//
//     if (doc && moo_edit_get_filename (doc))
//     {
//         char *dir = g_path_get_dirname (moo_edit_get_filename (doc));
//         char *display = g_filename_display_name ();
//
//         moo_history_list_add_builtin (list, "CURRENT_DOC_DIR",
//                                       moo_edit_);
//     }
#endif
}


static void
init_grep_dialog (MooEditWindow *window,
                  WindowStuff   *stuff)
{
    MooEdit *doc;
    MooEditView *view;
    GtkWidget *pattern_entry, *glob_entry;

    pattern_entry = MOO_COMBO(stuff->grep_xml->pattern_combo)->entry;
    glob_entry = MOO_COMBO(stuff->grep_xml->glob_combo)->entry;

    view = moo_edit_window_get_active_view (window);
    doc = view ? moo_edit_view_get_doc (view) : NULL;

    if (doc)
    {
        char *sel = moo_edit_get_selected_text (doc);
        if (sel && *sel && !strchr (sel, '\n'))
            gtk_entry_set_text (GTK_ENTRY (pattern_entry), sel);
        g_free (sel);
    }

    init_dir_entry (stuff->grep_xml->dir_combo, doc);

    if (!gtk_entry_get_text(GTK_ENTRY (glob_entry))[0])
        gtk_entry_set_text (GTK_ENTRY (glob_entry), "*");

    gtk_widget_grab_focus (pattern_entry);
}


static void
init_find_dialog (MooEditWindow *window,
                  WindowStuff   *stuff)
{
    GtkWidget *pattern_entry;

    pattern_entry = MOO_COMBO(stuff->find_xml->pattern_combo)->entry;

    init_dir_entry (stuff->find_xml->dir_combo,
                    moo_edit_window_get_active_doc (window));

    gtk_widget_grab_focus (pattern_entry);
}


static GSList *
get_directories (MooHistoryCombo *combo)
{
    GtkWidget *entry;
    char *text;
    MooFileEntryCompletion *completion;
    char **strv, **p;
    GSList *list = NULL;

    g_return_val_if_fail (combo != NULL, NULL);
    entry = MOO_COMBO (combo)->entry;

    completion = (MooFileEntryCompletion*) g_object_get_data (G_OBJECT (entry), "find-plugin-file-completion");
    text = _moo_file_entry_completion_get_path (completion);

    if (!text)
        return NULL;

    moo_history_list_add (moo_history_combo_get_list (combo),
                          gtk_entry_get_text (GTK_ENTRY (entry)));

    strv = g_strsplit (text, ";", 0);

    for (p = strv; p && *p; ++p)
        if (*p)
            list = g_slist_prepend (list, _moo_normalize_file_path (*p));

    g_strfreev (strv);
    g_free (text);
    return g_slist_reverse (list);
}


static void
do_grep (MooEditWindow *window,
         WindowStuff   *stuff)
{
    GtkWidget *pane;
    MooHistoryCombo *pattern_combo, *skip_combo, *glob_combo;
    GtkWidget *pattern_entry, *glob_entry;
    GtkWidget *skip_entry;
    const char *pattern, *glob, *skip;
    gboolean case_sensitive;
    GSList *dirs;

    ensure_output (stuff);
    pane = moo_edit_window_get_pane (window, FIND_PLUGIN_ID);
    g_return_if_fail (pane != NULL);

    dirs = get_directories (stuff->grep_xml->dir_combo);
    g_return_if_fail (dirs != NULL);

    pattern_combo = stuff->grep_xml->pattern_combo;
    moo_history_combo_commit (pattern_combo);
    pattern_entry = MOO_COMBO (pattern_combo)->entry;
    pattern = gtk_entry_get_text (GTK_ENTRY (pattern_entry));

    glob_combo = stuff->grep_xml->glob_combo;
    moo_history_combo_commit (glob_combo);
    glob_entry = MOO_COMBO (glob_combo)->entry;
    glob = gtk_entry_get_text (GTK_ENTRY (glob_entry));

    skip_combo = stuff->grep_xml->skip_combo;
    skip_entry = MOO_COMBO (skip_combo)->entry;
    moo_history_combo_commit (skip_combo);
    skip = gtk_entry_get_text (GTK_ENTRY (skip_entry));

    case_sensitive = gtk_toggle_button_get_active (
        GTK_TOGGLE_BUTTON (stuff->grep_xml->case_sensitive_button));

    moo_line_view_clear (MOO_LINE_VIEW (stuff->output));
    moo_big_paned_present_pane (window->paned, pane);

    execute_grep (pattern, glob, dirs, skip,
                  case_sensitive, stuff);

    g_slist_foreach (dirs, (GFunc) g_free, NULL);
    g_slist_free (dirs);
}


static void
do_find (MooEditWindow *window,
         WindowStuff   *stuff)
{
    GtkWidget *pane;
    MooHistoryCombo *pattern_combo, *skip_combo;
    GtkWidget *pattern_entry, *skip_entry;
    const char *pattern, *skip;
    GSList *dirs;

    ensure_output (stuff);
    pane = moo_edit_window_get_pane (window, FIND_PLUGIN_ID);
    g_return_if_fail (pane != NULL);

    pattern_combo = stuff->find_xml->pattern_combo;
    moo_history_combo_commit (pattern_combo);
    pattern_entry = MOO_COMBO (pattern_combo)->entry;
    pattern = gtk_entry_get_text (GTK_ENTRY (pattern_entry));

    skip_combo = stuff->find_xml->skip_combo;
    moo_history_combo_commit (skip_combo);
    skip_entry = MOO_COMBO (skip_combo)->entry;
    skip = gtk_entry_get_text (GTK_ENTRY (skip_entry));

    dirs = get_directories (stuff->find_xml->dir_combo);
    g_return_if_fail (dirs != NULL);

    if (!dirs->next)
    {
        moo_line_view_clear (MOO_LINE_VIEW (stuff->output));
        moo_big_paned_present_pane (window->paned, pane);
        execute_find (pattern, (const char*) dirs->data, skip, stuff);
    }

    g_slist_foreach (dirs, (GFunc) g_free, NULL);
    g_slist_free (dirs);
}


static FileLinePair*
file_line_pair_new (const char *filename,
                    int         line)
{
    FileLinePair *pair = g_new (FileLinePair, 1);
    pair->filename = g_strdup (filename);
    pair->line = line;
    return pair;
}


static void
file_line_pair_free (FileLinePair *pair)
{
    if (pair)
    {
        g_free (pair->filename);
        g_free (pair);
    }
}


static void
finish_group(WindowStuff *stuff)
{
//     if (stuff->group_start_line >= 0 && stuff->group_end_line > stuff->group_start_line)
//         moo_text_buffer_add_fold (MOO_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (stuff->output))),
//                                   stuff->group_start_line, stuff->group_end_line);
    stuff->group_start_line = -1;
    stuff->group_end_line = -1;
}


static gboolean
process_grep_line (MooLineView *view,
                   const char  *line,
                   WindowStuff *stuff)
{
    char *filename = NULL;
    char *number = NULL;
    const char *colon, *p;
    int view_line;
    int line_no;
    guint64 line_no_64;
    mgw_errno_t err;

    g_return_val_if_fail (line != NULL, FALSE);

    /* 'Binary file blah matches' */
    if (g_str_has_prefix (line, "Binary file "))
        return FALSE;

    if (!*line)
        return TRUE;

    p = line;
    if (!(colon = strchr (p, ':')))
        goto parse_error;

#ifdef __WIN32__
    /* Absolute filename 'C:\foobar\blah.txt:100:lalala' */
    if (g_ascii_isalpha(p[0]) && p[1] == ':')
    {
        if (!(colon = strchr (colon + 1, ':')))
            goto parse_error;
    }
#endif

    if (!colon[1])
        goto parse_error;

    filename = g_strndup (p, colon - p);
    p = colon + 1;

    if (!(colon = strchr (p, ':')) || !colon[1])
        goto parse_error;

    number = g_strndup (p, colon - p);
    p = colon + 1;

    if (!stuff->current_file || strcmp (stuff->current_file, filename) != 0)
    {
        g_free (stuff->current_file);
        stuff->current_file = filename;
        view_line = moo_line_view_write_line (view, filename, -1,
                                              stuff->file_tag);
        moo_line_view_set_data (view, view_line,
                                file_line_pair_new (filename, -1),
                                (GDestroyNotify) file_line_pair_free);

        finish_group(stuff);
        stuff->group_start_line = view_line;
    }
    else
    {
        g_free (filename);
    }

    line_no_64 = mgw_ascii_strtoull (number, NULL, 0, &err);

    if (mgw_errno_is_set (err))
    {
        g_warning ("could not parse number '%s'", number);
        line_no = -1;
    }
    else if (line_no_64 > G_MAXINT)
    {
        g_warning ("number '%s' is too large", number);
        line_no = -1;
    }
    else if (line_no_64 == 0)
    {
        g_warning ("number '%s' is zero", number);
        line_no = -1;
    }
    else
    {
        line_no = (int) (line_no_64 - 1);
    }

    view_line = moo_line_view_start_line (view);
    moo_line_view_write (view, number, -1, stuff->line_number_tag);
    moo_line_view_write (view, ": ", -1, NULL);
    moo_line_view_write (view, p, -1, stuff->match_tag);
    moo_line_view_end_line (view);
    stuff->group_end_line = view_line;

    moo_line_view_set_data (view, view_line,
                            file_line_pair_new (stuff->current_file, line_no),
                            (GDestroyNotify) file_line_pair_free);
    moo_line_view_set_cursor (view, view_line, MOO_TEXT_CURSOR_LINK);
    stuff->match_count++;

    g_free (number);
    return TRUE;

parse_error:
    g_warning ("could not parse line '%s'", line);
    g_free (filename);
    g_free (number);
    return FALSE;
}


static gboolean
process_find_line (MooLineView *view,
                   const char  *line,
                   WindowStuff *stuff)
{
    int view_line;

    view_line = moo_line_view_write_line (view, line, -1, stuff->match_tag);
    moo_line_view_set_data (view, view_line,
                            file_line_pair_new (line, -1),
                            (GDestroyNotify) file_line_pair_free);
    stuff->match_count++;

    return TRUE;
}


static gboolean
process_line (MooLineView *view,
              const char  *line,
              WindowStuff *stuff)
{
    switch (stuff->cmd)
    {
        case CMD_GREP:
            return process_grep_line (view, line, stuff);
        case CMD_FIND:
            return process_find_line (view, line, stuff);
        default:
            g_return_val_if_reached (FALSE);
    }
}


static char *
parse_globs (const char *string,
             gboolean    skip)
{
    char **pieces, **p;
    GString *command;

    if (!string || !string[0] || !strcmp (string, "*"))
        return NULL;

    pieces = g_strsplit_set (string, ";,", 0);
    command = g_string_new (NULL);

    for (p = pieces; p && *p; ++p)
        if (**p)
            g_string_append_printf (command, "%s-%s \"%s%s%s\"",
                                    p == pieces ? "" : " -o ",
                                    skip ? "wholename" : "name",
                                    skip ? "*" : "",
                                    *p,
                                    skip ? "*" : "");

    g_strfreev (pieces);
    return g_string_free (command, FALSE);
}

static void
append_globs (GString    *command,
              const char *globs_string,
              const char *skip_string)
{
    char *globs, *skip;

    globs = parse_globs (globs_string, FALSE);
    skip = parse_globs (skip_string, TRUE);

    if (globs && skip)
        g_string_append_printf (command, " \\( \\( %s \\) -a ! \\( %s \\) \\) ", globs, skip);
    else if (globs)
        g_string_append_printf (command, " \\( %s \\) ", globs);
    else if (skip)
        g_string_append_printf (command, " ! \\( %s \\) ", skip);
    else
        g_string_append (command, " ");

    g_free (globs);
    g_free (skip);
}


static void
append_grep_globs (GString    *command,
                   const char *string,
                   gboolean    skip)
{
    char **globs, **p;

    globs = g_strsplit (string, ";", 0);

    for (p = globs; p && *p; ++p)
    {
        if (!**p)
            continue;

        if (!skip)
        {
            char *quoted;

            if (!strcmp (*p, "*"))
                continue;

            quoted = g_shell_quote (*p);
            g_string_append_printf (command, " --include=%s", quoted);
            g_free (quoted);
        }
        else
        {
            gsize len = strlen (*p);

            if ((*p)[len-1] == '/')
            {
                char *quoted;
                (*p)[len-1] = 0;
                quoted = g_shell_quote (*p);
                g_string_append_printf (command, " --exclude-dir=%s", quoted);
                g_free (quoted);
            }
            else
            {
                char *quoted;
                quoted = g_shell_quote (*p);
                g_string_append_printf (command, " --exclude=%s", quoted);
                g_string_append_printf (command, " --exclude-dir=%s", quoted);
                g_free (quoted);
            }
        }
    }

    g_strfreev (globs);
}

static void
execute_grep (const char     *pattern,
              const char     *glob,
              GSList         *dirs,
              const char     *skip_files,
              gboolean        case_sensitive,
              WindowStuff    *stuff)
{
    GString *command = NULL;
    char *quoted_pattern;

    g_return_if_fail (stuff->output != NULL);
    g_return_if_fail (pattern && pattern[0]);
    g_return_if_fail (dirs != NULL);

    g_free (stuff->current_file);
    stuff->current_file = NULL;
    stuff->match_count = 0;
    stuff->group_start_line = -1;
    stuff->group_end_line = -1;

    command = g_string_new ("grep -E -r -I -s -H -n");
    if (!case_sensitive)
        g_string_append (command, " -i");
    append_grep_globs (command, glob, FALSE);
    append_grep_globs (command, skip_files, TRUE);

    quoted_pattern = g_shell_quote (pattern);
    g_string_append_printf (command, " -e %s", quoted_pattern);

    while (dirs)
    {
        const char *dir = (const char*) dirs->data;
        if (dir && *dir)
        {
            char *quoted_dir = g_shell_quote (dir);
            g_string_append_printf (command, " %s", quoted_dir);
            g_free (quoted_dir);
        }
        dirs = dirs->next;
    }

    stuff->cmd = CMD_GREP;
    moo_cmd_view_run_command (stuff->output, command->str, NULL, _("Find in Files"));

    g_free (quoted_pattern);
    g_string_free (command, TRUE);
}


static void
execute_find (const char     *pattern,
              const char     *dir,
              const char     *skip_files,
              WindowStuff    *stuff)
{
    GString *command = NULL;

    g_return_if_fail (stuff->cmd == 0);
    g_return_if_fail (pattern && pattern[0]);
    g_return_if_fail (dir && dir[0]);

    g_free (stuff->current_file);
    stuff->current_file = NULL;
    stuff->match_count = 0;
    stuff->group_start_line = -1;
    stuff->group_end_line = -1;

    command = g_string_new (NULL);
    g_string_printf (command, "find '%s' -type f", dir);
    append_globs (command, pattern, skip_files);
    g_string_append (command, " -print");

    stuff->cmd = CMD_FIND;
    moo_cmd_view_run_command (stuff->output, command->str, NULL, _("Find File"));
    g_string_free (command, TRUE);
}


static void
find_window_plugin_destroy (WindowStuff *stuff)
{
    MooEditWindow *window = MOO_WIN_PLUGIN(stuff)->window;

    if (stuff->output)
    {
        g_signal_handlers_disconnect_by_func (stuff->output,
                                              (gpointer) command_exit,
                                              stuff);
        g_signal_handlers_disconnect_by_func (stuff->output,
                                              (gpointer) process_line,
                                              stuff);
        moo_cmd_view_abort (stuff->output);
        moo_edit_window_remove_pane (window, FIND_PLUGIN_ID);
    }

    if (stuff->grep_dialog)
        gtk_widget_destroy (stuff->grep_dialog);
    if (stuff->find_dialog)
        gtk_widget_destroy (stuff->find_dialog);
    g_free (stuff->current_file);
}


static gboolean
command_exit (MooLineView *view,
              int          status,
              WindowStuff *stuff)
{
    int cmd = stuff->cmd;

    g_return_val_if_fail (cmd != 0, FALSE);

    stuff->cmd = 0;

    finish_group(stuff);

#ifndef __WIN32__
    if (WIFEXITED (status))
    {
        char *msg = NULL;
        guint8 exit_code = WEXITSTATUS (status);

        /* grep exits with status of 0 if something found, 1 if nothing found */
        if (cmd == CMD_GREP && (exit_code == 0 || exit_code == 1))
            msg = g_strdup_printf (dngettext (GETTEXT_PACKAGE,
                                              "*** %u match found ***",
                                              "*** %u matches found ***",
                                              stuff->match_count),
                                   stuff->match_count);
        /* xargs exits with code 123 if it's command exited with status 1-125*/
        else if (cmd == CMD_FIND && !exit_code)
            msg = g_strdup_printf (dngettext (GETTEXT_PACKAGE,
                                              "*** %u file found ***",
                                              "*** %u files found ***",
                                              stuff->match_count),
                                   stuff->match_count);
        else
            return FALSE;

        moo_line_view_write_line (view, msg, -1,
                                  stuff->message_tag);
        g_free (msg);
        return TRUE;
    }
#else
    if (status == 0 || status == 1)
    {
        char *msg = NULL;

        if (cmd == CMD_GREP)
            msg = g_strdup_printf (dngettext (GETTEXT_PACKAGE,
                                              "*** %u match found ***",
                                              "*** %u matches found ***",
                                              stuff->match_count),
                                   stuff->match_count);
        else
            msg = g_strdup_printf (dngettext (GETTEXT_PACKAGE,
                                              "*** %u file found ***",
                                              "*** %u files found ***",
                                              stuff->match_count),
                                   stuff->match_count);

        moo_line_view_write_line (view, msg, -1,
                                  stuff->message_tag);
        g_free (msg);
        return TRUE;
    }
#endif

    return FALSE;
}


static gboolean
output_activate (WindowStuff    *stuff,
                 int             line)
{
    MooEditor *editor;
    FileLinePair *line_data;

    line_data = (FileLinePair*) moo_line_view_get_data (MOO_LINE_VIEW (stuff->output), line);

    if (!line_data || (stuff->cmd == CMD_GREP && line_data->line < 0))
        return FALSE;

    editor = moo_edit_window_get_editor (stuff->window);
    moo_editor_open_path (editor, line_data->filename, NULL, line_data->line, stuff->window);

    return TRUE;
}


MOO_PLUGIN_DEFINE_INFO (find,
                        N_("Find"), N_("Finds everything"),
                        "Yevgen Muntyan <emuntyan@users.sourceforge.net>",
                        MOO_VERSION)
MOO_WIN_PLUGIN_DEFINE (Find, find)
MOO_PLUGIN_DEFINE (Find, find,
                   NULL, NULL, NULL, NULL, NULL,
                   find_window_plugin_get_type (), 0)


gboolean
_moo_find_plugin_init (void)
{
    return moo_plugin_register (FIND_PLUGIN_ID,
                                find_plugin_get_type (),
                                &find_plugin_info,
                                NULL);
}
