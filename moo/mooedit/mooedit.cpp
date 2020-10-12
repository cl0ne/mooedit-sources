/*
 *   mooedit.c
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
 * class:MooEdit: (parent GObject) (moo.doc-object-name doc): document object
 *
 * Object which represents a document. It has methods for file operations
 * and manipulating document text.
 **/

/**
 * flags:MooEditStatus
 *
 * @MOO_EDIT_STATUS_NORMAL: none of the below flags, it is equal to <code>0</code>.
 * @MOO_EDIT_STATUS_MODIFIED_ON_DISK: file has been modified on disk outside the program.
 * @MOO_EDIT_STATUS_DELETED: file has been deleted from disk.
 * @MOO_EDIT_STATUS_CHANGED_ON_DISK: file has been modified on disk outside the program or deleted from disk,
 * it is equal to <code>#MOO_EDIT_STATUS_MODIFIED_ON_DISK | #MOO_EDIT_STATUS_DELETED</code>.
 * @MOO_EDIT_STATUS_MODIFIED: document content is modifed and not saved yet.
 * @MOO_EDIT_STATUS_NEW: file doesn't exist on disk yet. This is the state of documents
 * opened from command line when requested file doesn't exist.
 * @MOO_EDIT_STATUS_CLEAN: doesn't prompt on close, even if it's modified.
 **/

#include "mooedit/mooeditaction-factory.h"
#include "mooedit/mooedit-private.h"
#include "mooedit/mooeditview-impl.h"
#include "mooedit/mooeditbookmark.h"
#include "mooedit/mooeditdialogs.h"
#include "mooedit/mooeditprefs.h"
#include "mooedit/mootextbuffer.h"
#include "mooedit/mooeditfiltersettings.h"
#include "mooedit/mooeditor-impl.h"
#include "mooedit/mooedittab-impl.h"
#include "mooedit/mooeditwindow-impl.h"
#include "mooedit/moolangmgr.h"
#include "marshals.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mootype-macros.h"
#include "mooutils/mooatom.h"
#include "mooutils/moocompat.h"
#include "mooedit/mooeditprogress-gxml.h"
#include <string.h>
#include <stdlib.h>

#define KEY_ENCODING "encoding"
#define KEY_LINE "line"

MOO_DEFINE_OBJECT_ARRAY (MooEdit, moo_edit)

MooEditList *_moo_edit_instances = NULL;
static guint moo_edit_apply_config_all_idle;

static GObject *moo_edit_constructor            (GType           type,
                                                 guint           n_construct_properties,
                                                 GObjectConstructParam *construct_param);
static void     moo_edit_finalize               (GObject        *object);
static void     moo_edit_dispose                (GObject        *object);

static void     moo_edit_set_property           (GObject        *object,
                                                 guint           prop_id,
                                                 const GValue   *value,
                                                 GParamSpec     *pspec);
static void     moo_edit_get_property           (GObject        *object,
                                                 guint           prop_id,
                                                 GValue         *value,
                                                 GParamSpec     *pspec);

static MooSaveResponse moo_edit_before_save     (MooEdit        *doc,
                                                 GFile          *file);

static void     config_changed                  (MooEdit        *edit);
static void     update_config_from_mode_lines   (MooEdit        *doc);
static void     moo_edit_recheck_config         (MooEdit        *doc);

static void     changed_cb                      (GtkTextBuffer  *buffer,
                                                 MooEdit        *edit);
static void     modified_changed_cb             (GtkTextBuffer  *buffer,
                                                 MooEdit        *edit);

static MooLang *moo_edit_get_lang               (MooEdit        *doc);

static void     moo_edit_apply_prefs            (MooEdit        *doc);
static void     moo_edit_freeze_notify          (MooEdit        *doc);
static void     moo_edit_thaw_notify            (MooEdit        *doc);


