/*
 *   fileselector.c
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

#include "moofileselector.h"
#include "mooedit/mooplugin-macro.h"
#include "mooedit/mooeditwindow.h"
#include "mooedit/mooedittab.h"
#include "moofileview/moobookmarkmgr.h"
#include "moofileview/moofileview-tools.h"
#include "plugins/mooplugin-builtin.h"
#include "marshals.h"
#include "mooutils/moostock.h"
#include "mooutils/mooprefs.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooutils.h"
#include "mooutils/mooentry.h"
#include "mooutils/moodialogs.h"
#include "mooutils/mooactionfactory.h"
#include "mooutils/mooi18n.h"
#include "mooutils/moo-mime.h"
#include "mooutils/moomenu.h"
#include "plugins/moofileselector-gxml.h"
#include "mooutils/moohelp.h"
#include "mooutils/mooatom.h"
#include <mooglib/moo-glib.h>
#include <mooglib/moo-stat.h>
#ifdef MOO_ENABLE_HELP
#include "moo-help-sections.h"
#endif
#include <gmodule.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

#define PREFS_LAST_DIR MOO_PLUGIN_PREFS_ROOT "/" MOO_FILE_SELECTOR_PLUGIN_ID "/last_dir"
#define PREFS_HIDDEN_FILES MOO_PLUGIN_PREFS_ROOT "/" MOO_FILE_SELECTOR_PLUGIN_ID "/show_hidden_files"


typedef struct {
    MooPlugin parent;
    MooBookmarkMgr *bookmark_mgr;
    GSList *instances;
} FileSelectorPlugin;

#define Plugin FileSelectorPlugin
#define FILE_SELECTOR_PLUGIN(mpl) ((FileSelectorPlugin*)mpl)


enum {
    TARGET_MOO_EDIT_TAB = 1,
    TARGET_URI_LIST = 2
};

static GtkTargetEntry targets[] = {
    { (char*) MOO_EDIT_TAB_ATOM_NAME, GTK_TARGET_SAME_APP, TARGET_MOO_EDIT_TAB },
    { (char*) "text/uri-list", 0, TARGET_URI_LIST }
};

#define OPEN_PANE_TIMEOUT 200

G_DEFINE_TYPE (MooFileSelector, _moo_file_selector, MOO_TYPE_FILE_VIEW)

enum {
    PROP_0,
    PROP_WINDOW
};


static void     moo_file_selector_set_property  (GObject            *object,
                                                 guint               prop_id,
                                                 const GValue       *value,
                                                 GParamSpec         *pspec);
static void     moo_file_selector_get_property  (GObject            *object,
                                                 guint               prop_id,
                                                 GValue             *value,
                                                 GParamSpec         *pspec);
static GObject *moo_file_selector_constructor   (GType               type,
                                                 guint               n_props,
                                                 GObjectConstructParam *props);
static void     moo_file_selector_finalize      (GObject            *object);

static void     moo_file_selector_select_file   (MooFileSelector    *filesel,
                                                 GFile              *file);
static void     moo_file_selector_select_path   (MooFileSelector    *filesel,
                                                 const char         *filename);
static gboolean moo_file_selector_chdir         (MooFileView        *fileview,
                                                 const char         *dir,
                                                 GError            **error);
static void     moo_file_selector_activate      (MooFileView        *fileview,
                                                 const char         *path);
static void     moo_file_selector_populate_popup(MooFileView        *fileview,
                                                 GList              *selected,
                                                 GtkMenu            *menu);

static void     goto_current_doc_dir            (MooFileSelector    *filesel);

static gboolean moo_file_selector_drop          (MooFileView        *fileview,
                                                 const char         *path,
                                                 GtkWidget          *widget,
                                                 GdkDragContext     *context,
                                                 int                 x,
                                                 int                 y,
                                                 guint               time);
static gboolean moo_file_selector_drop_data_received (MooFileView   *fileview,
                                                 const char         *path,
                                                 GtkWidget          *widget,
                                                 GdkDragContext     *context,
                                                 int                 x,
                                                 int                 y,
                                                 GtkSelectionData   *data,
                                                 guint               info,
                                                 guint               time);

static void     moo_file_selector_drop_doc      (MooFileSelector    *filesel,
                                                 MooEdit            *doc,
                                                 const char         *destdir,
                                                 GtkWidget          *widget,
                                                 GdkDragContext     *context,
                                                 int                 x,
                                                 int                 y,
                                                 guint               time);


static void
_moo_file_selector_class_init (MooFileSelectorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    MooFileViewClass *fileview_class = MOO_FILE_VIEW_CLASS (klass);

    gobject_class->set_property = moo_file_selector_set_property;
    gobject_class->get_property = moo_file_selector_get_property;
    gobject_class->constructor = moo_file_selector_constructor;
    gobject_class->finalize = moo_file_selector_finalize;

    fileview_class->chdir = moo_file_selector_chdir;
    fileview_class->activate = moo_file_selector_activate;
    fileview_class->drop = moo_file_selector_drop;
    fileview_class->drop_data_received = moo_file_selector_drop_data_received;
    fileview_class->populate_popup = moo_file_selector_populate_popup;

    g_object_class_install_property (gobject_class, PROP_WINDOW,
        g_param_spec_object ("window", "window", "window",
                             MOO_TYPE_EDIT_WINDOW, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    _moo_signal_new_cb ("goto-current-doc-dir",
                        G_OBJECT_CLASS_TYPE (klass),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                        G_CALLBACK (goto_current_doc_dir),
                        nullptr, nullptr,
                        _moo_marshal_VOID__VOID,
                        G_TYPE_NONE, 0);
}


static void
_moo_file_selector_init (MooFileSelector *filesel)
{
    filesel->targets = gtk_target_list_new (targets, G_N_ELEMENTS (targets));
    _moo_file_view_add_target (MOO_FILE_VIEW (filesel), MOO_EDIT_TAB_ATOM,
                               GTK_TARGET_SAME_APP, TARGET_MOO_EDIT_TAB);

#ifdef MOO_ENABLE_HELP
    moo_help_set_id (GTK_WIDGET (filesel), HELP_SECTION_FILE_SELECTOR);
#endif
    moo_help_connect_keys (GTK_WIDGET (filesel));
}


static void
moo_file_selector_finalize (GObject *object)
{
    MooFileSelector *filesel = MOO_FILE_SELECTOR (object);
    gtk_target_list_unref (filesel->targets);
    G_OBJECT_CLASS(_moo_file_selector_parent_class)->finalize (object);
}


static void
moo_file_selector_set_property (GObject        *object,
                                guint           prop_id,
                                const GValue   *value,
                                GParamSpec     *pspec)
{
    MooFileSelector *sel = MOO_FILE_SELECTOR (object);

    switch (prop_id)
    {
        case PROP_WINDOW:
            sel->window = (MooEditWindow*) g_value_get_object (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
moo_file_selector_get_property (GObject        *object,
                                guint           prop_id,
                                GValue         *value,
                                GParamSpec     *pspec)
{
    MooFileSelector *sel = MOO_FILE_SELECTOR (object);

    switch (prop_id)
    {
        case PROP_WINDOW:
            g_value_set_object (value, sel->window);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
file_selector_go_home (MooFileView *fileview)
{
    const char *dir;

    dir = moo_prefs_get_filename (PREFS_LAST_DIR);

    if (!dir || !moo_file_view_chdir_path (fileview, dir, nullptr))
        g_signal_emit_by_name (fileview, "go-home");
}


static gboolean
moo_file_selector_chdir (MooFileView    *fileview,
                         const char     *dir,
                         GError        **error)
{
    gboolean result;

    result = MOO_FILE_VIEW_CLASS(_moo_file_selector_parent_class)->chdir (fileview, dir, error);

    if (result)
    {
        char *new_dir = nullptr;
        g_object_get (fileview, "current-directory", &new_dir, nullptr);
        moo_prefs_set_filename (PREFS_LAST_DIR, new_dir);
        g_free (new_dir);
    }

    return result;
}


static void
moo_file_selector_activate (MooFileView    *fileview,
                            const char     *path)
{
    MgwStatBuf statbuf;
    MooFileSelector *filesel = MOO_FILE_SELECTOR (fileview);
    gboolean is_text = TRUE, is_exe = FALSE;
    mgw_errno_t err;

    g_return_if_fail (path != nullptr);

    if (mgw_stat (path, &statbuf, &err) != 0)
    {
        g_warning ("error in stat(%s): %s", path, mgw_strerror (err));
        return;
    }

    {
        const char *mime_type = moo_get_mime_type_for_file (path, &statbuf);

        if (!strcmp (mime_type, "application/x-trash"))
        {
            guint i;
            const char *bak_suffixes[] = {"~", "%", ".bak", ".old", ".sik"};

            for (i = 0; i < G_N_ELEMENTS (bak_suffixes); ++i)
                if (g_str_has_suffix (path, bak_suffixes[i]))
                {
                    char *tmp = g_strndup (path, strlen (path) - strlen (bak_suffixes[i]));
                    mime_type = moo_get_mime_type_for_filename (tmp);
                    g_free (tmp);
                    break;
                }
        }

        is_text = !strcmp (mime_type, "application/octet-stream") ||
                   moo_mime_type_is_subclass (mime_type, "text/plain");
        is_exe = !strcmp (mime_type, "application/x-executable");
    }

    if (is_text)
        moo_editor_open_path (moo_edit_window_get_editor (filesel->window),
                              path, nullptr, -1, filesel->window);
    else if (!is_exe)
        moo_open_file (path);
}


static void
goto_current_doc_dir (MooFileSelector *filesel)
{
    MooEdit *doc;
    GFile *file, *parent_file;
    GError *error = nullptr;

    doc = moo_edit_window_get_active_doc (filesel->window);
    file = doc ? moo_edit_get_file (doc) : nullptr;
    parent_file = file ? g_file_get_parent (file) : nullptr;

    if (parent_file)
    {
        if (moo_file_view_chdir (MOO_FILE_VIEW (filesel), parent_file, &error))
        {
            moo_file_view_focus_files (MOO_FILE_VIEW (filesel));
            moo_file_selector_select_file (filesel, file);
        }
    }

    if (error)
    {
        g_warning ("%s: %s", G_STRFUNC, moo_error_message (error));
        g_error_free (error);
    }

    g_object_unref (parent_file);
    g_object_unref (file);
}


/****************************************************************************/
/* NewFile
 */

