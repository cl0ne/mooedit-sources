/*
 *   mooeditdialogs.c
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

#include "mooedit/mooeditdialogs.h"
#include "mooedit/mooeditprefs.h"
#include "mooedit/mooedit-fileops.h"
#include "mooedit/mooeditfileinfo.h"
#include "mooutils/moodialogs.h"
#include "mooutils/moostock.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooencodings.h"
#include "mooutils/mooutils.h"
#include "mooedit/mootextfind-prompt-gxml.h"
#include "mooedit/mooeditsavemult-gxml.h"
#include "mooedit/mootryencoding-gxml.h"
#include <gtk/gtk.h>
#include <mooglib/moo-glib.h>
#include <string.h>
#ifdef __WIN32__
#include <mooutils/moofiledialog-win32.h>
#include <gdk/gdkwin32.h>
#endif // __WIN32__


#ifndef __WIN32__

MooOpenInfoArray *
_moo_edit_open_dialog (GtkWidget *widget,
                       MooEdit   *current_doc)
{
    MooFileDialog *dialog;
    const char *encoding;
    GFile *start = NULL;
    MooFileArray *files = NULL;
    MooOpenInfoArray *info_array = NULL;
    guint i;

    moo_prefs_create_key (moo_edit_setting (MOO_EDIT_PREFS_LAST_DIR), MOO_PREFS_STATE, G_TYPE_STRING, nullptr);

    if (current_doc && moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_DIALOGS_OPEN_FOLLOWS_DOC)))
    {
        GFile *file = moo_edit_get_file (current_doc);

        if (file)
            start = g_file_get_parent (file);

        g_object_unref (file);
    }

    if (!start)
        start = moo_prefs_get_file (moo_edit_setting (MOO_EDIT_PREFS_LAST_DIR));

    dialog = moo_file_dialog_new (MOO_FILE_DIALOG_OPEN, widget,
                                  TRUE, GTK_STOCK_OPEN, start,
                                  nullptr);
    g_object_set (dialog, "enable-encodings", TRUE, nullptr);
    moo_file_dialog_set_help_id (dialog, "dialog-open");
    moo_file_dialog_set_remember_size (dialog, moo_edit_setting (MOO_EDIT_PREFS_DIALOG_OPEN));

    moo_file_dialog_set_filter_mgr_id (dialog, "MooEdit");

    if (moo_file_dialog_run (dialog))
    {
        encoding = moo_file_dialog_get_encoding (dialog);

        if (encoding && !strcmp (encoding, MOO_ENCODING_AUTO))
            encoding = NULL;

        files = moo_file_dialog_get_files (dialog);
        g_return_val_if_fail (files != NULL && files->n_elms != 0, NULL);

        info_array = moo_open_info_array_new ();
        for (i = 0; i < files->n_elms; ++i)
            moo_open_info_array_take (info_array, moo_open_info_new_file (files->elms[i], encoding, -1, MooOpenFlags (0)));

        g_object_unref (start);
        start = g_file_get_parent (files->elms[0]);
        moo_prefs_set_file (moo_edit_setting (MOO_EDIT_PREFS_LAST_DIR), start);
    }

    g_object_unref (start);
    g_object_unref (dialog);
    moo_file_array_free (files);
    return info_array;
}


MooSaveInfo *
_moo_edit_save_as_dialog (MooEdit    *doc,
                          const char *display_basename)
{
    const char *encoding;
    MooFileDialog *dialog;
    MooSaveInfo *info;
    GFile *start = NULL;
    GFile *file = NULL;

    g_return_val_if_fail (MOO_IS_EDIT (doc), nullptr);

    moo_prefs_create_key (moo_edit_setting (MOO_EDIT_PREFS_LAST_DIR),
                          MOO_PREFS_STATE, G_TYPE_STRING, NULL);

    if (moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_DIALOGS_OPEN_FOLLOWS_DOC)))
    {
        file = moo_edit_get_file (doc);
        if (file)
            start = g_file_get_parent (file);
        g_object_unref (file);
        file = NULL;
    }

    if (!start)
        start = moo_prefs_get_file (moo_edit_setting (MOO_EDIT_PREFS_LAST_DIR));

    dialog = moo_file_dialog_new (MOO_FILE_DIALOG_SAVE,
                                  GTK_WIDGET (moo_edit_get_view (doc)),
                                  FALSE, GTK_STOCK_SAVE_AS,
                                  start, display_basename);
    g_object_set (dialog, "enable-encodings", TRUE, nullptr);
    moo_file_dialog_set_encoding (dialog, moo_edit_get_encoding (doc));
    moo_file_dialog_set_help_id (dialog, "dialog-save");

    moo_file_dialog_set_filter_mgr_id (dialog, "MooEdit");

    if (!moo_file_dialog_run (dialog))
    {
        g_object_unref (dialog);
        g_object_unref (start);
        return NULL;
    }

    encoding = moo_file_dialog_get_encoding (dialog);
    file = moo_file_dialog_get_file (dialog);
    g_return_val_if_fail (file != NULL, NULL);
    info = moo_save_info_new_file (file, encoding);

    g_object_unref (start);
    start = g_file_get_parent (file);
    moo_prefs_set_file (moo_edit_setting (MOO_EDIT_PREFS_LAST_DIR), start);

    g_object_unref (start);
    g_object_unref (file);
    g_object_unref (dialog);
    return info;
}

#else // __WIN32__

static std::string
get_folder_from_document(MooEdit* doc)
{
    GFile* file = moo_edit_get_file (doc);

    if (!file)
        return std::string();

    GFile* parent = g_file_get_parent(file);
    char *parent_path = g_file_get_path(parent);
    std::string start_folder (parent_path);

    g_free(parent_path);
    g_object_unref(parent);
    g_object_unref(file);

    return start_folder;
}

MooOpenInfoArray *
_moo_edit_open_dialog(GtkWidget* parent,
                      MooEdit*   current_doc)
{
    std::string start_folder;

    if (current_doc && moo_prefs_get_bool(moo_edit_setting(MOO_EDIT_PREFS_DIALOGS_OPEN_FOLLOWS_DOC)))
        start_folder = get_folder_from_document(current_doc);

    GtkWidget* toplevel = parent ? gtk_widget_get_toplevel(parent) : nullptr;
    HWND hwnd = toplevel ? reinterpret_cast<HWND> (GDK_WINDOW_HWND(toplevel->window)) : nullptr;
    std::vector<std::string> files = moo_show_win32_file_open_dialog (hwnd, start_folder);
    if (files.empty())
        return nullptr;

    MooOpenInfoArray *result = moo_open_info_array_new();
    for (const auto& path : files)
        moo_open_info_array_take(result, moo_open_info_new(path.c_str(), MOO_ENCODING_AUTO, -1, MOO_OPEN_FLAGS_NONE));
    return result;
}


MooSaveInfo*
_moo_edit_save_as_dialog(MooEdit*    doc,
                         const char* display_basename)
{
    MooEditView* view = moo_edit_get_view (doc);
    GtkWidget* toplevel = view ? gtk_widget_get_toplevel(GTK_WIDGET(view)) : nullptr;
    HWND hwnd = toplevel ? reinterpret_cast<HWND> (GDK_WINDOW_HWND(toplevel->window)) : nullptr;

    std::string start_folder;

    if (moo_prefs_get_bool(moo_edit_setting(MOO_EDIT_PREFS_DIALOGS_OPEN_FOLLOWS_DOC)))
        start_folder = get_folder_from_document (doc);

    std::string save_as = moo_show_win32_file_save_as_dialog(hwnd, start_folder, display_basename);
    if (save_as.empty())
        return nullptr;

    return moo_save_info_new (save_as.c_str(), nullptr);
}

#endif // __WIN32__


MooSaveChangesResponse
_moo_edit_save_changes_dialog (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), MOO_SAVE_CHANGES_RESPONSE_CANCEL);
    return moo_save_changes_dialog (moo_edit_get_display_basename (doc),
                                    GTK_WIDGET (moo_edit_get_view (doc)));
}


/****************************************************************************/
/* Save multiple
 */

