/*
 *   mootextview.c
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
 * class:MooTextView: (parent GtkTextView) (constructable): text view object
 **/

#include "mooedit/mooedit-accels.h"
#include "mooedit/mootextview-private.h"
#include "mooedit/mootextview.h"
#include "mooedit/mootextbuffer.h"
#include "mooedit/mootextfind.h"
#include "mooedit/mootext-private.h"
#include "mooedit/mooeditprefs.h"
#include "mooedit/moolangmgr.h"
#include "mooedit/mooeditwindow.h"
#include "mooedit/mooedit.h"
#include "marshals.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooundo.h"
#include "mooutils/mooeditops.h"
#include "mooutils/mooentry.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooatom.h"
#include "mooutils/mootype-macros.h"
#include "mooutils/moocompat.h"
#include "mooutils/mooutils-gobject.h"
#include "mooedit/mooquicksearch-gxml.h"
#include <gtk/gtk.h>
#include <mooglib/moo-glib.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

#define LIGHT_BLUE "#EEF6FF"
#define BOOL_CMP(b1,b2) ((b1 && b2) || (!b1 && !b2))
#define UPDATE_PRIORITY (GTK_TEXT_VIEW_PRIORITY_VALIDATE - 5)

#define MIN_LINE_MARK_WIDTH 6
#define DEFAULT_EXPANDER_SIZE 12
#define EXPANDER_PAD 1

static GtkTextWindowType window_types[4] = {
    GTK_TEXT_WINDOW_LEFT,
    GTK_TEXT_WINDOW_RIGHT,
    GTK_TEXT_WINDOW_TOP,
    GTK_TEXT_WINDOW_BOTTOM
};

static const GtkTargetEntry text_view_target_table[] = {
    { (char*) "GTK_TEXT_BUFFER_CONTENTS", GTK_TARGET_SAME_APP, 0 }
};

static void     undo_ops_iface_init         (MooUndoOpsIface    *iface);
static GObject *moo_text_view_constructor   (GType               type,
                                             guint               n_construct_properties,
                                             GObjectConstructParam *construct_param);
static void     moo_text_view_dispose       (GObject            *object);
static void     moo_text_view_finalize      (GObject            *object);

static void     moo_text_view_set_property  (GObject            *object,
                                             guint               prop_id,
                                             const GValue       *value,
                                             GParamSpec         *pspec);
static void     moo_text_view_get_property  (GObject            *object,
                                             guint               prop_id,
                                             GValue             *value,
                                             GParamSpec         *pspec);

static void     moo_text_view_realize       (GtkWidget          *widget);
static void     moo_text_view_unrealize     (GtkWidget          *widget);
static gboolean moo_text_view_expose        (GtkWidget          *widget,
                                             GdkEventExpose     *event);
static void     moo_text_view_style_set     (GtkWidget          *widget,
                                             GtkStyle           *previous_style);
static void     moo_text_view_size_request  (GtkWidget          *widget,
                                             GtkRequisition     *requisition);
static void     moo_text_view_size_allocate (GtkWidget          *widget,
                                             GtkAllocation      *allocation);

static void     moo_text_view_remove        (GtkContainer       *container,
                                             GtkWidget          *child);

static void     moo_text_view_copy_clipboard (GtkTextView       *text_view);
static void     moo_text_view_cut_clipboard (GtkTextView        *text_view);
static void     moo_text_view_paste_clipboard (GtkTextView      *text_view);
static void     moo_text_view_populate_popup(GtkTextView        *text_view,
                                             GtkMenu            *menu);

static void     moo_text_view_apply_style_scheme (MooTextView   *view,
                                             MooTextStyleScheme *scheme);

static void     invalidate_gcs              (MooTextView        *view);
static void     update_gcs                  (MooTextView        *view);
static void     update_tab_width            (MooTextView        *view);
static void     invalidate_right_margin     (MooTextView        *view);
static void     update_right_margin         (MooTextView        *view);

static GtkTextBuffer *get_buffer            (MooTextView        *view);
static MooTextBuffer *get_moo_buffer        (MooTextView        *view);
static GtkTextMark *get_insert              (MooTextView        *view);

static void     cursor_moved                (MooTextView        *view,
                                             GtkTextIter        *where);
static void     proxy_prop_notify           (MooTextView        *view,
                                             GParamSpec         *pspec);
static void     proxy_notify_can_undo_redo  (MooTextView        *view,
                                             GParamSpec         *pspec);

static void     find_word_at_cursor         (MooTextView        *view,
                                             gboolean            forward);
static void     find_interactive            (MooTextView        *view);
static void     replace_interactive         (MooTextView        *view);
static void     find_next_interactive       (MooTextView        *view);
static void     find_prev_interactive       (MooTextView        *view);
static void     goto_line_interactive       (MooTextView        *view);
static gboolean start_quick_search          (MooTextView        *view);
static void     moo_text_view_stop_quick_search (MooTextView    *view);

static void     insert_text_cb              (MooTextView        *view,
                                             GtkTextIter        *iter,
                                             gchar              *text,
                                             gint                len);
static gboolean moo_text_view_char_inserted (MooTextView        *view,
                                             GtkTextIter        *where,
                                             const char         *character);
static void     moo_text_view_delete_selection (MooTextView     *view);

static MooTextCursor moo_text_view_get_text_cursor
                                            (MooTextView        *view,
                                             int                 x,
                                             int                 y);

static void     set_manage_clipboard        (MooTextView        *view,
                                             gboolean            manage);
static void     selection_changed           (MooTextView        *view,
                                             MooTextBuffer      *buffer);
static void     highlight_updated           (GtkTextView        *view,
                                             const GtkTextIter  *start,
                                             const GtkTextIter  *end);

static void     set_draw_whitespace         (MooTextView        *view,
                                             MooDrawWsFlags      flags);

static void     buffer_changed              (MooTextView        *view);
static void     update_left_margin          (MooTextView        *view);
static void     draw_left_margin            (MooTextView        *view,
                                             GdkEventExpose     *event);
static void     draw_marks_background       (MooTextView        *view,
                                             GdkEventExpose     *event);
static gboolean update_n_lines_idle         (MooTextView        *view);

static void     add_line_mark               (MooTextView        *view,
                                             MooLineMark        *mark);
static void     remove_line_mark            (MooTextView        *view,
                                             MooLineMark        *mark);
static void     line_mark_added             (MooTextView        *view,
                                             MooLineMark        *mark);
static void     line_mark_deleted           (MooTextView        *view,
                                             MooLineMark        *mark);
static void     line_mark_changed           (MooTextView        *view,
                                             MooLineMark        *mark);
static void     line_mark_moved             (MooTextView        *view,
                                             MooLineMark        *mark);
static void     set_show_line_marks         (MooTextView        *view,
                                             gboolean            show);
static void     fold_added                  (MooTextView        *view,
                                             MooFold            *fold);
static void     fold_deleted                (MooTextView        *view);
static void     fold_toggled                (MooTextView        *view,
                                             MooFold            *fold);
static void     set_enable_folding          (MooTextView        *view,
                                             gboolean            show);

static gboolean text_iter_forward_visible_line (MooTextView     *view,
                                             GtkTextIter        *iter,
                                             int                *line);
static int      get_border_window_size      (GtkTextView        *text_view,
                                             GtkTextWindowType   type);


enum {
    DELETE_SELECTION,
    FIND_INTERACTIVE,
    FIND_WORD_AT_CURSOR,
    FIND_NEXT_INTERACTIVE,
    FIND_PREV_INTERACTIVE,
    REPLACE_INTERACTIVE,
    GOTO_LINE_INTERACTIVE,
    CURSOR_MOVED,
    CHAR_INSERTED,
    LINE_MARK_CLICKED,
    START_QUICK_SEARCH,
    UNDO,
    REDO,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];
gpointer _moo_text_view_parent_class = NULL;

enum {
    PROP_0,
    PROP_BUFFER,

    PROP_INDENTER,
    PROP_AUTO_INDENT,
    PROP_BACKSPACE_INDENTS,
    PROP_TAB_INDENTS,

    PROP_RIGHT_MARGIN_OFFSET,
    PROP_DRAW_RIGHT_MARGIN,
    PROP_RIGHT_MARGIN_COLOR,
    PROP_HIGHLIGHT_CURRENT_LINE,
    PROP_HIGHLIGHT_CURRENT_LINE_UNFOCUSED,
    PROP_HIGHLIGHT_MATCHING_BRACKETS,
    PROP_HIGHLIGHT_MISMATCHING_BRACKETS,
    PROP_CURRENT_LINE_COLOR,
    PROP_TAB_WIDTH,
    PROP_DRAW_WHITESPACE,
    PROP_HAS_TEXT,
    PROP_HAS_SELECTION,
    PROP_CAN_UNDO,
    PROP_CAN_REDO,
    PROP_MANAGE_CLIPBOARD,
    PROP_SMART_HOME_END,
    PROP_ENABLE_HIGHLIGHT,
    PROP_SHOW_LINE_NUMBERS,
    PROP_SHOW_LINE_MARKS,
    PROP_ENABLE_FOLDING,
    PROP_ENABLE_QUICK_SEARCH,
    PROP_QUICK_SEARCH_FLAGS
};


/* MOO_TYPE_TEXT_VIEW */
G_DEFINE_TYPE_WITH_CODE (MooTextView, moo_text_view, GTK_TYPE_TEXT_VIEW,
                         G_IMPLEMENT_INTERFACE (MOO_TYPE_UNDO_OPS, undo_ops_iface_init))


