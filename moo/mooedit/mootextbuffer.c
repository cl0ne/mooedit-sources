/*
 *   mootextbuffer.c
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
 * class:MooTextBuffer: (parent GtkTextBuffer) (moo.private 1)
 **/

#include "mooedit/mootextiter.h"
#include "mooedit/mootext-private.h"
#include "mooedit/moolang-private.h"
#include "mooedit/mootextstylescheme.h"
#include "marshals.h"
#include "mooutils/mooundo.h"
#include "mooutils/mooutils-gobject.h"
#include <string.h>


struct MooTextBufferPrivate {
    gboolean has_selection;
    gboolean has_text;

    MooTextStyleScheme *style_scheme;
    GtkSourceEngine *engine;
    MooLang *lang;
    gboolean may_apply_tag;
    gboolean do_highlight;

    gboolean highlight_matching_brackets;
    gboolean highlight_mismatching_brackets;
    guint num_brackets;
    gunichar *left_brackets, *right_brackets;
    GtkTextTag *correct_match_tag;
    GtkTextTag *incorrect_match_tag;
    GtkTextMark *bracket_mark[4];
    MooBracketMatchType bracket_found;

    LineBuffer *line_buf;
    MooFoldTree *fold_tree;

    guint non_interactive;
    int cursor_moved_frozen;
    gboolean cursor_moved;
    MooUndoStack *undo_stack;
    gpointer modifying_action;
    int move_cursor_to;
#if 0
    int cursor_was_at;
#endif
};


static void     moo_text_buffer_dispose             (GObject        *object);

static void     moo_text_buffer_set_property        (GObject        *object,
                                                     guint           prop_id,
                                                     const GValue   *value,
                                                     GParamSpec     *pspec);
static void     moo_text_buffer_get_property        (GObject        *object,
                                                     guint           prop_id,
                                                     GValue         *value,
                                                     GParamSpec     *pspec);

static void     moo_text_buffer_cursor_moved        (MooTextBuffer      *buffer,
                                                     const GtkTextIter  *iter);
static void     moo_text_buffer_mark_set            (GtkTextBuffer      *buffer,
                                                     const GtkTextIter  *iter,
                                                     GtkTextMark        *mark);
static void     moo_text_buffer_insert_text         (GtkTextBuffer      *buffer,
                                                     GtkTextIter        *pos,
                                                     const gchar        *text,
                                                     gint                length);
static void     moo_text_buffer_delete_range        (GtkTextBuffer      *buffer,
                                                     GtkTextIter        *start,
                                                     GtkTextIter        *end);
static void     moo_text_buffer_begin_user_action   (GtkTextBuffer      *buffer);
static void     moo_text_buffer_end_user_action     (GtkTextBuffer      *buffer);
static void     moo_text_buffer_modified_changed    (GtkTextBuffer      *buffer);

static void     moo_text_buffer_highlight_brackets  (MooTextBuffer      *buffer,
                                                     const GtkTextIter  *insert);
static void     moo_text_buffer_unhighlight_brackets(MooTextBuffer      *buffer);
static void     moo_text_buffer_set_bracket_match_style (MooTextBuffer  *buffer,
                                                     const MooTextStyle *style);
static void     moo_text_buffer_set_bracket_mismatch_style (MooTextBuffer *buffer,
                                                     const MooTextStyle *style);

static void     emit_cursor_moved                   (MooTextBuffer      *buffer,
                                                     const GtkTextIter  *iter);
static void     freeze_cursor_moved                 (MooTextBuffer      *buffer);
static void     thaw_cursor_moved                   (MooTextBuffer      *buffer);


static guint    INSERT_ACTION_TYPE;
static guint    DELETE_ACTION_TYPE;
static void     init_undo_actions                   (void);
static MooUndoAction *insert_action_new             (GtkTextBuffer      *buffer,
                                                     GtkTextIter        *pos,
                                                     const char         *text,
                                                     int                 length);
static MooUndoAction *delete_action_new             (GtkTextBuffer      *buffer,
                                                     GtkTextIter        *start,
                                                     GtkTextIter        *end);
#if 0
static void     before_undo_redo                    (MooTextBuffer      *buffer);
#endif
static void     after_undo_redo                     (MooTextBuffer      *buffer);
static void     proxy_notify_can_undo_redo          (MooTextBuffer      *buffer,
                                                     GParamSpec         *pspec);

static void     line_mark_moved                     (MooTextBuffer      *buffer,
                                                     MooLineMark        *mark);
static void     line_mark_deleted                   (MooTextBuffer      *buffer,
                                                     MooLineMark        *mark);


