/*
 *   moowindow.c
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
 * class:MooWindow: (parent GtkWindow): window object
 **/

#include "mooutils/moowindow.h"
#include "mooutils/mooaction-private.h"
#include "mooutils/mooactionbase-private.h"
#include "mooutils/moomenuaction.h"
#include "mooutils/mooaccelprefs.h"
#include "mooutils/mooaccel.h"
#include "mooutils/mooprefs.h"
#include "marshals.h"
#include "mooutils/moostock.h"
#include "mooutils/mooactionfactory.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-mem.h"
#include "mooutils/mooeditops.h"
#include "mooutils/mootype-macros.h"
#include "mooutils/mooutils-enums.h"
#include "mooutils/moocompat.h"
#include <gtk/gtk.h>
#include <gobject/gvaluecollector.h>


#define PREFS_REMEMBER_SIZE  "window/remember_size"
#define PREFS_X              "window/x"
#define PREFS_Y              "window/y"
#define PREFS_WIDTH          "window/width"
#define PREFS_HEIGHT         "window/height"
#define PREFS_MAXIMIZED      "window/maximized"
#define PREFS_SHOW_TOOLBAR   "window/show_toolbar"
#define PREFS_SHOW_MENUBAR   "window/show_menubar"
#define PREFS_SHOW_STATUSBAR "window/show_statusbar"
#define PREFS_TOOLBAR_STYLE  "window/toolbar_style"

#define DEFAULT_X            -1000000 // an impossible window position
#define DEFAULT_Y            -1000000 // an impossible window position

#define TOOLBAR_STYLE_ACTION_ID "ToolbarStyle"

static char *default_geometry = NULL;

static GSList *window_instances = NULL;

struct _MooWindowPrivate {
    guint save_size_id;

    char *toolbar_ui_name;
    GtkWidget *toolbar_holder;
    gboolean toolbar_visible;

    char *menubar_ui_name;
    GtkWidget *menubar_holder;
    gboolean menubar_visible;

    gboolean statusbar_visible;

    MooUiXml *ui_xml;
    MooActionCollection *actions;
    char *name;
    char *id;

    GSList *global_accels;
    guint update_accels_idle;

    GtkWidget *eo_widget;
    GtkWidget *default_eo_widget;
    GtkWidget *uo_widget;
    GtkWidget *default_uo_widget;

    guint global_accels_mode : 1;
};

typedef struct {
    guint keyval;
    GdkModifierType modifiers;
    GtkAction *action;
} AccelEntry;

static const char *setting (MooWindow *window, const char *s)
{
    static GString *key = NULL;

    if (!key)
        key = g_string_new (NULL);

    if (window->priv->id)
        g_string_printf (key, "%s/%s", window->priv->id, s);
    else
        g_string_assign (key, s);

    return key->str;
}

static void init_prefs (MooWindow *window);
static GtkToolbarStyle get_toolbar_style (MooWindow *window);


static const char  *moo_window_class_get_id             (MooWindowClass *klass);
static const char  *moo_window_class_get_name           (MooWindowClass *klass);

static GObject     *moo_window_constructor              (GType           type,
                                                         guint           n_props,
                                                         GObjectConstructParam *props);
static void         moo_window_dispose                  (GObject        *object);
static void         moo_window_set_property             (GObject        *object,
                                                         guint           prop_id,
                                                         const GValue   *value,
                                                         GParamSpec     *pspec);
static void         moo_window_get_property             (GObject        *object,
                                                         guint           prop_id,
                                                         GValue         *value,
                                                         GParamSpec     *pspec);

static void         moo_window_set_id                   (MooWindow      *window,
                                                         const char     *id);
static void         moo_window_create_class_actions     (MooWindow      *window);
static void         moo_window_add_action               (MooWindow      *window,
                                                         const char     *group,
                                                         GtkAction      *action);
static void         moo_window_remove_action            (MooWindow      *window,
                                                         const char     *action_id);

static gboolean     moo_window_key_press_event          (GtkWidget      *widget,
                                                         GdkEventKey    *event);
static gboolean     moo_window_delete_event             (GtkWidget      *widget,
                                                         GdkEventAny    *event);

static gboolean     moo_window_save_size                (MooWindow      *window);

static void         moo_window_update_ui                (MooWindow      *window);
static void         moo_window_update_menubar           (MooWindow      *window);
static void         moo_window_update_toolbar           (MooWindow      *window);

static void         moo_window_shortcuts_prefs_dialog   (MooWindow      *window);

static void         moo_window_set_menubar_visible      (MooWindow      *window,
                                                         gboolean        visible);
static void         moo_window_set_toolbar_visible      (MooWindow      *window,
                                                         gboolean        visible);
static void         moo_window_set_statusbar_visible    (MooWindow      *window,
                                                         gboolean        visible);

static GtkAction   *create_toolbar_style_action         (MooWindow      *window,
                                                         gpointer        dummy);

static MooCloseResponse moo_window_close_handler        (MooWindow      *window);

static void         moo_window_set_focus                (GtkWindow      *window,
                                                         GtkWidget      *widget);
static void         moo_window_disconnect_eo_widget     (MooWindow      *window);
static void         moo_window_disconnect_uo_widget     (MooWindow      *window);
static void         moo_window_action_cut               (MooWindow      *window);
static void         moo_window_action_copy              (MooWindow      *window);
static void         moo_window_action_paste             (MooWindow      *window);
static void         moo_window_action_delete            (MooWindow      *window);
static void         moo_window_action_select_all        (MooWindow      *window);
static void         moo_window_action_undo              (MooWindow      *window);
static void         moo_window_action_redo              (MooWindow      *window);

static void         accel_entry_free                    (AccelEntry     *entry);
static void         accels_changed                      (MooWindow      *window);


enum {
    PROP_0,
    PROP_ACCEL_GROUP,
    PROP_MENUBAR_UI_NAME,
    PROP_TOOLBAR_UI_NAME,
    PROP_ID,
    PROP_UI_XML,
    PROP_MENUBAR,
    PROP_TOOLBAR,
    PROP_ACTIONS,
    PROP_TOOLBAR_VISIBLE,
    PROP_MENUBAR_VISIBLE,
    PROP_STATUSBAR_VISIBLE,

    PROP_MEO_CAN_CUT,
    PROP_MEO_CAN_COPY,
    PROP_MEO_CAN_PASTE,
    PROP_MEO_CAN_SELECT_ALL,
    PROP_MEO_CAN_DELETE,

    PROP_MUO_CAN_UNDO,
    PROP_MUO_CAN_REDO
};

enum {
    CLOSE,
    LAST_SIGNAL
};


static G_GNUC_UNUSED guint signals[LAST_SIGNAL] = {0};


/* MOO_TYPE_WINDOW */
G_DEFINE_TYPE (MooWindow, moo_window, GTK_TYPE_WINDOW)
static gpointer moo_window_grand_parent_class;


#define INSTALL_PROP(prop_id,name)                                          \
    g_object_class_install_property (gobject_class, prop_id,                \
        g_param_spec_boolean (name, name, name, FALSE, G_PARAM_READABLE))