enum {
    COLUMN_SAVE = 0,
    COLUMN_EDIT,
    NUM_COLUMNS
};

static void
name_data_func (G_GNUC_UNUSED GtkTreeViewColumn *column,
                GtkCellRenderer   *cell,
                GtkTreeModel      *model,
                GtkTreeIter       *iter)
{
    MooEdit *doc = NULL;

    gtk_tree_model_get (model, iter, COLUMN_EDIT, &doc, -1);
    g_return_if_fail (MOO_IS_EDIT (doc));

    g_object_set (cell, "text", moo_edit_get_display_basename (doc), nullptr);
    g_object_unref (doc);
}


static void
save_toggled (GtkCellRendererToggle *cell,
              gchar                 *path,
              GtkTreeModel          *model)
{
    GtkTreePath *tree_path;
    GtkTreeIter iter;
    gboolean save = TRUE;
    gboolean active;
    gboolean sensitive;
    GtkDialog *dialog;

    g_return_if_fail (GTK_IS_LIST_STORE (model));

    tree_path = gtk_tree_path_new_from_string (path);
    g_return_if_fail (tree_path != NULL);

    gtk_tree_model_get_iter (model, &iter, tree_path);
    gtk_tree_model_get (model, &iter, COLUMN_SAVE, &save, -1);

    active = gtk_cell_renderer_toggle_get_active (cell);

    if (active == save)
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_SAVE, !save, -1);

    gtk_tree_path_free (tree_path);

    dialog = GTK_DIALOG (g_object_get_data (G_OBJECT (model), "moo-dialog"));
    g_return_if_fail (dialog != NULL);

    if (!save)
    {
        sensitive = TRUE;
    }
    else
    {
        sensitive = FALSE;
        gtk_tree_model_get_iter_first (model, &iter);

        do
        {
            gtk_tree_model_get (model, &iter, COLUMN_SAVE, &save, -1);
            if (save)
            {
                sensitive = TRUE;
                break;
            }
        }
        while (gtk_tree_model_iter_next (model, &iter));
    }

    gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_YES, sensitive);
}

