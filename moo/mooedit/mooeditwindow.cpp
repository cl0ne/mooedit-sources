/*
 *   mooeditwindow.c
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
 * class:MooEditWindow: (parent MooWindow) (moo.doc-object-name window): document window object
 **/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mooedit/mooedit-impl.h"
#include "mooedit/mooeditdialogs.h"
#include "mooedit/mooeditwindow-impl.h"
#include "mooedit/mooedit-accels.h"
#include "mooedit/mooeditor-impl.h"
#include "mooedit/mooeditview-impl.h"
#include "mooedit/mooedittab-impl.h"
#include "mooedit/mooeditfiltersettings.h"
#include "mooedit/moolang.h"
#include "mooedit/mootextbuffer.h"
#include "mooedit/mooeditprefs.h"
#include "mooedit/mooplugin.h"
#include "mooedit/mooeditaction.h"
#include "mooedit/mooeditbookmark.h"
#include "mooedit/moolangmgr.h"
#include "mooutils/moonotebook.h"
#include "mooutils/moostock.h"
#include "marshals.h"
#include "mooutils/moomenuaction.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/moodialogs.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooaction-private.h"
#include "mooutils/moofiledialog.h"
#include "mooutils/mooencodings.h"
#include "mooutils/moocompat.h"
#include "mooutils/mooutils-enums.h"
#include "mooedit/moostatusbar-gxml.h"
#include "moocpp/gobjptr.h"
#include <string.h>
#include <gtk/gtk.h>
#include <math.h>

#include <string>
#include <unordered_set>

#define ENABLE_PRINTING
#include "mooedit/mootextprint.h"

#define ACTIVE_DOC moo_edit_window_get_active_doc
#define ACTIVE_VIEW moo_edit_window_get_active_view
#define ACTIVE_TAB moo_edit_window_get_active_tab

#define LANG_ACTION_ID "LanguageMenu"
#define STOP_ACTION_ID "StopJob"

#define DOCUMENT_ACTION "SwitchToTab"

namespace {

const std::string DEFAULT_TITLE_FORMAT("%a - %f%s");
const std::string DEFAULT_TITLE_FORMAT_NO_DOC("%a");

} // namespace

#define PREFS_KEY_SPLIT_POS MOO_EDIT_PREFS_PREFIX "/window/splitpos"

typedef struct {
    MooActionCheckFunc func;
    gpointer data;
    GDestroyNotify notify;
} OnePropCheck;

#define N_ACTION_CHECKS 3
typedef struct {
    OnePropCheck checks[N_ACTION_CHECKS];
} ActionCheck;

static GHashTable *action_checks; /* char* -> ActionCheck* */
static std::unordered_set<MooEditWindow*> windows;

using MooNotebookPtr = ObjectPtr<MooNotebook>;

struct MooEditWindowPrivate {
    MooEditor *editor;

    guint statusbar_idle;
    guint last_msg_id;
    GtkLabel *cursor_label;
    GtkLabel *chars_label;
    GtkLabel *insert_label;
    GtkWidget *info;

    GtkWidget *doc_paned;
    std::vector<MooNotebookPtr> notebooks;
    MooEditTab *active_tab;
    guint save_params_idle;

    std::string title_format;
    std::string title_format_no_doc;

    GSList *stop_clients;
    GSList *jobs; /* Job* */

    GList *history;
    gboolean enable_history : 1;
    guint history_blocked : 1;
};

MOO_DEFINE_OBJECT_ARRAY (MooEditWindow, moo_edit_window)

enum {
    TARGET_MOO_EDIT_TAB = 1,
    TARGET_URI_LIST = 2
};

MOO_DEFINE_ATOM_GLOBAL (MOO_EDIT_TAB, moo_edit_tab)

static GtkTargetEntry dest_targets[] = {
    { (char*) MOO_EDIT_TAB_ATOM_NAME, GTK_TARGET_SAME_APP, TARGET_MOO_EDIT_TAB },
    { (char*) "text/uri-list", 0, TARGET_URI_LIST }
};


static void     action_checks_init              (void);
static void     moo_edit_window_check_actions   (MooEditWindow      *window);

static GObject *moo_edit_window_constructor     (GType               type,
                                                 guint               n_props,
                                                 GObjectConstructParam *props);
static void     moo_edit_window_finalize        (GObject            *object);
static void     moo_edit_window_destroy         (GtkObject          *object);

static void     moo_edit_window_set_property    (GObject            *object,
                                                 guint               prop_id,
                                                 const GValue       *value,
                                                 GParamSpec         *pspec);
static void     moo_edit_window_get_property    (GObject            *object,
                                                 guint               prop_id,
                                                 GValue             *value,
                                                 GParamSpec         *pspec);

static MooCloseResponse moo_edit_window_close_handler   (MooWindow          *window);

static MooCloseResponse moo_edit_window_before_close    (MooEditWindow      *window);

static void          queue_save_window_config           (MooEditWindow      *window);

static void          setup_notebook                     (MooEditWindow      *window,
                                                         MooNotebook        *notebook);
static int           get_active_tab                     (MooEditWindow      *window);
static MooNotebook  *get_notebook                       (MooEditWindow      *window,
                                                         int                 i);
static MooNotebook  *get_active_notebook                (MooEditWindow      *window);
static int           get_view_page_num                  (MooEditWindow      *window,
                                                         MooEditView        *view,
                                                         MooNotebook       **pnotebook);
static MooEditView  *get_notebook_active_view           (MooNotebook        *notebook);
static MooEdit      *get_notebook_active_doc            (MooNotebook        *notebook);
static void          save_doc_paned_config              (MooEditWindow      *window);
static void          show_notebook                      (MooEditWindow      *window,
                                                         MooNotebook        *notebook);
static void          move_tab_to_split_view             (MooEditWindow      *window,
                                                         MooEditTab         *tab);
static gboolean      can_move_to_split_notebook         (MooEditWindow      *window);
static gboolean      both_notebooks_visible             (MooEditWindow      *window);

static void          update_window_title                (MooEditWindow      *window);
static void          set_title_format_from_prefs        (MooEditWindow      *window);

static void          proxy_boolean_property             (MooEditWindow      *window,
                                                         GParamSpec         *prop,
                                                         MooEdit            *doc);
static void          edit_changed                       (MooEditWindow      *window,
                                                         MooEdit            *doc);
static void          edit_filename_changed              (MooEditWindow      *window,
                                                         MooEdit            *doc);
static void          edit_encoding_changed              (MooEditWindow      *window,
                                                         GParamSpec         *pspec,
                                                         MooEdit            *doc);
static void          edit_line_end_changed              (MooEditWindow      *window,
                                                         GParamSpec         *pspec,
                                                         MooEdit            *doc);
static void          edit_lang_changed                  (MooEditWindow      *window,
                                                         GParamSpec         *pspec,
                                                         MooEdit            *doc);
static void          view_overwrite_changed             (MooEditWindow      *window,
                                                         GParamSpec         *pspec,
                                                         MooEditView        *view);
static void          view_wrap_mode_changed             (MooEditWindow      *window,
                                                         GParamSpec         *pspec,
                                                         MooEditView        *view);
static void          view_show_line_numbers_changed     (MooEditWindow      *window,
                                                         GParamSpec         *pspec,
                                                         MooEditView        *view);
static GtkWidget    *create_tab_label                   (MooEditWindow      *window,
                                                         MooEditTab         *tab,
                                                         MooEdit            *doc);
static void          update_tab_labels                  (MooEditWindow      *window,
                                                         MooEdit            *doc);
static void          view_cursor_moved                  (MooEditWindow      *window,
                                                         GtkTextIter        *iter,
                                                         MooEditView        *view);
static void          update_lang_menu                   (MooEditWindow      *window);
static void          update_doc_view_actions            (MooEditWindow      *window);
static void          update_split_view_actions          (MooEditWindow      *window);

static void          create_statusbar                   (MooEditWindow      *window);
static void          update_statusbar                   (MooEditWindow      *window);
static MooEditTab   *get_nth_tab                        (MooNotebook        &notebook,
                                                         guint               n);
static MooEdit      *get_nth_doc                        (MooNotebook        *notebook,
                                                         guint               n);

static GtkAction    *create_lang_action                 (MooEditWindow      *window);

static void          create_paned                       (MooEditWindow      *window);
static void          save_paned_config                  (MooEditWindow      *window);

static void          moo_edit_window_connect_menubar    (MooWindow          *window);
static void          moo_edit_window_update_doc_list    (MooEditWindow      *window);
static void          window_menu_item_selected          (MooWindow          *window,
                                                         GtkMenuItem        *item);

static void          notebook_drag_data_recv            (GtkWidget          *widget,
                                                         GdkDragContext     *context,
                                                         int                 x,
                                                         int                 y,
                                                         GtkSelectionData   *data,
                                                         guint               info,
                                                         guint               time,
                                                         MooEditWindow      *window);
static gboolean      notebook_drag_drop                 (GtkWidget          *widget,
                                                         GdkDragContext     *context,
                                                         int                 x,
                                                         int                 y,
                                                         guint               time,
                                                         MooEditWindow      *window);
static gboolean      notebook_drag_motion               (GtkWidget          *widget,
                                                         GdkDragContext     *context,
                                                         int                 x,
                                                         int                 y,
                                                         guint               time,
                                                         MooEditWindow      *window);


/* actions */
static void action_new_doc                      (MooEditWindow      *window);
static void action_open                         (MooEditWindow      *window);
static void action_reload                       (MooEditWindow      *window);
static GtkAction *create_reopen_with_encoding_action (MooEditWindow *window);
static GtkAction *create_doc_encoding_action    (MooEditWindow      *window);
static GtkAction *create_doc_line_end_action    (MooEditWindow      *window);
static void action_save                         (MooEditWindow      *window);
static void action_save_as                      (MooEditWindow      *window);
static void action_close_tab                    (MooEditWindow      *window);
static void action_close_all                    (MooEditWindow      *window);
static void action_previous_tab                 (MooEditWindow      *window);
static void action_next_tab                     (MooEditWindow      *window);
static void action_previous_tab_in_view         (MooEditWindow      *window);
static void action_next_tab_in_view             (MooEditWindow      *window);
static void action_switch_to_tab                (MooEditWindow      *window,
                                                 guint               n);

static void action_toggle_bookmark              (MooEditWindow      *window);
static void action_next_bookmark                (MooEditWindow      *window);
static void action_prev_bookmark                (MooEditWindow      *window);
static GtkAction *create_goto_bookmark_action   (MooWindow          *window,
                                                 gpointer            data);

static void action_find_now_f                   (MooEditWindow      *window);
static void action_find_now_b                   (MooEditWindow      *window);
static void action_focus_doc                    (MooEditWindow      *window);
static void action_focus_other_split_notebook   (MooEditWindow      *window);
static void action_move_to_split_notebook       (MooEditWindow      *window);
static void action_abort_jobs                   (MooEditWindow      *window);

static void wrap_text_toggled                   (MooEditWindow      *window,
                                                 gboolean            active);
static void line_numbers_toggled                (MooEditWindow      *window,
                                                 gboolean            active);

#ifdef ENABLE_PRINTING
static void action_page_setup      (MooEditWindow    *window);
static void action_print           (MooEditWindow    *window);
static void action_print_preview   (MooEditWindow    *window);
static void action_print_pdf       (MooEditWindow    *window);
#endif

static void         split_view_horizontal_toggled       (MooEditWindow  *window,
                                                         gboolean        active);
static void         split_view_vertical_toggled         (MooEditWindow  *window,
                                                         gboolean        active);
static void         action_focus_next_split_view        (MooEditWindow  *window);
static void         update_split_view_actions           (MooEditWindow  *window);


G_DEFINE_TYPE (MooEditWindow, moo_edit_window, MOO_TYPE_WINDOW)

enum {
    PROP_0,
    PROP_EDITOR,
    PROP_ACTIVE_DOC,

    /* aux properties */
    PROP_CAN_RELOAD,
    PROP_HAS_OPEN_DOCUMENT,
    PROP_HAS_COMMENTS,
    PROP_HAS_JOBS_RUNNING,
    PROP_HAS_STOP_CLIENTS,
    PROP_CAN_MOVE_TO_SPLIT_NOTEBOOK
};

enum {
    BEFORE_CLOSE,
    WILL_CLOSE,
    NEW_DOC,
    CLOSE_DOC,
    CLOSE_DOC_AFTER,
    NUM_SIGNALS
};

static guint signals[NUM_SIGNALS];

#define INSTALL_PROP(prop_id,name)                                          \
    g_object_class_install_property (gobject_class, prop_id,                \
        g_param_spec_boolean (name, name, name, FALSE, G_PARAM_READABLE))

