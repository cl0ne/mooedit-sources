/*
 *   moodialogs.c
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

#include <gtk/gtk.h>
#include "mooutils/moodialogs.h"
#include "mooutils/mooprefs.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooi18n.h"
#include "mooutils/moocompat.h"

static GtkWidget *
create_message_dialog (GtkWindow  *parent,
                       GtkMessageType type,
                       GtkButtonsType buttons,
                       const char *text,
                       const char *secondary_text,
                       GtkResponseType default_response)
{
    GtkWidget *dialog;
    GtkButtonsType message_dialog_buttons = buttons;

    if (buttons == GTK_BUTTONS_CLOSE || buttons == GTK_BUTTONS_OK)
        message_dialog_buttons = GTK_BUTTONS_NONE;

    dialog = gtk_message_dialog_new_with_markup (parent,
                                                 GTK_DIALOG_MODAL,
                                                 type,
                                                 message_dialog_buttons,
                                                 "<span weight=\"bold\" size=\"larger\">%s</span>", text);
    if (secondary_text)
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                  "%s", secondary_text);

    if (buttons == GTK_BUTTONS_CLOSE || buttons == GTK_BUTTONS_OK)
    {
        gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                                buttons == GTK_BUTTONS_CLOSE ? GTK_STOCK_CLOSE : GTK_STOCK_OK,
                                GTK_RESPONSE_CANCEL,
                                NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_CANCEL);
    }

    if (default_response != GTK_RESPONSE_NONE)
        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         default_response);

    if (parent && parent->group)
        gtk_window_group_add_window (parent->group, GTK_WINDOW (dialog));

    return dialog;
}


/* gtkwindow.c */
static void
clamp_window_to_rectangle (gint               *x,
                           gint               *y,
                           gint                w,
                           gint                h,
                           const GdkRectangle *rect)
{
    /* if larger than the screen, center on the screen. */
    if (w > rect->width)
        *x = rect->x - (w - rect->width) / 2;
    else if (*x < rect->x)
        *x = rect->x;
    else if (*x + w > rect->x + rect->width)
        *x = rect->x + rect->width - w;

    if (h > rect->height)
        *y = rect->y - (h - rect->height) / 2;
    else if (*y < rect->y)
        *y = rect->y;
    else if (*y + h > rect->y + rect->height)
        *y = rect->y + rect->height - h;
}


static void
position_window (GtkWindow *dialog)
{
    GdkPoint *coord;
    int screen_width, screen_height, monitor_num;
    GdkRectangle monitor;
    GdkScreen *screen;
    GtkRequisition req;

    g_signal_handlers_disconnect_by_func (dialog,
                                          (gpointer) position_window,
                                          NULL);

    coord = (GdkPoint*) g_object_get_data (G_OBJECT (dialog), "moo-coords");
    g_return_if_fail (coord != NULL);

    screen = gtk_widget_get_screen (GTK_WIDGET (dialog));
    g_return_if_fail (screen != NULL);

    screen_width = gdk_screen_get_width (screen);
    screen_height = gdk_screen_get_height (screen);
    monitor_num = gdk_screen_get_monitor_at_point (screen, coord->x, coord->y);

    gtk_widget_size_request (GTK_WIDGET (dialog), &req);

    coord->x = coord->x - req.width / 2;
    coord->y = coord->y - req.height / 2;
    coord->x = CLAMP (coord->x, 0, screen_width - req.width);
    coord->y = CLAMP (coord->y, 0, screen_height - req.height);

    gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);
    clamp_window_to_rectangle (&coord->x, &coord->y, req.width, req.height, &monitor);

    gtk_window_move (dialog, coord->x, coord->y);
}


#ifdef __WIN32__
static void
on_hide (GtkWindow *window)
{
    GtkWindow *parent = gtk_window_get_transient_for (window);

    if (parent && GTK_WIDGET_DRAWABLE (parent))
        gtk_window_present (parent);

    g_signal_handlers_disconnect_by_func (window, (gpointer) on_hide, NULL);
}
#endif