static void
files_treeview_init (GtkTreeView *treeview, GtkWidget *dialog, MooEditArray *docs)
{
    GtkListStore *store;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;
    guint i;

    store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_BOOLEAN, MOO_TYPE_EDIT);

    for (i = 0; i < docs->n_elms; ++i)
    {
        GtkTreeIter iter;
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
                            COLUMN_SAVE, TRUE,
                            COLUMN_EDIT, docs->elms[i],
                            -1);
    }

    gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

    column = gtk_tree_view_column_new ();
    gtk_tree_view_append_column (treeview, column);
    cell = gtk_cell_renderer_toggle_new ();
    gtk_tree_view_column_pack_start (column, cell, FALSE);
    g_object_set (cell, "activatable", TRUE, nullptr);
    gtk_tree_view_column_add_attribute (column, cell, "active", COLUMN_SAVE);
    g_signal_connect (cell, "toggled", G_CALLBACK (save_toggled), store);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_append_column (treeview, column);
    cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, cell, TRUE);
    gtk_tree_view_column_set_cell_data_func (column, cell,
                                             (GtkTreeCellDataFunc) name_data_func,
                                             NULL, NULL);

    g_object_set_data (G_OBJECT (store), "moo-dialog", dialog);

    g_object_unref (store);
}


static void
files_treeview_get_to_save (GtkTreeView  *treeview,
                            MooEditArray *to_save)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    model = gtk_tree_view_get_model (treeview);
    g_return_if_fail (model != NULL);

    gtk_tree_model_get_iter_first (model, &iter);

    do
    {
        MooEdit *doc = NULL;
        gboolean save = TRUE;

        gtk_tree_model_get (model, &iter,
                            COLUMN_SAVE, &save,
                            COLUMN_EDIT, &doc, -1);
        g_return_if_fail (MOO_IS_EDIT (doc));

        if (save)
            moo_edit_array_append (to_save, doc);

        g_object_unref (doc);
    }
    while (gtk_tree_model_iter_next (model, &iter));
}