enum {
    DOC_STATUS_CHANGED,
    FILENAME_CHANGED,
    WILL_CLOSE,
    BEFORE_SAVE,
    WILL_SAVE,
    AFTER_SAVE,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

enum {
    PROP_0,
    PROP_EDITOR,
    PROP_ENABLE_BOOKMARKS,
    PROP_HAS_COMMENTS,
    PROP_LINE_END_TYPE,
    PROP_LANG,
    PROP_ENCODING
};

G_DEFINE_TYPE (MooEdit, moo_edit, G_TYPE_OBJECT)


static void
moo_edit_class_init (MooEditClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = moo_edit_set_property;
    gobject_class->get_property = moo_edit_get_property;
    gobject_class->constructor = moo_edit_constructor;
    gobject_class->finalize = moo_edit_finalize;
    gobject_class->dispose = moo_edit_dispose;

    klass->before_save = moo_edit_before_save;

    g_type_class_add_private (klass, sizeof (MooEditPrivate));

    g_object_class_install_property (gobject_class, PROP_EDITOR,
        g_param_spec_object ("editor", "editor", "editor",
                             MOO_TYPE_EDITOR, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property (gobject_class, PROP_ENABLE_BOOKMARKS,
        g_param_spec_boolean ("enable-bookmarks", "enable-bookmarks", "enable-bookmarks",
                              TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_HAS_COMMENTS,
        g_param_spec_boolean ("has-comments", "has-comments", "has-comments",
                              FALSE, G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_LINE_END_TYPE,
        g_param_spec_enum ("line-end-type", "line-end-type", "line-end-type",
                           MOO_TYPE_LINE_END_TYPE, MOO_LE_NONE,
                           (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_ENCODING,
        g_param_spec_string ("encoding", "encoding", "encoding",
                             NULL, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_LANG,
        g_param_spec_object ("lang", "lang", "lang",
                             MOO_TYPE_LANG, G_PARAM_READABLE));

    signals[DOC_STATUS_CHANGED] =
            g_signal_new ("doc-status-changed",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditClass, doc_status_changed),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[FILENAME_CHANGED] =
            g_signal_new ("filename-changed",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_FIRST,
                          G_STRUCT_OFFSET (MooEditClass, filename_changed),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    /**
     * MooEdit::will-close:
     *
     * @doc: the object which received the signal
     *
     * This signal is emitted before the document is closed.
     **/
    signals[WILL_CLOSE] =
            g_signal_new ("will-close",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditClass, will_close),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    /**
     * MooEdit::before-save:
     *
     * @doc: the object which received the signal
     * @file: the #GFile object which represents saved file
     *
     * This signal is emitted when the document is going to be saved on disk.
     * Callbacks should return #MOO_SAVE_RESPONSE_CANCEL if document
     * should not be saved, and #MOO_SAVE_RESPONSE_CONTINUE otherwise.
     *
     * For example, if before saving the file must be checked out from a version
     * control system, a callback can do that and return #MOO_SAVE_RESPONSE_CANCEL
     * if checkout failed.
     *
     * Callbacks should not modify document content. If you need to modify
     * it before saving, use MooEdit::will-save signal instead.
     *
     * Returns: #MOO_SAVE_RESPONSE_CANCEL to cancel saving,
     * #MOO_SAVE_RESPONSE_CONTINUE otherwise.
     **/
    signals[BEFORE_SAVE] =
            g_signal_new ("before-save",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditClass, before_save),
                          moo_signal_accumulator_continue_cancel,
                          GINT_TO_POINTER (MOO_SAVE_RESPONSE_CONTINUE),
                          _moo_marshal_ENUM__OBJECT,
                          MOO_TYPE_SAVE_RESPONSE, 1,
                          G_TYPE_FILE);

    /**
     * MooEdit::will-save:
     *
     * @doc: the object which received the signal
     * @file: the #GFile object which represents saved file
     *
     * This signal is emitted when the document is going to be saved on disk,
     * after MooEdit::before-save signal. Callbacks may modify document
     * content at this point.
     **/
    signals[WILL_SAVE] =
            g_signal_new ("will-save",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditClass, will_save),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT,
                          G_TYPE_NONE, 1,
                          G_TYPE_FILE);

    /**
     * MooEdit::after-save:
     *
     * @doc: the object which received the signal
     *
     * This signal is emitted after the document has been successfully saved on disk.
     **/
    signals[AFTER_SAVE] =
            g_signal_new ("after-save",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_FIRST,
                          G_STRUCT_OFFSET (MooEditClass, after_save),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    _moo_edit_init_config ();
    _moo_edit_class_init_actions (klass);
}


MooEditPrivate::MooEditPrivate()
    : editor(nullptr)
    , views(nullptr)
    , active_view(nullptr)
    , dead_active_view(false)
    , changed_handler_id(0)
    , modified_changed_handler_id(0)
    , apply_config_idle(0)
    , in_recheck_config(0)
    , filename(nullptr)
    , norm_name(nullptr)
    , display_filename(nullptr)
    , display_basename(nullptr)
    , encoding(nullptr)
    , line_end_type(MOO_LE_NONE)
    , status(MOO_EDIT_STATUS_NORMAL)
    , file_monitor_id(0)
    , modified_on_disk(false)
    , deleted_from_disk(false)
    , sync_timeout_id(0)
    , state(MOO_EDIT_STATE_NORMAL)
    , progress(nullptr)
    , enable_bookmarks(false)
    , bookmarks(nullptr)
    , update_bookmarks_idle(0)
    , actions(nullptr)
{
}

MooEditPrivate::~MooEditPrivate()
{
}


static void
moo_edit_init (MooEdit *edit)
{
    edit->priv = G_TYPE_INSTANCE_GET_PRIVATE (edit, MOO_TYPE_EDIT, MooEditPrivate);
    new(edit->priv) MooEditPrivate;

    edit->priv->views = moo_edit_view_array_new ();
    edit->priv->buffer = GTK_TEXT_BUFFER (g_object_new (MOO_TYPE_TEXT_BUFFER, NULL));

    edit->config = moo_edit_config_new ();
    g_signal_connect_swapped (edit->config, "notify",
                              G_CALLBACK (config_changed),
                              edit);

    edit->priv->actions = moo_action_collection_new ("MooEdit", "MooEdit");

    edit->priv->line_end_type = MOO_LE_NONE;
}


static GObject *
moo_edit_constructor (GType                  type,
                      guint                  n_construct_properties,
                      GObjectConstructParam *construct_param)
{
    GObject *object;
    MooEdit *doc;
    MooEditView *view;

    object = G_OBJECT_CLASS (moo_edit_parent_class)->constructor (
        type, n_construct_properties, construct_param);

    doc = MOO_EDIT (object);

    view = _moo_edit_view_new (doc);
    (void) view; g_assert (doc->priv->views->n_elms == 1 && doc->priv->views->elms[0] == view);

    _moo_edit_add_class_actions (doc);
    _moo_edit_instances = moo_edit_list_prepend (_moo_edit_instances, doc);

    doc->priv->changed_handler_id =
            g_signal_connect (doc->priv->buffer,
                              "changed",
                              G_CALLBACK (changed_cb),
                              doc);
    doc->priv->modified_changed_handler_id =
            g_signal_connect (doc->priv->buffer,
                              "modified-changed",
                              G_CALLBACK (modified_changed_cb),
                              doc);

    _moo_edit_set_file (doc, NULL, NULL);

    g_signal_connect_swapped (doc->priv->buffer, "line-mark-moved",
                              G_CALLBACK (_moo_edit_line_mark_moved),
                              doc);
    g_signal_connect_swapped (doc->priv->buffer, "line-mark-deleted",
                              G_CALLBACK (_moo_edit_line_mark_deleted),
                              doc);

    return object;
}


static void
moo_edit_finalize (GObject *object)
{
    MooEdit *edit = MOO_EDIT (object);

    moo_edit_view_array_free (edit->priv->views);
    g_object_unref (edit->priv->buffer);
    moo_file_free (edit->priv->file);
    g_free (edit->priv->filename);
    g_free (edit->priv->norm_name);
    g_free (edit->priv->display_filename);
    g_free (edit->priv->display_basename);
    g_free (edit->priv->encoding);

    edit->priv->~MooEditPrivate();

    G_OBJECT_CLASS (moo_edit_parent_class)->finalize (object);
}

void
_moo_edit_closed (MooEdit *doc)
{
    g_return_if_fail (MOO_IS_EDIT (doc));

    moo_assert (doc->priv->state == MOO_EDIT_STATE_NORMAL);

    _moo_edit_remove_untitled (doc);
    _moo_edit_instances = moo_edit_list_remove (_moo_edit_instances, doc);

    while (!moo_edit_view_array_is_empty (doc->priv->views))
        gtk_widget_destroy (GTK_WIDGET (doc->priv->views->elms[0]));

    if (doc->config)
    {
        g_signal_handlers_disconnect_by_func (doc->config,
                                              (gpointer) config_changed,
                                              doc);
        g_object_unref (doc->config);
        doc->config = NULL;
    }

    if (doc->priv->apply_config_idle)
    {
        g_source_remove (doc->priv->apply_config_idle);
        doc->priv->apply_config_idle = 0;
    }

    if (doc->priv->file_monitor_id)
    {
        _moo_edit_stop_file_watch (doc);
        doc->priv->file_monitor_id = 0;
    }

    if (doc->priv->sync_timeout_id)
    {
        g_source_remove (doc->priv->sync_timeout_id);
        doc->priv->sync_timeout_id = 0;
    }

    if (doc->priv->update_bookmarks_idle)
    {
        g_source_remove (doc->priv->update_bookmarks_idle);
        doc->priv->update_bookmarks_idle = 0;
    }

    _moo_edit_delete_bookmarks (doc, TRUE);

    if (doc->priv->actions)
    {
        g_object_unref (doc->priv->actions);
        doc->priv->actions = NULL;
    }
}

static void
moo_edit_dispose (GObject *object)
{
    _moo_edit_closed (MOO_EDIT (object));
    G_OBJECT_CLASS (moo_edit_parent_class)->dispose (object);
}


void
_moo_edit_set_active_view (MooEdit     *doc,
                           MooEditView *view)
{
    GtkTextBuffer *buffer;

    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (MOO_IS_EDIT_VIEW (view));

    g_return_if_fail (moo_edit_view_array_find (doc->priv->views, view) >= 0);

    buffer = moo_edit_get_buffer (doc);

    if (doc->priv->active_view != NULL && doc->priv->active_view != view)
    {
        GtkTextIter iter;
        GtkTextMark *mark = _moo_edit_view_get_fake_cursor_mark (doc->priv->active_view);
        gtk_text_buffer_get_iter_at_mark (buffer, &iter, gtk_text_buffer_get_insert (buffer));
        gtk_text_buffer_move_mark (buffer, mark, &iter);
    }

    if (doc->priv->dead_active_view ||
        (doc->priv->active_view != NULL && doc->priv->active_view != view))
    {
        GtkTextIter iter;
        GtkTextBuffer *buffer = moo_edit_get_buffer (doc);
        GtkTextMark *mark = _moo_edit_view_get_fake_cursor_mark (view);
        gtk_text_buffer_get_iter_at_mark (buffer, &iter, mark);
        gtk_text_buffer_place_cursor (buffer, &iter);
    }

    doc->priv->active_view = view;
    doc->priv->dead_active_view = FALSE;
}

void
_moo_edit_add_view (MooEdit     *doc,
                    MooEditView *view)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (MOO_IS_EDIT_VIEW (view));

    g_assert (moo_edit_view_array_find (doc->priv->views, view) < 0);

    g_object_ref_sink (view);
    moo_edit_view_array_append (doc->priv->views, view);
    g_object_unref (view);

    _moo_edit_view_apply_prefs (view);
    _moo_edit_view_apply_config (view);
}

void
_moo_edit_remove_view (MooEdit     *doc,
                       MooEditView *view)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (MOO_IS_EDIT_VIEW (view));

    g_return_if_fail (moo_edit_view_array_find (doc->priv->views, view) >= 0);

    if (view == doc->priv->active_view)
    {
        doc->priv->active_view = NULL;
        doc->priv->dead_active_view = TRUE;
    }

    g_object_ref (view);

    moo_edit_view_array_remove (doc->priv->views, view);
    _moo_edit_view_unset_doc (view);

    g_object_unref (view);
}


MooActionCollection *
_moo_edit_get_actions (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    return doc->priv->actions;
}


/*
 * autosync callback
 *
 * This will write the modified document back to disc and return
 * false to signal that no further calls are necessary.
 */
static gboolean
autosync_callback (gpointer user_data)
{
    MooEdit *edit = MOO_EDIT(user_data);
    GtkTextBuffer *buffer = moo_edit_get_buffer (edit);

    // if the buffer is modified, sync it to disk
    if (gtk_text_buffer_get_modified (buffer))
        moo_edit_save(edit, NULL);

    // zero the event ID and return false to reset the one-shot mechanism
    edit->priv->sync_timeout_id = 0;
    return FALSE;
}

/*
 * text change callback
 *
 * This callback is triggered on any change. It triggers a timer which, when
 * elapsed, syncs the document to dis. If the timer is already running, it
 * is cancelled and started anew, in order to not sync unnecessarily. Since
 * this is called for any change, even undo operations, the sync callback
 * above checks if the document is modified before syncing.
 */
static void
changed_cb (G_GNUC_UNUSED GtkTextBuffer *buffer,
            MooEdit                     *edit)
{
    // nothing to do when not auto-syncing
    if (!moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_AUTO_SYNC)))
        return;

    // cancel running timer
    if (edit->priv->sync_timeout_id != 0)
        g_source_remove(edit->priv->sync_timeout_id);

    // start timer
    edit->priv->sync_timeout_id = g_timeout_add (500, autosync_callback, edit);
}

static void
modified_changed_cb (GtkTextBuffer      *buffer,
                     MooEdit            *edit)
{
    // nothing to do when auto-syncing
    if (moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_AUTO_SYNC)))
        return;

    moo_edit_set_modified (edit, gtk_text_buffer_get_modified (buffer));
}


static void
modify_status (MooEdit       *edit,
               MooEditStatus  status,
               gboolean       add)
{
    if (add)
        edit->priv->status = (MooEditStatus) (edit->priv->status | status);
    else
        edit->priv->status = (MooEditStatus) (edit->priv->status & ~status);
}

/**
 * moo_edit_set_modified:
 **/
void
moo_edit_set_modified (MooEdit            *edit,
                       gboolean            modified)
{
    gboolean buf_modified;
    GtkTextBuffer *buffer;

    g_return_if_fail (MOO_IS_EDIT (edit));

    buffer = moo_edit_get_buffer (edit);

    buf_modified =
            gtk_text_buffer_get_modified (buffer);

    if (buf_modified != modified)
    {
        g_signal_handler_block (buffer,
                                edit->priv->modified_changed_handler_id);
        gtk_text_buffer_set_modified (buffer, modified);
        g_signal_handler_unblock (buffer,
                                  edit->priv->modified_changed_handler_id);
    }

    modify_status (edit, MOO_EDIT_STATUS_MODIFIED, modified);

    _moo_edit_status_changed (edit);
}


/**
 * moo_edit_set_clean:
 **/
void
moo_edit_set_clean (MooEdit  *edit,
                    gboolean  clean)
{
    g_return_if_fail (MOO_IS_EDIT (edit));
    modify_status (edit, MOO_EDIT_STATUS_CLEAN, clean);
    _moo_edit_status_changed (edit);
}

/**
 * moo_edit_get_clean:
 **/
gboolean
moo_edit_get_clean (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), FALSE);
    return (edit->priv->status & MOO_EDIT_STATUS_CLEAN) ? TRUE : FALSE;
}


