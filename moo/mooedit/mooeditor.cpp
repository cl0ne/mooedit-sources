/*
 *   mooeditor.c
 *
 *   Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
 *   Copyright (C) 2014 by Ulrich Eckhardt <ulrich.eckhardt@base-42.de>
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
 * class:MooEditor: (parent GObject): editor object
 **/

#include "mooedit/mooeditor-private.h"
#include "mooedit/mooeditwindow-impl.h"
#include "mooedit/mooeditdialogs.h"
#include "mooedit/mooedit-fileops.h"
#include "mooedit/mooplugin.h"
#include "mooedit/mooeditprefs.h"
#include "mooedit/mooedit-impl.h"
#include "mooedit/mooeditview-impl.h"
#include "mooedit/mooedit-accels.h"
#include "mooedit/mooeditfiltersettings.h"
#include "mooedit/mooeditfileinfo-impl.h"
#include "mooedit/mooedithistoryitem.h"
#include "mooedit/mooedit-ui.h"
#include "mooedit/medit-ui.h"
#include "mooutils/moomenuaction.h"
#include "marshals.h"
#include "mooutils/mooutils-cpp.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooaction-private.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/mooutils-enums.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/moostock.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooencodings.h"
#include "mooutils/moolist.h"
#include <mooglib/moo-glib.h>
#include <string.h>
#include <stdlib.h>

#define RECENT_ACTION_ID "OpenRecent"
#define RECENT_DIALOG_ACTION_ID "OpenRecentDialog"

#define CURRENT_SESSION_VERSION "2.0"

MOO_DEFINE_QUARK (moo-edit-reload-error, moo_edit_reload_error_quark)
MOO_DEFINE_QUARK (moo-edit-save-error, moo_edit_save_error_quark)

static MooEditor* editor_instance = NULL;

static void          set_single_window          (MooEditor      *editor,
                                                 gboolean        single);

static GtkAction    *create_open_recent_action  (MooWindow      *window,
                                                 gpointer        user_data);
static void          action_recent_dialog       (MooEditWindow  *window);

static MooEditWindow *create_window             (MooEditor      *editor);
static void          moo_editor_add_doc         (MooEditor      *editor,
                                                 MooEditWindow  *window,
                                                 MooEdit        *doc);
static MooSaveResponse moo_editor_before_save   (MooEditor    *editor,
                                                 MooEdit        *doc,
                                                 GFile          *file);
static void          moo_editor_will_save       (MooEditor    *editor,
                                                 MooEdit        *doc,
                                                 GFile          *file);
static MooCloseResponse moo_editor_before_close_window
                                                (MooEditor      *editor,
                                                 MooEditWindow  *window);
static void          do_close_window            (MooEditor      *editor,
                                                 MooEditWindow  *window);
static void          do_close_doc               (MooEditor      *editor,
                                                 MooEdit        *doc);
static gboolean      close_docs_real            (MooEditor      *editor,
                                                 MooEditArray   *docs);
static MooEditArray *find_modified              (MooEditArray   *docs);

static void          add_new_window_action      (void);
static void          remove_new_window_action   (void);

static GObject      *moo_editor_constructor     (GType           type,
                                                 guint           n_props,
                                                 GObjectConstructParam *props);
static void          moo_editor_finalize        (GObject        *object);
static void          moo_editor_set_property    (GObject        *object,
                                                 guint           prop_id,
                                                 const GValue   *value,
                                                 GParamSpec     *pspec);
static void          moo_editor_get_property    (GObject        *object,
                                                 guint           prop_id,
                                                 GValue         *value,
                                                 GParamSpec     *pspec);


enum {
    PROP_0,
    PROP_OPEN_SINGLE_FILE_INSTANCE,
    PROP_ALLOW_EMPTY_WINDOW,
    PROP_SINGLE_WINDOW,
    PROP_SAVE_BACKUPS,
    PROP_STRIP_WHITESPACE,
    PROP_EMBEDDED
};