enum {
    HIGHLIGHT_UPDATED, /* GtkSourceBuffer's */
    CURSOR_MOVED,
    SELECTION_CHANGED,
    LINE_MARK_ADDED,
    LINE_MARK_MOVED,
    LINE_MARK_DELETED,
    FOLD_ADDED,
    FOLD_DELETED,
    FOLD_TOGGLED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


enum {
    PROP_0,
    PROP_HIGHLIGHT_SYNTAX, /* mimic GtkSourceBuffer */
    PROP_HIGHLIGHT_MATCHING_BRACKETS,
    PROP_HIGHLIGHT_MISMATCHING_BRACKETS,
    PROP_BRACKET_MATCH_STYLE,
    PROP_BRACKET_MISMATCH_STYLE,
    PROP_HAS_TEXT,
    PROP_HAS_SELECTION,
    PROP_LANG,
    PROP_CAN_UNDO,
    PROP_CAN_REDO
};


/* MOO_TYPE_TEXT_BUFFER */
G_DEFINE_TYPE (MooTextBuffer, moo_text_buffer, GTK_TYPE_TEXT_BUFFER)


static void
moo_text_buffer_class_init (MooTextBufferClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkTextBufferClass *buffer_class = GTK_TEXT_BUFFER_CLASS (klass);

    init_undo_actions ();

    gobject_class->set_property = moo_text_buffer_set_property;
    gobject_class->get_property = moo_text_buffer_get_property;
    gobject_class->dispose = moo_text_buffer_dispose;

    buffer_class->mark_set = moo_text_buffer_mark_set;
    buffer_class->insert_text = moo_text_buffer_insert_text;
    buffer_class->delete_range = moo_text_buffer_delete_range;
    buffer_class->begin_user_action = moo_text_buffer_begin_user_action;
    buffer_class->end_user_action = moo_text_buffer_end_user_action;
    buffer_class->modified_changed = moo_text_buffer_modified_changed;

    klass->cursor_moved = moo_text_buffer_cursor_moved;

    g_type_class_add_private (klass, sizeof (MooTextBufferPrivate));

    g_object_class_install_property (gobject_class,
                                     PROP_HIGHLIGHT_SYNTAX,
                                     g_param_spec_boolean ("highlight-syntax",
                                             "highlight-syntax",
                                             "highlight-syntax",
                                             TRUE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_HIGHLIGHT_MATCHING_BRACKETS,
                                     g_param_spec_boolean ("highlight-matching-brackets",
                                             "highlight-matching-brackets",
                                             "highlight-matching-brackets",
                                             TRUE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_HIGHLIGHT_MISMATCHING_BRACKETS,
                                     g_param_spec_boolean ("highlight-mismatching-brackets",
                                             "highlight-mismatching-brackets",
                                             "highlight-mismatching-brackets",
                                             FALSE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_BRACKET_MATCH_STYLE,
                                     g_param_spec_object ("bracket-match-style",
                                             "bracket-match-style",
                                             "bracket-match-style",
                                             MOO_TYPE_TEXT_STYLE,
                                             (GParamFlags) (G_PARAM_CONSTRUCT | G_PARAM_WRITABLE)));

    g_object_class_install_property (gobject_class,
                                     PROP_BRACKET_MISMATCH_STYLE,
                                     g_param_spec_object ("bracket-mismatch-style",
                                             "bracket-mismatch-style",
                                             "bracket-mismatch-style",
                                             MOO_TYPE_TEXT_STYLE,
                                             (GParamFlags) (G_PARAM_CONSTRUCT | G_PARAM_WRITABLE)));

    g_object_class_install_property (gobject_class,
                                     PROP_HAS_TEXT,
                                     g_param_spec_boolean ("has-text",
                                             "has-text",
                                             "has-text",
                                             FALSE,
                                             G_PARAM_READABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_HAS_SELECTION,
                                     g_param_spec_boolean ("has-selection",
                                             "has-selection",
                                             "has-selection",
                                             FALSE,
                                             G_PARAM_READABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_LANG,
                                     g_param_spec_object ("lang",
                                             "lang",
                                             "lang",
                                             MOO_TYPE_LANG,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_CAN_UNDO,
                                     g_param_spec_boolean ("can-undo",
                                             "can-undo",
                                             "can-undo",
                                             FALSE,
                                             G_PARAM_READABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_CAN_REDO,
                                     g_param_spec_boolean ("can-redo",
                                             "can-redo",
                                             "can-redo",
                                             FALSE,
                                             G_PARAM_READABLE));

    signals[HIGHLIGHT_UPDATED] =
            g_signal_new ("highlight_updated",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          0, NULL, NULL,
                          _moo_marshal_VOID__BOXED_BOXED,
                          G_TYPE_NONE, 2,
                          GTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                          GTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE);

    signals[CURSOR_MOVED] =
            g_signal_new ("cursor-moved",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTextBufferClass, cursor_moved),
                          NULL, NULL,
                          _moo_marshal_VOID__BOXED,
                          G_TYPE_NONE, 1,
                          GTK_TYPE_TEXT_ITER);

    signals[SELECTION_CHANGED] =
            g_signal_new ("selection-changed",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTextBufferClass, selection_changed),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[LINE_MARK_ADDED] =
            g_signal_new ("line-mark-added",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTextBufferClass, line_mark_added),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT,
                          G_TYPE_NONE, 1,
                          MOO_TYPE_LINE_MARK);

    signals[LINE_MARK_MOVED] =
            g_signal_new ("line-mark-moved",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTextBufferClass, line_mark_moved),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT,
                          G_TYPE_NONE, 1,
                          MOO_TYPE_LINE_MARK);

    signals[LINE_MARK_DELETED] =
            g_signal_new ("line-mark-deleted",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTextBufferClass, line_mark_deleted),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT,
                          G_TYPE_NONE, 1,
                          MOO_TYPE_LINE_MARK);

    signals[FOLD_ADDED] =
            g_signal_new ("fold-added",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTextBufferClass, fold_added),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT,
                          G_TYPE_NONE, 1,
                          MOO_TYPE_FOLD);

    signals[FOLD_DELETED] =
            g_signal_new ("fold-deleted",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTextBufferClass, fold_deleted),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT,
                          G_TYPE_NONE, 1,
                          MOO_TYPE_FOLD);

    signals[FOLD_TOGGLED] =
            g_signal_new ("fold-toggled",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTextBufferClass, fold_toggled),
                          NULL, NULL,
                          _moo_marshal_VOID__OBJECT,
                          G_TYPE_NONE, 1,
                          MOO_TYPE_FOLD);
}


static void
moo_text_buffer_init (MooTextBuffer *buffer)
{
    buffer->priv = G_TYPE_INSTANCE_GET_PRIVATE (buffer, MOO_TYPE_TEXT_BUFFER, MooTextBufferPrivate);

    buffer->priv->line_buf = _moo_line_buffer_new ();
    buffer->priv->do_highlight = TRUE;
    buffer->priv->bracket_found = MOO_BRACKET_MATCH_NONE;

    buffer->priv->fold_tree = _moo_fold_tree_new (buffer);

    buffer->priv->undo_stack = moo_undo_stack_new (buffer);
    buffer->priv->move_cursor_to = -1;
#if 0
    g_signal_connect_swapped (buffer->priv->undo_stack, "undo",
                              G_CALLBACK (before_undo_redo),
                              buffer);
    g_signal_connect_swapped (buffer->priv->undo_stack, "redo",
                              G_CALLBACK (before_undo_redo),
                              buffer);
#endif
    g_signal_connect_data (buffer->priv->undo_stack, "undo",
                           G_CALLBACK (after_undo_redo),
                           buffer, NULL,
                           G_CONNECT_AFTER | G_CONNECT_SWAPPED);
    g_signal_connect_data (buffer->priv->undo_stack, "redo",
                           G_CALLBACK (after_undo_redo),
                           buffer, NULL,
                           G_CONNECT_AFTER | G_CONNECT_SWAPPED);

    g_signal_connect_swapped (buffer->priv->undo_stack, "notify::can-undo",
                              G_CALLBACK (proxy_notify_can_undo_redo), buffer);
    g_signal_connect_swapped (buffer->priv->undo_stack, "notify::can-redo",
                              G_CALLBACK (proxy_notify_can_undo_redo), buffer);

    moo_text_buffer_set_brackets (buffer, NULL);
}


static void
moo_text_buffer_dispose (GObject *object)
{
    MooTextBuffer *buffer = MOO_TEXT_BUFFER (object);

    if (buffer->priv->fold_tree)
    {
        /* XXX leak if folds are not deleted */
        _moo_fold_tree_free (buffer->priv->fold_tree);
        buffer->priv->fold_tree = NULL;
    }

    if (buffer->priv->engine)
    {
    	_gtk_source_engine_attach_buffer (buffer->priv->engine, NULL);
    	g_object_unref (buffer->priv->engine);
    	buffer->priv->engine = NULL;
    }

    if (buffer->priv->style_scheme)
    {
    	g_object_unref (buffer->priv->style_scheme);
    	buffer->priv->style_scheme = NULL;
    }

    if (buffer->priv->lang)
    {
    	g_object_unref (buffer->priv->lang);
    	buffer->priv->engine = NULL;
    }

    if (buffer->priv->line_buf)
    {
        _moo_line_buffer_free (buffer->priv->line_buf);
        buffer->priv->line_buf = NULL;
    }

    g_free (buffer->priv->left_brackets);
    g_free (buffer->priv->right_brackets);
    buffer->priv->left_brackets = NULL;
    buffer->priv->right_brackets = NULL;

#if 0
    g_signal_handlers_disconnect_by_func (buffer->priv->undo_stack,
                                          (gpointer) before_undo_redo,
                                          buffer);
#endif
    if (buffer->priv->undo_stack)
    {
        g_signal_handlers_disconnect_by_func (buffer->priv->undo_stack,
                                              (gpointer) after_undo_redo,
                                              buffer);
        g_object_unref (buffer->priv->undo_stack);
        buffer->priv->undo_stack = NULL;
    }

    G_OBJECT_CLASS (moo_text_buffer_parent_class)->dispose (object);
}


GtkTextBuffer *
moo_text_buffer_new (GtkTextTagTable *table)
{
    return GTK_TEXT_BUFFER (g_object_new (MOO_TYPE_TEXT_BUFFER,
                                          "tag-table", table,
                                          (const char*) NULL));
}


gpointer
_moo_text_buffer_get_undo_stack (MooTextBuffer *buffer)
{
    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), NULL);
    return buffer->priv->undo_stack;
}


gboolean
moo_text_buffer_can_redo (MooTextBuffer *buffer)
{
    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), FALSE);
    return moo_undo_stack_can_redo (buffer->priv->undo_stack);
}

gboolean
moo_text_buffer_can_undo (MooTextBuffer *buffer)
{
    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), FALSE);
    return moo_undo_stack_can_undo (buffer->priv->undo_stack);
}


static void
moo_text_buffer_begin_user_action (GtkTextBuffer *text_buffer)
{
    MooTextBuffer *buffer = MOO_TEXT_BUFFER (text_buffer);
    moo_text_buffer_freeze (buffer);
    moo_undo_stack_start_group (buffer->priv->undo_stack);
}


static void
moo_text_buffer_end_user_action (GtkTextBuffer *text_buffer)
{
    MooTextBuffer *buffer = MOO_TEXT_BUFFER (text_buffer);
    moo_text_buffer_thaw (buffer);
    moo_undo_stack_end_group (buffer->priv->undo_stack);
}