void
_moo_edit_status_changed (MooEdit *edit)
{
    g_return_if_fail (MOO_IS_EDIT (edit));
    g_signal_emit (edit, signals[DOC_STATUS_CHANGED], 0, NULL);
}


void
_moo_edit_set_status (MooEdit        *edit,
                      MooEditStatus   status)
{
    g_return_if_fail (MOO_IS_EDIT (edit));

    if (edit->priv->status != status)
    {
        edit->priv->status = status;
        _moo_edit_status_changed (edit);
    }
}


/**
 * moo_edit_is_empty:
 *
 * This function returns whether the document is "empty", i.e. is not modified,
 * is untitled, and contains no text.
 **/
gboolean
moo_edit_is_empty (MooEdit *edit)
{
    GtkTextIter start, end;

    g_return_val_if_fail (MOO_IS_EDIT (edit), FALSE);

    if (MOO_EDIT_IS_BUSY (edit) || moo_edit_is_modified (edit) || !MOO_EDIT_IS_UNTITLED (edit))
        return FALSE;

    gtk_text_buffer_get_bounds (moo_edit_get_buffer (edit), &start, &end);

    return !gtk_text_iter_compare (&start, &end);
}

/**
 * moo_edit_is_untitled:
 **/
gboolean
moo_edit_is_untitled (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), FALSE);
    return MOO_EDIT_IS_UNTITLED (edit);
}

