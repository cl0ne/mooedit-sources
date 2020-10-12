/*
 *   mooaccelbutton.c
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

#include "mooutils/mooaccelbutton.h"
#include "marshals.h"
#include "mooutils/moodialogs.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooaccelbutton-gxml.h"
#include "mooutils/mooaccel.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>


enum {
    PROP_0,
    PROP_ACCEL,
    PROP_TITLE
};

enum {
    ACCEL_SET,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};


static void         moo_accel_button_finalize       (GObject        *object);
static void         moo_accel_button_set_property   (GObject        *object,
                                                     guint           param_id,
                                                     const GValue   *value,
                                                     GParamSpec     *pspec);
static void         moo_accel_button_get_property   (GObject        *object,
                                                     guint           param_id,
                                                     GValue         *value,
                                                     GParamSpec     *pspec);

static void         moo_accel_button_clicked        (GtkButton      *button);

static const char  *_moo_accel_button_get_title     (MooAccelButton *button);
static void         _moo_accel_button_set_title     (MooAccelButton *button,
                                                     const char     *title);


G_DEFINE_TYPE (MooAccelButton, _moo_accel_button, GTK_TYPE_BUTTON)


static void
_moo_accel_button_class_init (MooAccelButtonClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->get_property = moo_accel_button_get_property;
    gobject_class->set_property = moo_accel_button_set_property;
    gobject_class->finalize = moo_accel_button_finalize;

    GTK_BUTTON_CLASS (klass)->clicked = moo_accel_button_clicked;

    klass->accel_set = NULL;

    g_object_class_install_property (gobject_class,
                                     PROP_ACCEL,
                                     g_param_spec_string (
                                        "accel",
                                        "accel",
                                        "accel",
                                        NULL,
                                        (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_TITLE,
                                     g_param_spec_string (
                                        "title",
                                        "title",
                                        "title",
                                        "Choose shortcut",
                                        (GParamFlags) G_PARAM_READWRITE));

    signals[ACCEL_SET] = g_signal_new ("accel-set",
                                       G_TYPE_FROM_CLASS (gobject_class),
                                       G_SIGNAL_RUN_LAST,
                                       G_STRUCT_OFFSET (MooAccelButtonClass, accel_set),
                                       NULL, NULL,
                                       _moo_marshal_VOID__STRING,
                                       G_TYPE_NONE, 1,
                                       G_TYPE_STRING);
}


static void
_moo_accel_button_init (MooAccelButton *button)
{
    button->accel = NULL;
    button->title = NULL;
}


static void
moo_accel_button_finalize (GObject *object)
{
    MooAccelButton *button = MOO_ACCEL_BUTTON (object);
    g_free (button->title);
    button->title = NULL;
    g_free (button->accel);
    button->accel = NULL;
    G_OBJECT_CLASS (_moo_accel_button_parent_class)->finalize (object);
}


static void
moo_accel_button_set_property (GObject        *object,
                               guint           param_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
    MooAccelButton *button = MOO_ACCEL_BUTTON (object);

    switch (param_id)
    {
        case PROP_TITLE:
            _moo_accel_button_set_title (button, g_value_get_string (value));
            break;

        case PROP_ACCEL:
            _moo_accel_button_set_accel (button, g_value_get_string (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    }
}


static void
moo_accel_button_get_property (GObject        *object,
                               guint           param_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
    MooAccelButton *button = MOO_ACCEL_BUTTON (object);

    switch (param_id)
    {
        case PROP_TITLE:
            g_value_set_string (value, _moo_accel_button_get_title (button));
            break;

        case PROP_ACCEL:
            g_value_set_string (value, _moo_accel_button_get_accel (button));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    }
}


static const char *
_moo_accel_button_get_title (MooAccelButton *button)
{
    g_return_val_if_fail (MOO_IS_ACCEL_BUTTON (button), NULL);
    return button->title;
}


static void
_moo_accel_button_set_title (MooAccelButton *button,
                             const char     *title)
{
    g_return_if_fail (MOO_IS_ACCEL_BUTTON (button));
    g_free (button->title);
    button->title = g_strdup (title);
    g_object_notify (G_OBJECT (button), "title");
}


const char *
_moo_accel_button_get_accel (MooAccelButton *button)
{
    g_return_val_if_fail (MOO_IS_ACCEL_BUTTON (button), NULL);
    return button->accel;
}


gboolean
_moo_accel_button_set_accel (MooAccelButton *button,
                             const char     *accel)
{
    guint accel_key = 0;
    GdkModifierType accel_mods = (GdkModifierType) 0;

    g_return_val_if_fail (MOO_IS_ACCEL_BUTTON (button), FALSE);

    if (accel && accel[0])
    {
        gtk_accelerator_parse (accel, &accel_key, &accel_mods);

        if (!accel_key || accel_key == GDK_VoidSymbol)
            return FALSE;
    }

    g_free (button->accel);

    if (accel_key || accel_mods)
    {
        char *label = gtk_accelerator_get_label (accel_key, accel_mods);
        button->accel = gtk_accelerator_name (accel_key, accel_mods);
        gtk_button_set_label (GTK_BUTTON (button), label);
        g_free (label);
    }
    else
    {
        button->accel = g_strdup ("");
        gtk_button_set_label (GTK_BUTTON (button), "");
    }

    g_object_notify (G_OBJECT (button), "accel");
    g_signal_emit (button, signals[ACCEL_SET], 0, button->accel);
    return TRUE;
}


typedef struct {
    GdkModifierType mods;
    guint key;
    GtkLabel *label;
    GtkDialog *dialog;
    guint commit_timeout;
} Stuff;


static gboolean
commit_timeout (Stuff *s)
{
    s->commit_timeout = 0;
    gtk_dialog_response (s->dialog, GTK_RESPONSE_OK);
    return FALSE;
}

static void
add_commit_timeout (Stuff *s)
{
    if (s->commit_timeout)
        g_source_remove (s->commit_timeout);
    s->commit_timeout = g_timeout_add (500, (GSourceFunc) commit_timeout, s);
}

static void
remove_commit_timeout (Stuff *s)
{
    if (s->commit_timeout)
        g_source_remove (s->commit_timeout);
    s->commit_timeout = 0;
}

static gboolean
key_event (GtkWidget    *widget,
           GdkEventKey  *event,
           Stuff        *s)
{
    GdkKeymap *keymap;
    guint keyval;
    GdkModifierType consumed_modifiers;
    int mods;

    keymap = gdk_keymap_get_for_display (gtk_widget_get_display (widget));
    moo_keymap_translate_keyboard_state (keymap, event->hardware_keycode,
                                         (GdkModifierType) event->state, event->group,
                                         NULL, NULL, NULL, &consumed_modifiers);

    keyval = gdk_keyval_to_lower (event->keyval);
    mods = event->state & gtk_accelerator_get_default_mod_mask () & ~consumed_modifiers;

    if (keyval != event->keyval)
        mods |= GDK_SHIFT_MASK;

    if (gtk_accelerator_valid (keyval, (GdkModifierType) mods))
    {
        char *label = gtk_accelerator_get_label (keyval, (GdkModifierType) mods);
        gtk_label_set_text (s->label, label);
        g_free (label);
        s->key = keyval;
        s->mods = (GdkModifierType) mods;
        add_commit_timeout (s);
    }
    else
    {
        remove_commit_timeout (s);
    }

    return TRUE;
}


static void
moo_accel_button_clicked (GtkButton *gtkbutton)
{
    MooAccelButton *button = MOO_ACCEL_BUTTON (gtkbutton);
    AccelDialogXml *xml;
    GtkWidget *dialog;
    GtkWidget *parent;
    Stuff s = { (GdkModifierType) 0, 0, NULL, NULL, 0 };
    int response;

    xml = accel_dialog_xml_new ();
    dialog = GTK_WIDGET (xml->AccelDialog);

    parent = gtk_widget_get_toplevel (GTK_WIDGET (gtkbutton));
    gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                  GTK_IS_WINDOW (parent) ? GTK_WINDOW (parent) : NULL);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);

    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_OK,
                                             GTK_RESPONSE_CANCEL,
                                            -1);

    if (button->title)
        gtk_window_set_title (GTK_WINDOW (dialog), button->title);

    gtk_button_set_use_underline (GTK_BUTTON (xml->ok), FALSE);
    gtk_button_set_use_underline (GTK_BUTTON (xml->cancel), FALSE);

    s.label = xml->label;
    s.dialog = GTK_DIALOG (dialog);

    g_signal_connect (xml->eventbox, "key-press-event", G_CALLBACK (key_event), &s);

    response = gtk_dialog_run (GTK_DIALOG (dialog));
    remove_commit_timeout (&s);

    gtk_widget_destroy (dialog);

    if (response == GTK_RESPONSE_OK)
    {
        if (s.key || s.mods)
        {
            char *accel = gtk_accelerator_name (s.key, s.mods);
            _moo_accel_button_set_accel (button, accel);
            g_free (accel);
        }
        else
        {
            _moo_accel_button_set_accel (button, "");
        }
    }
}