static void
moo_edit_window_class_init (MooEditWindowClass *klass)
{
    guint i;
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
    MooWindowClass *window_class = MOO_WINDOW_CLASS (klass);

    action_checks_init ();

    gobject_class->constructor = moo_edit_window_constructor;
    gobject_class->finalize = moo_edit_window_finalize;
    gobject_class->set_property = moo_edit_window_set_property;
    gobject_class->get_property = moo_edit_window_get_property;
    gtkobject_class->destroy = moo_edit_window_destroy;
    window_class->close = moo_edit_window_close_handler;

    klass->before_close = moo_edit_window_before_close;

    g_type_class_add_private (klass, sizeof (MooEditWindowPrivate));

    g_object_class_install_property (gobject_class, PROP_EDITOR,
        g_param_spec_object ("editor", "editor", "editor",
                             MOO_TYPE_EDITOR, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property (gobject_class, PROP_ACTIVE_DOC,
        g_param_spec_object ("active-doc", "active-doc", "active-doc",
                             MOO_TYPE_EDIT, (GParamFlags) G_PARAM_READWRITE));

    signals[BEFORE_CLOSE] =
            g_signal_new ("before-close",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditWindowClass, before_close),
                          moo_signal_accumulator_continue_cancel,
                          GINT_TO_POINTER (MOO_CLOSE_RESPONSE_CONTINUE),
                          _moo_marshal_ENUM__VOID,
                          MOO_TYPE_CLOSE_RESPONSE, 0);

    signals[WILL_CLOSE] =
            g_signal_new ("will-close",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditWindowClass, will_close),
                          nullptr, nullptr,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[NEW_DOC] =
            g_signal_new ("new-doc",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditWindowClass, new_doc),
                          nullptr, nullptr,
                          _moo_marshal_VOID__OBJECT,
                          G_TYPE_NONE, 1,
                          MOO_TYPE_EDIT);

    signals[CLOSE_DOC] =
            g_signal_new ("close-doc",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditWindowClass, close_doc),
                          nullptr, nullptr,
                          _moo_marshal_VOID__OBJECT,
                          G_TYPE_NONE, 1,
                          MOO_TYPE_EDIT);

    signals[CLOSE_DOC_AFTER] =
            g_signal_new ("close-doc-after",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooEditWindowClass, close_doc_after),
                          nullptr, nullptr,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    INSTALL_PROP (PROP_CAN_RELOAD, "can-reload");
    INSTALL_PROP (PROP_HAS_OPEN_DOCUMENT, "has-open-document");
    INSTALL_PROP (PROP_HAS_COMMENTS, "has-comments");
    INSTALL_PROP (PROP_HAS_JOBS_RUNNING, "has-jobs-running");
    INSTALL_PROP (PROP_HAS_STOP_CLIENTS, "has-stop-clients");
    INSTALL_PROP (PROP_CAN_MOVE_TO_SPLIT_NOTEBOOK, "can-move-to-split-notebook");

    moo_window_class_set_id (window_class, "Editor", "Editor");

    moo_window_class_new_action (window_class, "NewDoc", nullptr,
                                 "display-name", GTK_STOCK_NEW,
                                 "label", GTK_STOCK_NEW,
                                 "tooltip", _("Create new document"),
                                 "stock-id", GTK_STOCK_NEW,
                                 "default-accel", MOO_EDIT_ACCEL_NEW,
                                 "closure-callback", action_new_doc,
                                 nullptr);

    moo_window_class_new_action (window_class, "Open", nullptr,
                                 "display-name", GTK_STOCK_OPEN,
                                 "label", _("_Open..."),
                                 "tooltip", _("Open..."),
                                 "stock-id", GTK_STOCK_OPEN,
                                 "default-accel", MOO_EDIT_ACCEL_OPEN,
                                 "closure-callback", action_open,
                                 nullptr);

    moo_window_class_new_action (window_class, "Reload", nullptr,
                                 "display-name", _("Reload"),
                                 "label", _("_Reload"),
                                 "tooltip", _("Reload document"),
                                 "stock-id", GTK_STOCK_REFRESH,
                                 "default-accel", MOO_EDIT_ACCEL_RELOAD,
                                 "closure-callback", action_reload,
                                 "condition::sensitive", "can-reload",
                                 nullptr);

    moo_window_class_new_action_custom (window_class, "ReopenWithEncoding", nullptr,
                                        (MooWindowActionFunc) create_reopen_with_encoding_action,
                                        nullptr, nullptr);

    moo_window_class_new_action_custom (window_class, "EncodingMenu", nullptr,
                                        (MooWindowActionFunc) create_doc_encoding_action,
                                        nullptr, nullptr);

    moo_window_class_new_action_custom (window_class, "LineEndMenu", nullptr,
                                        (MooWindowActionFunc) create_doc_line_end_action,
                                        nullptr, nullptr);

    moo_window_class_new_action (window_class, "Save", nullptr,
                                 "display-name", GTK_STOCK_SAVE,
                                 "label", GTK_STOCK_SAVE,
                                 "tooltip", GTK_STOCK_SAVE,
                                 "stock-id", GTK_STOCK_SAVE,
                                 "default-accel", MOO_EDIT_ACCEL_SAVE,
                                 "closure-callback", action_save,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "SaveAs", nullptr,
                                 "display-name", GTK_STOCK_SAVE_AS,
                                 "label", _("Save _As..."),
                                 "tooltip", _("Save as..."),
                                 "stock-id", GTK_STOCK_SAVE_AS,
                                 "default-accel", MOO_EDIT_ACCEL_SAVE_AS,
                                 "closure-callback", action_save_as,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "Close", nullptr,
                                 "display-name", GTK_STOCK_CLOSE,
                                 "label", GTK_STOCK_CLOSE,
                                 "tooltip", _("Close document"),
                                 "stock-id", GTK_STOCK_CLOSE,
                                 "default-accel", MOO_EDIT_ACCEL_CLOSE,
                                 "closure-callback", action_close_tab,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "CloseAll", nullptr,
                                 "display-name", _("Close All"),
                                 "label", _("Close A_ll"),
                                 "tooltip", _("Close all documents"),
                                 "default-accel", MOO_EDIT_ACCEL_CLOSE_ALL,
                                 "closure-callback", action_close_all,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "PreviousTab", nullptr,
                                 "display-name", _("Previous Tab"),
                                 "label", _("_Previous Tab"),
                                 "tooltip", _("Previous tab"),
                                 "stock-id", GTK_STOCK_GO_BACK,
                                 "default-accel", MOO_EDIT_ACCEL_PREV_TAB,
                                 "closure-callback", action_previous_tab,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "NextTab", nullptr,
                                 "display-name", _("Next Tab"),
                                 "label", _("_Next Tab"),
                                 "tooltip", _("Next tab"),
                                 "stock-id", GTK_STOCK_GO_FORWARD,
                                 "default-accel", MOO_EDIT_ACCEL_NEXT_TAB,
                                 "closure-callback", action_next_tab,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "PreviousTabInView", nullptr,
                                 "display-name", _("Previous Tab In View"),
                                 "label", _("_Previous Tab In View"),
                                 "tooltip", _("Previous tab in view"),
                                 "default-accel", MOO_EDIT_ACCEL_PREV_TAB_IN_VIEW,
                                 "closure-callback", action_previous_tab_in_view,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "NextTabInView", nullptr,
                                 "display-name", _("Next Tab In View"),
                                 "label", _("_Next Tab In View"),
                                 "tooltip", _("Next tab in view"),
                                 "default-accel", MOO_EDIT_ACCEL_NEXT_TAB_IN_VIEW,
                                 "closure-callback", action_next_tab_in_view,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "Find", nullptr,
                                 "display-name", GTK_STOCK_FIND,
                                 "label", GTK_STOCK_FIND,
                                 "tooltip", GTK_STOCK_FIND,
                                 "stock-id", GTK_STOCK_FIND,
                                 "default-accel", MOO_EDIT_ACCEL_FIND,
                                 "closure-signal", "find-interactive",
                                 "closure-proxy-func", moo_edit_window_get_active_view,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "FindNext", nullptr,
                                 "display-name", _("Find Next"),
                                 "label", _("Find _Next"),
                                 "tooltip", _("Find next"),
                                 "stock-id", GTK_STOCK_GO_FORWARD,
                                 "default-accel", MOO_EDIT_ACCEL_FIND_NEXT,
                                 "closure-signal", "find-next-interactive",
                                 "closure-proxy-func", moo_edit_window_get_active_view,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "FindPrevious", nullptr,
                                 "display-name", _("Find Previous"),
                                 "label", _("Find _Previous"),
                                 "tooltip", _("Find previous"),
                                 "stock-id", GTK_STOCK_GO_BACK,
                                 "default-accel", MOO_EDIT_ACCEL_FIND_PREV,
                                 "closure-signal", "find-prev-interactive",
                                 "closure-proxy-func", moo_edit_window_get_active_view,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "Replace", nullptr,
                                 "display-name", GTK_STOCK_FIND_AND_REPLACE,
                                 "label", GTK_STOCK_FIND_AND_REPLACE,
                                 "tooltip", GTK_STOCK_FIND_AND_REPLACE,
                                 "stock-id", GTK_STOCK_FIND_AND_REPLACE,
                                 "default-accel", MOO_EDIT_ACCEL_REPLACE,
                                 "closure-signal", "replace-interactive",
                                 "closure-proxy-func", moo_edit_window_get_active_view,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "FindCurrent", nullptr,
                                 "display-name", _("Find Current Word"),
                                 "label", _("Find Current _Word"),
                                 "stock-id", GTK_STOCK_FIND,
                                 "default-accel", MOO_EDIT_ACCEL_FIND_CURRENT,
                                 "closure-callback", action_find_now_f,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "FindCurrentBack", nullptr,
                                 "display-name", _("Find Current Word Backwards"),
                                 "label", _("Find Current Word _Backwards"),
                                 "stock-id", GTK_STOCK_FIND,
                                 "default-accel", MOO_EDIT_ACCEL_FIND_CURRENT_BACK,
                                 "closure-callback", action_find_now_b,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "GoToLine", nullptr,
                                 "display-name", _("Go to Line"),
                                 "label", _("_Go to Line..."),
                                 "tooltip", _("Go to line..."),
                                 "default-accel", MOO_EDIT_ACCEL_GOTO_LINE,
                                 "closure-signal", "goto-line-interactive",
                                 "closure-proxy-func", moo_edit_window_get_active_view,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "WrapText", nullptr,
                                 "action-type::", MOO_TYPE_TOGGLE_ACTION,
                                 "display-name", _("Toggle Text Wrapping"),
                                 "label", _("_Wrap Text"),
                                 "toggled-callback", wrap_text_toggled,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "LineNumbers", nullptr,
                                 "action-type::", MOO_TYPE_TOGGLE_ACTION,
                                 "display-name", _("Toggle Line Numbers Display"),
                                 "label", _("Show _Line Numbers"),
                                 "toggled-callback", line_numbers_toggled,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "FocusDoc", nullptr,
                                 "display-name", _("Focus Document"),
                                 "label", _("_Focus Document"),
                                 "default-accel", MOO_EDIT_ACCEL_FOCUS_DOC,
                                 "closure-callback", action_focus_doc,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "FocusOtherSplitNotebook", nullptr,
                                 "display-name", _("Focus Other Split Notebook"),
                                 "label", _("Focus Other Split Notebook"),
                                 "default-accel", MOO_EDIT_ACCEL_FOCUS_SPLIT_NOTEBOOK,
                                 "closure-callback", action_focus_other_split_notebook,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "MoveToSplitView", nullptr,
                                 "display-name", _("Move to Split Notebook"),
                                 "label", _("_Move to Split Notebook"),
                                 "default-accel", MOO_EDIT_ACCEL_MOVE_TO_SPLIT_NOTEBOOK,
                                 "closure-callback", action_move_to_split_notebook,
                                 "condition::sensitive", "can-move-to-split-notebook",
                                 nullptr);

    moo_window_class_new_action (window_class, "SplitViewHorizontal", nullptr,
                                 "action-type::", MOO_TYPE_TOGGLE_ACTION,
                                 "display-name", _("Split View Horizontally"),
                                 "label", _("Split View Horizontally"),
                                 "toggled-callback", split_view_horizontal_toggled,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "SplitViewVertical", nullptr,
                                 "action-type::", MOO_TYPE_TOGGLE_ACTION,
                                 "display-name", _("Split View Vertically"),
                                 "label", _("Split View Vertically"),
                                 "toggled-callback", split_view_vertical_toggled,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "CycleSplitViews", nullptr,
                                 "display-name", _("Focus Next Split View"),
                                 "label", _("Focus Next Split View"),
                                 "closure-callback", action_focus_next_split_view,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, STOP_ACTION_ID, nullptr,
                                 "display-name", GTK_STOCK_STOP,
                                 "label", GTK_STOCK_STOP,
                                 "tooltip", GTK_STOCK_STOP,
                                 "stock-id", GTK_STOCK_STOP,
                                 "default-accel", MOO_EDIT_ACCEL_STOP,
                                 "closure-callback", action_abort_jobs,
                                 "condition::sensitive", "has-jobs-running",
                                 "condition::visible", "has-stop-clients",
                                 nullptr);

    for (i = 1; i < 10; ++i)
    {
        char *action_id = g_strdup_printf (DOCUMENT_ACTION "%u", i);
        char *accel = g_strdup_printf (MOO_EDIT_ACCEL_SWITCH_TO_TAB "%u", i);
        _moo_window_class_new_action_callback (window_class, action_id, nullptr,
                                               G_CALLBACK (action_switch_to_tab),
                                               _moo_marshal_VOID__UINT,
                                               G_TYPE_NONE, 1,
                                               G_TYPE_UINT, i - 1,
                                               "default-accel", accel,
                                               "connect-accel", TRUE,
                                               "accel-editable", FALSE,
                                               nullptr);
        g_free (accel);
        g_free (action_id);
    }

    for (i = 1; i < 10; ++i)
    {
        char *action_id = g_strdup_printf (MOO_EDIT_GOTO_BOOKMARK_ACTION "%u", i);
        moo_window_class_new_action_custom (window_class, action_id, nullptr,
                                            create_goto_bookmark_action,
                                            GUINT_TO_POINTER (i),
                                            nullptr);
        g_free (action_id);
    }

    moo_window_class_new_action (window_class, "ToggleBookmark", nullptr,
                                 "display-name", _("Toggle Bookmark"),
                                 "label", _("Toggle _Bookmark"),
                                 "stock-id", MOO_STOCK_EDIT_BOOKMARK,
                                 "default-accel", MOO_EDIT_ACCEL_BOOKMARK,
                                 "closure-callback", action_toggle_bookmark,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "NextBookmark", nullptr,
                                 "display-name", _("Next Bookmark"),
                                 "label", _("_Next Bookmark"),
                                 "default-accel", MOO_EDIT_ACCEL_NEXT_BOOKMARK,
                                 "connect-accel", TRUE,
                                 "closure-callback", action_next_bookmark,
                                 nullptr);

    moo_window_class_new_action (window_class, "PreviousBookmark", nullptr,
                                 "display-name", _("Previous Bookmark"),
                                 "label", _("_Previous Bookmark"),
                                 "default-accel", MOO_EDIT_ACCEL_PREV_BOOKMARK,
                                 "connect-accel", TRUE,
                                 "closure-callback", action_prev_bookmark,
                                 nullptr);

    moo_window_class_new_action (window_class, "Comment", nullptr,
                                 /* action */
                                 "display-name", _("Comment"),
                                 "label", _("Comment"),
                                 "tooltip", _("Comment"),
                                 "closure-callback", moo_edit_comment_selection,
                                 "closure-proxy-func", moo_edit_window_get_active_doc,
                                 "condition::sensitive", "has-comments",
                                 nullptr);

    moo_window_class_new_action (window_class, "Uncomment", nullptr,
                                 /* action */
                                 "display-name", _("Uncomment"),
                                 "label", _("Uncomment"),
                                 "tooltip", _("Uncomment"),
                                 "closure-callback", moo_edit_uncomment_selection,
                                 "closure-proxy-func", moo_edit_window_get_active_doc,
                                 "condition::sensitive", "has-comments",
                                 nullptr);

    moo_window_class_new_action (window_class, "Indent", nullptr,
                                 "display-name", GTK_STOCK_INDENT,
                                 "label", GTK_STOCK_INDENT,
                                 "tooltip", GTK_STOCK_INDENT,
                                 "stock-id", GTK_STOCK_INDENT,
                                 "closure-callback", moo_text_view_indent,
                                 "closure-proxy-func", moo_edit_window_get_active_view,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "Unindent", nullptr,
                                 "display-name", GTK_STOCK_UNINDENT,
                                 "label", GTK_STOCK_UNINDENT,
                                 "tooltip", GTK_STOCK_UNINDENT,
                                 "stock-id", GTK_STOCK_UNINDENT,
                                 "closure-callback", moo_text_view_unindent,
                                 "closure-proxy-func", moo_edit_window_get_active_view,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "NoDocuments", nullptr,
                                 /* Insensitive menu item which appears in Window menu with no documents open */
                                 "label", _("No Documents"),
                                 "no-accel", TRUE,
                                 "sensitive", FALSE,
                                 "condition::visible", "!has-open-document",
                                 nullptr);

#ifdef ENABLE_PRINTING
    moo_window_class_new_action (window_class, "PageSetup", nullptr,
                                 "display-name", _("Page Setup"),
                                 "label", _("Page S_etup..."),
                                 "tooltip", _("Page Setup..."),
                                 "default-accel", MOO_EDIT_ACCEL_PAGE_SETUP,
                                 "closure-callback", action_page_setup,
                                 nullptr);

    moo_window_class_new_action (window_class, "PrintPreview", nullptr,
                                 "display-name", GTK_STOCK_PRINT_PREVIEW,
                                 "label", GTK_STOCK_PRINT_PREVIEW,
                                 "tooltip", GTK_STOCK_PRINT_PREVIEW,
                                 "stock-id", GTK_STOCK_PRINT_PREVIEW,
                                 "closure-callback", action_print_preview,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "Print", nullptr,
                                 "display-name", GTK_STOCK_PRINT,
                                 "label", _("Print..."),
                                 "tooltip", _("Print..."),
                                 "default-accel", MOO_EDIT_ACCEL_PRINT,
                                 "stock-id", GTK_STOCK_PRINT,
                                 "closure-callback", action_print,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);

    moo_window_class_new_action (window_class, "PrintPdf", nullptr,
                                 "display-name", _("Export as PDF"),
                                 "label", _("E_xport as PDF..."),
                                 "tooltip", _("Export as PDF..."),
                                 "stock-id", GTK_STOCK_PRINT,
                                 "closure-callback", action_print_pdf,
                                 "condition::sensitive", "has-open-document",
                                 nullptr);
#endif

    moo_window_class_new_action_custom (window_class, LANG_ACTION_ID, nullptr,
                                        (MooWindowActionFunc) create_lang_action,
                                        nullptr, nullptr);

    {
        GValue val = { 0 };
        g_value_init (&val, G_TYPE_INT);
        g_value_set_int (&val, 0);
        moo_prefs_new_key (PREFS_KEY_SPLIT_POS, G_TYPE_INT, &val, MOO_PREFS_STATE);
    }
}


static void
moo_edit_window_init (MooEditWindow *window)
{
    window->priv = G_TYPE_INSTANCE_GET_PRIVATE (window, MOO_TYPE_EDIT_WINDOW, MooEditWindowPrivate);
    new(window->priv) MooEditWindowPrivate;

    window->priv->history = nullptr;
    window->priv->history_blocked = FALSE;
    window->priv->enable_history = TRUE;

    g_object_set (G_OBJECT (window),
                  "menubar-ui-name", "Editor/Menubar",
                  "toolbar-ui-name", "Editor/Toolbar",
                  nullptr);

    set_title_format_from_prefs (window);

    windows.insert(window);
}


/**
 * moo_edit_window_get_editor:
 */
MooEditor *
moo_edit_window_get_editor (MooEditWindow *window)
{
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), nullptr);
    return window->priv->editor;
}


static void
moo_edit_window_destroy (GtkObject *object)
{
    MooEditWindow *window = MOO_EDIT_WINDOW (object);

    if (window->priv->save_params_idle)
    {
        g_source_remove (window->priv->save_params_idle);
        window->priv->save_params_idle = 0;
    }

    if (window->priv->statusbar_idle)
    {
        g_source_remove (window->priv->statusbar_idle);
        window->priv->statusbar_idle = 0;
    }

    if (window->priv->stop_clients || window->priv->jobs)
    {
        GSList *list, *l;

        moo_edit_window_abort_jobs (window);
        g_slist_foreach (window->priv->jobs, (GFunc) g_free, nullptr);
        g_slist_free (window->priv->jobs);
        window->priv->jobs = nullptr;

        list = g_slist_copy (window->priv->stop_clients);
        for (l = list; l != nullptr; l = l->next)
            moo_edit_window_remove_stop_client (window, (GObject*) l->data);
        g_assert (window->priv->stop_clients == nullptr);
        g_slist_free (list);
    }

    window->priv->notebooks.clear();

    windows.erase(window);

    GTK_OBJECT_CLASS(moo_edit_window_parent_class)->destroy (object);
}


static void
moo_edit_window_finalize (GObject *object)
{
    MooEditWindow *window = MOO_EDIT_WINDOW (object);

    window->priv->~MooEditWindowPrivate();

    G_OBJECT_CLASS (moo_edit_window_parent_class)->finalize (object);
}