static void moo_text_view_class_init (MooTextViewClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
    GtkTextViewClass *text_view_class = GTK_TEXT_VIEW_CLASS (klass);
    GtkBindingSet *binding_set;

    _moo_text_view_parent_class = moo_text_view_parent_class;
    g_type_class_unref (g_type_class_ref (MOO_TYPE_INDENTER));
    g_type_class_add_private (klass, sizeof (MooTextViewPrivate));

    gobject_class->set_property = moo_text_view_set_property;
    gobject_class->get_property = moo_text_view_get_property;
    gobject_class->constructor = moo_text_view_constructor;
    gobject_class->finalize = moo_text_view_finalize;
    gobject_class->dispose = moo_text_view_dispose;

    widget_class->button_press_event = _moo_text_view_button_press_event;
    widget_class->button_release_event = _moo_text_view_button_release_event;
    widget_class->motion_notify_event = _moo_text_view_motion_event;

    widget_class->key_press_event = _moo_text_view_key_press_event;
    widget_class->realize = moo_text_view_realize;
    widget_class->unrealize = moo_text_view_unrealize;
    widget_class->expose_event = moo_text_view_expose;
    widget_class->style_set = moo_text_view_style_set;
    widget_class->size_request = moo_text_view_size_request;
    widget_class->size_allocate = moo_text_view_size_allocate;

    container_class->remove = moo_text_view_remove;

    text_view_class->move_cursor = _moo_text_view_move_cursor;
    text_view_class->delete_from_cursor = _moo_text_view_delete_from_cursor;
    text_view_class->copy_clipboard = moo_text_view_copy_clipboard;
    text_view_class->cut_clipboard = moo_text_view_cut_clipboard;
    text_view_class->paste_clipboard = moo_text_view_paste_clipboard;
    text_view_class->populate_popup = moo_text_view_populate_popup;

    klass->extend_selection = _moo_text_view_extend_selection;
    klass->find_word_at_cursor = find_word_at_cursor;
    klass->find_interactive = find_interactive;
    klass->find_next_interactive = find_next_interactive;
    klass->find_prev_interactive = find_prev_interactive;
    klass->replace_interactive = replace_interactive;
    klass->goto_line_interactive = goto_line_interactive;
    klass->char_inserted = moo_text_view_char_inserted;
    klass->apply_style_scheme = moo_text_view_apply_style_scheme;
    klass->get_text_cursor = moo_text_view_get_text_cursor;

    g_object_class_install_property (gobject_class,
                                     PROP_BUFFER,
                                     g_param_spec_object ("buffer",
                                             "buffer",
                                             "buffer",
                                             MOO_TYPE_TEXT_BUFFER,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property (gobject_class,
                                     PROP_RIGHT_MARGIN_OFFSET,
                                     g_param_spec_uint ("right-margin-offset",
                                             "right-margin-offset",
                                             "right-margin-offset",
                                             1, G_MAXUINT, 80,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_DRAW_RIGHT_MARGIN,
                                     g_param_spec_boolean ("draw-right-margin",
                                             "draw-right-margin",
                                             "draw-right-margin",
                                             FALSE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_RIGHT_MARGIN_COLOR,
                                     g_param_spec_string ("right-margin-color",
                                             "right-margin-color",
                                             "right-margin-color",
                                             LIGHT_BLUE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_HIGHLIGHT_CURRENT_LINE,
                                     g_param_spec_boolean ("highlight-current-line",
                                             "highlight-current-line",
                                             "highlight-current-line",
                                             FALSE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_HIGHLIGHT_CURRENT_LINE_UNFOCUSED,
                                     g_param_spec_boolean ("highlight-current-line-unfocused",
                                             "highlight-current-line-unfocused",
                                             "highlight-current-line-unfocused",
                                             FALSE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

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
                                     PROP_CURRENT_LINE_COLOR,
                                     g_param_spec_string ("current-line-color",
                                             "current-line-color",
                                             "current-line-color",
                                             LIGHT_BLUE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_TAB_WIDTH,
                                     g_param_spec_uint ("tab-width",
                                             "tab-width",
                                             "tab-width",
                                             1, G_MAXUINT, 8,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_DRAW_WHITESPACE,
                                     g_param_spec_flags ("draw-whitespace",
                                             "draw-whitespace",
                                             "draw-whitespace",
                                             MOO_TYPE_DRAW_WS_FLAGS,
                                             0,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_INDENTER,
                                     g_param_spec_object ("indenter",
                                             "indenter",
                                             "indenter",
                                             MOO_TYPE_INDENTER,
                                             (GParamFlags) G_PARAM_READWRITE));

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

    g_object_class_install_property (gobject_class,
                                     PROP_MANAGE_CLIPBOARD,
                                     g_param_spec_boolean ("manage-clipboard",
                                             "manage-clipboard",
                                             "manage-clipboard",
                                             TRUE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_SMART_HOME_END,
                                     g_param_spec_boolean ("smart-home-end",
                                             "smart-home-end",
                                             "smart-home-end",
                                             TRUE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_ENABLE_HIGHLIGHT,
                                     g_param_spec_boolean ("enable-highlight",
                                             "enable-highlight",
                                             "enable-highlight",
                                             TRUE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_SHOW_LINE_NUMBERS,
                                     g_param_spec_boolean ("show-line-numbers",
                                             "show-line-numbers",
                                             "show-line-numbers",
                                             FALSE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_SHOW_LINE_MARKS,
                                     g_param_spec_boolean ("show-line-marks",
                                             "show-line-marks",
                                             "show-line-marks",
                                             FALSE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_ENABLE_FOLDING,
                                     g_param_spec_boolean ("enable-folding",
                                             "enable-folding",
                                             "enable-folding",
                                             FALSE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_ENABLE_QUICK_SEARCH,
                                     g_param_spec_boolean ("enable-quick-search",
                                             "enable-quick-search",
                                             "enable-quick-search",
                                             TRUE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_QUICK_SEARCH_FLAGS,
                                     g_param_spec_flags ("quick-search-flags",
                                             "quick-search-flags",
                                             "quick-search-flags",
                                             MOO_TYPE_TEXT_SEARCH_FLAGS,
                                             MOO_TEXT_SEARCH_CASELESS,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_AUTO_INDENT,
                                     g_param_spec_boolean ("auto-indent",
                                             "auto-indent", "auto-indent",
                                             TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_BACKSPACE_INDENTS,
                                     g_param_spec_boolean ("backspace-indents",
                                             "backspace-indents", "backspace-indents",
                                             FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_TAB_INDENTS,
                                     g_param_spec_boolean ("tab-indents",
                                             "tab-indents", "tab-indents",
                                             FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    gtk_widget_class_install_style_property (widget_class,
                                             g_param_spec_int ("expander-size",
                                                     "Expander Size",
                                                     "Size of the expander arrow",
                                                     0,
                                                     G_MAXINT,
                                                     DEFAULT_EXPANDER_SIZE,
                                                     G_PARAM_READABLE));

    signals[UNDO] =
            _moo_signal_new_cb ("undo",
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (moo_text_view_undo),
                                g_signal_accumulator_true_handled, NULL,
                                _moo_marshal_BOOLEAN__VOID,
                                G_TYPE_BOOLEAN, 0);

    signals[REDO] =
            _moo_signal_new_cb ("redo",
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (moo_text_view_redo),
                                g_signal_accumulator_true_handled, NULL,
                                _moo_marshal_BOOLEAN__VOID,
                                G_TYPE_BOOLEAN, 0);

    signals[DELETE_SELECTION] =
            _moo_signal_new_cb ("delete-selection",
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST,
                                G_CALLBACK (moo_text_view_delete_selection),
                                NULL, NULL,
                                _moo_marshal_VOID__VOID,
                                G_TYPE_NONE, 0);

    signals[FIND_WORD_AT_CURSOR] =
            g_signal_new ("find-word-at-cursor",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                          G_STRUCT_OFFSET (MooTextViewClass, find_word_at_cursor),
                          NULL, NULL,
                          _moo_marshal_VOID__BOOLEAN,
                          G_TYPE_NONE, 1,
                          G_TYPE_BOOLEAN);

    signals[FIND_INTERACTIVE] =
            g_signal_new ("find-interactive",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                          G_STRUCT_OFFSET (MooTextViewClass, find_interactive),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[FIND_NEXT_INTERACTIVE] =
            g_signal_new ("find-next-interactive",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                          G_STRUCT_OFFSET (MooTextViewClass, find_next_interactive),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[FIND_PREV_INTERACTIVE] =
            g_signal_new ("find-prev-interactive",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                          G_STRUCT_OFFSET (MooTextViewClass, find_prev_interactive),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[REPLACE_INTERACTIVE] =
            g_signal_new ("replace-interactive",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                          G_STRUCT_OFFSET (MooTextViewClass, replace_interactive),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[GOTO_LINE_INTERACTIVE] =
            g_signal_new ("goto-line-interactive",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                          G_STRUCT_OFFSET (MooTextViewClass, goto_line_interactive),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[CHAR_INSERTED] =
            g_signal_new ("char-inserted",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTextViewClass, char_inserted),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOLEAN__BOXED_STRING,
                          G_TYPE_BOOLEAN, 2,
                          GTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
                          G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE);

    signals[CURSOR_MOVED] =
            g_signal_new ("cursor-moved",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          0, NULL, NULL,
                          _moo_marshal_VOID__BOXED,
                          G_TYPE_NONE, 1,
                          GTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE);

    signals[LINE_MARK_CLICKED] =
            g_signal_new ("line-mark-clicked",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooTextViewClass, line_mark_clicked),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOLEAN__INT,
                          G_TYPE_BOOLEAN, 1,
                          G_TYPE_INT);

    signals[START_QUICK_SEARCH] =
            _moo_signal_new_cb ("start-quick-search",
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (start_quick_search),
                                NULL, NULL,
                                _moo_marshal_BOOL__VOID,
                                G_TYPE_BOOLEAN, 0);

    binding_set = gtk_binding_set_by_class (klass);
    gtk_binding_entry_add_signal (binding_set, GDK_z, MOO_ACCEL_CTRL_MASK,
                                  "undo", 0);
    gtk_binding_entry_add_signal (binding_set, GDK_z, MOO_ACCEL_CTRL_MASK | GDK_SHIFT_MASK,
                                  "redo", 0);
}


static void
moo_text_view_init (MooTextView *view)
{
    view->priv = G_TYPE_INSTANCE_GET_PRIVATE (view, MOO_TYPE_TEXT_VIEW, MooTextViewPrivate);

    view->priv->constructed = FALSE;
    view->priv->buffer_type = MOO_TYPE_TEXT_BUFFER;
    view->priv->buffer = NULL;

    view->priv->colors[MOO_TEXT_VIEW_COLOR_CURRENT_LINE] = g_strdup (LIGHT_BLUE);
    view->priv->colors[MOO_TEXT_VIEW_COLOR_RIGHT_MARGIN] = g_strdup (LIGHT_BLUE);
    view->priv->right_margin_offset = 80;

    view->priv->word_chars = NULL;
    view->priv->n_word_chars = 0;

    view->priv->dnd.button = GDK_BUTTON_RELEASE;
    view->priv->dnd.type = MOO_TEXT_VIEW_DRAG_NONE;
    view->priv->dnd.start_x = -1;
    view->priv->dnd.start_y = -1;

    view->priv->last_search_stamp = -1;

    view->priv->tab_indents = FALSE;
    view->priv->backspace_indents = FALSE;
    view->priv->enter_indents = TRUE;
    view->priv->ctrl_up_down_scrolls = TRUE;
    view->priv->ctrl_page_up_down_scrolls = TRUE;
    view->priv->smart_home_end = TRUE;

    view->priv->highlight_matching_brackets = TRUE;
    view->priv->highlight_mismatching_brackets = FALSE;
    view->priv->tab_width = 8;

    view->priv->qs.flags = MOO_TEXT_SEARCH_CASELESS;

    view->priv->lm.show_icons = FALSE;
    view->priv->lm.show_numbers = FALSE;
    view->priv->lm.numbers_font = NULL;
    view->priv->lm.show_folds = FALSE;
    view->priv->n_lines = 1;

    gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 2);
    gtk_text_view_set_right_margin (GTK_TEXT_VIEW (view), 2);
}


void
moo_text_view_set_buffer_type (MooTextView *view,
                               GType        type)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    g_return_if_fail (g_type_is_a (type, MOO_TYPE_TEXT_BUFFER));

    if (!g_type_is_a (view->priv->buffer_type, type))
        view->priv->buffer_type = type;
}

static void
connect_buffer (MooTextView *view)
{
    MooUndoStack *undo_stack;
    GtkTextBuffer *buffer = view->priv->buffer;

    g_return_if_fail (buffer != NULL);

    g_signal_connect_swapped (buffer, "changed",
                              G_CALLBACK (buffer_changed), view);
    g_signal_connect_swapped (buffer, "cursor_moved",
                              G_CALLBACK (cursor_moved), view);
    g_signal_connect_swapped (buffer, "selection-changed",
                              G_CALLBACK (selection_changed), view);
    g_signal_connect_swapped (buffer, "highlight-updated",
                              G_CALLBACK (highlight_updated), view);
    g_signal_connect_swapped (buffer, "notify::has-selection",
                              G_CALLBACK (proxy_prop_notify), view);
    g_signal_connect_swapped (buffer, "notify::has-text",
                              G_CALLBACK (proxy_prop_notify), view);

    undo_stack = _moo_text_buffer_get_undo_stack (MOO_TEXT_BUFFER (buffer));
    g_signal_connect_swapped (undo_stack, "notify::can-undo",
                              G_CALLBACK (proxy_notify_can_undo_redo), view);
    g_signal_connect_swapped (undo_stack, "notify::can-redo",
                              G_CALLBACK (proxy_notify_can_undo_redo), view);

    g_signal_connect_data (buffer, "insert-text",
                           G_CALLBACK (insert_text_cb), view,
                           NULL, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

    g_signal_connect_swapped (buffer, "line-mark-added",
                              G_CALLBACK (line_mark_added), view);
    g_signal_connect_swapped (buffer, "line-mark-moved",
                              G_CALLBACK (line_mark_moved), view);
    g_signal_connect_swapped (buffer, "line-mark-deleted",
                              G_CALLBACK (line_mark_deleted), view);
    g_signal_connect_swapped (buffer, "fold-added",
                              G_CALLBACK (fold_added), view);
    g_signal_connect_swapped (buffer, "fold-deleted",
                              G_CALLBACK (fold_deleted), view);
    g_signal_connect_swapped (buffer, "fold-toggled",
                              G_CALLBACK (fold_toggled), view);
}

static void
disconnect_buffer (MooTextView *view)
{
    MooUndoStack *undo_stack;

    if (!view->priv->buffer)
        return;

    g_signal_handlers_disconnect_matched (view->priv->buffer,
                                          G_SIGNAL_MATCH_DATA,
                                          0, 0, NULL, NULL,
                                          view);

    undo_stack = _moo_text_buffer_get_undo_stack (MOO_TEXT_BUFFER (view->priv->buffer));

    g_signal_handlers_disconnect_matched (undo_stack,
                                          G_SIGNAL_MATCH_DATA,
                                          0, 0, NULL, NULL,
                                          view);
}

static GObject*
moo_text_view_constructor (GType                  type,
                           guint                  n_construct_properties,
                           GObjectConstructParam *construct_param)
{
    GtkTargetList *target_list;
    GObject *object;
    MooTextView *view;

    object = G_OBJECT_CLASS (moo_text_view_parent_class)->constructor (
        type, n_construct_properties, construct_param);

    view = MOO_TEXT_VIEW (object);

    if (!view->priv->buffer)
    {
        view->priv->buffer = GTK_TEXT_BUFFER (g_object_new (view->priv->buffer_type, (const char*) NULL));
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (view), view->priv->buffer);
    }

    g_assert (view->priv->buffer == gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));

    view->priv->constructed = TRUE;

    g_object_set (view->priv->buffer,
                  "highlight-matching-brackets", view->priv->highlight_matching_brackets,
                  "highlight-mismatching-brackets", view->priv->highlight_mismatching_brackets,
                  NULL);

    connect_buffer (view);

    target_list = gtk_target_list_new (text_view_target_table, G_N_ELEMENTS (text_view_target_table));
    gtk_target_list_add_text_targets (target_list, 0);
    gtk_drag_dest_set_target_list (GTK_WIDGET (view), target_list);
    gtk_target_list_unref (target_list);

    return object;
}


static void
moo_text_view_dispose (GObject *object)
{
    MooTextView *view = MOO_TEXT_VIEW (object);

    disconnect_buffer (view);

    if (view->priv->buffer)
    {
        g_object_unref (view->priv->buffer);
        view->priv->buffer = NULL;
    }

    if (view->priv->clipboard)
    {
        view->priv->clipboard->view = NULL;
        view->priv->clipboard = NULL;
    }

    if (view->priv->indenter)
    {
        g_object_unref (view->priv->indenter);
        view->priv->indenter = NULL;
    }

    while (view->priv->line_marks)
        remove_line_mark (view, view->priv->line_marks->data);

    if (view->priv->style_scheme)
    {
        g_object_unref (view->priv->style_scheme);
        view->priv->style_scheme = NULL;
    }

    if (view->priv->move_cursor_idle)
    {
        g_source_remove (view->priv->move_cursor_idle);
        view->priv->move_cursor_idle = 0;
    }

    G_OBJECT_CLASS (moo_text_view_parent_class)->dispose (object);
}

static void
moo_text_view_finalize (GObject *object)
{
    int i;
    MooTextView *view = MOO_TEXT_VIEW (object);

    for (i = 0; i < MOO_TEXT_VIEW_N_COLORS; ++i)
        g_free (view->priv->colors[i]);

    g_free (view->priv->char_inserted);

    G_OBJECT_CLASS (moo_text_view_parent_class)->finalize (object);
}


static void
moo_text_view_delete_selection (MooTextView *view)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    gtk_text_buffer_delete_selection (get_buffer (view), TRUE, TRUE);
}


static void
moo_text_view_message (MooTextView *view,
                       const char  *message)
{
    GtkWidget *toplevel;

    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    g_return_if_fail (message != NULL);

    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));

    if (MOO_IS_EDIT_WINDOW (toplevel))
        moo_window_message (MOO_WINDOW (toplevel), message);
}


static void
msg_to_statusbar (const char *message,
                  gpointer    data)
{
    moo_text_view_message (data, message);
}


static void
find_word_at_cursor (MooTextView *view,
                     gboolean     forward)
{
    moo_text_view_run_find_current_word (GTK_TEXT_VIEW (view), forward,
                                         msg_to_statusbar, view);
}

static void
find_interactive (MooTextView *view)
{
    moo_text_view_run_find (GTK_TEXT_VIEW (view),
                            msg_to_statusbar, view);
}

static void
replace_interactive (MooTextView *view)
{
    moo_text_view_run_replace (GTK_TEXT_VIEW (view),
                               msg_to_statusbar, view);
}

static void
find_next_interactive (MooTextView *view)
{
    moo_text_view_run_find_next (GTK_TEXT_VIEW (view),
                                 msg_to_statusbar, view);
}

static void
find_prev_interactive (MooTextView *view)
{
    moo_text_view_run_find_prev (GTK_TEXT_VIEW (view),
                                 msg_to_statusbar, view);
}

static void
goto_line_interactive (MooTextView *view)
{
    moo_text_view_run_goto_line (GTK_TEXT_VIEW (view));
}


/**
 * moo_text_view_set_font_from_string: (moo.private 1)
 *
 * @view:
 * @font: (type const-utf8)
 **/
void
moo_text_view_set_font_from_string (MooTextView *view,
                                    const char  *font)
{
    PangoFontDescription *font_desc = NULL;

    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    if (font)
        font_desc = pango_font_description_from_string (font);

    gtk_widget_modify_font (GTK_WIDGET (view), font_desc);

    if (font_desc)
        pango_font_description_free (font_desc);
}


static MooUndoStack *
get_undo_stack (MooTextView *view)
{
    return _moo_text_buffer_get_undo_stack (get_moo_buffer (view));
}


gboolean
moo_text_view_can_redo (MooTextView *view)
{
    g_return_val_if_fail (MOO_IS_TEXT_VIEW (view), FALSE);
    return moo_undo_stack_can_redo (get_undo_stack (view));
}


gboolean
moo_text_view_can_undo (MooTextView *view)
{
    g_return_val_if_fail (MOO_IS_TEXT_VIEW (view), FALSE);
    return moo_undo_stack_can_undo (get_undo_stack (view));
}


gboolean
moo_text_view_redo (MooTextView    *view)
{
    g_return_val_if_fail (MOO_IS_TEXT_VIEW (view), FALSE);

    if (moo_undo_stack_can_redo (get_undo_stack (view)) &&
        gtk_text_view_get_editable (GTK_TEXT_VIEW (view)))
    {
        moo_text_buffer_freeze (get_moo_buffer (view));
        moo_undo_stack_redo (get_undo_stack (view));
        gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
                                      get_insert (view),
                                      0, FALSE, 0, 0);
        moo_text_buffer_thaw (get_moo_buffer (view));
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


gboolean
moo_text_view_undo (MooTextView    *view)
{
    g_return_val_if_fail (MOO_IS_TEXT_VIEW (view), FALSE);

    if (moo_undo_stack_can_undo (get_undo_stack (view)) &&
        gtk_text_view_get_editable (GTK_TEXT_VIEW (view)))
    {
        moo_text_buffer_freeze (get_moo_buffer (view));
        moo_undo_stack_undo (get_undo_stack (view));
        gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
                                      get_insert (view),
                                      0, FALSE, 0, 0);
        moo_text_buffer_thaw (get_moo_buffer (view));
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


static MooTextCursor
moo_text_view_get_text_cursor (G_GNUC_UNUSED MooTextView *view,
                               G_GNUC_UNUSED int          x,
                               G_GNUC_UNUSED int          y)
{
    return MOO_TEXT_CURSOR_TEXT;
}


void
moo_text_view_select_all (MooTextView *view)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    g_signal_emit_by_name (view, "select-all", TRUE);
}


static void
moo_text_view_set_property (GObject        *object,
                            guint           prop_id,
                            const GValue   *value,
                            GParamSpec     *pspec)
{
    MooTextView *view = MOO_TEXT_VIEW (object);
    GtkTextBuffer *buffer;

    switch (prop_id)
    {
        case PROP_BUFFER:
            buffer = g_value_get_object (value);

            if (!buffer)
            {
                buffer = GTK_TEXT_BUFFER (g_object_new (view->priv->buffer_type, (const char*) NULL));
            }
            else if (!g_type_is_a (G_OBJECT_TYPE (buffer), view->priv->buffer_type))
            {
                g_critical ("buffer '%s' is not of type '%s', ignoring it",
                            g_type_name (G_OBJECT_TYPE (buffer)),
                            g_type_name (view->priv->buffer_type));
                buffer = GTK_TEXT_BUFFER (g_object_new (view->priv->buffer_type, (const char*) NULL));
            }
            else
            {
                g_object_ref (buffer);
            }

            if (view->priv->buffer)
                g_critical ("%s: buffer already set", G_STRFUNC);

            gtk_text_view_set_buffer (GTK_TEXT_VIEW (view), buffer);
            view->priv->buffer = buffer;
            break;

        case PROP_INDENTER:
            moo_text_view_set_indenter (view, g_value_get_object (value));
            break;

        case PROP_TAB_WIDTH:
            moo_text_view_set_tab_width (view, g_value_get_uint (value));
            break;

        case PROP_HIGHLIGHT_MATCHING_BRACKETS:
            view->priv->highlight_matching_brackets = g_value_get_boolean (value);
            if (view->priv->constructed)
                g_object_set (get_buffer (view), "highlight-matching-brackets",
                              view->priv->highlight_matching_brackets, NULL);
            break;

        case PROP_HIGHLIGHT_MISMATCHING_BRACKETS:
            view->priv->highlight_mismatching_brackets = g_value_get_boolean (value);
            if (view->priv->constructed)
                g_object_set (get_buffer (view), "highlight-mismatching-brackets",
                              view->priv->highlight_mismatching_brackets, NULL);
            break;

        case PROP_HIGHLIGHT_CURRENT_LINE:
            moo_text_view_set_highlight_current_line (view, g_value_get_boolean (value));
            break;
        case PROP_HIGHLIGHT_CURRENT_LINE_UNFOCUSED:
            moo_text_view_set_highlight_current_line_unfocused (view, g_value_get_boolean (value));
            break;
        case PROP_DRAW_RIGHT_MARGIN:
            moo_text_view_set_draw_right_margin (view, g_value_get_boolean (value));
            break;
        case PROP_RIGHT_MARGIN_OFFSET:
            moo_text_view_set_right_margin_offset (view, g_value_get_uint (value));
            break;

        case PROP_CURRENT_LINE_COLOR:
            moo_text_view_set_current_line_color (view, g_value_get_string (value));
            break;
        case PROP_RIGHT_MARGIN_COLOR:
            moo_text_view_set_right_margin_color (view, g_value_get_string (value));
            break;

        case PROP_DRAW_WHITESPACE:
            set_draw_whitespace (view, g_value_get_flags (value));
            break;

        case PROP_MANAGE_CLIPBOARD:
            set_manage_clipboard (view, g_value_get_boolean (value));
            break;

        case PROP_SMART_HOME_END:
            view->priv->smart_home_end = g_value_get_boolean (value) != 0;
            g_object_notify (object, "smart-home-end");
            break;

        case PROP_ENABLE_HIGHLIGHT:
            moo_text_buffer_set_highlight (get_moo_buffer (view),
                                           g_value_get_boolean (value));
            g_object_notify (object, "enable-highlight");
            break;

        case PROP_SHOW_LINE_NUMBERS:
            moo_text_view_set_show_line_numbers (view, g_value_get_boolean (value));
            break;

        case PROP_SHOW_LINE_MARKS:
            set_show_line_marks (view, g_value_get_boolean (value));
            break;

        case PROP_ENABLE_FOLDING:
            set_enable_folding (view, g_value_get_boolean (value));
            break;

        case PROP_ENABLE_QUICK_SEARCH:
            view->priv->qs.enable = g_value_get_boolean (value) != 0;
            g_object_notify (object, "enable-quick-search");
            break;

        case PROP_QUICK_SEARCH_FLAGS:
            view->priv->qs.flags = g_value_get_flags (value);
            g_object_notify (object, "quick-search-flags");
            break;

        case PROP_AUTO_INDENT:
            view->priv->enter_indents = g_value_get_boolean (value);
            g_object_notify (object, "auto-indent");
            break;

        case PROP_TAB_INDENTS:
            view->priv->tab_indents = g_value_get_boolean (value);
            g_object_notify (object, "tab-indents");
            break;

        case PROP_BACKSPACE_INDENTS:
            view->priv->backspace_indents = g_value_get_boolean (value);
            g_object_notify (object, "backspace-indents");
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
moo_text_view_get_property (GObject        *object,
                            guint           prop_id,
                            GValue         *value,
                            GParamSpec     *pspec)
{
    MooTextView *view = MOO_TEXT_VIEW (object);
    gboolean val;

    switch (prop_id)
    {
        case PROP_BUFFER:
            g_value_set_object (value, get_buffer (view));
            break;
        case PROP_INDENTER:
            g_value_set_object (value, view->priv->indenter);
            break;
        case PROP_TAB_WIDTH:
            g_value_set_uint (value, view->priv->tab_width);
            break;
        case PROP_HIGHLIGHT_CURRENT_LINE_UNFOCUSED:
            g_value_set_boolean (value, view->priv->highlight_current_line_unfocused);
            break;
        case PROP_HIGHLIGHT_CURRENT_LINE:
            g_value_set_boolean (value, view->priv->color_settings[MOO_TEXT_VIEW_COLOR_CURRENT_LINE]);
            break;
        case PROP_DRAW_RIGHT_MARGIN:
            g_value_set_boolean (value, view->priv->color_settings[MOO_TEXT_VIEW_COLOR_RIGHT_MARGIN]);
            break;
        case PROP_RIGHT_MARGIN_OFFSET:
            g_value_set_uint (value, view->priv->right_margin_offset);
            break;
        case PROP_HIGHLIGHT_MATCHING_BRACKETS:
            g_object_get (get_buffer (view), "highlight-matching-brackets", &val, NULL);
            g_value_set_boolean (value, val);
            break;
        case PROP_HIGHLIGHT_MISMATCHING_BRACKETS:
            g_object_get (get_buffer (view), "highlight-mismatching-brackets", &val, NULL);
            g_value_set_boolean (value, val);
            break;
        case PROP_CURRENT_LINE_COLOR:
            g_value_set_string (value, view->priv->colors[MOO_TEXT_VIEW_COLOR_CURRENT_LINE]);
            break;
        case PROP_RIGHT_MARGIN_COLOR:
            g_value_set_string (value, view->priv->colors[MOO_TEXT_VIEW_COLOR_RIGHT_MARGIN]);
            break;
        case PROP_DRAW_WHITESPACE:
            g_value_set_flags (value, view->priv->draw_whitespace);
            break;
        case PROP_HAS_TEXT:
            g_value_set_boolean (value, moo_text_view_has_text (view));
            break;
        case PROP_HAS_SELECTION:
            g_value_set_boolean (value, moo_text_view_has_selection (view));
            break;
        case PROP_CAN_UNDO:
            g_value_set_boolean (value, moo_text_view_can_undo (view));
            break;
        case PROP_CAN_REDO:
            g_value_set_boolean (value, moo_text_view_can_redo (view));
            break;
        case PROP_MANAGE_CLIPBOARD:
            g_value_set_boolean (value, view->priv->manage_clipboard != 0);
            break;
        case PROP_SMART_HOME_END:
            g_value_set_boolean (value, view->priv->smart_home_end != 0);
            break;
        case PROP_ENABLE_HIGHLIGHT:
            g_value_set_boolean (value, moo_text_buffer_get_highlight (get_moo_buffer (view)));
            break;
        case PROP_SHOW_LINE_NUMBERS:
            g_value_set_boolean (value, view->priv->lm.show_numbers);
            break;
        case PROP_SHOW_LINE_MARKS:
            g_value_set_boolean (value, view->priv->lm.show_icons);
            break;
        case PROP_ENABLE_FOLDING:
            g_value_set_boolean (value, view->priv->enable_folding);
            break;
        case PROP_ENABLE_QUICK_SEARCH:
            g_value_set_boolean (value, view->priv->qs.enable);
            break;
        case PROP_QUICK_SEARCH_FLAGS:
            g_value_set_flags (value, view->priv->qs.flags);
            break;
        case PROP_AUTO_INDENT:
            g_value_set_boolean (value, view->priv->enter_indents != 0);
            break;
        case PROP_TAB_INDENTS:
            g_value_set_boolean (value, view->priv->tab_indents);
            break;
        case PROP_BACKSPACE_INDENTS:
            g_value_set_boolean (value, view->priv->backspace_indents != 0);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


char *
moo_text_view_get_selection (GtkTextView *view)
{
    GtkTextBuffer *buf;
    GtkTextIter start, end;

    g_return_val_if_fail (GTK_IS_TEXT_VIEW (view), NULL);

    buf = gtk_text_view_get_buffer (view);

    if (gtk_text_buffer_get_selection_bounds (buf, &start, &end))
        return gtk_text_buffer_get_slice (buf, &start, &end, TRUE);
    else
        return NULL;
}


gboolean
moo_text_view_has_selection (MooTextView *view)
{
    g_return_val_if_fail (MOO_IS_TEXT_VIEW (view), FALSE);
    return moo_text_buffer_has_selection (get_moo_buffer (view));
}


gboolean
moo_text_view_has_text (MooTextView *view)
{
    g_return_val_if_fail (MOO_IS_TEXT_VIEW (view), FALSE);
    return moo_text_buffer_has_text (get_moo_buffer (view));
}


char *
moo_text_view_get_text (GtkTextView *view)
{
    GtkTextBuffer *buf;
    GtkTextIter start, end;
    char *text;

    g_return_val_if_fail (GTK_IS_TEXT_VIEW (view), NULL);

    buf = gtk_text_view_get_buffer (view);
    gtk_text_buffer_get_bounds (buf, &start, &end);
    text = gtk_text_buffer_get_slice (buf, &start, &end, TRUE);

    if (text && *text)
    {
        return text;
    }
    else
    {
        g_free (text);
        return NULL;
    }
}


static void
insert_text_cb (MooTextView        *view,
                GtkTextIter        *iter,
                gchar              *text,
                gint                len)
{
    if (view->priv->in_key_press && g_utf8_strlen (text, len) == 1)
    {
        view->priv->in_key_press = FALSE;
        view->priv->char_inserted = g_strdup (text);
        view->priv->char_inserted_offset = gtk_text_iter_get_offset (iter);
    }
}


void
_moo_text_view_check_char_inserted (MooTextView *view)
{
    if (view->priv->char_inserted)
    {
        gboolean result;
        GtkTextIter iter;

        gtk_text_buffer_get_iter_at_offset (get_buffer (view), &iter,
                                            view->priv->char_inserted_offset);

        g_signal_emit (view, signals[CHAR_INSERTED], 0,
                       &iter, view->priv->char_inserted,
                       &result);

        g_free (view->priv->char_inserted);
        view->priv->char_inserted = NULL;
    }
}


static gboolean
moo_text_view_char_inserted (MooTextView *view,
                             GtkTextIter *where,
                             const char  *character)
{
    if (view->priv->indenter)
    {
        GtkTextBuffer *buffer = get_buffer (view);
        gtk_text_buffer_begin_user_action (buffer);
        moo_indenter_character (view->priv->indenter,
                                character, where);
        gtk_text_buffer_end_user_action (buffer);
        return TRUE;
    }

    return FALSE;
}


void
moo_text_view_set_word_chars (MooTextView *view,
                              const char  *word_chars)
{
    guint i;

    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    g_return_if_fail (!word_chars || g_utf8_validate (word_chars, -1, NULL));

    g_free (view->priv->word_chars);
    view->priv->word_chars = NULL;
    view->priv->n_word_chars = 0;

    if (!word_chars || !word_chars[0])
        return;

    view->priv->n_word_chars = g_utf8_strlen (word_chars, -1);
    view->priv->word_chars = g_new0 (gunichar, view->priv->n_word_chars);

    for (i = 0; *word_chars; i++, word_chars = g_utf8_next_char (word_chars))
        view->priv->word_chars[i] = g_utf8_get_char (word_chars);
}


static void
cursor_moved (MooTextView *view,
              GtkTextIter *where)
{
    g_signal_emit (view, signals[CURSOR_MOVED], 0, where);
}


static void
proxy_prop_notify (MooTextView *view,
                   GParamSpec  *pspec)
{
    g_object_notify (G_OBJECT (view), pspec->name);
}


static void
undo_ops_undo (MooUndoOps *obj)
{
    gboolean retval;
    g_signal_emit_by_name (obj, "undo", &retval);
}

static void
undo_ops_redo (MooUndoOps *obj)
{
    gboolean retval;
    g_signal_emit_by_name (obj, "redo", &retval);
}

static gboolean
undo_ops_can_undo (MooUndoOps *obj)
{
    return moo_text_view_can_undo (MOO_TEXT_VIEW (obj));
}

static gboolean
undo_ops_can_redo (MooUndoOps *obj)
{
    return moo_text_view_can_redo (MOO_TEXT_VIEW (obj));
}

static void
undo_ops_iface_init (MooUndoOpsIface *iface)
{
    iface->undo = undo_ops_undo;
    iface->redo = undo_ops_redo;
    iface->can_undo = undo_ops_can_undo;
    iface->can_redo = undo_ops_can_redo;
}

static void
proxy_notify_can_undo_redo (MooTextView *view,
                            GParamSpec  *pspec)
{
    if (strcmp (pspec->name, "can-undo") == 0)
        moo_undo_ops_can_undo_changed (G_OBJECT (view));
    else
        moo_undo_ops_can_redo_changed (G_OBJECT (view));

    g_object_notify (G_OBJECT (view), pspec->name);
}


MooIndenter*
moo_text_view_get_indenter (MooTextView *view)
{
    g_return_val_if_fail (MOO_IS_TEXT_VIEW (view), NULL);
    return view->priv->indenter;
}


void
moo_text_view_set_indenter (MooTextView *view,
                            MooIndenter *indenter)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    g_return_if_fail (!indenter || MOO_IS_INDENTER (indenter));

    if (view->priv->indenter == indenter)
        return;

    if (view->priv->indenter)
        g_object_unref (view->priv->indenter);
    view->priv->indenter = indenter;
    if (view->priv->indenter)
        g_object_ref (view->priv->indenter);

    g_object_notify (G_OBJECT (view), "indenter");
}


typedef struct {
    MooTextView *view;
    int      line;
    int      character;
    gboolean visual;
} Scroll;


static int
iter_get_chars_in_line (const GtkTextIter *iter)
{
    GtkTextIter i = *iter;

    if (!gtk_text_iter_ends_line (&i))
        gtk_text_iter_forward_to_line_end (&i);

    return gtk_text_iter_get_line_offset (&i);
}

static gboolean
do_move_cursor (Scroll *scroll)
{
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    g_return_val_if_fail (scroll != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_TEXT_VIEW (scroll->view), FALSE);
    g_return_val_if_fail (scroll->line >= 0, FALSE);

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (scroll->view));

    if (scroll->line >= gtk_text_buffer_get_line_count (buffer))
        scroll->line = gtk_text_buffer_get_line_count (buffer) - 1;

    gtk_text_buffer_get_iter_at_line (buffer, &iter, scroll->line);

    if (scroll->character >= 0)
    {
        if (scroll->visual)
        {
            int line_len = moo_text_iter_get_visual_line_length (&iter, 8);

            if (scroll->character > line_len)
                scroll->character = line_len;
            else if (scroll->character < 0)
                scroll->character = 0;

            if (scroll->character == line_len)
            {
                if (!gtk_text_iter_ends_line (&iter))
                    gtk_text_iter_forward_to_line_end (&iter);
            }
            else
            {
                moo_text_iter_set_visual_line_offset (&iter, scroll->character, 8);
            }
        }
        else
        {
            int line_len = iter_get_chars_in_line (&iter);

            if (scroll->character > line_len)
                scroll->character = line_len;
            else if (scroll->character < 0)
                scroll->character = 0;

            if (scroll->character == line_len)
            {
                if (!gtk_text_iter_ends_line (&iter))
                    gtk_text_iter_forward_to_line_end (&iter);
            }
            else
            {
                gtk_text_iter_set_line_offset (&iter, scroll->character);
            }
        }
    }

    gtk_text_buffer_place_cursor (buffer, &iter);
    gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (scroll->view),
                                  gtk_text_buffer_get_insert (buffer),
                                  0.2, FALSE, 0, 0);

    if (MOO_IS_TEXT_VIEW (scroll->view))
    {
        MooTextView *view = MOO_TEXT_VIEW (scroll->view);

        if (view->priv->move_cursor_idle)
        {
            g_source_remove (view->priv->move_cursor_idle);
            view->priv->move_cursor_idle = 0;
        }
    }

    return FALSE;
}


void
moo_text_view_move_cursor (MooTextView *view,
                           int          line,
                           int          offset,
                           gboolean     offset_visual,
                           gboolean     in_idle)
{
    Scroll scroll;

    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    scroll.view = view;
    scroll.line = line;
    scroll.character = offset;
    scroll.visual = offset_visual;

    do_move_cursor (&scroll);

    if (in_idle)
    {
        MooTextView *mview;

        g_return_if_fail (MOO_IS_TEXT_VIEW (view));

        mview = MOO_TEXT_VIEW (view);
        mview->priv->move_cursor_idle =
            g_idle_add_full (G_PRIORITY_HIGH_IDLE + 9, /* between gtktextview's first validate priority and
                                                          * GTK_PRIORITY_RESIZE */
                                       (GSourceFunc) do_move_cursor,
                                       g_memdup (&scroll, sizeof scroll),
                                       g_free);
    }
}


void
moo_text_view_get_cursor (GtkTextView *view,
                          GtkTextIter *iter)
{
    GtkTextBuffer *buffer;

    g_return_if_fail (GTK_IS_TEXT_VIEW (view));
    g_return_if_fail (iter != NULL);

    buffer = gtk_text_view_get_buffer (view);
    gtk_text_buffer_get_iter_at_mark (buffer, iter,
                                      gtk_text_buffer_get_insert (buffer));
}


int
moo_text_view_get_cursor_line (GtkTextView *view)
{
    GtkTextIter iter;

    g_return_val_if_fail (GTK_IS_TEXT_VIEW (view), -1);

    moo_text_view_get_cursor (view, &iter);
    return gtk_text_iter_get_line (&iter);
}


static GtkTextBuffer*
get_buffer (MooTextView *view)
{
    return gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
}


static MooTextBuffer*
get_moo_buffer (MooTextView *view)
{
    return MOO_TEXT_BUFFER (get_buffer (view));
}


static GtkTextMark*
get_insert (MooTextView    *view)
{
    return gtk_text_buffer_get_insert (get_buffer (view));
}


static void
moo_text_view_set_color_setting (MooTextView     *view,
                                 const char      *prop,
                                 gboolean         setting,
                                 MooTextViewColor color_num)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    if (view->priv->color_settings[color_num] != setting)
    {
        view->priv->color_settings[color_num] = setting;
        invalidate_gcs (view);
        gtk_widget_queue_draw (GTK_WIDGET (view));
        g_object_notify (G_OBJECT (view), prop);
    }
}

void
moo_text_view_set_highlight_current_line (MooTextView *view,
                                          gboolean     highlight)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    moo_text_view_set_color_setting (view, "highlight-current-line", highlight,
                                     MOO_TEXT_VIEW_COLOR_CURRENT_LINE);
}

void
moo_text_view_set_highlight_current_line_unfocused (MooTextView *view,
                                                    gboolean     highlight)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    if (view->priv->highlight_current_line_unfocused != highlight)
    {
        view->priv->highlight_current_line_unfocused = highlight;
        gtk_widget_queue_draw (GTK_WIDGET (view));
        g_object_notify (G_OBJECT (view), "highlight-current-line-unfocused");
    }
}

void
moo_text_view_set_draw_right_margin (MooTextView *view,
                                     gboolean     draw)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    moo_text_view_set_color_setting (view, "draw-right-margin", draw,
                                     MOO_TEXT_VIEW_COLOR_RIGHT_MARGIN);
}


void
moo_text_view_set_right_margin_offset (MooTextView *view,
                                       guint        offset)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    g_return_if_fail (offset > 0);

    if (view->priv->right_margin_offset != offset)
    {
        view->priv->right_margin_offset = offset;

        invalidate_right_margin (view);

        if (view->priv->color_settings[MOO_TEXT_VIEW_COLOR_RIGHT_MARGIN])
            gtk_widget_queue_draw (GTK_WIDGET (view));

        g_object_notify (G_OBJECT (view), "right-margin-offset");
    }
}


static void
moo_text_view_set_color (MooTextView      *view,
                         MooTextViewColor  color_num,
                         const char       *color,
                         const char       *propname)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    g_return_if_fail (color_num < MOO_TEXT_VIEW_N_COLORS);

    MOO_ASSIGN_STRING (view->priv->colors[color_num], color);

    invalidate_gcs (view);

    if (GTK_WIDGET_DRAWABLE (view) && view->priv->color_settings[color_num])
        gtk_widget_queue_draw (GTK_WIDGET (view));

    g_object_notify (G_OBJECT (view), propname);
}

void
moo_text_view_set_current_line_color (MooTextView *view,
                                      const char  *color)
{
    moo_text_view_set_color (view, MOO_TEXT_VIEW_COLOR_CURRENT_LINE,
                             color, "current-line-color");
}

void
moo_text_view_set_right_margin_color (MooTextView *view,
                                      const char  *color)
{
    moo_text_view_set_color (view, MOO_TEXT_VIEW_COLOR_RIGHT_MARGIN,
                             color, "right-margin-color");
}


/********************************************************************/
/* Clipboard
 */

enum {
    TARGET_TEXT,
};

static const GtkTargetEntry targets[] = {
    { (char*) "STRING", 0, TARGET_TEXT },
    { (char*) "TEXT",   0, TARGET_TEXT },
    { (char*) "COMPOUND_TEXT", 0, TARGET_TEXT },
    { (char*) "UTF8_STRING", 0, TARGET_TEXT }
};


static void
add_selection_clipboard (MooTextView *view)
{
    GtkClipboard *clipboard;
    clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view), GDK_SELECTION_PRIMARY);
    gtk_text_buffer_add_selection_clipboard (get_buffer (view), clipboard);
}

static void
remove_selection_clipboard (MooTextView *view)
{
    GtkClipboard *clipboard;
    clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view), GDK_SELECTION_PRIMARY);
    gtk_text_buffer_remove_selection_clipboard (get_buffer (view), clipboard);
}


static void
clipboard_get_selection (G_GNUC_UNUSED GtkClipboard *clipboard,
                         GtkSelectionData *selection_data,
                         G_GNUC_UNUSED guint info,
                         gpointer data)
{
    MooTextView *view = data;
    GtkTextIter start, end;

    if (gtk_text_buffer_get_selection_bounds (get_buffer (view), &start, &end))
    {
        char *text = gtk_text_iter_get_text (&start, &end);
        gtk_selection_data_set_text (selection_data, text, -1);
        g_free (text);
    }
}


static void
clear_primary (MooTextView *view)
{
    GtkClipboard *clipboard;

    clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
                                          GDK_SELECTION_PRIMARY);

    if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (view))
        gtk_clipboard_clear (clipboard);
}


static void
selection_changed (MooTextView   *view,
                   MooTextBuffer *buffer)
{
    GtkClipboard *clipboard;

    if (!GTK_WIDGET_REALIZED (view) || !GTK_WIDGET_HAS_FOCUS (view) || !view->priv->manage_clipboard)
        return;

    clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
                                          GDK_SELECTION_PRIMARY);

    if (moo_text_buffer_has_selection (buffer))
        gtk_clipboard_set_with_owner (clipboard, targets,
                                      G_N_ELEMENTS (targets),
                                      clipboard_get_selection,
                                      NULL, G_OBJECT (view));
}


static void
set_manage_clipboard (MooTextView    *view,
                      gboolean        manage)
{
    if (BOOL_CMP (view->priv->manage_clipboard, manage))
        return;

    view->priv->manage_clipboard = manage;

    if (GTK_WIDGET_REALIZED (view))
    {
        if (manage)
        {
            remove_selection_clipboard (view);
            selection_changed (view, get_moo_buffer (view));
        }
        else
        {
            clear_primary (view);
            add_selection_clipboard (view);
        }
    }

    g_object_notify (G_OBJECT (view), "manage-clipboard");
}


static void
get_clipboard (G_GNUC_UNUSED GtkClipboard *clipboard,
               GtkSelectionData *selection_data,
               G_GNUC_UNUSED guint info,
               gpointer data)
{
    MooTextViewClipboard *contents = data;
    gtk_selection_data_set_text (selection_data, contents->text, -1);
}


static void
clear_clipboard (G_GNUC_UNUSED GtkClipboard *clipboard,
                 gpointer data)
{
    MooTextViewClipboard *contents = data;

    if (contents->view && contents->view->priv->clipboard == contents)
        contents->view->priv->clipboard = NULL;

    g_free (contents->text);
    g_slice_free (MooTextViewClipboard, contents);
}


static void
moo_text_view_cut_or_copy (GtkTextView *text_view,
                           gboolean     delete,
                           GdkAtom      clipboard_type)
{
    GtkTextBuffer *buffer;
    GtkTextIter start, end;
    GtkClipboard *clipboard;
    MooTextViewClipboard *contents;
    MooTextView *view = MOO_TEXT_VIEW (text_view);

    buffer = gtk_text_view_get_buffer (text_view);

    if (!gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
        return;

    clipboard = gtk_widget_get_clipboard (GTK_WIDGET (text_view),
                                          clipboard_type);

    contents = g_slice_new0 (MooTextViewClipboard);
    contents->text = gtk_text_buffer_get_slice (buffer, &start, &end, TRUE);
    contents->view = view;

    if (view->priv->clipboard)
        view->priv->clipboard->view = NULL;
    view->priv->clipboard = contents;

    if (!gtk_clipboard_set_with_data (clipboard, targets, G_N_ELEMENTS (targets),
                                      get_clipboard, clear_clipboard,
                                      contents))
        return;

    gtk_clipboard_set_can_store (clipboard, targets + 1,
                                 G_N_ELEMENTS (targets) - 1);

    if (delete)
    {
        gtk_text_buffer_begin_user_action (buffer);
        gtk_text_buffer_delete (buffer, &start, &end);
        gtk_text_buffer_end_user_action (buffer);
        gtk_text_view_scroll_mark_onscreen (text_view,
                                            gtk_text_buffer_get_insert (buffer));
    }
}

void
_moo_text_view_ensure_primary (GtkTextView *text_view)
{
    moo_text_view_cut_or_copy (text_view, FALSE, GDK_SELECTION_PRIMARY);
}

static void
moo_text_view_copy_clipboard (GtkTextView *text_view)
{
    moo_text_view_cut_or_copy (text_view, FALSE, GDK_SELECTION_CLIPBOARD);
}


static void
moo_text_view_cut_clipboard (GtkTextView *text_view)
{
    moo_text_view_cut_or_copy (text_view, TRUE, GDK_SELECTION_CLIPBOARD);
}


static void
paste_text (GtkTextView *text_view,
            const char  *text)
{
    GtkTextBuffer *buffer;
    GtkTextIter start, end;

    g_return_if_fail (text != NULL);

    buffer = gtk_text_view_get_buffer (text_view);

    if (gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
        gtk_text_buffer_delete (buffer, &start, &end);

    gtk_text_buffer_insert (buffer, &end, text, -1);
}


static void
moo_text_view_paste_clipboard (GtkTextView *text_view)
{
    char *text;
    GtkTextBuffer *buffer;
    GtkClipboard *clipboard;

    buffer = gtk_text_view_get_buffer (text_view);
    clipboard = gtk_widget_get_clipboard (GTK_WIDGET (text_view), GDK_SELECTION_CLIPBOARD);

    gtk_text_buffer_begin_user_action (buffer);

    if ((text = gtk_clipboard_wait_for_text (clipboard)))
    {
        paste_text (text_view, text);
        g_free (text);
    }

    gtk_text_buffer_end_user_action (buffer);
    gtk_text_view_scroll_mark_onscreen (text_view, gtk_text_buffer_get_insert (buffer));
}


/********************************************************************/
/* Drawing and stuff
 */

/* XXX: workaround for http://bugzilla.gnome.org/show_bug.cgi?id=336796 */
static void
lower_border_window (GtkTextView   *view,
                     MooTextViewPos pos)
{
    GdkWindow *window;

    g_return_if_fail (pos < 4);

    window = gtk_text_view_get_window (view, window_types[pos]);

    if (window)
        window = gdk_window_get_parent (window);

    if (window)
        gdk_window_lower (window);
}


static void
moo_text_view_realize (GtkWidget *widget)
{
    MooTextView *view = MOO_TEXT_VIEW (widget);
    guint i;

    GTK_WIDGET_CLASS(moo_text_view_parent_class)->realize (widget);

    update_tab_width (view);
    update_left_margin (view);

    if (view->priv->manage_clipboard)
    {
        remove_selection_clipboard (view);
        selection_changed (view, get_moo_buffer (view));
    }

    /* workaround for http://bugzilla.gnome.org/show_bug.cgi?id=336796 */
    for (i = 0; i < 4; ++i)
    {
        if (view->priv->children[i] && GTK_WIDGET_VISIBLE (view->priv->children[i]))
            lower_border_window (GTK_TEXT_VIEW (view), i);
    }

    _moo_text_view_update_text_cursor (view, -1, -1);
}


static void
moo_text_view_unrealize (GtkWidget *widget)
{
    MooTextView *view = MOO_TEXT_VIEW (widget);

    g_slist_foreach (view->priv->line_marks, (GFunc) _moo_line_mark_unrealize, NULL);
    g_object_set_data (G_OBJECT (widget), "moo-line-mark-icons", NULL);
    g_object_set_data (G_OBJECT (widget), "moo-line-mark-colors", NULL);

    if (view->priv->update_idle)
        g_source_remove (view->priv->update_idle);
    view->priv->update_idle = 0;

    if (view->priv->update_n_lines_idle)
        g_source_remove (view->priv->update_n_lines_idle);
    view->priv->update_n_lines_idle = 0;

    g_free (view->priv->update_rectangle);
    view->priv->update_rectangle = NULL;

    invalidate_gcs (view);

    if (view->priv->manage_clipboard)
    {
        clear_primary (view);
        add_selection_clipboard (view);
    }

    if (view->priv->dnd.scroll_timeout)
    {
        g_source_remove (view->priv->dnd.scroll_timeout);
        view->priv->dnd.scroll_timeout = 0;
    }

    GTK_WIDGET_CLASS(moo_text_view_parent_class)->unrealize (widget);
}


static void
invalidate_gcs (MooTextView *view)
{
    int i;

    for (i = 0; i < MOO_TEXT_VIEW_N_COLORS; ++i)
    {
        if (view->priv->gcs[i])
            gdk_color_free(view->priv->gcs[i]);
        view->priv->gcs[i] = NULL;
    }
}

static void
update_gc (MooTextView     *view,
           MooTextViewColor color_num)
{
    GdkColor color;
    GtkWidget *widget = GTK_WIDGET (view);
    GdkWindow *window;

    if (!GTK_WIDGET_REALIZED (widget))
        return;

    g_return_if_fail (color_num < MOO_TEXT_VIEW_N_COLORS);

    if (!view->priv->color_settings[color_num])
    {
        if (view->priv->gcs[color_num])
        {
            gdk_color_free(view->priv->gcs[color_num]);
            view->priv->gcs[color_num] = NULL;
        }

        return;
    }

    if (view->priv->gcs[color_num])
        return;

    window = gtk_text_view_get_window (GTK_TEXT_VIEW (view),
                                       GTK_TEXT_WINDOW_TEXT);
    g_return_if_fail (window != NULL);

    if (view->priv->colors[color_num])
    {
        if (!gdk_color_parse (view->priv->colors[color_num], &color))
        {
            g_warning ("could not parse color %s",
                       view->priv->colors[color_num]);
            color = widget->style->bg[GTK_STATE_NORMAL];
        }
    }
    else
    {
            color = widget->style->bg[GTK_STATE_NORMAL];
        }

    if (!view->priv->gcs[color_num])
        view->priv->gcs[color_num] = gdk_color_copy(&color);
}

static void
update_gcs (MooTextView *view)
{
    update_gc (view, MOO_TEXT_VIEW_COLOR_CURRENT_LINE);
    update_gc (view, MOO_TEXT_VIEW_COLOR_RIGHT_MARGIN);
}


static void
moo_text_view_draw_right_margin (GtkTextView    *text_view,
                                 GdkEventExpose *event)
{
    int x, y;
    cairo_t *cr;
    MooTextView *view = MOO_TEXT_VIEW (text_view);

    update_right_margin (view);

    y = gdk_window_get_height(event->window);
    x = view->priv->right_margin_pixel_offset + gtk_text_view_get_left_margin (text_view);
    if (text_view->hadjustment)
        x -= text_view->hadjustment->value;

    cr = gdk_cairo_create(event->window);
    gdk_cairo_set_source_color(cr, view->priv->gcs[MOO_TEXT_VIEW_COLOR_RIGHT_MARGIN]);
    cairo_rectangle(cr, x, 0, 1, y);
    cairo_fill(cr);
    cairo_destroy(cr);
}

static void
moo_text_view_get_line_yrange (GtkTextView       *text_view,
                               const GtkTextIter *iter,
                               int               *y,
                               int               *height)
{
    if (gtk_text_view_get_wrap_mode (text_view) == GTK_WRAP_NONE)
    {
        gtk_text_view_get_line_yrange (text_view, iter, y, height);
    }
    else
    {
        GdkRectangle rect;
        gtk_text_view_get_iter_location (text_view, iter, &rect);
        *y = rect.y;
        *height = rect.height;
    }
}

static void
moo_text_view_draw_current_line (GtkTextView    *text_view,
                                 GdkEventExpose *event)
{
    GdkRectangle visible_rect;
    GdkRectangle redraw_rect;
    GtkTextIter cur;
    int y, height;
    int win_y;
    int margin;
    cairo_t *cr;

    gtk_text_buffer_get_iter_at_mark (text_view->buffer,
                                      &cur,
                                      gtk_text_buffer_get_insert (text_view->buffer));

    moo_text_view_get_line_yrange (text_view, &cur, &y, &height);

    gtk_text_view_get_visible_rect (text_view, &visible_rect);

    gtk_text_view_buffer_to_window_coords (text_view,
                                           GTK_TEXT_WINDOW_TEXT,
                                           visible_rect.x,
                                           visible_rect.y,
                                           &redraw_rect.x,
                                           &redraw_rect.y);

    gtk_text_view_buffer_to_window_coords (text_view,
                                           GTK_TEXT_WINDOW_TEXT,
                                           0,
                                           y,
                                           NULL,
                                           &win_y);

    redraw_rect.width = visible_rect.width;
    redraw_rect.height = visible_rect.height;

    if (text_view->hadjustment)
    	margin = gtk_text_view_get_left_margin (text_view) - (int) text_view->hadjustment->value;
    else
    	margin = gtk_text_view_get_left_margin (text_view);

    cr = gdk_cairo_create (event->window);
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
    gdk_cairo_set_source_color(cr, MOO_TEXT_VIEW(text_view)->priv->gcs[MOO_TEXT_VIEW_COLOR_CURRENT_LINE]);
    cairo_rectangle(cr, redraw_rect.x + MAX(0, margin - 1),
                        win_y,
                        redraw_rect.width,
                        height);
    cairo_fill(cr);
    cairo_destroy(cr);
}


static void
draw_tab_at_iter (GtkTextView    *text_view,
                  GdkEventExpose *event,
                  GtkTextIter    *iter)
{
    GdkRectangle rect;
    GdkPoint points[3];

    gtk_text_view_get_iter_location (text_view, iter, &rect);
    gtk_text_view_buffer_to_window_coords (text_view, GTK_TEXT_WINDOW_TEXT,
                                           rect.x, rect.y + rect.height - 2,
                                           &points[0].x, &points[0].y);
    points[1] = points[0];
    points[2] = points[0];
    points[1].y += 1;
    points[2].x += 1;
    points[2].y += 1;
    gdk_draw_polygon (event->window,
                      GTK_WIDGET(text_view)->style->text_gc[GTK_STATE_NORMAL],
                      FALSE, points, 3);
}

static void
moo_text_view_draw_whitespace (GtkTextView       *text_view,
                               GdkEventExpose    *event,
                               const GtkTextIter *start,
                               const GtkTextIter *end)
{
    MooTextView *view = MOO_TEXT_VIEW (text_view);
    GtkTextIter iter = *start;
    GtkTextIter cursor;
    int cursor_line;
    int line;

    if (view->priv->draw_whitespace == 0)
        return;

    line = gtk_text_iter_get_line (&iter);
    moo_text_view_get_cursor (GTK_TEXT_VIEW (text_view), &cursor);
    cursor_line = gtk_text_iter_get_line (&cursor);
    cursor_line = -1; /* FIXME */

    do
    {
        gboolean trailing = TRUE;

        if (!gtk_text_iter_ends_line (&iter))
            gtk_text_iter_forward_to_line_end (&iter);

        while (!gtk_text_iter_starts_line (&iter))
        {
            gunichar c;

            if (line == cursor_line && gtk_text_iter_compare (&iter, &cursor) <= 0)
                break;

            gtk_text_iter_backward_char (&iter);
            c = gtk_text_iter_get_char (&iter);

            if (g_unichar_isspace (c))
            {
                if ((trailing && (view->priv->draw_whitespace & MOO_DRAW_WS_TRAILING) != 0) ||
                    (c == '\t' && (view->priv->draw_whitespace & MOO_DRAW_WS_TABS) != 0) ||
                    (c != '\t' && (view->priv->draw_whitespace & MOO_DRAW_WS_SPACES) != 0))
                        draw_tab_at_iter (text_view, event, &iter);
            }
            else if (trailing)
            {
                trailing = FALSE;
                if ((view->priv->draw_whitespace & ~MOO_DRAW_WS_TRAILING) == 0)
                    break;
            }
        }

        gtk_text_iter_forward_line (&iter);
        line += 1;
    }
    while (gtk_text_iter_compare (&iter, end) < 0);
}


static gboolean
moo_text_view_expose (GtkWidget      *widget,
                      GdkEventExpose *event)
{
    gboolean handled;
    MooTextView *view = MOO_TEXT_VIEW (widget);
    GtkTextView *text_view = GTK_TEXT_VIEW (widget);
    GdkWindow *text_window = gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT);
    GdkWindow *left_window = gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_LEFT);
    GtkTextIter start, end;

    update_gcs (view);

    view->priv->in_expose = TRUE;

    if (view->priv->update_n_lines_idle)
        update_n_lines_idle (view);

    if (GTK_WIDGET_SENSITIVE (view) &&
        event->window == text_window)
    {
        if ((GTK_WIDGET_HAS_FOCUS (view) ||
             view->priv->highlight_current_line_unfocused)
            && view->priv->color_settings[MOO_TEXT_VIEW_COLOR_CURRENT_LINE]
            && view->priv->gcs[MOO_TEXT_VIEW_COLOR_CURRENT_LINE])
        {
            moo_text_view_draw_current_line (text_view, event);
        }

        if (GTK_WIDGET_HAS_FOCUS (view) &&
            view->priv->color_settings[MOO_TEXT_VIEW_COLOR_RIGHT_MARGIN] &&
            view->priv->gcs[MOO_TEXT_VIEW_COLOR_RIGHT_MARGIN])
                moo_text_view_draw_right_margin (text_view, event);
    }

    if (event->window == left_window)
        draw_left_margin (view, event);
    else if (event->window == text_window)
        draw_marks_background (view, event);

    if (event->window == text_window)
    {
        GdkRectangle visible_rect;

        gtk_text_view_get_visible_rect (text_view, &visible_rect);
        gtk_text_view_get_line_at_y (text_view, &start, visible_rect.y, NULL);
        gtk_text_iter_backward_line (&start);
        gtk_text_view_get_line_at_y (text_view, &end, visible_rect.y + visible_rect.height, NULL);
        gtk_text_iter_forward_line (&end);

        _moo_text_buffer_update_highlight (get_moo_buffer (view), &start, &end, FALSE);
    }

    handled = GTK_WIDGET_CLASS(moo_text_view_parent_class)->expose_event (widget, event);

    if (event->window == text_window)
    {
        int first_line = gtk_text_iter_get_line (&start);
        int last_line = gtk_text_iter_get_line (&end);

        if (last_line - first_line < 1000)
        {
            if (view->priv->draw_whitespace != 0)
                moo_text_view_draw_whitespace (text_view, event, &start, &end);
        }
    }

    view->priv->in_expose = FALSE;

    return handled;
}


static gboolean
invalidate_rectangle (MooTextView *view)
{
    GdkWindow *window;
    GdkRectangle *rect = view->priv->update_rectangle;

    view->priv->update_rectangle = NULL;
    view->priv->update_idle = 0;

    gtk_text_view_buffer_to_window_coords (GTK_TEXT_VIEW (view),
                                           GTK_TEXT_WINDOW_TEXT,
                                           rect->x, rect->y,
                                           &rect->x, &rect->y);
    window = gtk_text_view_get_window (GTK_TEXT_VIEW (view), GTK_TEXT_WINDOW_TEXT);
    gdk_window_invalidate_rect (window, rect, FALSE);

    g_free (rect);
    return FALSE;
}


static void
highlight_updated (GtkTextView       *text_view,
                   const GtkTextIter *start,
                   const GtkTextIter *end)
{
    GdkRectangle visible, changed, update;
    int y, height;
    MooTextView *view = MOO_TEXT_VIEW (text_view);

    if (!GTK_WIDGET_DRAWABLE (text_view))
        return;

    gtk_text_view_get_visible_rect (text_view, &visible);

    gtk_text_view_get_line_yrange (text_view, start, &changed.y, &height);
    gtk_text_view_get_line_yrange (text_view, end, &y, &height);
    changed.height = y - changed.y + height;
    changed.x = visible.x;
    changed.width = visible.width;

    if (!gdk_rectangle_intersect (&changed, &visible, &update))
        return;

    if (view->priv->update_rectangle)
    {
        gdk_rectangle_union (view->priv->update_rectangle, &update,
                             view->priv->update_rectangle);
    }
    else
    {
        view->priv->update_rectangle = g_new (GdkRectangle, 1);
        *view->priv->update_rectangle = update;
    }

    if (view->priv->in_expose)
    {
        g_critical ("oops");

        if (!view->priv->update_idle)
            view->priv->update_idle =
                g_idle_add_full (G_PRIORITY_HIGH_IDLE,
                                               (GSourceFunc) invalidate_rectangle,
                                               view, NULL);
    }
    else
    {
        if (view->priv->update_idle)
            g_source_remove (view->priv->update_idle);
        view->priv->update_idle = 0;

        invalidate_rectangle (view);
    }
}


static void
set_draw_whitespace (MooTextView    *view,
                     MooDrawWsFlags  flags)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    if (view->priv->draw_whitespace == flags)
        return;

    view->priv->draw_whitespace = flags;
    g_object_notify (G_OBJECT (view), "draw-whitespace");

    if (GTK_WIDGET_DRAWABLE (view))
        gtk_widget_queue_draw (GTK_WIDGET (view));
}


GtkTextTag *
moo_text_view_lookup_tag (MooTextView    *view,
                          const char     *name)
{
    GtkTextBuffer *buffer;
    GtkTextTagTable *table;

    g_return_val_if_fail (MOO_IS_TEXT_VIEW (view), NULL);
    g_return_val_if_fail (name != NULL, NULL);

    buffer = get_buffer (view);
    table = gtk_text_buffer_get_tag_table (buffer);

    return gtk_text_tag_table_lookup (table, name);
}


#if 0
MOO_DEFINE_QUARK_STATIC ("moo-text-tag-cursor", cursor_quark)

void
moo_text_tag_set_cursor (GtkTextTag    *tag,
                         GdkCursorType  cursor)
{
    g_return_if_fail (GTK_IS_TEXT_TAG (tag));
    g_object_set_qdata (G_OBJECT (tag), cursor_quark (), GINT_TO_POINTER (cursor));
}

GdkCursorType
moo_text_tag_get_cursor (GtkTextTag *tag)
{
    g_return_val_if_fail (GTK_IS_TEXT_TAG (tag), 0);
    return GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (tag), cursor_quark ()));
}
#endif


void
moo_text_view_set_lang (MooTextView    *view,
                        MooLang        *lang)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    moo_text_buffer_set_lang (get_moo_buffer (view), lang);
    gtk_widget_queue_draw (GTK_WIDGET (view));
}


/**
 * moo_text_view_set_lang_by_id: (moo.private 1)
 *
 * @view:
 * @lang_id: (type const-utf8)
 */
void
moo_text_view_set_lang_by_id (MooTextView *view,
                              const char  *lang_id)
{
    MooLangMgr *mgr;
    MooLang *lang;
    MooTextStyleScheme *scheme;

    mgr = moo_lang_mgr_default ();
    lang = moo_lang_mgr_get_lang (mgr, lang_id);
    scheme = moo_lang_mgr_get_active_scheme (mgr);

    moo_text_view_set_style_scheme (view, scheme);
    moo_text_view_set_lang (view, lang);
}


MooLang*
moo_text_view_get_lang (MooTextView *view)
{
    g_return_val_if_fail (MOO_IS_TEXT_VIEW (view), NULL);
    return moo_text_buffer_get_lang (get_moo_buffer (view));
}


static void
moo_text_view_apply_style_scheme (MooTextView        *view,
                                  MooTextStyleScheme *scheme)
{
    _moo_text_style_scheme_apply (scheme, GTK_WIDGET (view));
    _moo_text_buffer_set_style_scheme (get_moo_buffer (view), scheme);
}

void
moo_text_view_set_style_scheme (MooTextView        *view,
                                MooTextStyleScheme *scheme)
{
    g_return_if_fail (MOO_IS_TEXT_STYLE_SCHEME (scheme));
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    if (view->priv->style_scheme == scheme)
        return;

    if (view->priv->style_scheme)
        g_object_unref (view->priv->style_scheme);

    view->priv->style_scheme = g_object_ref (scheme);

    MOO_TEXT_VIEW_GET_CLASS (view)->apply_style_scheme (view, scheme);
}

MooTextStyleScheme *
moo_text_view_get_style_scheme (MooTextView *view)
{
    g_return_val_if_fail (MOO_IS_TEXT_VIEW (view), NULL);
    return view->priv->style_scheme;
}


static void
moo_text_view_populate_popup (GtkTextView    *text_view,
                              GtkMenu        *menu)
{
    MooTextView *view = MOO_TEXT_VIEW (text_view);
    GtkWidget *item;

    item = gtk_separator_menu_item_new ();
    gtk_widget_show (item);
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_REDO, NULL);
    gtk_widget_show (item);
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
    g_signal_connect_swapped (item, "activate", G_CALLBACK (moo_text_view_redo), view);
    gtk_widget_set_sensitive (item, moo_text_view_can_redo (view));

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_UNDO, NULL);
    gtk_widget_show (item);
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
    g_signal_connect_swapped (item, "activate", G_CALLBACK (moo_text_view_undo), view);
    gtk_widget_set_sensitive (item, moo_text_view_can_undo (view));
}


int
_moo_text_view_get_line_height (MooTextView *view)
{
    PangoContext *ctx;
    PangoLayout *layout;
    PangoRectangle rect;
    int height;

    g_return_val_if_fail (MOO_IS_TEXT_VIEW (view), 10);

    ctx = gtk_widget_get_pango_context (GTK_WIDGET (view));
    g_return_val_if_fail (ctx != NULL, 10);

    layout = pango_layout_new (ctx);
    pango_layout_set_text (layout, "AA", -1);

    pango_layout_get_extents (layout, NULL, &rect);
    height = rect.height / PANGO_SCALE;

    g_object_unref (layout);
    return height;
}


/*****************************************************************************/
/* Left margin
 */

#define MARK_ICON_LPAD      0
#define MARK_ICON_RPAD      0
#define LINE_NUMBER_LPAD    0
#define LINE_NUMBER_RPAD    3

static PangoLayout *
create_line_numbers_layout (MooTextView *view)
{
    PangoLayout *layout;

    layout = gtk_widget_create_pango_layout (GTK_WIDGET (view), NULL);

    if (view->priv->lm.numbers_font)
        pango_layout_set_font_description (layout, view->priv->lm.numbers_font);

    return layout;
}


static int
get_n_digits (MooTextView *view)
{
    int i, n;
    int lines;

    lines = MAX (99, gtk_text_buffer_get_line_count (get_buffer (view)));

    for (i = 2, n = 100; ; ++i, n *= 10)
        if (lines < n)
            return i;
}


static void
update_line_mark_icons (MooTextView *view)
{
    int i;
    char str[32];
    GtkSettings *settings;
    PangoLayout *layout;

    g_return_if_fail (GTK_WIDGET_REALIZED (view));

    settings = gtk_widget_get_settings (GTK_WIDGET (view));

    if (!gtk_icon_size_lookup_for_settings (settings, GTK_ICON_SIZE_MENU,
                                            &view->priv->lm.icon_width, NULL))
        view->priv->lm.icon_width = 16;

    layout = gtk_widget_create_pango_layout (GTK_WIDGET (view), NULL);

    for (i = 1; i < 10; ++i)
    {
        PangoRectangle rect;
        g_snprintf (str, sizeof str, "<b>%d</b>", i);
        pango_layout_set_markup (layout, str, -1);
        pango_layout_get_pixel_extents  (layout, &rect, NULL);
        view->priv->lm.icon_width = MAX (view->priv->lm.icon_width, rect.width);
    }

    g_object_unref (layout);
}

static void
update_digit_width (MooTextView *view)
{
    int i;
    char str[32];
    PangoLayout *layout;

    layout = create_line_numbers_layout (view);
    view->priv->lm.digit_width = 0;
    view->priv->lm.numbers_width = 0;

    for (i = 0; i < 10; ++i)
    {
        int width;
        g_snprintf (str, sizeof str, "%d", i);
        pango_layout_set_text (layout, str, -1);
        pango_layout_get_pixel_size (layout, &width, NULL);
        view->priv->lm.digit_width = MAX (view->priv->lm.digit_width, width);
    }

    g_object_unref (layout);
}

static void
update_fold_width (MooTextView *view)
{
    gtk_widget_style_get (GTK_WIDGET (view), "expander-size",
                          &view->priv->lm.fold_width, NULL);
    view->priv->lm.fold_width += 2*EXPANDER_PAD;
}

static void
update_left_margin (MooTextView *view)
{
    int margin_size;
    int old_size;
    GtkTextView *text_view = GTK_TEXT_VIEW (view);

    if (!GTK_WIDGET_REALIZED (view))
        return;

    margin_size = 0;

    if (view->priv->lm.show_icons)
    {
        update_line_mark_icons (view);
        margin_size += view->priv->lm.icon_width + MARK_ICON_LPAD + MARK_ICON_RPAD;
    }

    if (view->priv->lm.show_numbers)
    {
        update_digit_width (view);
        view->priv->lm.numbers_width = view->priv->lm.digit_width * get_n_digits (view);
        margin_size += view->priv->lm.numbers_width + LINE_NUMBER_LPAD + LINE_NUMBER_RPAD;
    }

    if (view->priv->lm.show_folds)
    {
        update_fold_width (view);
        margin_size += view->priv->lm.fold_width;
    }

    old_size = get_border_window_size (text_view, GTK_TEXT_WINDOW_LEFT);
    gtk_text_view_set_border_window_size (text_view,
                                          GTK_TEXT_WINDOW_LEFT,
                                          margin_size);

    if (old_size == 0 && margin_size != 0)
    {
        GdkWindow *window = gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_LEFT);
        GdkCursor *cursor = gdk_cursor_new (GDK_RIGHT_PTR);
        gdk_window_set_cursor (window, cursor);
        gdk_cursor_unref (cursor);
    }

    /* XXX do not invalidate whole widget */
    gtk_widget_queue_draw (GTK_WIDGET (view));
}


static gboolean
update_n_lines_idle (MooTextView *view)
{
    int n_lines;

    view->priv->update_n_lines_idle = 0;

    n_lines = gtk_text_buffer_get_line_count (get_buffer (view));

    if (n_lines != view->priv->n_lines)
    {
        view->priv->n_lines = n_lines;
        update_left_margin (view);
    }

    return FALSE;
}

static void
buffer_changed (MooTextView *view)
{
    if (GTK_WIDGET_REALIZED (view) &&
        view->priv->lm.show_numbers &&
        !view->priv->update_n_lines_idle)
    {
        view->priv->update_n_lines_idle =
            g_idle_add_full (G_PRIORITY_HIGH,
                                       (GSourceFunc) update_n_lines_idle,
                                       view, NULL);
    }
}


static void
draw_marks (MooTextView    *view,
            GdkEventExpose *event,
            GSList         *marks,
            int             line_y,
            int             line_height)
{
    g_return_if_fail (marks != NULL);

    while (marks)
    {
        MooLineMark *mark;
        GdkPixbuf *pixbuf;
        const char *markup;

        mark = marks->data;
        marks = marks->next;

        if (!_moo_line_mark_get_pretty (mark))
            continue;

        pixbuf = moo_line_mark_get_pixbuf (mark);

        if (pixbuf)
        {
            int pw, ph;
            int x, y;

            pw = gdk_pixbuf_get_width (pixbuf);
            ph = gdk_pixbuf_get_height (pixbuf);
            x = MARK_ICON_LPAD + (view->priv->lm.icon_width - pw) / 2;
            y = line_y + (line_height - ph) / 2;

            gdk_draw_pixbuf (event->window, NULL, pixbuf,
                             0, 0, x, y, -1, -1,
                             GDK_RGB_DITHER_NORMAL, 0, 0);
        }

        markup = moo_line_mark_get_markup (mark);

        if (markup)
        {
            PangoLayout *layout;
            PangoRectangle rect;
            int x, y;

            layout = gtk_widget_create_pango_layout (GTK_WIDGET (view), NULL);
            pango_layout_set_markup (layout, markup, -1);
            pango_layout_get_pixel_extents (layout, &rect, NULL);
            x = MARK_ICON_LPAD + (view->priv->lm.icon_width - rect.width) / 2;
            y = line_y;

            gdk_draw_layout (event->window,
                             GTK_WIDGET(view)->style->fg_gc[GTK_WIDGET_STATE(view)],
                             x, y, layout);

            g_object_unref (layout);
        }
    }
}

static void
draw_fold_mark (MooTextView    *view,
                GdkEventExpose *event,
                MooFold        *fold,
                int             y,
                int             height,
                int             window_width)
{
    gtk_paint_expander (GTK_WIDGET(view)->style,
                        event->window,
                        GTK_WIDGET_STATE (view),
                        &event->area,
                        GTK_WIDGET(view),
                        "fold",
                        window_width - view->priv->lm.fold_width / 2,
                        y + height / 2,
                        fold->collapsed ? GTK_EXPANDER_COLLAPSED : GTK_EXPANDER_EXPANDED);
}

static void
draw_left_margin (MooTextView    *view,
                  GdkEventExpose *event)
{
    GtkTextView *text_view;
    GtkTextBuffer *buffer;
    PangoLayout *layout = NULL;
    int line, current_line, text_width, window_width, mark_icon_width;
    GdkRectangle area;
    GtkTextIter iter;
    char str[32];

    text_view = GTK_TEXT_VIEW (view);
    buffer = gtk_text_view_get_buffer (text_view);
    window_width = gdk_window_get_width (event->window);
    text_width = 0;
    mark_icon_width = 0;

    if (view->priv->lm.show_numbers)
    {
        layout = create_line_numbers_layout (view);
        text_width = view->priv->lm.numbers_width;
    }

    if (view->priv->lm.show_icons)
        mark_icon_width = view->priv->lm.icon_width + MARK_ICON_LPAD + MARK_ICON_RPAD;

    gtk_text_buffer_get_iter_at_mark (buffer, &iter, gtk_text_buffer_get_insert (buffer));
    current_line = gtk_text_iter_get_line (&iter);

    area = event->area;
    gtk_text_view_window_to_buffer_coords (text_view,
                                           GTK_TEXT_WINDOW_LEFT,
                                           area.x, area.y,
                                           &area.x, &area.y);

    gtk_text_view_get_line_at_y (text_view, &iter, area.y, NULL);
    line = gtk_text_iter_get_line (&iter);

    while (TRUE)
    {
        int y, height;

        gtk_text_view_get_line_yrange (text_view, &iter, &y, &height);

        if (y > area.y + area.height)
            break;

        gtk_text_view_buffer_to_window_coords (text_view, GTK_TEXT_WINDOW_LEFT,
                                               0, y, NULL, &y);

        if (view->priv->lm.show_numbers)
        {
            int x, w;

            if (line == current_line)
                g_snprintf (str, sizeof str, "<b>%d</b>", line + 1);
            else
                g_snprintf (str, sizeof str, "%d", line + 1);

            pango_layout_set_markup (layout, str, -1);
            pango_layout_get_pixel_size (layout, &w, NULL);
            x = mark_icon_width + LINE_NUMBER_LPAD + text_width - w;

            gtk_paint_layout (GTK_WIDGET (view)->style,
                              event->window,
                              GTK_WIDGET_STATE (view),
                              FALSE, &event->area,
                              GTK_WIDGET(view), NULL,
                              x, y, layout);
        }

        if (view->priv->lm.show_folds)
        {
            MooFold *fold = moo_text_buffer_get_fold_at_line (get_moo_buffer (view), line);

            if (fold)
                draw_fold_mark (view, event, fold, y, height, window_width);
        }

        if (view->priv->lm.show_icons)
        {
            GSList *marks = moo_text_buffer_get_line_marks_at_line (get_moo_buffer (view), line);

            if (marks)
                draw_marks (view, event, marks, y, height);

            g_slist_free (marks);
        }

        if (!text_iter_forward_visible_line (view, &iter, &line))
            break;
    }

    if (layout)
        g_object_unref (layout);
}


static void
draw_fold_background (MooTextView    *view,
                      GdkEventExpose *event,
                      MooFold        *fold,
                      int             y,
                      int             height,
                      int             window_width)
{
    if (fold->collapsed)
        gdk_draw_line (event->window,
                       GTK_WIDGET(view)->style->text_gc[GTK_STATE_NORMAL],
                       gtk_text_view_get_left_margin (GTK_TEXT_VIEW (view)),
                       y + height - 1,
                       gtk_text_view_get_left_margin (GTK_TEXT_VIEW (view)) + window_width,
                       y + height - 1);
}

static void
draw_marks_background (MooTextView    *view,
                       GdkEventExpose *event)
{
    GtkTextView *text_view;
    int line, window_width;
    GdkRectangle area;
    GtkTextIter iter;
    cairo_t* cr = NULL;

    text_view = GTK_TEXT_VIEW (view);

    area = event->area;
    gtk_text_view_window_to_buffer_coords (text_view,
                                           GTK_TEXT_WINDOW_TEXT,
                                           area.x, area.y,
                                           &area.x, &area.y);

    window_width = gdk_window_get_width (event->window);
    window_width -= gtk_text_view_get_left_margin (text_view);

    gtk_text_view_get_line_at_y (text_view, &iter, area.y, NULL);
    line = gtk_text_iter_get_line (&iter);

    while (TRUE)
    {
        int y, height;

        gtk_text_view_get_line_yrange (text_view, &iter, &y, &height);

        if (y > area.y + area.height)
            break;

        gtk_text_view_buffer_to_window_coords (text_view, GTK_TEXT_WINDOW_TEXT,
                                               0, y, NULL, &y);

        if (view->priv->enable_folding)
        {
            MooFold *fold = moo_text_buffer_get_fold_at_line (get_moo_buffer (view), line);

            if (fold)
                draw_fold_background (view, event, fold, y, height, window_width);
        }

        if (TRUE)
        {
            MooLineMark *mark;
            const GdkColor *color = NULL;
            GSList *marks = moo_text_buffer_get_line_marks_at_line (get_moo_buffer (view), line);

            if (marks)
            {
                while (marks)
                {
                    mark = marks->data;

                    if (_moo_line_mark_get_pretty (mark))
                        color = moo_line_mark_get_background (mark);

                    if (!color)
                        marks = g_slist_delete_link (marks, marks);
                    else
                        break;
                }

                /* XXX compose colors */
                if (color)
                {
                    if (!cr)
                        cr = gdk_cairo_create(event->window);

                    gdk_cairo_set_source_color (cr, color);
                    cairo_rectangle(cr, gtk_text_view_get_left_margin(text_view),
                                        y,
                                        window_width,
                                        height);
                    cairo_fill (cr);
                }
            }

            g_slist_free (marks);
        }

        if (!text_iter_forward_visible_line (view, &iter, &line))
            break;
    }

    if (cr)
        cairo_destroy(cr);
}


void
moo_text_view_set_show_line_numbers (MooTextView *view,
                                     gboolean     show)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    show = show != 0;

    if (view->priv->lm.show_numbers != show)
    {
        view->priv->lm.show_numbers = show;
        update_left_margin (view);
        g_object_notify (G_OBJECT (view), "show-line-numbers");
    }
}