static void
moo_position_window_real (GtkWidget  *window,
                          GtkWidget  *parent,
                          gboolean    at_mouse,
                          gboolean    at_coords,
                          int         x,
                          int         y)
{
    GtkWidget *toplevel = NULL;

    g_return_if_fail (GTK_IS_WINDOW (window));

    if (parent)
        toplevel = gtk_widget_get_toplevel (parent);
    if (toplevel && !GTK_IS_WINDOW (toplevel))
        toplevel = NULL;

    if (toplevel)
    {
        gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (toplevel));
#ifdef __WIN32__
        g_signal_handlers_disconnect_by_func (window, (gpointer) on_hide, NULL);
        g_signal_connect (window, "unmap", G_CALLBACK (on_hide), NULL);
#endif
    }

    if (toplevel && GTK_WINDOW(toplevel)->group)
        gtk_window_group_add_window (GTK_WINDOW(toplevel)->group, GTK_WINDOW (window));

    if (!at_mouse && !at_coords && parent && GTK_WIDGET_REALIZED (parent))
    {
        if (GTK_IS_WINDOW (parent))
        {
            gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ON_PARENT);
        }
        else
        {
            GdkWindow *parent_window = gtk_widget_get_parent_window (parent);
            gdk_window_get_origin (parent_window, &x, &y);
            x += parent->allocation.x;
            y += parent->allocation.y;
            x += parent->allocation.width / 2;
            y += parent->allocation.height / 2;
            at_coords = TRUE;
        }
    }

    if (at_mouse)
    {
        gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
    }
    else if (at_coords)
    {
        GdkPoint *coord = g_new (GdkPoint, 1);
        coord->x = x;
        coord->y = y;
        gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_NONE);
        g_object_set_data_full (G_OBJECT (window), "moo-coords", coord, g_free);

        if (!GTK_WIDGET_REALIZED (window))
        {
            g_signal_handlers_disconnect_by_func (window, (gpointer) position_window, NULL);
            g_signal_connect (window, "realize",
                              G_CALLBACK (position_window),
                              NULL);
        }
        else
        {
            position_window (GTK_WINDOW (window));
        }
    }
}


void
moo_position_window_at_pointer (GtkWidget  *window,
                                GtkWidget  *parent)
{
    moo_position_window_real (window, parent, TRUE, FALSE, 0, 0);
}


void
moo_window_set_parent (GtkWidget  *window,
                       GtkWidget  *parent)
{
    moo_position_window_real (window, parent, FALSE, FALSE, 0, 0);
}


static GtkResponseType
moo_message_dialog (GtkWidget  *parent,
                    GtkMessageType type,
                    GtkButtonsType buttons,
                    const char *text,
                    const char *secondary_text,
                    GtkResponseType default_response)
{
    GtkWidget *dialog, *toplevel = NULL;
    GtkResponseType response;

    if (parent)
        toplevel = gtk_widget_get_toplevel (parent);
    if (!toplevel || !GTK_IS_WINDOW (toplevel))
        toplevel = NULL;

    dialog = create_message_dialog (toplevel ? GTK_WINDOW (toplevel) : NULL,
                                    type, buttons, text, secondary_text, default_response);
    g_return_val_if_fail (dialog != NULL, GTK_RESPONSE_NONE);

    moo_position_window_real (dialog, parent, FALSE, FALSE, 0, 0);

    response = gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_destroy (dialog);
    return response;
}


/**
 * moo_question_dialog:
 *
 * @text: (type const-utf8)
 * @secondary_text: (type const-utf8) (allow-none) (default NULL)
 * @parent: (allow-none) (default NULL)
 * @default_response: (default GTK_RESPONSE_OK)
 **/
gboolean
moo_question_dialog (const char *text,
                     const char *secondary_text,
                     GtkWidget  *parent,
                     GtkResponseType default_response)
{
    return moo_message_dialog (parent,
                               GTK_MESSAGE_QUESTION,
                               GTK_BUTTONS_OK_CANCEL,
                               text, secondary_text,
                               default_response) == GTK_RESPONSE_OK;
}

/**
 * moo_error_dialog:
 *
 * @text: (type const-utf8)
 * @secondary_text: (type const-utf8) (allow-none) (default NULL)
 * @parent: (allow-none) (default NULL)
 **/
void
moo_error_dialog (const char *text,
                  const char *secondary_text,
                  GtkWidget  *parent)
{
    moo_message_dialog (parent,
                        GTK_MESSAGE_ERROR,
                        GTK_BUTTONS_CLOSE,
                        text, secondary_text,
                        GTK_RESPONSE_NONE);
}

/**
 * moo_info_dialog:
 *
 * @text: (type const-utf8)
 * @secondary_text: (type const-utf8) (allow-none) (default NULL)
 * @parent: (allow-none) (default NULL)
 **/
void
moo_info_dialog (const char *text,
                 const char *secondary_text,
                 GtkWidget  *parent)
{
    moo_message_dialog (parent,
                        GTK_MESSAGE_INFO,
                        GTK_BUTTONS_CLOSE,
                        text, secondary_text,
                        GTK_RESPONSE_NONE);
}