static void
moo_edit_window_set_property (GObject        *object,
                              guint           prop_id,
                              const GValue   *value,
                              GParamSpec     *pspec)
{
    MooEditWindow *window = MOO_EDIT_WINDOW (object);

    switch (prop_id)
    {
        case PROP_EDITOR:
            window->priv->editor = (MooEditor*) g_value_get_object (value);
            break;

        case PROP_ACTIVE_DOC:
            moo_edit_window_set_active_doc (window, (MooEdit*) g_value_get_object (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void     moo_edit_window_get_property(GObject        *object,
                                             guint           prop_id,
                                             GValue         *value,
                                             GParamSpec     *pspec)
{
    MooEditWindow *window = MOO_EDIT_WINDOW (object);
    MooEdit *doc;

    switch (prop_id)
    {
        case PROP_EDITOR:
            g_value_set_object (value, window->priv->editor);
            break;

        case PROP_ACTIVE_DOC:
            g_value_set_object (value, ACTIVE_DOC (window));
            break;

        case PROP_CAN_RELOAD:
            doc = ACTIVE_DOC (window);
            g_value_set_boolean (value, doc && !moo_edit_is_untitled (doc));
            break;
        case PROP_HAS_OPEN_DOCUMENT:
            g_value_set_boolean (value, moo_edit_window_get_n_tabs (window) != 0);
            break;
        case PROP_CAN_MOVE_TO_SPLIT_NOTEBOOK:
            g_value_set_boolean (value, can_move_to_split_notebook (window));
            break;
        case PROP_HAS_COMMENTS:
            doc = ACTIVE_DOC (window);
            g_value_set_boolean (value, doc && _moo_edit_has_comments (doc, nullptr, nullptr));
            break;
        case PROP_HAS_JOBS_RUNNING:
            g_value_set_boolean (value, window->priv->jobs != nullptr);
            break;
        case PROP_HAS_STOP_CLIENTS:
            g_value_set_boolean (value, window->priv->stop_clients != nullptr);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


/****************************************************************************/
/* Constructing
 */

static GObject *
moo_edit_window_constructor (GType                  type,
                             guint                  n_props,
                             GObjectConstructParam *props)
{
    MooEditWindow *window;
    GtkWindowGroup *group;
    int i;

    GObject *object =
            G_OBJECT_CLASS(moo_edit_window_parent_class)->constructor (type, n_props, props);

    window = MOO_EDIT_WINDOW (object);
    g_return_val_if_fail (window->priv->editor != nullptr, object);

    group = gtk_window_group_new ();
    gtk_window_group_add_window (group, GTK_WINDOW (window));
    g_object_unref (group);

    create_paned (window);

    window->priv->doc_paned = gtk_hpaned_new ();
    gtk_widget_show (window->priv->doc_paned);
    g_signal_connect_swapped (window->priv->doc_paned, "notify::position",
                              G_CALLBACK (queue_save_window_config), window);
    moo_big_paned_add_child (window->paned, window->priv->doc_paned);

    for (i = 0; i < 2; ++i)
    {
        GtkWidget *notebook = GTK_WIDGET (
            g_object_new (MOO_TYPE_NOTEBOOK,
                          "show-tabs", TRUE,
                          "enable-popup", TRUE,
                          "enable-reordering", TRUE,
                          (const char*) nullptr));

        if (i == 0)
            gtk_widget_show (notebook);

        window->priv->notebooks.push_back(g::ref_obj(MOO_NOTEBOOK(notebook)));

        if (i == 0)
            gtk_paned_pack1 (GTK_PANED (window->priv->doc_paned), notebook, TRUE, FALSE);
        else
            gtk_paned_pack2 (GTK_PANED (window->priv->doc_paned), notebook, TRUE, FALSE);

        setup_notebook (window, MOO_NOTEBOOK (notebook));
    }

    create_statusbar (window);

    g_signal_connect (window, "realize", G_CALLBACK (update_window_title), nullptr);
    g_signal_connect (window, "notify::menubar",
                      G_CALLBACK (moo_edit_window_connect_menubar), nullptr);

    edit_changed (window, nullptr);
    moo_edit_window_check_actions (window);

    return object;
}


static gboolean
save_window_config (MooEditWindow *window)
{
    window->priv->save_params_idle = 0;
    save_paned_config (window);
    save_doc_paned_config (window);
    return FALSE;
}

static void
queue_save_window_config (MooEditWindow *window)
{
    if (!window->priv->save_params_idle)
        window->priv->save_params_idle =
            g_idle_add ((GSourceFunc) save_window_config, window);
}


static char *
get_doc_status_string (MooEdit *doc)
{
    MooEditStatus status;
    gboolean modified;

    status = moo_edit_get_status (doc);
    modified = (status & MOO_EDIT_STATUS_MODIFIED) && !(status & MOO_EDIT_STATUS_CLEAN);

    if (!modified)
    {
        if (status & MOO_EDIT_STATUS_NEW)
            /* Translators: this goes into window title, %s stands for filename, e.g. "/foo/bar.txt [new file]" */
            return g_strdup_printf (_("%s [new file]"), "");
        else if (status & MOO_EDIT_STATUS_MODIFIED_ON_DISK)
            /* Translators: this goes into window title, %s stands for filename, e.g. "/foo/bar.txt [modified on disk]" */
            return g_strdup_printf (_("%s [modified on disk]"), "");
        else if (status & MOO_EDIT_STATUS_DELETED)
            /* Translators: this goes into window title, %s stands for filename, e.g. "/foo/bar.txt [deleted]" */
            return g_strdup_printf (_("%s [deleted]"), "");
        else
            return g_strdup_printf ("%s", "");
    }
    else
    {
        if (status & MOO_EDIT_STATUS_NEW)
            /* Translators: this goes into window title, %s stands for filename, e.g. "/foo/bar.txt [new file] [modified]" */
            return g_strdup_printf (_("%s [new file] [modified]"), "");
        else if (status & MOO_EDIT_STATUS_MODIFIED_ON_DISK)
            /* Translators: this goes into window title, %s stands for filename, e.g. "/foo/bar.txt [modified on disk] [modified]" */
            return g_strdup_printf (_("%s [modified on disk] [modified]"), "");
        else if (status & MOO_EDIT_STATUS_DELETED)
            /* Translators: this goes into window title, %s stands for filename, e.g. "/foo/bar.txt [deleted] [modified]" */
            return g_strdup_printf (_("%s [deleted] [modified]"), "");
        else
            /* Translators: this goes into window title, %s stands for filename, e.g. "/foo/bar.txt [modified]" */
            return g_strdup_printf (_("%s [modified]"), "");
    }
}

static std::string
parse_title_format(const std::string& format, MooEdit *doc)
{
    GString *str;

    str = g_string_new (nullptr);

    for (const char* p = format.c_str(); *p; )
    {
        if (*p == '%')
        {
            p++;

            if (!*p)
            {
                g_critical ("%s: trailing percent sign", G_STRFUNC);
                break;
            }

            switch (*p)
            {
                case 'a':
                    g_string_append (str, moo_get_display_app_name ());
                    break;
                case 'b':
                    if (!doc)
                        g_critical ("%s: %%b used without document", G_STRFUNC);
                    else
                        g_string_append (str, moo_edit_get_display_basename (doc));
                    break;
                case 'f':
                    if (!doc)
                        g_critical ("%s: %%f used without document", G_STRFUNC);
                    else
                        g_string_append (str, moo_edit_get_display_name (doc));
                    break;
                case 'u':
                    if (!doc)
                        g_critical ("%s: %%u used without document", G_STRFUNC);
                    else
                    {
                        gstr tmp = gstr::take (moo_edit_get_uri (doc));
                        if (!tmp.empty())
                            g_string_append (str, tmp.get());
                        else
                            g_string_append (str, moo_edit_get_display_name (doc));
                    }
                    break;
                case 's':
                    if (!doc)
                        g_critical ("%s: %%s used without document", G_STRFUNC);
                    else
                    {
                        char *tmp = get_doc_status_string (doc);
                        if (tmp)
                            g_string_append (str, tmp);
                        g_free (tmp);
                    }
                    break;
                case '%':
                    g_string_append_c (str, '%');
                    break;
                default:
                    g_critical ("%s: unknown format '%%%c'", G_STRFUNC, *p);
                    break;
            }
        }
        else
        {
            g_string_append_c (str, *p);
        }

        p++;
    }

    return g_string_free (str, FALSE);
}

static void
update_window_title (MooEditWindow *window)
{
    MooEdit *doc = ACTIVE_DOC (window);

    std::string title;

    if (doc)
        title = parse_title_format (window->priv->title_format, doc);
    else
        title = parse_title_format (window->priv->title_format_no_doc, nullptr);

    moo_window_set_title (MOO_WINDOW (window), title.c_str());
}

static std::string
check_format (const std::string& format)
{
    if (format.empty())
        return DEFAULT_TITLE_FORMAT;

    if (!g_utf8_validate (format.c_str(), -1, nullptr))
    {
        g_critical ("window title format is not valid UTF8");
        return DEFAULT_TITLE_FORMAT;
    }

    return format;
}

static void
set_title_format (MooEditWindow* window,
                  std::string    format,
                  std::string    format_no_doc)
{
    format = check_format (format);
    format_no_doc = check_format (format_no_doc);

    window->priv->title_format = std::move(format);
    window->priv->title_format_no_doc = std::move(format_no_doc);

    if (GTK_WIDGET_REALIZED (window))
        update_window_title (window);
}

static void
set_title_format_from_prefs (MooEditWindow *window)
{
    const char *format, *format_no_doc;
    format = moo_prefs_get_string (moo_edit_setting (MOO_EDIT_PREFS_TITLE_FORMAT));
    format_no_doc = moo_prefs_get_string (moo_edit_setting (MOO_EDIT_PREFS_TITLE_FORMAT_NO_DOC));
    set_title_format (window, format, format_no_doc);
}

void
_moo_edit_window_update_title (void)
{
    for (MooEditWindow* w: windows)
        set_title_format_from_prefs (w);
}


/**
 * moo_edit_window_close:
 */
gboolean
moo_edit_window_close (MooEditWindow *window)
{
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), FALSE);
    return moo_editor_close_window (moo_edit_window_get_editor (window), window);
}

/**
 * moo_edit_window_close_all:
 */
gboolean
moo_edit_window_close_all (MooEditWindow *window)
{
    MooEditArray *docs;
    gboolean result;

    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), FALSE);

    docs = moo_edit_window_get_docs (window);
    result = moo_editor_close_docs (window->priv->editor, docs);

    moo_edit_array_free (docs);
    return result;
}


static MooCloseResponse
moo_edit_window_before_close (G_GNUC_UNUSED MooEditWindow *window)
{
    return MOO_CLOSE_RESPONSE_CONTINUE;
}

static MooCloseResponse
moo_edit_window_close_handler (MooWindow *window)
{
    MooEditWindow *edit_window = MOO_EDIT_WINDOW (window);
    moo_editor_close_window (edit_window->priv->editor, edit_window);
    return MOO_CLOSE_RESPONSE_CANCEL;
}


/****************************************************************************/
/* Actions
 */

static void
action_new_doc (MooEditWindow *window)
{
    moo_editor_new_doc (window->priv->editor, window);
}


static void
action_open (MooEditWindow *window)
{
    MooEdit *active = moo_edit_window_get_active_doc (window);
    MooOpenInfoArray *files = _moo_edit_open_dialog (GTK_WIDGET (window), active);
    if (!moo_open_info_array_is_empty (files))
        moo_editor_open_files (window->priv->editor, files, GTK_WIDGET (window), nullptr);
    moo_open_info_array_free (files);
}


static void
action_reload (MooEditWindow *window)
{
    MooEdit *doc = ACTIVE_DOC (window);
    g_return_if_fail (doc != nullptr);
    moo_edit_reload (doc, nullptr, nullptr);
}


static void
reopen_encoding_item_activated (const char *encoding,
                                gpointer    data)
{
    MooEditWindow *window = (MooEditWindow*) data;
    MooEdit *doc;
    MooReloadInfo *info;

    doc = ACTIVE_DOC (window);
    g_return_if_fail (doc != nullptr);

    info = moo_reload_info_new (encoding, -1);
    moo_edit_reload (doc, info, nullptr);
}

static GtkAction *
create_reopen_with_encoding_action (MooEditWindow *window)
{
    GtkAction *action;

    action = _moo_encodings_menu_action_new ("ReopenWithEncoding",
                                             _("Reopen Using Encoding"),
                                             reopen_encoding_item_activated,
                                             window);
    moo_bind_bool_property (action, "sensitive",
                            window, "can-reload", FALSE);

    return action;
}


static void
update_doc_encoding_item (MooEditWindow *window)
{
    MooEdit *doc;
    GtkAction *action;
    const char *enc;

    if (!(doc = ACTIVE_DOC (window)))
        return;

    action = moo_window_get_action (MOO_WINDOW (window), "EncodingMenu");
    g_return_if_fail (action != nullptr);

    enc = moo_edit_get_encoding (doc);

    if (!enc)
        enc = _moo_edit_get_default_encoding ();

    _moo_encodings_menu_action_set_current (action, enc);
}

static void
doc_encoding_item_activated (const char *encoding,
                             gpointer    data)
{
    MooEditWindow *window = (MooEditWindow*) data;
    MooEdit *doc;

    doc = ACTIVE_DOC (window);
    g_return_if_fail (doc != nullptr);

    moo_edit_set_encoding (doc, encoding);
}

static GtkAction *
create_doc_encoding_action (MooEditWindow *window)
{
    GtkAction *action;

    action = _moo_encodings_menu_action_new ("EncodingMenu",
                                             _("_Encoding"),
                                             doc_encoding_item_activated,
                                             window);
    moo_bind_bool_property (action, "sensitive",
                            window, "has-open-document", FALSE);

    return action;
}


static const char *line_end_menu_items[] = {
    nullptr, /* MOO_LE_NONE */
    "LineEndUnix", /* MOO_LE_UNIX */
    "LineEndWin32", /* MOO_LE_WIN32 */
    "LineEndMac", /* MOO_LE_MAC */
};

static void
update_doc_line_end_item (MooEditWindow *window)
{
    MooEdit *doc;
    GtkAction *action;
    MooLineEndType le;

    if (!(doc = ACTIVE_DOC (window)))
        return;

    action = moo_window_get_action (MOO_WINDOW (window), "LineEndMenu");
    g_return_if_fail (action != nullptr);

    le = moo_edit_get_line_end_type (doc);
    g_return_if_fail (le > 0 && le < G_N_ELEMENTS(line_end_menu_items));

    moo_menu_mgr_set_active (moo_menu_action_get_mgr (MOO_MENU_ACTION (action)),
                             line_end_menu_items[le], TRUE);
}

static void
doc_line_end_item_set_active (MooEditWindow *window, gpointer data)
{
    MooEdit *doc;

    doc = ACTIVE_DOC (window);
    g_return_if_fail (doc != nullptr);

    moo_edit_set_line_end_type (doc, (MooLineEndType) GPOINTER_TO_INT (data));
}

static GtkAction *
create_doc_line_end_action (MooEditWindow *window)
{
    GtkAction *action;
    MooMenuMgr *mgr;

    action = moo_menu_action_new ("LineEndMenu", _("Line En_dings"));
    mgr = moo_menu_action_get_mgr (MOO_MENU_ACTION (action));

    g_signal_connect_swapped (mgr, "radio-set-active", G_CALLBACK (doc_line_end_item_set_active), window);

    moo_menu_mgr_append (mgr, nullptr,
                         line_end_menu_items[MOO_LE_WIN32],
                         _("_Windows (CR+LF)"), nullptr, MOO_MENU_ITEM_RADIO,
                         GINT_TO_POINTER (MOO_LE_WIN32), nullptr);
    moo_menu_mgr_insert (mgr, nullptr,
#ifdef MOO_OS_WIN32
                         1,
#else
                         0,
#endif
                         line_end_menu_items[MOO_LE_UNIX],
                         _("_Unix (LF)"), nullptr, MOO_MENU_ITEM_RADIO,
                         GINT_TO_POINTER (MOO_LE_UNIX), nullptr);
    moo_menu_mgr_append (mgr, nullptr,
                         line_end_menu_items[MOO_LE_MAC],
                         _("_Mac (CR)"), nullptr, MOO_MENU_ITEM_RADIO,
                         GINT_TO_POINTER (MOO_LE_MAC), nullptr);

    moo_bind_bool_property (action, "sensitive",
                            window, "has-open-document", FALSE);

    return action;
}


static void
action_save (MooEditWindow *window)
{
    MooEdit *doc = ACTIVE_DOC (window);
    g_return_if_fail (doc != nullptr);
    moo_editor_save (window->priv->editor, doc, nullptr);
}


static void
action_save_as (MooEditWindow *window)
{
    MooEdit *doc = ACTIVE_DOC (window);
    g_return_if_fail (doc != nullptr);
    moo_editor_save_as (window->priv->editor, doc, nullptr, nullptr);
}


static void
action_close_tab (MooEditWindow *window)
{
    MooEdit *doc = ACTIVE_DOC (window);
    g_return_if_fail (doc != nullptr);
    moo_editor_close_doc (window->priv->editor, doc);
}


static void
action_close_all (MooEditWindow *window)
{
    moo_edit_window_close_all (window);
}


static void
switch_to_tab (MooEditWindow *window,
               int            n)
{
    int n_docs = moo_edit_window_get_n_tabs (window);
    int i;

    if (n < 0)
        n = n_docs - 1;

    if (n < 0 || n >= n_docs)
        return;

    for (i = 0; i < 2; ++i)
    {
        MooNotebook *notebook = get_notebook (window, i);
        int n_pages = moo_notebook_get_n_pages (notebook);
        if (n < n_pages)
        {
            MooEditTab *tab = get_nth_tab (*notebook, n);
            moo_edit_window_set_active_tab (window, tab);
            break;
        }
        n -= n_pages;
    }
}

static void
action_previous_tab (MooEditWindow *window)
{
    int n;

    n = get_active_tab (window);

    if (n > 0)
        switch_to_tab (window, n - 1);
    else
        switch_to_tab (window, -1);
}


static void
action_next_tab (MooEditWindow *window)
{
    int n;

    n = get_active_tab (window);

    if (n < moo_edit_window_get_n_tabs (window) - 1)
        switch_to_tab (window, n + 1);
    else
        switch_to_tab (window, 0);
}

static int
get_first_tab_in_notebook (MooNotebook *notebook, MooEditWindow *window)
{
    int tab = 0;

    for (const auto& nb: window->priv->notebooks)
    {
        if (nb == notebook)
            return tab;

        tab += moo_notebook_get_n_pages (nb.get());
    }

    g_return_val_if_reached (-1);
}

static void
action_previous_tab_in_view (MooEditWindow *window)
{
    int n, tabs_in_notebook, first_tab;
    MooNotebook *notebook;

    n = get_active_tab (window);
    notebook = get_active_notebook (window);
    tabs_in_notebook = moo_notebook_get_n_pages (notebook);
    first_tab = get_first_tab_in_notebook (notebook, window);


    if (n > first_tab)
        switch_to_tab (window, n - 1);
    else
        switch_to_tab (window, first_tab + tabs_in_notebook - 1);
}

static void
action_next_tab_in_view (MooEditWindow *window)
{
    int n, tabs_in_notebook, first_tab;
    MooNotebook *notebook;

    n = get_active_tab (window);
    notebook = get_active_notebook (window);
    tabs_in_notebook = moo_notebook_get_n_pages (notebook);
    first_tab = get_first_tab_in_notebook (notebook, window);

    if (n < (first_tab + tabs_in_notebook - 1))
        switch_to_tab (window, n + 1);
    else
        switch_to_tab (window, first_tab);
}

static void
action_switch_to_tab (MooEditWindow *window,
                      guint          n)
{
    switch_to_tab (window, n);
}


static void
moo_edit_window_find_now (MooEditWindow *window,
                          gboolean       forward)
{
    MooEditView *view;

    view = ACTIVE_VIEW (window);
    g_return_if_fail (view != nullptr);

    g_signal_emit_by_name (view, "find-word-at-cursor", forward);
}

static void
action_find_now_f (MooEditWindow *window)
{
    moo_edit_window_find_now (window, TRUE);
}


static void
action_find_now_b (MooEditWindow *window)
{
    moo_edit_window_find_now (window, FALSE);
}


static void
action_focus_doc (MooEditWindow *window)
{
    MooEditView *active_view;

    active_view = ACTIVE_VIEW (window);
    g_return_if_fail (active_view != nullptr);

    if (!GTK_WIDGET_HAS_FOCUS (active_view))
    {
        gtk_widget_grab_focus (GTK_WIDGET (active_view));
    }
    else if (both_notebooks_visible (window))
    {
        MooNotebook *nb1 = get_notebook (window, 0);
        MooNotebook *nb2 = get_notebook (window, 1);
        MooEditView *view1 = get_notebook_active_view (nb1);
        MooEditView *view2 = get_notebook_active_view (nb2);
        if (view1 == active_view)
            gtk_widget_grab_focus (GTK_WIDGET (view2));
        else if (view2 == active_view)
            gtk_widget_grab_focus (GTK_WIDGET (view1));
        else
            g_return_if_reached ();
    }
}

static void
action_focus_other_split_notebook (MooEditWindow *window)
{
    if (both_notebooks_visible (window))
    {
        MooNotebook *current = get_active_notebook (window);
        MooNotebook *nb1 = get_notebook (window, 0);
        MooNotebook *nb2 = get_notebook (window, 1);

        if (current == nb1)
            moo_edit_window_set_active_tab (window,
                get_nth_tab(*nb2, moo_notebook_get_current_page (nb2)));
        else
            moo_edit_window_set_active_tab (window,
                get_nth_tab(*nb1, moo_notebook_get_current_page (nb1)));
    }
}


static void
action_move_to_split_notebook (MooEditWindow *window)
{
    move_tab_to_split_view (window, ACTIVE_TAB (window));
}


static void
action_abort_jobs (MooEditWindow *window)
{
    moo_edit_window_abort_jobs (window);
}


static void
action_toggle_bookmark (MooEditWindow *window)
{
    MooEditView *view = ACTIVE_VIEW (window);
    g_return_if_fail (view != nullptr);
    moo_edit_toggle_bookmark (moo_edit_view_get_doc (view),
                              moo_text_view_get_cursor_line (GTK_TEXT_VIEW (view)));
}


static void
action_next_bookmark (MooEditWindow *window)
{
    int cursor;
    GSList *bookmarks;
    MooEditView *view = ACTIVE_VIEW (window);
    MooEdit *doc = moo_edit_view_get_doc (view);

    g_return_if_fail (view != nullptr);

    cursor = moo_text_view_get_cursor_line (GTK_TEXT_VIEW (view));
    bookmarks = moo_edit_get_bookmarks_in_range (doc, cursor + 1, -1);

    if (bookmarks)
    {
        moo_edit_view_goto_bookmark (view, (MooEditBookmark*) bookmarks->data);
        g_slist_free (bookmarks);
    }
}


static void
action_prev_bookmark (MooEditWindow *window)
{
    int cursor;
    GSList *bookmarks = nullptr;
    MooEditView *view = ACTIVE_VIEW (window);
    MooEdit *doc = moo_edit_view_get_doc (view);

    g_return_if_fail (view != nullptr);

    cursor = moo_text_view_get_cursor_line (GTK_TEXT_VIEW (view));

    if (cursor > 0)
        bookmarks = moo_edit_get_bookmarks_in_range (doc, 0, cursor - 1);

    if (bookmarks)
    {
        GSList *last = g_slist_last (bookmarks);
        moo_edit_view_goto_bookmark (view, (MooEditBookmark*) last->data);
        g_slist_free (bookmarks);
    }
}


static void
goto_bookmark_activated (GtkAction *action,
                         gpointer   data)
{
    MooEditView *view;
    MooEditWindow *window;
    MooEditBookmark *bk;
    guint n = GPOINTER_TO_UINT (data);

    window = (MooEditWindow*) _moo_action_get_window (action);
    g_return_if_fail (window != nullptr);

    view = ACTIVE_VIEW (window);
    g_return_if_fail (view != nullptr);

    if ((bk = moo_edit_get_bookmark (moo_edit_view_get_doc (view), n)))
        moo_edit_view_goto_bookmark (view, bk);
}

static GtkAction *
create_goto_bookmark_action (MooWindow *window,
                             gpointer   data)
{
    GtkAction *action;
    guint n = GPOINTER_TO_UINT (data);
    char *accel;
    char *name;

    name = g_strdup_printf (MOO_EDIT_GOTO_BOOKMARK_ACTION "%u", n);
    accel = g_strdup_printf ("<ctrl>%u", n);

    action = GTK_ACTION (g_object_new (MOO_TYPE_ACTION, "name", name, "default-accel", accel,
                                       "connect-accel", TRUE, "accel-editable", FALSE,
                                       (const char*) nullptr));
    g_signal_connect (action, "activate", G_CALLBACK (goto_bookmark_activated), data);
    moo_bind_bool_property (action, "sensitive", window, "has-open-document", FALSE);

    g_free (name);
    g_free (accel);
    return action;
}


namespace {

const ObjectDataAccessor<GtkWidget, MooEditBookmark*> widget_bookmark("moo-bookmark");
const ObjectDataAccessor<GtkWidget, MooEdit*> widget_doc("moo-edit");
const ObjectDataAccessor<GtkWidget, MooEditView*> widget_view("moo-edit-view");
const ObjectDataAccessor<GtkWidget, MooEditTab*> data_moo_edit_tab("moo-edit-tab");

} // anonymous namespace

static void
bookmark_item_activated (GtkWidget *item)
{
    moo_edit_view_goto_bookmark (widget_view.get(item), widget_bookmark.get(item));
}

static GtkWidget *
create_bookmark_item (MooEditWindow   *window,
                      MooEditView     *view,
                      MooEditBookmark *bk)
{
    char *label, *bk_text;
    GtkWidget *item = nullptr;
    MooEdit *doc = moo_edit_view_get_doc (view);

    bk_text = _moo_edit_bookmark_get_text (bk);
    label = g_strdup_printf ("%d - \"%s\"", 1 + moo_line_mark_get_line (MOO_LINE_MARK (bk)),
                             MOO_NZS (bk_text));
    g_free (bk_text);

    if (bk->no)
    {
        GtkAction *action;
        char *action_name;

        action_name = g_strdup_printf (MOO_EDIT_GOTO_BOOKMARK_ACTION "%u", bk->no);
        action = moo_window_get_action (MOO_WINDOW (window), action_name);

        if (action)
        {
            g_object_set (action, "label", label, "use-underline", FALSE, nullptr);
            item = gtk_action_create_menu_item (action);
        }
        else
        {
            g_critical ("oops");
        }

        g_free (action_name);
    }

    if (!item)
    {
        item = gtk_menu_item_new_with_label (label);
        g_signal_connect (item, "activate", G_CALLBACK (bookmark_item_activated), nullptr);
    }

    widget_bookmark.set(item, g::object_ref (bk), g_object_unref);
    widget_doc.set(item, g::object_ref (doc), g_object_unref);
    widget_view.set(item, g::object_ref (view), g_object_unref);

    g_free (label);

    return item;
}

static void
populate_bookmark_menu (MooEditWindow *window,
                        GtkWidget     *menu,
                        GtkWidget     *next_bk_item)
{
    GtkAction *pn;
    MooEdit *doc;
    MooEditView *view;
    GtkWidget *item;
    const GSList *bookmarks;
    GList *children, *l;
    int pos;

    children = g_list_copy (GTK_MENU_SHELL (menu)->children);
    pos = g_list_index (children, next_bk_item);
    g_return_if_fail (pos >= 0);

    for (l = g_list_nth (children, pos + 1); l != nullptr; l = l->next)
    {
        if (g_object_get_data ((GObject*) l->data, "moo-bookmark-menu-item"))
            gtk_container_remove (GTK_CONTAINER (menu), (GtkWidget*) l->data);
        else
            break;
    }

    g_list_free (children);

    view = ACTIVE_VIEW (window);
    doc = view ? moo_edit_view_get_doc (view) : nullptr;
    bookmarks = doc ? moo_edit_list_bookmarks (doc) : nullptr;

    pn = moo_window_get_action (MOO_WINDOW (window), "PreviousBookmark");
    gtk_action_set_sensitive (pn, bookmarks != nullptr);
    pn = moo_window_get_action (MOO_WINDOW (window), "NextBookmark");
    gtk_action_set_sensitive (pn, bookmarks != nullptr);

    if (bookmarks)
    {
        item = gtk_separator_menu_item_new ();
        gtk_widget_show (item);
        g_object_set_data (G_OBJECT (item), "moo-bookmark-menu-item", GINT_TO_POINTER (TRUE));
        gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, ++pos);
    }

    while (bookmarks)
    {
        item = create_bookmark_item (window, view, (MooEditBookmark*) bookmarks->data);
        gtk_widget_show (item);
        g_object_set_data (G_OBJECT (item), "moo-bookmark-menu-item", GINT_TO_POINTER (TRUE));
        gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, ++pos);
        bookmarks = bookmarks->next;
    }
}