static void
moo_file_selector_populate_popup (MooFileView *fileview,
                                  GList       *selected,
                                  G_GNUC_UNUSED GtkMenu *menu)
{
    GtkAction *new_file, *open;

    new_file = moo_action_collection_get_action (moo_file_view_get_actions (fileview), "NewFile");
    open = moo_action_collection_get_action (moo_file_view_get_actions (fileview), "Open");

    if (new_file)
        gtk_action_set_sensitive (new_file, !selected || !selected->next);

    if (open)
        gtk_action_set_visible (open, selected && selected->next);
}


static GtkWidget *
create_new_file_dialog (GtkWidget         *parent,
                        const char        *dirname,
                        const char        *start_text,
                        NewFileDialogXml **xml)
{
    GtkWidget *dialog;
    char *display_dirname, *label_text;

    display_dirname = g_filename_display_basename (dirname);
    g_return_val_if_fail (display_dirname != nullptr, nullptr);

    *xml = new_file_dialog_xml_new ();
    dialog = GTK_WIDGET ((*xml)->NewFileDialog);

    moo_window_set_parent (dialog, parent);

    gtk_entry_set_text (GTK_ENTRY ((*xml)->entry), start_text);
    moo_entry_clear_undo ((*xml)->entry);

    label_text = g_strdup_printf (_("Create file in folder '%s':"), display_dirname);
    gtk_label_set_text ((*xml)->label, label_text);

    gtk_widget_show_all (dialog);
    gtk_widget_grab_focus (GTK_WIDGET ((*xml)->entry));

    moo_bind_bool_property ((*xml)->ok_button, "sensitive", (*xml)->entry, "empty", TRUE);

    g_free (label_text);
    g_free (display_dirname);
    return dialog;
}

