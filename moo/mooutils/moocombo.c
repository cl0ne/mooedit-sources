/*
 *   moocombo.c
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
 * class:MooCombo: (parent GtkTable) (constructable) (moo.private 1)
 **/

#include "marshals.h"
#include "mooutils/moocombo.h"
#include "mooutils/mooentry.h"
#include "mooutils/moocompat.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>


#define MAX_POPUP_LEN 15


struct _MooComboPrivate {
    gboolean use_button;
    GtkWidget *button;
    GtkSizeGroup* size_group;

    GtkWidget *popup;
    GtkTreeView *treeview;
    GtkTreeViewColumn *column;
    GtkTreeModel *model;
    GtkTreeRowReference *active_row;

    gboolean walking_list;
    char *real_text;

    int text_column;
    GtkCellRenderer *text_cell;

    MooComboGetTextFunc get_text_func;
    gpointer get_text_data;

    MooComboRowSeparatorFunc row_separator_func;
    gpointer row_separator_data;
};


static void moo_combo_cell_layout_init                  (GtkCellLayoutIface *iface);
static void moo_combo_cell_layout_pack_start            (GtkCellLayout      *cell_layout,
                                                         GtkCellRenderer    *cell,
                                                         gboolean            expand);
static void moo_combo_cell_layout_pack_end              (GtkCellLayout      *cell_layout,
                                                         GtkCellRenderer    *cell,
                                                         gboolean            expand);
static void moo_combo_cell_layout_clear                 (GtkCellLayout      *cell_layout);
static void moo_combo_cell_layout_add_attribute         (GtkCellLayout      *cell_layout,
                                                         GtkCellRenderer    *cell,
                                                         const gchar        *attribute,
                                                         gint                column);
static void moo_combo_cell_layout_set_cell_data_func    (GtkCellLayout      *cell_layout,
                                                         GtkCellRenderer    *cell,
                                                         GtkCellLayoutDataFunc func,
                                                         gpointer            func_data,
                                                         GDestroyNotify      destroy);
static void moo_combo_cell_layout_clear_attributes      (GtkCellLayout      *cell_layout,
                                                         GtkCellRenderer    *cell);
static void moo_combo_cell_layout_reorder               (GtkCellLayout      *cell_layout,
                                                         GtkCellRenderer    *cell,
                                                         gint                position);

static void     moo_combo_class_init        (MooComboClass  *klass);
static void     moo_combo_init              (MooCombo       *combo);
static void     moo_combo_destroy           (GtkObject      *object);
static void     moo_combo_set_property      (GObject        *object,
                                             guint           prop_id,
                                             const GValue   *value,
                                             GParamSpec     *pspec);
static void     moo_combo_get_property      (GObject        *object,
                                             guint           prop_id,
                                             GValue         *value,
                                             GParamSpec     *pspec);

static void     moo_combo_unmap             (GtkWidget      *widget);
static void     moo_combo_unrealize         (GtkWidget      *widget);
static gboolean moo_combo_focus             (GtkWidget      *widget,
                                             GtkDirectionType direction);

static void     moo_combo_popup_real        (MooCombo       *combo);
static void     moo_combo_popdown_real      (MooCombo       *combo);
static void     moo_combo_changed           (MooCombo       *combo);
static gboolean moo_combo_popup_key_press   (MooCombo       *combo,
                                             GdkEventKey    *event);

static void     create_arrow_button         (MooCombo       *combo);
static void     button_clicked              (MooCombo       *combo);

static void     create_popup_tree_view      (MooCombo       *combo);
static void     create_popup_window         (MooCombo       *combo);
static void     destroy_popup_window        (MooCombo       *combo);
static gboolean resize_popup                (MooCombo       *combo);
static gboolean entry_focus_out             (MooCombo       *combo);
static gboolean popup_button_press          (MooCombo       *combo,
                                             GdkEventButton *event);
static gboolean popup_key_press_cb          (MooCombo       *combo,
                                             GdkEventKey    *event);
static int      popup_get_selected          (MooCombo       *combo);
static gboolean list_button_press           (MooCombo       *combo,
                                             GdkEventButton *event);
static char    *default_get_text_func       (GtkTreeModel   *model,
                                             GtkTreeIter    *iter,
                                             gpointer        data);

static void     entry_changed               (MooCombo       *combo);


GType
moo_combo_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (!type))
    {
        static const GTypeInfo info =
        {
            sizeof (MooComboClass),
            NULL,		/* base_init */
            NULL,		/* base_finalize */
            (GClassInitFunc) moo_combo_class_init,
            NULL,		/* class_finalize */
            NULL,		/* class_data */
            sizeof (MooCombo),
            0,
            (GInstanceInitFunc) moo_combo_init,
            NULL
        };

        static const GInterfaceInfo cell_layout_info =
        {
            (GInterfaceInitFunc) moo_combo_cell_layout_init,
            NULL,
            NULL
        };

        type = g_type_register_static (GTK_TYPE_TABLE, "MooCombo", &info, (GTypeFlags) 0);
        g_type_add_interface_static (type, GTK_TYPE_CELL_LAYOUT, &cell_layout_info);
    }

    return type;
}


static gpointer moo_combo_parent_class = NULL;

enum {
    PROP_0,
    PROP_ACTIVATES_DEFAULT,
    PROP_USE_BUTTON
};

enum {
    POPUP,
    POPDOWN,
    CHANGED,
    POPUP_KEY_PRESS,
    NUM_SIGNALS
};