static void
doc_item_selected (MooWindow   *window,
                   GtkMenuItem *doc_item)
{
    GtkWidget *menu;
    GtkWidget *next_bk_item;

    menu = gtk_menu_item_get_submenu (doc_item);
    next_bk_item = moo_ui_xml_get_widget (moo_window_get_ui_xml (window),
                                          window->menubar,
                                          "Editor/Menubar/Document/NextBookmark");
    g_return_if_fail (menu != nullptr && next_bk_item != nullptr);

    populate_bookmark_menu (MOO_EDIT_WINDOW (window), menu, next_bk_item);
}


static void
moo_edit_window_connect_menubar (MooWindow *window)
{
    GtkWidget *doc_item;
    GtkWidget *win_item;

    if (!window->menubar)
        return;

    doc_item = moo_ui_xml_get_widget (moo_window_get_ui_xml (window),
                                      window->menubar,
                                      "Editor/Menubar/Document");
    g_return_if_fail (doc_item != nullptr);
    g_signal_connect_swapped (doc_item, "select",
                              G_CALLBACK (doc_item_selected),
                              window);

    win_item = moo_ui_xml_get_widget (moo_window_get_ui_xml (window),
                                      window->menubar,
                                      "Editor/Menubar/Window");
    g_return_if_fail (win_item != nullptr);
    g_signal_connect_swapped (win_item, "select",
                              G_CALLBACK (window_menu_item_selected),
                              window);
}


#ifdef ENABLE_PRINTING
static void
action_page_setup (MooEditWindow *window)
{
    _moo_edit_page_setup (GTK_WIDGET (window));
}


static void
action_print (MooEditWindow *window)
{
    MooEditView *view = ACTIVE_VIEW (window);
    g_return_if_fail (view != nullptr);
    _moo_edit_print (GTK_TEXT_VIEW (view), GTK_WIDGET (window));
}


static void
action_print_preview (MooEditWindow *window)
{
    MooEditView *view = ACTIVE_VIEW (window);
    g_return_if_fail (view != nullptr);
    _moo_edit_print_preview (GTK_TEXT_VIEW (view), GTK_WIDGET (window));
}


static void
action_print_pdf (MooEditWindow *window)
{
    char *start_name;
    const char *doc_name, *dot;
    const char *filename;
    MooEditView *view = ACTIVE_VIEW (window);
    MooEdit *doc = view ? moo_edit_view_get_doc (view) : nullptr;

    g_return_if_fail (view != nullptr);

    doc_name = doc ? moo_edit_get_display_basename (doc) : "output";
    dot = strrchr (doc_name, '.');

    if (dot && dot != doc_name)
    {
        start_name = g_new (char, (dot - doc_name) + 5);
        memcpy (start_name, doc_name, dot - doc_name);
        memcpy (start_name + (dot - doc_name), ".pdf", 5);
    }
    else
    {
        start_name = g_strdup_printf ("%s.pdf", doc_name);
    }

    filename = moo_file_dialogp (GTK_WIDGET (window),
                                 MOO_FILE_DIALOG_SAVE,
                                 start_name,
                                 _("Export as PDF"),
                                 moo_edit_setting (MOO_EDIT_PREFS_PDF_LAST_DIR),
                                 nullptr);

    if (filename)
        _moo_edit_export_pdf (GTK_TEXT_VIEW (view), filename);

    g_free (start_name);
}
#endif


static void
wrap_text_toggled (MooEditWindow *window,
                   gboolean       active)
{
    MooEditView *view = ACTIVE_VIEW (window);
    g_return_if_fail (view != nullptr);
    _moo_edit_view_ui_set_line_wrap (view, active);
}


static void
line_numbers_toggled (MooEditWindow *window,
                      gboolean       active)
{
    MooEditView *view = ACTIVE_VIEW (window);
    g_return_if_fail (view != nullptr);
    _moo_edit_view_ui_set_show_line_numbers (view, active);
}


/****************************************************************************/
/* Notebook popup menu
 */

static void
close_activated (GtkWidget     *item,
                 MooEditWindow *window)
{
    MooEdit *doc = widget_doc.get(item);
    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT (doc));
    moo_editor_close_doc (window->priv->editor, doc);
}


static void
close_others_activated (GtkWidget     *item,
                        MooEditWindow *window)
{
    MooEditArray *others;
    MooEdit *doc = widget_doc.get(item);

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT (doc));

    others = moo_edit_window_get_docs (window);
    moo_edit_array_remove (others, doc);

    if (!moo_edit_array_is_empty (others))
        moo_editor_close_docs (window->priv->editor, others);

    moo_edit_array_free (others);
}


static void
detach_activated (GtkWidget     *item,
                  MooEditWindow *window)
{
    MooEdit *doc = widget_doc.get(item);

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT (doc));

    _moo_editor_move_doc (window->priv->editor, doc, nullptr, nullptr, TRUE);
}


static void
move_to_split_notebook_activated (GtkWidget     *item,
                                  MooEditWindow *window)
{
    MooEditTab *tab = data_moo_edit_tab.get(item);
    move_tab_to_split_view (window, tab);
}


static void
copy_full_path_activated (GtkWidget     *item,
                          G_GNUC_UNUSED MooEditWindow *window)
{
    MooEdit *doc = widget_doc.get(item);
    GtkClipboard *clipboard = gtk_widget_get_clipboard(item, GDK_SELECTION_CLIPBOARD);
	char *filename = moo_edit_get_filename(doc);
	gtk_clipboard_set_text(clipboard, filename, -1);
	g_free(filename);
}


#ifdef __WIN32__

static void
open_containing_folder_activated (GtkWidget     *item,
                                  G_GNUC_UNUSED MooEditWindow *window)
{
	MooEdit *doc = widget_doc.get(item);
	char **argv = g_new0(char*, 4);
	argv[0] = g_strdup("explorer");
	argv[1] = g_strdup("/select,");
	argv[2] = moo_edit_get_filename(doc);
	g_spawn_async(nullptr, argv, nullptr, G_SPAWN_SEARCH_PATH, nullptr, nullptr, nullptr, nullptr);
	g_strfreev(argv);
}

#endif // __WIN32__


/****************************************************************************/
/* Documents
 */

namespace {
const ObjectDataAccessor<GtkWidget, MooNotebook*> data_notebook("moo-notebook");
const ObjectDataAccessor<MooEdit, GtkAction*> data_doc_list_action("moo-doc-list-action");
} // namespace

static gboolean
can_move_to_split_notebook (MooEditWindow *window)
{
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), FALSE);
    return moo_edit_window_get_n_tabs (window) > 1;
}

static void
move_tab_to_split_view (MooEditWindow *window,
                        MooEditTab    *tab)
{
    MooNotebook *nb1;
    MooNotebook *nb2;
    MooNotebook *old_nb, *new_nb;
    GtkWidget *label;
    MooEdit *doc;
    int n_pages1, n_pages2;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT_TAB (tab));

    doc = moo_edit_tab_get_doc (tab);
    old_nb = MOO_NOTEBOOK (gtk_widget_get_parent (GTK_WIDGET (tab)));
    g_assert (MOO_IS_NOTEBOOK (old_nb) &&
              gtk_widget_get_toplevel (GTK_WIDGET (old_nb)) == GTK_WIDGET (window));

    nb1 = get_notebook (window, 0);
    nb2 = get_notebook (window, 1);
    g_return_if_fail (nb1 && nb2);

    new_nb = old_nb == nb1 ? nb2 : nb1;

    n_pages1 = moo_notebook_get_n_pages (nb1);
    n_pages2 = moo_notebook_get_n_pages (nb2);

    if (new_nb == nb1)
    {
        n_pages1 += 1;
        n_pages2 -= 1;
    }
    else
    {
        n_pages1 -= 1;
        n_pages2 += 1;
    }

    if (!n_pages1)
        gtk_widget_hide (GTK_WIDGET (nb1));
    if (!n_pages2)
        gtk_widget_hide (GTK_WIDGET (nb2));
    if (n_pages1)
        show_notebook (window, nb1);
    if (n_pages2)
        show_notebook (window, nb2);

    g_object_ref (tab);

    window->priv->active_tab = nullptr;

    gtk_container_remove (GTK_CONTAINER (old_nb), GTK_WIDGET (tab));
    label = create_tab_label (window, tab, doc);
    gtk_widget_show (label);
    moo_notebook_insert_page (new_nb, GTK_WIDGET (tab), label,
                              moo_notebook_get_current_page (new_nb) + 1);

    moo_edit_window_set_active_view (window, moo_edit_tab_get_active_view (tab));
    edit_changed (window, doc);

    g_object_unref (tab);
}

void
_moo_edit_window_set_active_tab (MooEditWindow *window,
                                 MooEditTab    *tab)
{
    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT_TAB (tab));
    g_return_if_fail (moo_edit_tab_get_window (tab) == window);
    window->priv->active_tab = tab;
}

static void
notebook_switch_page (MooNotebook   *notebook,
                      guint          page_num,
                      MooEditWindow *window)
{
    if (notebook == get_active_notebook (window))
    {
        edit_changed (window, get_nth_doc (notebook, page_num));
        moo_edit_window_check_actions (window);
        moo_edit_window_update_doc_list (window);
        g_object_notify (G_OBJECT (window), "active-doc");
    }
}


static void
add_tab_menu_item (const char       *label,
                   MooEdit          *doc,
                   MooEditWindow    *window,
                   GtkMenu          *menu,
                   MooEditTab       *tab,
                   GCallback         cb,
                   gboolean          enabled)
{
    /* Item in document tab context menu */
    GtkWidget* item = gtk_menu_item_new_with_label(label);
    gtk_widget_show(item);
    gtk_widget_set_sensitive(item, enabled);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    widget_doc.set(item, doc);
    data_moo_edit_tab.set(item, tab);
    g_signal_connect(item, "activate", cb, window);
}

static void
add_separator (GtkMenu  *menu)
{
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),
                          GTK_WIDGET(g_object_new(GTK_TYPE_SEPARATOR_MENU_ITEM,
                                                  "visible", TRUE,
                                                  (const char*) nullptr)));
}

static gboolean
notebook_populate_popup (MooEditWindow      *window,
                         GtkWidget          *child,
                         GtkMenu            *menu)
{
    MooEdit *doc;
    MooEditView *view;

    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), TRUE);
    g_return_val_if_fail (MOO_IS_EDIT_TAB (child), TRUE);

    view = moo_edit_tab_get_active_view (MOO_EDIT_TAB (child));
    g_return_val_if_fail (MOO_IS_EDIT_VIEW (view), TRUE);

    doc = moo_edit_view_get_doc (view);

    /* Item in document tab context menu */
    add_tab_menu_item(C_("tab-context-menu", "Close"),
                      doc, window, menu, MOO_EDIT_TAB (child),
                      G_CALLBACK (close_activated),
                      TRUE);

    if (moo_edit_window_get_n_tabs (window) > 1)
    {
        /* Item in document tab context menu */
        add_tab_menu_item(C_("tab-context-menu", "Close All Others"),
                          doc, window, menu, MOO_EDIT_TAB (child),
                          G_CALLBACK (close_others_activated),
                          TRUE);
    }

    add_separator (menu);

    /* Item in document tab context menu */
    add_tab_menu_item(C_("tab-context-menu", "Copy Full Path"),
                      doc, window, menu, MOO_EDIT_TAB (child),
                      G_CALLBACK(copy_full_path_activated),
                      !moo_edit_is_untitled (doc));

#ifdef __WIN32__
    /* Item in document tab context menu */
    add_tab_menu_item(C_("tab-context-menu", "Open Containing Folder"),
                      doc, window, menu, MOO_EDIT_TAB (child),
                      G_CALLBACK(open_containing_folder_activated),
                      !moo_edit_is_untitled(doc));