static char *
new_file_dialog (GtkWidget   *parent,
                 const char  *dirname,
                 const char  *start_name)
{
    NewFileDialogXml *xml = nullptr;
    GtkWidget *dialog = nullptr;
    GtkEntry *entry = nullptr;
    char *fullname = nullptr;

    g_return_val_if_fail (dirname != nullptr, nullptr);

    while (TRUE)
    {
        const char *text;
        char *name;
        char *err_text;

        if (!dialog)
        {
            dialog = create_new_file_dialog (parent, dirname, start_name, &xml);
            g_return_val_if_fail (dialog != nullptr, nullptr);
            entry = GTK_ENTRY (xml->entry);
        }

        if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_OK)
            goto out;

        text = gtk_entry_get_text (entry);

        if (!text[0])
        {
            g_critical ("oops");
            goto out;
        }

        /* XXX error checking, you know */
        name = g_filename_from_utf8 (text, -1, nullptr, nullptr, nullptr);

        if (!name)
        {
            char *sec_text;
            err_text = g_strdup_printf (_("Could not create file '%s'"), text);
            sec_text = g_strdup_printf (_("Could not convert '%s' to filename encoding. "
                                          "Please consider simpler name, such as foo.blah "
                                          "or blah.foo"), text);
            moo_error_dialog (err_text, sec_text, dialog ? dialog : parent);
            g_free (err_text);
            g_free (sec_text);
            continue;
        }

        fullname = g_build_filename (dirname, name, nullptr);
        g_free (name);

        if (!g_file_test (fullname, G_FILE_TEST_EXISTS))
            goto out;

        err_text = g_strdup_printf (_("File '%s' already exists"), text);
        moo_error_dialog (err_text, nullptr, dialog);
        g_free (err_text);

        g_free (fullname);
        fullname = nullptr;
    }

out:
    if (dialog)
        gtk_widget_destroy (dialog);
    return fullname;
}


static void
file_selector_create_file (MooFileSelector *filesel)
{
    char *path = nullptr, *dir = nullptr;
    MooEdit *doc;
    GList *selected = nullptr;
    MooOpenInfo *info;

    selected = _moo_file_view_get_filenames (MOO_FILE_VIEW (filesel));

    if (selected)
    {
        if (selected->next)
        {
            g_critical ("oops");
            goto out;
        }

        dir = (char*) selected->data;
        g_list_free (selected);
        selected = nullptr;

        if (!g_file_test (dir, G_FILE_TEST_IS_DIR))
        {
            g_free (dir);
            dir = nullptr;
        }
    }

    if (!dir)
    {
        g_object_get (filesel, "current-directory", &dir, nullptr);

        if (!dir)
            goto out;
    }

    path = new_file_dialog (GTK_WIDGET (filesel), dir, _("Untitled"));

    if (!path)
        goto out;

    info = moo_open_info_new (path, nullptr, -1, MOO_OPEN_FLAGS_NONE);
    doc = moo_editor_new_file (moo_edit_window_get_editor (filesel->window),
                               info, GTK_WIDGET (filesel), nullptr);
    g_object_unref (info);

    if (doc)
        moo_edit_save (doc, nullptr);

out:
    g_free (path);
    g_free (dir);
    g_list_foreach (selected, (GFunc) g_free, nullptr);
    g_list_free (selected);
}