void
_moo_text_view_set_line_numbers_font (MooTextView *view,
                                      const char  *name)
{
    PangoFontDescription *font;

    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    if (name)
        font = pango_font_description_from_string (name);
    else
        font = NULL;

    if (font == view->priv->lm.numbers_font ||
        (font != NULL && view->priv->lm.numbers_font != NULL &&
         pango_font_description_equal (font, view->priv->lm.numbers_font)))
    {
        pango_font_description_free (font);
        return;
    }

    if (view->priv->lm.numbers_font)
        pango_font_description_free (view->priv->lm.numbers_font);

    view->priv->lm.numbers_font = font;

    update_left_margin (view);
}


static void
set_show_line_marks (MooTextView *view,
                     gboolean     show)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    show = show != 0;

    if (view->priv->lm.show_icons != show)
    {
        view->priv->lm.show_icons = show;
        update_left_margin (view);
        g_object_notify (G_OBJECT (view), "show-line-marks");
    }
}


static void
moo_text_view_style_set (GtkWidget *widget,
                         GtkStyle  *prev_style)
{
    MooTextView *view = MOO_TEXT_VIEW (widget);

    invalidate_gcs (view);
    invalidate_right_margin (view);
    update_tab_width (view);
    update_left_margin (view);

    GTK_WIDGET_CLASS(moo_text_view_parent_class)->style_set (widget, prev_style);
}