#endif // __WIN32__

    if (moo_edit_window_get_n_tabs (window) > 1)
    {
        add_separator (menu);

        /* Item in document tab context menu */
        add_tab_menu_item(C_("tab-context-menu", "Detach"),
                          doc, window, menu, MOO_EDIT_TAB (child),
                          G_CALLBACK (detach_activated),
                          TRUE);

        /* Item in document tab context menu */
        add_tab_menu_item(C_("tab-context-menu", "Move to Split Notebook"),
                          doc, window, menu, MOO_EDIT_TAB (child),
                          G_CALLBACK (move_to_split_notebook_activated),
                          TRUE);
    }

    return FALSE;
}


static gboolean
notebook_button_press (MooNotebook    *notebook,
                       GdkEventButton *event,
                       MooEditWindow  *window)
{
    int n;

    if (event->button != 2 || event->type != GDK_BUTTON_PRESS)
        return FALSE;

    n = moo_notebook_get_event_tab (notebook, (GdkEvent*) event);

    if (n < 0)
        return FALSE;

    moo_editor_close_doc (window->priv->editor,
                          get_nth_doc (notebook, n));

    return TRUE;
}


static void
set_use_tabs (MooEditWindow *window,
              MooNotebook   *notebook)
{
    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (notebook != nullptr);
    g_object_set (notebook, "show-tabs",
                  moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_USE_TABS)), nullptr);
}

void
_moo_edit_window_set_use_tabs (void)
{
    for (MooEditWindow* w: windows)
    {
        guint i;
        for (i = 0; i < 2; ++i)
            set_use_tabs (w, get_notebook (w, i));
    }
}


static MooNotebook *
get_notebook (MooEditWindow *window,
              int            i)
{
    g_return_val_if_fail (i >= 0 && i < (int) window->priv->notebooks.size(), nullptr);
    return window->priv->notebooks[i].get();
}

static MooEditView *
get_notebook_active_view (MooNotebook *notebook)
{
    GtkWidget *tab;
    int page;

    g_return_val_if_fail (notebook != nullptr, nullptr);

    page = moo_notebook_get_current_page (notebook);

    if (page < 0)
        return nullptr;

    tab = moo_notebook_get_nth_page (notebook, page);
    return moo_edit_tab_get_active_view (MOO_EDIT_TAB (tab));
}

static MooEdit *
get_notebook_active_doc (MooNotebook *notebook)
{
    MooEditView *view = get_notebook_active_view (notebook);
    return view ? moo_edit_view_get_doc (view) : nullptr;
}

static MooNotebook *
get_active_notebook (MooEditWindow *window)
{
    MooNotebook *nb1, *nb2;

    nb1 = get_notebook (window, 0);
    nb2 = get_notebook (window, 1);

    g_return_val_if_fail (nb2 != nullptr, nb1);
    g_return_val_if_fail (nb1 != nullptr, nb2);

    if (!GTK_WIDGET_VISIBLE (nb2))
        return nb1;
    else if (!GTK_WIDGET_VISIBLE (nb1))
        return nb2;

    if (window->priv->active_tab)
        return MOO_NOTEBOOK (gtk_widget_get_parent (GTK_WIDGET (window->priv->active_tab)));

    if (moo_notebook_get_n_pages (nb1) > 0)
        return nb1;
    else if (moo_notebook_get_n_pages (nb2) > 0)
        return nb2;
    else
        return nb1;
}


static gboolean
both_notebooks_visible (MooEditWindow *window)
{
    return window->priv->notebooks.size() == 2 &&
           GTK_WIDGET_VISIBLE (window->priv->notebooks[0].get()) &&
           GTK_WIDGET_VISIBLE (window->priv->notebooks[1].get());
}

static void
save_doc_paned_config (MooEditWindow *window)
{
    if (both_notebooks_visible (window))
    {
        int pos = gtk_paned_get_position (GTK_PANED (window->priv->doc_paned));
        int old_pos = moo_prefs_get_int (PREFS_KEY_SPLIT_POS);
        if (pos > 0 && (old_pos == 0 || old_pos != pos))
            moo_prefs_set_int (PREFS_KEY_SPLIT_POS, pos);
    }
}

static void
show_notebook (MooEditWindow *window,
               MooNotebook   *notebook)
{
    if (!GTK_WIDGET_VISIBLE (notebook))
    {
        gtk_widget_show (GTK_WIDGET (notebook));
        if (both_notebooks_visible (window))
        {
            int pos = moo_prefs_get_int (PREFS_KEY_SPLIT_POS);
            if (pos > 0)
                gtk_paned_set_position (GTK_PANED (window->priv->doc_paned), pos);
        }
    }
}

static void
notebook_close_button_clicked (GtkWidget     *button,
                               MooEditWindow *window)
{
    MooNotebook *notebook = data_notebook.get(button);
    g_return_if_fail (notebook != nullptr);
    moo_editor_close_doc (window->priv->editor, get_notebook_active_doc (notebook));
}

static void
setup_notebook (MooEditWindow *window,
                MooNotebook   *notebook)
{
    GtkWidget *button, *icon, *frame;

    set_use_tabs (window, notebook);

    g_signal_connect_after (notebook, "moo-switch-page",
                            G_CALLBACK (notebook_switch_page), window);
    g_signal_connect_swapped (notebook, "populate-popup",
                              G_CALLBACK (notebook_populate_popup), window);
    g_signal_connect (notebook, "button-press-event",
                      G_CALLBACK (notebook_button_press), window);

    frame = gtk_aspect_frame_new (nullptr, 0.5, 0.5, 1.0, FALSE);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

    button = gtk_button_new ();
    gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
    data_notebook.set(button, notebook);
    g_signal_connect (button, "clicked",
                      G_CALLBACK (notebook_close_button_clicked),
                      window);
    moo_bind_bool_property (button, "sensitive", window, "has-open-document", FALSE);

    icon = _moo_create_small_icon (MOO_SMALL_ICON_CLOSE);

    gtk_container_add (GTK_CONTAINER (button), icon);
    gtk_container_add (GTK_CONTAINER (frame), button);
    gtk_widget_show_all (frame);
    moo_notebook_set_action_widget (notebook, frame, TRUE);

    gtk_drag_dest_set (GTK_WIDGET (notebook), (GtkDestDefaults) 0,
                       dest_targets, G_N_ELEMENTS (dest_targets),
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
    gtk_drag_dest_add_text_targets (GTK_WIDGET (notebook));
    g_signal_connect (notebook, "drag-motion",
                      G_CALLBACK (notebook_drag_motion), window);
    g_signal_connect (notebook, "drag-drop",
                      G_CALLBACK (notebook_drag_drop), window);
    g_signal_connect (notebook, "drag-data-received",
                      G_CALLBACK (notebook_drag_data_recv), window);
}


static void
edit_changed (MooEditWindow *window,
              MooEdit       *doc)
{
    if (doc == ACTIVE_DOC (window))
    {
        g_object_freeze_notify (G_OBJECT (window));
        g_object_notify (G_OBJECT (window), "can-reload");
        g_object_notify (G_OBJECT (window), "has-open-document");
        g_object_notify (G_OBJECT (window), "has-comments");
        g_object_thaw_notify (G_OBJECT (window));

        update_split_view_actions (window);
        update_window_title (window);
        update_statusbar (window);
        update_lang_menu (window);
        update_doc_view_actions (window);
        update_doc_encoding_item (window);
	update_doc_line_end_item (window);
    }

    if (doc)
        update_tab_labels (window, doc);
}

static void
edit_encoding_changed (MooEditWindow *window,
                       G_GNUC_UNUSED GParamSpec *pspec,
                       MooEdit       *doc)
{
    if (doc == ACTIVE_DOC (window))
        update_doc_encoding_item (window);
}

static void
edit_line_end_changed (MooEditWindow *window,
                       G_GNUC_UNUSED GParamSpec *pspec,
                       MooEdit       *doc)
{
    if (doc == ACTIVE_DOC (window))
        update_doc_line_end_item (window);
}

static void
view_overwrite_changed (MooEditWindow *window,
                        G_GNUC_UNUSED GParamSpec *pspec,
                        MooEditView   *view)
{
    if (view == ACTIVE_VIEW (window))
        update_statusbar (window);
}

static void
view_cursor_moved (MooEditWindow *window,
                   G_GNUC_UNUSED GtkTextIter *iter,
                   MooEditView *view)
{
    if (view == ACTIVE_VIEW (window))
        update_statusbar (window);
}


static void
sync_proxies (GtkAction *action)
{
    GSList *l = gtk_action_get_proxies (action);
    while (l)
    {
        gtk_activatable_sync_action_properties ((GtkActivatable*) l->data, action);
        l = l->next;
    }
}


static void
view_wrap_mode_changed (MooEditWindow *window,
                        G_GNUC_UNUSED GParamSpec *pspec,
                        MooEditView   *view)
{
    GtkAction *action;
    GtkWrapMode mode;

    if (view != ACTIVE_VIEW (window))
        return;

    action = moo_window_get_action (MOO_WINDOW (window), "WrapText");
    g_return_if_fail (action != nullptr);

    g_object_get (view, "wrap-mode", &mode, nullptr);
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), mode != GTK_WRAP_NONE);

    /* XXX menu item and action go out of sync for some reason */
    sync_proxies (action);
}


static void
view_show_line_numbers_changed (MooEditWindow *window,
                                G_GNUC_UNUSED GParamSpec *pspec,
                                MooEditView   *view)
{
    GtkAction *action;
    gboolean show;

    if (view != ACTIVE_VIEW (window))
        return;

    action = moo_window_get_action (MOO_WINDOW (window), "LineNumbers");
    g_return_if_fail (action != nullptr);

    g_object_get (view, "show-line-numbers", &show, nullptr);
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), show);

    /* XXX menu item and action go out of sync for some reason */
    sync_proxies (action);
}


static void
update_doc_view_actions (MooEditWindow *window)
{
    MooEditView *view;

    view = ACTIVE_VIEW (window);

    if (!view)
        return;

    view_wrap_mode_changed (window, nullptr, view);
    view_show_line_numbers_changed (window, nullptr, view);
}


static void
edit_filename_changed (MooEditWindow      *window,
                       MooEdit            *doc)
{
    edit_changed (window, doc);
    moo_edit_window_update_doc_list (window);
}


static void
proxy_boolean_property (MooEditWindow *window,
                        GParamSpec    *prop,
                        MooEdit       *doc)
{
    if (doc == ACTIVE_DOC (window))
        g_object_notify (G_OBJECT (window), prop->name);
}


/**
 * moo_edit_window_get_active_doc:
 */
MooEdit *
moo_edit_window_get_active_doc (MooEditWindow  *window)
{
    MooEditView *view = moo_edit_window_get_active_view (window);
    return view ? moo_edit_view_get_doc (view) : nullptr;
}

/**
 * moo_edit_window_get_active_view:
 */
MooEditView *
moo_edit_window_get_active_view (MooEditWindow *window)
{
    MooEditTab *tab = moo_edit_window_get_active_tab (window);
    return tab ? moo_edit_tab_get_active_view (tab) : nullptr;
}

/**
 * moo_edit_window_get_active_tab:
 */
MooEditTab *
moo_edit_window_get_active_tab (MooEditWindow *window)
{
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), nullptr);

    if (window->priv->notebooks.empty())
        return nullptr;

    if (!window->priv->active_tab)
    {
        GtkWidget *tab;
        int page;
        MooNotebook *notebook = get_active_notebook (window);

        g_return_val_if_fail (notebook != nullptr, nullptr);

        page = moo_notebook_get_current_page (notebook);

        if (page < 0)
            return nullptr;

        tab = moo_notebook_get_nth_page (notebook, page);
        window->priv->active_tab = MOO_EDIT_TAB (tab);
    }

    return window->priv->active_tab;
}


/**
 * moo_edit_window_set_active_doc:
 */
void
moo_edit_window_set_active_doc (MooEditWindow *window,
                                MooEdit       *doc)
{
    MooEditView *view;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT (doc));

    view = moo_edit_get_view (doc);

    if (moo_edit_view_get_window (view) != window)
    {
        guint i;
        MooEditViewArray *views;

        view = nullptr;
        views = moo_edit_get_views (doc);

        for (i = 0; i < moo_edit_view_array_get_size (views); ++i)
        {
            MooEditView *tmp = views->elms[i];
            if (moo_edit_view_get_window (view) == window)
            {
                view = tmp;
                break;
            }
        }

        moo_edit_view_array_free (views);
    }

    g_return_if_fail (view != nullptr);

    moo_edit_window_set_active_view (window, view);
}

/**
 * moo_edit_window_set_active_view:
 */
void
moo_edit_window_set_active_view (MooEditWindow *window,
                                 MooEditView   *view)
{
    int page;
    MooNotebook *notebook = nullptr;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT_VIEW (view));

    page = get_view_page_num (window, view, &notebook);
    g_return_if_fail (page >= 0);

    window->priv->active_tab = moo_edit_view_get_tab (view);
    moo_notebook_set_current_page (notebook, page);
    gtk_widget_grab_focus (GTK_WIDGET (view));
}

/**
 * moo_edit_window_set_active_tab:
 */
void
moo_edit_window_set_active_tab (MooEditWindow *window,
                                MooEditTab    *tab)
{
    MooEditView *view = moo_edit_tab_get_active_view (tab);
    moo_edit_window_set_active_view (window, view);
}


/**
 * moo_edit_window_get_docs:
 *
 * Returns: (transfer full)
 */
MooEditArray *
moo_edit_window_get_docs (MooEditWindow *window)
{
    MooEditTabArray *tabs;
    MooEditArray *docs;
    guint i;

    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), nullptr);

    tabs = moo_edit_window_get_tabs (window);
    g_return_val_if_fail (tabs != nullptr, nullptr);

    docs = moo_edit_array_new ();

    for (i = 0; i < tabs->n_elms; i++)
    {
        MooEdit *doc = moo_edit_tab_get_doc (tabs->elms[i]);
        if (moo_edit_array_find (docs, doc) < 0)
            moo_edit_array_append (docs, doc);
    }

    moo_edit_tab_array_free (tabs);
    return docs;
}

/**
 * moo_edit_window_get_views:
 *
 * Returns: (transfer full)
 */
MooEditViewArray *
moo_edit_window_get_views (MooEditWindow *window)
{
    int inb;
    MooEditViewArray *views;

    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), nullptr);

    views = moo_edit_view_array_new ();

    for (inb = 0; inb < 2; inb++)
    {
        int ipage;
        MooNotebook *notebook = get_notebook (window, inb);
        int n_pages = moo_notebook_get_n_pages (notebook);
        for (ipage = 0; ipage < n_pages; ipage++)
        {
            MooEditTab *tab = get_nth_tab (*notebook, ipage);
            MooEditViewArray *tab_views = moo_edit_tab_get_views (tab);
            moo_edit_view_array_append_array (views, tab_views);
            moo_edit_view_array_free (tab_views);
        }
    }

    return views;
}

/**
 * moo_edit_window_get_tabs:
 *
 * Returns: (transfer full)
 */
MooEditTabArray *
moo_edit_window_get_tabs (MooEditWindow *window)
{
    int inb;
    MooEditTabArray *tabs;

    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), nullptr);

    tabs = moo_edit_tab_array_new ();

    for (inb = 0; inb < 2; inb++)
    {
        int ipage;
        MooNotebook *notebook = get_notebook (window, inb);
        int n_pages = moo_notebook_get_n_pages (notebook);
        for (ipage = 0; ipage < n_pages; ipage++)
            moo_edit_tab_array_append (tabs, get_nth_tab (*notebook, ipage));
    }

    return tabs;
}


/**
 * moo_edit_window_get_n_tabs:
 */
int
moo_edit_window_get_n_tabs (MooEditWindow *window)
{
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), 0);

    int n_docs = 0;

    for (const auto& notebook: window->priv->notebooks)
        n_docs += moo_notebook_get_n_pages (notebook.get());

    return n_docs;
}


static MooEdit *
get_nth_doc (MooNotebook *notebook,
             guint        n)
{
    MooEditTab *tab = get_nth_tab (*notebook, n);
    return tab ? moo_edit_tab_get_doc (tab) : nullptr;
}

static MooEditTab *
get_nth_tab (MooNotebook &notebook,
             guint        n)
{
    GtkWidget *tab = moo_notebook_get_nth_page (&notebook, n);
    return tab ? MOO_EDIT_TAB (tab) : nullptr;
}

static int
get_active_tab (MooEditWindow *window)
{
    int tab = 0;
    MooNotebook *notebook = get_active_notebook (window);

    g_return_val_if_fail (notebook != nullptr, -1);

    for (const auto& nb: window->priv->notebooks)
    {
        if (nb == notebook)
        {
            tab += moo_notebook_get_current_page (notebook);
            return tab;
        }

        tab += moo_notebook_get_n_pages (nb.get());
    }

    g_return_val_if_reached (-1);
}

static int
get_view_page_num (MooEditWindow  *window,
                   MooEditView    *view,
                   MooNotebook   **pnotebook)
{
    int i;
    MooEditTab *tab;

    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), -1);
    g_return_val_if_fail (MOO_IS_EDIT_VIEW (view), -1);

    tab = moo_edit_view_get_tab (view);

    for (i = 0; i < 2; ++i)
    {
        MooNotebook *notebook = get_notebook (window, i);
        int page = moo_notebook_page_num (notebook, GTK_WIDGET (tab));
        if (page >= 0)
        {
            if (pnotebook)
                *pnotebook = notebook;
            return page;
        }
    }

    if (pnotebook)
        *pnotebook = nullptr;
    return -1;
}

static int
get_tab_num (MooEditWindow *window,
             MooEditTab    *tab)
{
    int i;
    int pages;

    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), -1);
    g_return_val_if_fail (MOO_IS_EDIT_TAB (tab), -1);

    for (i = 0, pages = 0; i < 2; ++i)
    {
        MooNotebook *notebook = get_notebook (window, i);
        int page = moo_notebook_page_num (notebook, GTK_WIDGET (tab));
        if (page >= 0)
            return pages + page;
        pages += moo_notebook_get_n_pages (notebook);
    }

    return -1;
}

static int
get_doc_num (MooEditWindow *window,
             MooEdit       *doc)
{
    return get_tab_num (window, moo_edit_get_tab (doc));
}


static void
connect_view (MooEditWindow *window,
              MooEditView   *view)
{
    g_signal_connect_swapped (view, "notify::overwrite",
                              G_CALLBACK (view_overwrite_changed), window);
    g_signal_connect_swapped (view, "notify::wrap-mode",
                              G_CALLBACK (view_wrap_mode_changed), window);
    g_signal_connect_swapped (view, "notify::show-line-numbers",
                              G_CALLBACK (view_show_line_numbers_changed), window);
    g_signal_connect_swapped (view, "cursor-moved",
                              G_CALLBACK (view_cursor_moved), window);
}