static void
file_selector_open_files (MooFileSelector *filesel)
{
    GList *selected = nullptr;
    GList *files = nullptr;

    selected = _moo_file_view_get_filenames (MOO_FILE_VIEW (filesel));

    if (!selected)
    {
        g_critical ("oops");
        return;
    }

    while (selected)
    {
        char *filename = (char*) selected->data;

        if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
            files = g_list_prepend (files, filename);
        else
            g_free (filename);

        selected = g_list_delete_link (selected, selected);
    }

    files = g_list_reverse (files);

    while (files)
    {
        moo_editor_open_path (moo_edit_window_get_editor (filesel->window),
                              (const char*) files->data, nullptr, -1, filesel->window);
        g_free (files->data);
        files = g_list_delete_link (files, files);
    }
}

static void
notify_show_hidden_files (MooFileSelector *filesel)
{
    gboolean show = FALSE;
    g_object_get (filesel, "show-hidden-files", &show, nullptr);
    moo_prefs_set_bool (PREFS_HIDDEN_FILES, show);
}

/****************************************************************************/
/* Constructor
 */

static GObject *
moo_file_selector_constructor (GType           type,
                               guint           n_props,
                               GObjectConstructParam *props)
{
    MooPaneLabel *label;
    MooUiXml *xml;
    MooFileSelector *filesel;
    MooFileView *fileview;
    GObject *object;
    guint merge_id;
    GtkActionGroup *group;
    MooPane *pane;

    object = G_OBJECT_CLASS(_moo_file_selector_parent_class)->constructor (type, n_props, props);
    filesel = MOO_FILE_SELECTOR (object);
    fileview = MOO_FILE_VIEW (object);

    g_return_val_if_fail (filesel->window != nullptr, object);

    file_selector_go_home (MOO_FILE_VIEW (fileview));

    g_object_set (fileview, "show-hidden-files", moo_prefs_get_bool (PREFS_HIDDEN_FILES), nullptr);
    g_signal_connect (fileview, "notify::show-hidden-files", G_CALLBACK (notify_show_hidden_files), nullptr);

    group = moo_action_collection_get_group (moo_file_view_get_actions (MOO_FILE_VIEW (fileview)), nullptr);
    xml = moo_file_view_get_ui_xml (MOO_FILE_VIEW (fileview));
    merge_id = moo_ui_xml_new_merge_id (xml);

    moo_action_group_add_action (group, "GoToCurrentDocDir",
                                 "stock-id", GTK_STOCK_JUMP_TO,
                                 "tooltip", _("Go to current document directory"),
                                 "closure-object", fileview,
                                 "closure-signal", "goto-current-doc-dir",
                                 nullptr);
    moo_ui_xml_insert_markup (xml, merge_id,
                              "MooFileView/Toolbar", -1,
                              "<item action=\"GoToCurrentDocDir\"/>");
    _moo_file_view_setup_button_drag_dest (MOO_FILE_VIEW (filesel),
                                           "MooFileView/Toolbar/GoToCurrentDocDir",
                                           "goto-current-doc-dir");

    moo_action_group_add_action (group, "NewFile",
                                 "label", _("New File..."),
                                 "tooltip", _("New File..."),
                                 "stock-id", GTK_STOCK_NEW,
                                 "closure-object", filesel,
                                 "closure-callback", file_selector_create_file,
                                 nullptr);
    moo_action_group_add_action (group, "Open",
                                 "label", GTK_STOCK_OPEN,
                                 "tooltip", GTK_STOCK_OPEN,
                                 "stock-id", GTK_STOCK_OPEN,
                                 "closure-object", filesel,
                                 "closure-callback", file_selector_open_files,
                                 nullptr);
    moo_ui_xml_insert_markup_before (xml, merge_id,
                                     "MooFileView/Menu",
                                     "NewFolder",
                                     "<item action=\"NewFile\"/>"
                                     "<separator/>");
    moo_ui_xml_insert_markup (xml, merge_id,
                              "MooFileView/Menu", 0,
                              "<item action=\"Open\"/>");

    label = moo_pane_label_new (MOO_STOCK_FILE_SELECTOR,
                                nullptr, _("File Selector"),
                                _("File Selector"));
    moo_edit_window_add_pane (filesel->window, MOO_FILE_SELECTOR_PLUGIN_ID,
                              GTK_WIDGET (filesel), label, MOO_PANE_POS_RIGHT);
    moo_pane_label_free (label);

    pane = moo_big_paned_find_pane (filesel->window->paned,
                                    GTK_WIDGET (filesel), nullptr);
    moo_pane_set_drag_dest (pane);

    return object;
}