static void
moo_text_buffer_modified_changed (GtkTextBuffer *text_buffer)
{
    if (!gtk_text_buffer_get_modified (text_buffer))
        MOO_TEXT_BUFFER(text_buffer)->priv->modifying_action = NULL;
}


static void
update_selection (MooTextBuffer *buffer)
{
    GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER (buffer);
    gboolean has_selection;
    gboolean selection_changed = TRUE;

    has_selection = gtk_text_buffer_get_selection_bounds (text_buffer, NULL, NULL) != 0;

    selection_changed = has_selection || buffer->priv->has_selection;

    if (has_selection != buffer->priv->has_selection)
    {
        buffer->priv->has_selection = has_selection;
        g_object_notify (G_OBJECT (buffer), "has-selection");
    }

    if (selection_changed)
        g_signal_emit (buffer, signals[SELECTION_CHANGED], 0);
}


static void
moo_text_buffer_mark_set (GtkTextBuffer      *text_buffer,
                          const GtkTextIter  *iter,
                          GtkTextMark        *mark)
{
    GtkTextMark *insert = gtk_text_buffer_get_insert (text_buffer);
    GtkTextMark *sel_bound = gtk_text_buffer_get_selection_bound (text_buffer);
    MooTextBuffer *buffer = MOO_TEXT_BUFFER (text_buffer);

    if (GTK_TEXT_BUFFER_CLASS(moo_text_buffer_parent_class)->mark_set)
        GTK_TEXT_BUFFER_CLASS(moo_text_buffer_parent_class)->mark_set (text_buffer, iter, mark);

    if (mark == insert || mark == sel_bound)
        update_selection (buffer);

    if (mark == insert)
    {
        moo_text_buffer_unhighlight_brackets (buffer);
        emit_cursor_moved (buffer, iter);
    }
}


static GtkTextTag *
get_placeholder_tag (GtkTextBuffer *text_buffer)
{
    return gtk_text_tag_table_lookup (gtk_text_buffer_get_tag_table (text_buffer),
                                      MOO_PLACEHOLDER_TAG);
}


static void
moo_text_buffer_insert_text (GtkTextBuffer      *text_buffer,
                             GtkTextIter        *pos,
                             const gchar        *text,
                             gint                length)
{
    MooTextBuffer *buffer = MOO_TEXT_BUFFER (text_buffer);
    GtkTextTag *tag;
    int first_line, last_line;
    int start_offset, end_offset;
    gboolean starts_line, ins_line;

    if (!text[0])
        return;

    if (length < 0)
        length = (int) strlen (text);

    start_offset = gtk_text_iter_get_offset (pos);
    first_line = gtk_text_iter_get_line (pos);
    starts_line = gtk_text_iter_starts_line (pos);
    ins_line = (text[0] == '\n' || text[0] == '\r');

    if ((tag = get_placeholder_tag (text_buffer)) &&
         gtk_text_iter_has_tag (pos, tag) &&
         !gtk_text_iter_begins_tag (pos, tag))
    {
        GtkTextIter tag_start = *pos;
        GtkTextIter tag_end = *pos;
        gtk_text_iter_backward_to_tag_toggle (&tag_start, tag);
        gtk_text_iter_forward_to_tag_toggle (&tag_end, tag);
        gtk_text_buffer_remove_tag (text_buffer, tag, &tag_start, &tag_end);
    }

    if (!moo_undo_stack_frozen (buffer->priv->undo_stack))
    {
        MooUndoAction *action;
        action = insert_action_new (text_buffer, pos, text, length);
        moo_undo_stack_add_action (buffer->priv->undo_stack, INSERT_ACTION_TYPE, action);
    }

    moo_text_buffer_unhighlight_brackets (buffer);

    GTK_TEXT_BUFFER_CLASS(moo_text_buffer_parent_class)->insert_text (text_buffer, pos, text, length);

    last_line = gtk_text_iter_get_line (pos);

    if (last_line != first_line)
        _moo_line_buffer_split_line (buffer->priv->line_buf,
                                     first_line, last_line - first_line);

    /* XXX btree can do it better ? i guess it can't */
    if (starts_line && ins_line)
    {
        GSList *l, *marks;
        marks = moo_text_buffer_get_line_marks_at_line (buffer, first_line);
        for (l = marks; l != NULL; l = l->next)
            moo_text_buffer_move_line_mark (buffer, l->data, last_line);
        g_slist_free (marks);
    }

    emit_cursor_moved (buffer, pos);

    end_offset = gtk_text_iter_get_offset (pos);

    if (buffer->priv->engine)
        _gtk_source_engine_text_inserted (buffer->priv->engine, start_offset, end_offset);

    if (!buffer->priv->has_text)
    {
        buffer->priv->has_text = TRUE;
        g_object_notify (G_OBJECT (buffer), "has-text");
    }
}


static void
marks_moved_or_deleted (MooTextBuffer *buffer,
                        GSList        *moved,
                        GSList        *deleted)
{
    GSList *l;

    if (!moved && !deleted)
        return;

    g_slist_foreach (moved, (GFunc) g_object_ref, NULL);
    g_slist_foreach (deleted, (GFunc) g_object_ref, NULL);

    for (l = deleted; l != NULL; l = l->next)
        _moo_line_mark_set_buffer (l->data, NULL, NULL);

    for (l = deleted; l != NULL; l = l->next)
    {
        line_mark_deleted (buffer, l->data);
        g_object_unref (l->data);
    }

    for (l = moved; l != NULL; l = l->next)
        if (!moo_line_mark_get_deleted (l->data))
            line_mark_moved (buffer, l->data);

    g_slist_foreach (deleted, (GFunc) g_object_unref, NULL);
    g_slist_foreach (moved, (GFunc) g_object_unref, NULL);
}

static void
moo_text_buffer_delete_range (GtkTextBuffer      *text_buffer,
                              GtkTextIter        *start,
                              GtkTextIter        *end)
{
    MooTextBuffer *buffer = MOO_TEXT_BUFFER (text_buffer);
    int first_line, last_line, offset, length;
    gboolean starts_line;
    GSList *deleted_marks = NULL, *moved_marks = NULL;
    GtkTextTag *tag;

    gtk_text_iter_order (start, end);

    first_line = gtk_text_iter_get_line (start);
    last_line = gtk_text_iter_get_line (end);
    starts_line = gtk_text_iter_starts_line (start);
    offset = gtk_text_iter_get_offset (start);
    length = gtk_text_iter_get_offset (end) - offset;

    g_return_if_fail (length > 0);

    if ((tag = get_placeholder_tag (text_buffer)) &&
         (gtk_text_iter_has_tag (end, tag) ||
         gtk_text_iter_has_tag (start, tag)))
    {
        GtkTextIter tag_start = *start;
        GtkTextIter tag_end = *end;

        if (gtk_text_iter_has_tag (&tag_start, tag) &&
            !gtk_text_iter_begins_tag (&tag_start, tag))
                gtk_text_iter_backward_to_tag_toggle (&tag_start, tag);

        if (gtk_text_iter_has_tag (&tag_end, tag))
            gtk_text_iter_forward_to_tag_toggle (&tag_end, tag);

        gtk_text_buffer_remove_tag (text_buffer, tag, &tag_start, &tag_end);
    }

    moo_text_buffer_unhighlight_brackets (buffer);

#define MANY_LINES 1000
    if (buffer->priv->lang && buffer->priv->do_highlight &&
        last_line - first_line > MANY_LINES)
    {
        gtk_text_buffer_remove_all_tags (text_buffer, start, end);
    }
#undef MANY_LINES

    if (!moo_undo_stack_frozen (buffer->priv->undo_stack))
    {
        MooUndoAction *action;
        action = delete_action_new (text_buffer, start, end);
        moo_undo_stack_add_action (buffer->priv->undo_stack, DELETE_ACTION_TYPE, action);
    }

    GTK_TEXT_BUFFER_CLASS(moo_text_buffer_parent_class)->delete_range (text_buffer, start, end);

    if (first_line < last_line)
    {
        if (starts_line)
            _moo_line_buffer_delete (buffer->priv->line_buf,
                                     first_line,
                                     last_line - first_line,
                                     -1, &moved_marks, &deleted_marks);
        else
            _moo_line_buffer_delete (buffer->priv->line_buf,
                                     first_line + 1,
                                     last_line - first_line,
                                     first_line,
                                     &moved_marks, &deleted_marks);
    }

    /* It would be better if marks were moved/deleted before deleting text, but it
       could cause problems with invalidated iters. if they were deleted after
       deleting text, it would be even worse since our btree and gtk btree would not
       be syncronized when callbacks are called. So doing it here is safest (though
       not most simple or clean) */
    marks_moved_or_deleted (buffer, moved_marks, deleted_marks);
    g_slist_free (deleted_marks);
    g_slist_free (moved_marks);

    update_selection (buffer);
    emit_cursor_moved (buffer, start);

    if (buffer->priv->has_text)
    {
        buffer->priv->has_text = moo_text_buffer_has_text (buffer);
        if (!buffer->priv->has_text)
            g_object_notify (G_OBJECT (buffer), "has-text");
    }

    if (buffer->priv->engine != NULL)
        _gtk_source_engine_text_deleted (buffer->priv->engine, offset, length);
}