/**
 * moo_edit_is_modified:
 **/
gboolean
moo_edit_is_modified (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), FALSE);
    return (moo_edit_get_status (edit) & MOO_EDIT_STATUS_MODIFIED) != 0;
}

/**
 * moo_edit_get_status:
 **/
MooEditStatus
moo_edit_get_status (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), (MooEditStatus) 0);
    return edit->priv->status;
}


static void
moo_edit_set_property (GObject        *object,
                       guint           prop_id,
                       const GValue   *value,
                       GParamSpec     *pspec)
{
    MooEdit *edit = MOO_EDIT (object);

    switch (prop_id)
    {
        case PROP_EDITOR:
            edit->priv->editor = (MooEditor*) g_value_get_object (value);
            break;

        case PROP_ENABLE_BOOKMARKS:
            moo_edit_set_enable_bookmarks (edit, g_value_get_boolean (value));
            break;

        case PROP_ENCODING:
            moo_edit_set_encoding (edit, g_value_get_string (value));
            break;

        case PROP_LINE_END_TYPE:
            moo_edit_set_line_end_type (edit, (MooLineEndType) g_value_get_enum (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_edit_get_property (GObject        *object,
                       guint           prop_id,
                       GValue         *value,
                       GParamSpec     *pspec)
{
    MooEdit *edit = MOO_EDIT (object);

    switch (prop_id)
    {
        case PROP_EDITOR:
            g_value_set_object (value, edit->priv->editor);
            break;

        case PROP_ENABLE_BOOKMARKS:
            g_value_set_boolean (value, edit->priv->enable_bookmarks);
            break;

        case PROP_HAS_COMMENTS:
            g_value_set_boolean (value, _moo_edit_has_comments (edit, NULL, NULL));
            break;

        case PROP_ENCODING:
            g_value_set_string (value, edit->priv->encoding);
            break;

        case PROP_LANG:
            g_value_set_object (value, moo_edit_get_lang (edit));
            break;

        case PROP_LINE_END_TYPE:
            g_value_set_enum (value, edit->priv->line_end_type);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


/**
 * moo_edit_get_file:
 *
 * Returns: (transfer full)
 **/
GFile *
moo_edit_get_file (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), NULL);
    return edit->priv->file ? g_file_dup (edit->priv->file) : NULL;
}

/**
 * moo_edit_get_filename:
 *
 * Returns: (type filename)
 **/
char *
moo_edit_get_filename (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), NULL);
    return g_strdup (edit->priv->filename);
}

char *
_moo_edit_get_normalized_name (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), NULL);
    return g_strdup (edit->priv->norm_name);
}

char *
_moo_edit_get_utf8_filename (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), NULL);
    return edit->priv->filename ? g_strdup (edit->priv->display_filename) : NULL;
}

/**
 * moo_edit_get_display_name:
 *
 * Returns: (type const-utf8)
 **/
const char *
moo_edit_get_display_name (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), NULL);
    return edit->priv->display_filename;
}

/**
 * moo_edit_get_display_basename:
 *
 * Returns: (type const-utf8)
 **/
const char *
moo_edit_get_display_basename (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), NULL);
    return edit->priv->display_basename;
}

/**
 * moo_edit_get_uri:
 *
 * Returns: (type utf8)
 **/
char*
moo_edit_get_uri (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), nullptr);
    return edit->priv->file ? g_file_get_uri (edit->priv->file) : nullptr;
}

/**
 * moo_edit_get_encoding:
 *
 * Returns: (type const-utf8)
 **/
const char *
moo_edit_get_encoding (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), NULL);
    return edit->priv->encoding;
}

/**
 * moo_edit_set_encoding:
 *
 * @edit:
 * @encoding: (type const-utf8)
 **/
void
moo_edit_set_encoding (MooEdit    *edit,
                       const char *encoding)
{
    g_return_if_fail (MOO_IS_EDIT (edit));
    g_return_if_fail (encoding != NULL);

    if (!moo_str_equal (encoding, edit->priv->encoding))
    {
        MOO_ASSIGN_STRING (edit->priv->encoding, encoding);
        g_object_notify (G_OBJECT (edit), "encoding");
    }
}


/**
 * moo_edit_get_editor:
 **/
MooEditor *
moo_edit_get_editor (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    return doc->priv->editor;
}

/**
 * moo_edit_get_tab:
 **/
MooEditTab *
moo_edit_get_tab (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    return moo_edit_view_get_tab (moo_edit_get_view (doc));
}

/**
 * moo_edit_get_view:
 *
 * Get the active view of this document.
 **/
MooEditView *
moo_edit_get_view (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);

    if (!doc->priv->active_view)
        if (!moo_edit_view_array_is_empty (doc->priv->views))
            doc->priv->active_view = doc->priv->views->elms[doc->priv->views->n_elms - 1];

    return doc->priv->active_view;
}

/**
 * moo_edit_get_views:
 *
 * Get the list of views which belong to this document.
 **/
MooEditViewArray *
moo_edit_get_views (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    return moo_edit_view_array_copy (doc->priv->views);
}

/**
 * moo_edit_get_n_views:
 *
 * Get the number of views which belong to this document.
 **/
int
moo_edit_get_n_views (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), 0);
    return (int) moo_edit_view_array_get_size (doc->priv->views);
}

/**
 * moo_edit_get_window:
 **/
MooEditWindow *
moo_edit_get_window (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    return moo_edit_view_get_window (moo_edit_get_view (doc));
}

/**
 * moo_edit_get_buffer:
 **/
GtkTextBuffer *
moo_edit_get_buffer (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    return doc->priv->buffer;
}


typedef void (*SetVarFunc) (MooEdit    *edit,
                            const char *name,
                            char       *val);