enum {
    BEFORE_CLOSE_WINDOW,
    WILL_CLOSE_WINDOW,
    AFTER_CLOSE_WINDOW,
    WILL_CLOSE_DOC,
    BEFORE_SAVE,
    WILL_SAVE,
    AFTER_SAVE,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


/* MOO_TYPE_EDITOR */
G_DEFINE_TYPE (MooEditor, moo_editor, G_TYPE_OBJECT)


inline static gboolean test_flag(MooEditor *editor, MooEditorOptions flag)
{
    return (editor->priv->opts & flag) != 0;
}

inline static gboolean is_embedded(MooEditor *editor)
{
    return test_flag(editor, EMBEDDED);
}

inline static void set_flag(MooEditor *editor, MooEditorOptions flag, gboolean set_or_not)
{
    if (set_or_not)
        editor->priv->opts |= flag;
    else
        editor->priv->opts &= ~flag;
}

static void
moo_editor_class_init (MooEditorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    G_GNUC_UNUSED MooWindowClass *edit_window_class;

    gobject_class->constructor = moo_editor_constructor;
    gobject_class->finalize = moo_editor_finalize;
    gobject_class->set_property = moo_editor_set_property;
    gobject_class->get_property = moo_editor_get_property;

    klass->before_close_window = moo_editor_before_close_window;
    klass->before_save = moo_editor_before_save;
    klass->will_save = moo_editor_will_save;

    _moo_edit_init_config ();
    g_type_class_unref (g_type_class_ref (MOO_TYPE_EDIT));
    g_type_class_unref (g_type_class_ref (MOO_TYPE_EDIT_WINDOW));
    g_type_class_unref (g_type_class_ref (MOO_TYPE_INDENTER));

    g_type_class_add_private (klass, sizeof (MooEditorPrivate));

    g_object_class_install_property (gobject_class, PROP_OPEN_SINGLE_FILE_INSTANCE,
        g_param_spec_boolean ("open-single-file-instance", "open-single-file-instance", "open-single-file-instance",
                              TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_ALLOW_EMPTY_WINDOW,
        g_param_spec_boolean ("allow-empty-window", "allow-empty-window", "allow-empty-window",
                              FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_SINGLE_WINDOW,
        g_param_spec_boolean ("single-window", "single-window", "single-window",
                              FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_SAVE_BACKUPS,
        g_param_spec_boolean ("save-backups", "save-backups", "save-backups",
                              FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_STRIP_WHITESPACE,
        g_param_spec_boolean ("strip-whitespace", "strip-whitespace", "strip-whitespace",
                              FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_EMBEDDED,
        g_param_spec_boolean ("embedded", "embedded", "embedded",
                              FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    signals[BEFORE_CLOSE_WINDOW] =
            g_signal_new ("before-close-window",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditorClass, before_close_window),
                          moo_signal_accumulator_continue_cancel,
                          GINT_TO_POINTER (MOO_CLOSE_RESPONSE_CONTINUE),
                          _moo_marshal_ENUM__OBJECT,
                          MOO_TYPE_CLOSE_RESPONSE, 1,
                          MOO_TYPE_EDIT_WINDOW);

    /**
     * MooEditor::will-close-window:
     *
     * @editor: the object which received the signal
     * @window: the window which is about to be closed
     *
     * This signal is emitted before the window is closed.
     **/
    signals[WILL_CLOSE_WINDOW] =
            g_signal_new ("will-close-window",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditorClass, will_close_window),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT,
                          G_TYPE_NONE, 1,
                          MOO_TYPE_EDIT_WINDOW);

    signals[AFTER_CLOSE_WINDOW] =
            g_signal_new ("after-close-window",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditorClass, after_close_window),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    /**
     * MooEditor::will-close-doc:
     *
     * @editor: the object which received the signal
     * @doc: the document which is about to be closed
     *
     * This signal is emitted before the document is closed.
     **/
    signals[WILL_CLOSE_DOC] =
            g_signal_new ("will-close-doc",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditorClass, will_close_doc),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT,
                          G_TYPE_NONE, 1,
                          MOO_TYPE_EDIT);

    /**
     * MooEditor::before-save:
     *
     * @editor: the object which received the signal
     * @doc: the document which is about to be saved on disk
     * @file: the #GFile object which represents saved file
     *
     * This signal is emitted when the document is going to be saved on disk.
     * Callbacks must return #MOO_SAVE_RESPONSE_CANCEL if document
     * should not be saved, and #MOO_SAVE_RESPONSE_CONTINUE otherwise.
     *
     * For example, if before saving the file must be checked out from a version
     * control system, a callback can do that and return #MOO_SAVE_RESPONSE_CANCEL
     * if checkout failed.
     *
     * Callbacks must not modify document content. If you need to modify
     * it before saving, use MooEditor::will-save signal instead.
     *
     * Returns: #MOO_SAVE_RESPONSE_CANCEL to cancel saving,
     * #MOO_SAVE_RESPONSE_CONTINUE otherwise.
     **/
    signals[BEFORE_SAVE] =
            g_signal_new ("before-save",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditorClass, before_save),
                          moo_signal_accumulator_continue_cancel,
                          GINT_TO_POINTER (MOO_SAVE_RESPONSE_CONTINUE),
                          _moo_marshal_ENUM__OBJECT_OBJECT,
                          MOO_TYPE_SAVE_RESPONSE, 2,
                          MOO_TYPE_EDIT,
                          G_TYPE_FILE);

    /**
     * MooEditor::will-save:
     *
     * @editor: the object which received the signal
     * @doc: the document which is about to be saved on disk
     * @file: the #GFile object which represents saved file
     *
     * This signal is emitted when the document is going to be saved on disk,
     * after MooEditor::before-save signal. Callbacks may modify document
     * content at this point.
     **/
    signals[WILL_SAVE] =
            g_signal_new ("will-save",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditorClass, will_save),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT_OBJECT,
                          G_TYPE_NONE, 2,
                          MOO_TYPE_EDIT,
                          G_TYPE_FILE);

    /**
     * MooEditor::after-save:
     *
     * @editor: the object which received the signal
     * @doc: the document which was saved on disk
     *
     * This signal is emitted after the document has been successfully saved on disk.
     **/
    signals[AFTER_SAVE] =
            g_signal_new ("after-save",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditorClass, after_save),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT,
                          G_TYPE_NONE, 1,
                          MOO_TYPE_EDIT);

    edit_window_class = MOO_WINDOW_CLASS (g_type_class_ref (MOO_TYPE_EDIT_WINDOW));
    moo_window_class_new_action_custom (edit_window_class, RECENT_ACTION_ID, NULL,
                                        create_open_recent_action,
                                        NULL, NULL);
    moo_window_class_new_action (edit_window_class, RECENT_DIALOG_ACTION_ID, NULL,
                                 "display-name", _("Open Recent Files Dialog"),
                                 /* Translators: remove the part before and including | */
                                 "label", Q_("Open Recent|_More..."),
                                 "default-accel", MOO_EDIT_ACCEL_OPEN_RECENT_DIALOG,
                                 "closure-callback", action_recent_dialog,
                                 nullptr);
    g_type_class_unref (edit_window_class);

    add_new_window_action ();
}


static void
moo_editor_init (MooEditor *editor)
{
    editor->priv = G_TYPE_INSTANCE_GET_PRIVATE (editor, MOO_TYPE_EDITOR, MooEditorPrivate);
    editor->priv->windows = moo_edit_window_array_new ();
    editor->priv->windowless = moo_edit_array_new ();
}

static GObject *
moo_editor_constructor (GType                  type,
                        guint                  n_props,
                        GObjectConstructParam *props)
{
    MooEditor *editor;
    GObject *object;

    object = G_OBJECT_CLASS (moo_editor_parent_class)->constructor (type, n_props, props);
    editor = MOO_EDITOR (object);

    _moo_stock_init ();

    editor->priv->doc_ui_xml = moo_ui_xml_new ();
    moo_ui_xml_add_ui_from_string (editor->priv->doc_ui_xml,
                                   mooedit_ui_xml, -1);

    editor->priv->lang_mgr = g::object_ref (moo_lang_mgr_default ());
    g_signal_connect_swapped (editor->priv->lang_mgr, "loaded",
                              G_CALLBACK (_moo_editor_apply_prefs),
                              editor);

    editor->priv->history = NULL;
    if (!is_embedded (editor))
        editor->priv->history = MOO_HISTORY_MGR (
            g_object_new (MOO_TYPE_HISTORY_MGR,
                          "name", "Editor",
                          (const char*) NULL));

    _moo_edit_filter_settings_load ();
    _moo_editor_apply_prefs (editor);

    return object;
}


static void
moo_editor_set_property (GObject        *object,
                         guint           prop_id,
                         const GValue   *value,
                         GParamSpec     *pspec)
{
    MooEditor *editor = MOO_EDITOR (object);

    switch (prop_id) {
        case PROP_OPEN_SINGLE_FILE_INSTANCE:
            set_flag (editor, OPEN_SINGLE, g_value_get_boolean (value));
            break;

        case PROP_ALLOW_EMPTY_WINDOW:
            set_flag (editor, ALLOW_EMPTY_WINDOW, g_value_get_boolean (value));
            break;

        case PROP_SINGLE_WINDOW:
            set_single_window (editor, g_value_get_boolean (value));
            break;

        case PROP_SAVE_BACKUPS:
            set_flag (editor, SAVE_BACKUPS, g_value_get_boolean (value));
            break;

        case PROP_STRIP_WHITESPACE:
            set_flag (editor, STRIP_WHITESPACE, g_value_get_boolean (value));
            break;

        case PROP_EMBEDDED:
            set_flag (editor, EMBEDDED, g_value_get_boolean (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_editor_get_property (GObject        *object,
                         guint           prop_id,
                         GValue         *value,
                         GParamSpec     *pspec)
{
    MooEditor *editor = MOO_EDITOR (object);

    switch (prop_id) {
        case PROP_OPEN_SINGLE_FILE_INSTANCE:
            g_value_set_boolean (value, test_flag (editor, OPEN_SINGLE));
            break;

        case PROP_ALLOW_EMPTY_WINDOW:
            g_value_set_boolean (value, test_flag (editor, ALLOW_EMPTY_WINDOW));
            break;

        case PROP_SINGLE_WINDOW:
            g_value_set_boolean (value, test_flag (editor, SINGLE_WINDOW));
            break;

        case PROP_SAVE_BACKUPS:
            g_value_set_boolean (value, test_flag (editor, SAVE_BACKUPS));
            break;

        case PROP_STRIP_WHITESPACE:
            g_value_set_boolean (value, test_flag (editor, STRIP_WHITESPACE));
            break;

        case PROP_EMBEDDED:
            g_value_set_boolean (value, test_flag (editor, EMBEDDED));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_editor_finalize (GObject *object)
{
    MooEditor *editor = MOO_EDITOR (object);

    if (editor_instance == editor)
        editor_instance = NULL;

    if (editor->priv->ui_xml)
        g_object_unref (editor->priv->ui_xml);
    if (editor->priv->history)
        g_object_unref (editor->priv->history);
    g_object_unref (editor->priv->lang_mgr);
    g_object_unref (editor->priv->doc_ui_xml);

    if (editor->priv->file_watch)
    {
        GError *error = NULL;

        if (!moo_file_watch_close (editor->priv->file_watch, &error))
        {
            g_warning ("error in moo_file_watch_close: %s", moo_error_message (error));
            g_error_free (error);
            error = NULL;
        }

        moo_file_watch_unref (editor->priv->file_watch);
    }

    if (editor->priv->windows->n_elms)
        g_critical ("finalizing editor while some windows are open");
    if (editor->priv->windowless->n_elms)
        g_critical ("finalizing editor while some documents are open");

    moo_edit_window_array_free (editor->priv->windows);
    moo_edit_array_free (editor->priv->windowless);

    G_OBJECT_CLASS (moo_editor_parent_class)->finalize (object);
}


MooFileWatch *
_moo_editor_get_file_watch (MooEditor *editor)
{
    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);

    if (!editor->priv->file_watch)
        editor->priv->file_watch = moo_file_watch_new (NULL);

    return editor->priv->file_watch;
}


/**
 * moo_editor_create_instance: (static-method-of MooEditor) (moo.lua 0)
 *
 * @embedded: (default TRUE):
 *
 * Returns: (transfer full)
 */
MooEditor *
moo_editor_create_instance (gboolean embedded)
{
    if (!editor_instance)
    {
        editor_instance = MOO_EDITOR (g_object_new (MOO_TYPE_EDITOR,
                                                    "embedded", embedded,
                                                    (const char*) NULL));
        g_object_add_weak_pointer (G_OBJECT (editor_instance), reinterpret_cast<gpointer*> (&editor_instance));
    }
    else
    {
        g_object_ref (editor_instance);
    }

    return editor_instance;
}


/**
 * moo_editor_instance: (static-method-of MooEditor)
 **/
MooEditor *
moo_editor_instance (void)
{
    return editor_instance;
}


static GType
get_window_type (MooEditor *editor)
{
    return editor->priv->window_type ?
            editor->priv->window_type : MOO_TYPE_EDIT_WINDOW;
}


static GType
get_doc_type (MooEditor *editor)
{
    return editor->priv->doc_type ?
            editor->priv->doc_type : MOO_TYPE_EDIT;
}


static void
set_single_window (MooEditor      *editor,
                   gboolean        single)
{
    /* XXX move documents to some window if several windows open? */
    set_flag (editor, SINGLE_WINDOW, single);

    if (single)
        remove_new_window_action ();
    else
        add_new_window_action ();

    g_object_notify (G_OBJECT (editor), "single-window");
}


static void
add_new_window_action (void)
{
    MooWindowClass *klass;

    klass = MOO_WINDOW_CLASS (g_type_class_peek (MOO_TYPE_EDIT_WINDOW));

    if (!moo_window_class_find_action (klass, "NewWindow"))
        moo_window_class_new_action (klass, "NewWindow", NULL,
                                     "display-name", MOO_STOCK_NEW_WINDOW,
                                     "label", MOO_STOCK_NEW_WINDOW,
                                     "tooltip", _("Open new editor window"),
                                     "stock-id", MOO_STOCK_NEW_WINDOW,
                                     "default-accel", MOO_EDIT_ACCEL_NEW_WINDOW,
                                     "closure-callback", moo_editor_new_window,
                                     "closure-proxy-func", moo_edit_window_get_editor,
                                     nullptr);
}


static void
remove_new_window_action (void)
{
    MooWindowClass *klass;
    klass = MOO_WINDOW_CLASS (g_type_class_peek (MOO_TYPE_EDIT_WINDOW));
    moo_window_class_remove_action (klass, "NewWindow");
}


static MooEditWindow *
get_top_window (MooEditor *editor)
{
    GtkWindow *window;
    GSList *window_list;
    guint i;

    if (moo_edit_window_array_is_empty (editor->priv->windows))
        return NULL;

    for (window_list = NULL, i = 0; i < editor->priv->windows->n_elms; ++i)
        window_list = g_slist_prepend (window_list, editor->priv->windows->elms[i]);
    window_list = g_slist_reverse (window_list);

    window = _moo_get_top_window (window_list);

    if (!window)
        window = GTK_WINDOW (editor->priv->windows->elms[0]);

    g_slist_free (window_list);

    return MOO_EDIT_WINDOW (window);
}


/**
 * moo_editor_get_ui_xml: (moo.private 1)
 */
MooUiXml *
moo_editor_get_ui_xml (MooEditor *editor)
{
    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);

    if (!editor->priv->ui_xml)
    {
        editor->priv->ui_xml = moo_ui_xml_new ();
        moo_ui_xml_add_ui_from_string (editor->priv->ui_xml, medit_ui_xml, -1);
    }

    return editor->priv->ui_xml;
}


/**
 * moo_editor_get_doc_ui_xml: (moo.private 1)
 */
MooUiXml *
moo_editor_get_doc_ui_xml (MooEditor *editor)
{
    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    return editor->priv->doc_ui_xml;
}


/**
 * moo_editor_set_ui_xml: (moo.private 1)
 */
void
moo_editor_set_ui_xml (MooEditor      *editor,
                       MooUiXml       *xml)
{
    guint i;

    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (MOO_IS_UI_XML (xml));

    if (editor->priv->ui_xml == xml)
        return;

    if (editor->priv->ui_xml)
        g_object_unref (editor->priv->ui_xml);

    editor->priv->ui_xml = xml;

    if (editor->priv->ui_xml)
        g_object_ref (editor->priv->ui_xml);

    for (i = 0; i < editor->priv->windows->n_elms; ++i)
        moo_window_set_ui_xml (MOO_WINDOW (editor->priv->windows->elms[i]),
                               editor->priv->ui_xml);
}


MooHistoryMgr *
_moo_editor_get_history_mgr (MooEditor *editor)
{
    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    return editor->priv->history;
}

// static void
// add_recent_uri (MooEditor  *editor,
//                 const char *uri)
// {
//     if (!is_embedded (editor))
//         moo_history_mgr_add_uri (editor->priv->history, uri);
// }

static void
recent_item_activated (GSList   *items,
                       gpointer  data)
{
    MooEditWindow *window = MOO_EDIT_WINDOW (data);
    MooEditor *editor = moo_editor_instance ();

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDITOR (editor));

    while (items)
    {
        const char *encoding;
        const char *uri;
        char *filename;
        MooHistoryItem *item = static_cast<MooHistoryItem*>(items->data);

        uri = moo_history_item_get_uri (item);
        filename = g_filename_from_uri (uri, NULL, NULL);
        g_return_if_fail (filename != NULL);

        encoding = _moo_edit_history_item_get_encoding (item);
        if (!moo_editor_open_uri (editor, uri, encoding, -1, window))
            moo_history_mgr_remove_uri (editor->priv->history, uri);

        g_free (filename);

        items = items->next;
    }
}

static GtkWidget *
create_recent_menu (GtkAction *action)
{
    GtkWidget *menu, *item;
    GtkAction *action_more;
    MooWindow *window;
    MooEditor *editor;

    window = MOO_WINDOW (_moo_action_get_window (action));
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), NULL);

    editor = moo_editor_instance ();
    menu = moo_history_mgr_create_menu (editor->priv->history,
                                       recent_item_activated,
                                       window, NULL);
    moo_bind_bool_property (action,
                            "sensitive", editor->priv->history,
                            "empty", TRUE);

    item = gtk_separator_menu_item_new ();
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    action_more = moo_window_get_action (window, RECENT_DIALOG_ACTION_ID);
    item = gtk_action_create_menu_item (action_more);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);

    return item;
}

static GtkAction *
create_open_recent_action (G_GNUC_UNUSED MooWindow *window,
                           G_GNUC_UNUSED gpointer   user_data)
{
    GtkAction *action;

    action = moo_menu_action_new ("OpenRecent", _("Open Recent"));
    moo_menu_action_set_func (MOO_MENU_ACTION (action), create_recent_menu);

    return action;
}

static void
action_recent_dialog (MooEditWindow *window)
{
    GtkWidget *dialog;
    MooEditor *editor;

    editor = moo_editor_instance ();
    g_return_if_fail (MOO_IS_EDITOR (editor));

    dialog = moo_history_mgr_create_dialog (editor->priv->history,
                                           recent_item_activated,
                                           window, NULL);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
    _moo_window_set_remember_size (GTK_WINDOW (dialog),
                                   moo_edit_setting (MOO_EDIT_PREFS_DIALOGS "/recent-files"),
                                   400, 350,
                                   FALSE);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}


/*****************************************************************************/

static MooEditWindow *
create_window (MooEditor *editor)
{
    MooEditWindow *window = MOO_EDIT_WINDOW (
        g_object_new (get_window_type (editor),
                      "editor", editor,
                      "ui-xml", moo_editor_get_ui_xml (editor),
                      (const char*) NULL));
    moo_edit_window_array_append (editor->priv->windows, window);
    _moo_window_attach_plugins (window);
    gtk_widget_show (GTK_WIDGET (window));
    return window;
}


static void
moo_editor_add_doc (MooEditor      *editor,
                    MooEditWindow  *window,
                    MooEdit        *doc)
{
    if (!window)
        moo_edit_array_append (editor->priv->windowless, doc);
}


/**
 * moo_editor_new_window:
 */
MooEditWindow *
moo_editor_new_window (MooEditor *editor)
{
    MooEditWindow *window;
    MooEdit *doc;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);

    window = create_window (editor);

    if (!test_flag (editor, ALLOW_EMPTY_WINDOW))
    {
        doc = MOO_EDIT (g_object_new (get_doc_type (editor), "editor", editor, (const char*) NULL));
        _moo_edit_window_insert_doc (window, doc, NULL);
        moo_editor_add_doc (editor, window, doc);
        g_object_unref (doc);
    }

    return window;
}


// /* this creates MooEdit instance which can not be put into a window */
// MooEdit *
// moo_editor_create_doc (MooEditor      *editor,
//                        const char     *filename,
//                        const char     *encoding,
//                        GError        **error)
// {
//     MooEdit *doc;
//     GFile *file = NULL;
//
//     g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
//
//     doc = MOO_EDIT (g_object_new (get_doc_type (editor), "editor", editor, (const char*) NULL));
//
//     if (filename)
//         file = g_file_new_for_path (filename);
//
//     if (file && !_moo_edit_load_file (doc, file, encoding, NULL, error))
//     {
//         g_object_ref_sink (doc);
//         g_object_unref (file);
//         g_object_unref (doc);
//         return NULL;
//     }
//
//     moo_editor_add_doc (editor, NULL, doc);
//     _moo_doc_attach_plugins (NULL, doc);
//
//     g_object_unref (file);
//
//     return doc;
// }


/**
 * moo_editor_new_doc:
 *
 * @editor:
 * @window: (allow-none) (default NULL)
 */
MooEdit *
moo_editor_new_doc (MooEditor      *editor,
                    MooEditWindow  *window)
{
    MooEdit *doc;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    g_return_val_if_fail (!window || MOO_IS_EDIT_WINDOW (window), NULL);

    if (!window)
        window = get_top_window (editor);

    if (!window)
        window = moo_editor_new_window (editor);

    g_return_val_if_fail (window != NULL, NULL);

    doc = MOO_EDIT (g_object_new (get_doc_type (editor), "editor", editor, (const char*) NULL));
    _moo_edit_window_insert_doc (window, doc, NULL);
    moo_editor_add_doc (editor, window, doc);
    g_object_unref (doc);

    if (moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_AUTO_SYNC)))
    {
        // After creating the new document, prompt the user for a path to save
        // it to. If the user aborts, delete the document, too.
        if (!moo_editor_save_as (editor, doc, NULL, NULL))
        {
            moo_editor_close_doc (editor, doc);
            return NULL;
        }
    }

    return doc;
}