static gboolean
moo_file_selector_drop (MooFileView    *fileview,
                        const char     *path,
                        GtkWidget      *widget,
                        GdkDragContext *context,
                        int             x,
                        int             y,
                        guint           time)
{
    MooFileSelector *filesel = MOO_FILE_SELECTOR (fileview);
    GdkAtom target;

    filesel->waiting_for_tab = FALSE;
    target = gtk_drag_dest_find_target (widget, context, filesel->targets);

    if (target != MOO_EDIT_TAB_ATOM)
        return MOO_FILE_VIEW_CLASS (_moo_file_selector_parent_class)->
                drop (fileview, path, widget, context, x, y, time);

    filesel->waiting_for_tab = TRUE;

    gtk_drag_get_data (widget, context, MOO_EDIT_TAB_ATOM, time);

    return TRUE;
}


static gboolean
moo_file_selector_drop_data_received (MooFileView    *fileview,
                                      const char     *path,
                                      GtkWidget      *widget,
                                      GdkDragContext *context,
                                      int             x,
                                      int             y,
                                      GtkSelectionData *data,
                                      guint           info,
                                      guint           time)
{
    MooEditTab *tab;
    MooEditView *view;
    MooFileSelector *filesel = MOO_FILE_SELECTOR (fileview);

    if (!filesel->waiting_for_tab)
        goto parent;

    if (info != TARGET_MOO_EDIT_TAB)
    {
        g_critical ("oops");
        goto parent;
    }
    else if (data->length < 0)
    {
        g_warning ("could not get MOO_EDIT_TAB data");
        goto error;
    }

    tab = (MooEditTab*) moo_selection_data_get_pointer (data, MOO_EDIT_TAB_ATOM);

    if (!MOO_IS_EDIT_TAB (tab))
    {
        g_critical ("oops");
        goto error;
    }

    view = moo_edit_tab_get_active_view (tab);

    moo_file_selector_drop_doc (MOO_FILE_SELECTOR (fileview),
                                moo_edit_view_get_doc (view),
                                path, widget, context,
                                x, y, time);
    return TRUE;

error:
    _moo_file_view_drag_finish (fileview, context, FALSE, FALSE, time);
    return FALSE;

parent:
    return MOO_FILE_VIEW_CLASS(_moo_file_selector_parent_class)->
                drop_data_received (fileview, path, widget, context,
                                    x, y, data, info, time);
}


static GtkWidget *
create_save_as_dialog (GtkWidget        *parent,
                       const char       *start_text,
                       const char       *title,
                       SaveAsDialogXml **xml)
{
    GtkWidget *dialog;

    *xml = save_as_dialog_xml_new ();
    dialog = GTK_WIDGET ((*xml)->SaveAsDialog);

    gtk_window_set_title (GTK_WINDOW (dialog), title);
    moo_window_set_parent (dialog, parent);

    gtk_entry_set_text (GTK_ENTRY ((*xml)->entry), start_text);
    moo_entry_clear_undo (MOO_ENTRY ((*xml)->entry));

    gtk_widget_show_all (dialog);
    gtk_widget_grab_focus (GTK_WIDGET ((*xml)->entry));

    moo_bind_bool_property ((*xml)->ok_button, "sensitive", (*xml)->entry, "empty", TRUE);

    return dialog;
}

static char *
save_as_dialog (GtkWidget   *parent,
                const char  *dirname,
                const char  *start_name,
                gboolean     ask_name,
                const char  *title)
{
    SaveAsDialogXml *xml = nullptr;
    GtkWidget *dialog = nullptr;
    GtkEntry *entry = nullptr;
    char *fullname = nullptr;

    g_return_val_if_fail (dirname != nullptr, nullptr);

    while (TRUE)
    {
        const char *text;
        char *name, *display_dirname;

        if (!ask_name)
        {
            ask_name = TRUE;
            text = start_name;
        }
        else
        {
            if (!dialog)
            {
                dialog = create_save_as_dialog (parent, start_name, title, &xml);
                entry = GTK_ENTRY (xml->entry);
            }

            if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_OK)
                goto out;

            text = gtk_entry_get_text (entry);
        }

        if (!text[0])
        {
            g_critical ("oops");
            goto out;
        }

        /* XXX error checking, you know */
        name = g_filename_from_utf8 (text, -1, nullptr, nullptr, nullptr);

        if (!name)
        {
            char *err_text, *sec_text;
            err_text = g_strdup_printf (_("Could not save file as '%s'"), text);
            sec_text = g_strdup_printf (_("Could not convert '%s' to filename encoding. "
                                          "Please consider simpler name, such as foo.blah "
                                          "or blah.foo"), text);
            moo_error_dialog (err_text, sec_text, dialog ? dialog : parent);
            g_free (err_text);
            g_free (sec_text);
            continue;
        }

        fullname = g_build_filename (dirname, name, nullptr);
        g_free (name);

        if (!g_file_test (fullname, G_FILE_TEST_EXISTS))
            goto out;

        display_dirname = g_filename_display_name (dirname);

        if (moo_overwrite_file_dialog (text, display_dirname, dialog ? dialog : parent))
        {
            g_free (display_dirname);
            goto out;
        }

        g_free (display_dirname);
        g_free (fullname);
        fullname = nullptr;
    }