static GtkWidget *
find_widget_for_response (GtkDialog *dialog,
                          int        response)
{
    GList *l, *children;
    GtkWidget *ret = NULL;

    children = gtk_container_get_children (GTK_CONTAINER (dialog->action_area));

    for (l = children; ret == NULL && l != NULL; l = l->next)
    {
        GtkWidget *widget = GTK_WIDGET (l->data);
        int response_here = gtk_dialog_get_response_for_widget (dialog, widget);
        if (response_here == response)
            ret = widget;
    }

    g_list_free (children);
    return ret;
}

MooSaveChangesResponse
_moo_edit_save_multiple_changes_dialog (MooEditArray *docs,
                                        MooEditArray *to_save)
{
    GtkWidget *dialog;
    char *msg, *question;
    int response;
    MooSaveChangesResponse retval;
    SaveMultDialogXml *xml;

    g_return_val_if_fail (docs != NULL && docs->n_elms > 1, MOO_SAVE_CHANGES_RESPONSE_CANCEL);
    g_return_val_if_fail (to_save != NULL, MOO_SAVE_CHANGES_RESPONSE_CANCEL);

    xml = save_mult_dialog_xml_new ();
    dialog = GTK_WIDGET (xml->SaveMultDialog);

    moo_window_set_parent (dialog, GTK_WIDGET (moo_edit_get_view (docs->elms[0])));

    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                            MOO_STOCK_SAVE_NONE, GTK_RESPONSE_NO,
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            MOO_STOCK_SAVE_SELECTED, GTK_RESPONSE_YES,
                            nullptr);

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_YES,
                                             GTK_RESPONSE_NO,
                                             GTK_RESPONSE_CANCEL, -1);

    question = g_strdup_printf (dngettext (GETTEXT_PACKAGE,
                                           /* Translators: number of documents here is always greater than one, so
                                              ignore singular form (which is simply copy of the plural here) */
                                           "There are %u documents with unsaved changes. "
                                            "Save changes before closing?",
                                           "There are %u documents with unsaved changes. "
                                            "Save changes before closing?",
                                           docs->n_elms),
                                (guint) docs->n_elms);
    msg = g_markup_printf_escaped ("<span weight=\"bold\" size=\"larger\">%s</span>",
                                   question);
    gtk_label_set_markup (xml->label, msg);

    files_treeview_init (xml->treeview, dialog, docs);

    {
        GtkWidget *button;
        button = find_widget_for_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
        gtk_widget_grab_focus (button);
    }

    response = gtk_dialog_run (GTK_DIALOG (dialog));

    switch (response)
    {
        case GTK_RESPONSE_NO:
            retval = MOO_SAVE_CHANGES_RESPONSE_DONT_SAVE;
            break;
        case GTK_RESPONSE_YES:
            files_treeview_get_to_save (xml->treeview, to_save);
            retval = MOO_SAVE_CHANGES_RESPONSE_SAVE;
            break;
        default:
            retval = MOO_SAVE_CHANGES_RESPONSE_CANCEL;
    }

    g_free (question);
    g_free (msg);
    gtk_widget_destroy (dialog);
    return retval;
}


/*****************************************************************************/
/* Error dialogs
 */

void
_moo_edit_save_error_dialog (MooEdit *doc,
                             GFile   *file,
                             GError  *error)
{
    char *filename, *msg = NULL;

    g_return_if_fail (G_IS_FILE (file));

    filename = moo_file_get_display_name (file);

    msg = g_strdup_printf (_("Could not save file\n%s"), filename);

    moo_error_dialog (msg, moo_error_message (error),
                      GTK_WIDGET (moo_edit_get_view (doc)));

    g_free (msg);
    g_free (filename);
}

static gboolean
moo_edit_question_dialog (MooEdit    *doc,
                          const char *text,
                          const char *secondary,
                          const char *button,
                          int         default_response)
{
    int res;
    MooEditView *view;
    GtkWindow *parent;
    GtkWidget *dialog;

    view = doc ? moo_edit_get_view (doc) : NULL;
    parent = view ? GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view))) : NULL;

    dialog = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_WARNING,
                                     GTK_BUTTONS_NONE,
                                     "%s", text);
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              "%s", secondary);

    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            button, GTK_RESPONSE_YES,
                            nullptr);

    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_YES,
                                             GTK_RESPONSE_CANCEL,
                                             -1);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), default_response);

    res = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    return res == GTK_RESPONSE_YES;
}