static void
moo_window_class_init (MooWindowClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    GtkWindowClass *window_class = GTK_WINDOW_CLASS (klass);

    moo_window_grand_parent_class = g_type_class_peek_parent (moo_window_parent_class);

    gobject_class->constructor = moo_window_constructor;
    gobject_class->dispose = moo_window_dispose;
    gobject_class->set_property = moo_window_set_property;
    gobject_class->get_property = moo_window_get_property;

    widget_class->delete_event = moo_window_delete_event;
    widget_class->key_press_event = moo_window_key_press_event;
    window_class->set_focus = moo_window_set_focus;

    klass->close = moo_window_close_handler;

    moo_window_class_set_id (klass, "MooWindow", "Window");

    _moo_edit_ops_iface_install ();

    moo_window_class_new_action (klass, "ConfigureShortcuts", NULL,
                                 "label", _("Configure _Shortcuts..."),
                                 "no-accel", TRUE,
                                 "stock-id", MOO_STOCK_KEYBOARD,
                                 "closure-callback", moo_window_shortcuts_prefs_dialog,
                                 NULL);

    moo_window_class_new_action (klass, "ShowToolbar", NULL,
                                 "action-type::", MOO_TYPE_TOGGLE_ACTION,
                                 "display-name", _("Show Toolbar"),
                                 "label", _("Show Toolbar"),
                                 "condition::active", "toolbar-visible",
                                 NULL);

    moo_window_class_new_action (klass, "ShowMenubar", NULL,
                                 "action-type::", MOO_TYPE_TOGGLE_ACTION,
                                 "display-name", _("Show Menubar"),
                                 "label", _("Show Menubar"),
                                 "condition::active", "menubar-visible",
                                 "connect-accel", TRUE,
                                 NULL);

    moo_window_class_new_action (klass, "ShowStatusbar", NULL,
                                 "action-type::", MOO_TYPE_TOGGLE_ACTION,
                                 "display-name", _("Show Statusbar"),
                                 "label", _("Show Statusbar"),
                                 "condition::active", "statusbar-visible",
                                 NULL);

    moo_window_class_new_action_custom (klass, TOOLBAR_STYLE_ACTION_ID, NULL,
                                        create_toolbar_style_action,
                                        NULL, NULL);

    moo_window_class_new_action (klass, "Cut", NULL,
                                 "display-name", GTK_STOCK_CUT,
                                 "label", GTK_STOCK_CUT,
                                 "tooltip", GTK_STOCK_CUT,
                                 "stock-id", GTK_STOCK_CUT,
                                 "default-accel", MOO_ACCEL_CUT,
                                 "closure-callback", moo_window_action_cut,
                                 "condition::sensitive", "can-cut",
                                 NULL);

    moo_window_class_new_action (klass, "Copy", NULL,
                                 "display-name", GTK_STOCK_COPY,
                                 "label", GTK_STOCK_COPY,
                                 "tooltip", GTK_STOCK_COPY,
                                 "stock-id", GTK_STOCK_COPY,
                                 "default-accel", MOO_ACCEL_COPY,
                                 "closure-callback", moo_window_action_copy,
                                 "condition::sensitive", "can-copy",
                                 NULL);

    moo_window_class_new_action (klass, "Paste", NULL,
                                 "display-name", GTK_STOCK_PASTE,
                                 "label", GTK_STOCK_PASTE,
                                 "tooltip", GTK_STOCK_PASTE,
                                 "stock-id", GTK_STOCK_PASTE,
                                 "default-accel", MOO_ACCEL_PASTE,
                                 "closure-callback", moo_window_action_paste,
                                 "condition::sensitive", "can-paste",
                                 NULL);

    moo_window_class_new_action (klass, "Delete", NULL,
                                 "display-name", GTK_STOCK_DELETE,
                                 "label", GTK_STOCK_DELETE,
                                 "tooltip", GTK_STOCK_DELETE,
                                 "stock-id", GTK_STOCK_DELETE,
                                 "closure-callback", moo_window_action_delete,
                                 "condition::sensitive", "can-delete",
                                 NULL);

    moo_window_class_new_action (klass, "SelectAll", NULL,
                                 "display-name", GTK_STOCK_SELECT_ALL,
                                 "label", GTK_STOCK_SELECT_ALL,
                                 "tooltip", GTK_STOCK_SELECT_ALL,
                                 "stock-id", GTK_STOCK_SELECT_ALL,
                                 "default-accel", MOO_ACCEL_SELECT_ALL,
                                 "closure-callback", moo_window_action_select_all,
                                 "condition::sensitive", "can-select-all",
                                 NULL);

    moo_window_class_new_action (klass, "Undo", NULL,
                                 "display-name", GTK_STOCK_UNDO,
                                 "label", GTK_STOCK_UNDO,
                                 "tooltip", GTK_STOCK_UNDO,
                                 "stock-id", GTK_STOCK_UNDO,
                                 "default-accel", MOO_ACCEL_UNDO,
                                 "closure-callback", moo_window_action_undo,
                                 "condition::sensitive", "can-undo",
                                 NULL);

    moo_window_class_new_action (klass, "Redo", NULL,
                                 "display-name", GTK_STOCK_REDO,
                                 "label", GTK_STOCK_REDO,
                                 "tooltip", GTK_STOCK_REDO,
                                 "stock-id", GTK_STOCK_REDO,
                                 "default-accel", MOO_ACCEL_REDO,
                                 "closure-callback", moo_window_action_redo,
                                 "condition::sensitive", "can-redo",
                                 NULL);

    g_object_class_install_property (gobject_class, PROP_ACCEL_GROUP,
        g_param_spec_object ("accel-group", "accel-group", "accel-group",
                             GTK_TYPE_ACCEL_GROUP, G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_MENUBAR_UI_NAME,
        g_param_spec_string ("menubar-ui-name", "menubar-ui-name", "menubar-ui-name",
                             NULL, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_TOOLBAR_UI_NAME,
        g_param_spec_string ("toolbar-ui-name", "toolbar-ui-name", "toolbar-ui-name",
                             NULL, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_ID,
        g_param_spec_string ("id", "id", "id", NULL, G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_ACTIONS,
        g_param_spec_object ("actions", "actions", "actions",
                             MOO_TYPE_ACTION_COLLECTION, G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_UI_XML,
        g_param_spec_object ("ui-xml", "ui-xml", "ui-xml",
                             MOO_TYPE_UI_XML, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_TOOLBAR,
        g_param_spec_object ("toolbar", "toolbar", "toolbar",
                             GTK_TYPE_TOOLBAR, G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_MENUBAR,
        g_param_spec_object ("menubar", "menubar", "menubar",
                             GTK_TYPE_MENU, G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_TOOLBAR_VISIBLE,
        g_param_spec_boolean ("toolbar-visible", "toolbar-visible", "toolbar-visible",
                              TRUE, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_MENUBAR_VISIBLE,
        g_param_spec_boolean ("menubar-visible", "menubar-visible", "menubar-visible",
                              TRUE, (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_STATUSBAR_VISIBLE,
        g_param_spec_boolean ("statusbar-visible", "statusbar-visible", "statusbar-visible",
                              TRUE, (GParamFlags) G_PARAM_READWRITE));

    INSTALL_PROP (PROP_MEO_CAN_COPY, "can-copy");
    INSTALL_PROP (PROP_MEO_CAN_CUT, "can-cut");
    INSTALL_PROP (PROP_MEO_CAN_PASTE, "can-paste");
    INSTALL_PROP (PROP_MEO_CAN_DELETE, "can-delete");
    INSTALL_PROP (PROP_MEO_CAN_SELECT_ALL, "can-select-all");

    INSTALL_PROP (PROP_MUO_CAN_UNDO, "can-undo");
    INSTALL_PROP (PROP_MUO_CAN_REDO, "can-redo");

    signals[CLOSE] =
            g_signal_new ("close",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (MooWindowClass, close),
                      moo_signal_accumulator_continue_cancel,
                      GINT_TO_POINTER (MOO_CLOSE_RESPONSE_CONTINUE),
                      _moo_marshal_ENUM__VOID,
                      MOO_TYPE_CLOSE_RESPONSE, 0);
}


#ifdef __WIN32__
// Workaround for maximized vertically state bug: in that case the window size is
// exactly the vertical size of the monitor (minus task bar), and when the window
// is shown, it gets shown below the top edge, so the bottom piece of the window
// is covered by the task bar. To work that around, we save and restore the window
// position as well.
// Note, this is only a partial workaround, consequent windows still will not be
// positioned correctly.
static void
move_first_window (MooWindow *window)
{
    GList *l;
    GList *toplevels;
    gboolean first;
    int x, y;

    x = moo_prefs_get_int (setting (window, PREFS_X));
    y = moo_prefs_get_int (setting (window, PREFS_Y));

    if (x == DEFAULT_X || y == DEFAULT_Y)
        return;

    // Do this only for the first window
    first = TRUE;
    toplevels = gtk_window_list_toplevels ();
    for (l = toplevels; l != NULL; l = l->next)
    {
        if (window != l->data && MOO_IS_WINDOW (l->data))
        {
            first = FALSE;
            break;
        }
    }
    g_list_free (toplevels);
    toplevels = NULL;

    if (first)
        gtk_window_move (GTK_WINDOW (window), x, y);
}
#endif // __WIN32__


static GObject *
moo_window_constructor (GType                  type,
                        guint                  n_props,
                        GObjectConstructParam *props)
{
    GtkWidget *vbox;
    MooWindow *window;
    MooWindowClass *klass;
    GtkAction *action;

    GObject *object =
        G_OBJECT_CLASS(moo_window_parent_class)->constructor (type, n_props, props);

    window = MOO_WINDOW (object);

    klass = g_type_class_ref (type);
    moo_window_set_id (window, moo_window_class_get_id (klass));
    window->priv->name = g_strdup (moo_window_class_get_name (klass));

    init_prefs (window);

    moo_window_create_class_actions (window);
    window_instances = g_slist_prepend (window_instances, object);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    window->priv->menubar_holder = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (window->priv->menubar_holder);
    gtk_box_pack_start (GTK_BOX (vbox), window->priv->menubar_holder, FALSE, FALSE, 0);

    window->priv->toolbar_holder = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (window->priv->toolbar_holder);
    gtk_box_pack_start (GTK_BOX (vbox), window->priv->toolbar_holder, FALSE, FALSE, 0);

    gtk_box_pack_end (GTK_BOX (vbox), window->status_area, FALSE, FALSE, 0);

    window->menubar = NULL;
    window->toolbar = NULL;

    gtk_box_pack_start (GTK_BOX (vbox), window->vbox, TRUE, TRUE, 0);

    g_signal_connect (window, "notify::toolbar-ui-name",
                      G_CALLBACK (moo_window_update_toolbar), NULL);
    g_signal_connect (window, "notify::menubar-ui-name",
                      G_CALLBACK (moo_window_update_menubar), NULL);
    g_signal_connect (window, "notify::ui-xml",
                      G_CALLBACK (moo_window_update_ui), NULL);

    if (default_geometry && *default_geometry)
    {
        if (!gtk_window_parse_geometry (GTK_WINDOW (window), default_geometry))
            g_printerr (_("Could not parse geometry string '%s'\n"), default_geometry);
    }
    else if (moo_prefs_get_bool (setting (window, PREFS_REMEMBER_SIZE)))
    {
        int width = moo_prefs_get_int (setting (window, PREFS_WIDTH));
        int height = moo_prefs_get_int (setting (window, PREFS_HEIGHT));

        gtk_window_set_default_size (GTK_WINDOW (window), width, height);

#ifdef __WIN32__
        move_first_window (window);
#endif // __WIN32__

        if (moo_prefs_get_bool (setting (window, PREFS_MAXIMIZED)))
            gtk_window_maximize (GTK_WINDOW (window));
    }

    g_signal_connect (window, "configure-event",
                      G_CALLBACK (moo_window_save_size), NULL);

    moo_window_set_toolbar_visible (window,
        moo_prefs_get_bool (setting (window, PREFS_SHOW_TOOLBAR)));
    action = moo_window_get_action (window, "ShowToolbar");
    _moo_sync_toggle_action (action, window, "toolbar-visible", FALSE);

    moo_window_set_menubar_visible (window,
        moo_prefs_get_bool (setting (window, PREFS_SHOW_MENUBAR)));
    action = moo_window_get_action (window, "ShowMenubar");
    _moo_sync_toggle_action (action, window, "menubar-visible", FALSE);

    moo_window_set_statusbar_visible (window,
        moo_prefs_get_bool (setting (window, PREFS_SHOW_STATUSBAR)));
    action = moo_window_get_action (window, "ShowStatusbar");
    _moo_sync_toggle_action (action, window, "statusbar-visible", FALSE);

    moo_window_update_ui (window);

    accels_changed (window);

    g_type_class_unref (klass);
    return object;
}


void
moo_window_set_default_geometry (const char *geometry)
{
    char *tmp = default_geometry;
    default_geometry = g_strdup (geometry);
    g_free (tmp);
}


static void
parse_shadow_style (void)
{
    static gboolean been_here;

    if (!been_here)
    {
        gtk_rc_parse_string (
            "style \"no-shadow\" {\n"
            "    GtkStatusbar::shadow-type = GTK_SHADOW_NONE\n"
            "}\n"
            "widget \"no-shadow\" style \"no-shadow\"\n"
        );
        been_here = TRUE;
    }
}

static void
moo_window_init (MooWindow *window)
{
    GtkWidget *rg;

    window->priv = g_new0 (MooWindowPrivate, 1);

    window->vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (window->vbox);

    parse_shadow_style ();

    window->status_area = gtk_hbox_new (FALSE, 0);
    window->statusbar = g_object_new (GTK_TYPE_STATUSBAR,
                                      "has-resize-grip", FALSE,
                                      (const char*) NULL);
    gtk_widget_set_name (GTK_WIDGET (window->statusbar), "no-shadow");
    gtk_box_pack_start (GTK_BOX (window->status_area),
                        GTK_WIDGET (window->statusbar),
                        TRUE, TRUE, 0);
    rg = g_object_new (GTK_TYPE_STATUSBAR,
#ifdef GDK_WINDOWING_QUARTZ
                       "has-resize-grip", FALSE,
#else
                       "has-resize-grip", TRUE,
#endif
                       (const char*) NULL);
    gtk_widget_set_name (rg, "no-shadow");
    gtk_box_pack_end (GTK_BOX (window->status_area),
                      rg, FALSE, FALSE, 0);
    gtk_widget_set_size_request (rg, 24, -1);
    gtk_widget_show_all (window->status_area);

    window->priv->toolbar_visible = TRUE;
    window->priv->menubar_visible = TRUE;
    window->priv->statusbar_visible = TRUE;

    window->accel_group = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW (window),
                                window->accel_group);
}


static void
moo_window_dispose (GObject *object)
{
    MooWindow *window = MOO_WINDOW(object);

    window_instances = g_slist_remove (window_instances, object);

    if (window->priv)
    {
        window->priv->default_eo_widget = NULL;
        moo_window_disconnect_eo_widget (window);

        window->priv->default_uo_widget = NULL;
        moo_window_disconnect_uo_widget (window);

        if (window->priv->ui_xml)
            g_object_unref (window->priv->ui_xml);

        if (window->priv->actions)
        {
            _moo_action_collection_set_window (window->priv->actions, NULL);
            g_object_unref (window->priv->actions);
        }

        g_slist_foreach (window->priv->global_accels,
                         (GFunc) accel_entry_free, NULL);
        g_slist_free (window->priv->global_accels);

        if (window->priv->update_accels_idle)
            g_source_remove (window->priv->update_accels_idle);

        g_free (window->priv->name);
        g_free (window->priv->id);
        g_free (window->priv->menubar_ui_name);
        g_free (window->priv->toolbar_ui_name);

        if (window->accel_group)
            g_object_unref (window->accel_group);

        if (window->priv->save_size_id)
            g_source_remove (window->priv->save_size_id);
        window->priv->save_size_id = 0;

        g_free (window->priv);
        window->priv = NULL;
    }

    G_OBJECT_CLASS (moo_window_parent_class)->dispose (object);
}


static gboolean
moo_window_delete_event (GtkWidget      *widget,
                         G_GNUC_UNUSED GdkEventAny *event)
{
    MooCloseResponse result = FALSE;
    g_signal_emit_by_name (widget, "close", &result);
    return TRUE;
}


static AccelEntry *
accel_entry_new (guint            key,
                 GdkModifierType  mods,
                 GtkAction       *action)
{
    AccelEntry *entry = g_slice_new0 (AccelEntry);
    entry->keyval = key;
    entry->modifiers = mods;
    entry->action = g_object_ref (action);
    return entry;
}

static void
accel_entry_free (AccelEntry *entry)
{
    if (entry)
    {
        g_object_unref (entry->action);
        g_slice_free (AccelEntry, entry);
    }
}

static gboolean
update_accels (MooWindow *window)
{
    const GSList *l;

    window->priv->update_accels_idle = 0;

    g_slist_foreach (window->priv->global_accels, (GFunc) accel_entry_free, NULL);
    g_slist_free (window->priv->global_accels);
    window->priv->global_accels = NULL;

    for (l = moo_action_collection_get_groups (window->priv->actions); l != NULL; l = l->next)
    {
        GtkActionGroup *group = l->data;
        GList *actions;

        actions = gtk_action_group_list_actions (group);

        while (actions != NULL)
        {
            GtkAction *action = actions->data;
            const char *accel_path = NULL;

            if (MOO_IS_ACTION (action) &&
                !_moo_action_get_no_accel (action))
                    accel_path = gtk_action_get_accel_path (action);

            if (accel_path && _moo_accel_prefs_get_global (accel_path))
            {
                const char *accel;
                guint key;
                GdkModifierType mods;

                accel = _moo_get_accel (accel_path);

                if (accel && accel[0] && _moo_accel_parse (accel, &key, &mods))
                {
                    AccelEntry *entry = accel_entry_new (key, mods, action);
                    window->priv->global_accels =
                        g_slist_prepend (window->priv->global_accels, entry);
                }
            }

            actions = g_list_delete_link (actions, actions);
        }
    }

    return FALSE;
}

static void
accels_changed (MooWindow *window)
{
    if (!window->priv->update_accels_idle)
        window->priv->update_accels_idle =
            g_idle_add ((GSourceFunc) update_accels, window);
}

static gboolean
activate_global_accel (MooWindow   *window,
                       GdkEventKey *event)
{
    guint keyval;
    GdkModifierType mods;
    GSList *l;

    if (!window->priv->global_accels)
        return FALSE;

    moo_accel_translate_event (GTK_WIDGET (window), event, &keyval, &mods);

    for (l = window->priv->global_accels; l != NULL; l = l->next)
    {
        AccelEntry *entry = l->data;

        if (entry->keyval == keyval && entry->modifiers == mods)
        {
            gtk_action_activate (entry->action);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean
moo_window_key_press_event (GtkWidget   *widget,
                            GdkEventKey *event)
{
    MooWindow *window = MOO_WINDOW (widget);

    if (window->priv->global_accels_mode)
        return GTK_WIDGET_CLASS (moo_window_parent_class)->key_press_event (widget, event);
    else
        return activate_global_accel (window, event) ||
               /* Call propagate_key_event() and activate_key() in inverse order
                * from that used by Gtk, so that the focused widget gets it first. */
               gtk_window_propagate_key_event (GTK_WINDOW (widget), event) ||
               gtk_window_activate_key (GTK_WINDOW (widget), event) ||
               /* GtkWindowClass would call two guys above again and then chain up
                * to the parent, so it's a shortcut */
               GTK_WIDGET_CLASS (moo_window_grand_parent_class)->key_press_event (widget, event);
}


void
moo_window_set_global_accels (MooWindow *window,
                              gboolean   global)
{
    g_return_if_fail (MOO_IS_WINDOW (window));
    window->priv->global_accels_mode = global != 0;
}


static gboolean
save_size (MooWindow *window)
{
    window->priv->save_size_id = 0;

    if (MOO_IS_WINDOW (window) && GTK_WIDGET_REALIZED (window))
    {
        GdkWindowState state;
        state = gdk_window_get_state (GTK_WIDGET(window)->window);
        moo_prefs_set_bool (setting (window, PREFS_MAXIMIZED),
                            state & GDK_WINDOW_STATE_MAXIMIZED);

        if (!(state & GDK_WINDOW_STATE_MAXIMIZED))
        {
            int width, height;
            gtk_window_get_size (GTK_WINDOW (window), &width, &height);
            moo_prefs_set_int (setting (window, PREFS_WIDTH), width);
            moo_prefs_set_int (setting (window, PREFS_HEIGHT), height);

#ifdef __WIN32__
            // see move_first_window()
            {
                int x, y;
                gtk_window_get_position (GTK_WINDOW (window), &x, &y);
                // It refuses to restore a window snapped to the left,
                // move it one pixel to the right.
                if (x == 0 && y == 0)
                    x = 1;
                moo_prefs_set_int (setting (window, PREFS_X), x);
                moo_prefs_set_int (setting (window, PREFS_Y), y);
            }
#endif // __WIN32__
        }
    }

    return FALSE;
}


static gboolean
moo_window_save_size (MooWindow *window)
{
    if (!window->priv->save_size_id)
        window->priv->save_size_id =
                g_idle_add ((GSourceFunc)save_size, window);
    return FALSE;
}


static MooCloseResponse
moo_window_close_handler (G_GNUC_UNUSED MooWindow *window)
{
    return MOO_CLOSE_RESPONSE_CONTINUE;
}

gboolean
moo_window_close (MooWindow *window)
{
    MooCloseResponse response = MOO_CLOSE_RESPONSE_CONTINUE;

    g_signal_emit_by_name (window, "close", &response);

    if (response == MOO_CLOSE_RESPONSE_CANCEL)
    {
        return FALSE;
    }
    else
    {
        gtk_widget_destroy (GTK_WIDGET (window));
        return TRUE;
    }
}


void
moo_window_message (MooWindow  *window,
                    const char *text)
{
    g_return_if_fail (MOO_IS_WINDOW (window));

    gtk_statusbar_pop (window->statusbar, 0);

    if (text && text[0])
        gtk_statusbar_push (window->statusbar, 0, text);
}


static void
moo_window_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
    const char *name = NULL;
    MooWindow *window = MOO_WINDOW (object);

    switch (prop_id)
    {
        case PROP_TOOLBAR_UI_NAME:
            name = g_value_get_string (value);
            MOO_ASSIGN_STRING (window->priv->toolbar_ui_name, MOO_NZS (name));
            g_object_notify (object, "toolbar-ui-name");
            break;

        case PROP_MENUBAR_UI_NAME:
            name = g_value_get_string (value);
            MOO_ASSIGN_STRING (window->priv->menubar_ui_name, MOO_NZS (name));
            g_object_notify (object, "menubar-ui-name");
            break;

        case PROP_UI_XML:
            moo_window_set_ui_xml (window, g_value_get_object (value));
            break;

        case PROP_TOOLBAR_VISIBLE:
            moo_window_set_toolbar_visible (window, g_value_get_boolean (value));
            break;

        case PROP_MENUBAR_VISIBLE:
            moo_window_set_menubar_visible (window, g_value_get_boolean (value));
            break;

        case PROP_STATUSBAR_VISIBLE:
            moo_window_set_statusbar_visible (window, g_value_get_boolean (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_window_get_property (GObject      *object,
                         guint         prop_id,
                         GValue       *value,
                         GParamSpec   *pspec)
{
    MooWindow *window = MOO_WINDOW (object);

    switch (prop_id)
    {
        case PROP_ACCEL_GROUP:
            g_value_set_object (value,
                                window->accel_group);
            break;

        case PROP_TOOLBAR_UI_NAME:
            g_value_set_string (value,
                                window->priv->toolbar_ui_name);
            break;

        case PROP_MENUBAR_UI_NAME:
            g_value_set_string (value,
                                window->priv->menubar_ui_name);
            break;

        case PROP_ID:
            g_value_set_string (value, window->priv->id);
            break;

        case PROP_UI_XML:
            g_value_set_object (value, window->priv->ui_xml);
            break;

        case PROP_MENUBAR:
            g_value_set_object (value, window->menubar);
            break;

        case PROP_TOOLBAR:
            g_value_set_object (value, window->toolbar);
            break;

        case PROP_ACTIONS:
            g_value_set_object (value, moo_window_get_actions (window));
            break;

        case PROP_TOOLBAR_VISIBLE:
            g_value_set_boolean (value, window->priv->toolbar_visible);
            break;

        case PROP_MENUBAR_VISIBLE:
            g_value_set_boolean (value, window->priv->menubar_visible);
            break;

        case PROP_STATUSBAR_VISIBLE:
            g_value_set_boolean (value, window->priv->statusbar_visible);
            break;

        case PROP_MEO_CAN_COPY:
            g_value_set_boolean (value, window->priv->eo_widget &&
                _moo_edit_ops_can_do_op (G_OBJECT (window->priv->eo_widget), MOO_EDIT_OP_COPY));
            break;
        case PROP_MEO_CAN_CUT:
            g_value_set_boolean (value, window->priv->eo_widget &&
                _moo_edit_ops_can_do_op (G_OBJECT (window->priv->eo_widget), MOO_EDIT_OP_CUT));
            break;
        case PROP_MEO_CAN_PASTE:
            g_value_set_boolean (value, window->priv->eo_widget &&
                _moo_edit_ops_can_do_op (G_OBJECT (window->priv->eo_widget), MOO_EDIT_OP_PASTE));
            break;
        case PROP_MEO_CAN_DELETE:
            g_value_set_boolean (value, window->priv->eo_widget &&
                _moo_edit_ops_can_do_op (G_OBJECT (window->priv->eo_widget), MOO_EDIT_OP_DELETE));
            break;
        case PROP_MEO_CAN_SELECT_ALL:
            g_value_set_boolean (value, window->priv->eo_widget &&
                _moo_edit_ops_can_do_op (G_OBJECT (window->priv->eo_widget), MOO_EDIT_OP_SELECT_ALL));
            break;

        case PROP_MUO_CAN_UNDO:
            g_value_set_boolean (value, window->priv->uo_widget &&
                _moo_undo_ops_can_undo (G_OBJECT (window->priv->uo_widget)));
            break;
        case PROP_MUO_CAN_REDO:
            g_value_set_boolean (value, window->priv->uo_widget &&
                _moo_undo_ops_can_redo (G_OBJECT (window->priv->uo_widget)));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


void
moo_window_set_title (MooWindow  *window,
                      const char *title)
{
    const char *old_title;

    g_return_if_fail (window != NULL);

    old_title = gtk_window_get_title (GTK_WINDOW (window));

    if (!moo_str_equal (title, old_title))
        gtk_window_set_title (GTK_WINDOW (window), MOO_NZS (title));
}


static void
moo_window_update_toolbar (MooWindow *window)
{
    MooUiXml *xml;
    GtkToolbarStyle style;
    char *ui_name;
    MooActionCollection *actions;

    g_return_if_fail (MOO_IS_WINDOW (window));

    actions = moo_window_get_actions (window);
    xml = moo_window_get_ui_xml (window);
    ui_name = window->priv->toolbar_ui_name;
    ui_name = ui_name && ui_name[0] ? ui_name : NULL;

    if (window->toolbar)
    {
        MooUiXml *old_xml;
        char *old_name;

        old_xml = g_object_get_data (G_OBJECT (window->toolbar), "moo-window-ui-xml");
        old_name = g_object_get_data (G_OBJECT (window->toolbar), "moo-window-ui-name");

        if (!old_xml || old_xml != xml || !ui_name || strcmp (ui_name, old_name))
        {
            gtk_widget_destroy (window->toolbar);
            window->toolbar = NULL;
        }
    }

    if (window->toolbar || !xml || !ui_name)
        return;

    window->toolbar = moo_ui_xml_create_widget (xml, MOO_UI_TOOLBAR,
                                                ui_name, actions,
                                                window->accel_group);
    g_return_if_fail (window->toolbar != NULL);

    g_object_set_data_full (G_OBJECT (window->toolbar), "moo-window-ui-xml",
                            g_object_ref (xml), g_object_unref);
    g_object_set_data_full (G_OBJECT (window->toolbar), "moo-window-ui-name",
                            g_strdup (ui_name), g_free);

    gtk_box_pack_start (GTK_BOX (window->priv->toolbar_holder),
                        window->toolbar, FALSE, FALSE, 0);

    style = get_toolbar_style (window);
    gtk_toolbar_set_style (GTK_TOOLBAR (MOO_WINDOW(window)->toolbar), style);

    g_object_notify (G_OBJECT (window), "toolbar");
}


static void
moo_window_update_menubar (MooWindow *window)
{
    MooUiXml *xml;
    char *ui_name;
    MooActionCollection *actions;

    g_return_if_fail (MOO_IS_WINDOW (window));

    actions = moo_window_get_actions (window);
    xml = moo_window_get_ui_xml (window);
    ui_name = window->priv->menubar_ui_name;
    ui_name = ui_name && ui_name[0] ? ui_name : NULL;

    if (window->menubar)
    {
        MooUiXml *old_xml;
        char *old_name;

        old_xml = g_object_get_data (G_OBJECT (window->menubar), "moo-window-ui-xml");
        old_name = g_object_get_data (G_OBJECT (window->menubar), "moo-window-ui-name");

        if (!old_xml || old_xml != xml || !ui_name || strcmp (ui_name, old_name))
        {
            gtk_widget_destroy (window->menubar);
            window->menubar = NULL;
        }
    }

    if (window->menubar || !xml || !ui_name)
        return;

    window->menubar = moo_ui_xml_create_widget (xml, MOO_UI_MENUBAR,
                                                ui_name, actions,
                                                window->accel_group);
    g_return_if_fail (window->menubar != NULL);

    g_object_set_data_full (G_OBJECT (window->menubar), "moo-window-ui-xml",
                            g_object_ref (xml), g_object_unref);
    g_object_set_data_full (G_OBJECT (window->menubar), "moo-window-ui-name",
                            g_strdup (ui_name), g_free);

    gtk_box_pack_start (GTK_BOX (window->priv->menubar_holder),
                        window->menubar, FALSE, FALSE, 0);

    g_object_notify (G_OBJECT (window), "menubar");
}


static void
moo_window_update_ui (MooWindow *window)
{
    moo_window_update_toolbar (window);
    moo_window_update_menubar (window);
}


static void
moo_window_shortcuts_prefs_dialog (MooWindow *window)
{
    if (_moo_accel_prefs_dialog_run (moo_window_get_actions (window),
                                     GTK_WIDGET (window)))
        accels_changed (window);
}


static void
set_ui_elm_visible (MooWindow  *window,
                    gboolean   *flag,
                    const char *prop,
                    const char *prefs_key,
                    GtkWidget  *widget,
                    gboolean    visible)
{
    if (!visible != !*flag)
    {
        *flag = visible != 0;
        g_object_set (widget, "visible", visible, NULL);
        moo_prefs_set_bool (setting (window, prefs_key), visible);
        g_object_notify (G_OBJECT (window), prop);
    }
}

static void
moo_window_set_toolbar_visible (MooWindow  *window,
                                gboolean    visible)
{
    set_ui_elm_visible (window, &window->priv->toolbar_visible,
                        "toolbar-visible", PREFS_SHOW_TOOLBAR,
                        window->priv->toolbar_holder,
                        visible);
}

static void
moo_window_set_menubar_visible (MooWindow  *window,
                                gboolean    visible)
{
    set_ui_elm_visible (window, &window->priv->menubar_visible,
                        "menubar-visible", PREFS_SHOW_MENUBAR,
                        window->priv->menubar_holder,
                        visible);
}

static void
moo_window_set_statusbar_visible (MooWindow *window,
                                  gboolean   visible)
{
    set_ui_elm_visible (window, &window->priv->statusbar_visible,
                        "statusbar-visible", PREFS_SHOW_STATUSBAR,
                        window->status_area,
                        visible);
}


static void
toolbar_style_toggled (MooWindow            *window,
                       gpointer              data)
{
    GtkToolbarStyle style = GPOINTER_TO_INT (data);
    if (window->toolbar)
        gtk_toolbar_set_style (GTK_TOOLBAR (window->toolbar), style);
    moo_prefs_set_int (setting (window, PREFS_TOOLBAR_STYLE), style);
}


#define N_STYLES 4
#define ICONS_ONLY "icons-only"
#define LABELS_ONLY "labels-only"
#define ICONS_AND_LABELS "icons-and-labels"
#define ICONS_AND_LABELS_HORIZ "icons-and-labels-horiz"

static GtkAction*
create_toolbar_style_action (MooWindow      *window,
                             G_GNUC_UNUSED gpointer dummy)
{
    GtkAction *action;
    guint i;
    GtkToolbarStyle style;
    MooMenuMgr *menu_mgr;

    const char *labels[N_STYLES] = {
        N_("_Icons Only"),
        N_("_Labels Only"),
        N_("Labels _Below Icons"),
        N_("Labels Be_side Icons")
    };

    const char *ids[N_STYLES] = {
        ICONS_ONLY,
        LABELS_ONLY,
        ICONS_AND_LABELS,
        ICONS_AND_LABELS_HORIZ
    };

    action = moo_menu_action_new (TOOLBAR_STYLE_ACTION_ID, _("Toolbar _Style"));
    menu_mgr = moo_menu_action_get_mgr (MOO_MENU_ACTION (action));
    moo_menu_mgr_set_use_mnemonic (menu_mgr, TRUE);

    for (i = 0; i < N_STYLES; ++i)
        moo_menu_mgr_append (menu_mgr, NULL,
                             ids[i], _(labels[i]), NULL,
                             MOO_MENU_ITEM_RADIO,
                             GINT_TO_POINTER (i), NULL);

    style = get_toolbar_style (window);
    moo_menu_mgr_set_active (menu_mgr, ids[style], TRUE);

    g_signal_connect_swapped (menu_mgr, "radio-set-active",
                              G_CALLBACK (toolbar_style_toggled), window);

    return action;
}


static GtkToolbarStyle
get_toolbar_style_gtk (MooWindow *window)
{
    GtkSettings *settings = gtk_widget_get_settings (GTK_WIDGET (window));
    GtkToolbarStyle style = GTK_TOOLBAR_ICONS;
    gpointer toolbar_class;

    g_return_val_if_fail (settings != NULL, style);

    toolbar_class = g_type_class_ref (GTK_TYPE_TOOLBAR);
    g_object_get (settings, "gtk-toolbar-style", &style, NULL);
    g_type_class_unref (toolbar_class);

    g_return_val_if_fail (style < N_STYLES, 0);
    return style;
}

static void
init_prefs (MooWindow *window)
{
    moo_prefs_new_key_bool (setting (window, PREFS_REMEMBER_SIZE), TRUE);
    moo_prefs_new_key_bool (setting (window, PREFS_SHOW_TOOLBAR), TRUE);
    moo_prefs_new_key_bool (setting (window, PREFS_SHOW_MENUBAR), TRUE);
    moo_prefs_new_key_bool (setting (window, PREFS_SHOW_STATUSBAR), TRUE);
    moo_prefs_new_key_int (setting (window, PREFS_TOOLBAR_STYLE),
                           get_toolbar_style_gtk (window));

    moo_prefs_create_key (setting (window, PREFS_MAXIMIZED), MOO_PREFS_STATE, G_TYPE_BOOLEAN, FALSE);
    moo_prefs_create_key (setting (window, PREFS_WIDTH), MOO_PREFS_STATE, G_TYPE_INT, 800);
    moo_prefs_create_key (setting (window, PREFS_HEIGHT), MOO_PREFS_STATE, G_TYPE_INT, 600);
    moo_prefs_create_key (setting (window, PREFS_X), MOO_PREFS_STATE, G_TYPE_INT, DEFAULT_X);
    moo_prefs_create_key (setting (window, PREFS_Y), MOO_PREFS_STATE, G_TYPE_INT, DEFAULT_Y);
}

static GtkToolbarStyle
get_toolbar_style (MooWindow *window)
{
    GtkToolbarStyle s = moo_prefs_get_int (setting (window, PREFS_TOOLBAR_STYLE));
    g_return_val_if_fail (s < N_STYLES, GTK_TOOLBAR_ICONS);
    return s;
}

#undef N_STYLES


/*****************************************************************************/
/* Actions
 */

#define MOO_WINDOW_NAME_QUARK (moo_window_name_quark ())
MOO_DEFINE_QUARK_STATIC (moo-window-name, moo_window_name_quark)
#define MOO_WINDOW_ID_QUARK (moo_window_id_quark ())
MOO_DEFINE_QUARK_STATIC (moo-window-id, moo_window_id_quark)
#define MOO_WINDOW_ACTIONS_QUARK (moo_window_actions_quark ())
MOO_DEFINE_QUARK_STATIC (moo-window-actions, moo_window_actions_quark)

typedef struct {
    GHashTable *groups; /* name -> display_name */
    GHashTable *actions;
} ActionStore;

typedef struct {
    GValue *args;
    guint n_args;
    GType return_type;
    GCallback callback;
    GSignalCMarshaller marshal;
} ClosureInfo;

typedef struct {
    MooActionFactory *action;
    char *group;
    char **conditions;
    ClosureInfo *closure;
} ActionInfo;


static ActionInfo*
action_info_new (MooActionFactory *action,
                 const char       *group,
                 char            **conditions,
                 ClosureInfo      *closure)
{
    ActionInfo *info;

    g_return_val_if_fail (MOO_IS_ACTION_FACTORY (action), NULL);

    info = g_new0 (ActionInfo, 1);
    info->action = g_object_ref (action);
    info->group = g_strdup (group);
    info->conditions = g_strdupv (conditions);
    info->closure = closure;

    return info;
}


static void
action_info_free (ActionInfo *info)
{
    if (info)
    {
        if (info->closure)
        {
            guint i;
            for (i = 0; i < info->closure->n_args; ++i)
                g_value_unset (&info->closure->args[i]);
            g_free (info->closure);
        }

        g_object_unref (info->action);
        g_strfreev (info->conditions);
        g_free (info->group);
        g_free (info);
    }
}


static ActionStore *
action_store_new (void)
{
    ActionStore *store = g_new0 (ActionStore, 1);
    store->actions = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                            (GDestroyNotify) action_info_free);
    store->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    return store;
}


static ActionStore *
type_get_store (GType type)
{
    return g_type_get_qdata (type, MOO_WINDOW_ACTIONS_QUARK);
}


static ActionStore *
type_ensure_store (GType type)
{
    ActionStore *store = g_type_get_qdata (type, MOO_WINDOW_ACTIONS_QUARK);

    if (!store)
    {
        store = action_store_new ();
        g_type_set_qdata (type, MOO_WINDOW_ACTIONS_QUARK, store);
    }

    return store;
}


static void
action_activated (GtkAction   *action,
                  ClosureInfo *info)
{
    GClosure *closure;
    GValue return_value;
    GValue *retval_ptr = NULL;
    GValue *instance_and_params;

    closure = g_object_get_data (G_OBJECT (action), "moo-window-action-closure");
    g_return_if_fail (closure != NULL);

    instance_and_params = g_new (GValue, info->n_args + 1);
    if (info->n_args)
        MOO_ELMCPY (instance_and_params + 1, info->args, info->n_args);

    instance_and_params->g_type = 0;
    /* closure was created with closure_new_swap(), so first argument is NULL,
     * and it will be passed as last to the callback */
    g_value_init (instance_and_params, G_TYPE_POINTER);

    if (info->return_type != G_TYPE_NONE)
    {
        return_value.g_type = 0;
        g_value_init (&return_value, info->return_type);
        retval_ptr = &return_value;
    }

    g_closure_invoke (closure, retval_ptr, info->n_args + 1, instance_and_params, NULL);

    if (info->return_type != G_TYPE_NONE)
        g_value_unset (&return_value);

    g_free (instance_and_params);
}

static void
connect_closure (GtkAction   *action,
                 ClosureInfo *info,
                 MooWindow   *window)
{
    GClosure *closure;

    closure = g_cclosure_new_object_swap (info->callback, G_OBJECT (window));
    g_closure_set_marshal (closure, info->marshal);
    g_closure_ref (closure);
    g_closure_sink (closure);
    g_object_set_data_full (G_OBJECT (action), "moo-window-action-closure",
                            closure, (GDestroyNotify) g_closure_unref);

    g_signal_connect (action, "activate", G_CALLBACK (action_activated), info);
}

static void
disconnect_closure (GtkAction *action)
{
    g_signal_handlers_disconnect_matched (action,
                                          G_SIGNAL_MATCH_FUNC,
                                          0, 0, 0,
                                          (gpointer) action_activated,
                                          NULL);
}

static GtkAction *
create_action (const char *action_id,
               ActionInfo *info,
               MooWindow  *window)
{
    GtkAction *action;

    g_return_val_if_fail (info != NULL, NULL);
    g_return_val_if_fail (MOO_IS_ACTION_FACTORY (info->action), NULL);
    g_return_val_if_fail (action_id && action_id[0], NULL);

    if (g_type_is_a (info->action->action_type, MOO_TYPE_ACTION))
        action = moo_action_factory_create_action (info->action, window,
                                                   "closure-object", window,
                                                   "name", action_id,
                                                   NULL);
    else if (g_type_is_a (info->action->action_type, MOO_TYPE_TOGGLE_ACTION))
        action = moo_action_factory_create_action (info->action, window,
                                                   "toggled-object", window,
                                                   "name", action_id,
                                                   NULL);
    else
        action = moo_action_factory_create_action (info->action, window,
                                                   "name", action_id,
                                                   NULL);

    g_return_val_if_fail (action != NULL, NULL);

    if (info->conditions)
    {
        char **p;

        for (p = info->conditions; *p != NULL; p += 2)
        {
            gboolean invert;
            char *condition, *prop;

            invert = p[1][0] == '!';
            prop = p[1][0] == '!' ? p[1] + 1 : p[1];
            condition = p[0];

            if (!strcmp (condition, "active"))
                _moo_sync_toggle_action (action, window, prop, invert);
            else
                moo_bind_bool_property (action, condition, window, prop, invert);
        }
    }

    if (info->closure)
        connect_closure (action, info->closure, window);

    return action;
}


static const char *
moo_window_class_get_id (MooWindowClass *klass)
{
    GType type;

    g_return_val_if_fail (MOO_IS_WINDOW_CLASS (klass), NULL);

    type = G_OBJECT_CLASS_TYPE (klass);
    return g_type_get_qdata (type, MOO_WINDOW_ID_QUARK);
}


static const char *
moo_window_class_get_name (MooWindowClass     *klass)
{
    GType type;

    g_return_val_if_fail (MOO_IS_WINDOW_CLASS (klass), NULL);

    type = G_OBJECT_CLASS_TYPE (klass);
    return g_type_get_qdata (type, MOO_WINDOW_NAME_QUARK);
}


static void
moo_window_class_install_action (MooWindowClass     *klass,
                                 const char         *action_id,
                                 MooActionFactory   *factory,
                                 const char         *group,
                                 char              **conditions,
                                 ClosureInfo        *closure_info)
{
    ActionStore *store;
    ActionInfo *info;
    GSList *l;
    GType type;

    g_return_if_fail (MOO_IS_WINDOW_CLASS (klass));
    g_return_if_fail (MOO_IS_ACTION_FACTORY (factory));
    g_return_if_fail (action_id && action_id[0]);

    type = G_OBJECT_CLASS_TYPE (klass);
    store = type_ensure_store (type);

    if (g_hash_table_lookup (store->actions, action_id))
        moo_window_class_remove_action (klass, action_id);

    info = action_info_new (factory, group, conditions, closure_info);
    g_hash_table_insert (store->actions, g_strdup (action_id), info);

    for (l = window_instances; l != NULL; l = l->next)
    {
        if (g_type_is_a (G_OBJECT_TYPE (l->data), type))
        {
            GtkAction *action = create_action (action_id, info, l->data);

            if (action)
            {
                moo_window_add_action (l->data, group, action);
                g_object_unref (action);
            }
        }
    }
}


static GtkAction *
custom_action_factory_func (MooWindow        *window,
                            MooActionFactory *factory)
{
    MooWindowActionFunc func;
    gpointer func_data;

    g_return_val_if_fail (MOO_IS_WINDOW (window), NULL);

    func = g_object_get_data (G_OBJECT (factory), "moo-window-class-action-func");
    func_data = g_object_get_data (G_OBJECT (factory), "moo-window-class-action-func-data");

    g_return_val_if_fail (func != NULL, NULL);

    return func (window, func_data);
}


void
moo_window_class_new_action_custom (MooWindowClass     *klass,
                                    const char         *action_id,
                                    const char         *group,
                                    MooWindowActionFunc func,
                                    gpointer            data,
                                    GDestroyNotify      notify)
{
    MooActionFactory *action_factory;

    g_return_if_fail (MOO_IS_WINDOW_CLASS (klass));
    g_return_if_fail (action_id && action_id[0]);
    g_return_if_fail (func != NULL);

    action_factory = moo_action_factory_new_func ((MooActionFactoryFunc) custom_action_factory_func, NULL);
    g_object_set_data (G_OBJECT (action_factory), "moo-window-class-action-func", func);
    g_object_set_data_full (G_OBJECT (action_factory), "moo-window-class-action-func-data",
                            data, notify);

    moo_window_class_install_action (klass, action_id, action_factory, group, NULL, NULL);
    g_object_unref (action_factory);
}


/**
 * moo_window_class_find_action: (moo.lua 0) (moo.private 1)
 *
 * @klass: (type MooWindowClass*):
 * @id: (type const-utf8):
 **/
gboolean
moo_window_class_find_action (MooWindowClass *klass,
                              const char     *id)
{
    ActionStore *store;

    g_return_val_if_fail (MOO_IS_WINDOW_CLASS (klass), FALSE);

    store = type_get_store (G_OBJECT_CLASS_TYPE (klass));

    if (store)
        return g_hash_table_lookup (store->actions, id) != NULL;
    else
        return FALSE;
}


/**
 * moo_window_class_remove_action: (moo.lua 0) (moo.private 1)
 *
 * @klass: (type MooWindowClass*):
 * @id: (type const-utf8):
 **/
void
moo_window_class_remove_action (MooWindowClass     *klass,
                                const char         *action_id)
{
    ActionStore *store;
    GType type;
    GSList *l;

    g_return_if_fail (MOO_IS_WINDOW_CLASS (klass));
    g_return_if_fail (action_id != NULL);

    type = G_OBJECT_CLASS_TYPE (klass);
    store = type_get_store (type);

    if (store)
        g_hash_table_remove (store->actions, action_id);

    for (l = window_instances; l != NULL; l = l->next)
        if (g_type_is_a (G_OBJECT_TYPE (l->data), type))
            moo_window_remove_action (l->data, action_id);
}


void
moo_window_class_new_group (MooWindowClass *klass,
                            const char     *name,
                            const char     *display_name)
{
    ActionStore *store;
    GSList *l;
    GType type;

    g_return_if_fail (MOO_IS_WINDOW_CLASS (klass));
    g_return_if_fail (name != NULL);
    g_return_if_fail (display_name != NULL);

    type = G_OBJECT_CLASS_TYPE (klass);
    store = type_ensure_store (type);

    g_hash_table_insert (store->groups, g_strdup (name), g_strdup (display_name));

    for (l = window_instances; l != NULL; l = l->next)
    {
        if (g_type_is_a (G_OBJECT_TYPE (l->data), type))
        {
            MooWindow *window = l->data;
            moo_action_collection_add_group (window->priv->actions, name, display_name);
        }
    }
}


gboolean
moo_window_class_find_group (MooWindowClass *klass,
                             const char     *name)
{
    ActionStore *store;

    g_return_val_if_fail (MOO_IS_WINDOW_CLASS (klass), FALSE);

    store = type_get_store (G_OBJECT_CLASS_TYPE (klass));

    if (store)
        return g_hash_table_lookup (store->groups, name) != NULL;
    else
        return FALSE;
}


void
moo_window_class_remove_group (MooWindowClass *klass,
                               const char     *name)
{
    ActionStore *store;
    GType type;
    GSList *l;

    g_return_if_fail (MOO_IS_WINDOW_CLASS (klass));
    g_return_if_fail (name != NULL);

    type = G_OBJECT_CLASS_TYPE (klass);
    store = type_get_store (type);

    if (store)
        g_hash_table_remove (store->groups, name);

    for (l = window_instances; l != NULL; l = l->next)
    {
        if (g_type_is_a (G_OBJECT_TYPE (l->data), type))
        {
            MooWindow *window = l->data;
            GtkActionGroup *group = moo_action_collection_get_group (window->priv->actions, name);
            if (group)
                moo_action_collection_remove_group (window->priv->actions, group);
        }
    }
}


MooUiXml*
moo_window_get_ui_xml (MooWindow          *window)
{
    g_return_val_if_fail (MOO_IS_WINDOW (window), NULL);
    return window->priv->ui_xml;
}


void
moo_window_set_ui_xml (MooWindow          *window,
                       MooUiXml           *xml)
{
    g_return_if_fail (MOO_IS_WINDOW (window));
    g_return_if_fail (!xml || MOO_IS_UI_XML (xml));

    if (xml && xml == window->priv->ui_xml)
        return;

    if (window->priv->ui_xml)
        g_object_unref (window->priv->ui_xml);

    window->priv->ui_xml = xml ? g_object_ref (xml) : moo_ui_xml_new ();

    g_object_notify (G_OBJECT (window), "ui-xml");
}


MooActionCollection *
moo_window_get_actions (MooWindow *window)
{
    g_return_val_if_fail (MOO_IS_WINDOW (window), NULL);
    return window->priv->actions;
}


GtkAction *
moo_window_get_action (MooWindow  *window,
                       const char *action)
{
    g_return_val_if_fail (MOO_IS_WINDOW (window), NULL);
    g_return_val_if_fail (action != NULL, NULL);
    return moo_action_collection_get_action (window->priv->actions, action);
}


static void
moo_window_set_id (MooWindow      *window,
                   const char     *id)
{
    if (id)
    {
        g_free (window->priv->id);
        window->priv->id = g_strdup (id);
    }
}


static void
add_action (const char *id,
            ActionInfo *info,
            MooWindow  *window)
{
    GtkAction *action = create_action (id, info, window);

    if (action)
    {
        moo_window_add_action (window, info->group, action);
        g_object_unref (action);
    }
}

static void
add_group (const char *name,
           const char *display_name,
           MooWindow  *window)
{
    moo_action_collection_add_group (window->priv->actions, name, display_name);
}

static void
moo_window_create_class_actions (MooWindow *window)
{
    GType type;

    g_return_if_fail (MOO_IS_WINDOW (window));

    window->priv->actions = moo_action_collection_new (window->priv->id,
                                                       window->priv->name);
    _moo_action_collection_set_window (window->priv->actions, window);

    type = G_OBJECT_TYPE (window);

    while (TRUE)
    {
        ActionStore *store;

        store = type_get_store (type);

        if (store)
        {
            g_hash_table_foreach (store->groups, (GHFunc) add_group, window);
            g_hash_table_foreach (store->actions, (GHFunc) add_action, window);
        }

        if (type == MOO_TYPE_WINDOW)
            break;

        type = g_type_parent (type);
    }
}


void
moo_window_class_set_id (MooWindowClass     *klass,
                         const char         *id,
                         const char         *name)
{
    GType type;

    g_return_if_fail (MOO_IS_WINDOW_CLASS (klass));
    g_return_if_fail (id != NULL && name != NULL);

    type = G_OBJECT_CLASS_TYPE (klass);
    g_return_if_fail (g_type_get_qdata (type, MOO_WINDOW_ID_QUARK) == NULL);
    g_type_set_qdata (type, MOO_WINDOW_ID_QUARK, g_strdup (id));
    g_return_if_fail (g_type_get_qdata (type, MOO_WINDOW_NAME_QUARK) == NULL);
    g_type_set_qdata (type, MOO_WINDOW_NAME_QUARK, g_strdup (name));
}


static void
moo_window_add_action (MooWindow  *window,
                       const char *group_name,
                       GtkAction  *action)
{
    GtkActionGroup *group;
    MooActionCollection *coll;

    g_return_if_fail (MOO_IS_WINDOW (window));
    g_return_if_fail (MOO_IS_ACTION_BASE (action));

    coll = moo_window_get_actions (window);
    group = moo_action_collection_get_group (coll, group_name);
    g_return_if_fail (group != NULL);

    gtk_action_group_add_action (group, action);
    gtk_action_set_accel_group (action, window->accel_group);

    if (!_moo_action_get_dead (action) && !_moo_action_get_no_accel (action))
    {
        char *accel_path;

        accel_path = _moo_action_make_accel_path (action);
        _moo_accel_register (accel_path, _moo_action_get_default_accel (action));
        _moo_action_set_accel_path (action, accel_path);

        if (_moo_action_get_connect_accel (action))
            gtk_action_connect_accelerator (action);

        accels_changed (window);

        g_free (accel_path);
    }
}


static void
moo_window_remove_action (MooWindow  *window,
                          const char *action_id)
{
    MooActionCollection *coll;
    GtkAction *action;

    g_return_if_fail (MOO_IS_WINDOW (window));
    g_return_if_fail (action_id != NULL);

    coll = moo_window_get_actions (window);
    action = moo_action_collection_get_action (coll, action_id);

    if (action)
    {
        disconnect_closure (action);
        moo_action_collection_remove_action (coll, action);
    }
}


#define COLLECT_ARGS(n_args__,args_array__,vargs__,error__)     \
G_STMT_START {                                                  \
    guint i__;                                                  \
                                                                \
    for (i__ = 0; i__ < (n_args__); ++i__)                      \
    {                                                           \
        GValue value__;                                         \
        GType type__;                                           \
                                                                \
        type__ = va_arg ((vargs__), GType);                     \
                                                                \
        if (!G_TYPE_IS_VALUE_TYPE (type__))                     \
        {                                                       \
            *(error__) = g_strdup_printf ("%s: invalid type",   \
                                          G_STRLOC);            \
            break;                                              \
        }                                                       \
                                                                \
        value__.g_type = 0;                                     \
        g_value_init (&value__, type__);                        \
        G_VALUE_COLLECT (&value__, (vargs__), 0, (error__));    \
                                                                \
        if (*(error__))                                         \
            break;                                              \
                                                                \
        g_array_append_val ((args_array__), value__);           \
    }                                                           \
} G_STMT_END


#define COLLECT_PROPS(action_class__,action_type__,action_params__,conditions__,prop_name__,vargs__,error__)    \
G_STMT_START {                                                                                                  \
    while (prop_name__)                                                                                         \
    {                                                                                                           \
        GParameter param__ = {NULL, {0, {{0}, {0}}}};                                                           \
                                                                                                                \
        /* ignore id property */                                                                                \
        if (!strcmp ((prop_name__), "id") || !strcmp ((prop_name__), "name"))                                   \
        {                                                                                                       \
            *(error__) = g_strdup_printf ("%s: id property specified", G_STRLOC);                               \
            break;                                                                                              \
        }                                                                                                       \
                                                                                                                \
        if (!strcmp ((prop_name__), "action-type::") || !strcmp ((prop_name__), "action_type::"))               \
        {                                                                                                       \
            g_value_init (&param__.value, MOO_TYPE_GTYPE);                                                      \
            G_VALUE_COLLECT (&param__.value, (vargs__), 0, (error__));                                          \
                                                                                                                \
            if (*(error__))                                                                                     \
                break;                                                                                          \
                                                                                                                \
            *(action_type__) = _moo_value_get_gtype (&param__.value);                                           \
                                                                                                                \
            if (!g_type_is_a (*(action_type__), MOO_TYPE_ACTION_BASE))                                          \
            {                                                                                                   \
                *(error__) = g_strdup_printf ("%s: invalid action type %s",                                     \
                                              G_STRLOC, g_type_name (*(action_type__)));                        \
                break;                                                                                          \
            }                                                                                                   \
                                                                                                                \
            *(action_class__) = g_type_class_ref (*(action_type__));                                            \
        }                                                                                                       \
        else if (!strncmp ((prop_name__), "condition::", strlen ("condition::")))                               \
        {                                                                                                       \
            const char *suffix__ = strstr ((prop_name__), "::");                                                \
                                                                                                                \
            if (!suffix__ || !suffix__[1] || !suffix__[2])                                                      \
            {                                                                                                   \
                *(error__) = g_strdup_printf ("%s: invalid condition name '%s'",                                \
                                              G_STRLOC, (prop_name__));                                         \
                break;                                                                                          \
            }                                                                                                   \
                                                                                                                \
            g_ptr_array_add ((conditions__), g_strdup (suffix__ + 2));                                          \
                                                                                                                \
            (prop_name__) = va_arg ((vargs__), gchar*);                                                         \
                                                                                                                \
            if (!(prop_name__))                                                                                 \
            {                                                                                                   \
                *(error__) = g_strdup_printf ("%s: unterminated '%s' property", G_STRLOC,                       \
                                              (char*) g_ptr_array_index ((conditions__),                        \
                                                                         (conditions__)->len - 1));             \
                break;                                                                                          \
            }                                                                                                   \
                                                                                                                \
            g_ptr_array_add ((conditions__), g_strdup (prop_name__));                                           \
        }                                                                                                       \
        else                                                                                                    \
        {                                                                                                       \
            GParamSpec *pspec__;                                                                                \
                                                                                                                \
            if (!*(action_class__))                                                                             \
            {                                                                                                   \
                if (!*(action_type__))                                                                          \
                    *(action_type__) = MOO_TYPE_ACTION;                                                         \
                *(action_class__) = g_type_class_ref (*(action_type__));                                        \
            }                                                                                                   \
                                                                                                                \
            pspec__ = g_object_class_find_property (*(action_class__), (prop_name__));                          \
                                                                                                                \
            if (!pspec__)                                                                                       \
            {                                                                                                   \
                *(error__) = g_strdup_printf ("%s: no property '%s' in class '%s'",                             \
                                              G_STRLOC, (prop_name__),                                          \
                                              g_type_name (*(action_type__)));                                  \
                break;                                                                                          \
            }                                                                                                   \
                                                                                                                \
            g_value_init (&param__.value, G_PARAM_SPEC_VALUE_TYPE (pspec__));                                   \
            G_VALUE_COLLECT (&param__.value, (vargs__), 0, (error__));                                          \
                                                                                                                \
            if (*(error__))                                                                                     \
            {                                                                                                   \
                g_value_unset (&param__.value);                                                                 \
                break;                                                                                          \
            }                                                                                                   \
                                                                                                                \
            param__.name = g_strdup (prop_name__);                                                              \
            g_array_append_val ((action_params__), param__);                                                    \
        }                                                                                                       \
                                                                                                                \
        (prop_name__) = va_arg ((vargs__), char*);                                                              \
    }                                                                                                           \
} G_STMT_END

static gboolean
collect_params_and_props (guint              n_callback_args,
                          MooActionFactory **action_factory_p,
                          char            ***conditions_p,
                          GValue           **callback_args_p,
                          char             **error,
                          va_list            var_args)
{
    const char *prop_name;
    GArray *callback_args = NULL;
    GType action_type = 0;
    GObjectClass *action_class = NULL;
    GArray *action_params = NULL;
    GPtrArray *conditions = NULL;

    *error = NULL;

    g_return_val_if_fail (!n_callback_args || callback_args_p != NULL, FALSE);

    conditions = g_ptr_array_new ();
    action_params = g_array_new (FALSE, TRUE, sizeof (GParameter));

    if (n_callback_args)
    {
        callback_args = g_array_sized_new (FALSE, FALSE, sizeof (GValue),
                                           n_callback_args);
        COLLECT_ARGS (n_callback_args, callback_args, var_args, error);
    }

    if (*error)
        goto error;

    prop_name = va_arg (var_args, char*);

    COLLECT_PROPS (&action_class, &action_type,
                   action_params, conditions,
                   prop_name, var_args, error);

    if (*error)
        goto error;

    {
        MooActionFactory *action_factory;

        action_factory = moo_action_factory_new_a (action_type,
                                                   (GParameter*) action_params->data,
                                                   action_params->len);

        if (!action_factory)
        {
            g_warning ("oops");
            goto error;
        }

        *action_factory_p = action_factory;
        _moo_param_array_free ((GParameter*) action_params->data, action_params->len);
        g_array_free (action_params, FALSE);
        action_params = NULL;

        g_ptr_array_add (conditions, NULL);
        *conditions_p = (char**) g_ptr_array_free (conditions, FALSE);
        conditions = NULL;

        if (n_callback_args)
            *callback_args_p = (GValue*) g_array_free (callback_args, FALSE);
        else if (callback_args)
            g_array_free (callback_args, TRUE);

        if (action_class)
            g_type_class_unref (action_class);

        return TRUE;
    }

error:
    if (action_params)
    {
        guint i;
        GParameter *params = (GParameter*) action_params->data;

        for (i = 0; i < action_params->len; ++i)
        {
            g_value_unset (&params[i].value);
            g_free ((char*) params[i].name);
        }

        g_array_free (action_params, TRUE);
    }

    if (callback_args)
    {
        guint i;
        for (i = 0; i < callback_args->len; ++i)
            g_value_unset (&g_array_index (callback_args, GValue, i));
        g_array_free (callback_args, TRUE);
    }

    if (conditions)
    {
        guint i;
        for (i = 0; i < conditions->len; ++i)
            g_free (g_ptr_array_index (conditions, i));
        g_ptr_array_free (conditions, TRUE);
    }

    if (action_class)
        g_type_class_unref (action_class);

    return FALSE;
}


void
moo_window_class_new_action (MooWindowClass     *klass,
                              const char         *action_id,
                              const char         *group,
                              ...)
{
    MooActionFactory *action_factory;
    char **conditions;
    char *error = NULL;
    gboolean success;
    va_list var_args;

    va_start (var_args, group);
    success = collect_params_and_props (0, &action_factory, &conditions,
                                        NULL, &error, var_args);
    va_end (var_args);

    if (!success)
    {
        if (error)
            g_critical ("%s", error);
        g_free (error);
        return;
    }

    moo_window_class_install_action (klass, action_id, action_factory, group, conditions, NULL);

    g_object_unref (action_factory);
    g_strfreev (conditions);
}


void
_moo_window_class_new_action_callback (MooWindowClass *klass,
                                       const char     *id,
                                       const char     *group,
                                       GCallback       callback,
                                       GSignalCMarshaller marshal,
                                       GType           return_type,
                                       guint           n_args,
                                       ...)
{
    GValue *callback_args = NULL;
    MooActionFactory *action_factory;
    char **conditions;
    char *error = NULL;
    gboolean success;
    ClosureInfo *closure_info;
    va_list args_and_props;

    va_start (args_and_props, n_args);
    success = collect_params_and_props (n_args, &action_factory, &conditions,
                                        &callback_args, &error, args_and_props);
    va_end (args_and_props);

    if (!success)
    {
        if (error)
            g_critical ("%s", error);
        g_free (error);
        return;
    }

    closure_info = g_new0 (ClosureInfo, 1);
    closure_info->n_args = n_args;
    closure_info->args = callback_args;
    closure_info->return_type = return_type;
    closure_info->callback = callback;
    closure_info->marshal = marshal;

    moo_window_class_install_action (klass, id, action_factory,
                                     group, conditions, closure_info);

    g_object_unref (action_factory);
    g_strfreev (conditions);
}


/*************************************************************************/
/* MooEditOps
 */

void
moo_window_set_edit_ops_widget (MooWindow *window,
                                GtkWidget *widget)
{
    g_return_if_fail (MOO_IS_WINDOW (window));
    g_return_if_fail (!widget || GTK_IS_WIDGET (widget));

    if (widget == window->priv->default_eo_widget)
        return;

    if (widget)
    {
        GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
        g_return_if_fail (toplevel == GTK_WIDGET (window));
        g_return_if_fail (_moo_edit_ops_check (G_OBJECT (widget)));
    }

    /* XXX remove it when it's removed from the window or destroyed */
    window->priv->default_eo_widget = widget;
}


static void
moo_window_action_cut (MooWindow *window)
{
    g_return_if_fail (window->priv->eo_widget != NULL);
    _moo_edit_ops_do_op (G_OBJECT (window->priv->eo_widget),
                         MOO_EDIT_OP_CUT);
}

static void
moo_window_action_copy (MooWindow *window)
{
    g_return_if_fail (window->priv->eo_widget != NULL);
    _moo_edit_ops_do_op (G_OBJECT (window->priv->eo_widget),
                         MOO_EDIT_OP_COPY);
}

static void
moo_window_action_paste (MooWindow *window)
{
    g_return_if_fail (window->priv->eo_widget != NULL);
    _moo_edit_ops_do_op (G_OBJECT (window->priv->eo_widget),
                         MOO_EDIT_OP_PASTE);
}

static void
moo_window_action_delete (MooWindow *window)
{
    g_return_if_fail (window->priv->eo_widget != NULL);
    _moo_edit_ops_do_op (G_OBJECT (window->priv->eo_widget),
                         MOO_EDIT_OP_DELETE);
}

static void
moo_window_action_select_all (MooWindow *window)
{
    g_return_if_fail (window->priv->eo_widget != NULL);
    _moo_edit_ops_do_op (G_OBJECT (window->priv->eo_widget),
                         MOO_EDIT_OP_SELECT_ALL);
}

static void
moo_window_action_undo (MooWindow *window)
{
    g_return_if_fail (window->priv->uo_widget != NULL);
    _moo_undo_ops_undo (G_OBJECT (window->priv->uo_widget));
}

static void
moo_window_action_redo (MooWindow *window)
{
    g_return_if_fail (window->priv->uo_widget != NULL);
    _moo_undo_ops_redo (G_OBJECT (window->priv->uo_widget));
}


static void
emit_can_do_op_changed (MooWindow     *window,
                        MooEditOpType  type)
{
    switch (type)
    {
        case MOO_EDIT_OP_CUT:
            g_object_notify (G_OBJECT (window), "can-cut");
            break;
        case MOO_EDIT_OP_COPY:
            g_object_notify (G_OBJECT (window), "can-copy");
            break;
        case MOO_EDIT_OP_PASTE:
            g_object_notify (G_OBJECT (window), "can-paste");
            break;
        case MOO_EDIT_OP_DELETE:
            g_object_notify (G_OBJECT (window), "can-delete");
            break;
        case MOO_EDIT_OP_SELECT_ALL:
            g_object_notify (G_OBJECT (window), "can-select-all");
            break;
        default:
            g_return_if_reached ();
    }
}

static void
moo_window_connect_eo_widget (MooWindow *window,
                              GtkWidget *widget)
{
    g_return_if_fail (_moo_edit_ops_check (G_OBJECT (widget)));

    window->priv->eo_widget = g_object_ref (widget);

    _moo_edit_ops_connect (G_OBJECT (widget));
    g_signal_connect_swapped (widget, "moo-edit-ops-can-do-op-changed",
                              G_CALLBACK (emit_can_do_op_changed), window);
}

static void
moo_window_disconnect_eo_widget (MooWindow *window)
{
    GtkWidget *widget;

    widget = window->priv->eo_widget;
    window->priv->eo_widget = NULL;

    if (widget)
    {
        _moo_edit_ops_disconnect (G_OBJECT (widget));
        g_signal_handlers_disconnect_by_func (widget,
                                              (gpointer) emit_can_do_op_changed,
                                              window);
        g_object_unref (widget);
    }
}

static GtkWidget *
find_widget_for_edit_ops (MooWindow *window)
{
    GtkWidget *widget = gtk_window_get_focus (GTK_WINDOW (window));

    while (widget)
    {
        if (_moo_edit_ops_check (G_OBJECT (widget)))
            return widget;
        widget = widget->parent;
    }

    return window->priv ? window->priv->default_eo_widget : NULL;
}

static void
check_edit_ops_widget (MooWindow *window)
{
    GtkWidget *widget;

    widget = find_widget_for_edit_ops (window);

    if (window->priv && widget != window->priv->eo_widget)
    {
        int i;

        moo_window_disconnect_eo_widget (window);

        if (widget)
            moo_window_connect_eo_widget (window, widget);

        for (i = 0; i < MOO_N_EDIT_OPS; i++)
            emit_can_do_op_changed (window, i);
    }
}


static void
emit_can_undo_changed (MooWindow *window)
{
    g_object_notify (G_OBJECT (window), "can-undo");
}

static void
emit_can_redo_changed (MooWindow *window)
{
    g_object_notify (G_OBJECT (window), "can-redo");
}

static void
moo_window_connect_uo_widget (MooWindow *window,
                              GtkWidget *widget)
{
    g_return_if_fail (_moo_undo_ops_check (G_OBJECT (widget)));

    window->priv->uo_widget = g_object_ref (widget);

    g_signal_connect_swapped (widget, "moo-undo-ops-can-undo-changed",
                              G_CALLBACK (emit_can_undo_changed), window);
    g_signal_connect_swapped (widget, "moo-undo-ops-can-redo-changed",
                              G_CALLBACK (emit_can_redo_changed), window);
}

static void
moo_window_disconnect_uo_widget (MooWindow *window)
{
    GtkWidget *widget;

    widget = window->priv->uo_widget;
    window->priv->uo_widget = NULL;

    if (widget)
    {
        g_signal_handlers_disconnect_by_func (widget,
                                              (gpointer) emit_can_undo_changed,
                                              window);
        g_signal_handlers_disconnect_by_func (widget,
                                              (gpointer) emit_can_redo_changed,
                                              window);
        g_object_unref (widget);
    }
}

static GtkWidget *
find_widget_for_undo_ops (MooWindow *window)
{
    GtkWidget *widget = gtk_window_get_focus (GTK_WINDOW (window));

    while (widget)
    {
        if (_moo_undo_ops_check (G_OBJECT (widget)))
            return widget;

        widget = widget->parent;
    }

    return window->priv ? window->priv->default_uo_widget : NULL;
}

static void
check_undo_ops_widget (MooWindow *window)
{
    GtkWidget *widget;

    widget = find_widget_for_undo_ops (window);

    if (window->priv && widget != window->priv->uo_widget)
    {
        moo_window_disconnect_uo_widget (window);

        if (widget)
            moo_window_connect_uo_widget (window, widget);

        emit_can_undo_changed (window);
        emit_can_redo_changed (window);
    }
}

static void
moo_window_set_focus (GtkWindow *window,
                      GtkWidget *widget)
{
    GTK_WINDOW_CLASS (moo_window_parent_class)->set_focus (window, widget);
    check_edit_ops_widget (MOO_WINDOW (window));
    check_undo_ops_widget (MOO_WINDOW (window));
}