static void
set_enable_folding (MooTextView *view,
                    gboolean     show)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    show = show != 0;

    if (show == view->priv->enable_folding)
        return;

    if (show && !moo_text_view_lookup_tag (view, MOO_FOLD_TAG))
        gtk_text_buffer_create_tag (get_buffer (view), MOO_FOLD_TAG,
                                    "invisible", TRUE, NULL);

    view->priv->lm.show_folds = show;
    view->priv->enable_folding = show;

    update_left_margin (view);

    g_object_notify (G_OBJECT (view), "enable-folding");
}


static void
update_tab_width (MooTextView *view)
{
    PangoTabArray *tabs;
    PangoLayout *layout;
    int tab_width;
    char *string;

    if (!GTK_WIDGET_REALIZED (view))
        return;

    g_return_if_fail (view->priv->tab_width > 0);
    g_return_if_fail (GTK_WIDGET (view)->style != NULL);

    string = g_strnfill (view->priv->tab_width, ' ');
    layout = gtk_widget_create_pango_layout (GTK_WIDGET (view), string);
    pango_layout_get_size (layout, &tab_width, NULL);

    tabs = pango_tab_array_new (2, FALSE);
    pango_tab_array_set_tab (tabs, 0, PANGO_TAB_LEFT, 0);
    pango_tab_array_set_tab (tabs, 1, PANGO_TAB_LEFT, tab_width);

    gtk_text_view_set_tabs (GTK_TEXT_VIEW (view), tabs);

    pango_tab_array_free (tabs);
    g_object_unref (layout);
    g_free (string);
}