gboolean
_moo_edit_save_error_enc_dialog (MooEdit    *doc,
                                 GFile      *file,
                                 const char *encoding)
{
    char *filename;
    char *secondary;
    gboolean result;

    g_return_val_if_fail (G_IS_FILE (file), FALSE);
    g_return_val_if_fail (encoding != NULL, FALSE);

    filename = moo_file_get_display_name (file);

    secondary = g_strdup_printf (_("Could not save file %s in encoding %s. "
                                   "Do you want to save it in UTF-8 encoding instead?"),
                                 filename, encoding);

    result = moo_edit_question_dialog (doc, _("Save file in UTF-8 encoding?"),
                                       secondary, GTK_STOCK_OK, GTK_RESPONSE_YES);

    g_free (secondary);
    g_free (filename);
    return result;
}


MooEditTryEncodingResponse
_moo_edit_try_encoding_dialog (G_GNUC_UNUSED GFile *file,
                               const char  *encoding,
                               char       **new_encoding)
{
    MooEditWindow *window;
    GtkWidget *dialog;
    TryEncodingDialogXml *xml;
    int dialog_response;
    char *filename = NULL;
    char *msg = NULL;
    char *secondary = NULL;

    filename = moo_file_get_display_name (file);

    if (filename)
    {
        /* Could not open file foo.txt */
        char *tmp = g_strdup_printf (_("Could not open file\n%s"), filename);
        msg = g_markup_printf_escaped ("<b><big>%s</big></b>", tmp);
        g_free (tmp);
    }
    else
    {
        const char *tmp = _("Could not open file");
        msg = g_markup_printf_escaped ("<b><big>%s</big></b>", tmp);
    }

    if (encoding)
        secondary = g_strdup_printf (_("Could not open file using character encoding %s. "
                                       "Try to select another encoding below."), encoding);
    else
        secondary = g_strdup_printf (_("Could not detect file character encoding. "
                                       "Try to select an encoding below."));

    xml = try_encoding_dialog_xml_new ();
    g_return_val_if_fail (xml && xml->TryEncodingDialog, MOO_EDIT_TRY_ENCODING_RESPONSE_CANCEL);

    gtk_label_set_markup (xml->label_primary, msg);
    gtk_label_set_text (xml->label_secondary, secondary);

    dialog = GTK_WIDGET (xml->TryEncodingDialog);

    _moo_encodings_combo_init (GTK_COMBO_BOX (xml->encoding_combo), MOO_ENCODING_COMBO_OPEN, FALSE);
    _moo_encodings_combo_set_enc (GTK_COMBO_BOX (xml->encoding_combo), encoding, MOO_ENCODING_COMBO_OPEN);

    if ((window = moo_editor_get_active_window (moo_editor_instance ())))
        moo_window_set_parent (dialog, GTK_WIDGET (window));

    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK, GTK_RESPONSE_OK,
                            nullptr);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_OK,
                                             GTK_RESPONSE_CANCEL,
                                             -1);

    dialog_response = gtk_dialog_run (GTK_DIALOG (dialog));

    *new_encoding = g_strdup (_moo_encodings_combo_get_enc (GTK_COMBO_BOX (xml->encoding_combo), MOO_ENCODING_COMBO_OPEN));

    gtk_widget_destroy (dialog);
    g_free (secondary);
    g_free (msg);

    return dialog_response == GTK_RESPONSE_OK ?
        MOO_EDIT_TRY_ENCODING_RESPONSE_TRY_ANOTHER :
        MOO_EDIT_TRY_ENCODING_RESPONSE_CANCEL;
}


