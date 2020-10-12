/**
 * class:MooEditView: (parent MooTextView) (moo.doc-object-name view): document view object
 **/

#include "mooedit/mooeditview-priv.h"
#include "mooedit/mooedit-impl.h"
#include "mooedit/mooeditwindow-impl.h"
#include "mooedit/mooedittab-impl.h"
#include "mooedit/mooeditor-impl.h"
#include "mooedit/mooeditbookmark.h"
#include "mooedit/mooeditprefs.h"
#include "mooutils/mooutils.h"
#include "mooutils/moocompat.h"

MOO_DEFINE_OBJECT_ARRAY (MooEditView, moo_edit_view)

static void     moo_edit_view_dispose               (GObject            *object);

static gboolean moo_edit_view_focus_in              (GtkWidget          *widget,
                                                     GdkEventFocus      *event);
static gboolean moo_edit_view_popup_menu            (GtkWidget          *widget);
static gboolean moo_edit_view_drag_motion           (GtkWidget          *widget,
                                                     GdkDragContext     *context,
                                                     gint                x,
                                                     gint                y,
                                                     guint               time);
static gboolean moo_edit_view_drag_drop             (GtkWidget          *widget,
                                                     GdkDragContext     *context,
                                                     gint                x,
                                                     gint                y,
                                                     guint               time);
static void     moo_edit_view_apply_style_scheme    (MooTextView        *view,
                                                     MooTextStyleScheme *scheme);
static gboolean moo_edit_view_line_mark_clicked     (MooTextView        *view,
                                                     int                 line);

G_DEFINE_TYPE (MooEditView, moo_edit_view, MOO_TYPE_TEXT_VIEW)

static void
moo_edit_view_class_init (MooEditViewClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    MooTextViewClass *textview_class = MOO_TEXT_VIEW_CLASS (klass);

    gobject_class->dispose = moo_edit_view_dispose;

    widget_class->popup_menu = moo_edit_view_popup_menu;
    widget_class->drag_motion = moo_edit_view_drag_motion;
    widget_class->drag_drop = moo_edit_view_drag_drop;
    widget_class->focus_in_event = moo_edit_view_focus_in;

    textview_class->line_mark_clicked = moo_edit_view_line_mark_clicked;
    textview_class->apply_style_scheme = moo_edit_view_apply_style_scheme;

    g_type_class_add_private (klass, sizeof (MooEditViewPrivate));
}


static void
moo_edit_view_init (MooEditView *view)
{
    view->priv = G_TYPE_INSTANCE_GET_PRIVATE (view, MOO_TYPE_EDIT_VIEW, MooEditViewPrivate);
}

void
_moo_edit_view_unset_doc (MooEditView *view)
{
    g_return_if_fail (MOO_IS_EDIT_VIEW (view));
    view->priv->doc = NULL;
}

static void
moo_edit_view_dispose (GObject *object)
{
    MooEditView *view = MOO_EDIT_VIEW (object);

    if (view->priv->fake_cursor_mark)
    {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
        gtk_text_buffer_delete_mark (buffer, view->priv->fake_cursor_mark);
        view->priv->fake_cursor_mark = NULL;
    }

    if (view->priv->doc)
    {
        _moo_edit_remove_view (view->priv->doc, view);
        g_assert (view->priv->doc == NULL);
        view->priv->doc = NULL;
    }

    G_OBJECT_CLASS (moo_edit_view_parent_class)->dispose (object);
}


MooEditView *
_moo_edit_view_new (MooEdit *doc)
{
    MooEditView *view;
    MooIndenter *indent;

    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);

    view = g::object_new<MooEditView> ("buffer", moo_edit_get_buffer (doc));
    view->priv->doc = doc;
    view->priv->editor = moo_edit_get_editor (doc);

    g_assert (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)) == moo_edit_get_buffer (doc));

    indent = moo_indenter_new (doc);
    moo_text_view_set_indenter (MOO_TEXT_VIEW (view), indent);
    g_object_unref (indent);

    _moo_edit_add_view (doc, view);

    return view;
}


static gboolean
moo_edit_view_line_mark_clicked (MooTextView *view,
                                 int          line)
{
    moo_edit_toggle_bookmark (moo_edit_view_get_doc (MOO_EDIT_VIEW (view)), line);
    return TRUE;
}

static void
moo_edit_view_apply_style_scheme (MooTextView        *view,
                                  MooTextStyleScheme *scheme)
{
    MOO_TEXT_VIEW_CLASS (moo_edit_view_parent_class)->apply_style_scheme (view, scheme);
    _moo_edit_update_bookmarks_style (moo_edit_view_get_doc (MOO_EDIT_VIEW (view)));
}


static gboolean
moo_edit_view_focus_in (GtkWidget     *widget,
                        GdkEventFocus *event)
{
    gboolean retval = FALSE;
    MooEditView *view = MOO_EDIT_VIEW (widget);

    if (GTK_WIDGET_CLASS (moo_edit_view_parent_class)->focus_in_event)
        retval = GTK_WIDGET_CLASS (moo_edit_view_parent_class)->focus_in_event (widget, event);

    _moo_edit_set_active_view (view->priv->doc, view);

    if (view->priv->tab)
        _moo_edit_tab_set_focused_view (view->priv->tab, view);

    return retval;
}