static guint signals[NUM_SIGNALS];

static void
moo_combo_class_init (MooComboClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    GtkBindingSet *binding_set;

    g_type_class_add_private (klass, sizeof (MooComboPrivate));

    moo_combo_parent_class = g_type_class_peek_parent (klass);

    gobject_class->set_property = moo_combo_set_property;
    gobject_class->get_property = moo_combo_get_property;

    gtkobject_class->destroy = moo_combo_destroy;

    widget_class->unmap = moo_combo_unmap;
    widget_class->unrealize = moo_combo_unrealize;
    widget_class->focus = moo_combo_focus;

    klass->popup = moo_combo_popup_real;
    klass->popdown = moo_combo_popdown_real;
    klass->popup_key_press = moo_combo_popup_key_press;

    g_object_class_install_property (gobject_class,
                                     PROP_ACTIVATES_DEFAULT,
                                     g_param_spec_boolean ("activates-default",
                                             "Activates default",
                                             "Whether to activate the default widget (such as the default button in a dialog) when Enter is pressed",
                                             FALSE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_USE_BUTTON,
                                     g_param_spec_boolean ("use-button",
                                             "use-button",
                                             "use-button",
                                             TRUE,
                                             (GParamFlags) G_PARAM_READWRITE));

    signals[POPUP] =
            g_signal_new ("popup",
                          G_OBJECT_CLASS_TYPE (klass),
                          (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                          G_STRUCT_OFFSET (MooComboClass, popup),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[POPDOWN] =
            g_signal_new ("popdown",
                          G_OBJECT_CLASS_TYPE (klass),
                          (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                          G_STRUCT_OFFSET (MooComboClass, popdown),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[CHANGED] =
            g_signal_new ("changed",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooComboClass, changed),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[POPUP_KEY_PRESS] =
            g_signal_new ("popup-key-press",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooComboClass, popup_key_press),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOLEAN__BOXED,
                          G_TYPE_BOOLEAN, 1,
                          GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

    binding_set = gtk_binding_set_by_class (klass);
    gtk_binding_entry_add_signal (binding_set, GDK_space, GDK_CONTROL_MASK,
                                  "popup", 0);
}


static void
moo_combo_cell_layout_init (GtkCellLayoutIface *iface)
{
    iface->pack_start = moo_combo_cell_layout_pack_start;
    iface->pack_end = moo_combo_cell_layout_pack_end;
    iface->clear = moo_combo_cell_layout_clear;
    iface->add_attribute = moo_combo_cell_layout_add_attribute;
    iface->set_cell_data_func = moo_combo_cell_layout_set_cell_data_func;
    iface->clear_attributes = moo_combo_cell_layout_clear_attributes;
    iface->reorder = moo_combo_cell_layout_reorder;
}


static void
moo_combo_init (MooCombo *combo)
{
    combo->priv = G_TYPE_INSTANCE_GET_PRIVATE (combo, MOO_TYPE_COMBO, MooComboPrivate);

    combo->priv->use_button = TRUE;

    combo->priv->get_text_func = default_get_text_func;
    combo->priv->get_text_data = combo;

    gtk_table_resize (GTK_TABLE (combo), 1, 2);

    combo->priv->size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

    combo->entry = moo_entry_new ();
    gtk_widget_show (combo->entry);
    gtk_table_attach (GTK_TABLE (combo), combo->entry,
                      0, 1, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) 0, 0, 0);
    gtk_size_group_add_widget (combo->priv->size_group, combo->entry);

    g_signal_connect_swapped (combo->entry, "changed",
                              G_CALLBACK (entry_changed), combo);

    create_arrow_button (combo);
    create_popup_tree_view (combo);
}


static void
create_arrow_button (MooCombo       *combo)
{
    GtkWidget *arrow, *frame;

    combo->priv->button = gtk_button_new ();
    gtk_button_set_focus_on_click (GTK_BUTTON (combo->priv->button), FALSE);
    gtk_widget_show (combo->priv->button);
    gtk_size_group_add_widget (combo->priv->size_group, combo->priv->button);
    gtk_table_attach (GTK_TABLE (combo), combo->priv->button,
                      1, 2, 0, 1,
                      (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);

    g_signal_connect_swapped (combo->priv->button, "clicked",
                              G_CALLBACK (button_clicked), combo);

    frame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1, FALSE);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
    gtk_widget_show (frame);
    gtk_container_add (GTK_CONTAINER (combo->priv->button), frame);

    arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_ETCHED_IN);
    gtk_widget_set_size_request (arrow, 10, 10);
    gtk_widget_show (arrow);
    gtk_container_add (GTK_CONTAINER (frame), arrow);
}


static void
moo_combo_destroy (GtkObject *object)
{
    MooCombo *combo = MOO_COMBO (object);

    moo_combo_set_model (combo, NULL);

    combo->priv->button = NULL;
    combo->entry = NULL;

    gtk_tree_row_reference_free (combo->priv->active_row);
    combo->priv->active_row = NULL;

    if (combo->priv->model)
    {
        g_object_unref (combo->priv->model);
        combo->priv->model = NULL;
    }

    if (combo->priv->treeview)
    {
        g_object_unref (combo->priv->treeview);
        combo->priv->treeview = NULL;
    }

    if (combo->priv->popup)
    {
        gtk_widget_destroy (combo->priv->popup);
        combo->priv->popup = NULL;
        combo->priv->treeview = NULL;
    }

    if (combo->priv->size_group)
    {
        g_object_unref (combo->priv->size_group);
        combo->priv->size_group = NULL;
    }

    GTK_OBJECT_CLASS(moo_combo_parent_class)->destroy (object);
}


static void
moo_combo_set_property (GObject        *object,
                        guint           prop_id,
                        const GValue   *value,
                        GParamSpec     *pspec)
{
    MooCombo *combo = MOO_COMBO (object);

    switch (prop_id)
    {
        case PROP_ACTIVATES_DEFAULT:
            gtk_entry_set_activates_default (GTK_ENTRY (combo->entry),
                                             g_value_get_boolean (value));
            break;

        case PROP_USE_BUTTON:
            moo_combo_set_use_button (combo, g_value_get_boolean (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_combo_get_property (GObject        *object,
                        guint           prop_id,
                        GValue         *value,
                        GParamSpec     *pspec)
{
    MooCombo *combo = MOO_COMBO (object);

    switch (prop_id)
    {
        case PROP_ACTIVATES_DEFAULT:
            g_value_set_boolean (value, gtk_entry_get_activates_default (GTK_ENTRY (combo->entry)));
            break;

        case PROP_USE_BUTTON:
            g_value_set_boolean (value, combo->priv->use_button);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


GtkWidget *
moo_combo_new (void)
{
    return GTK_WIDGET (g_object_new (MOO_TYPE_COMBO, (const char*) NULL));
}


static void
create_popup_window (MooCombo *combo)
{
    GtkWidget *scrolled_window, *frame;

    if (combo->priv->popup)
        return;

    combo->priv->popup = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_widget_set_size_request (combo->priv->popup, -1, -1);
    gtk_window_set_default_size (GTK_WINDOW (combo->priv->popup), 1, 1);
    gtk_window_set_resizable (GTK_WINDOW (combo->priv->popup), FALSE);
    gtk_widget_add_events (combo->priv->popup, GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK);

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request (scrolled_window, -1, -1);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                         GTK_SHADOW_ETCHED_IN);
    /* a nasty hack to get the treeview to size nicely */
    gtk_widget_set_size_request (GTK_SCROLLED_WINDOW (scrolled_window)->vscrollbar, -1, 0);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_NONE);
    gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (combo->priv->treeview));

    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
    gtk_container_add (GTK_CONTAINER (frame), scrolled_window);

    gtk_widget_show_all (frame);
    gtk_container_add (GTK_CONTAINER (combo->priv->popup), frame);
}

static void
destroy_popup_window (MooCombo *combo)
{
    if (combo->priv->popup)
    {
        GtkWidget *tree_view = GTK_WIDGET (combo->priv->treeview);
        gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (tree_view)), tree_view);
        gtk_widget_destroy (combo->priv->popup);
        combo->priv->popup = NULL;
    }
}

static void
create_popup_tree_view (MooCombo *combo)
{
    GtkTreeSelection *selection;

    combo->priv->column = gtk_tree_view_column_new ();

    combo->priv->treeview = GTK_TREE_VIEW (gtk_tree_view_new ());
    g_object_ref_sink (combo->priv->treeview);

    gtk_widget_set_size_request (GTK_WIDGET (combo->priv->treeview), -1, -1);
    gtk_tree_view_append_column (combo->priv->treeview, combo->priv->column);
    gtk_tree_view_set_headers_visible (combo->priv->treeview, FALSE);
    gtk_tree_view_set_hover_selection (combo->priv->treeview, TRUE);

    selection = gtk_tree_view_get_selection (combo->priv->treeview);
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
}


void
moo_combo_popup (MooCombo   *combo)
{
    g_return_if_fail (MOO_IS_COMBO (combo));
    g_signal_emit (combo, signals[POPUP], 0);
}


void
moo_combo_popdown (MooCombo   *combo)
{
    g_return_if_fail (MOO_IS_COMBO (combo));

    if (combo->priv->walking_list)
    {
        combo->priv->walking_list = FALSE;
        g_free (combo->priv->real_text);
        combo->priv->real_text = NULL;
    }

    g_signal_emit (combo, signals[POPDOWN], 0);
}


/* XXX _gtk_entry_get_borders from gtkentry.c */
static void
entry_get_borders (GtkEntry *entry,
                   gint     *xborder,
                   gint     *yborder)
{
    GtkWidget *widget = GTK_WIDGET (entry);
    gint focus_width;
    gboolean interior_focus;

    gtk_widget_style_get (widget,
                          "interior-focus", &interior_focus,
                          "focus-line-width", &focus_width,
                          NULL);

    if (entry->has_frame)
    {
        *xborder = widget->style->xthickness;
        *yborder = widget->style->ythickness;
    }
    else
    {
        *xborder = 0;
        *yborder = 0;
    }

    if (!interior_focus)
    {
        *xborder += focus_width;
        *yborder += focus_width;
    }
}


static gboolean
model_is_empty (GtkTreeModel *model)
{
    GtkTreeIter iter;
    return !gtk_tree_model_get_iter_first (model, &iter);
}


static void
moo_combo_popup_real (MooCombo *combo)
{
    GtkWidget *window;
    GtkTreeSelection *selection;

    if (moo_combo_popup_shown (combo))
        return;

    if (model_is_empty (combo->priv->model))
        return;

    window = gtk_widget_get_toplevel (GTK_WIDGET (combo->entry));
    g_return_if_fail (GTK_IS_WINDOW (window));

    create_popup_window (combo);

    gtk_widget_realize (combo->priv->popup);

    if (GTK_WINDOW (window)->group)
        gtk_window_group_add_window (GTK_WINDOW (window)->group,
                                     GTK_WINDOW (combo->priv->popup));
    gtk_window_set_modal (GTK_WINDOW (combo->priv->popup), TRUE);

    resize_popup (combo);

    gtk_widget_show (combo->priv->popup);

    gtk_widget_ensure_style (GTK_WIDGET (combo->priv->treeview));
    gtk_widget_modify_bg (GTK_WIDGET (combo->priv->treeview), GTK_STATE_ACTIVE,
                          &GTK_WIDGET(combo->priv->treeview)->style->base[GTK_STATE_SELECTED]);
    gtk_widget_modify_base (GTK_WIDGET (combo->priv->treeview), GTK_STATE_ACTIVE,
                            &GTK_WIDGET(combo->priv->treeview)->style->base[GTK_STATE_SELECTED]);

    gtk_grab_add (combo->priv->popup);
    gdk_pointer_grab (combo->priv->popup->window, TRUE,
                      (GdkEventMask) (GDK_BUTTON_PRESS_MASK |
                              GDK_BUTTON_RELEASE_MASK |
                              GDK_POINTER_MOTION_MASK),
                      NULL, NULL, GDK_CURRENT_TIME);

    g_signal_connect_swapped (combo->entry, "focus-out-event",
                              G_CALLBACK (entry_focus_out), combo);

    g_signal_connect_swapped (combo->priv->popup, "button-press-event",
                              G_CALLBACK (popup_button_press), combo);
    g_signal_connect_swapped (combo->priv->popup, "key-press-event",
                              G_CALLBACK (popup_key_press_cb), combo);

    g_signal_connect_swapped (combo->priv->treeview, "button-press-event",
                              G_CALLBACK (list_button_press), combo);

    /* treeview selects something on focus */
    selection = gtk_tree_view_get_selection (combo->priv->treeview);
    gtk_tree_selection_unselect_all (selection);
}


static void
moo_combo_popdown_real (MooCombo       *combo)
{
    if (!moo_combo_popup_shown (combo))
        return;

    g_signal_handlers_disconnect_by_func (combo->entry,
                                          (gpointer) entry_focus_out,
                                          combo);

    g_signal_handlers_disconnect_by_func (combo->priv->popup,
                                          (gpointer) popup_button_press,
                                          combo);
    g_signal_handlers_disconnect_by_func (combo->priv->popup,
                                          (gpointer) popup_key_press_cb,
                                          combo);

    g_signal_handlers_disconnect_by_func (combo->priv->treeview,
                                          (gpointer) list_button_press,
                                          combo);

    gdk_pointer_ungrab (GDK_CURRENT_TIME);
    gtk_grab_remove (combo->priv->popup);
    gtk_widget_hide (combo->priv->popup);
}


void
moo_combo_update_popup (MooCombo *combo)
{
    g_return_if_fail (MOO_IS_COMBO (combo));

    if (moo_combo_popup_shown (combo))
        resize_popup (combo);
}


typedef struct {
    MooCombo *combo;
    int count;
} CountSeparatorsData;

static gboolean
count_separators (GtkTreeModel *model,
                  G_GNUC_UNUSED GtkTreePath *path,
                  GtkTreeIter *iter,
                  gpointer user_data)
{
    CountSeparatorsData *data = (CountSeparatorsData*) user_data;

    if (data->combo->priv->row_separator_func (model, iter, data->combo->priv->row_separator_data))
        data->count++;

    return FALSE;
}

/* XXX gtk_entry_resize_popup from gtkentrycompletion.c */
static gboolean
resize_popup (MooCombo *combo)
{
    GtkWidget *widget = GTK_WIDGET (combo->entry);
    int x, y;
    int matches, items, height, x_border, y_border;
    GdkScreen *screen;
    int monitor_num;
    GdkRectangle monitor;
    GtkRequisition popup_req;
    GtkRequisition combo_req;
    gboolean above;
    int width;
    int separator_height = 0, vert_separator = 0;
    int selected;

    g_return_val_if_fail (GTK_WIDGET_REALIZED (combo->entry), FALSE);

    gdk_window_get_origin (widget->window, &x, &y);
    /* XXX */
    entry_get_borders (GTK_ENTRY (combo->entry), &x_border, &y_border);

    matches = gtk_tree_model_iter_n_children (combo->priv->model, NULL);

    /* XXX */
    if (combo->priv->row_separator_func)
    {
        CountSeparatorsData data;

        int focus_line_width;

        data.combo = combo;
        data.count = 0;
        gtk_tree_model_foreach (combo->priv->model,
                                (GtkTreeModelForeachFunc) count_separators,
                                &data);

        if (data.count)
        {
            matches -= data.count;
            gtk_widget_style_get (GTK_WIDGET (combo->priv->treeview),
                                  "vertical-separator", &separator_height,
                                  "focus-line-width", &focus_line_width, NULL);
            separator_height *= data.count;
            separator_height += 2 * data.count * focus_line_width;
        }
    }

    items = MIN (matches, MAX_POPUP_LEN);

    gtk_tree_view_column_cell_get_size (combo->priv->column, NULL,
                                        NULL, NULL, NULL, &height);

    screen = gtk_widget_get_screen (widget);
    monitor_num = gdk_screen_get_monitor_at_window (screen, widget->window);
    gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

    width = MIN (GTK_WIDGET(combo)->allocation.width, monitor.width) - 2 * x_border;
    gtk_widget_style_get (GTK_WIDGET (combo->priv->treeview), "vertical-separator",
                          &vert_separator, NULL);
    gtk_widget_set_size_request (GTK_WIDGET (combo->priv->treeview), width,
                                 separator_height + items * (height + vert_separator));

    gtk_widget_set_size_request (combo->priv->popup, -1, -1);
    gtk_widget_size_request (combo->priv->popup, &popup_req);
    gtk_widget_size_request (widget, &combo_req);

    if (x < monitor.x)
        x = monitor.x;
    else if (x + popup_req.width > monitor.x + monitor.width)
        x = monitor.x + monitor.width - popup_req.width;

    if (y + combo_req.height + popup_req.height <= monitor.y + monitor.height)
    {
        y += combo_req.height;
        above = FALSE;
    }
    else
    {
        y -= popup_req.height;
        above = TRUE;
    }

    gtk_window_move (GTK_WINDOW (combo->priv->popup), x, y);

    selected = popup_get_selected (combo);

    if (selected >= 0)
    {
        GtkTreePath *path = gtk_tree_path_new_from_indices (selected, -1);
        gtk_tree_view_scroll_to_cell (combo->priv->treeview, path, NULL, FALSE, 0, 0);
        gtk_tree_path_free (path);
    }

    return above;
}


static gboolean
entry_focus_out (MooCombo *combo)
{
    moo_combo_popdown (combo);
    return FALSE;
}


static gboolean
popup_button_press (MooCombo       *combo,
                    GdkEventButton *event)
{
    if (event->window == combo->priv->popup->window)
    {
        gint width, height;
        gdk_drawable_get_size (GDK_DRAWABLE (event->window),
                               &width, &height);
        if (event->x < 0 || event->x >= width ||
            event->y < 0 || event->y >= height)
        {
            moo_combo_popdown (combo);
        }
    }
    else
    {
        moo_combo_popdown (combo);
        return FALSE;
    }

    return TRUE;
}


static int
popup_get_selected (MooCombo     *combo)
{
    int ind;
    GtkTreeIter iter;
    GtkTreePath *path;
    GtkTreeView *treeview = combo->priv->treeview;
    GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);

    if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
        path = gtk_tree_model_get_path (combo->priv->model, &iter);
        g_return_val_if_fail (path != NULL, -1);
        ind = gtk_tree_path_get_indices (path)[0];
        gtk_tree_path_free (path);
        return ind;
    }
    else
    {
        return -1;
    }
}


static void
combo_set_text_silent (MooCombo   *combo,
                       const char *text)
{
    g_return_if_fail (text != NULL);

    g_signal_handlers_block_by_func (combo->entry, (gpointer) entry_changed, combo);

    moo_entry_begin_undo_group (MOO_ENTRY (combo->entry));
    gtk_entry_set_text (GTK_ENTRY (combo->entry), text);
    gtk_editable_set_position (GTK_EDITABLE (combo->entry), -1);

    g_signal_handlers_unblock_by_func (combo->entry, (gpointer) entry_changed, combo);
}


static void
popup_move_selection (MooCombo     *combo,
                      GdkEventKey  *event)
{
    int n_items, current_item, new_item = -1;
    GtkTreePath *path;

    n_items = gtk_tree_model_iter_n_children (combo->priv->model, NULL);
    g_return_if_fail (n_items != 0);

    current_item = popup_get_selected (combo);

    switch (event->keyval)
    {
        case GDK_Down:
        case GDK_KP_Down:
            if (current_item < n_items - 1)
                new_item = current_item + 1;
            else
                new_item = -1;
            break;

        case GDK_Up:
        case GDK_KP_Up:
            if (current_item < 0)
                new_item = n_items - 1;
            else
                new_item = current_item - 1;
            break;

        case GDK_Page_Down:
        case GDK_KP_Page_Down:
            new_item = current_item + MAX_POPUP_LEN - 1;
            if (new_item >= n_items)
                new_item = n_items - 1;
            break;

        case GDK_Page_Up:
        case GDK_KP_Page_Up:
            new_item = current_item - MAX_POPUP_LEN + 1;
            if (new_item < 0)
                new_item = 0;
            break;

        case GDK_Tab:
        case GDK_KP_Tab:
            if (current_item < n_items - 1)
                new_item = current_item + 1;
            else
                new_item = 0;
            break;

        case GDK_ISO_Left_Tab:
            if (current_item <= 0)
                new_item = n_items - 1;
            else
                new_item = current_item - 1;
            break;

        default:
            g_return_if_reached ();
    }

    if (new_item >= 0 && new_item < n_items)
    {
        GtkTreeIter iter;
        char *new_text;

        path = gtk_tree_path_new_from_indices (new_item, -1);
        gtk_tree_view_set_cursor (combo->priv->treeview, path, NULL, FALSE);
        gtk_tree_view_scroll_to_cell (combo->priv->treeview, path, NULL, FALSE, 0, 0);

        if (!combo->priv->walking_list)
        {
            combo->priv->walking_list = TRUE;
            combo->priv->real_text = g_strdup (gtk_entry_get_text (GTK_ENTRY (combo->entry)));
        }

        gtk_tree_model_get_iter (combo->priv->model, &iter, path);
        new_text = moo_combo_get_text_at_iter (combo, &iter);
        combo_set_text_silent (combo, new_text);

        gtk_tree_path_free (path);
        g_free (new_text);
    }
    else
    {
        if (combo->priv->walking_list)
        {
            combo->priv->walking_list = FALSE;
            combo_set_text_silent (combo, combo->priv->real_text);
            g_free (combo->priv->real_text);
            combo->priv->real_text = NULL;
        }

        gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (combo->priv->treeview));
    }
}


char*
moo_combo_get_text_at_iter (MooCombo       *combo,
                            GtkTreeIter    *iter)
{
    char *text;

    g_return_val_if_fail (MOO_IS_COMBO (combo), NULL);

    text = combo->priv->get_text_func (combo->priv->model, iter,
                                       combo->priv->get_text_data);
    g_return_val_if_fail (text != NULL, g_strdup (""));
    return text;
}


static gboolean
popup_return_key (MooCombo *combo)
{
    GtkTreeIter iter;
    GtkTreeView *treeview = combo->priv->treeview;
    GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);

    if (gtk_tree_selection_get_selected (selection, NULL, &iter))
        moo_combo_set_active_iter (combo, &iter);
    else
        moo_combo_popdown (combo);

    return TRUE;
}