void
_moo_edit_open_error_dialog (GtkWidget  *widget,
                             GFile      *file,
                             GError     *error)
{
    char *filename, *msg = NULL;
    char *secondary;

    filename = moo_file_get_display_name (file);

    if (filename)
        /* Could not open file foo.txt */
        msg = g_strdup_printf (_("Could not open file\n%s"), filename);
    else
        msg = g_strdup (_("Could not open file"));

    secondary = error ? g_strdup (error->message) : NULL;

    moo_error_dialog (msg, secondary, widget);

    g_free (msg);
    g_free (secondary);
    g_free (filename);
}


void
_moo_edit_reload_error_dialog (MooEdit *doc,
                               GError  *error)
{
    const char *filename;
    char *msg = NULL;

    g_return_if_fail (MOO_IS_EDIT (doc));

    filename = moo_edit_get_display_basename (doc);

    if (!filename)
    {
        g_critical ("oops");
        filename = "";
    }

    /* Could not reload file foo.txt */
    msg = g_strdup_printf (_("Could not reload file\n%s"), filename);
    /* XXX */
    moo_error_dialog (msg, moo_error_message (error),
                      GTK_WIDGET (moo_edit_get_view (doc)));

    g_free (msg);
}


/*****************************************************************************/
/* Confirmation and alerts
 */

gboolean
_moo_edit_reload_modified_dialog (MooEdit *doc)
{
    const char *name;
    char *question;
    gboolean result;

    name = moo_edit_get_display_basename (doc);

    if (!name)
    {
        g_critical ("oops");
        name = "";
    }

    question = g_strdup_printf (_("Discard changes in file '%s'?"), name);
    result = moo_edit_question_dialog (doc, question,
                                       _("If you reload the document, changes will be discarded"),
                                       _("_Reload"),
                                       GTK_RESPONSE_CANCEL);

    g_free (question);
    return result;
}

gboolean
_moo_edit_overwrite_modified_dialog (MooEdit *doc)
{
    const char *name;
    char *question, *secondary;
    gboolean result;

    name = moo_edit_get_display_basename (doc);

    if (!name)
    {
        g_critical ("oops");
        name = "";
    }

    question = g_strdup_printf (_("Overwrite modified file '%s'?"), name);
    secondary = g_strdup_printf (_("File '%s' was modified on disk by another process. If you save it, "
                                   "changes on disk will be lost."), name);
    result = moo_edit_question_dialog (doc, question, secondary, _("Over_write"), GTK_RESPONSE_CANCEL);

    g_free (question);
    g_free (secondary);
    return result;
}


/***************************************************************************/
/* Search dialogs
 */

gboolean
_moo_text_search_from_start_dialog (GtkWidget *widget,
                                    gboolean   backwards)
{
    GtkWidget *dialog;
    int response;
    const char *msg;

    if (backwards)
        msg = _("Beginning of document reached.\n"
                "Continue from the end?");
    else
        msg = _("End of document reached.\n"
                "Continue from the beginning?");

    dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                     "%s", msg);
    moo_window_set_parent (dialog, widget);

    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                            GTK_STOCK_NO, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_YES, GTK_RESPONSE_YES,
                            nullptr);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_YES,
                                             GTK_RESPONSE_CANCEL,
                                             -1);

    response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    return response == GTK_RESPONSE_YES;
}


void
_moo_text_regex_error_dialog (GtkWidget  *parent,
                              GError     *error)
{
    GtkWidget *dialog;
    char *msg_text = NULL;

    if (error)
    {
        if (error->domain != G_REGEX_ERROR)
            g_warning ("unknown error domain");
        msg_text = g_strdup (error->message);
    }
    else
    {
        msg_text = g_strdup_printf (_("Invalid regular expression"));
    }

    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE,
                                     "%s", msg_text);
    moo_window_set_parent (dialog, parent);
    gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CLOSE,
                            GTK_RESPONSE_CANCEL, nullptr);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    g_free (msg_text);
}


GtkWidget *
_moo_text_prompt_on_replace_dialog (GtkWidget *parent)
{
    FindPromptDialogXml *xml;
    xml = find_prompt_dialog_xml_new ();
    moo_window_set_parent (GTK_WIDGET (xml->FindPromptDialog), parent);
    return GTK_WIDGET (xml->FindPromptDialog);
}