void
moo_text_view_set_tab_width (MooTextView *view,
                             guint        width)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    g_return_if_fail (width > 0);

    if (width == view->priv->tab_width)
        return;

    view->priv->tab_width = width;
    update_tab_width (view);

    g_object_notify (G_OBJECT (view), "tab-width");
}


static void
invalidate_right_margin (MooTextView *view)
{
    view->priv->right_margin_pixel_offset = -1;
}

static void
update_right_margin (MooTextView *view)
{
    PangoLayout *layout;
    char *string;

    if (view->priv->right_margin_pixel_offset > 0 ||
        !GTK_WIDGET_REALIZED (view))
            return;

    g_return_if_fail (view->priv->right_margin_offset > 0 &&
                      view->priv->right_margin_offset < 1000);

    string = g_strnfill (view->priv->right_margin_offset, '_');
    layout = gtk_widget_create_pango_layout (GTK_WIDGET (view), string);
    pango_layout_get_pixel_size (layout, &view->priv->right_margin_pixel_offset, NULL);

    g_object_unref (layout);
    g_free (string);
}


static gboolean
text_iter_forward_visible_line (MooTextView *view,
                                GtkTextIter *iter,
                                int         *line)
{
    if (!view->priv->enable_folding)
    {
        if (!gtk_text_iter_forward_line (iter))
        {
            if (gtk_text_iter_get_line (iter) == *line)
                return FALSE;
        }

        *line += 1;
        return TRUE;
    }
    else
    {
        GtkTextTagTable *table = gtk_text_buffer_get_tag_table (get_buffer (view));
        GtkTextTag *tag = gtk_text_tag_table_lookup (table, MOO_FOLD_TAG);

        g_return_val_if_fail (tag != NULL, FALSE);

        while (TRUE)
        {
            if (!gtk_text_iter_forward_line (iter))
            {
                if (gtk_text_iter_get_line (iter) == *line)
                    return FALSE;
            }

            *line += 1;

            if (!gtk_text_iter_has_tag (iter, tag) && !gtk_text_iter_begins_tag (iter, tag))
                return TRUE;
        }
    }
}