static gboolean
moo_combo_popup_key_press (MooCombo    *combo,
                           GdkEventKey *event)
{
    switch (event->keyval)
    {
        case GDK_Down:
        case GDK_Up:
        case GDK_KP_Down:
        case GDK_KP_Up:
        case GDK_Page_Down:
        case GDK_Page_Up:
        case GDK_Tab:
        case GDK_KP_Tab:
        case GDK_ISO_Left_Tab:
            popup_move_selection (combo, event);
            return TRUE;

        case GDK_Escape:
            moo_combo_popdown (combo);
            return TRUE;

        case GDK_Return:
        case GDK_ISO_Enter:
        case GDK_KP_Enter:
            return popup_return_key (combo);

        default:
            return gtk_widget_event (combo->entry, (GdkEvent*) event) ||
                    gtk_widget_event (GTK_WIDGET (combo), (GdkEvent*) event);
    }
}


static gboolean
popup_key_press_cb (MooCombo    *combo,
                    GdkEventKey *event)
{
    gboolean retval;
    g_signal_emit (combo, signals[POPUP_KEY_PRESS], 0, event, &retval);
    return retval;
}


static gboolean
list_button_press (MooCombo       *combo,
                   GdkEventButton *event)
{
    GtkTreePath *path;
    GtkTreeIter iter;

    if (gtk_tree_view_get_path_at_pos (combo->priv->treeview,
                                       (int) event->x, (int) event->y,
                                       &path, NULL, NULL, NULL))
    {
        gtk_tree_model_get_iter (combo->priv->model, &iter, path);

        if (!combo->priv->row_separator_func ||
             !combo->priv->row_separator_func (combo->priv->model, &iter,
                                               combo->priv->row_separator_data))
        {
            moo_combo_set_active_iter (combo, &iter);
            gtk_tree_path_free (path);
            return TRUE;
        }

        gtk_tree_path_free (path);
    }

    return FALSE;
}