static void
parse_mode_string (MooEdit    *edit,
                   char       *string,
                   const char *var_val_separator,
                   SetVarFunc  func)
{
    char **vars, **p;

    vars = NULL;

    g_strstrip (string);

    vars = g_strsplit (string, ";", 0);

    if (!vars)
        goto out;

    for (p = vars; *p != NULL; p++)
    {
        char *sep, *var, *value;

        g_strstrip (*p);
        sep = strstr (*p, var_val_separator);

        if (!sep || sep == *p || !sep[1])
            goto out;

        var = g_strndup (*p, sep - *p);
        g_strstrip (var);

        if (!var[0])
        {
            g_free (var);
            goto out;
        }

        value = sep + 1;
        g_strstrip (value);

        if (!value)
        {
            g_free (var);
            goto out;
        }

        func (edit, var, value);

        g_free (var);
    }

out:
    g_strfreev (vars);
}


static void
set_kate_var (MooEdit    *edit,
              const char *name,
              const char *val)
{
    if (!g_ascii_strcasecmp (name, "space-indent"))
    {
        gboolean spaces = FALSE;

        if (moo_edit_config_parse_bool (val, &spaces))
            moo_edit_config_parse_one (edit->config, "indent-use-tabs",
                                       spaces ? "false" : "true",
                                       MOO_EDIT_CONFIG_SOURCE_FILE);
    }
    else
    {
        moo_edit_config_parse_one (edit->config, name, val,
                                   MOO_EDIT_CONFIG_SOURCE_FILE);
    }
}

static void
parse_kate_mode_string (MooEdit *edit,
                        char    *string)
{
    parse_mode_string (edit, string, " ", (SetVarFunc) set_kate_var);
}


static void
set_emacs_var (MooEdit    *edit,
               const char *name,
               char       *val)
{
    if (!g_ascii_strcasecmp (name, "mode"))
    {
        moo_edit_config_parse_one (edit->config, "lang", val,
                                   MOO_EDIT_CONFIG_SOURCE_FILE);
    }
    else if (!g_ascii_strcasecmp (name, "tab-width"))
    {
        moo_edit_config_parse_one (edit->config, "tab-width", val,
                                   MOO_EDIT_CONFIG_SOURCE_FILE);
    }
    else if (!g_ascii_strcasecmp (name, "c-basic-offset"))
    {
        moo_edit_config_parse_one (edit->config, "indent-width", val,
                                   MOO_EDIT_CONFIG_SOURCE_FILE);
    }
    else if (!g_ascii_strcasecmp (name, "indent-tabs-mode"))
    {
        if (!g_ascii_strcasecmp (val, "nil"))
            moo_edit_config_parse_one (edit->config, "indent-use-tabs", "false",
                                       MOO_EDIT_CONFIG_SOURCE_FILE);
        else
            moo_edit_config_parse_one (edit->config, "indent-use-tabs", "true",
                                       MOO_EDIT_CONFIG_SOURCE_FILE);
    }
}

static void
parse_emacs_mode_string (MooEdit *edit,
                         char    *string)
{
    MooLangMgr *mgr;

    g_strstrip (string);

    mgr = moo_lang_mgr_default ();

    if (_moo_lang_mgr_find_lang (mgr, string))
        set_emacs_var (edit, "mode", string);
    else
        parse_mode_string (edit, string, ":", set_emacs_var);
}


static void
parse_moo_mode_string (MooEdit *edit,
                       char    *string)
{
    moo_edit_config_parse (edit->config, string, MOO_EDIT_CONFIG_SOURCE_FILE);
}


#define KATE_MODE_STRING        "kate:"
#define EMACS_MODE_STRING       "-*-"
#define MOO_MODE_STRING         "-%-"

static void
try_mode_string (MooEdit    *edit,
                 char       *string)
{
    char *start, *end;

    if ((start = strstr (string, KATE_MODE_STRING)))
    {
        start += strlen (KATE_MODE_STRING);
        parse_kate_mode_string (edit, start);
        return;
    }

    if ((start = strstr (string, EMACS_MODE_STRING)))
    {
        start += strlen (EMACS_MODE_STRING);

        if ((end = strstr (start, EMACS_MODE_STRING)) && end > start)
        {
            end[0] = 0;
            parse_emacs_mode_string (edit, start);
            return;
        }
    }

    if ((start = strstr (string, MOO_MODE_STRING)))
    {
        start += strlen (MOO_MODE_STRING);

        if ((end = strstr (start, MOO_MODE_STRING)) && end > start)
        {
            end[0] = 0;
            parse_moo_mode_string (edit, start);
            return;
        }
    }
}

static void
update_config_from_mode_lines (MooEdit *edit)
{
    GtkTextBuffer *buffer = moo_edit_get_buffer (edit);
    GtkTextIter start, end;
    char *first = NULL, *second = NULL, *last = NULL;

    gtk_text_buffer_get_start_iter (buffer, &start);

    if (!gtk_text_iter_ends_line (&start))
    {
        end = start;
        gtk_text_iter_forward_to_line_end (&end);
        first = gtk_text_buffer_get_slice (buffer, &start, &end, TRUE);
    }

    if (gtk_text_iter_forward_line (&start))
    {
        if (!gtk_text_iter_ends_line (&start))
        {
            end = start;
            gtk_text_iter_forward_to_line_end (&end);
            second = gtk_text_buffer_get_slice (buffer, &start, &end, TRUE);
        }
    }

    gtk_text_buffer_get_end_iter (buffer, &end);

    if (gtk_text_iter_starts_line (&end))
    {
        gtk_text_iter_backward_line (&end);
        start = end;
        gtk_text_iter_forward_to_line_end (&end);
    }
    else
    {
        start = end;
        gtk_text_iter_set_line_offset (&start, 0);
    }

    if (gtk_text_iter_get_line (&start) != 1 &&
        gtk_text_iter_get_line (&start) != 2)
            last = gtk_text_buffer_get_slice (buffer, &start, &end, TRUE);

    if (first)
        try_mode_string (edit, first);
    if (second)
        try_mode_string (edit, second);
    if (last)
        try_mode_string (edit, last);

    g_free (first);
    g_free (second);
    g_free (last);
}


static MooLang *
moo_edit_get_lang (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    moo_assert (!doc->priv->in_recheck_config);
    return moo_text_buffer_get_lang (MOO_TEXT_BUFFER (doc->priv->buffer));
}

/**
 * moo_edit_get_lang_id:
 *
 * Returns: (type utf8): id of language currently used in the document. If no language
 * is used, then string "none" is returned.
 */