/**
 * moo_warning_dialog:
 *
 * @text: (type const-utf8)
 * @secondary_text: (type const-utf8) (allow-none) (default NULL)
 * @parent: (allow-none) (default NULL)
 **/
void
moo_warning_dialog (const char *text,
                    const char *secondary_text,
                    GtkWidget  *parent)
{
    moo_message_dialog (parent,
                        GTK_MESSAGE_WARNING,
                        GTK_BUTTONS_CLOSE,
                        text, secondary_text,
                        GTK_RESPONSE_NONE);
}


/**
 * moo_overwrite_file_dialog:
 *
 * @display_name: (type const-utf8)
 * @display_dirname: (type const-utf8)
 * @parent: (allow-none) (default NULL)
 **/
gboolean
moo_overwrite_file_dialog (const char *display_name,
                           const char *display_dirname,
                           GtkWidget  *parent)
{
    int response;
    GtkWidget *dialog, *button, *toplevel = NULL;

    g_return_val_if_fail (display_name != NULL, FALSE);
    g_return_val_if_fail (display_dirname != NULL, FALSE);

    if (parent)
        parent = gtk_widget_get_toplevel (parent);

    dialog = gtk_message_dialog_new (toplevel ? GTK_WINDOW (toplevel) : NULL,
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_WARNING,
                                     GTK_BUTTONS_NONE,
                                     "A file named \"%s\" already exists.  Do you want to replace it?",
                                     display_name);

    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              "The file already exists in \"%s\".  Replacing it will "
                                              "overwrite its contents.",
                                              display_dirname);

    gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

    button = gtk_button_new_with_mnemonic ("_Replace");
    gtk_button_set_image (GTK_BUTTON (button),
                          gtk_image_new_from_stock (GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_BUTTON));
    gtk_widget_show (button);
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_YES);

    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_YES,
                                             GTK_RESPONSE_CANCEL,
                                             -1);

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

    moo_window_set_parent (dialog, parent);

    response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    return response == GTK_RESPONSE_YES;
}

/**
 * moo_save_changes_dialog:
 *
 * @display_name: (type const-utf8)
 * @parent: (allow-none) (default NULL)
 **/
MooSaveChangesResponse
moo_save_changes_dialog (const char *display_name,
                         GtkWidget  *parent)
{
    GtkDialog *dialog = NULL;
    int response;

    if (parent)
        parent = gtk_widget_get_toplevel (parent);

    dialog = GTK_DIALOG (gtk_message_dialog_new (
        parent ? GTK_WINDOW (parent) : NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_NONE,
        display_name ?
            _("Save changes to document \"%s\" before closing?") :
            _("Save changes to the document before closing?"),
        display_name));

    gtk_message_dialog_format_secondary_text (
        GTK_MESSAGE_DIALOG (dialog),
        _("If you don't save, changes will be discarded"));

    gtk_dialog_add_buttons (dialog,
        GTK_STOCK_DISCARD, GTK_RESPONSE_NO,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_YES,
        NULL);

    gtk_dialog_set_alternative_button_order (dialog,
        GTK_RESPONSE_YES, GTK_RESPONSE_NO, GTK_RESPONSE_CANCEL, -1);

    gtk_dialog_set_default_response (dialog, GTK_RESPONSE_YES);

    if (parent)
        moo_window_set_parent (GTK_WIDGET (dialog), parent);

    response = gtk_dialog_run (dialog);
    if (response == GTK_RESPONSE_DELETE_EVENT)
        response = GTK_RESPONSE_CANCEL;
    gtk_widget_destroy (GTK_WIDGET (dialog));

    switch (response)
    {
        case GTK_RESPONSE_NO:
            return MOO_SAVE_CHANGES_RESPONSE_DONT_SAVE;
        case GTK_RESPONSE_CANCEL:
            return MOO_SAVE_CHANGES_RESPONSE_CANCEL;
        case GTK_RESPONSE_YES:
            return MOO_SAVE_CHANGES_RESPONSE_SAVE;
    }

    g_return_val_if_reached (MOO_SAVE_CHANGES_RESPONSE_CANCEL);
}


typedef struct {
    char *key_maximized;
    char *key_width;
    char *key_height;
    char *key_x;
    char *key_y;
    gboolean remember_position;
    guint save_size_idle;
} PositionInfo;