#if 0
static void
before_undo_redo (MooTextBuffer *buffer)
{
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (buffer), &iter,
                                      gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (buffer)));
    buffer->priv->cursor_was_at = gtk_text_iter_get_offset (&iter);
}
#endif

static void
after_undo_redo (MooTextBuffer *buffer)
{
    GtkTextIter iter;
    int move_to = -1;

    if (buffer->priv->move_cursor_to >= 0)
        move_to = buffer->priv->move_cursor_to;
#if 0
    else
        move_to = buffer->priv->cursor_was_at;
#endif

    if (move_to >= 0)
    {
        gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (buffer), &iter, move_to);
        gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (buffer), &iter);
    }

    buffer->priv->move_cursor_to = -1;
#if 0
    buffer->priv->cursor_was_at = -1;
#endif
}

static void
proxy_notify_can_undo_redo (MooTextBuffer *buffer,
                            GParamSpec    *pspec)
{
    g_object_notify (G_OBJECT (buffer), pspec->name);
}


void
moo_text_buffer_set_lang (MooTextBuffer  *buffer,
                          MooLang        *lang)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));

    if (buffer->priv->lang == lang)
        return;

    if (buffer->priv->engine)
    {
    	_gtk_source_engine_attach_buffer (buffer->priv->engine, NULL);
    	g_object_unref (buffer->priv->engine);
    	buffer->priv->engine = NULL;
    }

    if (buffer->priv->lang)
        g_object_unref (buffer->priv->lang);

    buffer->priv->lang = lang;

    if (lang)
    {
        g_object_ref (lang);

    	buffer->priv->engine = _moo_lang_get_engine (lang);

    	if (buffer->priv->engine)
    	{
            _gtk_source_engine_attach_buffer (buffer->priv->engine,
    						  GTK_TEXT_BUFFER (buffer));

            if (buffer->priv->style_scheme)
                _gtk_source_engine_set_style_scheme (buffer->priv->engine,
                                                     GTK_SOURCE_STYLE_SCHEME (buffer->priv->style_scheme));
    	}
    }

    moo_text_buffer_set_brackets (buffer, lang ? _moo_lang_get_brackets (lang) : NULL);
}


MooLang*
moo_text_buffer_get_lang (MooTextBuffer  *buffer)
{
    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), NULL);
    return buffer->priv->lang;
}


void
moo_text_buffer_set_highlight (MooTextBuffer *buffer,
                               gboolean       highlight)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));

    highlight = highlight != 0;

    if (highlight == buffer->priv->do_highlight)
        return;

    buffer->priv->do_highlight = highlight;
    g_object_notify (G_OBJECT (buffer), "highlight-syntax");
}


gboolean
moo_text_buffer_get_highlight (MooTextBuffer *buffer)
{
    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), FALSE);
    return buffer->priv->do_highlight;
}


static void
moo_text_buffer_set_property (GObject        *object,
                              guint           prop_id,
                              const GValue   *value,
                              GParamSpec     *pspec)
{
    MooTextBuffer *buffer = MOO_TEXT_BUFFER (object);

    switch (prop_id)
    {
        case PROP_HIGHLIGHT_SYNTAX:
            moo_text_buffer_set_highlight (buffer, g_value_get_boolean (value));
            break;

        case PROP_HIGHLIGHT_MATCHING_BRACKETS:
            buffer->priv->highlight_matching_brackets = g_value_get_boolean (value) != 0;
            g_object_notify (object, "highlight-matching-brackets");
            break;

        case PROP_HIGHLIGHT_MISMATCHING_BRACKETS:
            buffer->priv->highlight_mismatching_brackets = g_value_get_boolean (value) != 0;
            g_object_notify (object, "highlight-mismatching-brackets");
            break;

        case PROP_BRACKET_MATCH_STYLE:
            moo_text_buffer_set_bracket_match_style (buffer, g_value_get_object (value));
            break;

        case PROP_BRACKET_MISMATCH_STYLE:
            moo_text_buffer_set_bracket_mismatch_style (buffer, g_value_get_object (value));
            break;

        case PROP_LANG:
            moo_text_buffer_set_lang (buffer, g_value_get_object (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_text_buffer_get_property (GObject        *object,
                              guint           prop_id,
                              GValue         *value,
                              GParamSpec     *pspec)
{
    MooTextBuffer *buffer = MOO_TEXT_BUFFER (object);

    switch (prop_id)
    {
        case PROP_HIGHLIGHT_SYNTAX:
            g_value_set_boolean (value, buffer->priv->do_highlight);
            break;

        case PROP_HIGHLIGHT_MATCHING_BRACKETS:
            g_value_set_boolean (value, buffer->priv->highlight_matching_brackets);
            break;

        case PROP_HIGHLIGHT_MISMATCHING_BRACKETS:
            g_value_set_boolean (value, buffer->priv->highlight_mismatching_brackets);
            break;

        case PROP_LANG:
            g_value_set_object (value, buffer->priv->lang);
            break;

        case PROP_HAS_TEXT:
            g_value_set_boolean (value, moo_text_buffer_has_text (buffer));
            break;

        case PROP_HAS_SELECTION:
            g_value_set_boolean (value, buffer->priv->has_selection);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


gboolean
moo_text_buffer_has_text (MooTextBuffer *buffer)
{
    GtkTextIter start, end;

    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), TRUE);

    gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer), &start, &end);
    return gtk_text_iter_compare (&start, &end) ? TRUE : FALSE;
}


gboolean
moo_text_buffer_has_selection (MooTextBuffer *buffer)
{
    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), FALSE);
    return gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (buffer), NULL, NULL);
}


void
_moo_text_buffer_update_highlight (MooTextBuffer      *buffer,
                                   const GtkTextIter  *start,
                                   const GtkTextIter  *end,
                                   gboolean            synchronous)
{
	g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));

	if (buffer->priv->engine != NULL)
		_gtk_source_engine_update_highlight (buffer->priv->engine,
						     start, end, synchronous);
}


#if 0
// void
// _moo_text_buffer_apply_syntax_tag (MooTextBuffer      *buffer,
//                                    GtkTextTag         *tag,
//                                    const GtkTextIter  *start,
//                                    const GtkTextIter  *end)
// {
//     GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER (buffer);
//     buffer->priv->may_apply_tag = TRUE;
//     gtk_text_buffer_apply_tag (text_buffer, tag, start, end);
//     buffer->priv->may_apply_tag = FALSE;
// }
#endif


void
_moo_text_buffer_set_style_scheme (MooTextBuffer      *buffer,
                                   MooTextStyleScheme *scheme)
{
    MooTextStyle *style;

    g_return_if_fail (MOO_IS_TEXT_STYLE_SCHEME (scheme));
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));

    if (scheme == buffer->priv->style_scheme)
        return;

    if (buffer->priv->style_scheme)
        g_object_unref (buffer->priv->style_scheme);
    buffer->priv->style_scheme = g_object_ref (scheme);

    style = _moo_text_style_scheme_get_bracket_match_style (scheme);
    moo_text_buffer_set_bracket_match_style (buffer, style);

    style = _moo_text_style_scheme_get_bracket_mismatch_style (scheme);
    moo_text_buffer_set_bracket_mismatch_style (buffer, style);

    if (buffer->priv->engine)
        _gtk_source_engine_set_style_scheme (buffer->priv->engine,
                                             GTK_SOURCE_STYLE_SCHEME (scheme));
}