static void
disconnect_view (MooEditWindow *window,
                 MooEditView   *view)
{
    g_signal_handlers_disconnect_by_func (view, (gpointer) view_cursor_moved, window);
    g_signal_handlers_disconnect_by_func (view, (gpointer) view_overwrite_changed, window);
    g_signal_handlers_disconnect_by_func (view, (gpointer) view_wrap_mode_changed, window);
    g_signal_handlers_disconnect_by_func (view, (gpointer) view_show_line_numbers_changed, window);
}


void
_moo_edit_window_insert_doc (MooEditWindow *window,
                             MooEdit       *doc,
                             MooEditView   *after_view)
{
    MooEditTab *tab;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (!moo_edit_get_window (doc));
    g_return_if_fail (!after_view || moo_edit_view_get_window (after_view) == window);

    g_assert (moo_edit_get_n_views (doc) == 1);

    tab = _moo_edit_tab_new (doc);
    _moo_edit_window_insert_tab (window, tab, after_view);
}

void
_moo_edit_window_insert_tab (MooEditWindow *window,
                             MooEditTab    *tab,
                             MooEditView   *after_view)
{
    GtkWidget *label;
    MooEdit *doc;
    MooEditViewArray *views;
    MooNotebook *notebook = nullptr;
    int page = -1;
    guint i;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT_TAB (tab));
    g_return_if_fail (!moo_edit_tab_get_window (tab));
    g_return_if_fail (!after_view || moo_edit_view_get_window (after_view) == window);

    doc = moo_edit_tab_get_doc (tab);
    views = moo_edit_get_views (doc);

    label = create_tab_label (window, tab, doc);
    gtk_widget_show (label);

    if (after_view)
    {
        page = get_view_page_num (window, after_view, &notebook);
        if (page < 0)
            g_critical ("oops");
    }

    if (page < 0)
    {
        notebook = get_active_notebook (window);
        page = moo_notebook_get_current_page (notebook) + 1;
    }

    show_notebook (window, notebook);
    moo_notebook_insert_page (notebook, GTK_WIDGET (tab), label, page);

    g_signal_connect_swapped (doc, "doc_status_changed",
                              G_CALLBACK (edit_changed), window);
    g_signal_connect_swapped (doc, "notify::encoding",
                              G_CALLBACK (edit_encoding_changed), window);
    g_signal_connect_swapped (doc, "notify::line-end",
                              G_CALLBACK (edit_line_end_changed), window);
    g_signal_connect_swapped (doc, "filename_changed",
                              G_CALLBACK (edit_filename_changed), window);
    g_signal_connect_swapped (doc, "notify::has-comments",
                              G_CALLBACK (proxy_boolean_property), window);
    g_signal_connect_swapped (doc, "notify::lang",
                              G_CALLBACK (edit_lang_changed), window);

    for (i = 0; i < moo_edit_view_array_get_size (views); ++i)
        connect_view (window, views->elms[i]);

    g_object_ref (doc);

    moo_edit_window_update_doc_list (window);
    g_signal_emit (window, signals[NEW_DOC], 0, doc);

    _moo_doc_attach_plugins (window, doc);

    moo_edit_window_set_active_doc (window, doc);
    edit_changed (window, doc);
    gtk_widget_grab_focus (GTK_WIDGET (moo_edit_get_view (doc)));

    g_object_notify (G_OBJECT (window), "can-move-to-split-notebook");

    moo_edit_view_array_free (views);
}


void
_moo_edit_window_remove_doc (MooEditWindow *window,
                             MooEdit       *doc)
{
    int page;
    GtkAction *action;
    MooEditView *new_view;
    MooEditViewArray *views;
    gboolean had_focus = FALSE;
    MooNotebook *notebook = nullptr;
    MooEditTab *tab;
    guint i;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (MOO_IS_EDIT (doc));

    tab = moo_edit_get_tab (doc);
    g_return_if_fail (MOO_IS_EDIT_TAB (tab));

    views = moo_edit_get_views (doc);
    g_return_if_fail (moo_edit_view_array_get_size (views) > 0);

    page = get_view_page_num (window, views->elms[0], &notebook);
    g_return_if_fail (notebook != nullptr && page >= 0);

    if (tab == window->priv->active_tab)
        window->priv->active_tab = nullptr;

    for (i = 0; i < views->n_elms; ++i)
    {
        MooEditView *view = views->elms[i];
        had_focus = had_focus || GTK_WIDGET_HAS_FOCUS (view);
    }

    g_signal_emit (window, signals[CLOSE_DOC], 0, doc);

    g_signal_handlers_disconnect_by_func (doc, (gpointer) edit_changed, window);
    g_signal_handlers_disconnect_by_func (doc, (gpointer) edit_filename_changed, window);
    g_signal_handlers_disconnect_by_func (doc, (gpointer) proxy_boolean_property, window);
    g_signal_handlers_disconnect_by_func (doc, (gpointer) edit_lang_changed, window);

    for (i = 0; i < views->n_elms; ++i)
        disconnect_view (window, views->elms[i]);

    _moo_doc_detach_plugins (window, doc);

    action = data_doc_list_action.get(doc);

    if (action)
    {
        moo_action_collection_remove_action (moo_window_get_actions (MOO_WINDOW (window)), action);
        data_doc_list_action.set(doc, nullptr);
    }

    if (window->priv->enable_history)
    {
        window->priv->history = g_list_remove (window->priv->history, doc);
        window->priv->history_blocked = TRUE;
    }

    moo_edit_window_update_doc_list (window);

    moo_notebook_remove_page (notebook, page);

    if (moo_notebook_get_n_pages (notebook) == 0)
        gtk_widget_hide (GTK_WIDGET (notebook));

    if (window->priv->enable_history)
    {
        window->priv->history_blocked = FALSE;
        if (window->priv->history)
            moo_edit_window_set_active_doc (window, (MooEdit*) window->priv->history->data);
    }

    edit_changed (window, nullptr);

    g_signal_emit (window, signals[CLOSE_DOC_AFTER], 0);

    new_view = ACTIVE_VIEW (window);

    if (!new_view)
        moo_edit_window_check_actions (window);
    else if (had_focus)
        gtk_widget_grab_focus (GTK_WIDGET (new_view));

    g_object_freeze_notify (G_OBJECT (window));
    g_object_notify (G_OBJECT (window), "active-doc");
    g_object_notify (G_OBJECT (window), "can-move-to-split-notebook");
    g_object_thaw_notify (G_OBJECT (window));

    moo_edit_view_array_free (views);
    g_object_unref (doc);
}


typedef struct {
    int x;
    int y;
    gboolean drag_started;
} DragInfo;

namespace {

const ObjectDataAccessor<GtkWidget, GtkWidget*> data_moo_edit_icon("moo-edit-icon");
const ObjectDataAccessor<GtkWidget, GtkWidget*> data_moo_edit_icon_evbox("moo-edit-icon-evbox");
const ObjectDataAccessor<GtkWidget, GtkWidget*> data_moo_edit_label("moo-edit-label");
const ObjectDataAccessor<GtkWidget, DragInfo*> data_drag_info("moo-drag-info");

} // namespace

static gboolean tab_icon_button_press       (GtkWidget      *evbox,
                                             GdkEventButton *event,
                                             MooEditWindow  *window);
static gboolean tab_icon_button_release     (GtkWidget      *evbox,
                                             GdkEventButton *event,
                                             MooEditWindow  *window);
static gboolean tab_icon_motion_notify      (GtkWidget      *evbox,
                                             GdkEventMotion *event,
                                             MooEditWindow  *window);

static void     tab_icon_drag_begin         (GtkWidget      *evbox,
                                             GdkDragContext *context,
                                             MooEditWindow  *window);
static void     tab_icon_drag_data_get      (GtkWidget      *evbox,
                                             GdkDragContext *context,
                                             GtkSelectionData *data,
                                             guint           info,
                                             guint           time,
                                             MooEditWindow  *window);
static void     tab_icon_drag_end           (GtkWidget      *evbox,
                                             GdkDragContext *context,
                                             MooEditWindow  *window);

static gboolean
tab_icon_button_release (GtkWidget      *evbox,
                         G_GNUC_UNUSED GdkEventButton *event,
                         MooEditWindow  *window)
{
    data_drag_info.set(evbox, nullptr);
    g_signal_handlers_disconnect_by_func (evbox, (gpointer) tab_icon_button_release, window);
    g_signal_handlers_disconnect_by_func (evbox, (gpointer) tab_icon_motion_notify, window);
    return FALSE;
}


static void
tab_icon_start_drag (GtkWidget      *evbox,
                     GdkEvent       *event,
                     MooEditWindow  *window)
{
    GtkTargetList *targets;
    MooEdit *doc;

    doc = widget_doc.get(evbox);
    g_return_if_fail (MOO_IS_EDIT (doc));

    g_signal_connect (evbox, "drag-begin", G_CALLBACK (tab_icon_drag_begin), window);
    g_signal_connect (evbox, "drag-data-get", G_CALLBACK (tab_icon_drag_data_get), window);
    g_signal_connect (evbox, "drag-end", G_CALLBACK (tab_icon_drag_end), window);

    targets = gtk_target_list_new (nullptr, 0);

    gtk_target_list_add (targets,
                         moo_atom_uri_list (),
                         0, TARGET_URI_LIST);
    gtk_target_list_add (targets,
                         MOO_EDIT_TAB_ATOM,
                         GTK_TARGET_SAME_APP,
                         TARGET_MOO_EDIT_TAB);

    gtk_drag_begin (evbox, targets,
                    GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK,
                    1, (GdkEvent*) event);

    gtk_target_list_unref (targets);
}


static gboolean
tab_icon_motion_notify (GtkWidget      *evbox,
                        GdkEventMotion *event,
                        MooEditWindow  *window)
{
    DragInfo *info;

    info = data_drag_info.get(evbox);
    g_return_val_if_fail (info != nullptr, FALSE);

    if (info->drag_started)
        return TRUE;

    if (gtk_drag_check_threshold (evbox, info->x, info->y, (int) event->x, (int) event->y))
    {
        info->drag_started = TRUE;
        tab_icon_start_drag (evbox, (GdkEvent*) event, window);
    }

    return TRUE;
}


static gboolean
tab_icon_button_press (GtkWidget        *evbox,
                       GdkEventButton   *event,
                       MooEditWindow    *window)
{
    DragInfo *info;

    if (event->button != 1 || event->type != GDK_BUTTON_PRESS)
        return FALSE;

    info = g_new0 (DragInfo, 1);
    info->x = event->x;
    info->y = event->y;
    data_drag_info.set(evbox, info, g_free);

    g_signal_connect (evbox, "motion-notify-event", G_CALLBACK (tab_icon_motion_notify), window);
    g_signal_connect (evbox, "button-release-event", G_CALLBACK (tab_icon_button_release), window);

    return FALSE;
}


static void
tab_icon_drag_begin (GtkWidget      *evbox,
                     GdkDragContext *context,
                     G_GNUC_UNUSED MooEditWindow *window)
{
    GdkPixbuf *pixbuf;
    GtkImage *icon = GTK_IMAGE(data_moo_edit_icon.get(evbox));
    pixbuf = gtk_image_get_pixbuf (icon);
    gtk_drag_set_icon_pixbuf (context, pixbuf, 0, 0);
}


static void
tab_icon_drag_data_get (GtkWidget      *evbox,
                        G_GNUC_UNUSED GdkDragContext *context,
                        GtkSelectionData *data,
                        guint           info,
                        G_GNUC_UNUSED guint           time,
                        G_GNUC_UNUSED MooEditWindow  *window)
{
    MooEdit *doc = widget_doc.get(evbox);
    MooEditTab *tab = data_moo_edit_tab.get(evbox);

    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (MOO_IS_EDIT_TAB (tab));

    if (info == TARGET_MOO_EDIT_TAB)
    {
        moo_selection_data_set_pointer (data, MOO_EDIT_TAB_ATOM, tab);
    }
    else if (info == TARGET_URI_LIST)
    {
        const char *uris[] = { nullptr, nullptr };
        gstr uri = gstr::take (moo_edit_get_uri (doc));
        if (!uri.empty())
            uris[0] = uri.get();
        gtk_selection_data_set_uris (data, (char**) uris);
    }
    else
    {
        g_critical ("drag-data-get oops");
        gtk_selection_data_set_text (data, "", -1);
    }
}


static void
tab_icon_drag_end (GtkWidget      *evbox,
                   G_GNUC_UNUSED GdkDragContext *context,
                   MooEditWindow  *window)
{
    data_drag_info.set(evbox, nullptr);
    g_signal_handlers_disconnect_by_func (evbox, (gpointer) tab_icon_drag_begin, window);
    g_signal_handlers_disconnect_by_func (evbox, (gpointer) tab_icon_drag_data_get, window);
    g_signal_handlers_disconnect_by_func (evbox, (gpointer) tab_icon_drag_end, window);
}


static void
tab_close_button_clicked (MooEdit *doc)
{
    moo_edit_close (doc);
}

static GtkWidget *
create_tab_label (MooEditWindow *window,
                  MooEditTab    *tab,
                  MooEdit       *doc)
{
    GtkWidget *hbox, *icon, *label, *evbox;
    GtkSizeGroup *group;

    group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

    hbox = gtk_hbox_new (FALSE, 3);
    gtk_widget_show (hbox);

    evbox = gtk_event_box_new ();
    gtk_event_box_set_visible_window (GTK_EVENT_BOX (evbox), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), evbox, FALSE, FALSE, 0);

    icon = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (evbox), icon);
    gtk_widget_show_all (evbox);

    label = gtk_label_new (moo_edit_get_display_basename (doc));
    gtk_label_set_single_line_mode (GTK_LABEL (label), TRUE);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    if (0)
    {
        GtkWidget *frame;
        GtkWidget *button;
        GtkWidget *icon;

        {
            static gboolean been_here;
            if (!been_here)
            {
                been_here = TRUE;
                gtk_rc_parse_string (
                    "style \"moo-edit-tab-close-button\" {\n"
                    "   GtkWidget::focus-line-width = 0\n"
                    "   GtkWidget::focus-padding = 0\n"
                    "   GtkButton::default-border = { 0, 0, 0, 0 }\n"
                    "   GtkButton::default-outside-border = { 0, 0, 0, 0 }\n"
                    "   GtkButton::inner-border = { 0, 0, 0, 0 }\n"
                    "}\n"
                    "widget \"*.moo-edit-tab-close-button\" style \"moo-edit-tab-close-button\""
                );
            }
        }

        frame = gtk_aspect_frame_new (nullptr, 0.5, 0.5, 1.0, FALSE);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

        button = gtk_button_new ();

        icon = _moo_create_small_icon (MOO_SMALL_ICON_CLOSE);
        gtk_widget_set_size_request (icon, 9, 9);

        gtk_container_add (GTK_CONTAINER (button), icon);

        gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
        gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
        gtk_widget_set_name (button, "moo-edit-tab-close-button");
        g_signal_connect_swapped (button, "clicked",
                                  G_CALLBACK (tab_close_button_clicked),
                                  doc);

        gtk_container_add (GTK_CONTAINER (frame), button);
        gtk_widget_show_all (frame);

        gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
        gtk_container_set_border_width (GTK_CONTAINER (button), 0);

        gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
        gtk_size_group_add_widget (group, frame);
    }

    gtk_size_group_add_widget (group, evbox);
    gtk_size_group_add_widget (group, label);

    data_moo_edit_icon.set(hbox, icon);
    data_moo_edit_icon_evbox.set(hbox, evbox);
    data_moo_edit_label.set(hbox, label);
    data_moo_edit_tab.set(hbox, tab);
    data_moo_edit_icon.set(evbox, icon);
    widget_doc.set(evbox, doc);
    data_moo_edit_tab.set(evbox, tab);
    widget_doc.set(icon, doc);
    data_moo_edit_tab.set(icon, tab);

    g_signal_connect (evbox, "button-press-event",
                      G_CALLBACK (tab_icon_button_press),
                      window);

    g_object_unref (group);

    return hbox;
}


static void
set_tab_icon (GtkWidget *image,
              GdkPixbuf *pixbuf)
{
    GdkPixbuf *old_pixbuf;

    /* file icons are cached, so it's likely the same pixbuf
     * object as before (and it happens every time you switch tabs) */
    old_pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (image));

    if (old_pixbuf != pixbuf)
        gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
}

static void
update_tab_label (MooEditTab    *tab,
                  MooNotebook   &notebook)
{
    GtkWidget *hbox, *icon, *label, *evbox;
    MooEditStatus status;
    char *label_text;
    gboolean modified, deleted;
    GdkPixbuf *pixbuf;
    MooEdit *doc;

    doc = moo_edit_tab_get_doc (tab);
    g_return_if_fail (doc != nullptr);

    hbox = moo_notebook_get_tab_label (&notebook, GTK_WIDGET (tab));
    g_return_if_fail (GTK_IS_WIDGET (hbox));

    icon = data_moo_edit_icon.get(hbox);
    label = data_moo_edit_label.get(hbox);
    evbox = data_moo_edit_icon_evbox.get(hbox);
    g_return_if_fail (GTK_IS_WIDGET (icon) && GTK_IS_WIDGET (label));
    g_return_if_fail (GTK_IS_WIDGET (evbox));

    _moo_widget_set_tooltip (hbox, moo_edit_get_display_name (doc));

    status = moo_edit_get_status (doc);

    deleted = status & (MOO_EDIT_STATUS_DELETED | MOO_EDIT_STATUS_MODIFIED_ON_DISK);
    modified = (status & MOO_EDIT_STATUS_MODIFIED) && !(status & MOO_EDIT_STATUS_CLEAN);

    label_text = g_strdup_printf ("%s%s%s",
                                  deleted ? "!" : "",
                                  modified ? "*" : "",
                                  moo_edit_get_display_basename (doc));
    gtk_label_set_text (GTK_LABEL (label), label_text);

    {
        int width, max_width, height;
        PangoLayout *M = gtk_widget_create_pango_layout (label, "MMMMMMMMMMMMMMMMMMMM");
        PangoLayout *layout = gtk_widget_create_pango_layout (label, label_text);
        pango_layout_get_pixel_size (layout, &width, &height);
        pango_layout_get_pixel_size (M, &max_width, &height);

        if (width > max_width)
        {
            gtk_widget_set_size_request (label, max_width, -1);
            gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
        }
        else
        {
            gtk_widget_set_size_request (label, -1, -1);
            gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_NONE);
        }

        g_object_unref (layout);
        g_object_unref (M);
    }

    pixbuf = _moo_edit_get_icon (doc, icon, GTK_ICON_SIZE_MENU);
    set_tab_icon (icon, pixbuf);

    g_free (label_text);
}

static void
update_tab_labels (MooEditWindow *window,
                   MooEdit       *doc)
{
    for (const auto& notebook: window->priv->notebooks)
    {
        int i;
        int n_pages = moo_notebook_get_n_pages (notebook.get());
        for (i = 0; i < n_pages; i++)
        {
            MooEditTab *tab = get_nth_tab (*notebook, i);
            if (doc == moo_edit_tab_get_doc (tab))
                update_tab_label (tab, *notebook);
        }
    }
}


