/*
 *   moonotebook.c
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
 * class:MooNotebook: (parent GtkNotebook) (constructable) (moo.private 1)
 **/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "marshals.h"
#include "mooutils/moonotebook.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/moopane.h"
#include "mooutils/moocompat.h"
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string.h>

#if defined(MOO_BROKEN_GTK_THEME)
#define DETAIL_NOTEBOOK NULL
#define DETAIL_TAB      NULL
#else
#define DETAIL_NOTEBOOK ((char*) "notebook")
#define DETAIL_TAB      ((char*) "tab")
#endif

#define MIN_LABELS_WIDTH 10
#define DEFAULT_HBORDER 6
#define DEFAULT_VBORDER 3
#define LABEL_FOCUS_HEIGHT 2
#define LABEL_OVERLAP 2
#define LABEL_ALPHA 128

typedef struct {
    GtkWidget *widget;
    int width;
    int height;
    int offset;
} Label;

typedef struct {
    Label       *label;
    GtkWidget   *child;
    GtkWidget   *focus_child;
} Page;

typedef enum {
    FOCUS_NONE = 0,
    FOCUS_LEFT,
    FOCUS_RIGHT,
    FOCUS_ARROWS,
    FOCUS_LABEL,
    FOCUS_CHILD
} FocusType;

enum {
    LEFT = 0,
    RIGHT = 1
};

#define PopupFunc MooNotebookPopupCreationFunc

struct _MooNotebookPrivate {
    GdkWindow   *tab_window;

    Page        *current_page;
    GSList      *pages;

    gboolean     enable_popup;

    gboolean     enable_reordering;
    gboolean     button_pressed;
    gboolean     in_drag;
    gboolean     want_snapshot;
    GdkPixmap   *snapshot_pixmap;
    GdkPixbuf   *snapshot_pixbuf;
    Page        *drag_page;
    int          drag_page_index;
    int          drag_tab_x_delta;
    int          drag_tab_x;
    int          drag_mouse_x;
    guint        drag_scroll_id;
    gboolean     drag_scroll_right;

    FocusType    focus;
    Page        *focus_page;

    gboolean     show_tabs;
    gboolean     tabs_visible;
    int          tabs_height;

    gboolean     label_move_onscreen;
    int          labels_offset;
    int          labels_width;          /* total labels width */
    int          labels_visible_width; /* labels window width */
    guint        label_vborder;
    guint        label_hborder;
    gboolean     labels_homogeneous;

    GtkWidget   *action_widgets[2];
    int          action_widgets_size[2];

    gboolean     arrows_gtk;
    gboolean     arrows_visible;
    int          arrows_size;
    GtkWidget   *arrows;
    GtkWidget   *left_arrow;
    GtkWidget   *right_arrow;

    int          child_height;
};


#if 0
/* XXX try to decide what these invariants are, and do check them */
#define DO_CHECK_INVARIANTS
static void NOTEBOOK_CHECK_INVARIANTS (MooNotebook *nb);
#else
#define NOTEBOOK_CHECK_INVARIANTS(nb) ((void) 0)
#endif


/*
 *   VISIBLE_FOREACH_START (notebook, page) {
 *      do_something_with_pages (notebook, page);
 *   } VISIBLE_FOREACH_END
 */
#define VISIBLE_FOREACH_START(nb,page)                          \
G_STMT_START {                                                  \
    GSList *l__;                                                \
    for (l__ = nb->priv->pages; l__ != NULL; l__ = l__->next)   \
    {                                                           \
        Page *page = l__->data;                                 \
        if (GTK_WIDGET_VISIBLE (page->child))                   \

#define VISIBLE_FOREACH_END                                     \
    }                                                           \
} G_STMT_END


static void     moo_notebook_class_init     (MooNotebookClass *klass);
static void     moo_notebook_init           (MooNotebook    *nb);
static void     moo_notebook_finalize       (GObject        *object);
static void     moo_notebook_destroy        (GtkObject      *object);
static void     moo_notebook_set_property   (GObject        *object,
                                             guint           prop_id,
                                             const GValue   *value,
                                             GParamSpec     *pspec);
static void     moo_notebook_get_property   (GObject        *object,
                                             guint           prop_id,
                                             GValue         *value,
                                             GParamSpec     *pspec);

static void     moo_notebook_style_set      (GtkWidget      *widget,
                                             GtkStyle       *prev_style);
static void     moo_notebook_realize        (GtkWidget      *widget);
static void     moo_notebook_unrealize      (GtkWidget      *widget);
static void     moo_notebook_map            (GtkWidget      *widget);
static void     moo_notebook_unmap          (GtkWidget      *widget);

static void     moo_notebook_size_request   (GtkWidget      *widget,
                                             GtkRequisition *requisition);
static void     moo_notebook_size_allocate  (GtkWidget      *widget,
                                             GtkAllocation  *allocation);

#if 0
static void     moo_notebook_parent_set     (GtkWidget      *widget,
                                             GtkWidget      *previous_parent);
#endif
static gboolean moo_notebook_focus          (GtkWidget      *widget,
                                             GtkDirectionType direction);
static gboolean moo_notebook_focus_in       (GtkWidget      *widget,
                                             GdkEventFocus  *event);
static gboolean moo_notebook_focus_out      (GtkWidget      *widget,
                                             GdkEventFocus  *event);
static gboolean moo_notebook_expose         (GtkWidget      *widget,
                                             GdkEventExpose *event);
static void     moo_notebook_draw_labels    (MooNotebook    *nb,
                                             GdkEventExpose *event);
static void     moo_notebook_draw_label     (MooNotebook    *nb,
                                             Page           *page,
                                             GdkEventExpose *event);
static void     moo_notebook_draw_dragged_label (MooNotebook    *nb,
                                             GdkEventExpose *event);
static void     moo_notebook_draw_child_border (MooNotebook *nb,
                                             GdkEventExpose *event);

static gboolean moo_notebook_button_press   (GtkWidget      *widget,
                                             GdkEventButton *event);
static gboolean moo_notebook_button_release (GtkWidget      *widget,
                                             GdkEventButton *event);
static gboolean moo_notebook_key_press      (GtkWidget      *widget,
                                             GdkEventKey    *event);
static gboolean moo_notebook_motion         (GtkWidget      *widget,
                                             GdkEventMotion *event);
#if 0
static gboolean moo_notebook_enter          (GtkWidget      *widget,
                                             GdkEventCrossing *event);
static gboolean moo_notebook_leave          (GtkWidget      *widget,
                                             GdkEventCrossing *event);
#endif
static gboolean moo_notebook_scroll_event   (GtkWidget      *widget,
                                             GdkEventScroll *event);

static void     moo_notebook_forall         (GtkContainer   *container,
                                             gboolean        include_internals,
                                             GtkCallback     callback,
                                             gpointer        callback_data);
static void     moo_notebook_add            (GtkContainer   *container,
                                             GtkWidget      *widget);
static void     moo_notebook_remove         (GtkContainer   *container,
                                             GtkWidget      *widget);
static void     moo_notebook_set_focus_child(GtkContainer  *container,
                                             GtkWidget      *widget);

static void     moo_notebook_switch_page    (MooNotebook     *nb,
                                             guint            page_num);

static void     notebook_create_arrows      (MooNotebook    *nb);
static void     left_arrow_clicked          (GtkWidget      *button,
                                             MooNotebook    *nb);
static void     right_arrow_clicked         (GtkWidget      *button,
                                             MooNotebook    *nb);

static void     child_visible_notify        (GtkWidget      *child,
                                             GParamSpec     *arg,
                                             MooNotebook    *nb);
static void     moo_notebook_check_action_widgets (MooNotebook *nb);
static void     moo_notebook_check_tabs     (MooNotebook    *nb);

static int      page_index                  (MooNotebook    *nb,
                                             Page           *page);
static int      page_visible_index          (MooNotebook    *nb,
                                             Page           *page);
static int      get_n_visible_pages         (MooNotebook    *nb);
static Page    *get_nth_visible_page        (MooNotebook    *nb,
                                             int             n);
static int      find_next_visible_page      (MooNotebook    *nb,
                                             int             n);
static Page    *get_nth_page                (MooNotebook    *nb,
                                             int             n);
static Page    *find_child                  (MooNotebook    *nb,
                                             GtkWidget      *child);
static Page    *find_grand_child            (MooNotebook    *nb,
                                             GtkWidget      *child);
static Page    *find_label                  (MooNotebook    *nb,
                                             GtkWidget      *label);
static void     delete_page                 (MooNotebook    *nb,
                                             Page           *page);
static GSList  *get_visible_pages           (MooNotebook    *nb);
static void     moo_notebook_check_arrows   (MooNotebook    *nb);

static void     moo_notebook_set_homogeneous(MooNotebook    *nb,
                                             gboolean        homogeneous);
static void     labels_scroll               (MooNotebook    *nb,
                                             GtkDirectionType where);
static void     labels_size_request         (MooNotebook    *nb,
                                             GtkRequisition *requisition);
static int      labels_get_height_request   (MooNotebook    *nb);
static void     labels_size_allocate        (MooNotebook    *nb,
                                             GtkAllocation  *allocation);
static void     labels_scroll_to_offset     (MooNotebook    *nb,
                                             int             offset,
                                             gboolean        realloc);
static Page    *find_label_at_x             (MooNotebook    *nb,
                                             int             x,
                                             gboolean        widget_only);
static Page    *find_label_at_xy            (MooNotebook    *nb,
                                             int             x,
                                             int             y,
                                             gboolean        widget_only);
static void     labels_move_label_onscreen  (MooNotebook    *nb,
                                             Page           *page,
                                             gboolean        realloc);
static void     labels_invalidate           (MooNotebook    *nb);

static gboolean moo_notebook_maybe_popup    (MooNotebook    *notebook,
                                             GdkEventButton *event);

/* MOO_TYPE_NOTEBOOK */
G_DEFINE_TYPE (MooNotebook, moo_notebook, GTK_TYPE_NOTEBOOK)

enum {
    PROP_0,
    PROP_SHOW_TABS,
    PROP_TAB_BORDER,
    PROP_TAB_HBORDER,
    PROP_TAB_VBORDER,
    PROP_PAGE,
    PROP_ENABLE_POPUP,
    PROP_ENABLE_REORDERING,
    PROP_HOMOGENEOUS
};

enum {
    SWITCH_PAGE,
    POPULATE_POPUP,
    NUM_SIGNALS
};

static guint signals[NUM_SIGNALS];
static gpointer moo_notebook_grand_parent_class;