static void
invalidate_line (MooTextView *view,
                 int          line,
                 gboolean     left,
                 gboolean     text)
{
    GtkTextIter iter;
    GdkRectangle rect = {0, 0, 0, 0};

    if (!GTK_WIDGET_DRAWABLE (view))
        return;

    gtk_text_buffer_get_iter_at_line (get_buffer (view), &iter, line);
    gtk_text_view_get_line_yrange (GTK_TEXT_VIEW (view), &iter, &rect.y, &rect.height);

    gtk_text_view_buffer_to_window_coords (GTK_TEXT_VIEW (view),
                                           GTK_TEXT_WINDOW_TEXT,
                                           0, rect.y, NULL, &rect.y);

    if (left)
    {
        GdkWindow *window = gtk_text_view_get_window (GTK_TEXT_VIEW (view),
                                                      GTK_TEXT_WINDOW_LEFT);
        if (window)
        {
            rect.width = gdk_window_get_width (window);
            gdk_window_invalidate_rect (window, &rect, FALSE);
        }
    }

    if (text)
    {
        GdkWindow *window = gtk_text_view_get_window (GTK_TEXT_VIEW (view),
                                                      GTK_TEXT_WINDOW_TEXT);
        if (window)
        {
            rect.width = gdk_window_get_width (window);
            gdk_window_invalidate_rect (window, &rect, FALSE);
        }
    }
}


