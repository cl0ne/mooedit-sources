/*
 *   moowindow.h
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

#ifndef MOOUI_MOOWINDOW_H
#define MOOUI_MOOWINDOW_H

#include <mooutils/mooutils-gobject.h>
#include <mooutils/moouixml.h>
#include <mooutils/mooactioncollection.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_WINDOW              (moo_window_get_type ())
#define MOO_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_WINDOW, MooWindow))
#define MOO_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_WINDOW, MooWindowClass))
#define MOO_IS_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_WINDOW))
#define MOO_IS_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_WINDOW))
#define MOO_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_WINDOW, MooWindowClass))

/**
 * enum:MooCloseResponse
 *
 * @MOO_CLOSE_RESPONSE_CONTINUE:
 * @MOO_CLOSE_RESPONSE_CANCEL:
 **/
typedef enum {
    MOO_CLOSE_RESPONSE_CONTINUE = 4,
    MOO_CLOSE_RESPONSE_CANCEL
} MooCloseResponse;

typedef struct MooWindow        MooWindow;
typedef struct _MooWindowPrivate MooWindowPrivate;
typedef struct _MooWindowClass   MooWindowClass;

#ifdef __cplusplus
struct MooWindow : public GtkWindow
{
#else
struct MooWindow
{
    GtkWindow            gtkwindow;
#endif

    GtkAccelGroup       *accel_group;
    MooWindowPrivate    *priv;

    GtkWidget           *menubar;
    GtkWidget           *toolbar;
    GtkWidget           *vbox;
    GtkWidget           *status_area;
    GtkStatusbar        *statusbar;
};

struct _MooWindowClass
{
    GtkWindowClass      parent_class;

    MooCloseResponse (*close) (MooWindow *window);
};

typedef GtkAction *(*MooWindowActionFunc) (MooWindow *window,
                                           gpointer   data);

GType       moo_window_get_type             (void) G_GNUC_CONST;

gboolean    moo_window_close                (MooWindow  *window);
void        moo_window_message              (MooWindow  *window,
                                             const char *text);

void        moo_window_set_title            (MooWindow  *window,
                                             const char *title);

void        moo_window_set_edit_ops_widget  (MooWindow  *window,
                                             GtkWidget  *widget);

/*****************************************************************************/
/* Actions
 */

void        moo_window_class_set_id         (MooWindowClass     *klass,
                                             const char         *id,
                                             const char         *name);

void        moo_window_class_new_action     (MooWindowClass     *klass,
                                             const char         *id,
                                             const char         *group,
                                             ...) G_GNUC_NULL_TERMINATED;
void        moo_window_class_new_action_custom (MooWindowClass  *klass,
                                             const char         *id,
                                             const char         *group,
                                             MooWindowActionFunc func,
                                             gpointer            data,
                                             GDestroyNotify      notify);
void        _moo_window_class_new_action_callback
                                            (MooWindowClass     *klass,
                                             const char         *id,
                                             const char         *group,
                                             GCallback           callback,
                                             GSignalCMarshaller  marshal,
                                             GType               return_type,
                                             guint               n_args,
                                             ...) G_GNUC_NULL_TERMINATED;

gboolean    moo_window_class_find_action    (MooWindowClass     *klass,
                                             const char         *id);
void        moo_window_class_remove_action  (MooWindowClass     *klass,
                                             const char         *id);

void        moo_window_class_new_group      (MooWindowClass     *klass,
                                             const char         *name,
                                             const char         *display_name);
gboolean    moo_window_class_find_group     (MooWindowClass     *klass,
                                             const char         *name);
void        moo_window_class_remove_group   (MooWindowClass     *klass,
                                             const char         *name);

MooUiXml   *moo_window_get_ui_xml           (MooWindow          *window);
void        moo_window_set_ui_xml           (MooWindow          *window,
                                             MooUiXml           *xml);

MooActionCollection *moo_window_get_actions (MooWindow          *window);
GtkAction  *moo_window_get_action           (MooWindow          *window,
                                             const char         *action);

void        moo_window_set_global_accels    (MooWindow          *window,
                                             gboolean            global);

void        moo_window_set_default_geometry (const char         *geometry);


G_END_DECLS

#endif /* MOOUI_MOOWINDOW_H */
