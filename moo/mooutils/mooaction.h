/*
 *   mooaction.h
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

#ifndef MOO_ACTION_H
#define MOO_ACTION_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_ACTION                     (moo_action_get_type ())
#define MOO_ACTION(object)                  (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_ACTION, MooAction))
#define MOO_ACTION_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_ACTION, MooActionClass))
#define MOO_IS_ACTION(object)               (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_ACTION))
#define MOO_IS_ACTION_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_ACTION))
#define MOO_ACTION_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_ACTION, MooActionClass))

typedef struct _MooAction        MooAction;
typedef struct _MooActionPrivate MooActionPrivate;
typedef struct _MooActionClass   MooActionClass;

struct _MooAction {
    GtkAction base;
    MooActionPrivate *priv;
};

struct _MooActionClass {
    GtkActionClass base_class;
};


#define MOO_TYPE_TOGGLE_ACTION                     (moo_toggle_action_get_type ())
#define MOO_TOGGLE_ACTION(object)                  (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_TOGGLE_ACTION, MooToggleAction))
#define MOO_TOGGLE_ACTION_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_TOGGLE_ACTION, MooToggleActionClass))
#define MOO_IS_TOGGLE_ACTION(object)               (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_TOGGLE_ACTION))
#define MOO_IS_TOGGLE_ACTION_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_TOGGLE_ACTION))
#define MOO_TOGGLE_ACTION_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_TOGGLE_ACTION, MooToggleActionClass))

typedef struct _MooToggleAction        MooToggleAction;
typedef struct _MooToggleActionPrivate MooToggleActionPrivate;
typedef struct _MooToggleActionClass   MooToggleActionClass;

struct _MooToggleAction {
    GtkToggleAction base;
    MooToggleActionPrivate *priv;
};

struct _MooToggleActionClass {
    GtkToggleActionClass base_class;
};


#define MOO_TYPE_RADIO_ACTION                     (moo_radio_action_get_type ())
#define MOO_RADIO_ACTION(object)                  (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_RADIO_ACTION, MooRadioAction))
#define MOO_RADIO_ACTION_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_RADIO_ACTION, MooRadioActionClass))
#define MOO_IS_RADIO_ACTION(object)               (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_RADIO_ACTION))
#define MOO_IS_RADIO_ACTION_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_RADIO_ACTION))
#define MOO_RADIO_ACTION_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_RADIO_ACTION, MooRadioActionClass))

typedef struct _MooRadioAction        MooRadioAction;
typedef struct _MooRadioActionClass   MooRadioActionClass;

struct _MooRadioAction {
    GtkRadioAction base;
};

struct _MooRadioActionClass {
    GtkRadioActionClass base_class;
};


GType           moo_action_get_type         (void) G_GNUC_CONST;
GType           moo_toggle_action_get_type  (void) G_GNUC_CONST;
GType           moo_radio_action_get_type   (void) G_GNUC_CONST;

gpointer       _moo_action_get_window       (gpointer   action);


G_END_DECLS

#endif /* MOO_ACTION_H */