static void
add_line_mark (MooTextView *view,
               MooLineMark *mark)
{
    view->priv->line_marks = g_slist_prepend (view->priv->line_marks,
                                              g_object_ref (mark));
    g_signal_connect_swapped (mark, "changed",
                              G_CALLBACK (line_mark_changed), view);
}

static void
remove_line_mark (MooTextView *view,
                  MooLineMark *mark)
{
    if (g_slist_find (view->priv->line_marks, mark))
    {
        view->priv->line_marks = g_slist_remove (view->priv->line_marks, mark);
        g_signal_handlers_disconnect_by_func (mark, (gpointer) line_mark_changed, view);
        g_object_unref (mark);
    }
}


static void
line_mark_deleted (MooTextView *view,
                   MooLineMark *mark)
{
    if (_moo_line_mark_get_pretty (mark))
        gtk_widget_queue_draw (GTK_WIDGET (view));

    remove_line_mark (view, mark);
}


static void
line_mark_changed (MooTextView *view,
                   MooLineMark *mark)
{
    invalidate_line (view, moo_line_mark_get_line (mark), TRUE, TRUE);
}


static void
line_mark_moved (MooTextView *view,
                 MooLineMark *mark)
{
    /* XXX */
    if (_moo_line_mark_get_pretty (mark))
        gtk_widget_queue_draw (GTK_WIDGET (view));
}


static void
line_mark_added (MooTextView *view,
                 MooLineMark *mark)
{
    g_return_if_fail (!g_slist_find (view->priv->line_marks, mark));

    add_line_mark (view, mark);

    if (!moo_line_mark_get_visible (mark))
        return;

    _moo_line_mark_set_pretty (mark, TRUE);

    if (GTK_WIDGET_REALIZED (view))
        _moo_line_mark_realize (mark, GTK_WIDGET (view));

    invalidate_line (view, moo_line_mark_get_line (mark), TRUE, TRUE);
}


static void
fold_added (MooTextView *view,
            MooFold     *fold)
{
    if (view->priv->enable_folding)
        invalidate_line (view, _moo_fold_get_start (fold), TRUE, fold->collapsed);
}


static void
fold_deleted (MooTextView *view)
{
    if (view->priv->enable_folding)
        gtk_widget_queue_draw (GTK_WIDGET (view));
}


static void
fold_toggled (MooTextView        *view,
              MooFold            *fold)
{
    if (view->priv->enable_folding)
    {
        if (fold)
            invalidate_line (view, _moo_fold_get_start (fold), TRUE, TRUE);
        else
            gtk_widget_queue_draw (GTK_WIDGET (view));
    }
}


/***************************************************************************/
/* Children
 */

/* http://bugzilla.gnome.org/show_bug.cgi?id=323843 */
static int
get_border_window_size (GtkTextView      *text_view,
                        GtkTextWindowType type)
{
    if (GTK_WIDGET_REALIZED (text_view) && gtk_text_view_get_window (text_view, type))
        return gtk_text_view_get_border_window_size (text_view, type);
    else
        return 0;
}


void
moo_text_view_add_child_in_border (MooTextView        *view,
                                   GtkWidget          *widget,
                                   GtkTextWindowType   which_border)
{
    MooTextViewPos pos = MOO_TEXT_VIEW_POS_INVALID;
    GtkWidget **child;
    GtkRequisition child_req = {0, 0};
    int border_size = 0;

    g_return_if_fail (MOO_IS_TEXT_VIEW (view));
    g_return_if_fail (GTK_IS_WIDGET (widget));
    g_return_if_fail (widget->parent == NULL);

    switch (which_border)
    {
        case GTK_TEXT_WINDOW_LEFT:
            pos = MOO_TEXT_VIEW_POS_LEFT;
            break;
        case GTK_TEXT_WINDOW_RIGHT:
            pos = MOO_TEXT_VIEW_POS_RIGHT;
            break;
        case GTK_TEXT_WINDOW_TOP:
            pos = MOO_TEXT_VIEW_POS_TOP;
            break;
        case GTK_TEXT_WINDOW_BOTTOM:
            pos = MOO_TEXT_VIEW_POS_BOTTOM;
            break;
        default:
            g_return_if_reached ();
    }

    g_return_if_fail (pos < MOO_TEXT_VIEW_POS_INVALID);

    child = &view->priv->children[pos];
    g_return_if_fail (*child == NULL);

    g_object_ref_sink (widget);

    *child = widget;

    if (GTK_WIDGET_VISIBLE (widget))
    {
        gtk_widget_size_request (widget, &child_req);

        switch (which_border)
        {
            case GTK_TEXT_WINDOW_LEFT:
            case GTK_TEXT_WINDOW_RIGHT:
                border_size = child_req.width;
                break;
            case GTK_TEXT_WINDOW_TOP:
            case GTK_TEXT_WINDOW_BOTTOM:
                border_size = child_req.height;
                break;
            default:
                g_assert_not_reached ();
        }

        gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (view),
                                              which_border, MIN (1, border_size));
        lower_border_window (GTK_TEXT_VIEW (view), pos);
    }

    gtk_text_view_add_child_in_window (GTK_TEXT_VIEW (view), widget,
                                       GTK_TEXT_WINDOW_WIDGET, 0, 0);
}