/****************************************************************************/
/* Panes
 */

static void
save_paned_config (MooEditWindow *window)
{
    char *config;
    const char *old_config;

    config = moo_big_paned_get_config (window->paned);
    g_return_if_fail (config != nullptr);

    old_config = moo_prefs_get_string (MOO_EDIT_PREFS_PREFIX "/window/paned");

    if (!old_config || strcmp (old_config, config) != 0)
        moo_prefs_set_string (MOO_EDIT_PREFS_PREFIX "/window/paned", config);

    g_free (config);
}

static void
create_paned (MooEditWindow *window)
{
    MooBigPaned *paned;
    const char *config;

    paned = MOO_BIG_PANED (g_object_new (MOO_TYPE_BIG_PANED,
                                         "handle-cursor-type", GDK_FLEUR,
                                         "enable-detaching", TRUE,
                                         (const char*) nullptr));
    gtk_widget_show (GTK_WIDGET (paned));
    gtk_box_pack_start (GTK_BOX (MOO_WINDOW(window)->vbox),
                        GTK_WIDGET (paned), TRUE, TRUE, 0);

    window->paned = paned;

    moo_prefs_create_key (MOO_EDIT_PREFS_PREFIX "/window/paned",
                          MOO_PREFS_STATE, G_TYPE_STRING, nullptr);
    config = moo_prefs_get_string (MOO_EDIT_PREFS_PREFIX "/window/paned");

    if (config)
        moo_big_paned_set_config (paned, config);

    g_signal_connect_swapped (paned, "config-changed",
                              G_CALLBACK (queue_save_window_config),
                              window);
}


static char *
make_show_pane_action_id (const char *user_id)
{
    return g_strdup_printf ("MooEditWindow-ShowPane-%s", user_id);
}

static void
show_pane_callback (MooEditWindow *window,
                    const char    *user_id)
{
    moo_edit_window_show_pane (window, user_id);
}

static void
add_pane_action (MooEditWindow *window,
                 const char    *user_id,
                 MooPaneLabel  *label)
{
    char *action_id;
    MooWindowClass *klass;
    GtkAction *action;
    MooUiXml *xml;

    action_id = make_show_pane_action_id (user_id);
    klass = (MooWindowClass*) g_type_class_peek (MOO_TYPE_EDIT_WINDOW);

    if (!moo_window_class_find_action (klass, action_id))
    {
        guint merge_id;

        _moo_window_class_new_action_callback (klass, action_id, nullptr,
                                               G_CALLBACK (show_pane_callback),
                                               _moo_marshal_VOID__STRING,
                                               G_TYPE_NONE, 1,
                                               G_TYPE_STRING, user_id,
                                               "visible", FALSE,
                                               "display-name", label->label,
                                               "label", label->label,
                                               /* XXX IconInfo */
                                               "stock-id", label->icon_stock_id,
                                               nullptr);

        xml = moo_editor_get_ui_xml (moo_editor_instance ());
        merge_id = moo_ui_xml_new_merge_id (xml);
        moo_ui_xml_add_item (xml, merge_id,
                             "Editor/Menubar/View/PanesMenu",
                             action_id, action_id, -1);
    }

    action = moo_window_get_action (MOO_WINDOW (window), action_id);
    g_return_if_fail (action != nullptr);
    g_object_set (action, "visible", TRUE, nullptr);

    g_free (action_id);
}

static void
remove_pane_action (MooEditWindow *window,
                    const char    *user_id)
{
    char *action_id;
    GtkAction *action;

    action_id = make_show_pane_action_id (user_id);
    action = moo_window_get_action (MOO_WINDOW (window), action_id);

    if (action)
        g_object_set (action, "visible", FALSE, nullptr);

#if 0
    klass = g_type_class_peek (MOO_TYPE_EDIT_WINDOW);

    if (moo_window_class_find_action (klass, action_id))
        moo_window_class_remove_last_action (klass, action_id);
#endif

    g_free (action_id);
}

static MooPane *
moo_edit_window_add_pane_full (MooEditWindow  *window,
                               const char     *user_id,
                               GtkWidget      *widget,
                               MooPaneLabel   *label,
                               MooPanePosition position,
                               gboolean        add_menu)
{
    MooPane *pane;

    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), nullptr);
    g_return_val_if_fail (user_id != nullptr, nullptr);
    g_return_val_if_fail (GTK_IS_WIDGET (widget), nullptr);
    g_return_val_if_fail (label != nullptr, nullptr);

    g_return_val_if_fail (moo_edit_window_get_pane (window, user_id) == nullptr, nullptr);

    g_object_ref_sink (widget);

    pane = moo_big_paned_insert_pane (window->paned, widget, user_id, label, position, -1);

    if (pane != nullptr)
    {
        moo_pane_set_removable (pane, FALSE);

        if (add_menu)
            add_pane_action (window, user_id, label);
    }

    g_object_unref (widget);
    return pane;
}

/**
 * moo_edit_window_add_pane: (moo.lua 0)
 *
 * @window:
 * @user_id: (type const-utf8)
 * @widget:
 * @label:
 * @position:
 */
MooPane *
moo_edit_window_add_pane (MooEditWindow  *window,
                          const char     *user_id,
                          GtkWidget      *widget,
                          MooPaneLabel   *label,
                          MooPanePosition position)
{
    return moo_edit_window_add_pane_full (window, user_id, widget,
                                          label, position, TRUE);
}


/**
 * moo_edit_window_remove_pane: (moo.lua 0)
 *
 * @window:
 * @user_id: (type const-utf8)
 */
gboolean
moo_edit_window_remove_pane (MooEditWindow *window,
                             const char    *user_id)
{
    MooPane *pane;

    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), FALSE);
    g_return_val_if_fail (user_id != nullptr, FALSE);

    remove_pane_action (window, user_id);

    if (!(pane = moo_big_paned_lookup_pane (window->paned, user_id)))
        return FALSE;

    moo_big_paned_remove_pane (window->paned, moo_pane_get_child (pane));
    return TRUE;
}


/**
 * moo_edit_window_show_pane: (moo.lua 0) (moo.private 1)
 *
 * @window:
 * @user_id: (type const-utf8)
 */
void
moo_edit_window_show_pane (MooEditWindow *window,
                           const char    *user_id)
{
    GtkWidget *pane;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));

    pane = moo_edit_window_get_pane (window, user_id);
    g_return_if_fail (pane != nullptr);

    moo_big_paned_present_pane (window->paned, pane);
}


/**
 * moo_edit_window_get_pane: (moo.lua 0)
 *
 * @window:
 * @user_id: (type const-utf8)
 */
GtkWidget*
moo_edit_window_get_pane (MooEditWindow  *window,
                          const char     *user_id)
{
    MooPane *pane;

    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), nullptr);
    g_return_val_if_fail (user_id != nullptr, nullptr);

    pane = moo_big_paned_lookup_pane (window->paned, user_id);
    return pane ? moo_pane_get_child (pane) : nullptr;
}


/****************************************************************************/
/* Statusbar
 */

static void
set_statusbar_numbers (MooEditWindow *window,
                       int            line,
                       int            column,
                       int            chars)
{
    char line_buf[10] = {0};
    char column_buf[10] = {0};
    char chars_buf[10] = {0};
    char *text, *text2;

    if (line > 0 && column > 0)
    {
        g_snprintf (line_buf, sizeof line_buf, "%d", line);
        g_snprintf (column_buf, sizeof column_buf, "%d", column);
    }

    if (chars >= 0)
        g_snprintf (chars_buf, sizeof chars_buf, "%d", chars);

    text = g_strdup_printf (_("Line: %s Col: %s"), line_buf, column_buf);
    text2 = g_strdup_printf (_("Chars: %s"), chars_buf);

    gtk_label_set_text (window->priv->cursor_label, text);
    gtk_label_set_text (window->priv->chars_label, text2);

    g_free (text2);
    g_free (text);
}

static void
do_update_statusbar (MooEditWindow *window)
{
    MooEdit *doc;
    MooEditView *view;
    int line, column, chars;
    GtkTextIter iter;
    GtkTextBuffer *buffer;
    gboolean ovr;

    view = ACTIVE_VIEW (window);

    if (!view)
    {
        gtk_widget_set_sensitive (window->priv->info, FALSE);
        set_statusbar_numbers (window, 0, 0, -1);
        return;
    }

    gtk_widget_set_sensitive (window->priv->info, TRUE);

    doc = moo_edit_view_get_doc (view);
    buffer = moo_edit_get_buffer (doc);

    gtk_text_buffer_get_iter_at_mark (buffer, &iter,
                                      gtk_text_buffer_get_insert (buffer));
    line = gtk_text_iter_get_line (&iter) + 1;
    column = moo_text_iter_get_visual_line_offset (&iter, 8) + 1;
    chars = gtk_text_buffer_get_char_count (buffer);

    set_statusbar_numbers (window, line, column, chars);

    ovr = gtk_text_view_get_overwrite (GTK_TEXT_VIEW (view));
    /* Label in the editor window statusbar - Overwrite or Insert mode */
    gtk_label_set_text (window->priv->insert_label, ovr ? _("OVR") : _("INS"));
}

static gboolean
update_statusbar_idle (MooEditWindow *window)
{
    window->priv->statusbar_idle = 0;
    do_update_statusbar (window);
    return FALSE;
}

static void
update_statusbar (MooEditWindow *window)
{
    if (!window->priv->statusbar_idle)
        window->priv->statusbar_idle =
            g_idle_add_full (G_PRIORITY_HIGH,
                             (GSourceFunc) update_statusbar_idle,
                             window, nullptr);
    moo_window_message (MOO_WINDOW (window), nullptr);
}

static void
create_statusbar (MooEditWindow *window)
{
    EditorStatusbarXml *xml;

    xml = editor_statusbar_xml_new ();

    gtk_container_add_with_properties (GTK_CONTAINER (MOO_WINDOW (window)->status_area),
                                       GTK_WIDGET (xml->EditorStatusbar),
                                       "pack-type", GTK_PACK_END,
                                       "expand", FALSE,
                                       "fill", FALSE,
                                       nullptr);

    window->priv->cursor_label = xml->cursor;
    window->priv->chars_label = xml->chars;
    window->priv->insert_label = xml->insert;
    window->priv->info = GTK_WIDGET (xml->info);
}


/*****************************************************************************/
/* Language menu
 */

static int
cmp_langs (MooLang *lang1,
           MooLang *lang2)
{
    return g_utf8_collate (_moo_lang_display_name (lang1),
                           _moo_lang_display_name (lang2));
}


static void
lang_item_activated (MooEditWindow *window,
                     const char    *lang_name)
{
    MooEdit *doc = ACTIVE_DOC (window);
    const char *old_val;
    gboolean do_set = FALSE;

    g_return_if_fail (doc != nullptr);
    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));

    old_val = moo_edit_config_get_string (doc->config, "lang");

    if (old_val)
        do_set = !lang_name || strcmp (old_val, lang_name);
    else
        do_set = !!lang_name;

    if (do_set)
        moo_edit_config_set (doc->config, MOO_EDIT_CONFIG_SOURCE_USER,
                             "lang", lang_name, nullptr);
}


static GtkAction*
create_lang_action (MooEditWindow *window)
{
    GtkAction *action;
    MooMenuMgr *menu_mgr;
    MooLangMgr *lang_mgr;
    GSList *langs, *sections, *l;

    lang_mgr = moo_lang_mgr_default ();

    /* TODO display names, etc. */
    sections = moo_lang_mgr_get_sections (lang_mgr);
    sections = g_slist_sort (sections, (GCompareFunc) strcmp);

    langs = moo_lang_mgr_get_available_langs (lang_mgr);
    langs = g_slist_sort (langs, (GCompareFunc) cmp_langs);

    action = moo_menu_action_new (LANG_ACTION_ID, _("_Language"));
    menu_mgr = moo_menu_action_get_mgr (MOO_MENU_ACTION (action));

    moo_menu_mgr_append (menu_mgr, nullptr,
                         /* Menu item in the Language menu */
                         MOO_LANG_NONE, Q_("Language|None"), nullptr,
                         MOO_MENU_ITEM_RADIO, nullptr, nullptr);

    for (l = sections; l != nullptr; l = l->next)
        moo_menu_mgr_append (menu_mgr, nullptr,
                             (const char*) l->data, (const char*) l->data, nullptr,
                             MOO_MENU_ITEM_FLAGS_NONE, nullptr, nullptr);

    for (l = langs; l != nullptr; l = l->next)
    {
        MooLang *lang = (MooLang*) l->data;
        if (!_moo_lang_get_hidden (lang))
            moo_menu_mgr_append (menu_mgr, _moo_lang_get_section (lang),
                                 _moo_lang_id (lang),
                                 _moo_lang_display_name (lang),
                                 nullptr, MOO_MENU_ITEM_RADIO,
                                 g_strdup (_moo_lang_id (lang)),
                                 g_free);
    }

    g_signal_connect_swapped (menu_mgr, "radio-set-active",
                              G_CALLBACK (lang_item_activated), window);

    g_slist_foreach (langs, (GFunc) g_object_unref, nullptr);
    g_slist_free (langs);
    g_slist_foreach (sections, (GFunc) g_free, nullptr);
    g_slist_free (sections);

    moo_bind_bool_property (action, "sensitive", window, "has-open-document", FALSE);
    return action;
}


static void
update_lang_menu (MooEditWindow      *window)
{
    MooEditView *view;
    GtkAction *action;
    MooLang *lang;

    view = ACTIVE_VIEW (window);

    if (!view)
        return;

    lang = moo_text_view_get_lang (MOO_TEXT_VIEW (view));
    action = moo_window_get_action (MOO_WINDOW (window), LANG_ACTION_ID);
    g_return_if_fail (action != nullptr);

    moo_menu_mgr_set_active (moo_menu_action_get_mgr (MOO_MENU_ACTION (action)),
                             _moo_lang_id (lang), TRUE);
}


static void
edit_lang_changed (MooEditWindow      *window,
                   G_GNUC_UNUSED GParamSpec *pspec,
                   MooEdit            *doc)
{
    if (doc == ACTIVE_DOC (window))
    {
        update_lang_menu (window);
        moo_edit_window_check_actions (window);
    }
}


/*****************************************************************************/
/* Action properties checks
 */

static void
window_check_one_action (const char    *action_id,
                         ActionCheck   *set,
                         MooEditWindow *window,
                         MooEdit       *doc)
{
    MooActionCheckFunc func;
    GtkAction *action;
    gboolean visible = TRUE, sensitive = TRUE;

    action = moo_window_get_action (MOO_WINDOW (window), action_id);

    if (!action)
        return;

    if ((func = set->checks[MOO_ACTION_CHECK_ACTIVE].func))
    {
        if (!func (action, window, doc,
                   set->checks[MOO_ACTION_CHECK_ACTIVE].data))
        {
            visible = FALSE;
            sensitive = FALSE;
        }
    }

    if (visible && (func = set->checks[MOO_ACTION_CHECK_VISIBLE].func))
    {
        gpointer data = set->checks[MOO_ACTION_CHECK_VISIBLE].data;
        visible = func (action, window, doc, data);
    }

    if (sensitive && (func = set->checks[MOO_ACTION_CHECK_SENSITIVE].func))
    {
        gpointer data = set->checks[MOO_ACTION_CHECK_SENSITIVE].data;
        sensitive = func (action, window, doc, data);
    }

    g_object_set (action, "visible", visible, "sensitive", sensitive, nullptr);
}


struct CheckActionData
{
    MooEdit *doc;
    MooEditWindow *window;
};

static void
check_action_hash_cb (const char    *action_id,
                      ActionCheck   *check,
                      gpointer       user_data)
{
    CheckActionData* data = (CheckActionData*) user_data;

    window_check_one_action (action_id, check, data->window, data->doc);
}


static void
moo_edit_window_check_actions (MooEditWindow *window)
{
    CheckActionData data;

    data.window = window;
    data.doc = ACTIVE_DOC (window);

    g_hash_table_foreach (action_checks,
                          (GHFunc) check_action_hash_cb,
                          &data);
}


void
moo_edit_window_set_action_check (const char     *action_id,
                                  MooActionCheckType type,
                                  MooActionCheckFunc func,
                                  gpointer        data,
                                  GDestroyNotify  notify)
{
    ActionCheck *check;

    g_return_if_fail (action_id != nullptr);
    g_return_if_fail (type < N_ACTION_CHECKS);
    g_return_if_fail (func != nullptr);

    action_checks_init ();

    check = (ActionCheck*) g_hash_table_lookup (action_checks, action_id);

    if (!check)
    {
        check = g_new0 (ActionCheck, 1);
        g_hash_table_insert (action_checks, g_strdup (action_id), check);
    }

    if (check->checks[type].func)
    {
        check->checks[type].func = nullptr;

        if (check->checks[type].notify)
            check->checks[type].notify (check->checks[type].data);
    }

    check->checks[type].func = func;
    check->checks[type].data = data;
    check->checks[type].notify = notify;

    for (MooEditWindow* w: windows)
    {
        MooEdit *doc = ACTIVE_DOC (w);
        window_check_one_action (action_id, check, w, doc);
    }
}


static void
moo_edit_window_remove_action_check (const char        *action_id,
                                     MooActionCheckType type)
{
    ActionCheck *check;
    gboolean remove = TRUE;
    guint i;

    g_return_if_fail (action_id != nullptr);
    g_return_if_fail (type <= N_ACTION_CHECKS);

    if (!action_checks)
        return;

    check = (ActionCheck*) g_hash_table_lookup (action_checks, action_id);

    if (!check)
        return;

    if (type < N_ACTION_CHECKS)
    {
        for (i = 0; i < N_ACTION_CHECKS; ++i)
        {
            if (check->checks[i].func && i != type)
            {
                remove = FALSE;
                break;
            }
        }

        if (!remove && check->checks[type].func && check->checks[type].notify)
        {
            check->checks[type].func = nullptr;
            check->checks[type].notify (check->checks[type].data);
        }
    }

    if (remove)
    {
        g_hash_table_remove (action_checks, action_id);

        for (i = 0; i < N_ACTION_CHECKS; ++i)
            if (check->checks[i].func && check->checks[i].notify)
                check->checks[i].notify (check->checks[i].data);

        g_free (check);
    }
}


static gboolean
check_action_filter (G_GNUC_UNUSED GtkAction *action,
                     G_GNUC_UNUSED MooEditWindow *window,
                     MooEdit *doc,
                     gpointer filter)
{
    gboolean value = FALSE;

    if (doc)
        value = _moo_edit_filter_match ((MooEditFilter*) filter, doc);

    return value;
}

/**
 * moo_edit_window_set_action_filter: (moo.lua 0)
 *
 * @action_id: (type const-utf8)
 * @type:
 * @filter_string: (type const-utf8)
 */