void
_moo_editor_move_doc (MooEditor     *editor,
                      MooEdit       *doc,
                      MooEditWindow *dest,
                      MooEditView   *dest_view,
                      gboolean       focus)
{
    MooEditWindow *old_window;
    MooEdit *dest_doc = NULL;
    MooEditTab *tab;

    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (MOO_IS_EDIT (doc) && moo_edit_get_editor (doc) == editor);
    g_return_if_fail (!dest || (MOO_IS_EDIT_WINDOW (dest) && moo_edit_window_get_editor (dest) == editor));

    if (!dest)
    {
        dest = moo_editor_new_window (editor);
        dest_view = moo_edit_window_get_active_view (dest);
    }

    tab = moo_edit_view_get_tab (moo_edit_get_view (doc));

    g_object_ref (tab);
    g_object_ref (doc);

    if ((old_window = moo_edit_get_window (doc)))
    {
        _moo_edit_window_remove_doc (old_window, doc);

        if (!moo_edit_window_get_active_doc (old_window))
            moo_editor_close_window (editor, old_window);
    }

    _moo_edit_window_insert_tab (dest, tab, dest_view);
    moo_editor_add_doc (editor, dest, doc);

    dest_doc = dest_view ? moo_edit_view_get_doc (dest_view) : NULL;
    if (dest_doc && moo_edit_is_empty (dest_doc))
        moo_editor_close_doc (editor, dest_doc);

    if (focus)
        moo_editor_set_active_doc (editor, doc);

    g_object_unref (doc);
    g_object_unref (tab);
}


static void
update_history_item_for_doc (MooEditor *editor,
                             MooEdit   *doc,
                             gboolean   add)
{
    MooHistoryItem *item;
    int line;
    const char *enc;
    MooEditView *view;

    if (is_embedded (editor))
        return;

    gstr uri = gstr::take (moo_edit_get_uri (doc));
    if (uri.empty())
        return;

    item = moo_history_item_new (uri.get(), NULL);

    view = moo_edit_get_view (doc);
    line = moo_text_view_get_cursor_line (GTK_TEXT_VIEW (view));
    if (line != 0)
        _moo_edit_history_item_set_line (item, line);

    enc = moo_edit_get_encoding (doc);
    if (enc && strcmp (enc, MOO_ENCODING_UTF8) != 0)
        _moo_edit_history_item_set_encoding (item, enc);

    if (add)
        moo_history_mgr_add_file (editor->priv->history, item);
    else
        moo_history_mgr_update_file (editor->priv->history, item);

    moo_history_item_free (item);
}


static MooEdit *
moo_editor_load_file (MooEditor       *editor,
                      MooOpenInfo     *info,
                      MooEditWindow   *window,
                      GtkWidget       *parent,
                      gboolean         silent,
                      gboolean         add_history,
                      GError         **error)
{
    MooEdit *doc;
    MooEditView *view = NULL;
    gboolean new_doc = FALSE;
    gboolean new_object = FALSE;
    const char *recent_encoding = NULL;
    GError *error_here = NULL;
    gboolean success = TRUE;
    char *uri;
    char *filename;
    int line;

    moo_return_error_if_fail_p (MOO_IS_EDITOR (editor));
    moo_return_error_if_fail_p (info != NULL && G_IS_FILE (info->file));

    uri = g_file_get_uri (info->file);
    filename = g_file_get_path (info->file);
    line = info->line;
    doc = moo_editor_get_doc_for_file (editor, info->file);

    if (!filename)
    {
        g_set_error (&error_here, MOO_EDIT_FILE_ERROR,
                     MOO_EDIT_FILE_ERROR_NOT_IMPLEMENTED,
                     "Loading remote files is not implemented");
        success = FALSE;
    }

    if (success && !doc)
    {
        new_doc = TRUE;

        if (!window)
            window = moo_editor_get_active_window (editor);

        if (window)
        {
            doc = moo_edit_window_get_active_doc (window);

            if (doc && !moo_edit_is_empty (doc))
                doc = NULL;
        }
    }

    if (success && !doc)
    {
        new_object = TRUE;
        doc = MOO_EDIT (g_object_new (get_doc_type (editor), "editor", editor, (const char*) NULL));
    }

    if (success)
    {
        view = moo_edit_get_view (doc);

        if (!new_doc && line < 0 && (info->flags & MOO_OPEN_FLAG_RELOAD) != 0)
            line = moo_text_view_get_cursor_line (GTK_TEXT_VIEW (view));

        if (!info->encoding)
        {
            MooHistoryItem *hist_item = moo_history_mgr_find_uri (editor->priv->history, uri);
            if (hist_item)
                recent_encoding = _moo_edit_history_item_get_encoding (hist_item);
        }
    }

    if (success && new_doc)
    {
        if ((info->flags & MOO_OPEN_FLAG_CREATE_NEW) && _moo_edit_file_is_new (info->file))
        {
            _moo_edit_set_status (doc, MOO_EDIT_STATUS_NEW);
            _moo_edit_set_file (doc, info->file, info->encoding);
        }
        else
        {
            success = _moo_edit_load_file (doc, info->file, info->encoding, recent_encoding, &error_here);
        }
    }

    if (!success)
    {
        if (!silent && !_moo_is_file_error_cancelled (error_here))
        {
            if (!parent && !window)
                window = moo_editor_get_active_window (editor);
            if (!parent && window)
                parent = GTK_WIDGET (window);
            _moo_edit_open_error_dialog (parent, info->file, error_here);
        }

        g_propagate_error (error, error_here);
        error_here = NULL;
    }
    else if (!new_doc && (info->flags & MOO_OPEN_FLAG_RELOAD))
    {
        success = _moo_edit_reload_file (doc, info->encoding, &error_here);

        if (!success)
        {
            if (!silent && !_moo_is_file_error_cancelled (error_here))
                _moo_edit_reload_error_dialog (doc, error_here);
            g_propagate_error (error, error_here);
            error_here = NULL;
        }
    }

    if (success && new_object)
    {
        if (!window || (info->flags & MOO_OPEN_FLAG_NEW_WINDOW))
            window = create_window (editor);

        _moo_edit_window_insert_doc (window, doc, NULL);
        moo_editor_add_doc (editor, window, doc);
    }

    if (success)
    {
        MooHistoryItem *hist_item;

        if (line < 0 && new_doc)
        {
            hist_item = moo_history_mgr_find_uri (editor->priv->history, uri);
            if (hist_item)
                line = _moo_edit_history_item_get_line (hist_item);
        }

        if (line >= 0)
            moo_text_view_move_cursor (MOO_TEXT_VIEW (view), line, 0, FALSE, TRUE);
    }

    if (success && add_history)
        update_history_item_for_doc (editor, doc, TRUE);

    if (success)
    {
        moo_editor_set_active_doc (editor, doc);
        gtk_widget_grab_focus (GTK_WIDGET (view));
    }

    if (new_object)
        g_object_unref (doc);

    g_free (filename);
    g_free (uri);
    return success ? doc : NULL;
}