char *
moo_edit_get_lang_id (MooEdit *doc)
{
    MooLang *lang = NULL;

    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);

    if (!doc->priv->in_recheck_config)
    {
        lang = moo_edit_get_lang (doc);
    }
    else
    {
        const char *lang_id = moo_edit_config_get_string (doc->config, "lang");
        lang = lang_id ? _moo_lang_mgr_find_lang (moo_lang_mgr_default (), lang_id) : NULL;
    }

    return g_strdup (_moo_lang_id (lang));
}

static gboolean
moo_edit_apply_config_all_in_idle (void)
{
    MooEditList *l;

    moo_edit_apply_config_all_idle = 0;

    for (l = _moo_edit_instances; l != NULL; l = l->next)
        moo_edit_recheck_config (l->data);

    return FALSE;
}

void
_moo_edit_queue_recheck_config_all (void)
{
    if (!moo_edit_apply_config_all_idle)
        moo_edit_apply_config_all_idle =
            g_idle_add ((GSourceFunc) moo_edit_apply_config_all_in_idle, NULL);
}


static void
update_lang_config_from_lang_globs (MooEdit *doc)
{
    const char *lang_id = NULL;

    if (doc->priv->file)
    {
        MooLangMgr *mgr = moo_lang_mgr_default ();
        MooLang *lang = moo_lang_mgr_get_lang_for_file (mgr, doc->priv->file);
        lang_id = lang ? _moo_lang_id (lang) : NULL;
    }

    moo_edit_config_set (doc->config, MOO_EDIT_CONFIG_SOURCE_FILENAME,
                         "lang", lang_id, (char*) NULL);
}

static void
update_config_from_filter_settings (MooEdit *doc)
{
    char *filter_config = NULL;

    filter_config = _moo_edit_filter_settings_get_for_doc (doc);

    if (filter_config)
        moo_edit_config_parse (doc->config, filter_config,
                               MOO_EDIT_CONFIG_SOURCE_FILENAME);

    g_free (filter_config);
}

static void
update_config_from_lang (MooEdit *doc)
{
    char *lang_id = moo_edit_get_lang_id (doc);
    _moo_lang_mgr_update_config (moo_lang_mgr_default (), doc->config, lang_id);
    g_free (lang_id);
}

static void
moo_edit_apply_config (MooEdit *doc)
{
    guint i;
    const char *lang_id = moo_edit_config_get_string (doc->config, "lang");
    MooLangMgr *mgr = moo_lang_mgr_default ();
    MooLang *lang = lang_id ? _moo_lang_mgr_find_lang (mgr, lang_id) : NULL;

    moo_text_buffer_set_lang (MOO_TEXT_BUFFER (doc->priv->buffer), lang);

    g_object_notify (G_OBJECT (doc), "has-comments");
    g_object_notify (G_OBJECT (doc), "lang");

    for (i = 0; i < doc->priv->views->n_elms; ++i)
        _moo_edit_view_apply_config (doc->priv->views->elms[i]);
}

static void
moo_edit_recheck_config (MooEdit *doc)
{
    g_return_if_fail (!doc->priv->in_recheck_config);

    if (doc->priv->apply_config_idle)
    {
        g_source_remove (doc->priv->apply_config_idle);
        doc->priv->apply_config_idle = 0;
    }

    moo_edit_freeze_notify (doc);
    doc->priv->in_recheck_config = TRUE;

    // this resets settings from global config
    moo_edit_config_unset_by_source (doc->config, MOO_EDIT_CONFIG_SOURCE_FILE);

    // First global settings
    moo_edit_apply_prefs (doc);

    // then language from globs
    update_lang_config_from_lang_globs (doc);

    // then settings from mode lines, these may change lang
    update_config_from_mode_lines (doc);

    // then filter settings, these also may change lang
    update_config_from_filter_settings (doc);

    // update config for lang
    update_config_from_lang (doc);

    // finally apply config
    moo_edit_apply_config (doc);

    doc->priv->in_recheck_config = FALSE;
    moo_edit_thaw_notify (doc);
}

static gboolean
moo_edit_recheck_config_in_idle (MooEdit *edit)
{
    edit->priv->apply_config_idle = 0;
    moo_edit_recheck_config (edit);
    return FALSE;
}

void
_moo_edit_queue_recheck_config (MooEdit *doc)
{
    g_return_if_fail (!doc->priv->in_recheck_config);
    if (!doc->priv->apply_config_idle)
        doc->priv->apply_config_idle =
                g_idle_add_full (G_PRIORITY_HIGH,
                                 (GSourceFunc) moo_edit_recheck_config_in_idle,
                                 doc, NULL);
}

static void
config_changed (MooEdit *doc)
{
    if (!doc->priv->in_recheck_config)
        _moo_edit_queue_recheck_config (doc);
}


static void
moo_edit_apply_prefs (MooEdit *edit)
{
    guint i;

    g_return_if_fail (MOO_IS_EDIT (edit));

    moo_edit_freeze_notify (edit);

    for (i = 0; i < edit->priv->views->n_elms; ++i)
        _moo_edit_view_apply_prefs (edit->priv->views->elms[i]);

    moo_edit_thaw_notify (edit);
}

static void
moo_edit_freeze_notify (MooEdit *doc)
{
    guint i;

    g_return_if_fail (MOO_IS_EDIT (doc));

    g_object_freeze_notify (G_OBJECT (doc));

    for (i = 0; i < doc->priv->views->n_elms; ++i)
       g_object_freeze_notify (G_OBJECT (doc->priv->views->elms[i]));
}

static void
moo_edit_thaw_notify (MooEdit *doc)
{
    guint i;

    g_return_if_fail (MOO_IS_EDIT (doc));

    for (i = 0; i < doc->priv->views->n_elms; ++i)
       g_object_thaw_notify (G_OBJECT (doc->priv->views->elms[i]));

    g_object_thaw_notify (G_OBJECT (doc));
}


/**
 * moo_edit_reload:
 *
 * @edit:
 * @info: (allow-none) (default NULL):
 * @error:
 *
 * Reload document from disk
 *
 * Returns: whether document was successfully reloaded
 **/
gboolean
moo_edit_reload (MooEdit       *doc,
                 MooReloadInfo *info,
                 GError       **error)
{
    moo_return_error_if_fail (MOO_IS_EDIT (doc));
    return moo_editor_reload (doc->priv->editor, doc, info, error);
}

/**
 * moo_edit_close:
 *
 * Returns: whether document was closed
 **/
gboolean
moo_edit_close (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), FALSE);
    return moo_editor_close_doc (edit->priv->editor, edit);
}

static MooSaveResponse
moo_edit_before_save (G_GNUC_UNUSED MooEdit *doc,
                      G_GNUC_UNUSED GFile *file)
{
    return MOO_SAVE_RESPONSE_CONTINUE;
}