static void
moo_text_view_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
    guint i;
    MooTextView *view;
    GtkTextView *text_view;

    view = MOO_TEXT_VIEW (widget);
    text_view = GTK_TEXT_VIEW (widget);

    for (i = 0; i < 4; i++)
    {
        int border_size = 0;
        GtkWidget *child = view->priv->children[i];
        GtkRequisition child_req;

        if (child && GTK_WIDGET_VISIBLE (child))
            gtk_widget_size_request (child, &child_req);
        else
            child_req.width = child_req.height = 0;

        if (child)
        {
            int old_size;

            switch (i)
            {
                case MOO_TEXT_VIEW_POS_LEFT:
                case MOO_TEXT_VIEW_POS_RIGHT:
                    border_size = child_req.width;
                    break;
                case MOO_TEXT_VIEW_POS_TOP:
                case MOO_TEXT_VIEW_POS_BOTTOM:
                    border_size = child_req.height;
                    break;
            }

            old_size = get_border_window_size (text_view,
                                               window_types[i]);
            gtk_text_view_set_border_window_size (text_view,
                                                  window_types[i],
                                                  border_size);
            if (!old_size)
                lower_border_window (GTK_TEXT_VIEW (view), i);
        }
    }

    GTK_WIDGET_CLASS(moo_text_view_parent_class)->size_request (widget, requisition);
}


static void
moo_text_view_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
    guint i;
    int right, left, bottom, top, border_width;
    MooTextView *view;
    GtkTextView *text_view;

    view = MOO_TEXT_VIEW (widget);
    text_view = GTK_TEXT_VIEW (widget);

    GTK_WIDGET_CLASS(moo_text_view_parent_class)->size_allocate (widget, allocation);

    border_width = GTK_CONTAINER(widget)->border_width;

    right = get_border_window_size (text_view, GTK_TEXT_WINDOW_RIGHT);
    left = get_border_window_size (text_view, GTK_TEXT_WINDOW_LEFT);
    bottom = get_border_window_size (text_view, GTK_TEXT_WINDOW_BOTTOM);
    top = get_border_window_size (text_view, GTK_TEXT_WINDOW_TOP);

    for (i = 0; i < 4; i++)
    {
        GtkWidget *child = view->priv->children[i];
        GtkAllocation child_alloc = {0, 0, 0, 0};
        GtkRequisition child_req;

        if (!child || !GTK_WIDGET_VISIBLE (child))
            continue;

        child_alloc.x = left + border_width;
        child_alloc.y = top + border_width;
        gtk_widget_get_child_requisition (child, &child_req);

        switch (i)
        {
            case MOO_TEXT_VIEW_POS_RIGHT:
                child_alloc.x = MAX (allocation->width - border_width - right, 0);
                // fallthrough
            case MOO_TEXT_VIEW_POS_LEFT:
                child_alloc.width = child_req.width;
                child_alloc.height = MAX (allocation->height - 2*border_width - top - bottom, 1);
                break;

            case MOO_TEXT_VIEW_POS_BOTTOM:
                child_alloc.y = MAX (allocation->height - bottom - border_width, 0);
                // fallthrough
            case MOO_TEXT_VIEW_POS_TOP:
                child_alloc.height = child_req.height;
                child_alloc.width = MAX (allocation->width - 2*border_width - left - right, 1);
                break;
        }

        gtk_text_view_move_child (text_view, child, child_alloc.x, child_alloc.y);
        gtk_widget_size_allocate (child, &child_alloc);
    }
}


static void
moo_text_view_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
    guint i;
    MooTextView *view = MOO_TEXT_VIEW (container);

    if (widget == view->priv->qs.evbox)
    {
        view->priv->qs.in_search = FALSE;
        view->priv->qs.evbox = NULL;
        view->priv->qs.entry = NULL;
        view->priv->qs.case_sensitive = NULL;
        view->priv->qs.regex = NULL;
    }

    for (i = 0; i < 4; ++i)
    {
        if (view->priv->children[i] == widget)
        {
            view->priv->children[i] = NULL;
            g_object_unref (widget);
            gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (view),
                                                  window_types[i], 0);
            break;
        }
    }

    GTK_CONTAINER_CLASS(moo_text_view_parent_class)->remove (container, widget);
}


/***************************************************************************/
/* Search
 */

static void
quick_search_set_widgets_from_flags (MooTextView *view);

static void
moo_text_view_set_quick_search_flags (MooTextView        *view,
                                      MooTextSearchFlags  flags)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    if (flags != view->priv->qs.flags)
    {
        view->priv->qs.flags = flags;

        if (view->priv->qs.evbox)
            quick_search_set_widgets_from_flags (view);

        g_object_notify (G_OBJECT (view), "quick-search-flags");
    }
}


static void
quick_search_option_toggled (MooTextView *view)
{
    MooTextSearchFlags flags = 0;

    if (!gtk_toggle_button_get_active (view->priv->qs.case_sensitive))
        flags |= MOO_TEXT_SEARCH_CASELESS;
    if (gtk_toggle_button_get_active (view->priv->qs.regex))
        flags |= MOO_TEXT_SEARCH_REGEX;

    moo_text_view_set_quick_search_flags (view, flags);

    if (MOO_IS_EDIT_VIEW (view))
        moo_prefs_set_int (moo_edit_setting (MOO_EDIT_PREFS_QUICK_SEARCH_FLAGS), flags);
}


static void
quick_search_set_widgets_from_flags (MooTextView *view)
{
    g_signal_handlers_block_by_func (view->priv->qs.case_sensitive,
                                     quick_search_option_toggled, view);
    g_signal_handlers_block_by_func (view->priv->qs.regex,
                                     quick_search_option_toggled, view);

    gtk_toggle_button_set_active (view->priv->qs.case_sensitive,
                                  !(view->priv->qs.flags & MOO_TEXT_SEARCH_CASELESS));
    gtk_toggle_button_set_active (view->priv->qs.regex,
                                  view->priv->qs.flags & MOO_TEXT_SEARCH_REGEX);

    g_signal_handlers_unblock_by_func (view->priv->qs.case_sensitive,
                                       quick_search_option_toggled, view);
    g_signal_handlers_unblock_by_func (view->priv->qs.regex,
                                       quick_search_option_toggled, view);
}


static void
scroll_selection_onscreen (GtkTextView *text_view)
{
    GtkTextIter iter;
    GtkTextBuffer *buffer;
    GdkRectangle rect, visible_rect;

    buffer = gtk_text_view_get_buffer (text_view);
    gtk_text_buffer_get_iter_at_mark (buffer, &iter,
                                      gtk_text_buffer_get_selection_bound (buffer));
    gtk_text_view_scroll_to_iter (text_view, &iter, 0.0, FALSE, 0.0, 0.0);

    gtk_text_buffer_get_iter_at_mark (buffer, &iter,
                                      gtk_text_buffer_get_insert (buffer));

    gtk_text_view_get_iter_location (text_view, &iter, &rect);
    gtk_text_view_get_visible_rect (text_view, &visible_rect);

    if (rect.x < visible_rect.x || rect.y < visible_rect.y ||
        rect.x + rect.width > visible_rect.x + visible_rect.width ||
        rect.y + rect.height > visible_rect.y + visible_rect.height)
            gtk_text_view_scroll_to_iter (text_view, &iter, 0.0, FALSE, 0.0, 0.0);
}


static void
quick_search_message (MooTextView *view,
                      const char  *msg)
{
    moo_text_view_message (view, msg);
}


static void
quick_search_find_from (MooTextView *view,
                        const char  *text,
                        GtkTextIter *start)
{
    gboolean found;
    GtkTextIter match_start, match_end;
    GtkTextBuffer *buffer;

    if (view->priv->qs.flags & MOO_TEXT_SEARCH_REGEX)
    {
        GError *error = NULL;
        GRegex *re = g_regex_new (text, 0, 0, &error);

        if (!re)
        {
            char *msg = g_strdup_printf ("Invalid pattern '%s'", text);
            quick_search_message (view, msg);
            g_free (msg);
            return;
        }

        g_regex_unref (re);
    }

    buffer = get_buffer (view);
    found = moo_text_search_forward (start, text, view->priv->qs.flags,
                                     &match_start, &match_end, NULL);

    if (!found)
    {
        GtkTextIter iter;

        gtk_text_buffer_get_start_iter (buffer, &iter);

        if (!gtk_text_iter_equal (start, &iter))
            found = moo_text_search_forward (&iter, text, view->priv->qs.flags,
                                             &match_start, &match_end, NULL);
    }

    if (found)
    {
        gtk_text_buffer_select_range (buffer, &match_end, &match_start);
        scroll_selection_onscreen (GTK_TEXT_VIEW (view));
    }
    else
    {
        char *message;

        if (view->priv->qs.flags & MOO_TEXT_SEARCH_REGEX)
            message = g_strdup_printf ("Pattern '%s' not found", text);
        else
            message = g_strdup_printf ("Text '%s' not found", text);

        quick_search_message (view, message);
        g_free (message);
    }
}


static void
quick_search_find (MooTextView *view,
                   const char  *text)
{
    GtkTextIter iter1, iter2, insert;
    GtkTextBuffer *buffer = get_buffer (view);

    if (gtk_text_buffer_get_selection_bounds (buffer, &iter1, &iter2))
    {
        gtk_text_buffer_get_iter_at_mark (buffer, &insert, gtk_text_buffer_get_insert (buffer));

        if (gtk_text_iter_equal (&iter1, &insert))
        {
            gtk_text_buffer_place_cursor (buffer, &insert);
            quick_search_find_from (view, text, &insert);
            return;
        }
    }

    quick_search_find_from (view, text, &iter1);
}


static void
quick_search_find_next (MooTextView *view,
                        GtkEntry    *entry,
                        gboolean     from_start)
{
    const char *text;
    GtkTextIter start;
    GtkTextBuffer *buffer = get_buffer (view);

    text = gtk_entry_get_text (entry);

    if (text[0])
    {
        if (from_start)
            gtk_text_buffer_get_start_iter (buffer, &start);
        else
            gtk_text_buffer_get_iter_at_mark (buffer, &start, gtk_text_buffer_get_insert (buffer));
        quick_search_find_from (view, text, &start);
    }
}


static void
search_entry_changed (MooTextView *view,
                      GtkEntry    *entry)
{
    const char *text;

    if (!view->priv->qs.in_search)
        return;

    text = gtk_entry_get_text (entry);

    if (text[0])
        quick_search_find (view, text);
}


static gboolean
search_entry_focus_out (MooTextView *view)
{
    moo_text_view_stop_quick_search (view);
    return FALSE;
}


static gboolean
search_entry_key_press (MooTextView *view,
                        GdkEventKey *event,
                        GtkEntry    *entry)
{
    if (!view->priv->qs.in_search)
        return FALSE;

    switch (event->keyval)
    {
        case GDK_Escape:
            moo_text_view_stop_quick_search (view);
            return TRUE;

        case GDK_Return:
        case GDK_KP_Enter:
            quick_search_find_next (view, entry,
                                    event->state & GDK_CONTROL_MASK);
            return TRUE;
    }

    return FALSE;
}


static void
moo_text_view_start_quick_search (MooTextView *view)
{
    char *text = NULL;
    GtkTextIter iter1, iter2;
    GtkTextBuffer *buffer;

    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    if (view->priv->qs.in_search)
        return;

    if (!view->priv->qs.entry)
    {
        QsBoxXml *xml;

        xml = qs_box_xml_new ();

        view->priv->qs.evbox = GTK_WIDGET (xml->QsBox);
        g_return_if_fail (view->priv->qs.evbox != NULL);

        view->priv->qs.entry = GTK_WIDGET (xml->entry);
        view->priv->qs.case_sensitive = GTK_TOGGLE_BUTTON (xml->case_sensitive);
        view->priv->qs.regex = GTK_TOGGLE_BUTTON (xml->regex);

        g_signal_connect_swapped (view->priv->qs.entry, "changed",
                                  G_CALLBACK (search_entry_changed), view);
        g_signal_connect_swapped (view->priv->qs.entry, "focus-out-event",
                                  G_CALLBACK (search_entry_focus_out), view);
        g_signal_connect_swapped (view->priv->qs.entry, "key-press-event",
                                  G_CALLBACK (search_entry_key_press), view);

        g_signal_connect_swapped (view->priv->qs.case_sensitive, "toggled",
                                  G_CALLBACK (quick_search_option_toggled), view);
        g_signal_connect_swapped (view->priv->qs.regex, "toggled",
                                  G_CALLBACK (quick_search_option_toggled), view);
        quick_search_set_widgets_from_flags (view);

        moo_text_view_add_child_in_border (view, view->priv->qs.evbox,
                                           GTK_TEXT_WINDOW_BOTTOM);
    }

    buffer = get_buffer (view);

    if (gtk_text_buffer_get_selection_bounds (buffer, &iter1, &iter2) &&
        gtk_text_iter_get_line (&iter1) == gtk_text_iter_get_line (&iter2))
    {
        text = gtk_text_buffer_get_slice (buffer, &iter1, &iter2, TRUE);
    }

    if (text)
        gtk_entry_set_text (GTK_ENTRY (view->priv->qs.entry), text);

    gtk_widget_show (view->priv->qs.evbox);
    gtk_widget_grab_focus (view->priv->qs.entry);

    view->priv->qs.in_search = TRUE;

    g_free (text);
}


static void
moo_text_view_stop_quick_search (MooTextView *view)
{
    g_return_if_fail (MOO_IS_TEXT_VIEW (view));

    if (view->priv->qs.in_search)
    {
        view->priv->qs.in_search = FALSE;
        gtk_widget_hide (view->priv->qs.evbox);
        gtk_widget_grab_focus (GTK_WIDGET (view));
    }
}


static gboolean
start_quick_search (MooTextView *view)
{
    if (view->priv->qs.enable)
    {
        moo_text_view_start_quick_search (view);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