// static MooEdit *
// moo_editor_load_file (MooEditor       *editor,
//                       MooOpenInfo *info,
//                       MooEditWindow   *window,
//                       GtkWidget       *parent,
//                       gboolean         silent,
//                       gboolean         add_history,
//                       GError         **error)
// {
//     GError *error = NULL;
//     gboolean new_doc = FALSE;
//     MooEdit *doc = NULL;
//     char *uri;
//     gboolean result = TRUE;
//     const char *recent_encoding = NULL;
//
//     *docp = NULL;
//     uri = g_file_get_uri (info->file);
//     g_return_val_if_fail (uri != NULL, FALSE);
//
//     if ((doc = moo_editor_get_doc_for_uri (editor, uri)))
//     {
//         *docp = doc;
//         if (add_history)
//             add_recent_uri (editor, uri);
//         g_free (uri);
//         return FALSE;
//     }
//
//     if (window)
//     {
//         doc = moo_edit_window_get_active_doc (window);
//
//         if (doc && moo_edit_is_empty (doc))
//             g_object_ref (doc);
//         else
//             doc = NULL;
//     }
//
//     if (!doc)
//     {
//         doc = MOO_EDIT (g_object_new (get_doc_type (editor), "editor", editor, (const char*) NULL));
//         g_object_ref_sink (doc);
//         new_doc = TRUE;
//     }
//
//     if (!info->encoding)
//     {
//         MooHistoryItem *hist_item = moo_history_mgr_find_uri (editor->priv->history, uri);
//         if (hist_item)
//             recent_encoding = _moo_edit_history_item_get_encoding (hist_item);
//     }
//
//     /* XXX open_single */
//     if (!_moo_edit_load_file (doc, info->file, info->encoding, recent_encoding, &error))
//     {
//         if (!silent)
//         {
//             if (!parent && !window)
//                 window = moo_editor_get_active_window (editor);
//             if (!parent && window)
//                 parent = GTK_WIDGET (window);
//             _moo_edit_open_error_dialog (parent, info->file, info->encoding, error);
//         }
//         g_error_free (error);
//         result = FALSE;
//     }
//     else
//     {
//         MooHistoryItem *hist_item;
//         int line = info->line;
//
//         if (line < 0)
//         {
//             hist_item = moo_history_mgr_find_uri (editor->priv->history, uri);
//             if (hist_item)
//                 line = _moo_edit_history_item_get_line (hist_item);
//         }
//
//         if (line >= 0)
//             moo_text_view_move_cursor (MOO_TEXT_VIEW (doc), line, 0, FALSE, TRUE);
//
//         if (!window)
//             window = moo_editor_get_active_window (editor);
//         if (!window)
//             window = create_window (editor);
//
//         if (new_doc)
//         {
//             _moo_edit_window_insert_doc (window, doc, NULL);
//             moo_editor_add_doc (editor, window, doc);
//         }
//
//         if (add_history)
//             update_history_item_for_doc (editor, doc, TRUE);
//     }
//
//     if (result)
//         *docp = doc;
//
//     g_free (uri);
//     g_object_unref (doc);
//     return result;
// }

static MooEditArray *
_moo_editor_open_files (MooEditor         *editor,
                        MooOpenInfoArray  *files,
                        GtkWidget         *parent,
                        GError           **error)
{
    guint i;
    MooEdit *bring_to_front = NULL;
    gboolean result = TRUE;
    MooEditWindow *window = NULL;
    MooEditArray *docs;

    moo_return_error_if_fail_p (MOO_IS_EDITOR (editor));
    moo_return_error_if_fail_p (!parent || GTK_IS_WIDGET (parent));
    moo_return_error_if_fail_p (!moo_open_info_array_is_empty (files));

    if (parent)
    {
        GtkWidget *top = gtk_widget_get_toplevel (parent);
        if (MOO_IS_EDIT_WINDOW (top))
            window = MOO_EDIT_WINDOW (top);
    }

    docs = moo_edit_array_new ();

    for (i = 0; i < files->n_elms; ++i)
    {
        MooOpenInfo *info = files->elms[i];
        MooEdit *doc = NULL;

        if (!window)
            window = moo_editor_get_active_window (editor);

        doc = moo_editor_load_file (editor, info, window, parent,
                                    is_embedded (editor), TRUE, error);

        if (doc)
        {
            parent = GTK_WIDGET (moo_edit_get_view (doc));
            bring_to_front = doc;
            moo_edit_array_append (docs, doc);
        }
        else
        {
            result = FALSE;
            break;
        }
    }

    if (bring_to_front)
    {
        moo_editor_set_active_doc (editor, bring_to_front);
        gtk_widget_grab_focus (GTK_WIDGET (moo_edit_get_view (bring_to_front)));
    }

    if (!result)
    {
        moo_edit_array_free (docs);
        docs = NULL;
    }

    return docs;
}


/**
 * moo_editor_get_active_doc:
 */
MooEdit *
moo_editor_get_active_doc (MooEditor *editor)
{
    MooEditWindow *window = moo_editor_get_active_window (editor);
    return window ? moo_edit_window_get_active_doc (window) : NULL;
}

/**
 * moo_editor_get_active_view:
 */
MooEditView *
moo_editor_get_active_view (MooEditor *editor)
{
    MooEditWindow *window = moo_editor_get_active_window (editor);
    return window ? moo_edit_window_get_active_view (window) : NULL;
}

/**
 * moo_editor_get_active_window:
 */
MooEditWindow *
moo_editor_get_active_window (MooEditor *editor)
{
    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    return get_top_window (editor);
}


void
moo_editor_present (MooEditor *editor,
                    guint32    stamp)
{
    MooEditWindow *window;

    g_return_if_fail (MOO_IS_EDITOR (editor));

    window = moo_editor_get_active_window (editor);

    if (!window)
        window = moo_editor_new_window (editor);

    g_return_if_fail (window != NULL);
    moo_window_present (GTK_WINDOW (window), stamp);
}


/**
 * moo_editor_set_active_window:
 */
void
moo_editor_set_active_window (MooEditor      *editor,
                              MooEditWindow  *window)
{
    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    gtk_window_present (GTK_WINDOW (window));
}

/**
 * moo_editor_set_active_view:
 */
void
moo_editor_set_active_view (MooEditor   *editor,
                            MooEditView *view)
{
    MooEditWindow *window;

    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (MOO_IS_EDIT_VIEW (view));

    window = moo_edit_view_get_window (view);
    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));

    moo_window_present (GTK_WINDOW (window), 0);
    moo_edit_window_set_active_view (window, view);
}

/**
 * moo_editor_set_active_doc:
 */
void
moo_editor_set_active_doc (MooEditor      *editor,
                           MooEdit        *doc)
{
    moo_editor_set_active_view (editor, moo_edit_get_view (doc));
}


static MooEdit *
find_busy (MooEditArray *docs)
{
    guint i;
    for (i = 0; i < docs->n_elms; ++i)
        if (MOO_EDIT_IS_BUSY (docs->elms[i]))
            return docs->elms[i];
    return NULL;
}

static MooCloseResponse
moo_editor_before_close_window (MooEditor     *editor,
                                MooEditWindow *window)
{
    MooSaveChangesResponse response;
    MooEditArray *modified;
    MooEditArray *docs;
    gboolean do_close = FALSE;
    MooEdit *busy = NULL;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), MOO_CLOSE_RESPONSE_CANCEL);
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), MOO_CLOSE_RESPONSE_CANCEL);

    docs = moo_edit_window_get_docs (window);
    busy = find_busy (docs);

    if (busy)
    {
        moo_editor_set_active_doc (editor, busy);
        moo_edit_array_free (docs);
        return MOO_CLOSE_RESPONSE_CANCEL;
    }

    modified = find_modified (docs);

    if (moo_edit_array_is_empty (modified))
    {
        do_close = TRUE;
    }
    else if (modified->n_elms == 1)
    {
        if (window)
            moo_edit_window_set_active_doc (window, modified->elms[0]);

        response = _moo_edit_save_changes_dialog (modified->elms[0]);

        switch (response)
        {
            case MOO_SAVE_CHANGES_RESPONSE_SAVE:
                if (moo_editor_save (editor, modified->elms[0], NULL))
                    do_close = TRUE;
                break;

            case MOO_SAVE_CHANGES_RESPONSE_CANCEL:
                break;

            default:
                do_close = TRUE;
                break;
        }
    }
    else
    {
        guint i;
        MooEditArray *to_save;
        gboolean saved = TRUE;

        to_save = moo_edit_array_new ();
        response = _moo_edit_save_multiple_changes_dialog (modified, to_save);

        switch (response)
        {
            case MOO_SAVE_CHANGES_RESPONSE_SAVE:
                for (i = 0; i < to_save->n_elms; ++i)
                    if (!moo_editor_save (editor, to_save->elms[i], NULL))
                    {
                        saved = FALSE;
                        break;
                    }
                if (saved)
                    do_close = TRUE;
                break;

            case MOO_SAVE_CHANGES_RESPONSE_CANCEL:
                break;

            default:
                do_close = TRUE;
                break;
        }

        moo_edit_array_free (to_save);
    }

    moo_edit_array_free (modified);
    moo_edit_array_free (docs);
    return do_close ? MOO_CLOSE_RESPONSE_CONTINUE : MOO_CLOSE_RESPONSE_CANCEL;
}