/**
 * moo_edit_save:
 **/
gboolean
moo_edit_save (MooEdit *doc,
               GError **error)
{
    moo_return_error_if_fail (MOO_IS_EDIT (doc));
    return moo_editor_save (doc->priv->editor, doc, error);
}

/**
 * moo_edit_save_as:
 *
 * @doc:
 * @info: (allow-none):
 * @error:
 *
 * Save document with new filename and/or encoding. If @info is
 * missing or %NULL then the user is asked for a new filename first.
 **/
gboolean
moo_edit_save_as (MooEdit     *doc,
                  MooSaveInfo *info,
                  GError     **error)
{
    moo_return_error_if_fail (MOO_IS_EDIT (doc));
    return moo_editor_save_as (doc->priv->editor, doc, info, error);
}

/**
 * moo_edit_save_copy:
 **/
gboolean
moo_edit_save_copy (MooEdit     *doc,
                    MooSaveInfo *info,
                    GError     **error)
{
    moo_return_error_if_fail (MOO_IS_EDIT (doc));
    return moo_editor_save_copy (doc->priv->editor, doc, info, error);
}


gboolean
_moo_edit_is_busy (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), FALSE);
    return _moo_edit_get_state (doc) != MOO_EDIT_STATE_NORMAL;
}

MooEditState
_moo_edit_get_state (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), MOO_EDIT_STATE_NORMAL);
    return doc->priv->state;
}

void
_moo_edit_set_progress_text (MooEdit    *doc,
                             const char *text)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (doc->priv->state != MOO_EDIT_STATE_NORMAL);
    g_return_if_fail (doc->priv->progress != NULL);
    _moo_edit_progress_set_text (doc->priv->progress, text);
}

void
_moo_edit_set_state (MooEdit        *doc,
                     MooEditState    state,
                     const char     *text,
                     GDestroyNotify  cancel,
                     gpointer        data)
{
    guint i;
    MooEditTab *tab;

    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (state == MOO_EDIT_STATE_NORMAL ||
                      doc->priv->state == MOO_EDIT_STATE_NORMAL);

    if (doc->priv->progress)
        _moo_edit_progress_set_cancel_func (doc->priv->progress, cancel, data);

    if (state == doc->priv->state)
        return;

    doc->priv->state = state;

    for (i = 0; i < moo_edit_view_array_get_size (doc->priv->views); ++i)
        gtk_text_view_set_editable (GTK_TEXT_VIEW (doc->priv->views->elms[i]), !state);

    tab = moo_edit_get_tab (doc);

    if (!tab)
        return;

    if (!state)
    {
        _moo_edit_tab_destroy_progress (tab);
        g_object_unref (doc->priv->progress);
        doc->priv->progress = NULL;
    }
    else
    {
        doc->priv->progress = _moo_edit_tab_create_progress (tab);
        _moo_edit_progress_start (doc->priv->progress, text, cancel, data);
        g_object_ref (doc->priv->progress);
    }
}


/*****************************************************************************/
/* Comment/uncomment
 */

/* TODO: all this stuff, it's pretty lame */

gboolean
_moo_edit_has_comments (MooEdit  *edit,
                        gboolean *single_line,
                        gboolean *multi_line)
{
    MooLang *lang;
    gboolean single, multi;

    lang = moo_edit_get_lang (edit);

    if (!lang)
        return FALSE;

    single = _moo_lang_get_line_comment (lang) != NULL;
    multi = _moo_lang_get_block_comment_start (lang) && _moo_lang_get_block_comment_end (lang);

    if (single_line)
        *single_line = single;
    if (multi_line)
        *multi_line = multi;

    return single || multi;
}


static void
line_comment (GtkTextBuffer *buffer,
              const char    *comment_string,
              GtkTextIter   *start,
              GtkTextIter   *end)
{
    int first, last, i;
    GtkTextIter iter;
    char *comment_and_space;

    g_return_if_fail (comment_string && comment_string[0]);

    first = gtk_text_iter_get_line (start);
    last = gtk_text_iter_get_line (end);

    if (!gtk_text_iter_equal (start, end) && gtk_text_iter_starts_line (end))
        last -= 1;

    comment_and_space = g_strdup_printf ("%s ", comment_string);

    for (i = first; i <= last; ++i)
    {
        gtk_text_buffer_get_iter_at_line (buffer, &iter, i);

        if (gtk_text_iter_ends_line (&iter))
            gtk_text_buffer_insert (buffer, &iter, comment_string, -1);
        else
            gtk_text_buffer_insert (buffer, &iter, comment_and_space, -1);
    }

    g_free (comment_and_space);
}


static void
line_uncomment (GtkTextBuffer *buffer,
                const char    *comment_string,
                GtkTextIter   *start,
                GtkTextIter   *end)
{
    int first, last, i;
    guint chars;

    g_return_if_fail (comment_string && comment_string[0]);

    first = gtk_text_iter_get_line (start);
    last = gtk_text_iter_get_line (end);

    if (!gtk_text_iter_equal (start, end) && gtk_text_iter_starts_line (end))
        last -= 1;

    chars = g_utf8_strlen (comment_string, -1);

    for (i = first; i <= last; ++i)
    {
        char *text;

        gtk_text_buffer_get_iter_at_line (buffer, start, i);
        *end = *start;
        gtk_text_iter_forward_chars (end, chars);
        text = gtk_text_iter_get_slice (start, end);

        if (!strcmp (comment_string, text))
        {
            if (gtk_text_iter_get_char (end) == ' ')
                gtk_text_iter_forward_char (end);
            gtk_text_buffer_delete (buffer, start, end);
        }

        g_free (text);
    }
}


static void
iter_to_line_end (GtkTextIter *iter)
{
    if (!gtk_text_iter_ends_line (iter))
        gtk_text_iter_forward_to_line_end (iter);
}

static void
block_comment (GtkTextBuffer *buffer,
               const char    *comment_start,
               const char    *comment_end,
               GtkTextIter   *start,
               GtkTextIter   *end)
{
    GtkTextMark *end_mark;

    g_return_if_fail (comment_start && comment_start[0]);
    g_return_if_fail (comment_end && comment_end[0]);

    if (gtk_text_iter_equal (start, end))
    {
        gtk_text_iter_set_line_offset (start, 0);
        iter_to_line_end (end);
    }
    else
    {
        if (gtk_text_iter_starts_line (end))
        {
            gtk_text_iter_backward_line (end);
            iter_to_line_end (end);
        }
    }

    end_mark = gtk_text_buffer_create_mark (buffer, NULL, end, FALSE);
    gtk_text_buffer_insert (buffer, start, comment_start, -1);
    gtk_text_buffer_get_iter_at_mark (buffer, start, end_mark);
    gtk_text_buffer_insert (buffer, start, comment_end, -1);
    gtk_text_buffer_delete_mark (buffer, end_mark);
}