void
moo_text_buffer_freeze (MooTextBuffer *buffer)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));
    freeze_cursor_moved (buffer);
}


void
moo_text_buffer_thaw (MooTextBuffer *buffer)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));
    thaw_cursor_moved (buffer);
}


void
moo_text_buffer_begin_non_undoable_action (MooTextBuffer *buffer)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));
    moo_undo_stack_freeze (buffer->priv->undo_stack);
}

void
moo_text_buffer_end_non_undoable_action (MooTextBuffer *buffer)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));
    moo_undo_stack_thaw (buffer->priv->undo_stack);
}


void
moo_text_buffer_begin_non_interactive_action (MooTextBuffer *buffer)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));
    buffer->priv->non_interactive++;
}


void
moo_text_buffer_end_non_interactive_action (MooTextBuffer *buffer)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));
    g_return_if_fail (buffer->priv->non_interactive > 0);
    buffer->priv->non_interactive--;
}


int
moo_text_iter_get_visual_line_length (const GtkTextIter *iter,
                                      int                tab_width)
{
    GtkTextIter end = *iter;

    if (!gtk_text_iter_ends_line (iter))
        gtk_text_iter_forward_to_line_end (&end);

    return moo_text_iter_get_visual_line_offset (&end, tab_width);
}


int
moo_text_iter_get_visual_line_offset (const GtkTextIter  *iter,
                                      int                 tab_width)
{
    int offset = 0;
    GtkTextIter start = *iter;

    gtk_text_iter_set_line_offset (&start, 0);

    while (gtk_text_iter_compare (&start, iter) < 0)
    {
        if (gtk_text_iter_get_char (&start) == '\t')
            offset += tab_width - offset % tab_width;
        else
            offset += 1;
        gtk_text_iter_forward_char (&start);
    }

    return offset;
}


void
moo_text_iter_set_visual_line_offset (GtkTextIter *iter,
                                      int          offset,
                                      int          tab_width)
{
    int i = 0;

    gtk_text_iter_set_line_offset (iter, 0);

    while (!gtk_text_iter_ends_line (iter) && i < offset)
    {
        if (gtk_text_iter_get_char (iter) != '\t')
            i += 1;
        else
            i += tab_width - i % tab_width;

        if (i > offset)
            break;

        gtk_text_iter_forward_char (iter);
    }
}


/*****************************************************************************/
/* Matching brackets
 */

#define FIND_BRACKETS_LIMIT 3000

static void
emit_cursor_moved (MooTextBuffer      *buffer,
                   const GtkTextIter  *where)
{
    if (!buffer->priv->cursor_moved_frozen)
        g_signal_emit (buffer, signals[CURSOR_MOVED], 0, where);
    else
        buffer->priv->cursor_moved = TRUE;
}

static void
freeze_cursor_moved (MooTextBuffer *buffer)
{
    buffer->priv->cursor_moved_frozen++;
}

static void
thaw_cursor_moved (MooTextBuffer *buffer)
{
    g_return_if_fail (buffer->priv->cursor_moved_frozen > 0);

    /* XXX it should check cursor after text is rehighlighted */
    if (!--buffer->priv->cursor_moved_frozen && buffer->priv->cursor_moved)
    {
        GtkTextIter iter;
        GtkTextMark *insert = gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (buffer));
        gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (buffer), &iter, insert);
        emit_cursor_moved (buffer, &iter);
        buffer->priv->cursor_moved = FALSE;
    }
}


static void
moo_text_buffer_highlight_brackets (MooTextBuffer     *buffer,
                                    const GtkTextIter *insert)
{
    GtkTextIter iter[4];
    MooBracketMatchType bracket_match;
    GtkTextTag *tag = NULL;
    GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER (buffer);
    int i;

    if (!insert)
    {
        static GtkTextIter insert_iter;
        gtk_text_buffer_get_iter_at_mark (text_buffer, &insert_iter,
                                          gtk_text_buffer_get_insert (text_buffer));
        insert = &insert_iter;
    }

    moo_text_buffer_unhighlight_brackets (buffer);

    if (!buffer->priv->highlight_matching_brackets &&
        !buffer->priv->highlight_mismatching_brackets)
            return;

    iter[0] = *insert;

    if (!moo_text_iter_at_bracket (&iter[0]))
        return;

    iter[2] = iter[0];
    bracket_match = moo_text_iter_find_matching_bracket (&iter[2], FIND_BRACKETS_LIMIT);

    buffer->priv->bracket_found = bracket_match;

    switch (bracket_match)
    {
        case MOO_BRACKET_MATCH_CORRECT:
            if (buffer->priv->highlight_matching_brackets)
                tag = buffer->priv->correct_match_tag;
            break;
        case MOO_BRACKET_MATCH_INCORRECT:
            if (buffer->priv->highlight_mismatching_brackets)
                tag = buffer->priv->incorrect_match_tag;
            break;
        default:
            tag = NULL;
    }

    if (tag)
    {
        iter[1] = iter[0];
        gtk_text_iter_forward_char (&iter[1]);

        iter[3] = iter[2];
        gtk_text_iter_forward_char (&iter[3]);

        if (!buffer->priv->bracket_mark[0])
            for (i = 0; i < 4; ++i)
                buffer->priv->bracket_mark[i] =
                        gtk_text_buffer_create_mark (text_buffer, NULL, &iter[i], i/2 == 0);
        else
            for (i = 0; i < 4; ++i)
                gtk_text_buffer_move_mark (text_buffer, buffer->priv->bracket_mark[i], &iter[i]);

        gtk_text_buffer_apply_tag (text_buffer, tag, &iter[0], &iter[1]);
        gtk_text_buffer_apply_tag (text_buffer, tag, &iter[2], &iter[3]);
    }
}


static void
moo_text_buffer_unhighlight_brackets (MooTextBuffer *buffer)
{
    GtkTextTag *tag;
    GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER (buffer);

    switch (buffer->priv->bracket_found)
    {
        case MOO_BRACKET_MATCH_CORRECT:
            tag = buffer->priv->correct_match_tag;
            break;
        case MOO_BRACKET_MATCH_INCORRECT:
            tag = buffer->priv->incorrect_match_tag;
            break;
        default:
            return;
    }

    if (tag && buffer->priv->bracket_mark[0])
    {
        GtkTextIter iter[4];
        guint i;

        for (i = 0; i < 4; ++i)
            gtk_text_buffer_get_iter_at_mark (text_buffer, &iter[i],
                                              buffer->priv->bracket_mark[i]);

        gtk_text_buffer_remove_tag (text_buffer, tag, &iter[0], &iter[1]);
        gtk_text_buffer_remove_tag (text_buffer, tag, &iter[2], &iter[3]);
    }

    buffer->priv->bracket_found = MOO_BRACKET_MATCH_NONE;
}


static void
moo_text_buffer_cursor_moved (MooTextBuffer      *buffer,
                              const GtkTextIter  *where)
{
    g_return_if_fail (where != NULL);
    moo_text_buffer_highlight_brackets (buffer, where);
}


static void
moo_text_buffer_set_bracket_match_style (MooTextBuffer      *buffer,
                                         const MooTextStyle *style)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));

    if (!buffer->priv->correct_match_tag)
        buffer->priv->correct_match_tag =
                gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (buffer), NULL, NULL);

    _moo_text_style_apply_to_tag (style, buffer->priv->correct_match_tag);
    g_object_notify (G_OBJECT (buffer), "bracket-match-style");
}


static void
moo_text_buffer_set_bracket_mismatch_style (MooTextBuffer      *buffer,
                                            const MooTextStyle *style)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));

    if (!buffer->priv->incorrect_match_tag)
        buffer->priv->incorrect_match_tag =
                gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (buffer), NULL, NULL);

    _moo_text_style_apply_to_tag (style, buffer->priv->incorrect_match_tag);
    g_object_notify (G_OBJECT (buffer), "bracket-mismatch-style");
}