/**
 * moo_editor_close_window:
 */
gboolean
moo_editor_close_window (MooEditor      *editor,
                         MooEditWindow  *window)
{
    MooCloseResponse response = MOO_CLOSE_RESPONSE_CONTINUE;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), FALSE);

    if (moo_edit_window_array_find (editor->priv->windows, window) < 0)
        return TRUE;

    g_object_ref (window);

    g_signal_emit_by_name (window, "before-close", &response);

    if (response != MOO_CLOSE_RESPONSE_CANCEL && moo_edit_window_array_find (editor->priv->windows, window) >= 0)
        g_signal_emit (editor, signals[BEFORE_CLOSE_WINDOW], 0, window, &response);

    if (response != MOO_CLOSE_RESPONSE_CANCEL && moo_edit_window_array_find (editor->priv->windows, window) >= 0)
        do_close_window (editor, window);

    g_object_unref (window);

    return response != MOO_CLOSE_RESPONSE_CANCEL;
}


static void
do_close_window (MooEditor      *editor,
                 MooEditWindow  *window)
{
    MooEditArray *docs;
    guint i;

    g_signal_emit (editor, signals[WILL_CLOSE_WINDOW], 0, window);
    g_signal_emit_by_name (window, "will-close");

    docs = moo_edit_window_get_docs (window);

    for (i = 0; i < docs->n_elms; ++i)
        do_close_doc (editor, docs->elms[i]);

    moo_edit_window_array_remove (editor->priv->windows, window);

    _moo_window_detach_plugins (window);
    gtk_widget_destroy (GTK_WIDGET (window));

    g_signal_emit (editor, signals[AFTER_CLOSE_WINDOW], 0);

    moo_edit_array_free (docs);
}


static void
do_close_doc (MooEditor *editor,
              MooEdit   *doc)
{
    MooEditWindow *window;

    g_object_ref (doc);

    g_signal_emit (editor, signals[WILL_CLOSE_DOC], 0, doc);
    g_signal_emit_by_name (doc, "will-close");

    window = moo_edit_get_window (doc);

    if (!window)
    {
        g_assert (moo_edit_array_find (editor->priv->windowless, doc) >= 0);
        moo_edit_array_remove (editor->priv->windowless, doc);
    }

    update_history_item_for_doc (editor, doc, TRUE);

    if (window)
        _moo_edit_window_remove_doc (window, doc);
    else
        _moo_doc_detach_plugins (NULL, doc);

    _moo_edit_closed (doc);
    g_object_unref (doc);
}


/**
 * moo_editor_close_doc:
 */
gboolean
moo_editor_close_doc (MooEditor *editor,
                      MooEdit   *doc)
{
    gboolean result;
    MooEditArray *docs;

    docs = moo_edit_array_new ();
    moo_edit_array_append (docs, doc);
    result = moo_editor_close_docs (editor, docs);
    moo_edit_array_free (docs);
    return result;
}


/**
 * moo_editor_close_docs:
 */
gboolean
moo_editor_close_docs (MooEditor    *editor,
                       MooEditArray *docs)
{
    guint i;
    MooEditWindow *window;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);

    if (moo_edit_array_is_empty (docs))
        return TRUE;

    for (i = 0; i < docs->n_elms; ++i)
    {
        MooEdit *doc = docs->elms[i];

        g_return_val_if_fail (MOO_IS_EDIT (doc), FALSE);

        if (MOO_EDIT_IS_BUSY (doc))
        {
            moo_editor_set_active_doc (editor, doc);
            return FALSE;
        }
    }

    window = moo_edit_get_window (docs->elms[0]);

    if (close_docs_real (editor, docs))
    {
        if (window &&
            !moo_edit_window_get_n_tabs (window) &&
            !test_flag (editor, ALLOW_EMPTY_WINDOW))
        {
            MooEdit *doc = MOO_EDIT (g_object_new (get_doc_type (editor),
                                                   "editor", editor,
                                                   (const char*) NULL));
            _moo_edit_window_insert_doc (window, doc, NULL);
            moo_editor_add_doc (editor, window, doc);
            g_object_unref (doc);
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


static gboolean
close_docs_real (MooEditor    *editor,
                 MooEditArray *docs)
{
    MooSaveChangesResponse response;
    MooEditArray *modified;
    gboolean do_close = FALSE;

    modified = find_modified (docs);

    if (moo_edit_array_is_empty (modified))
    {
        do_close = TRUE;
    }
    else if (modified->n_elms == 1)
    {
        MooEditWindow *window = moo_edit_get_window (modified->elms[0]);

        if (window)
            moo_edit_window_set_active_doc (window, modified->elms[0]);

        response = _moo_edit_save_changes_dialog (modified->elms[0]);

        switch (response)
        {
            case MOO_SAVE_CHANGES_RESPONSE_SAVE:
                if (moo_editor_save (editor, modified->elms[0], NULL))
                    do_close = TRUE;
                break;

            case MOO_SAVE_CHANGES_RESPONSE_CANCEL:
                break;

            default:
                do_close = TRUE;
                break;
        }
    }
    else
    {
        guint i;
        MooEditArray *to_save;
        gboolean saved = TRUE;

        to_save = moo_edit_array_new ();
        response = _moo_edit_save_multiple_changes_dialog (modified, to_save);

        switch (response)
        {
            case MOO_SAVE_CHANGES_RESPONSE_SAVE:
                for (i = 0; i < to_save->n_elms; ++i)
                    if (!moo_editor_save (editor, to_save->elms[i], NULL))
                {
                    saved = FALSE;
                    break;
                }
                if (saved)
                    do_close = TRUE;
                break;

            case MOO_SAVE_CHANGES_RESPONSE_CANCEL:
                break;

            default:
                do_close = TRUE;
                break;
        }

        moo_edit_array_free (to_save);
    }

    if (do_close)
    {
        guint i;
        for (i = 0; i < docs->n_elms; ++i)
            do_close_doc (editor, docs->elms[i]);
    }

    moo_edit_array_free (modified);
    return do_close;
}


static MooEditArray *
find_modified (MooEditArray *docs)
{
    guint i;
    MooEditArray *modified = moo_edit_array_new ();
    for (i = 0; i < docs->n_elms; ++i)
        if (moo_edit_is_modified (docs->elms[i]) && !moo_edit_get_clean (docs->elms[i]))
            moo_edit_array_append (modified, docs->elms[i]);
    return modified;
}


gboolean
_moo_editor_close_all (MooEditor *editor)
{
    guint i;
    MooEditWindowArray *windows;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), FALSE);

    windows = moo_editor_get_windows (editor);

    for (i = 0; i < windows->n_elms; ++i)
    {
        if (!moo_editor_close_window (editor, windows->elms[i]))
        {
            moo_edit_window_array_free (windows);
            return FALSE;
        }
    }

    moo_edit_window_array_free (windows);
    return TRUE;
}


/* Remove after March 2009 */
static char *
filename_from_utf8 (const char *encoded)
{
    if (g_str_has_prefix (encoded, "base64"))
    {
        guchar *filename;
        gsize len;

        filename = g_base64_decode (encoded + strlen ("base64"), &len);

        if (!filename || !len || filename[len-1] != 0)
        {
            g_critical ("oops");
            return NULL;
        }

        return (char*) filename;
    }
    else
    {
        return g_strdup (encoded);
    }
}

static MooEdit *
load_doc_session (MooEditor     *editor,
                  MooEditWindow *window,
                  MooMarkupNode *elm,
                  gboolean       file_is_uri)
{
    const char *uri = NULL;
    const char *encoding;
    char *freeme = NULL;
    MooEdit *doc = NULL;
    MooOpenInfo *info;

    if (file_is_uri)
    {
        uri = moo_markup_get_content (elm);
    }
    else
    {
        const char *filename_utf8 = moo_markup_get_content (elm);
        char *filename = filename_from_utf8 (filename_utf8);
        if (filename)
        {
            freeme = g_filename_to_uri (filename, NULL, NULL);
            uri = freeme;
        }
        g_free (filename);
    }

    if (!uri || !uri[0])
    {
        g_free (freeme);
        return moo_editor_new_doc (editor, window);
    }

    encoding = moo_markup_get_prop (elm, "encoding");
    info = moo_open_info_new_uri (uri, encoding, -1, MOO_OPEN_FLAGS_NONE);

    doc = moo_editor_load_file (editor, info, window, GTK_WIDGET (window), TRUE, FALSE, NULL);

    g_object_unref (info);
    g_free (freeme);
    return doc;
}

static MooMarkupNode *
save_doc_session (MooEdit       *doc,
                  MooMarkupNode *elm)
{
    const char *encoding;
    MooMarkupNode *node;

    gstr uri = gstr::take (moo_edit_get_uri (doc));
    encoding = moo_edit_get_encoding (doc);

    if (!uri.empty())
    {
        node = moo_markup_create_text_element (elm, "document", uri.get());

        if (encoding && encoding[0])
            moo_markup_set_prop (node, "encoding", encoding);
    }
    else
    {
        node = moo_markup_create_element (elm, "document");
    }

    return node;
}

static MooEditWindow *
load_window_session (MooEditor     *editor,
                     MooMarkupNode *elm,
                     gboolean       file_is_uri)
{
    MooEditWindow *window;
    MooEdit *active_doc = NULL;
    MooMarkupNode *node;

    window = create_window (editor);

    for (node = elm->children; node != NULL; node = node->next)
    {
        if (MOO_MARKUP_IS_ELEMENT (node))
        {
            MooEdit *doc;

            doc = load_doc_session (editor, window, node, file_is_uri);

            if (doc && moo_markup_bool_prop (node, "active", FALSE))
                active_doc = doc;
        }
    }

    if (active_doc)
        moo_edit_window_set_active_doc (window, active_doc);

    return window;
}