out:
    if (dialog)
        gtk_widget_destroy (dialog);
    return fullname;
}


static void
moo_file_selector_select_path (MooFileSelector *filesel,
                               const char      *filename)
{
    char *basename;

    g_return_if_fail (filename != nullptr);

    basename = g_path_get_basename (filename);
    _moo_file_view_select_name (MOO_FILE_VIEW (filesel), basename);

    g_free (basename);
}

static void
moo_file_selector_select_file (MooFileSelector *filesel,
                               GFile           *file)
{
    char *filename = g_file_get_path (file);
    moo_file_selector_select_path (filesel, filename);
    g_free (filename);
}


static gboolean
save_as_path (MooEdit    *doc,
              const char *path)
{
    MooSaveInfo *info;
    gboolean result;
    info = moo_save_info_new (path, nullptr);
    result = moo_edit_save_as (doc, info, nullptr);
    g_object_unref (info);
    return result;
}


static gboolean
drop_untitled (MooFileSelector *filesel,
               MooEdit         *doc,
               const char      *destdir,
               GtkWidget       *widget,
               G_GNUC_UNUSED GdkDragContext *context,
               G_GNUC_UNUSED int             x,
               G_GNUC_UNUSED int             y,
               G_GNUC_UNUSED guint time)
{
    char *filename;
    gboolean result;

    filename = save_as_dialog (widget, destdir,
                               moo_edit_get_display_basename (doc),
                               TRUE, _("Save As"));

    if (!filename)
        return FALSE;

    result = save_as_path (doc, filename);

    if (result)
        moo_file_selector_select_path (filesel, filename);

    g_free (filename);
    return result;
}


static void
doc_save_as (MooFileSelector *filesel,
             MooEdit         *doc,
             gboolean         ask_name,
             const char      *destdir)
{
    char *filename;

    filename = save_as_dialog (GTK_WIDGET (filesel), destdir,
                               moo_edit_get_display_basename (doc),
                               ask_name, _("Save As"));

    if (filename)
    {
        if (save_as_path (doc, filename))
            moo_file_selector_select_path (filesel, filename);
        g_free (filename);
    }
}

static void
doc_save_copy (MooFileSelector *filesel,
               MooEdit         *doc,
               gboolean         ask_name,
               const char      *destdir)
{
    char *filename;

    filename = save_as_dialog (GTK_WIDGET (filesel), destdir,
                               moo_edit_get_display_basename (doc),
                               ask_name, _("Save Copy As"));

    if (filename)
    {
        MooSaveInfo *info = moo_save_info_new (filename, nullptr);
        if (moo_edit_save_copy (doc, info, nullptr))
            moo_file_selector_select_path (filesel, filename);
        g_object_unref (info);
        g_free (filename);
    }
}

static void
doc_move (MooFileSelector *filesel,
          MooEdit         *doc,
          gboolean         ask_name,
          const char      *destdir)
{
    GFile *old_file;
    char *filename;

    old_file = moo_edit_get_file (doc);
    g_return_if_fail (old_file != nullptr);

    filename = save_as_dialog (GTK_WIDGET (filesel), destdir,
                               moo_edit_get_display_basename (doc),
                               ask_name, _("Rename To"));

    if (filename)
    {
        if (save_as_path (doc, filename))
        {
            g_file_delete (old_file, nullptr, nullptr);
            moo_file_selector_select_path (filesel, filename);
        }

        g_free (filename);
    }

    g_object_unref (old_file);
}


typedef enum {
    DROP_DOC_ASK = 1,
    DROP_DOC_SAVE_AS,
    DROP_DOC_SAVE_COPY,
    DROP_DOC_MOVE
} DropDocAction;


static void
drop_item_activated (GObject         *item,
                     MooFileSelector *filesel)
{
    DropDocAction action;
    gpointer data;
    MooEdit *doc;
    char *destdir;
    gboolean alternate;

    data = g_object_get_data (item, "moo-file-selector-drop-action");
    doc = (MooEdit*) g_object_get_data (item, "moo-file-selector-drop-doc");
    destdir = (char*) g_object_get_data (item, "moo-file-selector-drop-destdir");
    alternate = GPOINTER_TO_INT (g_object_get_data (item, "moo-menu-item-alternate"));
    g_return_if_fail (doc != nullptr && destdir != nullptr);

    action = (DropDocAction) GPOINTER_TO_INT (data);

    switch (action)
    {
        case DROP_DOC_SAVE_AS:
            doc_save_as (filesel, doc, alternate, destdir);
            break;
        case DROP_DOC_SAVE_COPY:
            doc_save_copy (filesel, doc, alternate, destdir);
            break;
        case DROP_DOC_MOVE:
            doc_move (filesel, doc, alternate, destdir);
            break;
        default:
            g_return_if_reached ();
    }
}