gboolean
_moo_text_buffer_is_bracket_tag (MooTextBuffer *buffer,
                                 GtkTextTag    *tag)
{
    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), FALSE);

    return tag == buffer->priv->correct_match_tag ||
           tag == buffer->priv->incorrect_match_tag;
}


static gboolean
parse_brackets (const char     *string,
                gunichar      **left_brackets,
                gunichar      **right_brackets,
                guint          *num)
{
    long len, i;
    const char *p;
    gunichar *left, *right;

    g_return_val_if_fail (g_utf8_validate (string, -1, NULL), FALSE);
    len = g_utf8_strlen (string, -1);
    g_return_val_if_fail (len > 0 && (len / 2) * 2 == len, FALSE);

    len /= 2;
    p = string;
    left = g_new (gunichar, len);
    right = g_new (gunichar, len);

    for (i = 0; i < len; ++i)
    {
        left[i] = g_utf8_get_char (p);
        p = g_utf8_next_char (p);
        right[i] = g_utf8_get_char (p);
        p = g_utf8_next_char (p);
    }

    *left_brackets = left;
    *right_brackets = right;
    *num = len;

    return TRUE;
}


void
moo_text_buffer_set_brackets (MooTextBuffer *buffer,
                              const gchar   *string)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));

    buffer->priv->num_brackets = 0;
    g_free (buffer->priv->left_brackets);
    buffer->priv->left_brackets = NULL;
    g_free (buffer->priv->right_brackets);
    buffer->priv->right_brackets = NULL;

    if (!string)
    {
        buffer->priv->num_brackets = 3;
        buffer->priv->left_brackets = g_new (gunichar, 3);
        buffer->priv->right_brackets = g_new (gunichar, 3);
        buffer->priv->left_brackets[0] = '(';
        buffer->priv->left_brackets[1] = '{';
        buffer->priv->left_brackets[2] = '[';
        buffer->priv->right_brackets[0] = ')';
        buffer->priv->right_brackets[1] = '}';
        buffer->priv->right_brackets[2] = ']';
        return;
    }
    else if (!string[0])
    {
        return;
    }

    parse_brackets (string,
                    &(buffer->priv->left_brackets),
                    &(buffer->priv->right_brackets),
                    &(buffer->priv->num_brackets));
}


inline static gboolean
find (gunichar *vec, guint size, gunichar c)
{
    guint i;

    for (i = 0; i < size; ++i)
        if (c == vec[i])
            return TRUE;

    return FALSE;
}


gboolean
moo_text_iter_at_bracket (GtkTextIter *iter)
{
    gunichar left, right;
    GtkTextIter prev;
    MooTextBuffer *buffer;

    buffer = MOO_TEXT_BUFFER (gtk_text_iter_get_buffer (iter));
    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), FALSE);

    left = 0;
    right = gtk_text_iter_get_char (iter);
    prev = *iter;

    if (!gtk_text_iter_starts_line (&prev) && gtk_text_iter_backward_char (&prev))
        left = gtk_text_iter_get_char (&prev);

    if (right && find (buffer->priv->left_brackets, buffer->priv->num_brackets, right))
        return TRUE;

    if (left && (find (buffer->priv->left_brackets, buffer->priv->num_brackets, left) ||
        find (buffer->priv->right_brackets, buffer->priv->num_brackets, left)))
    {
        *iter = prev;
        return TRUE;
    }

    if (right && find (buffer->priv->right_brackets, buffer->priv->num_brackets, right))
        return TRUE;

    return FALSE;
}


MooBracketMatchType
moo_text_iter_find_matching_bracket (GtkTextIter *iter,
                                     int          limit)
{
    int addition = 0;
    int count;
    gunichar *same_direction = NULL, *inverse_direction = NULL;
    guint to_find;
    gunichar bracket = gtk_text_iter_get_char (iter);
    gunichar bracket_to_find = 0;
    guint stack = 0;
    GtkTextIter b;
    MooTextBuffer *buffer;

    buffer = MOO_TEXT_BUFFER (gtk_text_iter_get_buffer (iter));
    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), MOO_BRACKET_MATCH_NOT_AT_BRACKET);

    for (to_find = 0; to_find < buffer->priv->num_brackets; ++to_find)
    {
        if (bracket == buffer->priv->left_brackets[to_find])
        {
            bracket_to_find = buffer->priv->right_brackets[to_find];
            addition = 1;
            inverse_direction = buffer->priv->right_brackets;
            same_direction = buffer->priv->left_brackets;
            break;
        }
    }

    if (to_find == buffer->priv->num_brackets)
    {
        for (to_find = 0; to_find < buffer->priv->num_brackets; ++to_find)
        {
            if (bracket == buffer->priv->right_brackets[to_find])
            {
                bracket_to_find = buffer->priv->left_brackets[to_find];
                addition = -1;
                same_direction = buffer->priv->right_brackets;
                inverse_direction = buffer->priv->left_brackets;
                break;
            }
        }
    }

    if (to_find == buffer->priv->num_brackets)
        return MOO_BRACKET_MATCH_NOT_AT_BRACKET;

    stack = 0;
    b = *iter;

    if (limit < 0)
        limit = G_MAXINT;
    count = 0;

    while (gtk_text_iter_forward_chars (&b, addition) && count++ < limit)
    {
        gunichar c = gtk_text_iter_get_char (&b);

        if (c == bracket_to_find && !stack)
        {
            *iter = b;
            return MOO_BRACKET_MATCH_CORRECT;
        }

        if (find (same_direction, buffer->priv->num_brackets, c))
        {
            ++stack;
        }
        else if (find (inverse_direction, buffer->priv->num_brackets, c))
        {
            if (stack)
            {
                --stack;
            }
            else
            {
                *iter = b;
                return MOO_BRACKET_MATCH_INCORRECT;
            }
        }
    }

    return MOO_BRACKET_MATCH_NONE;
}


/*****************************************************************************/
/* Undo/redo
 */

typedef enum {
    ACTION_INSERT,
    ACTION_DELETE
} ActionType;

typedef struct {
    char *text;
    guint interactive : 1;
    guint mergeable   : 1;
} EditAction;

typedef struct {
    EditAction edit;
    int pos;
    int length;
    int chars;
} InsertAction;

typedef struct {
    EditAction edit;
    int start;
    int end;
    guint forward : 1;
} DeleteAction;

#define EDIT_ACTION(action__)           ((EditAction*)action__)
#define ACTION_INTERACTIVE(action__)    (((EditAction*)action__)->interactive)

static void     edit_action_destroy     (EditAction     *action,
                                         MooTextBuffer  *buffer);

static void     insert_action_undo      (InsertAction   *action,
                                         GtkTextBuffer  *buffer);
static void     insert_action_redo      (InsertAction   *action,
                                         GtkTextBuffer  *buffer);
static gboolean insert_action_merge     (InsertAction   *action,
                                         InsertAction   *what,
                                         MooTextBuffer  *buffer);

static void     delete_action_undo      (DeleteAction   *action,
                                         GtkTextBuffer  *buffer);
static void     delete_action_redo      (DeleteAction   *action,
                                         GtkTextBuffer  *buffer);
static gboolean delete_action_merge     (DeleteAction   *action,
                                         DeleteAction   *what,
                                         MooTextBuffer  *buffer);

static MooUndoActionClass InsertActionClass = {
    (MooUndoActionUndo) insert_action_undo,
    (MooUndoActionRedo) insert_action_redo,
    (MooUndoActionMerge) insert_action_merge,
    (MooUndoActionDestroy) edit_action_destroy
};

static MooUndoActionClass DeleteActionClass = {
    (MooUndoActionUndo) delete_action_undo,
    (MooUndoActionRedo) delete_action_redo,
    (MooUndoActionMerge) delete_action_merge,
    (MooUndoActionDestroy) edit_action_destroy
};