/**
 * moo_edit_view_get_doc:
 **/
MooEdit *
moo_edit_view_get_doc (MooEditView *view)
{
    g_return_val_if_fail (MOO_IS_EDIT_VIEW (view), NULL);
    return view->priv->doc;
}

/**
 * moo_edit_view_get_editor:
 **/
MooEditor *
moo_edit_view_get_editor (MooEditView *view)
{
    g_return_val_if_fail (MOO_IS_EDIT_VIEW (view), NULL);
    return view->priv->editor;
}

/**
 * moo_edit_view_get_tab:
 **/
MooEditTab *
moo_edit_view_get_tab (MooEditView *view)
{
    g_return_val_if_fail (MOO_IS_EDIT_VIEW (view), NULL);
    return view->priv->tab;
}

void
_moo_edit_view_set_tab (MooEditView *view,
                        MooEditTab  *tab)
{
    g_return_if_fail (MOO_IS_EDIT_VIEW (view));
    g_return_if_fail (MOO_IS_EDIT_TAB (tab));
    g_return_if_fail (view->priv->tab == NULL);
    view->priv->tab = tab;
}

/**
 * moo_edit_view_get_window:
 **/
MooEditWindow *
moo_edit_view_get_window (MooEditView *view)
{
    GtkWidget *toplevel;

    g_return_val_if_fail (MOO_IS_EDIT_VIEW (view), NULL);

    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));

    if (MOO_IS_EDIT_WINDOW (toplevel))
        return MOO_EDIT_WINDOW (toplevel);
    else
        return NULL;
}


GtkTextMark *
_moo_edit_view_get_fake_cursor_mark (MooEditView *view)
{
    g_return_val_if_fail (MOO_IS_EDIT_VIEW (view), NULL);

    if (!view->priv->fake_cursor_mark)
    {
        GtkTextIter iter;
        GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
        gtk_text_buffer_get_start_iter (buffer, &iter);
        view->priv->fake_cursor_mark = gtk_text_buffer_create_mark (buffer, NULL, &iter, FALSE);
    }

    return view->priv->fake_cursor_mark;
}


void
_moo_edit_view_apply_config (MooEditView *view)
{
    GtkWrapMode wrap_mode;
    gboolean line_numbers;
    guint tab_width;
    char *word_chars;

    g_return_if_fail (MOO_IS_EDIT_VIEW (view));
    g_return_if_fail (view->priv->doc && view->priv->doc->config);

    moo_edit_config_get (view->priv->doc->config,
                         "wrap-mode", &wrap_mode,
                         "show-line-numbers", &line_numbers,
                         "tab-width", &tab_width,
                         "word-chars", &word_chars,
                         (char*) 0);

    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), wrap_mode);
    moo_text_view_set_show_line_numbers (MOO_TEXT_VIEW (view), line_numbers);
    moo_text_view_set_tab_width (MOO_TEXT_VIEW (view), tab_width);
    moo_text_view_set_word_chars (MOO_TEXT_VIEW (view), word_chars);

    gtk_widget_queue_draw (GTK_WIDGET (view));

    g_free (word_chars);
}


static gboolean
find_uri_atom (GdkDragContext *context)
{
    GList *targets;
    GdkAtom atom;

    atom = moo_atom_uri_list ();
    targets = context->targets;

    while (targets)
    {
        if (targets->data == GUINT_TO_POINTER (atom))
            return TRUE;
        targets = targets->next;
    }

    return FALSE;
}

static gboolean
moo_edit_view_drag_motion (GtkWidget      *widget,
                           GdkDragContext *context,
                           gint            x,
                           gint            y,
                           guint           time)
{
    if (find_uri_atom (context))
        return FALSE;

    return GTK_WIDGET_CLASS (moo_edit_view_parent_class)->drag_motion (widget, context, x, y, time);
}

static gboolean
moo_edit_view_drag_drop (GtkWidget      *widget,
                         GdkDragContext *context,
                         gint            x,
                         gint            y,
                         guint           time)
{
    if (find_uri_atom (context))
        return FALSE;

    return GTK_WIDGET_CLASS (moo_edit_view_parent_class)->drag_drop (widget, context, x, y, time);
}


/*****************************************************************************/
/* popup menu
 */