GtkTreeModel*
moo_combo_get_model (MooCombo       *combo)
{
    g_return_val_if_fail (MOO_IS_COMBO (combo), NULL);
    return combo->priv->model;
}


void
moo_combo_set_model (MooCombo       *combo,
                     GtkTreeModel   *model)
{
    g_return_if_fail (MOO_IS_COMBO (combo));
    g_return_if_fail (!model || GTK_IS_TREE_MODEL (model));

    if (model == combo->priv->model)
        return;

    if (combo->priv->model)
    {
        g_object_unref (combo->priv->model);
        gtk_tree_view_set_model (combo->priv->treeview, NULL);
        combo->priv->model = NULL;
    }

    if (model)
    {
        combo->priv->model = model;
        g_object_ref (combo->priv->model);
        gtk_tree_view_set_model (combo->priv->treeview, combo->priv->model);
    }

    moo_combo_update_popup (combo);

    if (combo->priv->active_row)
    {
        gtk_tree_row_reference_free (combo->priv->active_row);
        combo->priv->active_row = NULL;
        moo_combo_changed (combo);
    }
}


void
moo_combo_set_text_column (MooCombo       *combo,
                           int             column)
{
    g_return_if_fail (MOO_IS_COMBO (combo));
    g_return_if_fail (column >= 0);

    combo->priv->text_column = column;

    if (combo->priv->text_cell)
    {
        gtk_tree_view_column_clear_attributes (combo->priv->column, combo->priv->text_cell);
    }
    else
    {
        combo->priv->text_cell = gtk_cell_renderer_text_new ();
        g_object_set (combo->priv->text_cell, "single-paragraph-mode", TRUE, NULL);
        gtk_tree_view_column_pack_start (combo->priv->column, combo->priv->text_cell, TRUE);
    }

    gtk_tree_view_column_set_attributes (combo->priv->column, combo->priv->text_cell,
                                         "text", combo->priv->text_column, NULL);
}