void
moo_edit_window_set_action_filter (const char        *action_id,
                                   MooActionCheckType type,
                                   const char        *filter_string)
{
    MooEditFilter *filter = nullptr;

    g_return_if_fail (action_id != nullptr);
    g_return_if_fail (type < N_ACTION_CHECKS);

    if (filter_string && filter_string[0])
        filter = _moo_edit_filter_new (filter_string, MOO_EDIT_FILTER_ACTION);

    if (filter)
        moo_edit_window_set_action_check (action_id, type,
                                          check_action_filter,
                                          filter,
                                          (GDestroyNotify) _moo_edit_filter_free);
    else
        moo_edit_window_remove_action_check (action_id, type);
}


static void
action_checks_init (void)
{
    if (!action_checks)
        action_checks =
                g_hash_table_new_full (g_str_hash, g_str_equal, g_free, nullptr);
}


/*****************************************************************************/
/* Stop button
 */

typedef struct {
    gpointer job;
    MooAbortJobFunc abort;
} Job;

static void moo_edit_window_job_finished (MooEditWindow  *window,
                                          gpointer        job);
static void moo_edit_window_job_started  (MooEditWindow  *window,
                                          const char     *name,
                                          MooAbortJobFunc func,
                                          gpointer        job);


static void
client_died (MooEditWindow  *window,
             gpointer        client)
{
    window->priv->stop_clients = g_slist_remove (window->priv->stop_clients, client);
    moo_edit_window_job_finished (window, client);
}


static void
abort_client_job (gpointer client)
{
    gboolean ret;
    g_signal_emit_by_name (client, "abort", &ret);
}


static void
client_job_started (gpointer        client,
                    const char     *job_name,
                    MooEditWindow  *window)
{
    moo_edit_window_job_started (window, job_name, abort_client_job, client);
}


static void
client_job_finished (gpointer        client,
                     MooEditWindow  *window)
{
    moo_edit_window_job_finished (window, client);
}


/**
 * moo_edit_window_add_stop_client: (moo.lua 0)
 */
void
moo_edit_window_add_stop_client (MooEditWindow  *window,
                                 GObject        *client)
{
    GType type, return_type;
    guint signal_abort, signal_started, signal_finished;
    GSignalQuery query;
    gboolean had_clients;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (G_IS_OBJECT (client));

    g_return_if_fail (!g_slist_find (window->priv->stop_clients, client));

    type = G_OBJECT_TYPE (client);
    signal_abort = g_signal_lookup ("abort", type);
    signal_started = g_signal_lookup ("job-started", type);
    signal_finished = g_signal_lookup ("job-finished", type);
    g_return_if_fail (signal_abort && signal_started && signal_finished);

#define REAL_TYPE(t__) ((t__) & ~(G_SIGNAL_TYPE_STATIC_SCOPE))
    g_signal_query (signal_abort, &query);
    return_type = REAL_TYPE (query.return_type);
    g_return_if_fail (return_type == G_TYPE_NONE || return_type == G_TYPE_BOOLEAN);
    g_return_if_fail (query.n_params == 0);

    g_signal_query (signal_started, &query);
    g_return_if_fail (REAL_TYPE (query.return_type) == G_TYPE_NONE);
    g_return_if_fail (query.n_params == 1);
    g_return_if_fail (REAL_TYPE (query.param_types[0]) == G_TYPE_STRING);

    g_signal_query (signal_finished, &query);
    g_return_if_fail (REAL_TYPE (query.return_type) == G_TYPE_NONE);
    g_return_if_fail (query.n_params == 0);
#undef REAL_TYPE

    had_clients = window->priv->stop_clients != nullptr;
    window->priv->stop_clients = g_slist_prepend (window->priv->stop_clients, client);
    g_object_weak_ref (client, (GWeakNotify) client_died, window);
    g_signal_connect (client, "job-started", G_CALLBACK (client_job_started), window);
    g_signal_connect (client, "job-finished", G_CALLBACK (client_job_finished), window);

    if (!had_clients)
        g_object_notify (G_OBJECT (window), "has-stop-clients");
}


/**
 * moo_edit_window_remove_stop_client: (moo.lua 0)
 */
void
moo_edit_window_remove_stop_client (MooEditWindow  *window,
                                    GObject        *client)
{
    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (g_slist_find (window->priv->stop_clients, client));

    window->priv->stop_clients = g_slist_remove (window->priv->stop_clients, client);

    if (G_IS_OBJECT (client))
    {
        g_object_weak_unref (client, (GWeakNotify) client_died, window);
        g_signal_handlers_disconnect_by_func (client, (gpointer)client_job_started, window);
        g_signal_handlers_disconnect_by_func (client, (gpointer)client_job_finished, window);
    }

    if (!window->priv->stop_clients)
        g_object_notify (G_OBJECT (window), "has-stop-clients");
}


/**
 * moo_edit_window_abort_jobs: (moo.lua 0)
 */
void
moo_edit_window_abort_jobs (MooEditWindow *window)
{
    GSList *l, *jobs;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));

    jobs = g_slist_copy (window->priv->jobs);

    for (l = jobs; l != nullptr; l = l->next)
    {
        Job *j = (Job*) l->data;
        j->abort (j->job);
    }

    g_slist_free (jobs);
}


static void
moo_edit_window_job_started (MooEditWindow  *window,
                             G_GNUC_UNUSED const char *name,
                             MooAbortJobFunc func,
                             gpointer        job)
{
    Job *j;
    gboolean had_jobs;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (func != nullptr);
    g_return_if_fail (job != nullptr);

    j = g_new0 (Job, 1);
    j->abort = func;
    j->job = job;

    had_jobs = window->priv->jobs != nullptr;
    window->priv->jobs = g_slist_prepend (window->priv->jobs, j);

    if (!had_jobs)
        g_object_notify (G_OBJECT (window), "has-jobs-running");
}


static void
moo_edit_window_job_finished (MooEditWindow  *window,
                              gpointer        job)
{
    GSList *l;
    Job *j = nullptr;

    g_return_if_fail (MOO_IS_EDIT_WINDOW (window));
    g_return_if_fail (job != nullptr);

    for (l = window->priv->jobs; l != nullptr; l = l->next)
    {
        j = (Job*) l->data;

        if (j->job == job)
            break;
        else
            j = nullptr;
    }

    if (j)
    {
        window->priv->jobs = g_slist_remove (window->priv->jobs, j);

        if (!window->priv->jobs)
            g_object_notify (G_OBJECT (window), "has-jobs-running");

        g_free (j);
    }
}


/************************************************************************/
/* Doc list
 */

namespace {
const ObjectDataAccessor<GtkWidget, bool> data_is_doc_menu_item("moo-document-menu-item");
} // namespace

static int
compare_docs_for_menu (MooEdit       *doc1,
                       MooEdit       *doc2)
{
    char *k1, *k2;
    int result;

    k1 = g_utf8_collate_key_for_filename (moo_edit_get_display_basename (doc1), -1);
    k2 = g_utf8_collate_key_for_filename (moo_edit_get_display_basename (doc2), -1);

    result = strcmp (k1, k2);

    if (!result)
    {
        MooEditWindow *window = moo_edit_get_window (doc1);
        result = get_doc_num (window, doc1) - get_doc_num (window, doc2);
    }

    g_free (k2);
    g_free (k1);
    return result;
}

static void
doc_menu_item_activated (MooEdit *doc)
{
    moo_editor_set_active_doc (moo_edit_get_editor (doc), doc);
}

static void
populate_window_menu (MooEditWindow *window,
                      GtkWidget     *menu,
                      GtkWidget     *no_docs_item)
{
    MooEdit *active_doc;
    MooEditArray *docs;
    GList *children, *l;
    int pos;
    guint i;
    GtkWidget *item;

    children = g_list_copy (GTK_MENU_SHELL (menu)->children);
    pos = g_list_index (children, no_docs_item);
    g_return_if_fail (pos >= 0);

    for (l = g_list_nth (children, pos + 1); l != nullptr; l = l->next)
    {
        if (data_is_doc_menu_item.get(GTK_WIDGET (l->data)))
            gtk_container_remove (GTK_CONTAINER (menu), (GtkWidget*) l->data);
        else
            break;
    }

    g_list_free (children);

    docs = moo_editor_get_docs (moo_edit_window_get_editor (window));

    if (moo_edit_array_is_empty (docs))
    {
        moo_edit_array_free (docs);
        return;
    }

    item = gtk_separator_menu_item_new ();
    data_is_doc_menu_item.set(item, true);
    gtk_widget_show (item);
    gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, ++pos);

    moo_edit_array_sort (docs, (GCompareFunc) compare_docs_for_menu);
    active_doc = ACTIVE_DOC (window);

    for (i = 0; i < docs->n_elms; ++i)
    {
        int idx;
        MooEdit *doc = docs->elms[i];

        idx = get_doc_num (window, doc);

        item = gtk_check_menu_item_new_with_label (moo_edit_get_display_basename (doc));
        _moo_widget_set_tooltip (item, moo_edit_get_display_name (doc));
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), doc == active_doc);
        gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (item), TRUE);
        data_is_doc_menu_item.set(item, TRUE);
        gtk_widget_show (item);
        gtk_menu_shell_insert (GTK_MENU_SHELL (menu), item, ++pos);
        g_signal_connect_swapped (item, "activate", G_CALLBACK (doc_menu_item_activated), doc);

        if (idx >= 0 && idx < 9)
        {
            char *action_name = g_strdup_printf (DOCUMENT_ACTION "%u", idx + 1);
            GtkAction *action = moo_window_get_action (MOO_WINDOW (window), action_name);
            gtk_menu_item_set_accel_path (GTK_MENU_ITEM (item),
                                          gtk_action_get_accel_path (action));
            g_free (action_name);
        }
    }

    moo_edit_array_free (docs);
}

static void
window_menu_item_selected (MooWindow   *window,
                           GtkMenuItem *win_item)
{
    GtkWidget *menu;
    GtkWidget *no_docs_item;

    menu = gtk_menu_item_get_submenu (win_item);
    no_docs_item = moo_ui_xml_get_widget (moo_window_get_ui_xml (window),
                                          window->menubar,
                                          "Editor/Menubar/Window/NoDocuments");
    g_return_if_fail (menu != nullptr && no_docs_item != nullptr);

    populate_window_menu (MOO_EDIT_WINDOW (window), menu, no_docs_item);
}


static void
moo_edit_window_update_doc_list (MooEditWindow *window)
{
    MooEdit *doc;

    if (window->priv->enable_history &&
        !window->priv->history_blocked &&
        (doc = ACTIVE_DOC (window)))
    {
        GList *link = g_list_find (window->priv->history, doc);

        if (link && link != window->priv->history)
        {
            window->priv->history = g_list_delete_link (window->priv->history, link);
            window->priv->history = g_list_prepend (window->priv->history, doc);
        }
        else if (!link)
        {
            window->priv->history = g_list_prepend (window->priv->history, doc);
            if (g_list_length (window->priv->history) > 2)
                window->priv->history = g_list_delete_link (window->priv->history,
                                                            g_list_last (window->priv->history));
        }
    }
}


/************************************************************************/
/* Drag into the window
 */

namespace {
const ObjectDataAccessor<GtkWidget, bool> data_window_drop("moo-edit-window-drop");
} // namespace

static gboolean
notebook_drag_motion (GtkWidget          *widget,
                      GdkDragContext     *context,
                      G_GNUC_UNUSED int   x,
                      G_GNUC_UNUSED int   y,
                      guint               time,
                      G_GNUC_UNUSED MooEditWindow *window)
{
    GdkAtom target;

    target = gtk_drag_dest_find_target (widget, context, nullptr);

    if (target == GDK_NONE)
        return FALSE;

    if (target == MOO_EDIT_TAB_ATOM)
        gtk_drag_get_data (widget, context, MOO_EDIT_TAB_ATOM, time);
    else
        gdk_drag_status (context, context->suggested_action, time);

    return TRUE;
}


static gboolean
notebook_drag_drop (GtkWidget          *widget,
                    GdkDragContext     *context,
                    G_GNUC_UNUSED int   x,
                    G_GNUC_UNUSED int   y,
                    guint               time,
                    G_GNUC_UNUSED MooEditWindow *window)
{
    GdkAtom target;

    target = gtk_drag_dest_find_target (widget, context, nullptr);

    if (target == GDK_NONE)
    {
        gtk_drag_finish (context, FALSE, FALSE, time);
    }
    else
    {
        data_window_drop.set(widget, true);
        gtk_drag_get_data (widget, context, target, time);
    }

    return TRUE;
}


static void
notebook_drag_data_recv (GtkWidget          *widget,
                         GdkDragContext     *context,
                         G_GNUC_UNUSED int   x,
                         G_GNUC_UNUSED int   y,
                         GtkSelectionData   *data,
                         guint               info,
                         guint               time,
                         MooEditWindow      *window)
{
    gboolean finished = FALSE;

    if (data_window_drop.get(widget))
    {
        data_window_drop.set(widget, false);

        if (data->target == MOO_EDIT_TAB_ATOM)
        {
            GtkWidget *toplevel;
            GtkWidget *src_notebook = nullptr;
            MooEdit *doc;
            MooEditTab *tab;

            tab = (MooEditTab*) moo_selection_data_get_pointer (data, MOO_EDIT_TAB_ATOM);
            doc = tab ? moo_edit_tab_get_doc (tab) : nullptr;

            if (!doc)
                goto out;

            toplevel = gtk_widget_get_toplevel (GTK_WIDGET (tab));

            if (toplevel == GTK_WIDGET (window))
                src_notebook = gtk_widget_get_parent (GTK_WIDGET (tab));

            g_assert (!src_notebook || MOO_IS_NOTEBOOK (src_notebook));

            if (toplevel != GTK_WIDGET (window))
                _moo_editor_move_doc (window->priv->editor, doc, window,
                                      get_notebook_active_view (MOO_NOTEBOOK (widget)),
                                      TRUE);
            else if (src_notebook != widget)
                move_tab_to_split_view (window, tab);

            goto out;
        }
        else if (data->target == moo_atom_uri_list ())
        {
            char **uris;
            char **u;

            /* XXX this is wrong but works. gtk_selection_data_get_uris()
             * does not work on windows */
            uris = g_uri_list_extract_uris ((char*) data->data);

            if (!uris)
                goto out;

            for (u = uris; *u; ++u)
            {
                char *filename = g_filename_from_uri (*u, nullptr, nullptr);
                if (!filename || !g_file_test (filename, G_FILE_TEST_IS_DIR))
                    moo_editor_open_uri (window->priv->editor, *u, nullptr, -1, window);
                g_free (filename);
            }

            g_strfreev (uris);
            gtk_drag_finish (context, TRUE, FALSE, time);
            finished = TRUE;
        }
        else
        {
            MooEdit *doc;
            GtkTextBuffer *buf;
            char *text = (char *) gtk_selection_data_get_text (data);

            if (!text)
                goto out;

            doc = moo_editor_new_doc (window->priv->editor, window);

            if (!doc)
            {
                g_free (text);
                goto out;
            }

            /* XXX */
            buf = moo_edit_get_buffer (doc);
            gtk_text_buffer_set_text (buf, text, -1);

            g_free (text);
            gtk_drag_finish (context, TRUE,
                             context->suggested_action == GDK_ACTION_MOVE,
                             time);
            finished = TRUE;
        }
    }
    else
    {
        if (info == TARGET_MOO_EDIT_TAB)
        {
            GtkWidget *toplevel;
            MooEditTab *tab;
            gboolean can_move = TRUE;

            tab = (MooEditTab*) moo_selection_data_get_pointer (data, MOO_EDIT_TAB_ATOM);

            if (!tab)
            {
                g_critical ("oops");
                gdk_drag_status (context, (GdkDragAction) 0, time);
                return;
            }

            toplevel = gtk_widget_get_toplevel (GTK_WIDGET (tab));

            if (toplevel == GTK_WIDGET (window))
            {
                GtkWidget *src_notebook = nullptr;
                src_notebook = gtk_widget_get_parent (GTK_WIDGET (tab));
                g_assert (MOO_IS_NOTEBOOK (widget));
                can_move = src_notebook != widget;
            }

            if (!can_move)
            {
                gdk_drag_status (context, (GdkDragAction) 0, time);
                return;
            }

            gdk_drag_status (context, GDK_ACTION_MOVE, time);
        }
        else
        {
            gdk_drag_status (context, (GdkDragAction) 0, time);
        }

        return;
    }

out:
    if (!finished)
        gtk_drag_finish (context, FALSE, FALSE, time);
}


/**************************************************************************************************
 *
 * tabs
 *
 */

static void
split_view_horizontal_toggled (MooEditWindow *window,
                               gboolean       active)
{
    MooEditTab *tab = ACTIVE_TAB (window);
    g_return_if_fail (tab != nullptr);
    if (_moo_edit_tab_set_split_horizontal (tab, active))
        update_split_view_actions (window);
}

static void
split_view_vertical_toggled (MooEditWindow *window,
                             gboolean       active)
{
    MooEditTab *tab = ACTIVE_TAB (window);
    g_return_if_fail (tab != nullptr);
    if (_moo_edit_tab_set_split_vertical (tab, active))
        update_split_view_actions (window);
}

static void
action_focus_next_split_view (MooEditWindow *window)
{
    MooEditTab *tab = ACTIVE_TAB (window);
    g_return_if_fail (tab != nullptr);
    _moo_edit_tab_focus_next_view (tab);
    update_doc_view_actions (window);
}

static void
update_split_view_actions (MooEditWindow *window)
{
    GtkAction *action_split_horizontal;
    GtkAction *action_split_vertical;
    GtkAction *action_cycle;
    gboolean has_split_horizontal;
    gboolean has_split_vertical;
    MooEditTab *tab = ACTIVE_TAB (window);

    if (!tab)
        return;

    action_cycle = moo_window_get_action (MOO_WINDOW (window), "CycleSplitViews");
    g_return_if_fail (action_cycle != nullptr);
    action_split_horizontal = moo_window_get_action (MOO_WINDOW (window), "SplitViewHorizontal");
    g_return_if_fail (action_split_horizontal != nullptr);
    action_split_vertical = moo_window_get_action (MOO_WINDOW (window), "SplitViewVertical");
    g_return_if_fail (action_split_vertical != nullptr);

    has_split_horizontal = _moo_edit_tab_get_split_horizontal (tab);
    has_split_vertical = _moo_edit_tab_get_split_vertical (tab);

    gtk_action_set_sensitive (action_cycle, has_split_horizontal || has_split_vertical);
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_split_horizontal), has_split_horizontal);
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_split_vertical), has_split_vertical);

    /* XXX menu item and action go out of sync for some reason */
    sync_proxies (action_cycle);
    sync_proxies (action_split_horizontal);
    sync_proxies (action_split_vertical);
}