/* gtktextview.c */
static void
popup_position_func (GtkMenu   *menu,
                     gint      *x,
                     gint      *y,
                     gboolean  *push_in,
                     gpointer   user_data)
{
    GtkTextView *text_view;
    GtkWidget *widget;
    GdkRectangle cursor_rect;
    GdkRectangle onscreen_rect;
    gint root_x, root_y;
    GtkTextIter iter;
    GtkRequisition req;
    GdkScreen *screen;
    gint monitor_num;
    GdkRectangle monitor;

    text_view = GTK_TEXT_VIEW (user_data);
    widget = GTK_WIDGET (text_view);

    g_return_if_fail (GTK_WIDGET_REALIZED (text_view));

    screen = gtk_widget_get_screen (widget);

    gdk_window_get_origin (widget->window, &root_x, &root_y);

    gtk_text_buffer_get_iter_at_mark (gtk_text_view_get_buffer (text_view),
                                      &iter,
                                      gtk_text_buffer_get_insert (gtk_text_view_get_buffer (text_view)));

    gtk_text_view_get_iter_location (text_view,
                                     &iter,
                                     &cursor_rect);

    gtk_text_view_get_visible_rect (text_view, &onscreen_rect);

    gtk_widget_size_request (GTK_WIDGET (menu), &req);

    /* can't use rectangle_intersect since cursor rect can have 0 width */
    if (cursor_rect.x >= onscreen_rect.x &&
        cursor_rect.x < onscreen_rect.x + onscreen_rect.width &&
        cursor_rect.y >= onscreen_rect.y &&
        cursor_rect.y < onscreen_rect.y + onscreen_rect.height)
    {
        gtk_text_view_buffer_to_window_coords (text_view,
                                               GTK_TEXT_WINDOW_WIDGET,
                                               cursor_rect.x, cursor_rect.y,
                                               &cursor_rect.x, &cursor_rect.y);

        *x = root_x + cursor_rect.x + cursor_rect.width;
        *y = root_y + cursor_rect.y + cursor_rect.height;
    }
    else
    {
        /* Just center the menu, since cursor is offscreen. */
        *x = root_x + (widget->allocation.width / 2 - req.width / 2);
        *y = root_y + (widget->allocation.height / 2 - req.height / 2);
    }

    /* Ensure sanity */
    *x = CLAMP (*x, root_x, (root_x + widget->allocation.width));
    *y = CLAMP (*y, root_y, (root_y + widget->allocation.height));

    monitor_num = gdk_screen_get_monitor_at_point (screen, *x, *y);
    gtk_menu_set_monitor (menu, monitor_num);
    gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

    *x = CLAMP (*x, monitor.x, monitor.x + MAX (0, monitor.width - req.width));
    *y = CLAMP (*y, monitor.y, monitor.y + MAX (0, monitor.height - req.height));

    *push_in = FALSE;
}

void
_moo_edit_view_do_popup (MooEditView    *view,
                         GdkEventButton *event)
{
    MooUiXml *xml;
    MooEditWindow *window;
    GtkMenu *menu;

    window = moo_edit_view_get_window (view);
    xml = moo_editor_get_doc_ui_xml (view->priv->editor);
    g_return_if_fail (xml != NULL);

    _moo_edit_check_actions (view->priv->doc, view);

    menu = (GtkMenu*) moo_ui_xml_create_widget (xml, MOO_UI_MENU, "Editor/Popup",
                                                _moo_edit_get_actions (view->priv->doc),
                                                window ? MOO_WINDOW(window)->accel_group : NULL);
    g_return_if_fail (menu != NULL);
    g_object_ref_sink (menu);

    if (event)
    {
        gtk_menu_popup (menu, NULL, NULL, NULL, NULL,
                        event->button, event->time);
    }
    else
    {
        gtk_menu_popup (menu, NULL, NULL,
                        popup_position_func, view,
                        0, gtk_get_current_event_time ());
        gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), FALSE);
    }

    g_object_unref (menu);
}

static gboolean
moo_edit_view_popup_menu (GtkWidget *widget)
{
    _moo_edit_view_do_popup (MOO_EDIT_VIEW (widget), NULL);
    return TRUE;
}


void
_moo_edit_view_ui_set_line_wrap (MooEditView *view,
                                 gboolean     enabled)
{
    GtkWrapMode mode;
    gboolean old_enabled;

    g_return_if_fail (MOO_IS_EDIT_VIEW (view));
    g_return_if_fail (view->priv->doc && view->priv->doc->config);

    g_object_get (view, "wrap-mode", &mode, nullptr);

    enabled = enabled != 0;
    old_enabled = mode != GTK_WRAP_NONE;

    if (enabled == old_enabled)
        return;

    if (!enabled)
        mode = GTK_WRAP_NONE;
    else if (moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_WRAP_WORDS)))
        mode = GTK_WRAP_WORD;
    else
        mode = GTK_WRAP_CHAR;

    moo_edit_config_set (view->priv->doc->config,
                         MOO_EDIT_CONFIG_SOURCE_USER,
                         "wrap-mode", mode, nullptr);
}

void
_moo_edit_view_ui_set_show_line_numbers (MooEditView *view,
                                         gboolean     show)
{
    gboolean old_show;

    g_return_if_fail (MOO_IS_EDIT_VIEW (view));
    g_return_if_fail (view->priv->doc && view->priv->doc->config);

    g_object_get (view, "show-line-numbers", &old_show, nullptr);

    if (!old_show == !show)
        return;

    moo_edit_config_set (view->priv->doc->config,
                         MOO_EDIT_CONFIG_SOURCE_USER,
                         "show-line-numbers", show,
                         (char*) NULL);
}