int
moo_combo_get_text_column (MooCombo       *combo)
{
    g_return_val_if_fail (MOO_IS_COMBO (combo), -1);
    return combo->priv->text_column;
}


void
moo_combo_set_get_text_func (MooCombo       *combo,
                             MooComboGetTextFunc func,
                             gpointer        data)
{
    g_return_if_fail (MOO_IS_COMBO (combo));

    if (func)
    {
        combo->priv->get_text_func = func;
        combo->priv->get_text_data = data;
    }
    else
    {
        combo->priv->get_text_func = default_get_text_func;
        combo->priv->get_text_data = combo;
    }
}


static char*
default_get_text_func (GtkTreeModel   *model,
                       GtkTreeIter    *iter,
                       gpointer        data)
{
    char *text = NULL;
    MooCombo *combo = MOO_COMBO (data);
    gtk_tree_model_get (model, iter, combo->priv->text_column, &text, -1);
    return text;
}


GtkWidget*
moo_combo_new_text (void)
{
    GtkWidget *combo;
    GtkListStore *store;

    store = gtk_list_store_new (1, G_TYPE_STRING);
    combo = moo_combo_new ();
    moo_combo_set_model (MOO_COMBO (combo), GTK_TREE_MODEL (store));
    moo_combo_set_text_column (MOO_COMBO (combo), 0);

    g_object_unref (store);
    return combo;
}