static void
init_undo_actions (void)
{
    INSERT_ACTION_TYPE = moo_undo_action_register (&InsertActionClass);
    DELETE_ACTION_TYPE = moo_undo_action_register (&DeleteActionClass);
}


static EditAction *
action_new (ActionType     type,
            MooTextBuffer *buffer)
{
    EditAction *action;
    gsize size = 0;
    GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER (buffer);

    switch (type)
    {
        case ACTION_INSERT:
            size = sizeof (InsertAction);
            break;
        case ACTION_DELETE:
            size = sizeof (DeleteAction);
            break;
    }

    g_assert (size != 0);
    action = g_malloc0 (size);
    action->interactive = (!buffer->priv->non_interactive && text_buffer->user_action_count) ? TRUE : FALSE;

    if (!gtk_text_buffer_get_modified (text_buffer))
        buffer->priv->modifying_action = action;

    return action;
}


static MooUndoAction *
insert_action_new (GtkTextBuffer    *buffer,
                   GtkTextIter      *pos,
                   const char       *text,
                   int               length)
{
    InsertAction *action;
    EditAction *edit_action;

    if (length < 0)
        length = (int) strlen (text);

    g_return_val_if_fail (length > 0, NULL);

    edit_action = action_new (ACTION_INSERT, MOO_TEXT_BUFFER (buffer));
    action = (InsertAction*) edit_action;

    action->pos = gtk_text_iter_get_offset (pos);
    action->edit.text = g_strndup (text, length);
    action->length = length;
    action->chars = g_utf8_strlen (text, length);

    if (action->chars > 1 || text[0] == '\n')
        edit_action->mergeable = FALSE;
    else
        edit_action->mergeable = TRUE;

    return (MooUndoAction*) action;
}


static MooUndoAction *
delete_action_new (GtkTextBuffer      *buffer,
                   GtkTextIter        *start,
                   GtkTextIter        *end)
{
    GtkTextIter iter;
    DeleteAction *action;
    EditAction *edit_action;
    int start_offset, end_offset;

    start_offset = gtk_text_iter_get_offset (start);
    end_offset = gtk_text_iter_get_offset (end);

    g_return_val_if_fail (start_offset < end_offset, NULL);

    edit_action = action_new (ACTION_DELETE, MOO_TEXT_BUFFER (buffer));
    action = (DeleteAction*) edit_action;

    action->start = start_offset;
    action->end = end_offset;
    action->edit.text = gtk_text_buffer_get_slice (buffer, start, end, TRUE);

    if (edit_action->interactive)
    {
        /* figure out if the user used the Delete or the Backspace key */
        gtk_text_buffer_get_iter_at_mark (buffer, &iter, gtk_text_buffer_get_insert (buffer));

        if (gtk_text_iter_get_offset (&iter) <= action->start)
            action->forward = TRUE;
        else
            action->forward = FALSE;
    }

    if (action->end - action->start > 1 || action->edit.text[0] == '\n')
        edit_action->mergeable = FALSE;
    else
        edit_action->mergeable = TRUE;

    return (MooUndoAction*) action;
}


static void
action_undo_or_redo (EditAction     *action,
                     GtkTextBuffer  *text_buffer,
                     gboolean        was_modified)
{
    MooTextBuffer *buffer = MOO_TEXT_BUFFER (text_buffer);

    if (!was_modified)
    {
        g_return_if_fail (gtk_text_buffer_get_modified (text_buffer));
        g_return_if_fail (buffer->priv->modifying_action == NULL);
        buffer->priv->modifying_action = action;
    }
    else if (buffer->priv->modifying_action == action)
    {
        gtk_text_buffer_set_modified (text_buffer, FALSE);
        buffer->priv->modifying_action = NULL;
    }
}


static void
insert_action_undo (InsertAction   *action,
                    GtkTextBuffer  *buffer)
{
    GtkTextIter start, end;
    gboolean was_modified;

    was_modified = gtk_text_buffer_get_modified (buffer);

    gtk_text_buffer_get_iter_at_offset (buffer, &start, action->pos);
    gtk_text_buffer_get_iter_at_offset (buffer, &end,
                                        action->pos + action->chars);
    gtk_text_buffer_delete (buffer, &start, &end);

    if (ACTION_INTERACTIVE (action))
        MOO_TEXT_BUFFER(buffer)->priv->move_cursor_to = action->pos;

    action_undo_or_redo (EDIT_ACTION (action), buffer, was_modified);
}


static void
delete_action_undo (DeleteAction   *action,
                    GtkTextBuffer  *buffer)
{
    GtkTextIter start;
    gboolean was_modified;

    was_modified = gtk_text_buffer_get_modified (buffer);

    gtk_text_buffer_get_iter_at_offset (buffer, &start, action->start);
    gtk_text_buffer_insert (buffer, &start, action->edit.text, -1);

    if (ACTION_INTERACTIVE (action))
    {
        if (action->forward)
            MOO_TEXT_BUFFER(buffer)->priv->move_cursor_to = action->start;
        else
            MOO_TEXT_BUFFER(buffer)->priv->move_cursor_to = action->end;
    }

    action_undo_or_redo (EDIT_ACTION (action), buffer, was_modified);
}


static void
insert_action_redo (InsertAction   *action,
                    GtkTextBuffer  *buffer)
{
    GtkTextIter start;
    gboolean was_modified;

    was_modified = gtk_text_buffer_get_modified (buffer);

    gtk_text_buffer_get_iter_at_offset (buffer, &start, action->pos);
    gtk_text_buffer_insert (buffer, &start, action->edit.text, action->length);

    if (ACTION_INTERACTIVE (action))
        MOO_TEXT_BUFFER(buffer)->priv->move_cursor_to = action->pos + action->length;

    action_undo_or_redo (EDIT_ACTION (action), buffer, was_modified);
}


static void
delete_action_redo (DeleteAction   *action,
                    GtkTextBuffer  *buffer)
{
    GtkTextIter start, end;
    gboolean was_modified;

    was_modified = gtk_text_buffer_get_modified (buffer);

    gtk_text_buffer_get_iter_at_offset (buffer, &start, action->start);
    gtk_text_buffer_get_iter_at_offset (buffer, &end, action->end);
    gtk_text_buffer_delete (buffer, &start, &end);

    if (ACTION_INTERACTIVE (action))
        MOO_TEXT_BUFFER(buffer)->priv->move_cursor_to = action->start;

    action_undo_or_redo (EDIT_ACTION (action), buffer, was_modified);
}


static void
edit_action_destroy (EditAction    *action,
                     MooTextBuffer *buffer)
{
    if (action)
    {
        if (buffer->priv->modifying_action == action)
            buffer->priv->modifying_action = NULL;
        g_free (action->text);
        g_free (action);
    }
}


static gboolean
action_merge (EditAction     *last_action,
              EditAction     *action,
              MooTextBuffer  *buffer)
{
    if (!last_action->mergeable)
        return FALSE;

    if (!action->mergeable ||
         buffer->priv->modifying_action == action ||
         action->interactive != last_action->interactive)
    {
        last_action->mergeable = FALSE;
        return FALSE;
    }

    return TRUE;
}


#define IS_SPACE_OR_TAB(c__) ((c__) == ' ' || (c__) == '\t')

static gboolean
insert_action_merge (InsertAction   *last_action,
                     InsertAction   *action,
                     MooTextBuffer  *buffer)
{
    char *tmp;

    if (!action_merge (EDIT_ACTION (last_action), EDIT_ACTION (action), buffer))
        return FALSE;

    if (action->pos != (last_action->pos + last_action->chars) ||
        (!IS_SPACE_OR_TAB (action->edit.text[0]) &&
          IS_SPACE_OR_TAB (last_action->edit.text[last_action->length-1])))
    {
        EDIT_ACTION(last_action)->mergeable = FALSE;
        return FALSE;
    }

    tmp = g_strconcat (last_action->edit.text, action->edit.text, NULL);
    g_free (last_action->edit.text);
    last_action->length += action->length;
    last_action->edit.text = tmp;
    last_action->chars += action->chars;

    return TRUE;
}