static GtkWidget *
create_menu_item (MooFileSelector *filesel,
                  MooEdit         *doc,
                  const char      *destdir,
                  const char      *stock_icon,
                  const char      *label,
                  const char      *alternate_label,
                  const char      *accel_label,
                  DropDocAction    action)
{
    GtkWidget *item;

    item = gtk_image_menu_item_new_with_label (label);

    if (stock_icon)
    {
        GtkWidget *icon = gtk_image_new_from_stock (stock_icon, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), icon);
    }

    g_object_set_data_full (G_OBJECT (item), "moo-menu-item-label",
                            g_strdup (label), g_free);
    g_object_set_data_full (G_OBJECT (item), "moo-menu-item-alternate-label",
                            g_strdup (alternate_label), g_free);

    g_object_set_data_full (G_OBJECT (item), "moo-file-selector-drop-doc",
                            g_object_ref (doc), g_object_unref);
    g_object_set_data_full (G_OBJECT (item), "moo-file-selector-drop-destdir",
                            g_strdup (destdir), g_free);
    g_object_set_data (G_OBJECT (item), "moo-file-selector-drop-action",
                       GINT_TO_POINTER (action));

    g_signal_connect (item, "activate", G_CALLBACK (drop_item_activated), filesel);
    gtk_widget_show (item);
    _moo_menu_item_set_accel_label (item, accel_label);

    return item;
}

static void
alternate_toggled (GtkWidget *menu)
{
    GdkModifierType mask;
    gboolean alternate;
    GSList *items, *l;

    mask = _moo_get_modifiers (menu);
    alternate = (mask & GDK_SHIFT_MASK) != 0;

    items = (GSList*) g_object_get_data (G_OBJECT (menu), "moo-menu-items");

    for (l = items; l != nullptr; l = l->next)
    {
        GtkWidget *item = (GtkWidget*) l->data;
        const char *label;

        if (alternate)
            label = (char*) g_object_get_data (G_OBJECT (item), "moo-menu-item-alternate-label");
        else
            label = (char*) g_object_get_data (G_OBJECT (item), "moo-menu-item-label");

        g_object_set_data (G_OBJECT (item), "moo-menu-item-alternate",
                           GINT_TO_POINTER (alternate));
        _moo_menu_item_set_label (item, label, FALSE);
    }
}

static GtkWidget *
create_drop_doc_menu (MooFileSelector *filesel,
                      MooEdit         *doc,
                      const char      *destdir)
{
    GtkWidget *menu, *item;
    GSList *items = nullptr;

    menu = moo_menu_new ();
    g_signal_connect (menu, "alternate-toggled", G_CALLBACK (alternate_toggled), nullptr);

    item = create_menu_item (filesel, doc, destdir,
                             MOO_STOCK_FILE_MOVE,
                             _("Move Here"),
                             _("Move/Rename..."),
                             "Shift",
                             DROP_DOC_MOVE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    items = g_slist_prepend (items, item);

    item = create_menu_item (filesel, doc, destdir,
                             MOO_STOCK_FILE_SAVE_AS,
                             _("Save Here"),
                             _("Save As..."),
                             "Control",
                             DROP_DOC_SAVE_AS);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    items = g_slist_prepend (items, item);

    item = create_menu_item (filesel, doc, destdir,
                             MOO_STOCK_FILE_SAVE_COPY,
                             _("Save Copy"),
                             _("Save Copy As..."),
                             "Control+Shift",
                             DROP_DOC_SAVE_COPY);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    items = g_slist_prepend (items, item);

    g_object_set_data_full (G_OBJECT (menu), "moo-menu-items",
                            items, (GDestroyNotify) g_slist_free);

    item = gtk_separator_menu_item_new ();
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CANCEL, nullptr);
    gtk_widget_show (item);
    _moo_menu_item_set_accel_label (item, "Escape");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    return menu;
}