void
moo_combo_add_text (MooCombo   *combo,
                    const char *text)
{
    GtkTreeIter iter;

    g_return_if_fail (MOO_IS_COMBO (combo));
    g_return_if_fail (text != NULL);
    g_return_if_fail (GTK_IS_LIST_STORE (combo->priv->model));

    gtk_list_store_append (GTK_LIST_STORE (combo->priv->model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (combo->priv->model), &iter,
                        combo->priv->text_column, text, -1);
}


static void
button_clicked (MooCombo       *combo)
{
    if (moo_combo_popup_shown (combo))
        moo_combo_popdown (combo);
    else
        moo_combo_popup (combo);
}


gboolean
moo_combo_popup_shown (MooCombo       *combo)
{
    g_return_val_if_fail (MOO_IS_COMBO (combo), FALSE);
    return combo->priv->popup && GTK_WIDGET_MAPPED (combo->priv->popup);
}


static void
moo_combo_unmap (GtkWidget      *widget)
{
    moo_combo_popdown (MOO_COMBO (widget));
    destroy_popup_window (MOO_COMBO (widget));
    GTK_WIDGET_CLASS(moo_combo_parent_class)->unmap (widget);
}


static void
moo_combo_unrealize (GtkWidget      *widget)
{
    moo_combo_popdown (MOO_COMBO (widget));
    destroy_popup_window (MOO_COMBO (widget));
    GTK_WIDGET_CLASS(moo_combo_parent_class)->unrealize (widget);
}


void
moo_combo_entry_set_text (MooCombo       *combo,
                          const char     *text)
{
    g_return_if_fail (MOO_IS_COMBO (combo));
    g_return_if_fail (text != NULL);
    gtk_entry_set_text (GTK_ENTRY (combo->entry), text);
}


const char*
moo_combo_entry_get_text (MooCombo       *combo)
{
    g_return_val_if_fail (MOO_IS_COMBO (combo), NULL);
    return gtk_entry_get_text (GTK_ENTRY (combo->entry));
}


void
moo_combo_select_region (MooCombo       *combo,
                         int             start,
                         int             end)
{
    g_return_if_fail (MOO_IS_COMBO (combo));
    gtk_editable_select_region (GTK_EDITABLE (combo->entry), start, end);
}


void
moo_combo_entry_set_activates_default (MooCombo *combo,
                                       gboolean  setting)
{
    g_return_if_fail (MOO_IS_COMBO (combo));
    g_object_set (combo, "activates-default", setting, NULL);
}


static void
moo_combo_cell_layout_pack_start (GtkCellLayout      *cell_layout,
                                  GtkCellRenderer    *cell,
                                  gboolean            expand)
{
    gtk_tree_view_column_pack_start (MOO_COMBO (cell_layout)->priv->column,
                                     cell, expand);
}

static void
moo_combo_cell_layout_pack_end (GtkCellLayout      *cell_layout,
                                GtkCellRenderer    *cell,
                                gboolean            expand)
{
    gtk_tree_view_column_pack_end (MOO_COMBO (cell_layout)->priv->column,
                                   cell, expand);
}

static void
moo_combo_cell_layout_clear (GtkCellLayout      *cell_layout)
{
    gtk_tree_view_column_clear (MOO_COMBO (cell_layout)->priv->column);
}

static void
moo_combo_cell_layout_add_attribute (GtkCellLayout      *cell_layout,
                                     GtkCellRenderer    *cell,
                                     const gchar        *attribute,
                                     gint                column)
{
    gtk_tree_view_column_add_attribute (MOO_COMBO (cell_layout)->priv->column,
                                        cell, attribute, column);
}

static void
moo_combo_cell_layout_set_cell_data_func (GtkCellLayout      *cell_layout,
                                          GtkCellRenderer    *cell,
                                          GtkCellLayoutDataFunc func,
                                          gpointer            func_data,
                                          GDestroyNotify      destroy)
{
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (MOO_COMBO (cell_layout)->priv->column),
                                        cell, func, func_data, destroy);
}