static gboolean
delete_action_merge (DeleteAction   *last_action,
                     DeleteAction   *action,
                     MooTextBuffer  *buffer)
{
    char *tmp;

    if (!action_merge (EDIT_ACTION (last_action), EDIT_ACTION (action), buffer))
        return FALSE;

    if (last_action->forward != action->forward ||
        (last_action->start != action->start &&
         last_action->start != action->end))
    {
        EDIT_ACTION(last_action)->mergeable = FALSE;
        return FALSE;
    }

    if (last_action->start == action->start)
    {
        char *text_end = g_utf8_offset_to_pointer (last_action->edit.text,
                                                   last_action->end - last_action->start - 1);

        /* Deleted with the delete key */
        if (!IS_SPACE_OR_TAB (action->edit.text[0]) && IS_SPACE_OR_TAB (*text_end))
        {
            EDIT_ACTION(last_action)->mergeable = FALSE;
            return FALSE;
        }

        tmp = g_strconcat (last_action->edit.text, action->edit.text, NULL);
        g_free (last_action->edit.text);
        last_action->end += (action->end - action->start);
        last_action->edit.text = tmp;
    }
    else
    {
        /* Deleted with the backspace key */
        if (!IS_SPACE_OR_TAB (action->edit.text[0]) &&
            IS_SPACE_OR_TAB (last_action->edit.text[0]))
        {
            EDIT_ACTION(last_action)->mergeable = FALSE;
            return FALSE;
        }

        tmp = g_strconcat (action->edit.text, last_action->edit.text, NULL);
        g_free (last_action->edit.text);
        last_action->start = action->start;
        last_action->edit.text = tmp;
    }

    return TRUE;
}


void
moo_text_buffer_add_line_mark (MooTextBuffer *buffer,
                               MooLineMark   *mark,
                               int            line_no)
{
    Line *line;

    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));
    g_return_if_fail (MOO_IS_LINE_MARK (mark));
    g_return_if_fail (!moo_line_mark_get_buffer (mark));

    g_object_ref (mark);
    g_object_freeze_notify (G_OBJECT (mark));

    if (line_no < 0 || line_no >= gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (buffer)))
        line_no = 0;

    /* XXX ??? */
    line = _moo_line_buffer_get_line (buffer->priv->line_buf, line_no);
    g_return_if_fail (line != NULL);

    _moo_line_mark_set_buffer (mark, buffer, buffer->priv->line_buf);
    _moo_line_buffer_add_mark (buffer->priv->line_buf, mark, line_no);

    g_signal_emit (buffer, signals[LINE_MARK_ADDED], 0, mark);
    g_object_thaw_notify (G_OBJECT (mark));
}


void
moo_text_buffer_delete_line_mark (MooTextBuffer *buffer,
                                  MooLineMark   *mark)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));
    g_return_if_fail (MOO_IS_LINE_MARK (mark));
    g_return_if_fail (moo_line_mark_get_buffer (mark) == buffer);

    _moo_line_buffer_remove_mark (buffer->priv->line_buf, mark);
    line_mark_deleted (buffer, mark);
    g_object_unref (mark);
}


static void
line_mark_moved (MooTextBuffer      *buffer,
                 MooLineMark        *mark)
{
    g_signal_emit (buffer, signals[LINE_MARK_MOVED], 0, mark);
}


static void
line_mark_deleted (MooTextBuffer      *buffer,
                   MooLineMark        *mark)
{
    _moo_line_mark_set_buffer (mark, NULL, NULL);
    _moo_line_mark_deleted (mark);
    g_signal_emit (buffer, signals[LINE_MARK_DELETED], 0, mark);
}


void
moo_text_buffer_move_line_mark (MooTextBuffer *buffer,
                                MooLineMark   *mark,
                                int            line)
{
    int old_line;

    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));
    g_return_if_fail (MOO_IS_LINE_MARK (mark));
    g_return_if_fail (moo_line_mark_get_buffer (mark) == buffer);

    old_line = moo_line_mark_get_line (mark);

    if (old_line == line)
        return;

    _moo_line_buffer_move_mark (buffer->priv->line_buf, mark, line);
    line_mark_moved (buffer, mark);
}


GSList *
moo_text_buffer_get_line_marks_in_range (MooTextBuffer *buffer,
                                         int            first_line,
                                         int            last_line)
{
    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), NULL);
    g_return_val_if_fail (first_line >= 0, NULL);
    return _moo_line_buffer_get_marks_in_range (buffer->priv->line_buf, first_line, last_line);
}


GSList *
moo_text_buffer_get_line_marks_at_line (MooTextBuffer *buffer,
                                        int            line)
{
    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), NULL);
    g_return_val_if_fail (line >= 0, NULL);
    return _moo_line_buffer_get_marks_in_range (buffer->priv->line_buf, line, line);
}


MooFold *
moo_text_buffer_add_fold (MooTextBuffer *buffer,
                          int            first_line,
                          int            last_line)
{
    MooFold *fold;

    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), NULL);
    g_return_val_if_fail (first_line >= 0, NULL);
    g_return_val_if_fail (last_line > first_line, NULL);
    g_return_val_if_fail (last_line < gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (buffer)), NULL);

    fold = _moo_fold_tree_add (buffer->priv->fold_tree, first_line, last_line);
    g_return_val_if_fail (fold != NULL, NULL);

    g_object_ref (fold);
    g_signal_emit (buffer, signals[FOLD_ADDED], 0, fold);

    return fold;
}


static void
fold_deleted (MooTextBuffer *buffer,
              MooFold       *fold)
{
    g_signal_emit (buffer, signals[FOLD_DELETED], 0, fold);
}

void
moo_text_buffer_delete_fold (MooTextBuffer *buffer,
                             MooFold       *fold)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));
    g_return_if_fail (MOO_IS_FOLD (fold));
    g_return_if_fail (!_moo_fold_is_deleted (fold));

    _moo_fold_tree_remove (buffer->priv->fold_tree, fold);
    fold_deleted (buffer, fold);
    g_object_unref (fold);
}


#if 0
GSList *
moo_text_buffer_get_folds_in_range (MooTextBuffer *buffer,
                                    int            first_line,
                                    int            last_line)
{
    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), NULL);
    g_return_val_if_fail (first_line >= 0, NULL);

    if (last_line < 0)
        last_line = gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (buffer)) - 1;

    g_return_val_if_fail (last_line >= first_line, NULL);
    g_return_val_if_fail (last_line < gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (buffer)), NULL);

    return _moo_fold_tree_get (buffer->priv->fold_tree, first_line, last_line);
}
#endif


MooFold *
moo_text_buffer_get_fold_at_line (MooTextBuffer *buffer,
                                  int            line)
{
    GSList *marks, *l;
    MooFold *fold;

    g_return_val_if_fail (MOO_IS_TEXT_BUFFER (buffer), NULL);
    g_return_val_if_fail (line >= 0, NULL);
    g_return_val_if_fail (line < gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (buffer)), NULL);

    fold = NULL;
    marks = moo_text_buffer_get_line_marks_at_line (buffer, line);

    for (l = marks; l != NULL; l = l->next)
    {
        fold = _moo_line_mark_get_fold (l->data);
        if (fold && fold->start == l->data)
            break;
        fold = NULL;
    }

    g_slist_free (marks);
    return fold;
}


void
moo_text_buffer_toggle_fold (MooTextBuffer *buffer,
                             MooFold       *fold)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));
    g_return_if_fail (MOO_IS_FOLD (fold));
    g_return_if_fail (!_moo_fold_is_deleted (fold));

    if (fold->collapsed)
        _moo_fold_tree_expand (buffer->priv->fold_tree, fold);
    else
        _moo_fold_tree_collapse (buffer->priv->fold_tree, fold);

    g_signal_emit (buffer, signals[FOLD_TOGGLED], 0, fold);
}

void
moo_text_buffer_toggle_folds (MooTextBuffer *buffer)
{
    g_return_if_fail (MOO_IS_TEXT_BUFFER (buffer));
    if (_moo_fold_tree_toggle (buffer->priv->fold_tree))
        g_signal_emit (buffer, signals[FOLD_TOGGLED], 0, NULL);
}