static void
moo_file_selector_drop_doc (MooFileSelector *filesel,
                            MooEdit         *doc,
                            const char      *destdir,
                            GtkWidget       *widget,
                            GdkDragContext  *context,
                            int              x,
                            int              y,
                            guint            time)
{
    GdkModifierType mods;
    DropDocAction action;

    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (destdir != nullptr);
    g_return_if_fail (GTK_IS_WIDGET (widget));

    if (moo_edit_is_untitled (doc))
    {
        gboolean result = drop_untitled (filesel, doc, destdir,
                                         widget, context, x, y, time);
        _moo_file_view_drag_finish (MOO_FILE_VIEW (filesel), context, result, FALSE, time);
        return;
    }

    mods = _moo_get_modifiers (widget) & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK);

    if (!mods || mods & GDK_MOD1_MASK)
        action = DROP_DOC_ASK;
    else if (mods == (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
        action = DROP_DOC_SAVE_COPY;
    else if (mods & GDK_SHIFT_MASK)
        action = DROP_DOC_MOVE;
    else
        action = DROP_DOC_SAVE_AS;

    if (action == DROP_DOC_ASK)
    {
        GtkWidget *menu = create_drop_doc_menu (filesel, doc, destdir);
        g_object_ref_sink (menu);
        _moo_file_view_drag_finish (MOO_FILE_VIEW (filesel), context, TRUE, FALSE, time);
        gtk_menu_popup (GTK_MENU (menu), nullptr, nullptr, nullptr, nullptr, 0, 0);
        g_object_unref (menu);
        return;
    }

    _moo_file_view_drag_finish (MOO_FILE_VIEW (filesel), context, TRUE, FALSE, time);

    switch (action)
    {
        case DROP_DOC_SAVE_AS:
            doc_save_as (filesel, doc, FALSE, destdir);
            break;
        case DROP_DOC_SAVE_COPY:
            doc_save_copy (filesel, doc, FALSE, destdir);
            break;
        case DROP_DOC_MOVE:
            doc_move (filesel, doc, FALSE, destdir);
            break;
        default:
            g_return_if_reached ();
    }
}


/****************************************************************************/
/* Plugin
 */

static gboolean
file_selector_plugin_init (G_GNUC_UNUSED Plugin *plugin)
{
    moo_prefs_create_key (PREFS_LAST_DIR, MOO_PREFS_STATE, G_TYPE_STRING, nullptr);
    moo_prefs_create_key (PREFS_HIDDEN_FILES, MOO_PREFS_RC, G_TYPE_BOOLEAN, FALSE);
    return TRUE;
}


static void
file_selector_plugin_deinit (Plugin *plugin)
{
    if (plugin->bookmark_mgr)
        g_object_unref (plugin->bookmark_mgr);
    plugin->bookmark_mgr = nullptr;
}


static void
file_selector_plugin_attach (MooPlugin     *mplugin,
                             MooEditWindow *window)
{
    GtkWidget *filesel;
    Plugin *plugin = FILE_SELECTOR_PLUGIN (mplugin);

    if (!plugin->bookmark_mgr)
        plugin->bookmark_mgr = _moo_bookmark_mgr_new ();

    /* it attaches itself to window */
    filesel = GTK_WIDGET (
        g_object_new (MOO_TYPE_FILE_SELECTOR,
                      "bookmark-mgr", plugin->bookmark_mgr,
                      "window", window,
                      (const char*) nullptr));

    plugin->instances = g_slist_prepend (plugin->instances, filesel);
}


static void
file_selector_plugin_detach (MooPlugin     *mplugin,
                             MooEditWindow *window)
{
    Plugin *plugin = FILE_SELECTOR_PLUGIN (mplugin);
    GtkWidget *filesel = moo_edit_window_get_pane (window, MOO_FILE_SELECTOR_PLUGIN_ID);

    g_return_if_fail (filesel != nullptr);

    plugin->instances = g_slist_remove (plugin->instances, filesel);
    moo_edit_window_remove_pane (window, MOO_FILE_SELECTOR_PLUGIN_ID);
}


void
_moo_file_selector_update_tools (MooPlugin *plugin)
{
    GSList *l;
    for (l = ((Plugin*)plugin)->instances; l != nullptr; l = l->next)
        _moo_file_view_tools_load ((MooFileView*) l->data);
}


MOO_PLUGIN_DEFINE_INFO (file_selector,
                        N_("File Selector"),
                        N_("File selector pane for editor window"),
                        "Yevgen Muntyan <emuntyan@users.sourceforge.net>",
                        MOO_VERSION)
MOO_PLUGIN_DEFINE (FileSelector, file_selector,
                   file_selector_plugin_attach, file_selector_plugin_detach,
                   nullptr, nullptr,
                   _moo_file_selector_prefs_page,
                   0, 0)


static gpointer
get_widget_meth (G_GNUC_UNUSED gpointer plugin,
                 MooEditWindow *window)
{
    gpointer widget = moo_edit_window_get_pane (window, MOO_FILE_SELECTOR_PLUGIN_ID);
    return widget ? g_object_ref (widget) : nullptr;
}


gboolean
_moo_file_selector_plugin_init (void)
{
    GType ptype = file_selector_plugin_get_type ();

    if (!moo_plugin_register (MOO_FILE_SELECTOR_PLUGIN_ID,
                              ptype,
                              &file_selector_plugin_info,
                              nullptr))
        return FALSE;

    moo_plugin_method_new ("get-widget", ptype,
                           G_CALLBACK (get_widget_meth),
                           _moo_marshal_OBJECT__OBJECT,
                           _moo_file_selector_get_type (),
                           1, MOO_TYPE_EDIT_WINDOW);

    return TRUE;
}