static void
moo_combo_cell_layout_clear_attributes (GtkCellLayout      *cell_layout,
                                        GtkCellRenderer    *cell)
{
    gtk_tree_view_column_clear_attributes (MOO_COMBO (cell_layout)->priv->column, cell);
}

static void
moo_combo_cell_layout_reorder (GtkCellLayout      *cell_layout,
                               GtkCellRenderer    *cell,
                               gint                position)
{
    gtk_cell_layout_reorder (GTK_CELL_LAYOUT (MOO_COMBO (cell_layout)->priv->column),
                             cell, position);
}


void
moo_combo_set_use_button (MooCombo       *combo,
                          gboolean        use)
{
    g_return_if_fail (MOO_IS_COMBO (combo));

    use = use ? TRUE : FALSE;
    if (use == combo->priv->use_button)
        return;

    if (use)
    {
        create_arrow_button (combo);
    }
    else
    {
        g_signal_handlers_disconnect_by_func (combo->priv->button,
                                              (gpointer) button_clicked,
                                              combo);
        gtk_container_remove (GTK_CONTAINER (combo), combo->priv->button);
        combo->priv->button = NULL;
    }
}


void
moo_combo_set_active_iter (MooCombo       *combo,
                           GtkTreeIter    *iter)
{
    GtkTreePath *path;
    char *text;

    g_return_if_fail (MOO_IS_COMBO (combo));
    g_return_if_fail (combo->priv->model != NULL && iter != NULL);

    if (combo->priv->active_row)
        gtk_tree_row_reference_free (combo->priv->active_row);

    path = gtk_tree_model_get_path (combo->priv->model, iter);
    combo->priv->active_row = gtk_tree_row_reference_new (combo->priv->model, path);

    if (!combo->priv->active_row || !gtk_tree_row_reference_valid (combo->priv->active_row))
    {
        gtk_tree_row_reference_free (combo->priv->active_row);
        combo->priv->active_row = NULL;
        g_return_if_reached ();
    }

    text = moo_combo_get_text_at_iter (combo, iter);
    g_return_if_fail (text != NULL);

    g_signal_handlers_block_by_func (combo->entry,
                                     (gpointer) entry_changed, combo);
    moo_entry_begin_undo_group (MOO_ENTRY (combo->entry));

    moo_combo_popdown (combo);
    gtk_entry_set_text (GTK_ENTRY (combo->entry), text);
    gtk_editable_set_position (GTK_EDITABLE (combo->entry), -1);

    moo_entry_end_undo_group (MOO_ENTRY (combo->entry));
    g_signal_handlers_unblock_by_func (combo->entry,
                                       (gpointer) entry_changed, combo);

    moo_combo_changed (combo);

    gtk_tree_path_free (path);
    g_free (text);
}