static void
block_uncomment (GtkTextBuffer *buffer,
                 const char    *comment_start,
                 const char    *comment_end,
                 GtkTextIter   *start,
                 GtkTextIter   *end)
{
    GtkTextIter start1, start2, end1, end2;
    GtkTextMark *mark1, *mark2;
    GtkTextIter limit;
    gboolean found;

    g_return_if_fail (comment_start && comment_start[0]);
    g_return_if_fail (comment_end && comment_end[0]);

    if (!gtk_text_iter_equal (start, end) && gtk_text_iter_starts_line (end))
    {
        gtk_text_iter_backward_line (end);
        iter_to_line_end (end);
    }

    limit = *end;
    found = moo_text_search_forward (start, comment_start, (MooTextSearchFlags) 0,
                                     &start1, &start2,
                                     &limit);

    if (!found)
    {
        gtk_text_iter_set_line_offset (&limit, 0);
        found = gtk_text_iter_backward_search (start, comment_start, (GtkTextSearchFlags) 0,
                                               &start1, &start2,
                                               &limit);
    }

    if (!found)
        return;

    limit = start2;
    found = gtk_text_iter_backward_search (end, comment_end, (GtkTextSearchFlags) 0,
                                           &end1, &end2, &limit);

    if (!found)
    {
        limit = *end;
        iter_to_line_end (&limit);
        found = moo_text_search_forward (end, comment_end, (MooTextSearchFlags) 0,
                                         &end1, &end2, &limit);
    }

    if (!found)
        return;

    g_assert (gtk_text_iter_compare (&start2, &end1) < 0);

    mark1 = gtk_text_buffer_create_mark (buffer, NULL, &end1, FALSE);
    mark2 = gtk_text_buffer_create_mark (buffer, NULL, &end2, FALSE);
    gtk_text_buffer_delete (buffer, &start1, &start2);
    gtk_text_buffer_get_iter_at_mark (buffer, &end1, mark1);
    gtk_text_buffer_get_iter_at_mark (buffer, &end2, mark2);
    gtk_text_buffer_delete (buffer, &end1, &end2);
    gtk_text_buffer_delete_mark (buffer, mark1);
    gtk_text_buffer_delete_mark (buffer, mark2);
}


/**
 * moo_edit_comment_selection:
 **/
void
moo_edit_comment_selection (MooEdit *edit)
{
    MooLang *lang;
    GtkTextIter start, end;
    GtkTextBuffer *buffer;
    gboolean has_selection, single_line, multi_line;
    gboolean adjust_selection = FALSE, move_insert = FALSE;
    int sel_start_line = 0, sel_start_offset = 0;

    g_return_if_fail (MOO_IS_EDIT (edit));

    lang = moo_edit_get_lang (edit);

    if (!_moo_edit_has_comments (edit, &single_line, &multi_line))
        return;

    buffer = moo_edit_get_buffer (edit);
    has_selection = gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

    gtk_text_buffer_begin_user_action (buffer);

    if (has_selection)
    {
        GtkTextIter iter;
        adjust_selection = TRUE;
        gtk_text_buffer_get_iter_at_mark (buffer, &iter,
                                          gtk_text_buffer_get_insert (buffer));
        move_insert = gtk_text_iter_equal (&iter, &start);
        sel_start_line = gtk_text_iter_get_line (&start);
        sel_start_offset = gtk_text_iter_get_line_offset (&start);
    }

    /* FIXME */
    if (single_line)
        line_comment (buffer, _moo_lang_get_line_comment (lang), &start, &end);
    else
        block_comment (buffer,
                       _moo_lang_get_block_comment_start (lang),
                       _moo_lang_get_block_comment_end (lang),
                       &start, &end);


    if (adjust_selection)
    {
        const char *mark = move_insert ? "insert" : "selection_bound";
        gtk_text_buffer_get_iter_at_line_offset (buffer, &start,
                                                 sel_start_line,
                                                 sel_start_offset);
        gtk_text_buffer_move_mark_by_name (buffer, mark, &start);
    }

    gtk_text_buffer_end_user_action (buffer);
}


/**
 * moo_edit_uncomment_selection:
 **/
void
moo_edit_uncomment_selection (MooEdit *edit)
{
    MooLang *lang;
    GtkTextIter start, end;
    GtkTextBuffer *buffer;
    gboolean single_line, multi_line;

    g_return_if_fail (MOO_IS_EDIT (edit));

    lang = moo_edit_get_lang (edit);

    if (!_moo_edit_has_comments (edit, &single_line, &multi_line))
        return;

    buffer = moo_edit_get_buffer (edit);
    gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

    gtk_text_buffer_begin_user_action (buffer);

    /* FIXME */
    if (single_line)
        line_uncomment (buffer, _moo_lang_get_line_comment (lang), &start, &end);
    else
        block_uncomment (buffer,
                         _moo_lang_get_block_comment_start (lang),
                         _moo_lang_get_block_comment_end (lang),
                         &start, &end);

    gtk_text_buffer_end_user_action (buffer);
}


void
_moo_edit_ensure_newline (MooEdit *edit)
{
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    g_return_if_fail (MOO_IS_EDIT (edit));

    buffer = moo_edit_get_buffer (edit);
    gtk_text_buffer_get_end_iter (buffer, &iter);

    if (!gtk_text_iter_starts_line (&iter))
        gtk_text_buffer_insert (buffer, &iter, "\n", -1);
}

void
_moo_edit_strip_whitespace (MooEdit *doc)
{
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    g_return_if_fail (MOO_IS_EDIT (doc));

    buffer = moo_edit_get_buffer (doc);
    gtk_text_buffer_begin_user_action (buffer);

    for (gtk_text_buffer_get_start_iter (buffer, &iter);
         !gtk_text_iter_is_end (&iter);
         gtk_text_iter_forward_line (&iter))
    {
        GtkTextIter end;
        char *slice, *p;
        gssize len;

        if (gtk_text_iter_ends_line (&iter))
            continue;

        end = iter;
        gtk_text_iter_forward_to_line_end (&end);

        slice = gtk_text_buffer_get_slice (buffer, &iter, &end, TRUE);
        len = strlen (slice);
        g_assert (len > 0);

        for (p = slice + len; p > slice && (p[-1] == ' ' || p[-1] == '\t'); --p) ;

        if (*p)
        {
            gtk_text_iter_forward_chars (&iter, g_utf8_pointer_to_offset (slice, p));
            gtk_text_buffer_delete (buffer, &iter, &end);
        }

        g_free (slice);
    }

    gtk_text_buffer_end_user_action (buffer);
}