static void
position_info_free (PositionInfo *pinfo)
{
    if (pinfo)
    {
        if (pinfo->save_size_idle)
            g_source_remove (pinfo->save_size_idle);
        g_free (pinfo->key_maximized);
        g_free (pinfo->key_width);
        g_free (pinfo->key_height);
        g_free (pinfo->key_x);
        g_free (pinfo->key_y);
        g_free (pinfo);
    }
}

static gboolean
save_size (GtkWindow *window)
{
    PositionInfo *pinfo;
    GdkWindowState state;

    pinfo = (PositionInfo*) g_object_get_data (G_OBJECT (window), "moo-position-info");
    g_return_val_if_fail (pinfo != NULL, FALSE);

    pinfo->save_size_idle = 0;

    if (!GTK_WIDGET_REALIZED (window))
        return FALSE;

    state = gdk_window_get_state (GTK_WIDGET(window)->window);
    moo_prefs_set_bool (pinfo->key_maximized,
                        state & GDK_WINDOW_STATE_MAXIMIZED);

    if (!(state & GDK_WINDOW_STATE_MAXIMIZED))
    {
        int width, height;
        gtk_window_get_size (GTK_WINDOW (window), &width, &height);
        moo_prefs_set_int (pinfo->key_width, width);
        moo_prefs_set_int (pinfo->key_height, height);
    }

    if (pinfo->remember_position)
    {
        int x, y;
        gtk_window_get_position (window, &x, &y);
        moo_prefs_set_int (pinfo->key_x, x);
        moo_prefs_set_int (pinfo->key_y, y);
    }

    return FALSE;
}

static gboolean
configure_event (GtkWindow *window,
                 G_GNUC_UNUSED GdkEventConfigure *event,
                 PositionInfo *pinfo)
{
    if (!pinfo->save_size_idle)
        pinfo->save_size_idle = g_idle_add ((GSourceFunc) save_size, window);
    return FALSE;
}

static void
set_initial_size (GtkWindow    *window,
                  PositionInfo *pinfo)
{
    int width, height;

    width = moo_prefs_get_int (pinfo->key_width);
    height = moo_prefs_get_int (pinfo->key_height);

    if (width > 0 && height > 0)
        gtk_window_set_default_size (window, width, height);

    if (moo_prefs_get_bool (pinfo->key_maximized))
        gtk_window_maximize (window);

    if (pinfo->remember_position)
    {
        int x, y;

        x = moo_prefs_get_int (pinfo->key_x);
        y = moo_prefs_get_int (pinfo->key_y);

        if (x < G_MAXINT && y < G_MAXINT)
            gtk_window_move (window, x, y);
    }
}


void
_moo_window_set_remember_size (GtkWindow  *window,
                               const char *prefs_key,
                               int         default_width,
                               int         default_height,
                               gboolean    remember_position)
{
    PositionInfo *pinfo;

    g_return_if_fail (prefs_key != NULL);

    pinfo = (PositionInfo*) g_object_get_data (G_OBJECT (window), "moo-position-info");

    if (pinfo)
        g_signal_handlers_disconnect_by_func (window, (gpointer) configure_event, pinfo);

    pinfo = g_new0 (PositionInfo, 1);
    pinfo->key_maximized = moo_prefs_make_key (prefs_key, "maximized", NULL);
    pinfo->key_width = moo_prefs_make_key (prefs_key, "width", NULL);
    pinfo->key_height = moo_prefs_make_key (prefs_key, "height", NULL);
    pinfo->key_x = moo_prefs_make_key (prefs_key, "x", NULL);
    pinfo->key_y = moo_prefs_make_key (prefs_key, "y", NULL);
    pinfo->remember_position = remember_position;

    moo_prefs_create_key (pinfo->key_maximized, MOO_PREFS_STATE, G_TYPE_BOOLEAN, FALSE);
    moo_prefs_create_key (pinfo->key_width, MOO_PREFS_STATE, G_TYPE_INT, default_width);
    moo_prefs_create_key (pinfo->key_height, MOO_PREFS_STATE, G_TYPE_INT, default_height);

    if (remember_position)
    {
       moo_prefs_create_key (pinfo->key_x, MOO_PREFS_STATE, G_TYPE_INT, G_MAXINT);
       moo_prefs_create_key (pinfo->key_y, MOO_PREFS_STATE, G_TYPE_INT, G_MAXINT);
    }

    g_object_set_data_full (G_OBJECT (window), "moo-position-info",
                            pinfo, (GDestroyNotify) position_info_free);
    set_initial_size (window, pinfo);
    g_signal_connect (window, "configure-event",
                      G_CALLBACK (configure_event), pinfo);
}