void
moo_combo_set_active (MooCombo *combo,
                      int       row)
{
    GtkTreeIter iter;

    g_return_if_fail (MOO_IS_COMBO (combo));
    g_return_if_fail (row >= 0);
    g_return_if_fail (combo->priv->model != NULL);

    if (gtk_tree_model_iter_nth_child (combo->priv->model, &iter, NULL, row))
        moo_combo_set_active_iter (combo, &iter);
}


gboolean
moo_combo_get_active_iter (MooCombo       *combo,
                           GtkTreeIter    *iter)
{
    g_return_val_if_fail (MOO_IS_COMBO (combo), FALSE);

    if (!combo->priv->active_row)
        return FALSE;

    if (!gtk_tree_row_reference_valid (combo->priv->active_row))
    {
        gtk_tree_row_reference_free (combo->priv->active_row);
        combo->priv->active_row = NULL;
        return FALSE;
    }

    if (iter)
    {
        GtkTreePath *path;

        path = gtk_tree_row_reference_get_path (combo->priv->active_row);
        g_return_val_if_fail (path != NULL, FALSE);

        gtk_tree_model_get_iter (combo->priv->model, iter, path);
        gtk_tree_path_free (path);
    }

    return TRUE;
}


static void
moo_combo_changed (MooCombo *combo)
{
    if (!combo->priv->walking_list)
        gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (combo->priv->treeview));

    g_signal_emit (combo, signals[CHANGED], 0);
}


static void
entry_changed (MooCombo       *combo)
{
    GtkTreeIter iter;

    if (combo->priv->walking_list)
    {
        combo->priv->walking_list = FALSE;
        g_free (combo->priv->real_text);
        combo->priv->real_text = NULL;
    }

    if (moo_combo_get_active_iter (combo, &iter))
    {
        char *text = moo_combo_get_text_at_iter (combo, &iter);
        const char *entry_text = gtk_entry_get_text (GTK_ENTRY (combo->entry));

        g_return_if_fail (text != NULL);

        if (strcmp (text, entry_text))
        {
            gtk_tree_row_reference_free (combo->priv->active_row);
            combo->priv->active_row = NULL;
            moo_combo_changed (combo);
        }

        g_free (text);
    }
    else
    {
        moo_combo_changed (combo);
    }
}


void
moo_combo_set_row_separator_func (MooCombo       *combo,
                                  MooComboRowSeparatorFunc func,
                                  gpointer        data)
{
    g_return_if_fail (MOO_IS_COMBO (combo));
    gtk_tree_view_set_row_separator_func (combo->priv->treeview, func, data, NULL);
    combo->priv->row_separator_func = func;
    combo->priv->row_separator_data = data;
}


static gboolean
moo_combo_focus (GtkWidget      *widget,
                 GtkDirectionType direction)
{
    MooCombo *combo = MOO_COMBO (widget);
    return gtk_widget_child_focus (combo->entry, direction);
}