static MooMarkupNode *
save_window_session (MooEditWindow *window,
                     MooMarkupNode *elm)
{
    MooMarkupNode *node;
    MooEdit *active_doc;
    MooEditArray *docs;
    guint i;

    active_doc = moo_edit_window_get_active_doc (window);
    docs = moo_edit_window_get_docs (window);

    node = moo_markup_create_element (elm, "window");

    for (i = 0; i < docs->n_elms; ++i)
    {
        MooMarkupNode *doc_node;
        MooEdit *doc = docs->elms[i];

        doc_node = save_doc_session (doc, node);

        if (doc_node && doc == active_doc)
            moo_markup_set_bool_prop (doc_node, "active", TRUE);
    }

    moo_edit_array_free (docs);
    return node;
}

void
_moo_editor_load_session (MooEditor     *editor,
                          MooMarkupNode *xml)
{
    MooMarkupNode *editor_node;
    gboolean old_format = FALSE;

    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (MOO_MARKUP_IS_ELEMENT (xml));

    editor_node = moo_markup_get_element (xml, "editor");

    if (editor_node)
    {
        const char *version = moo_markup_get_prop (editor_node, "version");
        if (!version)
            old_format = TRUE;
        else if (strcmp (version, CURRENT_SESSION_VERSION) != 0)
            editor_node = NULL;
    }

    if (editor_node)
    {
        MooEditWindow *active_window = NULL;
        MooMarkupNode *node;

        for (node = editor_node->children; node != NULL; node = node->next)
        {
            MooEditWindow *window;

            if (!MOO_MARKUP_IS_ELEMENT (node))
                continue;

            window = load_window_session (editor, node, !old_format);

            if (window && moo_markup_bool_prop (node, "active", FALSE))
                active_window = window;
        }

        if (active_window)
            moo_editor_set_active_window (editor, active_window);
    }
}

void
_moo_editor_save_session (MooEditor     *editor,
                          MooMarkupNode *xml)
{
    MooMarkupNode *node;
    MooEditWindow *active_window;
    MooEditWindowArray *windows;
    guint i;

    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (MOO_MARKUP_IS_ELEMENT (xml));

    active_window = moo_editor_get_active_window (editor);
    windows = moo_editor_get_windows (editor);

    node = moo_markup_create_element (xml, "editor");
    moo_markup_set_prop (node, "version", CURRENT_SESSION_VERSION);

    for (i = 0; i < windows->n_elms; ++i)
    {
        MooEditWindow *window = windows->elms[i];
        MooMarkupNode *window_node;

        window_node = save_window_session (window, node);

        if (window_node && window == active_window)
            moo_markup_set_bool_prop (window_node, "active", TRUE);
    }

    moo_edit_window_array_free (windows);
}


/**
 * moo_editor_get_windows:
 *
 * Returns: (transfer full)
 */
MooEditWindowArray *
moo_editor_get_windows (MooEditor *editor)
{
    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    return moo_edit_window_array_copy (editor->priv->windows);
}

/**
 * moo_editor_get_docs:
 *
 * Returns: (transfer full)
 */
MooEditArray *
moo_editor_get_docs (MooEditor *editor)
{
    guint i;
    MooEditArray *docs;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);

    docs = moo_edit_array_new ();

    for (i = 0; i < editor->priv->windows->n_elms; ++i)
    {
        MooEditArray *docs_here = moo_edit_window_get_docs (editor->priv->windows->elms[i]);
        moo_edit_array_append_array (docs, docs_here);
        moo_edit_array_free (docs_here);
    }

    moo_edit_array_append_array (docs, editor->priv->windowless);

    return docs;
}


/**
 * moo_editor_open_files:
 *
 * @editor:
 * @files:
 * @parent: (allow-none) (default NULL)
 * @error:
 */
gboolean
moo_editor_open_files (MooEditor        *editor,
                       MooOpenInfoArray *files,
                       GtkWidget        *parent,
                       GError          **error)
{
    MooEditArray *docs;
    gboolean ret;

    moo_return_error_if_fail (MOO_IS_EDITOR (editor));
    moo_return_error_if_fail (!moo_open_info_array_is_empty (files));

    docs = _moo_editor_open_files (editor, files, parent, error);
    ret = !moo_edit_array_is_empty (docs);

    moo_assert (moo_edit_array_is_empty (docs) ||
                moo_edit_array_get_size (docs) ==
                    moo_open_info_array_get_size (files));

    moo_edit_array_free (docs);
    return ret;
}

/**
 * moo_editor_new_file:
 *
 * @editor:
 * @info:
 * @parent: (allow-none) (default NULL)
 * @error:
 */
MooEdit *
moo_editor_new_file (MooEditor    *editor,
                     MooOpenInfo  *info,
                     GtkWidget    *parent,
                     GError      **error)
{
    MooEdit *doc;
    MooOpenInfo *info_copy;

    moo_return_error_if_fail_p (MOO_IS_EDITOR (editor));
    moo_return_error_if_fail_p (info != NULL);

    info_copy = moo_open_info_dup (info);
    moo_return_error_if_fail_p (info_copy != NULL);

    info_copy->flags |= MOO_OPEN_FLAG_CREATE_NEW;

    doc = moo_editor_open_file (editor, info_copy, parent, error);

    g_object_unref (info_copy);
    return doc;
}

/**
 * moo_editor_open_file:
 *
 * @editor:
 * @info:
 * @parent: (allow-none) (default NULL)
 * @error:
 */
MooEdit *
moo_editor_open_file (MooEditor   *editor,
                      MooOpenInfo *info,
                      GtkWidget   *parent,
                      GError     **error)
{
    MooEditArray *docs;
    MooOpenInfoArray *files;
    MooEdit *ret = NULL;

    moo_return_error_if_fail_p (info != NULL);

    files = moo_open_info_array_new ();
    moo_open_info_array_append (files, info);

    docs = _moo_editor_open_files (editor, files, parent, error);

    moo_open_info_array_free (files);

    if (docs)
    {
        moo_release_assert (docs->n_elms > 0);
        ret = docs->elms[0];
    }

    moo_edit_array_free (docs);
    return ret;
}

/**
 * moo_editor_open_uri: (moo-kwargs)
 *
 * @editor:
 * @uri: (type const-utf8)
 * @encoding: (type const-utf8) (allow-none) (default NULL)
 * @line: (default -1)
 * @window: (allow-none) (default NULL)
 */
MooEdit *
moo_editor_open_uri (MooEditor     *editor,
                     const char    *uri,
                     const char    *encoding,
                     int            line,
                     MooEditWindow *window)
{
    MooEdit *ret;
    MooOpenInfo *info;

    info = moo_open_info_new_uri (uri, encoding, line, MOO_OPEN_FLAGS_NONE);
    g_return_val_if_fail (info != NULL, NULL);

    ret = moo_editor_open_file (editor, info, window ? GTK_WIDGET (window) : NULL, NULL);

    g_object_unref (info);
    return ret;
}

/**
 * moo_editor_open_path: (moo-kwargs)
 *
 * @editor:
 * @path: (type const-filename)
 * @encoding: (type const-utf8) (allow-none) (default NULL)
 * @line: (default -1)
 * @window: (allow-none) (default NULL)
 */
MooEdit *
moo_editor_open_path (MooEditor     *editor,
                      const char    *path,
                      const char    *encoding,
                      int            line,
                      MooEditWindow *window)
{
    MooEdit *ret;
    MooOpenInfo *info;

    info = moo_open_info_new (path, encoding, line, MOO_OPEN_FLAGS_NONE);
    g_return_val_if_fail (info != NULL, NULL);

    ret = moo_editor_open_file (editor, info, window ? GTK_WIDGET (window) : NULL, NULL);

    g_object_unref (info);
    return ret;
}

/**
 * moo_editor_create_doc: (moo.lua 0)
 *
 * @editor:
 * @filename: (type const-filename) (allow-none) (default NULL)
 * @encoding: (type const-utf8) (allow-none) (default NULL)
 * @error:
 *
 * Create a document instance which can be embedded into arbitrary
 * widget.
 *
 * This method may not be used in medit (use moo_editor_new_doc(),
 * moo_editor_new_file(), moo_editor_open_file(), moo_editor_open_files(),
 * moo_editor_open_uri(), moo_editor_open_path() instead).
 */
MooEdit *
moo_editor_create_doc (MooEditor   *editor,
                       const char  *filename,
                       const char  *encoding,
                       GError     **error)
{
    MooEdit *doc;
    GFile *file = NULL;

    moo_return_error_if_fail_p (MOO_IS_EDITOR (editor));

    if (filename)
        file = g_file_new_for_path (filename);

    doc = MOO_EDIT (g_object_new (get_doc_type (editor), "editor", editor, (const char*) NULL));

    if (file == NULL || _moo_edit_load_file (doc, file, encoding, NULL, error))
    {
        moo_editor_add_doc (editor, NULL, doc);
    }
    else
    {
        g_object_unref (doc);
        doc = NULL;
    }

    moo_file_free (file);
    return doc;
}

// MooEdit *
// moo_editor_open_file (MooEditor      *editor,
//                       MooEditWindow  *window,
//                       GtkWidget      *parent,
//                       const char     *filename,
//                       const char     *encoding)
// {
//     gboolean result;
//
//     g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
//     g_return_val_if_fail (!window || MOO_IS_EDIT_WINDOW (window), NULL);
//     g_return_val_if_fail (!parent || GTK_IS_WIDGET (parent), NULL);
//
//     if (!filename)
//     {
//         result = moo_editor_open (editor, window, parent, NULL);
//     }
//     else
//     {
//         MooFileEncArray *list;
//
//         list = moo_file_enc_array_new ();
//         moo_file_enc_array_take (list, moo_file_enc_new_for_path (filename, encoding));
//
//         result = moo_editor_open (editor, window, parent, list);
//
//         moo_file_enc_array_free (list);
//     }
//
//     if (!result)
//         return NULL;
//
//     return moo_editor_get_doc (editor, filename);
// }