static void moo_notebook_class_init (MooNotebookClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

    moo_notebook_parent_class = g_type_class_peek_parent (klass);
    moo_notebook_grand_parent_class = g_type_class_peek_parent (moo_notebook_parent_class);

    gobject_class->finalize = moo_notebook_finalize;
    gobject_class->set_property = moo_notebook_set_property;
    gobject_class->get_property = moo_notebook_get_property;

    gtkobject_class->destroy = moo_notebook_destroy;

#if 0
    widget_class->parent_set = moo_notebook_parent_set;
#endif
    widget_class->style_set = moo_notebook_style_set;
    widget_class->realize = moo_notebook_realize;
    widget_class->unrealize = moo_notebook_unrealize;
    widget_class->map = moo_notebook_map;
    widget_class->unmap = moo_notebook_unmap;
    widget_class->focus_in_event = moo_notebook_focus_in;
    widget_class->focus_out_event = moo_notebook_focus_out;
    widget_class->expose_event = moo_notebook_expose;
    widget_class->size_request = moo_notebook_size_request;
    widget_class->size_allocate = moo_notebook_size_allocate;
    widget_class->button_press_event = moo_notebook_button_press;
    widget_class->button_release_event = moo_notebook_button_release;
    widget_class->scroll_event = moo_notebook_scroll_event;
    widget_class->key_press_event = moo_notebook_key_press;
    widget_class->focus = moo_notebook_focus;
    widget_class->motion_notify_event = moo_notebook_motion;
#if 0
    widget_class->enter_notify_event = moo_notebook_enter;
    widget_class->leave_notify_event = moo_notebook_leave;
#endif

    widget_class->drag_begin = NULL;
    widget_class->drag_end = NULL;
    widget_class->drag_motion = NULL;
    widget_class->drag_leave = NULL;
    widget_class->drag_drop = NULL;
    widget_class->drag_data_get = NULL;
    widget_class->drag_data_received = NULL;

    container_class->forall = moo_notebook_forall;
    container_class->set_focus_child = moo_notebook_set_focus_child;
    container_class->remove = moo_notebook_remove;
    container_class->add = moo_notebook_add;

    klass->switch_page = moo_notebook_switch_page;

    g_object_class_install_property (gobject_class,
                                     PROP_PAGE,
                                     g_param_spec_int ("page",
                                             "Page",
                                             "The index of the current page",
                                             0, G_MAXINT, 0,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_TAB_BORDER,
                                     g_param_spec_uint ("tab-border",
                                             "Tab Border",
                                             "Width of the border around the tab labels",
                                             0, G_MAXUINT, 2,
                                             G_PARAM_WRITABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_TAB_HBORDER,
                                     g_param_spec_uint ("tab-hborder",
                                             "Horizontal Tab Border",
                                             "Width of the horizontal border of tab labels",
                                             0, G_MAXUINT, 2,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_TAB_VBORDER,
                                     g_param_spec_uint ("tab-vborder",
                                             "Vertical Tab Border",
                                             "Width of the vertical border of tab labels",
                                             0, G_MAXUINT, 2,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_SHOW_TABS,
                                     g_param_spec_boolean ("show-tabs",
                                             "Show Tabs",
                                             "Whether tabs should be shown or not",
                                             TRUE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_ENABLE_POPUP,
                                     g_param_spec_boolean ("enable-popup",
                                             "Enable Popup",
                                             "If TRUE, pressing the right mouse button on the notebook pops up a menu",
                                             FALSE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_HOMOGENEOUS,
                                     g_param_spec_boolean ("homogeneous",
                                             "Homogeneous",
                                             "Whether tabs should have homogeneous sizes",
                                             FALSE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_ENABLE_REORDERING,
                                     g_param_spec_boolean ("enable-reordering",
                                             "enable-reordering",
                                             "enable-reordering",
                                             TRUE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    signals[SWITCH_PAGE] =
            g_signal_new ("moo-switch-page",
                          G_TYPE_FROM_CLASS (gobject_class),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooNotebookClass, switch_page),
                          NULL, NULL,
                          _moo_marshal_VOID__UINT,
                          G_TYPE_NONE, 1,
                          G_TYPE_UINT);

    signals[POPULATE_POPUP] =
            g_signal_new ("populate-popup",
                          G_TYPE_FROM_CLASS (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooNotebookClass, populate_popup),
                          g_signal_accumulator_true_handled, NULL,
                          _moo_marshal_BOOL__OBJECT_OBJECT,
                          G_TYPE_BOOLEAN, 2,
                          GTK_TYPE_WIDGET,
                          GTK_TYPE_MENU);
}


static void
moo_notebook_init (MooNotebook *notebook)
{
    GTK_WIDGET_SET_CAN_FOCUS (notebook);
    GTK_WIDGET_SET_NO_WINDOW (notebook);

    notebook->priv = g_new0 (MooNotebookPrivate, 1);

    notebook->priv->enable_popup = FALSE;

    notebook->priv->enable_reordering = TRUE;
    notebook->priv->button_pressed = FALSE;

    notebook->priv->focus = FOCUS_NONE;
    notebook->priv->focus_page = NULL;

    notebook->priv->show_tabs = TRUE;
    notebook->priv->tabs_visible = FALSE;
    notebook->priv->tabs_height = 0;

    notebook->priv->label_hborder = DEFAULT_HBORDER;
    notebook->priv->label_vborder = DEFAULT_VBORDER;
    notebook->priv->labels_homogeneous = FALSE;

    notebook->priv->arrows_gtk = FALSE;
    notebook->priv->arrows_visible = FALSE;
    notebook->priv->arrows_size = 0;

    notebook->priv->child_height = -1;

    notebook_create_arrows (notebook);
}


static void
notebook_create_arrows (MooNotebook *nb)
{
    GtkWidget *box, *button, *arrow;

    box = gtk_hbox_new (FALSE, 0);

    button = gtk_button_new ();
    gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
    nb->priv->left_arrow = button;
    arrow = _moo_create_arrow_icon (GTK_ARROW_LEFT);
    gtk_container_add (GTK_CONTAINER (button), arrow);
    gtk_widget_show (button);
    gtk_widget_show (arrow);
    gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);

    button = gtk_button_new ();
    gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
    nb->priv->right_arrow = button;
    arrow = _moo_create_arrow_icon (GTK_ARROW_RIGHT);
    gtk_container_add (GTK_CONTAINER (button), arrow);
    gtk_widget_show (button);
    gtk_widget_show (arrow);
    gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);

    nb->priv->arrows = box;
    g_object_ref_sink (box);
    gtk_widget_set_parent (box, GTK_WIDGET (nb));

    g_signal_connect (nb->priv->left_arrow, "clicked",
                      G_CALLBACK (left_arrow_clicked), nb);
    g_signal_connect (nb->priv->right_arrow, "clicked",
                      G_CALLBACK (right_arrow_clicked), nb);

    gtk_widget_set_sensitive (nb->priv->right_arrow, FALSE);
    gtk_widget_set_sensitive (nb->priv->left_arrow, FALSE);
}


static void
moo_notebook_destroy (GtkObject *object)
{
    GSList *l;
    MooNotebook *nb = MOO_NOTEBOOK (object);

    for (l = nb->priv->pages; l != NULL; l = l->next)
    {
        Page *page = l->data;

        g_signal_handlers_disconnect_by_func (page->child,
                                              (gpointer) child_visible_notify,
                                              nb);
        gtk_widget_unparent (page->child);
        gtk_widget_destroy (page->child);
        g_object_unref (page->child);

        gtk_widget_unparent (page->label->widget);
        gtk_widget_destroy (page->label->widget);
        g_object_unref (page->label->widget);

        g_free (page->label);
        g_free (page);
    }

    g_slist_free (nb->priv->pages);
    nb->priv->pages = NULL;
    nb->priv->current_page = NULL;

    GTK_OBJECT_CLASS(moo_notebook_parent_class)->destroy (object);
}


static void
moo_notebook_set_property (GObject        *object,
                           guint           prop_id,
                           const GValue   *value,
                           GParamSpec     *pspec)
{
    MooNotebook *nb = MOO_NOTEBOOK (object);

    switch (prop_id)
    {
        case PROP_TAB_BORDER:
            nb->priv->label_vborder = g_value_get_uint (value);
            nb->priv->label_hborder = g_value_get_uint (value);
            if (GTK_WIDGET_VISIBLE (nb) && nb->priv->tabs_visible)
                gtk_widget_queue_resize (GTK_WIDGET (nb));
            g_object_freeze_notify (object);
            g_object_notify (object, "tab-hborder");
            g_object_notify (object, "tab-vborder");
            g_object_thaw_notify (object);
            break;

        case PROP_TAB_HBORDER:
            nb->priv->label_hborder = g_value_get_uint (value);
            if (GTK_WIDGET_VISIBLE (nb) && nb->priv->tabs_visible)
                gtk_widget_queue_resize (GTK_WIDGET (nb));
            g_object_notify (object, "tab-hborder");
            break;

        case PROP_TAB_VBORDER:
            nb->priv->label_vborder = g_value_get_uint (value);
            if (GTK_WIDGET_VISIBLE (nb) && nb->priv->tabs_visible)
                gtk_widget_queue_resize (GTK_WIDGET (nb));
            g_object_notify (object, "tab-vborder");
            break;

        case PROP_PAGE:
            moo_notebook_set_current_page (nb, g_value_get_int (value));
            break;

        case PROP_ENABLE_POPUP:
            moo_notebook_enable_popup (nb, g_value_get_boolean (value));
            break;

        case PROP_HOMOGENEOUS:
            moo_notebook_set_homogeneous (nb, g_value_get_boolean (value));
            break;

        case PROP_ENABLE_REORDERING:
            moo_notebook_enable_reordering (nb, g_value_get_boolean (value));
            break;

        case PROP_SHOW_TABS:
            moo_notebook_set_show_tabs (nb, g_value_get_boolean (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
moo_notebook_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    MooNotebook *nb = MOO_NOTEBOOK (object);

    switch (prop_id)
    {
        case PROP_TAB_HBORDER:
            g_value_set_uint (value, nb->priv->label_hborder);
            break;
        case PROP_TAB_VBORDER:
            g_value_set_uint (value, nb->priv->label_vborder);
            break;
        case PROP_PAGE:
            g_value_set_int (value, moo_notebook_get_current_page (nb));
            break;
        case PROP_ENABLE_POPUP:
            g_value_set_boolean (value, nb->priv->enable_popup);
            break;
        case PROP_HOMOGENEOUS:
            g_value_set_boolean (value, nb->priv->labels_homogeneous);
            break;
        case PROP_ENABLE_REORDERING:
            g_value_set_boolean (value, nb->priv->enable_reordering);
            break;
        case PROP_SHOW_TABS:
            g_value_set_boolean (value, nb->priv->show_tabs);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
moo_notebook_finalize (GObject *object)
{
    MooNotebook *notebook = MOO_NOTEBOOK (object);

    g_object_unref (notebook->priv->arrows);

    /* XXX */

    g_free (notebook->priv);
    G_OBJECT_CLASS (moo_notebook_parent_class)->finalize (object);
}


GtkWidget *
moo_notebook_new (void)
{
    return g_object_new (MOO_TYPE_NOTEBOOK, (const char*) NULL);
}


static void
labels_size_request (MooNotebook    *nb,
                     GtkRequisition *requisition)
{
    GtkRequisition child_req;

    g_assert (nb->priv->tabs_visible);

    requisition->width = 0;
    requisition->height = 0;

    VISIBLE_FOREACH_START (nb, page)
    {
        gtk_widget_size_request (page->label->widget, &child_req);
        requisition->height = MAX (requisition->height, child_req.height);
    }
    VISIBLE_FOREACH_END;

    requisition->width = MIN_LABELS_WIDTH;
    requisition->height += 2 * nb->priv->label_vborder + LABEL_FOCUS_HEIGHT;
}


#define MAKE_POSITIVE(a,b)  \
G_STMT_START {              \
    a = MAX (a, 0);         \
    b = MAX (b, 0);         \
} G_STMT_END

static int
get_border_width (MooNotebook *nb)
{
    return nb->priv->tabs_visible ?
            gtk_container_get_border_width (GTK_CONTAINER (nb)) : 0;
}

static void
moo_notebook_size_request (GtkWidget      *widget,
                           GtkRequisition *requisition)
{
    GtkRequisition child_req;
    MooNotebook *nb = MOO_NOTEBOOK (widget);
    int border_width = get_border_width (nb);
    int xthickness = widget->style->xthickness;
    int ythickness = widget->style->ythickness;

    NOTEBOOK_CHECK_INVARIANTS (nb);

    requisition->width = 2 * border_width;
    requisition->height = 2 * border_width;

    if (nb->priv->current_page)
    {
        gtk_widget_size_request (nb->priv->current_page->child, requisition);
        requisition->width = MAX (0, requisition->width);
        requisition->height = MAX (0, requisition->height);
        requisition->width += 2*border_width + 2*xthickness;
        requisition->height += 2*border_width + 2*ythickness;
    }

    if (nb->priv->tabs_visible)
    {
        GtkRequisition action_req;

        labels_size_request (nb, &child_req);

        if (nb->priv->action_widgets[LEFT] &&
            GTK_WIDGET_VISIBLE (nb->priv->action_widgets[LEFT]))
        {
            gtk_widget_size_request (nb->priv->action_widgets[LEFT],
                                     &action_req);
            MAKE_POSITIVE (action_req.width, action_req.height);
            child_req.width += action_req.width;
            child_req.height = MAX (child_req.height, action_req.height);
        }

        if (nb->priv->action_widgets[RIGHT] &&
            GTK_WIDGET_VISIBLE (nb->priv->action_widgets[RIGHT]))
        {
            gtk_widget_size_request (nb->priv->action_widgets[RIGHT],
                                     &action_req);
            MAKE_POSITIVE (action_req.width, action_req.height);
            child_req.width += action_req.width;
            child_req.height = MAX (child_req.height, action_req.height);
        }

        gtk_widget_size_request (nb->priv->arrows, &action_req);
        MAKE_POSITIVE (action_req.width, action_req.height);
        child_req.height = MAX (child_req.height, action_req.height);
        child_req.width += MAX (action_req.width, 2*child_req.height);

        if (child_req.width > 0)
            requisition->width = MAX (requisition->width, child_req.width);
        if (child_req.height > 0)
            requisition->height += child_req.height;
    }
}


static int
labels_get_height_request (MooNotebook *nb)
{
    int height = 0;
    GtkRequisition child_req;
    gboolean has_visible = FALSE;

    g_assert (nb->priv->tabs_visible);

    VISIBLE_FOREACH_START (nb, page)
    {
        gtk_widget_get_child_requisition (page->label->widget,
                                          &child_req);
        height = MAX (height, child_req.height);
        has_visible = TRUE;
    }
    VISIBLE_FOREACH_END;

    if (!has_visible)
        return 0;
    else
        return height + 2 * nb->priv->label_vborder + LABEL_FOCUS_HEIGHT;
}


static int
get_tab_window_width (MooNotebook *nb)
{
    GtkWidget *widget = GTK_WIDGET (nb);
    return widget->allocation.width - nb->priv->action_widgets_size[LEFT] -
            nb->priv->arrows_size - nb->priv->action_widgets_size[RIGHT] -
            2 * get_border_width (nb);
}


static void
moo_notebook_size_allocate (GtkWidget     *widget,
                            GtkAllocation *allocation)
{
    GtkAllocation child_allocation, tabs_allocation;
    MooNotebook *nb = MOO_NOTEBOOK (widget);
    int xthickness = widget->style->xthickness;
    int ythickness = widget->style->ythickness;
    int border_width = get_border_width (nb);

    NOTEBOOK_CHECK_INVARIANTS (nb);

    widget->allocation = *allocation;

    if (nb->priv->tabs_visible)
    {
        int height;
    	GtkRequisition left_req = {0};
    	GtkRequisition right_req = {0};
    	GtkRequisition arrows_req = {0};

        /* only reports height needed for tab labels */
        height = labels_get_height_request (nb);

        if (nb->priv->action_widgets[LEFT] &&
            GTK_WIDGET_VISIBLE (nb->priv->action_widgets[LEFT]))
        {
            gtk_widget_get_child_requisition (nb->priv->action_widgets[LEFT],
                                              &left_req);
            MAKE_POSITIVE (left_req.width, left_req.height);
            height = MAX (left_req.height, height);
        }

        if (nb->priv->action_widgets[RIGHT] &&
            GTK_WIDGET_VISIBLE (nb->priv->action_widgets[RIGHT]))
        {
            gtk_widget_get_child_requisition (nb->priv->action_widgets[RIGHT],
                                              &right_req);
            MAKE_POSITIVE (right_req.width, right_req.height);
            height = MAX (right_req.height, height);
        }

        if (nb->priv->arrows_visible)
        {
            gtk_widget_get_child_requisition (nb->priv->arrows,
                                              &arrows_req);
            MAKE_POSITIVE (arrows_req.width, arrows_req.height);
            height = MAX (arrows_req.height, height);
            arrows_req.width = MAX (arrows_req.width, 2*height);
        }

        nb->priv->tabs_height = CLAMP (height, 0, allocation->height - 2*border_width);

        if (nb->priv->action_widgets[LEFT] &&
            GTK_WIDGET_VISIBLE (nb->priv->action_widgets[LEFT]))
        {
            nb->priv->action_widgets_size[LEFT] =
                    CLAMP (left_req.width, 0, allocation->width - 2*border_width);
        }

        if (nb->priv->action_widgets[RIGHT] &&
            GTK_WIDGET_VISIBLE (nb->priv->action_widgets[RIGHT]))
        {
            nb->priv->action_widgets_size[RIGHT] =
                    CLAMP (right_req.width, 0,
                           allocation->width - nb->priv->action_widgets_size[LEFT] - 2*border_width);
        }

        if (nb->priv->arrows_visible)
        {
            nb->priv->arrows_size =
                    CLAMP (arrows_req.width, 0,
                           allocation->width - nb->priv->action_widgets_size[LEFT]
                                   - nb->priv->action_widgets_size[RIGHT] - 2*border_width);
        }
    }

    nb->priv->child_height = allocation->height - nb->priv->tabs_height - 2*border_width;

    tabs_allocation.x = 0;
    tabs_allocation.y = 0;
    tabs_allocation.width = get_tab_window_width (nb);
    tabs_allocation.height = nb->priv->tabs_height;

    if (GTK_WIDGET_REALIZED (widget) && nb->priv->tabs_visible)
        gdk_window_move_resize (nb->priv->tab_window,
                                allocation->x + border_width + nb->priv->action_widgets_size[LEFT],
                                allocation->y + border_width,
                                tabs_allocation.width,
                                tabs_allocation.height);

    if (nb->priv->tabs_visible)
    {
        if (nb->priv->action_widgets[LEFT] &&
            GTK_WIDGET_VISIBLE (nb->priv->action_widgets[LEFT]))
        {
            child_allocation.x = allocation->x + border_width;
            child_allocation.y = allocation->y + border_width;
            child_allocation.width = nb->priv->action_widgets_size[LEFT];
            child_allocation.height = nb->priv->tabs_height;
            gtk_widget_size_allocate (nb->priv->action_widgets[LEFT],
                                      &child_allocation);
        }

        if (nb->priv->action_widgets[RIGHT] &&
            GTK_WIDGET_VISIBLE (nb->priv->action_widgets[RIGHT]))
        {
            child_allocation.x = allocation->x + allocation->width -
                                 nb->priv->action_widgets_size[RIGHT] -
                                 border_width;
            child_allocation.y = allocation->y + border_width;
            child_allocation.width = nb->priv->action_widgets_size[RIGHT];
            child_allocation.height = nb->priv->tabs_height;
            gtk_widget_size_allocate (nb->priv->action_widgets[RIGHT],
                                      &child_allocation);
        }

        if (nb->priv->arrows_visible)
        {
            child_allocation.x = allocation->x + allocation->width -
                                 nb->priv->action_widgets_size[RIGHT] -
                                 nb->priv->arrows_size - border_width;
            child_allocation.y = allocation->y + border_width;
            child_allocation.width = nb->priv->arrows_size;
            child_allocation.height = nb->priv->tabs_height;
            gtk_widget_size_allocate (nb->priv->arrows, &child_allocation);
        }

        labels_size_allocate (nb, &tabs_allocation);
    }

    if (nb->priv->current_page)
    {
        child_allocation.x = allocation->x + border_width + xthickness;
        child_allocation.y = allocation->y + nb->priv->tabs_height + border_width + ythickness;
        child_allocation.width = MAX (0, allocation->width - 2*border_width - 2*xthickness);
        child_allocation.height = MAX (0, nb->priv->child_height - 2*ythickness);
        gtk_widget_size_allocate (nb->priv->current_page->child,
                                  &child_allocation);
    }
}


#if 0
static void
mangle_class_name (char *string)
{
    g_return_if_fail (string != NULL);

    while ((string = strstr (string, "MooNotebook")))
    {
        string[0] = 'G';
        string[1] = 't';
        string[2] = 'k';
        string += strlen ("MooNotebook");
    }
}


static void
update_notebook_style (GtkWidget *widget)
{
    GtkStyle *style;
    GtkSettings *settings;
    char *path, *class_path;

    if (!gtk_widget_has_screen (widget))
        return;

    settings = gtk_widget_get_settings (widget);

    gtk_widget_path (widget, NULL, &path, NULL);
    mangle_class_name (path);
    gtk_widget_class_path (widget, NULL, &class_path, NULL);
    mangle_class_name (class_path);

    style = gtk_rc_get_style_by_paths (settings, path, class_path,
                                       GTK_TYPE_NOTEBOOK);
    gtk_widget_set_style (widget, style);

    g_free (path);
    g_free (class_path);
}


static void
moo_notebook_parent_set (GtkWidget *widget,
                         G_GNUC_UNUSED GtkWidget *previous_parent)
{
    update_notebook_style (widget);
}
#endif


static void
moo_notebook_realize (GtkWidget *widget)
{
    MooNotebook *nb = MOO_NOTEBOOK (widget);
    static GdkWindowAttr attributes;
    gint attributes_mask;
    GSList *l;
    int border_width = get_border_width (nb);

    GTK_WIDGET_SET_REALIZED (widget);

    widget->window = gtk_widget_get_parent_window (widget);
    g_object_ref (widget->window);

    widget->style = gtk_style_attach (widget->style, widget->window);

    /* Tabs window */
    attributes.x = widget->allocation.x + border_width + nb->priv->action_widgets_size[LEFT];
    attributes.y = widget->allocation.y + border_width;
    attributes.width = widget->allocation.width - nb->priv->action_widgets_size[LEFT] -
                       nb->priv->arrows_size - nb->priv->action_widgets_size[RIGHT] -
                       2*border_width;
    attributes.height = nb->priv->tabs_height;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events (widget) |
            GDK_EXPOSURE_MASK |
            GDK_POINTER_MOTION_MASK |
            GDK_BUTTON_PRESS_MASK |
            GDK_BUTTON_RELEASE_MASK |
            GDK_KEY_PRESS_MASK |
            GDK_KEY_RELEASE_MASK |
            GDK_ENTER_NOTIFY_MASK |
            GDK_LEAVE_NOTIFY_MASK |
            GDK_SCROLL_MASK;

    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);
    attributes.wclass = GDK_INPUT_OUTPUT;

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    nb->priv->tab_window = gdk_window_new (widget->window, &attributes, attributes_mask);
    gdk_window_set_user_data (nb->priv->tab_window, widget);

#if 0
    update_notebook_style (widget);
#endif
    gtk_style_set_background (widget->style, nb->priv->tab_window, GTK_STATE_NORMAL);

    for (l = nb->priv->pages; l != NULL; l = l->next)
    {
        Page *page = l->data;
        gtk_widget_set_parent_window (page->label->widget, nb->priv->tab_window);
    }
}


static void
moo_notebook_style_set (GtkWidget *widget,
                        GtkStyle  *prev_style)
{
    MooNotebook *nb = MOO_NOTEBOOK (widget);

    if (nb->priv->tab_window)
        gtk_style_set_background (widget->style,
                                  nb->priv->tab_window,
                                  GTK_STATE_NORMAL);

    if (GTK_WIDGET_CLASS(moo_notebook_grand_parent_class)->style_set)
        GTK_WIDGET_CLASS(moo_notebook_grand_parent_class)->style_set (widget, prev_style);
}


static void
moo_notebook_unrealize (GtkWidget *widget)
{
    MooNotebook *nb = MOO_NOTEBOOK (widget);

    gdk_window_set_user_data (nb->priv->tab_window, NULL);
    gdk_window_destroy (nb->priv->tab_window);
    nb->priv->tab_window = NULL;

    /* we don't call GtkNotebook's realize, so don't call unrealize() either */
    GTK_WIDGET_CLASS(moo_notebook_grand_parent_class)->unrealize (widget);
}


static void
moo_notebook_map (GtkWidget *widget)
{
    MooNotebook *nb = MOO_NOTEBOOK (widget);
    GtkWidget *left = nb->priv->action_widgets[LEFT];
    GtkWidget *right = nb->priv->action_widgets[RIGHT];

    g_return_if_fail (GTK_WIDGET_REALIZED (widget) == TRUE);

    if (GTK_WIDGET_MAPPED (widget))
        return;

    GTK_WIDGET_SET_MAPPED (widget);

    if (nb->priv->tabs_visible)
    {
        gdk_window_show (nb->priv->tab_window);

        if (left && GTK_WIDGET_VISIBLE (left))
            gtk_widget_map (left);

        if (right && GTK_WIDGET_VISIBLE (right))
            gtk_widget_map (right);

        if (nb->priv->arrows_visible)
            gtk_widget_map (nb->priv->arrows);

        VISIBLE_FOREACH_START (nb, page)
        {
            gtk_widget_map (page->label->widget);
        }
        VISIBLE_FOREACH_END;
    }

    if (nb->priv->current_page)
        gtk_widget_map (nb->priv->current_page->child);
}


static void
moo_notebook_unmap (GtkWidget *widget)
{
    MooNotebook *nb = MOO_NOTEBOOK (widget);
    GtkWidget *left = nb->priv->action_widgets[LEFT];
    GtkWidget *right = nb->priv->action_widgets[RIGHT];

    if (!GTK_WIDGET_MAPPED (widget))
        return;

    if (nb->priv->current_page)
        gtk_widget_unmap (nb->priv->current_page->child);

    VISIBLE_FOREACH_START (nb, page)
    {
        gtk_widget_map (page->label->widget);
    }
    VISIBLE_FOREACH_END;

    if (nb->priv->arrows_visible)
        gtk_widget_unmap (nb->priv->arrows);

    if (right && GTK_WIDGET_MAPPED (right))
        gtk_widget_unmap (right);

    if (left && GTK_WIDGET_MAPPED (left))
        gtk_widget_unmap (left);

    gdk_window_hide (nb->priv->tab_window);

    GTK_WIDGET_UNSET_MAPPED (widget);
}


static void
moo_notebook_forall (GtkContainer *container,
                     gboolean      include_internals,
                     GtkCallback   callback,
                     gpointer      callback_data)
{
    MooNotebook *nb = MOO_NOTEBOOK (container);
    GSList *l;

    for (l = nb->priv->pages; l != NULL; l = l->next)
    {
        Page *page = l->data;
        callback (page->child, callback_data);
        if (include_internals && page != nb->priv->drag_page)
            callback (page->label->widget, callback_data);
    }

    if (include_internals)
    {
        if (nb->priv->action_widgets[LEFT])
            callback (nb->priv->action_widgets[LEFT], callback_data);
        if (nb->priv->action_widgets[RIGHT])
            callback (nb->priv->action_widgets[RIGHT], callback_data);
        callback (nb->priv->arrows, callback_data);
    }
}


static void
moo_notebook_draw_child_border (MooNotebook    *nb,
                                GdkEventExpose *event)
{
    GtkWidget *widget = GTK_WIDGET (nb);
    Page *page = nb->priv->current_page;
    int border_width = get_border_width (nb);
    gboolean draw_gap = TRUE;
    int gap_x = 0;
    int gap_width = 0;

    if (!page)
        return;

    if (nb->priv->in_drag || !nb->priv->tabs_visible)
    {
        draw_gap = FALSE;
    }
    else
    {
        int tab_width = get_tab_window_width (nb);

        gap_x = page->label->offset - nb->priv->labels_offset;
        gap_width = page->label->width;

        if (gap_x < 0)
        {
            if (gap_x + gap_width <= 0)
            {
                draw_gap = FALSE;
            }
            else
            {
                gap_width += gap_x;
                gap_x = 0;
            }
        }
        else if (gap_x >= tab_width)
        {
            draw_gap = FALSE;
        }
        else if (gap_x + gap_width >= tab_width)
        {
            gap_width = tab_width - gap_x;
        }

        gap_x += nb->priv->action_widgets_size[LEFT];
    }

    if (draw_gap)
    {
        gtk_paint_box_gap (widget->style,
                           event->window,
                           GTK_STATE_NORMAL,
                           GTK_SHADOW_OUT,
                           &event->area,
                           widget,
                           DETAIL_NOTEBOOK,
                           widget->allocation.x + border_width,
                           widget->allocation.y + border_width + nb->priv->tabs_height,
                           widget->allocation.width - 2*border_width,
                           nb->priv->child_height,
                           GTK_POS_TOP,
                           gap_x, gap_width);
    }
    else
    {
        gtk_paint_box (widget->style,
                       event->window,
                       GTK_STATE_NORMAL,
                       GTK_SHADOW_OUT,
                       &event->area,
                       widget,
                       DETAIL_NOTEBOOK,
                       widget->allocation.x + border_width,
                       widget->allocation.y + border_width + nb->priv->tabs_height,
                       widget->allocation.width - 2*border_width,
                       nb->priv->child_height);
    }
}


static gboolean
moo_notebook_expose (GtkWidget      *widget,
                     GdkEventExpose *event)
{
    MooNotebook *nb = MOO_NOTEBOOK (widget);

    if (event->window == nb->priv->tab_window)
        moo_notebook_draw_labels (nb, event);

    if (event->window == widget->window && nb->priv->tabs_visible)
        moo_notebook_draw_child_border (nb, event);

    /* do not let GtkNotebook try to draw */
    GTK_WIDGET_CLASS(moo_notebook_grand_parent_class)->expose_event (widget, event);

    if (nb->priv->in_drag && event->window == nb->priv->tab_window)
        moo_notebook_draw_dragged_label (nb, event);

    return FALSE;
}


static gboolean
moo_notebook_focus_in (GtkWidget      *widget,
                       G_GNUC_UNUSED GdkEventFocus *event)
{
    /* XXX invalidate only one label, not whole tab window */
    if (MOO_NOTEBOOK(widget)->priv->focus_page)
        labels_invalidate (MOO_NOTEBOOK (widget));
    return FALSE;
}


static gboolean
moo_notebook_focus_out (GtkWidget      *widget,
                        G_GNUC_UNUSED GdkEventFocus *event)
{
    /* XXX invalidate only one label, not whole tab window */
    if (MOO_NOTEBOOK(widget)->priv->focus_page)
        labels_invalidate (MOO_NOTEBOOK (widget));
    return FALSE;
}


static void
moo_notebook_add (GtkContainer *container,
                  GtkWidget    *widget)
{
    moo_notebook_insert_page (MOO_NOTEBOOK (container), widget, NULL, -1);
}


/* XXX focus */
static void
moo_notebook_remove (GtkContainer *container,
                     GtkWidget    *widget)
{
    MooNotebook *nb = MOO_NOTEBOOK (container);
    Page *page;

    g_return_if_fail (widget != nb->priv->arrows);

    if (nb->priv->action_widgets[LEFT] == widget ||
        nb->priv->action_widgets[RIGHT] == widget)
    {
        if (nb->priv->action_widgets[LEFT] == widget)
            nb->priv->action_widgets[LEFT] = NULL;
        else
            nb->priv->action_widgets[RIGHT] = NULL;

        gtk_widget_unparent (widget);
        g_object_unref (widget);

        moo_notebook_check_action_widgets (nb);
    }
    else if ((page = find_child (nb, widget)))
    {
        g_signal_handlers_disconnect_by_func (widget,
                                              (gpointer) child_visible_notify,
                                              nb);
        gtk_widget_unparent (widget);
        g_object_unref (widget);
        gtk_widget_unparent (page->label->widget);
        g_object_unref (page->label->widget);

        delete_page (nb, page);
    }
    else if ((page = find_label (nb, widget)))
    {
        char *text;

        gtk_widget_unparent (widget);
        g_object_unref (widget);

        text = g_strdup_printf ("Page %d", page_index (nb, page));
        widget = gtk_label_new (text);
        g_free (text);

        g_object_ref_sink (widget);

        if (GTK_WIDGET_REALIZED (nb))
            gtk_widget_set_parent_window (widget, nb->priv->tab_window);

        gtk_widget_set_parent (widget, GTK_WIDGET (nb));
    }
    else
    {
        g_return_if_reached ();
    }

    gtk_widget_queue_resize (GTK_WIDGET (nb));
}


static void
child_visible_notify (GtkWidget      *child,
                      G_GNUC_UNUSED GParamSpec *arg,
                      MooNotebook    *nb)
{
    Page *page;

    if ((page = find_child (nb, child)))
    {
        if (GTK_WIDGET_VISIBLE (child))
        {
            if (nb->priv->show_tabs)
            {
                gtk_widget_show (page->label->widget);
                GTK_WIDGET_SET_CAN_FOCUS (nb);
            }

            if (!nb->priv->current_page)
                nb->priv->current_page = page;
        }
        else
        {
            gtk_widget_hide (page->label->widget);
            if (nb->priv->current_page == page)
            {
                int n;
                nb->priv->current_page = NULL;
                n = find_next_visible_page (nb, page_index (nb, page));
                if (n >= 0)
                    moo_notebook_set_current_page (nb, n);
                else
                    GTK_WIDGET_UNSET_CAN_FOCUS (nb);
            }
        }
    }

    moo_notebook_check_action_widgets (nb);
    gtk_widget_queue_resize (GTK_WIDGET (nb));
}


void
moo_notebook_set_show_tabs (MooNotebook *notebook,
                            gboolean     show_tabs)
{
    g_return_if_fail (MOO_IS_NOTEBOOK (notebook));

    if (show_tabs == notebook->priv->show_tabs)
        return;

    notebook->priv->show_tabs = show_tabs;
    moo_notebook_check_tabs (notebook);
    gtk_widget_queue_resize (GTK_WIDGET (notebook));

    g_object_notify (G_OBJECT (notebook), "show-tabs");
}


/*************************************************************************/
/* Children
 */

static int
page_index (MooNotebook *nb,
            Page        *page)
{
    g_return_val_if_fail (page != NULL, -1);
    return g_slist_index (nb->priv->pages, page);
}


static int
page_visible_index (MooNotebook *nb,
                    Page        *page)
{
    int i = 0;

    g_return_val_if_fail (page != NULL, -1);

    VISIBLE_FOREACH_START (nb, p) {
        if (p == page)
            return i;
        else
            i++;
    } VISIBLE_FOREACH_END;

    return -1;
}


static int
get_n_visible_pages (MooNotebook *nb)
{
    int n = 0;

    VISIBLE_FOREACH_START (nb, p) {
        n++;
    } VISIBLE_FOREACH_END;

    return n;
}


static Page *
get_nth_visible_page (MooNotebook *nb,
                      int          n)
{
    int i = 0;

    VISIBLE_FOREACH_START (nb, page) {
        if (i == n)
            return page;
        else
            i++;
    } VISIBLE_FOREACH_END;

    return NULL;
}


/* XXX focus */
gint
moo_notebook_insert_page (MooNotebook *nb,
                          GtkWidget   *child,
                          GtkWidget   *label,
                          gint         position)
{
    Page *page;

    g_return_val_if_fail (MOO_IS_NOTEBOOK (nb), -1);
    g_return_val_if_fail (GTK_IS_WIDGET (child), -1);
    g_return_val_if_fail (child->parent == NULL, -1);
    g_return_val_if_fail (!label || GTK_IS_WIDGET (label), -1);

    /* XXX GTK_WIDGET_SET_FLAGS (nb, GTK_CAN_FOCUS); */

    if (position < 0 || position > moo_notebook_get_n_pages (nb))
        position = moo_notebook_get_n_pages (nb);

    page = g_new0 (Page, 1);
    page->child = child;
    page->label = g_new0 (Label, 1);
    nb->priv->pages = g_slist_insert (nb->priv->pages, page, position);

    if (!label)
    {
        char *text = g_strdup_printf ("Page %d", position);
        label = gtk_label_new (text);
        g_free (text);
    }

    page->label->widget = label;

    g_object_ref_sink (child);
    g_object_ref_sink (label);

    if (GTK_WIDGET_REALIZED (nb))
        gtk_widget_set_parent_window (label, nb->priv->tab_window);

    gtk_widget_set_parent (child, GTK_WIDGET (nb));
    gtk_widget_set_parent (label, GTK_WIDGET (nb));

    if (GTK_WIDGET_VISIBLE (child) && nb->priv->tabs_visible)
    {
        /* XXX do something about tabs */
        GTK_WIDGET_SET_CAN_FOCUS (nb);
        gtk_widget_show (label);
    }

    g_signal_connect (child, "notify::visible",
                      G_CALLBACK (child_visible_notify), nb);

    if (!nb->priv->current_page && GTK_WIDGET_VISIBLE (child))
        moo_notebook_set_current_page (nb, position);

    moo_notebook_check_tabs (nb);
    gtk_widget_queue_resize (GTK_WIDGET (nb));

    return position;
}


void
moo_notebook_reorder_child (MooNotebook *notebook,
                            GtkWidget   *child,
                            gint         position)
{
    Page *page;
    int n_pages;

    g_return_if_fail (MOO_IS_NOTEBOOK (notebook));

    page = find_child (notebook, child);
    g_return_if_fail (page != NULL);

    n_pages = moo_notebook_get_n_pages (notebook);

    if (position < 0 || position > n_pages - 1)
        position = n_pages - 1;

    if (page_index (notebook, page) == position)
        return;

    notebook->priv->pages = g_slist_remove (notebook->priv->pages, page);
    notebook->priv->pages = g_slist_insert (notebook->priv->pages, page, position);

    gtk_widget_queue_resize (GTK_WIDGET (notebook));
}


static Page *
find_child (MooNotebook *nb,
            GtkWidget   *child)
{
    GSList *l;

    for (l = nb->priv->pages; l != NULL; l = l->next)
    {
        Page *page = l->data;
        if (page->child == child)
            return page;
    }

    return NULL;
}


static Page *
find_grand_child (MooNotebook *nb,
                  GtkWidget   *child)
{
    GSList *l;

    g_return_val_if_fail (GTK_IS_WIDGET (child), NULL);

    for (l = nb->priv->pages; l != NULL; l = l->next)
    {
        Page *page = l->data;
        if (page->child == child || gtk_widget_is_ancestor (child, page->child))
            return page;
    }

    return NULL;
}


static Page *
find_label (MooNotebook *nb,
            GtkWidget   *label)
{
    GSList *l;

    for (l = nb->priv->pages; l != NULL; l = l->next)
    {
        Page *page = l->data;
        if (page->label->widget == label)
            return page;
    }

    return NULL;
}


static Page *
get_nth_page (MooNotebook *nb,
              int          n)
{
    if (n < 0)
        return NULL;
    else
        return g_slist_nth_data (nb->priv->pages, n);
}


static int
find_next_visible_page (MooNotebook *nb,
                        int          n)
{
    int num_pages = moo_notebook_get_n_pages (nb);

    if (!num_pages)
        return -1;

    if (n > 0)
    {
        int i;
        for (i = MIN (n - 1, num_pages - 1); i >= 0; --i)
        {
            Page *page = get_nth_page (nb, i);
            if (GTK_WIDGET_VISIBLE (page->child))
                return i;
        }
    }

    if (n < num_pages)
    {
        int i;
        for (i = MAX (0, n); i < num_pages; ++i)
        {
            Page *page = get_nth_page (nb, i);
            if (GTK_WIDGET_VISIBLE (page->child))
                return i;
        }
    }

    return -1;
}


static void
delete_page (MooNotebook *nb,
             Page        *page)
{
    int n = page_index (nb, page);
    g_return_if_fail (n >= 0);

    if (page == nb->priv->current_page)
        nb->priv->current_page = NULL;

    if (page->focus_child)
        g_object_weak_unref (G_OBJECT (page->focus_child),
                             (GWeakNotify) g_nullify_pointer,
                             &page->focus_child);

    g_free (page->label);
    g_free (page);

    nb->priv->pages = g_slist_remove (nb->priv->pages, page);

    if (!nb->priv->current_page)
    {
        n = find_next_visible_page (nb, n);

        if (n >= 0)
            moo_notebook_set_current_page (nb, n);
    }

    moo_notebook_check_tabs (nb);
    labels_invalidate (nb);
}


void
moo_notebook_remove_page (MooNotebook *notebook,
                          gint         page_num)
{
    Page *page;
    g_return_if_fail (MOO_IS_NOTEBOOK (notebook));
    page = get_nth_page (notebook, page_num);
    g_return_if_fail (page != NULL);
    gtk_container_remove (GTK_CONTAINER (notebook), page->child);
}


gint
moo_notebook_get_n_pages (MooNotebook *notebook)
{
    g_return_val_if_fail (MOO_IS_NOTEBOOK (notebook), 0);
    return g_slist_length (notebook->priv->pages);
}


gint
moo_notebook_page_num (MooNotebook *notebook,
                       GtkWidget   *child)
{
    Page *page;

    g_return_val_if_fail (MOO_IS_NOTEBOOK (notebook), -1);
    g_return_val_if_fail (GTK_IS_WIDGET (child), -1);

    page = find_child (notebook, child);

    if (page)
        return page_index (notebook, page);
    else
        return -1;
}


gint
moo_notebook_get_current_page (MooNotebook *notebook)
{
    g_return_val_if_fail (MOO_IS_NOTEBOOK (notebook), -1);

    if (notebook->priv->current_page)
        return page_index (notebook, notebook->priv->current_page);
    else
        return -1;
}


GtkWidget *
moo_notebook_get_nth_page (MooNotebook *notebook,
                           gint         page_num)
{
    Page *page;

    g_return_val_if_fail (MOO_IS_NOTEBOOK (notebook), NULL);

    page = get_nth_page (notebook, page_num);

    if (page)
        return page->child;
    else
        return NULL;
}


static void
moo_notebook_switch_page (MooNotebook *nb,
                          guint        page_num)
{
    Page *page;
    FocusType old_focus = nb->priv->focus;

    page = nb->priv->current_page;

    /* TODO: gtk_widget_set_child_visible */

    if (page)
        gtk_widget_unmap (page->child);

    page = get_nth_page (nb, page_num);
    g_return_if_fail (page != NULL);

    if (GTK_WIDGET_MAPPED (nb))
        gtk_widget_map (page->child);

    gtk_widget_show (page->label->widget);

    nb->priv->current_page = page;
    nb->priv->label_move_onscreen = TRUE;

    /* XXX write some focus_child() */
    if (old_focus == FOCUS_CHILD)
        gtk_widget_child_focus (page->child, GTK_DIR_TAB_FORWARD);

    labels_invalidate (nb);
    gtk_widget_queue_resize (GTK_WIDGET (nb));
    g_object_notify (G_OBJECT (nb), "page");
}


void
moo_notebook_set_current_page (MooNotebook *notebook,
                               gint         page_num)
{
    int num_pages;

    g_return_if_fail (MOO_IS_NOTEBOOK (notebook));

    num_pages = moo_notebook_get_n_pages (notebook);

    if (!num_pages || page_num >= num_pages)
        return;

    if (page_num < 0)
        page_num = num_pages - 1;

    if (page_num == moo_notebook_get_current_page (notebook))
        return;

    g_signal_emit (notebook, signals[SWITCH_PAGE], 0, (guint) page_num);
}


static GSList *
get_visible_pages (MooNotebook *nb)
{
    GSList *list = NULL, *l;
    for (l = nb->priv->pages; l != NULL; l = l->next)
        if (GTK_WIDGET_VISIBLE (((Page*)l->data)->child))
            list = g_slist_prepend (list, l->data);
    return g_slist_reverse (list);
}


void
moo_notebook_set_action_widget (MooNotebook *notebook,
                                GtkWidget   *widget,
                                gboolean     right)
{
    GtkWidget **widget_ptr;

    right = right ? TRUE : FALSE;

    g_return_if_fail (MOO_IS_NOTEBOOK (notebook));
    g_return_if_fail (!widget || GTK_IS_WIDGET (widget));
    g_return_if_fail (!widget || widget->parent == NULL);

    widget_ptr = right ? &notebook->priv->action_widgets[RIGHT] :
            &notebook->priv->action_widgets[LEFT];

    if (widget == *widget_ptr)
        return;

    if (*widget_ptr)
        gtk_container_remove (GTK_CONTAINER (notebook), *widget_ptr);

    g_return_if_fail (*widget_ptr == NULL);

    if (widget)
    {
        *widget_ptr = widget;
        g_object_ref_sink (widget);
        gtk_widget_set_parent (widget, GTK_WIDGET (notebook));
    }

    moo_notebook_check_action_widgets (notebook);
    gtk_widget_queue_resize (GTK_WIDGET (notebook));
}


GtkWidget *
moo_notebook_get_action_widget (MooNotebook *notebook,
                                gboolean     right)
{
    g_return_val_if_fail (MOO_IS_NOTEBOOK (notebook), NULL);
    if (right)
        return notebook->priv->action_widgets[RIGHT];
    else
        return notebook->priv->action_widgets[LEFT];
}


static void
moo_notebook_check_action_widgets (MooNotebook *nb)
{
    if (!nb->priv->action_widgets[LEFT] ||
         !GTK_WIDGET_VISIBLE (nb->priv->action_widgets[LEFT]))
            nb->priv->action_widgets_size[LEFT] = 0;
    if (!nb->priv->action_widgets[RIGHT] ||
         !GTK_WIDGET_VISIBLE (nb->priv->action_widgets[RIGHT]))
            nb->priv->action_widgets_size[RIGHT] = 0;
    moo_notebook_check_tabs (nb);
}


static void
moo_notebook_check_tabs (MooNotebook *nb)
{
    if (!nb->priv->show_tabs)
        nb->priv->tabs_visible = FALSE;
    else
        nb->priv->tabs_visible = get_n_visible_pages (nb) ||
                (nb->priv->action_widgets[LEFT] &&
                    GTK_WIDGET_VISIBLE (nb->priv->action_widgets[LEFT])) ||
                (nb->priv->action_widgets[RIGHT] &&
                    GTK_WIDGET_VISIBLE (nb->priv->action_widgets[RIGHT]));

    if (!nb->priv->tabs_visible)
    {
        if (nb->priv->tab_window)
            gdk_window_hide (nb->priv->tab_window);

        nb->priv->tabs_height = 0;
    }
    else if (nb->priv->tab_window)
    {
        gdk_window_show (nb->priv->tab_window);
    }

    NOTEBOOK_CHECK_INVARIANTS (nb);
}


GtkWidget *
moo_notebook_get_tab_label (MooNotebook *notebook,
                            GtkWidget   *child)
{
    Page *page;

    g_return_val_if_fail (MOO_IS_NOTEBOOK (notebook), NULL);
    g_return_val_if_fail (GTK_IS_WIDGET (child), NULL);

    page = find_child (notebook, child);
    g_return_val_if_fail (page != NULL, NULL);

    return page->label->widget;
}


void
moo_notebook_set_tab_label (MooNotebook *notebook,
                            GtkWidget   *child,
                            GtkWidget   *tab_label)
{
    Page *page;

    g_return_if_fail (MOO_IS_NOTEBOOK (notebook));
    g_return_if_fail (GTK_IS_WIDGET (child));
    g_return_if_fail (GTK_IS_WIDGET (tab_label));
    g_return_if_fail (tab_label->parent == NULL);

    page = find_child (notebook, child);
    g_return_if_fail (page != NULL);

    gtk_widget_unparent (page->label->widget);
    g_object_unref (page->label->widget);

    page->label->widget = tab_label;
    g_object_ref_sink (tab_label);

    if (GTK_WIDGET_REALIZED (notebook))
        gtk_widget_set_parent_window (tab_label, notebook->priv->tab_window);

    gtk_widget_set_parent (tab_label, GTK_WIDGET (notebook));
}


/*************************************************************************/
/* Labels
 */

static void
labels_size_allocate (MooNotebook   *nb,
                      GtkAllocation *allocation)
{
    GtkAllocation child_alloc;
    GtkRequisition child_req;
    GSList *list, *l;
    int width, max_offset, height;
    gboolean move_onscreen_again = FALSE;
    gboolean invalidate = FALSE;

    if (!(list = get_visible_pages (nb)))
    {
        if (nb->priv->arrows_visible)
        {
            nb->priv->arrows_visible = FALSE;
            nb->priv->arrows_size = 0;
            gtk_widget_hide (nb->priv->arrows);
        }

        return;
    }

    height = allocation->height;

    if (nb->priv->in_drag)
    {
        Page *page = nb->priv->drag_page;
        int i = g_slist_index (list, page);

        g_return_if_fail (i >= 0);

        if (i != nb->priv->drag_page_index)
        {
            list = g_slist_remove (list, page);
            list = g_slist_insert (list, page, nb->priv->drag_page_index);
        }
    }

    if (nb->priv->labels_homogeneous)
    {
        int max_width = 2*LABEL_OVERLAP;

        for (l = list; l != NULL; l = l->next)
        {
            Page *page = l->data;
            GtkWidget *label = page->label->widget;

            gtk_widget_get_child_requisition (label, &child_req);

            max_width = MAX (max_width, child_req.width + 2 * (int) nb->priv->label_hborder);
        }

        for (l = list, width = 0; l != NULL; l = l->next)
        {
            Page *page = l->data;

            if (max_width != page->label->width)
                invalidate = TRUE;

            page->label->width = max_width;
            page->label->height = allocation->height;

            if (width)
            {
                page->label->offset = width - LABEL_OVERLAP;
                width += (page->label->width - LABEL_OVERLAP);
            }
            else
            {
                page->label->offset = 0;
                width += page->label->width;
            }
        }
    }
    else
    {
        for (l = list, width = 0; l != NULL; l = l->next)
        {
            Page *page = l->data;
            GtkWidget *label = page->label->widget;
            int new_width;

            gtk_widget_get_child_requisition (label, &child_req);

            new_width = MAX (0, child_req.width) + 2 * nb->priv->label_hborder;
            new_width = MAX (new_width, 2*LABEL_OVERLAP);

            if (new_width != page->label->width)
                invalidate = TRUE;

            page->label->width = new_width;
            page->label->height = allocation->height;

            if (width)
            {
                page->label->offset = width - LABEL_OVERLAP;
                width += (page->label->width - LABEL_OVERLAP);
            }
            else
            {
                page->label->offset = 0;
                width += page->label->width;
            }
        }
    }

    max_offset = MAX (width - allocation->width, 0);
    nb->priv->labels_offset = CLAMP (nb->priv->labels_offset, 0, max_offset);
    nb->priv->labels_width = width;
    nb->priv->labels_visible_width = allocation->width;

    if (nb->priv->arrows_visible)
    {
        if (nb->priv->labels_width <= nb->priv->labels_visible_width)
        {
            nb->priv->arrows_visible = FALSE;
            nb->priv->arrows_size = 0;
            gtk_widget_hide (nb->priv->arrows);
        }

        moo_notebook_check_arrows (nb);
    }
    else
    {
        if (nb->priv->labels_width > nb->priv->labels_visible_width)
        {
            nb->priv->arrows_visible = TRUE;
            gtk_widget_show (nb->priv->arrows);
            move_onscreen_again = TRUE;
        }
    }

    if (nb->priv->arrows_gtk || nb->priv->label_move_onscreen)
    {
        labels_move_label_onscreen (nb, nb->priv->current_page, FALSE);
        if (!move_onscreen_again)
            nb->priv->label_move_onscreen = FALSE;
    }
    else
    {
        /* TODO is something needed here? */
    }

    for (l = list; l != NULL; l = l->next)
    {
        Page *page = l->data;
        GtkWidget *label = page->label->widget;

        gtk_widget_get_child_requisition (label, &child_req);

        if (page != nb->priv->drag_page)
            child_alloc.x = page->label->offset;
        else
            child_alloc.x = nb->priv->drag_tab_x;

        child_alloc.x += (nb->priv->label_hborder - nb->priv->labels_offset);
        child_alloc.width = child_req.width;
        child_alloc.y = (height - LABEL_FOCUS_HEIGHT)/2 -
                child_req.height/2 + LABEL_FOCUS_HEIGHT;
        child_alloc.height = child_req.height;

        gtk_widget_size_allocate (label, &child_alloc);
    }

    if (invalidate)
        labels_invalidate (nb);

    g_slist_free (list);
}


static void
moo_notebook_draw_label (MooNotebook    *nb,
                         Page           *page,
                         GdkEventExpose *event)
{
    GtkWidget *widget = GTK_WIDGET (nb);
    GdkWindow *window = nb->priv->tab_window;
    int x, y, height;
    GtkStateType state;

    if (page == nb->priv->current_page)
    {
        y = 0;
        state = GTK_STATE_NORMAL;
    }
    else
    {
        y = LABEL_FOCUS_HEIGHT;
        state = GTK_STATE_ACTIVE;
    }

    x = page->label->offset - nb->priv->labels_offset;
    height = nb->priv->tabs_height - y;

    gtk_paint_extension (widget->style,
                         window,
                         state,
                         GTK_SHADOW_OUT,
                         &event->area,
                         widget,
                         DETAIL_TAB,
                         x, y,
                         page->label->width,
                         height,
                         GTK_POS_BOTTOM);

    if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (nb)) &&
        page == nb->priv->focus_page)
    {
        int focus_width;

        gtk_widget_style_get (widget, "focus-line-width", &focus_width, NULL);

        gtk_paint_focus (widget->style,
                         window,
                         state,
                         &event->area,
                         widget,
                         DETAIL_TAB,
                         page->label->widget->allocation.x - focus_width,
                         page->label->widget->allocation.y - focus_width,
                         page->label->widget->allocation.width + 2 * focus_width,
                         page->label->widget->allocation.height + 2 * focus_width);
    }
}

static void
moo_notebook_draw_labels (MooNotebook    *nb,
                          GdkEventExpose *event)
{
    if (!nb->priv->current_page)
        return;

    VISIBLE_FOREACH_START (nb, page)
    {
        if (page != nb->priv->current_page)
            moo_notebook_draw_label (nb, page, event);
    }
    VISIBLE_FOREACH_END;

    if (!nb->priv->in_drag)
        moo_notebook_draw_label (nb, nb->priv->current_page, event);
}


static void
moo_notebook_draw_dragged_label (MooNotebook    *nb,
                                 GdkEventExpose *event)
{
    GtkWidget *widget = GTK_WIDGET (nb);
    int width, height;

    g_return_if_fail (nb->priv->drag_page != NULL);

    width = nb->priv->drag_page->label->width;
    height = nb->priv->tabs_height;

    if (nb->priv->want_snapshot)
    {
        GdkPixbuf *pixbuf;
        guchar *pixels;
        int rowstride, row, i;

        g_return_if_fail (nb->priv->snapshot_pixmap == NULL &&
                nb->priv->snapshot_pixbuf == NULL);

        /* TODO: this event may not cover whole label area */
        moo_notebook_draw_label (nb, nb->priv->drag_page, event);
        gtk_container_propagate_expose (GTK_CONTAINER (nb),
                                        nb->priv->drag_page->label->widget,
                                        event);

        nb->priv->want_snapshot = FALSE;

        pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE,
                                 8, width, height);

        if (!gdk_pixbuf_get_from_drawable (pixbuf, nb->priv->tab_window,
                                           gdk_drawable_get_colormap (nb->priv->tab_window),
                                           nb->priv->drag_page->label->offset - nb->priv->labels_offset,
                                           0, 0, 0,
                                           width, height))
        {
            g_critical ("could not create pixbuf");
            g_object_unref (pixbuf);

            nb->priv->snapshot_pixmap =
                    gdk_pixmap_new (nb->priv->tab_window,
                                    width, height, -1);
            gdk_draw_drawable (nb->priv->snapshot_pixmap,
                               widget->style->bg_gc[GTK_STATE_NORMAL],
                               nb->priv->tab_window,
                               nb->priv->drag_page->label->offset - nb->priv->labels_offset,
                               0, 0, 0,
                               width, height);
        }
        else
        {
            nb->priv->snapshot_pixbuf = pixbuf;

            pixels = gdk_pixbuf_get_pixels (pixbuf);
            rowstride = gdk_pixbuf_get_rowstride (pixbuf);

            for (row = 0; row < height; ++row)
                for (i = 0; i < width; ++i)
                    pixels[row*rowstride + 4*i + 3] = LABEL_ALPHA;
        }
    }
    else
    {
        GdkRectangle area;

        area.x = nb->priv->drag_tab_x - nb->priv->labels_offset;
        area.y = 0;
        area.width = width;
        area.height = height;

        if (!gdk_rectangle_intersect (&area, &event->area, &area))
            return;

        g_return_if_fail (nb->priv->snapshot_pixmap != NULL || nb->priv->snapshot_pixbuf != NULL);

        if (nb->priv->snapshot_pixbuf)
            gdk_draw_pixbuf (nb->priv->tab_window,
                             widget->style->bg_gc[GTK_STATE_NORMAL],
                             nb->priv->snapshot_pixbuf,
                             area.x - nb->priv->drag_tab_x + nb->priv->labels_offset,
                             area.y,
                             area.x,
                             area.y,
                             area.width,
                             area.height,
                             GDK_RGB_DITHER_NONE, 0, 0);
        else
            gdk_draw_drawable (nb->priv->tab_window,
                               widget->style->bg_gc[GTK_STATE_NORMAL],
                               nb->priv->snapshot_pixmap,
                               event->area.x - nb->priv->drag_tab_x + nb->priv->labels_offset,
                               event->area.y,
                               event->area.x,
                               event->area.y,
                               event->area.width,
                               event->area.height);
    }
}


static void
moo_notebook_check_arrows (MooNotebook *nb)
{
    gboolean sensitive[2];
    GSList *visible;
    Page *page;

    if (!nb->priv->arrows_visible)
        return;

    if (!(visible = get_visible_pages (nb)))
    {
        sensitive[LEFT] = sensitive[RIGHT] = FALSE;
    }
    else if (nb->priv->arrows_gtk)
    {
        page = nb->priv->current_page;

        g_return_if_fail (page != NULL);
        g_return_if_fail (g_slist_find (visible, page) != NULL);

        sensitive[LEFT] = (page != visible->data);
        sensitive[RIGHT] = (page != g_slist_last(visible)->data);
    }
    else
    {
        sensitive[LEFT] = (nb->priv->labels_offset > 0);
        sensitive[RIGHT] = nb->priv->labels_width <= nb->priv->labels_visible_width ||
                           nb->priv->labels_offset < nb->priv->labels_width - nb->priv->labels_visible_width - 1;
    }

    /* TODO focus */
    gtk_widget_set_sensitive (nb->priv->left_arrow, sensitive[LEFT]);
    gtk_widget_set_sensitive (nb->priv->right_arrow, sensitive[RIGHT]);

    g_slist_free (visible);
}


static void
left_arrow_clicked (GtkWidget   *button,
                    MooNotebook *nb)
{
    g_return_if_fail (button == nb->priv->left_arrow);

    if (nb->priv->arrows_gtk)
    {
        int n = moo_notebook_get_current_page (nb) - 1;
        g_return_if_fail (n >= 0);
        moo_notebook_set_current_page (nb, n);
    }
    else
    {
        labels_scroll (nb, GTK_DIR_LEFT);
    }
}


static void
right_arrow_clicked (GtkWidget   *button,
                     MooNotebook *nb)
{
    g_return_if_fail (button == nb->priv->right_arrow);

    if (nb->priv->arrows_gtk)
    {
        int n = moo_notebook_get_current_page (nb) + 1;
        g_return_if_fail (n < moo_notebook_get_n_pages (nb));
        moo_notebook_set_current_page (nb, n);
    }
    else
    {
        labels_scroll (nb, GTK_DIR_RIGHT);
    }
}


#define SCROLL_PAD 6

static void
labels_scroll (MooNotebook      *nb,
               GtkDirectionType  where)
{
    GSList *visible;
    Page *page;
    int offset, index_;

    g_return_if_fail (!nb->priv->arrows_gtk);
    g_return_if_fail (nb->priv->tabs_visible);

    if (nb->priv->labels_visible_width >= nb->priv->labels_width)
        return;

    visible = get_visible_pages (nb);
    g_return_if_fail (visible != NULL);

    if (where == GTK_DIR_LEFT)
    {
        page = find_label_at_x (nb, 0, FALSE);
        index_ = g_slist_index (visible, page);
        g_return_if_fail (index_ >= 0);

        if (index_ == 0)
            offset = 0;
        else
            offset = page->label->offset - SCROLL_PAD;
    }
    else
    {
        page = find_label_at_x (nb, nb->priv->labels_visible_width - 1, FALSE);
        index_ = g_slist_index (visible, page);
        g_return_if_fail (index_ >= 0);

        if (index_ == (int) g_slist_length (visible) - 1)
            offset = nb->priv->labels_width - nb->priv->labels_visible_width;
        else
            offset = page->label->offset + page->label->width -
                    nb->priv->labels_visible_width + SCROLL_PAD;
    }

    labels_scroll_to_offset (nb, offset, TRUE);
}


static void
labels_scroll_to_offset (MooNotebook *nb,
                         int          offset,
                         gboolean     realloc)
{
    int delta;

    offset = CLAMP (offset, 0, nb->priv->labels_width - nb->priv->labels_visible_width);
    delta = offset - nb->priv->labels_offset;

    nb->priv->labels_offset += delta;

    if (realloc)
        gtk_widget_queue_resize (GTK_WIDGET (nb));

    labels_invalidate (nb);
    moo_notebook_check_arrows (nb);
}


static Page *
find_label_at_x (MooNotebook    *nb,
                 int             x,
                 gboolean        widget_only)
{
    return find_label_at_xy (nb, x, nb->priv->tabs_height / 2, widget_only);
}


static Page*
find_label_at_xy (MooNotebook    *nb,
                  int             x,
                  int             y,
                  gboolean        widget_only)
{
    /* TODO labels overlap */

    if (y < 0 || y >= nb->priv->tabs_height)
        return NULL;

    /* here it must go through all the pages because they may be swapped on drag */
    /* XXX so it could just check if we are in drag */
    VISIBLE_FOREACH_START(nb, page)
    {
        int lx = page->label->offset - nb->priv->labels_offset;
        int lwidth = page->label->width;
        int ax = page->label->widget->allocation.x;
        int awidth = page->label->widget->allocation.width;
        int ay = page->label->widget->allocation.y;
        int aheight = page->label->widget->allocation.height;

        if (x >= lx && x < lx + lwidth)
        {
            if (widget_only)
            {
                if (x >= ax && x < ax + awidth && y >= ay && y >= ay + aheight)
                    return page;
                else
                    return NULL;
            }
            else
            {
                return page;
            }
        }
    }
    VISIBLE_FOREACH_END;

    return NULL;
}


static void
labels_move_label_onscreen (MooNotebook *nb,
                            Page        *page,
                            gboolean     realloc)
{
    int offset;

    g_return_if_fail (page != NULL);

    if (nb->priv->labels_visible_width >= nb->priv->labels_width)
        return;

    if (page->label->width >= nb->priv->labels_visible_width ||
        page->label->offset < nb->priv->labels_offset)
    {
        offset = page->label->offset - SCROLL_PAD;
    }
    else if (page->label->offset + page->label->width >
             nb->priv->labels_offset + nb->priv->labels_visible_width)
    {
        offset = page->label->offset + page->label->width -
                nb->priv->labels_visible_width + SCROLL_PAD;
    }
    else
    {
        return;
    }

    labels_scroll_to_offset (nb, offset, realloc);
}


static void
moo_notebook_set_homogeneous (MooNotebook *nb,
                              gboolean     homogeneous)
{
    g_return_if_fail (MOO_IS_NOTEBOOK (nb));
    if (nb->priv->labels_homogeneous != homogeneous)
    {
        nb->priv->labels_homogeneous = homogeneous;
        if (nb->priv->tabs_visible)
            gtk_widget_queue_resize (GTK_WIDGET (nb));
    }
}


/*****************************************************************************/
/* Mouse stuff
 */

#define SCROLL_TIMEOUT 50
#define SCROLL_STEP 5

static void     tab_drag_cancel             (MooNotebook    *nb);
static void     tab_drag_start              (MooNotebook    *nb,
                                             GdkEventMotion *event);
static void     tab_drag_motion             (MooNotebook    *nb,
                                             GdkEventMotion *event);
static void     tab_drag_end                (MooNotebook    *nb,
                                             gboolean        drop);
static void     drag_scroll_start           (MooNotebook    *nb,
                                             gboolean        right);
static void     drag_scroll_stop            (MooNotebook    *nb);
static gboolean drag_scroll                 (MooNotebook    *nb);


static void
tab_drag_cancel (MooNotebook *nb)
{
    g_print ("cancel\n");
    tab_drag_end (nb, FALSE);
}


static void
drag_scroll_start (MooNotebook *nb,
                   gboolean     right)
{
    if (nb->priv->drag_scroll_id)
    {
        if (nb->priv->drag_scroll_right != right)
            g_source_remove (nb->priv->drag_scroll_id);
        else
            return;
    }

    nb->priv->drag_scroll_right = right;
    nb->priv->drag_scroll_id = g_timeout_add (SCROLL_TIMEOUT,
                                              (GSourceFunc) drag_scroll,
                                              nb);
}


static void
drag_scroll_stop (MooNotebook *nb)
{
    if (nb->priv->drag_scroll_id)
        g_source_remove (nb->priv->drag_scroll_id);
    nb->priv->drag_scroll_id = 0;
}


static gboolean
drag_scroll (MooNotebook *nb)
{
    int delta;
    int offset, max_offset;
    int width;
    gboolean retval = TRUE;

    offset = nb->priv->labels_offset;
    max_offset = nb->priv->labels_width - nb->priv->labels_visible_width;
    width = nb->priv->drag_page->label->width;

    if (max_offset <= 0)
    {
        drag_scroll_stop (nb);
        g_critical ("oops");
        retval = FALSE;
        goto out;
    }

    if (nb->priv->drag_scroll_right)
    {
        if (nb->priv->drag_mouse_x > nb->priv->labels_visible_width)
            delta = SCROLL_STEP * (2 + (nb->priv->drag_mouse_x - nb->priv->labels_visible_width)/width);
        else
            delta = SCROLL_STEP;

        if (offset + delta > max_offset)
            delta = MAX (0, max_offset - offset);
    }
    else
    {
        if (nb->priv->drag_mouse_x < 0)
            delta = -SCROLL_STEP * (2 + (-nb->priv->drag_mouse_x)/width);
        else
            delta = -SCROLL_STEP;

        if (offset + delta < 0)
            delta = MIN (-offset, 0);
    }

    if (delta)
    {
        nb->priv->labels_offset += delta;
        nb->priv->drag_tab_x -= delta;
        tab_drag_motion (nb, NULL);
    }

out:
    return retval;
}


static void
tab_drag_end (MooNotebook *nb,
              gboolean     drop)
{
    int index_;
    Page *old_page;
    Page *drag_page = nb->priv->drag_page;

    nb->priv->button_pressed = FALSE;
    nb->priv->in_drag = FALSE;
    nb->priv->drag_page = NULL;

    if (nb->priv->snapshot_pixbuf)
        g_object_unref (nb->priv->snapshot_pixbuf);
    if (nb->priv->snapshot_pixmap)
        g_object_unref (nb->priv->snapshot_pixmap);
    nb->priv->snapshot_pixbuf = NULL;
    nb->priv->snapshot_pixmap = NULL;
    nb->priv->want_snapshot = FALSE;

    drag_scroll_stop (nb);

    index_ = page_visible_index (nb, drag_page);

    if (drop && index_ != nb->priv->drag_page_index)
    {
        index_ = nb->priv->drag_page_index;
        old_page = get_nth_visible_page (nb, index_);
        g_return_if_fail (old_page != NULL);

        index_ = page_index (nb, old_page);
        moo_notebook_reorder_child (nb, drag_page->child, index_);
        labels_move_label_onscreen  (nb, get_nth_page (nb, index_), FALSE);
    }

    labels_invalidate (nb);
    gtk_widget_queue_resize (GTK_WIDGET (nb));
}


static gboolean
translate_coords (GdkWindow *parent_window,
                  GdkWindow *window,
                  int       *x,
                  int       *y)
{
    while (window != parent_window)
    {
        int dx, dy;

        gdk_window_get_position (window, &dx, &dy);
        (*x) += dx;
        (*y) += dy;

        window = gdk_window_get_parent (window);

        if (!window)
            return FALSE;
    }

    return TRUE;
}


static void
tab_drag_start (MooNotebook    *nb,
                GdkEventMotion *event)
{
    Page *page;
    GdkRectangle rect;
    int event_x, event_y;

    g_return_if_fail (nb->priv->button_pressed);

    event_x = (int) event->x;
    event_y = (int) event->y;

    if (!translate_coords (nb->priv->tab_window, event->window, &event_x, &event_y))
    {
        g_critical ("oops");
        return;
    }

    page = nb->priv->current_page;
    g_return_if_fail (page != NULL);
    nb->priv->drag_page = page;
    nb->priv->drag_page_index = page_visible_index (nb, page);

    nb->priv->drag_tab_x_delta = event_x + nb->priv->labels_offset - page->label->offset;
    nb->priv->in_drag = TRUE;

    nb->priv->want_snapshot = TRUE;

    rect.x = page->label->offset - nb->priv->labels_offset;
    rect.y = 0;
    rect.width = page->label->width;
    rect.height = nb->priv->tabs_height;

    gdk_window_invalidate_rect (nb->priv->tab_window, &rect, TRUE);
    gdk_window_process_updates (nb->priv->tab_window, TRUE);
}


static void
tab_drag_motion (MooNotebook    *nb,
                 GdkEventMotion *event)
{
    int x, new_index, width, offset, num, i;
    GSList *visible, *l;
    Page *drag_page;
    int event_x, event_y;

    if (!nb->priv->in_drag)
    {
        tab_drag_cancel (nb);
        g_return_if_reached ();
    }

    if (event)
    {
        event_x = (int) event->x;
        event_y = (int) event->y;

        if (!translate_coords (nb->priv->tab_window, event->window, &event_x, &event_y))
        {
            g_critical ("oops");
            return;
        }
    }
    else
    {
        event_x = nb->priv->drag_mouse_x;
    }

    width = nb->priv->drag_page->label->width;
    num = get_n_visible_pages (nb);

    if (event)
    {
        x = event_x + nb->priv->labels_offset - nb->priv->drag_tab_x_delta;
        nb->priv->drag_mouse_x = event_x;
    }
    else
    {
        x = nb->priv->drag_mouse_x + nb->priv->labels_offset - nb->priv->drag_tab_x_delta;
    }

    nb->priv->drag_tab_x = x;

    drag_page = nb->priv->drag_page;
    visible = get_visible_pages (nb);
    visible = g_slist_remove (visible, drag_page);
    visible = g_slist_insert (visible, drag_page, nb->priv->drag_page_index);

    new_index = nb->priv->drag_page_index;

    for (l = visible, i = 0, offset = 0; l != NULL; l = l->next, ++i)
    {
        Page *page;
        int min_width;

        page = l->data;
        min_width = MIN (page->label->width, width);

        if (i == new_index)
        {
            offset += page->label->width;
            continue;
        }

        if (i == 0)
        {
            if (x + min_width/2 < offset + min_width)
            {
                new_index = i;
                break;
            }
        }
        else if (i == num - 1)
        {
            if (x + min_width/2 >= offset)
            {
                new_index = i;
                break;
            }
        }
        else if (x + min_width/2 >= offset &&
                 x + min_width/2 < offset + min_width)
        {
            new_index = i;
            break;
        }

        offset += page->label->width;
    }

    nb->priv->drag_page_index = new_index;

    if (nb->priv->labels_width > nb->priv->labels_visible_width)
    {
        x += (nb->priv->drag_tab_x_delta - nb->priv->labels_offset);

        if (x < width/3)
            drag_scroll_start (nb, LEFT);
        else if (x + width/3 > nb->priv->labels_visible_width)
            drag_scroll_start (nb, RIGHT);
        else
            drag_scroll_stop (nb);
    }

    gtk_widget_queue_resize (GTK_WIDGET (nb));
    g_slist_free (visible);
}


int
moo_notebook_get_event_tab (MooNotebook    *nb,
                            GdkEvent       *event)
{
    int x = 0, y = 0;
    Page *page;

    g_return_val_if_fail (MOO_IS_NOTEBOOK (nb), -1);
    g_return_val_if_fail (event != NULL && event->any.window != NULL, -1);
    g_return_val_if_fail (nb->priv->tab_window != NULL, -1);

    switch (event->type)
    {
        case GDK_MOTION_NOTIFY:
            x = (int)event->motion.x;
            y = (int)event->motion.y;
            break;
        case GDK_BUTTON_PRESS:
        case GDK_2BUTTON_PRESS:
        case GDK_3BUTTON_PRESS:
        case GDK_BUTTON_RELEASE:
            x = (int)event->button.x;
            y = (int)event->button.y;
            break;
        case GDK_ENTER_NOTIFY:
        case GDK_LEAVE_NOTIFY:
            x = (int)event->crossing.x;
            y = (int)event->crossing.y;
            break;
        case GDK_SCROLL:
            x = (int)event->scroll.x;
            y = (int)event->scroll.y;
            break;
        default:
            g_return_val_if_reached (-1);
    }

    if (!translate_coords (nb->priv->tab_window, event->any.window, &x, &y))
        return -1;

    page = find_label_at_xy (nb, x, y, FALSE);

    return page ? page_index (nb, page) : -1;
}


static gboolean
moo_notebook_button_press (GtkWidget      *widget,
                           GdkEventButton *event)
{
    Page *page;
    MooNotebook *nb = MOO_NOTEBOOK (widget);
    int x, y;

    x = (int) event->x;
    y = (int) event->y;

    if (!translate_coords (nb->priv->tab_window, event->window, &x, &y))
        return FALSE;

    if (event->button == 1 && event->type == GDK_BUTTON_PRESS)
    {
        if (nb->priv->button_pressed || nb->priv->in_drag)
        {
            tab_drag_cancel (nb);
            g_return_val_if_reached (FALSE);
        }

        if (!nb->priv->focus && !GTK_WIDGET_HAS_FOCUS (widget))
            gtk_widget_grab_focus (widget);

        page = find_label_at_xy (nb, x, y, FALSE);

        if (page)
        {
            gboolean focus_label = FALSE;

            if (page == nb->priv->current_page)
            {
                focus_label = TRUE;
            }
            else
            {
                moo_notebook_set_current_page (nb, page_index (nb, page));
                g_assert (page == nb->priv->current_page);
                if (page->focus_child && GTK_WIDGET_CAN_FOCUS (page->focus_child))
                    gtk_widget_grab_focus (page->focus_child);
                else if (!gtk_widget_child_focus (page->child, GTK_DIR_TAB_FORWARD))
                    focus_label = TRUE;
            }

            if (focus_label &&
                !gtk_widget_child_focus (page->label->widget, GTK_DIR_TAB_FORWARD))
            {
                nb->priv->focus = FOCUS_NONE;
                nb->priv->focus_page = page;
                if (!GTK_WIDGET_HAS_FOCUS (widget))
                    gtk_widget_grab_focus (widget);
                labels_invalidate (nb);
            }

            if (get_n_visible_pages (nb) > 1)
                nb->priv->button_pressed = TRUE;

            return TRUE;
        }

        return FALSE;
    }

    if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    {
        return moo_notebook_maybe_popup (nb, event);
    }

    return FALSE;
}


static gboolean
moo_notebook_button_release (GtkWidget *widget,
                             G_GNUC_UNUSED GdkEventButton *event)
{
    MooNotebook *nb = MOO_NOTEBOOK (widget);

    if (!nb->priv->button_pressed)
        return FALSE;

    nb->priv->button_pressed = FALSE;

    if (nb->priv->in_drag)
        tab_drag_end (nb, TRUE);

    return TRUE;
}


static gboolean
moo_notebook_scroll_event (GtkWidget      *widget,
                           GdkEventScroll *event)
{
    MooNotebook *nb = MOO_NOTEBOOK (widget);
    int x, y;

    x = (int) event->x;
    y = (int) event->y;

    if (!translate_coords (nb->priv->tab_window, event->window, &x, &y))
        return FALSE;

    switch (event->direction)
    {
        case GDK_SCROLL_UP:
        case GDK_SCROLL_LEFT:
            labels_scroll (nb, GTK_DIR_LEFT);
            return TRUE;

        case GDK_SCROLL_DOWN:
        case GDK_SCROLL_RIGHT:
            labels_scroll (nb, GTK_DIR_RIGHT);
            return TRUE;
    }

    return FALSE;
}


static gboolean
moo_notebook_key_press (GtkWidget   *widget,
                        GdkEventKey *event)
{
    MooNotebook *nb = MOO_NOTEBOOK (widget);

    if (nb->priv->in_drag && event->keyval == GDK_Escape)
    {
        tab_drag_end (nb, FALSE);
        return TRUE;
    }
    else if (GTK_WIDGET_HAS_FOCUS (nb) &&
             nb->priv->focus == FOCUS_NONE &&
             nb->priv->focus_page &&
             nb->priv->focus_page != nb->priv->current_page)
    {
        switch (event->keyval)
        {
            case GDK_Return:
            case GDK_space:
                moo_notebook_set_current_page (nb, page_index (nb, nb->priv->focus_page));
                return TRUE;
        }
    }

    return FALSE;
}


static gboolean
moo_notebook_motion (GtkWidget      *widget,
                     GdkEventMotion *event)
{
    MooNotebook *nb = MOO_NOTEBOOK (widget);

    if (!nb->priv->button_pressed)
    {
        if (nb->priv->in_drag)
        {
            tab_drag_cancel (nb);
            g_return_val_if_reached (FALSE);
        }
        else
        {
            return FALSE;
        }
    }

    if (nb->priv->in_drag)
    {
        tab_drag_motion (nb, event);
        return TRUE;
    }

    tab_drag_start (nb, event);
    tab_drag_motion (nb, event);
    return TRUE;
}


void
moo_notebook_enable_reordering (MooNotebook *notebook,
                                gboolean     enable)
{
    g_return_if_fail (MOO_IS_NOTEBOOK (notebook));
    notebook->priv->enable_reordering = enable;
    g_object_notify (G_OBJECT (notebook), "enable-reordering");
}


/*****************************************************************************/
/* Popup menu
 */

static void
popup_position_func (G_GNUC_UNUSED GtkMenu *menu,
                     gint           *x,
                     gint           *y,
                     gboolean       *push_in,
                     gpointer        user_data)
{
    struct {
        MooNotebook *nb;
        Page *page;
        GdkEventButton *event;
    } *data = user_data;

    g_return_if_fail (data != NULL);
    g_return_if_fail (data->nb != NULL && data->nb->priv->tab_window != NULL);
    g_return_if_fail (data->page != NULL);

    if (data->event)
        gdk_window_get_origin (data->event->window, x, y);
    else
        gdk_window_get_origin (data->nb->priv->tab_window, x, y);

    if (data->event)
    {
        (*x) += (int) data->event->x;
        (*y) += (int) data->event->y;
    }
    else
    {
        (*x) += (data->page->label->offset - data->nb->priv->labels_offset +
                data->page->label->width * 2 / 3);
        (*y) += (data->nb->priv->tabs_height * 2 / 3);
    }

    *push_in = TRUE;
}


static gboolean
moo_notebook_do_popup (MooNotebook    *nb,
                       Page           *page,
                       GdkEventButton *event)
{
    GtkWidget *menu;
    gboolean dont = FALSE;
    struct {
        MooNotebook *nb;
        Page *page;
        GdkEventButton *event;
    } data;

    g_return_val_if_fail (page != NULL, FALSE);

    menu = gtk_menu_new ();
    g_object_ref_sink (menu);

    g_signal_emit (nb, signals[POPULATE_POPUP], 0, page->child, menu, &dont);

    if (dont)
    {
        g_object_unref (menu);
        return FALSE;
    }

    data.nb = nb;
    data.page = page;
    data.event = event;

    gtk_menu_popup (GTK_MENU (menu),NULL, NULL,
                    popup_position_func, &data,
                    event ? event->button : 0,
                    event ? event->time : gtk_get_current_event_time ());
    g_object_unref (menu);
    return TRUE;
}


static gboolean
moo_notebook_maybe_popup (MooNotebook    *nb,
                          GdkEventButton *event)
{
    Page *page;
    int event_x, event_y;

    if (!nb->priv->enable_popup)
        return FALSE;

    event_x = event->x;
    event_y = event->y;

    if (!translate_coords (nb->priv->tab_window, event->window, &event_x, &event_y))
    {
        g_critical ("oops");
        return FALSE;
    }

    page = find_label_at_xy (nb, event_x, event_y, FALSE);

    if (!page)
        return FALSE;

    return moo_notebook_do_popup (nb, page, event);
}


void
moo_notebook_enable_popup (MooNotebook *notebook,
                           gboolean     enable)
{
    g_return_if_fail (MOO_IS_NOTEBOOK (notebook));
    notebook->priv->enable_popup = enable;
    g_object_notify (G_OBJECT (notebook), "enable-popup");
}


/*****************************************************************************/
/* Focus and keyboard navigation
 */

static gboolean
focus_to_action_widget (MooNotebook     *nb,
                        int              n,
                        GtkDirectionType direction)
{
    GtkWidget *widget = nb->priv->action_widgets[n];

    if (!nb->priv->tabs_visible || !widget || !GTK_WIDGET_VISIBLE (widget))
        return FALSE;

    return gtk_widget_child_focus (widget, direction);
}

static void
labels_invalidate (MooNotebook *nb)
{
    GdkRectangle rect;
    GtkWidget *widget = GTK_WIDGET(nb);
    int border_width = get_border_width (nb);

    if (!GTK_WIDGET_MAPPED (nb))
        return;

    rect.x = 0;
    rect.y = 0;
    rect.width = nb->priv->labels_visible_width;
    rect.height = nb->priv->tabs_height;
    gdk_window_invalidate_rect (nb->priv->tab_window, &rect, TRUE);

    rect.x = widget->allocation.x + border_width;
    rect.y = widget->allocation.y + border_width + nb->priv->tabs_height;
    rect.width = widget->allocation.width - 2*border_width;
    rect.height = 2*widget->style->ythickness;
    gdk_window_invalidate_rect (widget->window, &rect, FALSE);
}

static gboolean
focus_to_next_label (MooNotebook      *nb,
                     GtkDirectionType  direction,
                     gboolean          forward)
{
    Page *page, *next;
    GSList *visible;

    g_return_val_if_fail (nb->priv->focus == FOCUS_LABEL || !nb->priv->focus, FALSE);
    g_return_val_if_fail (nb->priv->tabs_visible, FALSE);

    page = nb->priv->focus_page;
    g_return_val_if_fail (page != NULL, FALSE);
    g_return_val_if_fail (GTK_WIDGET_VISIBLE (page->child), FALSE);

    visible = get_visible_pages (nb);
    g_return_val_if_fail (g_slist_find (visible, page) != NULL, FALSE);

    if (forward)
    {
        if (page == g_slist_last(visible)->data)
            return FALSE;
        else
            next = g_slist_nth_data (visible, g_slist_index (visible, page) + 1);
    }
    else
    {
        if (page == visible->data)
            return FALSE;
        else
            next = g_slist_nth_data (visible, g_slist_index (visible, page) - 1);
    }

    g_return_val_if_fail (next != NULL, FALSE);

    if (nb->priv->arrows_gtk)
    {
        moo_notebook_set_current_page (nb, page_index (nb, next));
        g_return_val_if_fail (nb->priv->current_page == next, TRUE);
    }
    else
    {
        labels_move_label_onscreen (nb, next, TRUE);
    }

    if (!gtk_widget_child_focus (next->label->widget, direction))
    {
        nb->priv->focus = FOCUS_NONE;
        nb->priv->focus_page = next;
        gtk_widget_grab_focus (GTK_WIDGET (nb));
        labels_invalidate (nb);
    }

    return TRUE;
}

static gboolean
focus_to_labels (MooNotebook      *nb,
                 GtkDirectionType  direction,
                 gboolean          forward)
{
    Page *page;

    if (!nb->priv->tabs_visible)
        return FALSE;

    if (!nb->priv->current_page)
        return FALSE;

    if (nb->priv->arrows_gtk)
    {
        page = nb->priv->current_page;
    }
    else
    {
        if (forward)
        {
            page = find_label_at_x (nb, 0, FALSE);
        }
        else
        {
            int x = MIN (nb->priv->labels_width, nb->priv->labels_visible_width) - 1;
            page = find_label_at_x (nb, x, FALSE);
        }
    }

    g_return_val_if_fail (page != NULL, FALSE);

    if (!gtk_widget_child_focus (page->label->widget, direction))
    {
        nb->priv->focus = FOCUS_NONE;
        nb->priv->focus_page = page;
        gtk_widget_grab_focus (GTK_WIDGET (nb));
        labels_invalidate (nb);
    }

    return TRUE;
}

static gboolean
focus_to_arrows (MooNotebook     *nb,
                 GtkDirectionType direction)
{
    if (!nb->priv->tabs_visible || !nb->priv->arrows_visible)
        return FALSE;
    return gtk_widget_child_focus (nb->priv->arrows, direction);
}

static gboolean
focus_to_child (MooNotebook     *nb,
                GtkDirectionType direction)
{
    if (!nb->priv->current_page)
        return FALSE;
    return gtk_widget_child_focus (nb->priv->current_page->child, direction);
}

static gboolean
moo_notebook_focus (GtkWidget       *widget,
                    GtkDirectionType direction)
{
    MooNotebook *nb = MOO_NOTEBOOK (widget);
    Page *page;

    switch (nb->priv->focus)
    {
        case FOCUS_NONE:
            if (gtk_widget_is_focus (widget))
            {
                nb->priv->focus = FOCUS_LABEL;
                return moo_notebook_focus (widget, direction);
            }

            switch (direction)
            {
                case GTK_DIR_TAB_FORWARD:
                case GTK_DIR_RIGHT:
                case GTK_DIR_DOWN:
                    return focus_to_action_widget (nb, LEFT, direction) ||
                            focus_to_labels (nb, direction, TRUE) ||
                            focus_to_arrows (nb, direction) ||
                            focus_to_action_widget (nb, RIGHT, direction) ||
                            focus_to_child (nb, direction);

                case GTK_DIR_TAB_BACKWARD:
                case GTK_DIR_LEFT:
                    return focus_to_action_widget (nb, RIGHT, direction) ||
                            focus_to_arrows (nb, direction) ||
                            focus_to_labels (nb, direction, FALSE) ||
                            focus_to_action_widget (nb, LEFT, direction) ||
                            focus_to_child (nb, direction);

                case GTK_DIR_UP:
                    return focus_to_child (nb, direction) ||
                            focus_to_action_widget (nb, LEFT, direction) ||
                            focus_to_labels (nb, direction, TRUE) ||
                            focus_to_arrows (nb, direction) ||
                            focus_to_action_widget (nb, RIGHT, direction);
            }
            break;

        case FOCUS_LEFT:
            if (gtk_widget_child_focus (nb->priv->action_widgets[LEFT], direction))
                return TRUE;

            switch (direction)
            {
                case GTK_DIR_TAB_FORWARD:
                    return focus_to_labels (nb, direction, TRUE) ||
                            focus_to_arrows (nb, direction) ||
                            focus_to_action_widget (nb, RIGHT, direction) ||
                            focus_to_child (nb, direction);

                case GTK_DIR_RIGHT:
                    return focus_to_labels (nb, direction, TRUE) ||
                            focus_to_arrows (nb, direction) ||
                            focus_to_action_widget (nb, RIGHT, direction);

                case GTK_DIR_TAB_BACKWARD:
                case GTK_DIR_UP:
                case GTK_DIR_LEFT:
                    return FALSE;

                case GTK_DIR_DOWN:
                    return focus_to_child (nb, direction);
            }
            break;

        case FOCUS_RIGHT:
            if (gtk_widget_child_focus (nb->priv->action_widgets[RIGHT], direction))
                return TRUE;

            switch (direction)
            {
                case GTK_DIR_TAB_FORWARD:
                case GTK_DIR_RIGHT:
                case GTK_DIR_UP:
                    return FALSE;

                case GTK_DIR_TAB_BACKWARD:
                case GTK_DIR_LEFT:
                    return focus_to_arrows (nb, direction) ||
                            focus_to_labels (nb, direction, FALSE) ||
                            focus_to_action_widget (nb, LEFT, direction);

                case GTK_DIR_DOWN:
                    return focus_to_child (nb, direction);
            }
            break;

        case FOCUS_ARROWS:
            if (gtk_widget_child_focus (nb->priv->arrows, direction))
                return TRUE;

            switch (direction)
            {
                case GTK_DIR_TAB_FORWARD:
                case GTK_DIR_RIGHT:
                    return focus_to_action_widget (nb, RIGHT, direction);

                case GTK_DIR_UP:
                    return FALSE;

                case GTK_DIR_TAB_BACKWARD:
                case GTK_DIR_LEFT:
                    return focus_to_labels (nb, direction, FALSE) ||
                            focus_to_action_widget (nb, LEFT, direction);

                case GTK_DIR_DOWN:
                    return focus_to_child (nb, direction);
            }
            break;

        case FOCUS_LABEL:
            if (nb->priv->focus_page &&
                GTK_WIDGET_VISIBLE (nb->priv->focus_page->child) &&
                gtk_widget_child_focus (nb->priv->focus_page->label->widget, direction))
                    return TRUE;

            switch (direction)
            {
                case GTK_DIR_RIGHT:
                    return focus_to_next_label (nb, direction, TRUE) ||
                            focus_to_arrows (nb, direction) ||
                            focus_to_action_widget (nb, RIGHT, direction);

                case GTK_DIR_LEFT:
                    return focus_to_next_label (nb, direction, FALSE) ||
                            focus_to_action_widget (nb, LEFT, direction);

                case GTK_DIR_TAB_FORWARD:
                    return focus_to_arrows (nb, direction) ||
                            focus_to_action_widget (nb, RIGHT, direction);

                case GTK_DIR_UP:
                    return FALSE;

                case GTK_DIR_TAB_BACKWARD:
                    return focus_to_action_widget (nb, LEFT, direction);

                case GTK_DIR_DOWN:
                    return focus_to_child (nb, direction);
            }
            break;

        case FOCUS_CHILD:
            page = nb->priv->current_page;
            if (gtk_widget_child_focus (page->child, direction))
                return TRUE;

            switch (direction)
            {
                case GTK_DIR_RIGHT:
                case GTK_DIR_LEFT:
                case GTK_DIR_DOWN:
                case GTK_DIR_TAB_FORWARD:
                    return FALSE;

                case GTK_DIR_UP:
                case GTK_DIR_TAB_BACKWARD:
                    return focus_to_action_widget (nb, LEFT, direction) ||
                            focus_to_labels (nb, direction, TRUE) ||
                            focus_to_arrows (nb, direction) ||
                            focus_to_action_widget (nb, RIGHT, direction);
            }
            break;
    }

    g_return_val_if_reached (FALSE);
}


static void
moo_notebook_set_focus_child (GtkContainer *container,
                              GtkWidget    *child)
{
    Page *page;
    GtkWidget *focus_child, *window;
    MooNotebook *nb = MOO_NOTEBOOK (container);

    window = gtk_widget_get_toplevel (GTK_WIDGET (nb));

    if (GTK_IS_WINDOW (window))
    {
        focus_child = gtk_window_get_focus (GTK_WINDOW (window));

        if (focus_child && (page = find_grand_child (nb, focus_child)))
        {
            if (page->focus_child != focus_child)
            {
                if (page->focus_child)
                    g_object_weak_unref (G_OBJECT (page->focus_child),
                                         (GWeakNotify) g_nullify_pointer,
                                         &page->focus_child);

                page->focus_child = focus_child;

                g_object_weak_ref (G_OBJECT (page->focus_child),
                                   (GWeakNotify) g_nullify_pointer,
                                   &page->focus_child);
            }
        }
    }

    if (child)
    {
        if (nb->priv->focus_page)
            labels_invalidate (nb);
        nb->priv->focus_page = NULL;
    }

    if (!child)
    {
        nb->priv->focus = FOCUS_NONE;
    }
    else if ((page = find_child (nb, child)))
    {
        nb->priv->focus = FOCUS_CHILD;

        if (page != nb->priv->current_page)
            moo_notebook_set_current_page (nb, page_index (nb, page));
    }
    else if ((page = find_label (nb, child)))
    {
        nb->priv->focus = FOCUS_LABEL;
        nb->priv->focus_page = page;

        if (nb->priv->arrows_gtk && page != nb->priv->current_page)
            moo_notebook_set_current_page (nb, page_index (nb, page));
        else if (!nb->priv->arrows_gtk)
            labels_move_label_onscreen (nb, page, TRUE);
    }
    else if (child == nb->priv->action_widgets[LEFT])
    {
        nb->priv->focus = FOCUS_LEFT;
    }
    else if (child == nb->priv->action_widgets[RIGHT])
    {
        nb->priv->focus = FOCUS_RIGHT;
    }
    else if (child == nb->priv->arrows)
    {
        nb->priv->focus = FOCUS_ARROWS;
    }

    GTK_CONTAINER_CLASS(moo_notebook_grand_parent_class)->set_focus_child (container, child);
}