// MooEdit *
// moo_editor_open_file_line (MooEditor      *editor,
//                            const char     *filename,
//                            int             line,
//                            MooEditWindow  *window)
// {
//     MooEdit *doc = NULL;
//     char *freeme = NULL;
//     MooFileEnc *fenc = NULL;
//
//     g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
//     g_return_val_if_fail (filename != NULL, NULL);
//
//     doc = moo_editor_get_doc (editor, filename);
//
//     if (doc)
//     {
//         if (line >= 0)
//             moo_text_view_move_cursor (MOO_TEXT_VIEW (doc), line, 0, FALSE, FALSE);
//         moo_editor_set_active_doc (editor, doc);
//         gtk_widget_grab_focus (GTK_WIDGET (doc));
//         return doc;
//     }
//
//     freeme = _moo_normalize_file_path (filename);
//     filename = freeme;
//
//     if (!g_file_test (filename, G_FILE_TEST_EXISTS))
//         goto out;
//
//     fenc = moo_file_enc_new_for_path (filename, NULL);
//     moo_editor_load_file (editor, window, NULL, fenc,
//                           is_embedded (editor),
//                           TRUE, line, &doc);
//
//     /* XXX */
//     moo_editor_set_active_doc (editor, doc);
//     if (line >= 0)
//         moo_text_view_move_cursor (MOO_TEXT_VIEW (doc), line, 0, FALSE, TRUE);
//     gtk_widget_grab_focus (GTK_WIDGET (doc));
//
// out:
//     moo_file_enc_free (fenc);
//     g_free (freeme);
//     return doc;
// }


// static MooEdit *
// moo_editor_new_uri (MooEditor     *editor,
//                     MooEditWindow *window,
//                     GtkWidget     *parent,
//                     const char    *uri,
//                     const char    *encoding)
// {
//     MooEdit *doc = NULL;
//     char *path;
//     GFile *file;
//
//     g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
//     g_return_val_if_fail (!window || MOO_IS_EDIT_WINDOW (window), NULL);
//     g_return_val_if_fail (!parent || GTK_IS_WIDGET (parent), NULL);
//
//     if (!uri)
//         return moo_editor_open_uri (editor, window, parent, NULL, NULL);
//
//     file = g_file_new_for_uri (uri);
//     path = g_file_get_path (file);
//
//     if (path && g_file_test (path, G_FILE_TEST_EXISTS))
//     {
//         g_free (path);
//         g_object_unref (file);
//         return moo_editor_open_uri (editor, window, parent,
//                                     uri, encoding);
//     }
//
//     if (!window)
//         window = moo_editor_get_active_window (editor);
//
//     if (window)
//     {
//         doc = moo_edit_window_get_active_doc (window);
//
//         if (!doc || !moo_edit_is_empty (doc))
//             doc = NULL;
//     }
//
//     if (!doc)
//         doc = moo_editor_new_doc (editor, window);
//
//     _moo_edit_set_status (doc, MOO_EDIT_STATUS_NEW);
//     _moo_edit_set_file (doc, file, encoding);
//     moo_editor_set_active_doc (editor, doc);
//     gtk_widget_grab_focus (GTK_WIDGET (doc));
//
//     g_free (path);
//     g_object_unref (file);
//     return doc;
// }

// MooEdit *
// moo_editor_new_file (MooEditor      *editor,
//                      MooEditWindow  *window,
//                      GtkWidget      *parent,
//                      const char     *filename,
//                      const char     *encoding)
// {
//     MooEdit *doc = NULL;
//     char *freeme = NULL;
//     GFile *file;
//
//     g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
//     g_return_val_if_fail (!window || MOO_IS_EDIT_WINDOW (window), NULL);
//     g_return_val_if_fail (!parent || GTK_IS_WIDGET (parent), NULL);
//
//     if (!filename)
//         return moo_editor_open_file (editor, window, parent, NULL, NULL);
//
//     if (g_file_test (filename, G_FILE_TEST_EXISTS))
//         return moo_editor_open_file (editor, window, parent,
//                                      filename, encoding);
//
//     freeme = _moo_normalize_file_path (filename);
//     filename = freeme;
//
//     if (!window)
//         window = moo_editor_get_active_window (editor);
//
//     if (window)
//     {
//         doc = moo_edit_window_get_active_doc (window);
//
//         if (!doc || !moo_edit_is_empty (doc))
//             doc = NULL;
//     }
//
//     if (!doc)
//         doc = moo_editor_new_doc (editor, window);
//
//     _moo_edit_set_status (doc, MOO_EDIT_STATUS_NEW);
//     file = g_file_new_for_path (filename);
//     _moo_edit_set_file (doc, file, encoding);
//     moo_editor_set_active_doc (editor, doc);
//     gtk_widget_grab_focus (GTK_WIDGET (doc));
//
//     g_object_unref (file);
//     g_free (freeme);
//     return doc;
// }
//
//
// MooEdit *
// moo_editor_open_uri (MooEditor      *editor,
//                      MooEditWindow  *window,
//                      GtkWidget      *parent,
//                      const char     *uri,
//                      const char     *encoding)
// {
//     char *filename;
//     MooEdit *doc;
//
//     g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
//     g_return_val_if_fail (!window || MOO_IS_EDIT_WINDOW (window), NULL);
//     g_return_val_if_fail (!parent || GTK_IS_WIDGET (parent), NULL);
//     g_return_val_if_fail (uri != NULL, NULL);
//
//     filename = g_filename_from_uri (uri, NULL, NULL);
//     g_return_val_if_fail (filename != NULL, NULL);
//
//     doc = moo_editor_open_file (editor, window, parent, filename, encoding);
//
//     g_free (filename);
//     return doc;
// }


/**
 * moo_editor_reload:
 *
 * @editor:
 * @doc:
 * @info: (allow-none) (default NULL)
 * @error:
 **/
gboolean
moo_editor_reload (MooEditor     *editor,
                   MooEdit       *doc,
                   MooReloadInfo *info,
                   GError       **error)
{
    guint i;
    GError *error_here = NULL;
    MooEditViewArray *views = NULL;
    gboolean ret = FALSE;

    moo_return_error_if_fail (MOO_IS_EDITOR (editor));

    if (MOO_EDIT_IS_BUSY (doc))
    {
        g_set_error (error,
                     MOO_EDIT_RELOAD_ERROR,
                     MOO_EDIT_RELOAD_ERROR_BUSY,
                     "document is busy");
        goto out;
    }

    if (moo_edit_is_untitled (doc))
    {
        g_set_error (error,
                     MOO_EDIT_RELOAD_ERROR,
                     MOO_EDIT_RELOAD_ERROR_UNTITLED,
                     "document is untitled");
        goto out;
    }

    if (!is_embedded (editor) &&
        !moo_edit_get_clean (doc) &&
        moo_edit_is_modified (doc) &&
        !_moo_edit_reload_modified_dialog (doc))
    {
        g_set_error (error,
                     MOO_EDIT_RELOAD_ERROR,
                     MOO_EDIT_RELOAD_ERROR_CANCELLED,
                     "cancelled by user");
        goto out;
    }

    views = moo_edit_get_views (doc);

    for (i = 0; i < moo_edit_view_array_get_size (views); ++i)
    {
        int cursor_line, cursor_offset;
        GtkTextIter iter;
        MooEditView *view = views->elms[i];

        moo_text_view_get_cursor (GTK_TEXT_VIEW (view), &iter);
        cursor_line = gtk_text_iter_get_line (&iter);
        cursor_offset = moo_text_iter_get_visual_line_offset (&iter, 8);

        if (info != NULL && info->line >= 0 && info->line != cursor_line)
        {
            cursor_line = info->line;
            cursor_offset = 0;
        }

        g_object_set_data (G_OBJECT (view), "moo-reload-cursor-line", GINT_TO_POINTER (cursor_line));
        g_object_set_data (G_OBJECT (view), "moo-reload-cursor-offset", GINT_TO_POINTER (cursor_offset));
    }

    if (!_moo_edit_reload_file (doc, info ? info->encoding : NULL, &error_here))
    {
        if (!is_embedded (editor) && !_moo_is_file_error_cancelled (error_here))
            _moo_edit_reload_error_dialog (doc, error_here);

        g_propagate_error (error, error_here);

        g_object_set_data (G_OBJECT (doc), "moo-scroll-to", NULL);
        goto out;
    }

    for (i = 0; i < moo_edit_view_array_get_size (views); ++i)
    {
        int cursor_line, cursor_offset;
        MooEditView *view = views->elms[i];

        cursor_line = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (view), "moo-reload-cursor-line"));
        cursor_offset = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (view), "moo-reload-cursor-offset"));

        moo_text_view_move_cursor (MOO_TEXT_VIEW (view), cursor_line,
                                   cursor_offset, TRUE, TRUE);
    }

    ret = TRUE;

out:
    moo_edit_view_array_free (views);
    return ret;
}


static MooSaveResponse
moo_editor_before_save (G_GNUC_UNUSED MooEditor *editor,
                        G_GNUC_UNUSED MooEdit *doc,
                        G_GNUC_UNUSED GFile *file)
{
    return MOO_SAVE_RESPONSE_CONTINUE;
}

static MooEditSaveFlags
moo_editor_get_save_flags (MooEditor *editor)
{
    MooEditSaveFlags flags = MOO_EDIT_SAVE_FLAGS_NONE;

    if (test_flag (editor, SAVE_BACKUPS))
        flags |= MOO_EDIT_SAVE_BACKUP;

    return flags;
}

static void
moo_editor_will_save (G_GNUC_UNUSED MooEditor *editor,
                      MooEdit *doc,
                      G_GNUC_UNUSED GFile *file)
{
    if (moo_edit_config_get_bool (doc->config, "strip"))
        _moo_edit_strip_whitespace (doc);
    if (moo_edit_config_get_bool (doc->config, "add-newline"))
        _moo_edit_ensure_newline (doc);
}

static gboolean
do_save (MooEditor    *editor,
         MooEdit      *doc,
         GFile        *file,
         const char   *encoding,
         GError      **error)
{
    int response = MOO_SAVE_RESPONSE_CONTINUE;
    GError *error_here = NULL;
    gboolean result;

    g_signal_emit (editor, signals[BEFORE_SAVE], 0, doc, file, &response);

    if (response != MOO_SAVE_RESPONSE_CANCEL)
        g_signal_emit_by_name (doc, "before-save", file, &response);

    if (response == MOO_SAVE_RESPONSE_CANCEL)
    {
        g_set_error (error,
                     MOO_EDIT_SAVE_ERROR,
                     MOO_EDIT_SAVE_ERROR_CANCELLED,
                     "cancelled");
        return FALSE;
    }

    g_signal_emit (editor, signals[WILL_SAVE], 0, doc, file);
    g_signal_emit_by_name (doc, "will-save", file);

    result = _moo_edit_save_file (doc, file, encoding,
                                  moo_editor_get_save_flags (editor),
                                  &error_here);
    if (!result && error_here->domain == MOO_EDIT_FILE_ERROR &&
        error_here->code == MOO_EDIT_FILE_ERROR_ENCODING)
    {
        g_error_free (error_here);
        error_here = NULL;

        if (_moo_edit_save_error_enc_dialog (doc, file, encoding))
        {
            result = _moo_edit_save_file (doc, file, "UTF-8",
                                          moo_editor_get_save_flags (editor),
                                          &error_here);
        }
        else
        {
            g_set_error (error,
                         MOO_EDIT_SAVE_ERROR,
                         MOO_EDIT_SAVE_ERROR_CANCELLED,
                         "cancelled");
            return FALSE;
        }
    }

    if (!result)
    {
        if (!is_embedded (editor))
            _moo_edit_save_error_dialog (doc, file, error_here);
        g_propagate_error (error, error_here);
        return FALSE;
    }

    update_history_item_for_doc (editor, doc, TRUE);

    g_signal_emit_by_name (doc, "after-save");
    g_signal_emit (editor, signals[AFTER_SAVE], 0, doc);

    return TRUE;
}

/**
 * moo_editor_save:
 **/
gboolean
moo_editor_save (MooEditor  *editor,
                 MooEdit    *doc,
                 GError    **error)
{
    GFile *file;
    char *encoding;
    gboolean result = FALSE;

    moo_return_error_if_fail (MOO_IS_EDITOR (editor));
    moo_return_error_if_fail (MOO_IS_EDIT (doc));

    if (MOO_EDIT_IS_BUSY (doc))
    {
        g_set_error (error,
                     MOO_EDIT_SAVE_ERROR,
                     MOO_EDIT_SAVE_ERROR_BUSY,
                     "document is busy");
        return FALSE;
    }

    if (moo_edit_is_untitled (doc))
        return moo_editor_save_as (editor, doc, NULL, error);

    file = moo_edit_get_file (doc);
    encoding = g_strdup (moo_edit_get_encoding (doc));

    if (!is_embedded (editor) &&
        (moo_edit_get_status (doc) & MOO_EDIT_STATUS_MODIFIED_ON_DISK) &&
        !_moo_edit_overwrite_modified_dialog (doc))
    {
        g_set_error (error,
                     MOO_EDIT_SAVE_ERROR,
                     MOO_EDIT_SAVE_ERROR_CANCELLED,
                     "cancelled by user");
        goto out;
    }

    result = do_save (editor, doc, file, encoding, error);

    /* fall through */
out:
    g_object_unref (file);
    g_free (encoding);
    return result;
}

/**
 * moo_editor_save_as:
 *
 * @editor:
 * @doc:
 * @info: (allow-none) (default NULL)
 * @error:
 *
 * Save document with new filename and/or encoding. If @info is
 * missing or %NULL then user is asked for new filename first.
 **/
gboolean
moo_editor_save_as (MooEditor   *editor,
                    MooEdit     *doc,
                    MooSaveInfo *info,
                    GError     **error)
{
    MooSaveInfo *freeme = NULL;
    gboolean result = FALSE;

    moo_return_error_if_fail (MOO_IS_EDITOR (editor));
    moo_return_error_if_fail (MOO_IS_EDIT (doc));
    moo_return_error_if_fail (!info || info->file);

    if (MOO_EDIT_IS_BUSY (doc))
    {
        g_set_error (error,
                     MOO_EDIT_SAVE_ERROR,
                     MOO_EDIT_SAVE_ERROR_BUSY,
                     "document is busy");
        return FALSE;
    }

    if (!info)
    {
        freeme = _moo_edit_save_as_dialog (doc, moo_edit_get_display_basename (doc));

        if (!freeme)
        {
            g_set_error (error,
                         MOO_EDIT_SAVE_ERROR,
                         MOO_EDIT_SAVE_ERROR_CANCELLED,
                         "cancelled by user");
            goto out;
        }

        info = freeme;
    }
    else if (!info->encoding)
    {
        freeme = moo_save_info_new_file (info->file, moo_edit_get_encoding (doc));
        info = freeme;
    }

    update_history_item_for_doc (editor, doc, FALSE);

    result = do_save (editor, doc, info->file, info->encoding, error);

    /* fall through */
out:
    if (freeme)
        g_object_unref (freeme);
    return result;
}

/**
 * moo_editor_save_copy:
 **/
gboolean
moo_editor_save_copy (MooEditor   *editor,
                      MooEdit     *doc,
                      MooSaveInfo *info,
                      GError     **error)
{
    gboolean retval;

    moo_return_error_if_fail (MOO_IS_EDITOR (editor));
    moo_return_error_if_fail (MOO_IS_EDIT (doc));
    moo_return_error_if_fail (info != NULL && info->file != NULL);

    retval = _moo_edit_save_file_copy (doc, info->file,
                                       info->encoding ? info->encoding : moo_edit_get_encoding (doc),
                                       moo_editor_get_save_flags (editor),
                                       error);

    return retval;
}


static MooEdit *
doc_array_find_norm_name (MooEditArray *docs,
                          const char   *norm_name)
{
    guint i;

    g_return_val_if_fail (docs != NULL, NULL);
    g_return_val_if_fail (norm_name != NULL, NULL);

    for (i = 0; i < docs->n_elms; ++i)
    {
        MooEdit *doc = docs->elms[i];
        char *doc_norm_name = _moo_edit_get_normalized_name (doc);
        gboolean this_doc = doc_norm_name != NULL && strcmp (doc_norm_name, norm_name) == 0;
        g_free (doc_norm_name);
        if (this_doc)
            return doc;
    }

    return NULL;
}

/**
 * moo_editor_get_doc_for_file:
 *
 * Finds open document by #GFile.
 */
MooEdit *
moo_editor_get_doc_for_file (MooEditor *editor,
                             GFile     *file)
{
    char *norm_name = NULL;
    MooEdit *doc = NULL;
    guint i;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    g_return_val_if_fail (G_IS_FILE (file), NULL);

    norm_name = _moo_file_get_normalized_name (file);
    g_return_val_if_fail (norm_name != NULL, NULL);

    doc = doc_array_find_norm_name (editor->priv->windowless, norm_name);

    for (i = 0; !doc && i < editor->priv->windows->n_elms; ++i)
    {
        MooEditArray *docs = moo_edit_window_get_docs (editor->priv->windows->elms[i]);
        doc = doc_array_find_norm_name (docs, norm_name);
        moo_edit_array_free (docs);
    }

    g_free (norm_name);
    return doc;
}

/**
 * moo_editor_get_doc:
 *
 * @editor:
 * @filename: (type const-filename)
 *
 * Finds open document by filename.
 */
MooEdit *
moo_editor_get_doc (MooEditor  *editor,
                    const char *filename)
{
    GFile *file;
    MooEdit *doc;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    g_return_val_if_fail (filename != NULL, NULL);

    file = g_file_new_for_path (filename);
    doc = moo_editor_get_doc_for_file (editor, file);

    g_object_unref (file);
    return doc;
}

/**
 * moo_editor_get_doc_for_uri:
 *
 * @editor:
 * @uri: (type const-utf8)
 *
 * Finds open document by URI.
 */
MooEdit *
moo_editor_get_doc_for_uri (MooEditor  *editor,
                            const char *uri)
{
    GFile *file;
    MooEdit *doc;

    g_return_val_if_fail (MOO_IS_EDITOR (editor), NULL);
    g_return_val_if_fail (uri != NULL, NULL);

    file = g_file_new_for_uri (uri);
    doc = moo_editor_get_doc_for_file (editor, file);

    g_object_unref (file);
    return doc;
}


/**
 * moo_editor_set_window_type: (moo.lua 0)
 */
void
moo_editor_set_window_type (MooEditor      *editor,
                            GType           type)
{
    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (g_type_is_a (type, MOO_TYPE_EDIT_WINDOW));
    editor->priv->window_type = type;
}


/**
 * moo_editor_set_doc_type: (moo.lua 0)
 */
void
moo_editor_set_doc_type (MooEditor      *editor,
                         GType           type)
{
    g_return_if_fail (MOO_IS_EDITOR (editor));
    g_return_if_fail (g_type_is_a (type, MOO_TYPE_EDIT));
    editor->priv->doc_type = type;
}


void
_moo_editor_apply_prefs (MooEditor *editor)
{
    gboolean backups;
    const char *color_scheme;

    _moo_edit_window_update_title ();
    _moo_edit_window_set_use_tabs ();

    _moo_edit_update_global_config ();
    _moo_edit_queue_recheck_config_all ();

    color_scheme = moo_prefs_get_string (moo_edit_setting (MOO_EDIT_PREFS_COLOR_SCHEME));

    if (color_scheme)
        _moo_lang_mgr_set_active_scheme (editor->priv->lang_mgr, color_scheme);

    backups = moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_MAKE_BACKUPS));

    g_object_set (editor,
                  "save-backups", backups,
                  nullptr);
}


// void
// _moo_editor_open_uri (MooEditor  *editor,
//                       const char *uri,
//                       const char *encoding,
//                       guint       line,
//                       guint       options)
// {
//     MooEdit *doc;
//     MooEditWindow *window;
//
//     g_return_if_fail (MOO_IS_EDITOR (editor));
//     g_return_if_fail (uri != NULL);
//
//     doc = moo_editor_get_doc_for_uri (editor, uri);
//
//     if (doc)
//     {
//         if (line > 0)
//             moo_text_view_move_cursor (MOO_TEXT_VIEW (doc), line - 1, 0, FALSE, FALSE);
//         moo_editor_set_active_doc (editor, doc);
//         gtk_widget_grab_focus (GTK_WIDGET (doc));
//
//         if (options & MOO_OPEN_RELOAD)
//             _moo_editor_reload (editor, doc, NULL, NULL);
//
//         return;
//     }
//
//     window = moo_editor_get_active_window (editor);
//     doc = window ? moo_edit_window_get_active_doc (window) : NULL;
//
//     if (!doc || !moo_edit_is_empty (doc))
//     {
//         gboolean new_window = moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_OPEN_NEW_WINDOW));
//
//         if (options & MOO_OPEN_NEW_TAB)
//             new_window = FALSE;
//         else if (options & MOO_OPEN_NEW_WINDOW)
//             new_window = TRUE;
//
//         if (new_window)
//             window = moo_editor_new_window (editor);
//     }
//
//     doc = moo_editor_new_uri (editor, window, NULL, uri, encoding);
//     g_return_if_fail (doc != NULL);
//
//     moo_editor_set_active_doc (editor, doc);
//     if (line > 0)
//         moo_text_view_move_cursor (MOO_TEXT_VIEW (doc), line - 1, 0, FALSE, TRUE);
//     gtk_widget_grab_focus (GTK_WIDGET (doc));
//
//     if (options & MOO_OPEN_RELOAD)
//         _moo_editor_reload (editor, doc, NULL, NULL);
// }
