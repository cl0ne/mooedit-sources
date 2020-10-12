/*
 *   mooentry.c
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
 * class:MooEntry: (parent GtkEntry) (constructable) (moo.private 1)
 **/

#include "marshals.h"
#include "mooutils/mooaccel.h"
#include "mooutils/mooentry.h"
#include "mooutils/mooundo.h"
#include "mooutils/mooutils-gobject.h"
#include "mooutils/mooeditops.h"
#include "mooutils/mooi18n.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>


struct _MooEntryPrivate {
    MooUndoStack *undo_stack;
    gboolean enable_undo;
    gboolean enable_undo_menu;
    guint use_ctrl_u : 1;
    guint grab_selection : 1;
    guint fool_entry : 1;
    guint empty : 1;
    guint special_chars_menu : 1;
};

static guint INSERT_ACTION_TYPE;
static guint DELETE_ACTION_TYPE;


static void     moo_entry_class_init        (MooEntryClass      *klass);
static void     moo_entry_editable_init     (GtkEditableClass   *klass);
static void     moo_entry_undo_ops_init     (MooUndoOpsIface    *iface);

static void     moo_entry_init              (MooEntry           *entry);
static void     moo_entry_finalize          (GObject            *object);
static void     moo_entry_set_property      (GObject            *object,
                                             guint               prop_id,
                                             const GValue       *value,
                                             GParamSpec         *pspec);
static void     moo_entry_get_property      (GObject            *object,
                                             guint               prop_id,
                                             GValue             *value,
                                             GParamSpec         *pspec);

static gboolean moo_entry_button_release    (GtkWidget          *widget,
                                             GdkEventButton     *event);

static void     moo_entry_delete_to_start   (MooEntry           *entry);

static void     moo_entry_populate_popup    (GtkEntry           *entry,
                                             GtkMenu            *menu);

static void     moo_entry_delete_from_cursor(GtkEntry           *entry,
                                             GtkDeleteType       type,
                                             gint                count);
static void     moo_entry_cut_clipboard     (GtkEntry           *entry);
static void     moo_entry_paste_clipboard   (GtkEntry           *entry);

static void     moo_entry_do_insert_text    (GtkEditable        *editable,
                                             const gchar        *text,
                                             gint                length,
                                             gint               *position);
static void     moo_entry_do_delete_text    (GtkEditable        *editable,
                                             gint                start_pos,
                                             gint                end_pos);
static void     moo_entry_set_selection_bounds (GtkEditable     *editable,
                                             gint                start_pos,
                                             gint                end_pos);
static gboolean moo_entry_get_selection_bounds (GtkEditable     *editable,
                                             gint               *start_pos,
                                             gint               *end_pos);
static void     moo_entry_changed           (GtkEditable        *editable);

static void     init_undo_actions           (void);
static MooUndoAction *insert_action_new     (GtkEditable        *editable,
                                             const gchar        *text,
                                             gssize              length,
                                             gint               *position);
static MooUndoAction *delete_action_new     (GtkEditable        *editable,
                                             gint                start_pos,
                                             gint                end_pos);


GType
moo_entry_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (!type))
    {
        static const GTypeInfo info =
        {
            sizeof (MooEntryClass),
            NULL,		/* base_init */
            NULL,		/* base_finalize */
            (GClassInitFunc) moo_entry_class_init,
            NULL,		/* class_finalize */
            NULL,		/* class_data */
            sizeof (MooEntry),
            0,
            (GInstanceInitFunc) moo_entry_init,
            NULL
        };

        static const GInterfaceInfo editable_info = {
            (GInterfaceInitFunc) moo_entry_editable_init, NULL, NULL
        };

        static const GInterfaceInfo undo_ops_info = {
            (GInterfaceInitFunc) moo_entry_undo_ops_init, NULL, NULL
        };

        type = g_type_register_static (GTK_TYPE_ENTRY, "MooEntry", &info, (GTypeFlags) 0);
        g_type_add_interface_static (type, GTK_TYPE_EDITABLE, &editable_info);
        g_type_add_interface_static (type, MOO_TYPE_UNDO_OPS, &undo_ops_info);
    }

    return type;
}


enum {
    PROP_0,
    PROP_ENABLE_UNDO,
    PROP_ENABLE_UNDO_MENU,
    PROP_GRAB_SELECTION,
    PROP_EMPTY,
    PROP_USE_SPECIAL_CHARS_MENU
};

enum {
    UNDO,
    REDO,
    BEGIN_USER_ACTION,
    END_USER_ACTION,
    DELETE_TO_START,
    NUM_SIGNALS
};

static guint signals[NUM_SIGNALS];
static GtkEditableClass *parent_editable_iface;
static gpointer moo_entry_parent_class;

static void
moo_entry_class_init (MooEntryClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    GtkEntryClass *entry_class = GTK_ENTRY_CLASS (klass);
    GtkBindingSet *binding_set;

    init_undo_actions ();

    gobject_class->finalize = moo_entry_finalize;
    gobject_class->set_property = moo_entry_set_property;
    gobject_class->get_property = moo_entry_get_property;

    widget_class->button_release_event = moo_entry_button_release;

    entry_class->populate_popup = moo_entry_populate_popup;
    entry_class->delete_from_cursor = moo_entry_delete_from_cursor;
    entry_class->cut_clipboard = moo_entry_cut_clipboard;
    entry_class->paste_clipboard = moo_entry_paste_clipboard;

    klass->undo = moo_entry_undo;
    klass->redo = moo_entry_redo;

    moo_entry_parent_class = g_type_class_peek_parent (klass);
    parent_editable_iface = reinterpret_cast<GtkEditableClass*> (g_type_interface_peek(moo_entry_parent_class, GTK_TYPE_EDITABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_ENABLE_UNDO,
                                     g_param_spec_boolean ("enable-undo",
                                             "enable-undo",
                                             "enable-undo",
                                             TRUE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_ENABLE_UNDO_MENU,
                                     g_param_spec_boolean ("enable-undo-menu",
                                             "enable-undo-menu",
                                             "enable-undo-menu",
                                             TRUE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_GRAB_SELECTION,
                                     g_param_spec_boolean ("grab-selection",
                                             "grab-selection",
                                             "grab-selection",
                                             TRUE,
                                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class,
                                     PROP_EMPTY,
                                     g_param_spec_boolean ("empty",
                                             "empty",
                                             "empty",
                                             TRUE,
                                             G_PARAM_READABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_USE_SPECIAL_CHARS_MENU,
                                     g_param_spec_boolean ("use-special-chars-menu",
                                             "use-special-chars-menu",
                                             "use-special-chars-menu",
                                             FALSE,
                                             (GParamFlags) G_PARAM_READWRITE));

    signals[UNDO] = g_signal_lookup ("undo", GTK_TYPE_ENTRY);

    if (!signals[UNDO])
        signals[UNDO] =
                g_signal_new ("undo",
                              G_OBJECT_CLASS_TYPE (klass),
                              (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                              G_STRUCT_OFFSET (MooEntryClass, undo),
                              NULL, NULL,
                              _moo_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);

    signals[REDO] = g_signal_lookup ("redo", GTK_TYPE_ENTRY);

    if (!signals[REDO])
        signals[REDO] =
                g_signal_new ("redo",
                              G_OBJECT_CLASS_TYPE (klass),
                              (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                              G_STRUCT_OFFSET (MooEntryClass, redo),
                              NULL, NULL,
                              _moo_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);

    signals[DELETE_TO_START] =
            _moo_signal_new_cb ("delete-to-start",
                                G_OBJECT_CLASS_TYPE (klass),
                                (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                                G_CALLBACK (moo_entry_delete_to_start),
                                NULL, NULL,
                                _moo_marshal_VOID__VOID,
                                G_TYPE_NONE, 0);

    binding_set = gtk_binding_set_by_class (klass);
    gtk_binding_entry_add_signal (binding_set, GDK_z,
                                  MOO_ACCEL_CTRL_MASK,
                                  "undo", 0);
    gtk_binding_entry_add_signal (binding_set, GDK_z,
                                  (GdkModifierType) (MOO_ACCEL_CTRL_MASK | GDK_SHIFT_MASK),
                                  "redo", 0);
    gtk_binding_entry_add_signal (binding_set, GDK_u,
                                  GDK_CONTROL_MASK,
                                  "delete-to-start", 0);
}


static void
moo_entry_editable_init (GtkEditableClass   *klass)
{
    klass->do_insert_text = moo_entry_do_insert_text;
    klass->do_delete_text = moo_entry_do_delete_text;
    klass->set_selection_bounds = moo_entry_set_selection_bounds;
    klass->get_selection_bounds = moo_entry_get_selection_bounds;
    klass->changed = moo_entry_changed;
}


static void
moo_entry_init (MooEntry *entry)
{
    entry->priv = g_new0 (MooEntryPrivate, 1);
    entry->priv->undo_stack = moo_undo_stack_new (entry);
    entry->priv->use_ctrl_u = TRUE;
    entry->priv->empty = TRUE;
}


static void
moo_entry_set_property (GObject        *object,
                        guint           prop_id,
                        const GValue   *value,
                        GParamSpec     *pspec)
{
    MooEntry *entry = MOO_ENTRY (object);

    switch (prop_id)
    {
        case PROP_ENABLE_UNDO:
            entry->priv->enable_undo = g_value_get_boolean (value);
            g_object_notify (object, "enable-undo");
            break;

        case PROP_ENABLE_UNDO_MENU:
            entry->priv->enable_undo_menu = g_value_get_boolean (value);
            g_object_notify (object, "enable-undo-menu");
            break;

        case PROP_GRAB_SELECTION:
            entry->priv->grab_selection = g_value_get_boolean (value) ? TRUE : FALSE;
            g_object_notify (object, "grab-selection");
            break;

        case PROP_USE_SPECIAL_CHARS_MENU:
            moo_entry_set_use_special_chars_menu (entry, g_value_get_boolean (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_entry_get_property (GObject        *object,
                        guint           prop_id,
                        GValue         *value,
                        GParamSpec     *pspec)
{
    MooEntry *entry = MOO_ENTRY (object);

    switch (prop_id)
    {
        case PROP_ENABLE_UNDO:
            g_value_set_boolean (value, entry->priv->enable_undo);
            break;

        case PROP_ENABLE_UNDO_MENU:
            g_value_set_boolean (value, entry->priv->enable_undo_menu);
            break;

        case PROP_GRAB_SELECTION:
            g_value_set_boolean (value, entry->priv->grab_selection ? TRUE : FALSE);
            break;

        case PROP_EMPTY:
            g_value_set_boolean (value, gtk_entry_get_text_length (GTK_ENTRY (entry)) == 0);
            break;

        case PROP_USE_SPECIAL_CHARS_MENU:
            g_value_set_boolean (value, entry->priv->special_chars_menu != 0);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_entry_finalize (GObject *object)
{
    MooEntry *entry = MOO_ENTRY (object);

    g_object_unref (entry->priv->undo_stack);
    g_free (entry->priv);

    G_OBJECT_CLASS (moo_entry_parent_class)->finalize (object);
}


static void
moo_entry_changed (GtkEditable *editable)
{
    MooEntry *entry = MOO_ENTRY (editable);
    gboolean empty = gtk_entry_get_text_length (GTK_ENTRY (editable)) == 0;

    if ((empty && !entry->priv->empty) ||
         (!empty && entry->priv->empty))
    {
        entry->priv->empty = empty;
        g_object_notify (G_OBJECT (entry), "empty");
    }

    if (parent_editable_iface->changed)
        parent_editable_iface->changed (editable);
}


void
moo_entry_undo (MooEntry *entry)
{
    g_return_if_fail (MOO_IS_ENTRY (entry));

    if (entry->priv->enable_undo && moo_undo_stack_can_undo (entry->priv->undo_stack))
        moo_undo_stack_undo (entry->priv->undo_stack);

    moo_undo_ops_can_undo_changed (G_OBJECT (entry));
    moo_undo_ops_can_redo_changed (G_OBJECT (entry));
}

void
moo_entry_redo (MooEntry *entry)
{
    g_return_if_fail (MOO_IS_ENTRY (entry));

    if (entry->priv->enable_undo && moo_undo_stack_can_redo (entry->priv->undo_stack))
        moo_undo_stack_redo (entry->priv->undo_stack);

    moo_undo_ops_can_undo_changed (G_OBJECT (entry));
    moo_undo_ops_can_redo_changed (G_OBJECT (entry));
}

void
moo_entry_begin_undo_group (MooEntry *entry)
{
    g_return_if_fail (MOO_IS_ENTRY (entry));
    moo_undo_stack_start_group (entry->priv->undo_stack);
}

void
moo_entry_end_undo_group (MooEntry *entry)
{
    g_return_if_fail (MOO_IS_ENTRY (entry));
    moo_undo_stack_end_group (entry->priv->undo_stack);
}

void
moo_entry_clear_undo (MooEntry *entry)
{
    g_return_if_fail (MOO_IS_ENTRY (entry));
    moo_undo_stack_clear (entry->priv->undo_stack);
}

static gboolean
undo_ops_can_undo (MooUndoOps *obj)
{
    MooEntry *entry = MOO_ENTRY (obj);
    return entry->priv->enable_undo &&
            moo_undo_stack_can_undo (entry->priv->undo_stack);
}

static gboolean
undo_ops_can_redo (MooUndoOps *obj)
{
    MooEntry *entry = MOO_ENTRY (obj);
    return entry->priv->enable_undo &&
            moo_undo_stack_can_redo (entry->priv->undo_stack);
}

static void
moo_entry_undo_ops_init (MooUndoOpsIface *iface)
{
    iface->undo = (void(*)(MooUndoOps*)) moo_entry_undo;
    iface->redo = (void(*)(MooUndoOps*)) moo_entry_redo;
    iface->can_undo = undo_ops_can_undo;
    iface->can_redo = undo_ops_can_redo;
}


GtkWidget *
moo_entry_new (void)
{
    return GTK_WIDGET (g_object_new (MOO_TYPE_ENTRY, (const char*) NULL));
}


static void
moo_entry_insert_at_cursor (GtkEntry   *entry,
                            const char *text,
                            int         len)
{
    int start, end, pos;

    if (MOO_IS_ENTRY (entry))
        moo_entry_begin_undo_group (MOO_ENTRY (entry));

    if (gtk_editable_get_selection_bounds (GTK_EDITABLE (entry), &start, &end))
        gtk_editable_delete_text (GTK_EDITABLE (entry), start, end);

    pos = gtk_editable_get_position (GTK_EDITABLE (entry));
    gtk_editable_insert_text (GTK_EDITABLE (entry), text, len, &pos);
    gtk_editable_set_position (GTK_EDITABLE (entry), pos);

    if (MOO_IS_ENTRY (entry))
        moo_entry_end_undo_group (MOO_ENTRY (entry));
}


static void
special_char_item_activated (GtkWidget *item,
                             GtkEntry  *entry)
{
    const char *text;

    text = (const char*) g_object_get_data (G_OBJECT (item), "moo-entry-special-char");
    g_return_if_fail (text != NULL);

    moo_entry_insert_at_cursor (entry, text, -1);
}

static void
create_special_char_item (MooEntry   *entry,
                          GtkWidget  *menu,
                          const char *label,
                          const char *text)
{
    GtkWidget *item;

    item = gtk_menu_item_new_with_label (label);
    g_object_set_data (G_OBJECT (item), "moo-entry-special-char", (char*) text);
    g_signal_connect (item, "activate",
                      G_CALLBACK (special_char_item_activated),
                      entry);

    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
}

static GtkWidget *
create_special_chars_menu (MooEntry *entry)
{
    GtkWidget *menu = gtk_menu_new ();
    /* menu item in Insert Special Character submenu of text entry context menu */
    create_special_char_item (entry, menu, C_("insert-special-char", "Line End"), "\n");
    /* menu item in Insert Special Character submenu of text entry context menu */
    create_special_char_item (entry, menu, C_("insert-special-char", "Tab"), "\t");
    gtk_widget_show_all (menu);
    return menu;
}


static void
moo_entry_populate_popup (GtkEntry           *gtkentry,
                          GtkMenu            *menu)
{
    GtkWidget *item;
    MooEntry *entry = MOO_ENTRY (gtkentry);

    if (entry->priv->enable_undo_menu)
    {
        item = gtk_separator_menu_item_new ();
        gtk_widget_show (item);
        gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);

        item = gtk_image_menu_item_new_from_stock (GTK_STOCK_REDO, NULL);
        gtk_widget_show (item);
        gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
        gtk_widget_set_sensitive (item, entry->priv->enable_undo &&
                moo_undo_stack_can_redo (entry->priv->undo_stack));
        g_signal_connect_swapped (item, "activate", G_CALLBACK (moo_entry_redo), entry);

        item = gtk_image_menu_item_new_from_stock (GTK_STOCK_UNDO, NULL);
        gtk_widget_show (item);
        gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
        gtk_widget_set_sensitive (item, entry->priv->enable_undo &&
                moo_undo_stack_can_undo (entry->priv->undo_stack));
        g_signal_connect_swapped (item, "activate", G_CALLBACK (moo_entry_undo), entry);
    }

    if (entry->priv->special_chars_menu)
    {
        GtkWidget *submenu;

        item = gtk_separator_menu_item_new ();
        gtk_widget_show (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

        /* label of Insert Special Character submenu in text entry context menu */
        item = gtk_menu_item_new_with_label (C_("insert-special-char", "Insert Special Character"));
        gtk_widget_show (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

        submenu = create_special_chars_menu (entry);
        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
    }
}


void
moo_entry_set_use_special_chars_menu (MooEntry   *entry,
                                      gboolean    use)
{
    g_return_if_fail (MOO_IS_ENTRY (entry));

    if ((entry->priv->special_chars_menu != 0) == (use != 0))
        return;

    entry->priv->special_chars_menu = use != 0;
    g_object_notify (G_OBJECT (entry), "use-special-chars-menu");
}


static void
moo_entry_delete_from_cursor (GtkEntry           *entry,
                              GtkDeleteType       type,
                              gint                count)
{
    moo_undo_stack_new_group (MOO_ENTRY(entry)->priv->undo_stack);
    GTK_ENTRY_CLASS(moo_entry_parent_class)->delete_from_cursor (entry, type, count);
}

static void
moo_entry_cut_clipboard (GtkEntry           *entry)
{
    moo_undo_stack_new_group (MOO_ENTRY(entry)->priv->undo_stack);
    GTK_ENTRY_CLASS(moo_entry_parent_class)->cut_clipboard (entry);
}

static void
moo_entry_paste_clipboard (GtkEntry           *entry)
{
    moo_undo_stack_new_group (MOO_ENTRY(entry)->priv->undo_stack);
    GTK_ENTRY_CLASS(moo_entry_parent_class)->paste_clipboard (entry);
}

static void
moo_entry_do_insert_text (GtkEditable        *editable,
                          const gchar        *text,
                          gint                length,
                          gint               *position)
{
    if (length < 0)
        length = (int) strlen (text);

    if (*position < 0)
        *position = gtk_entry_get_text_length (GTK_ENTRY (editable));

    if (length > 0)
    {
        moo_undo_stack_add_action (MOO_ENTRY(editable)->priv->undo_stack,
                                   INSERT_ACTION_TYPE,
                                   insert_action_new (editable, text, length, position));
        parent_editable_iface->do_insert_text (editable, text, length, position);
        moo_undo_ops_can_undo_changed (G_OBJECT (editable));
    }
}

static void
moo_entry_do_delete_text (GtkEditable        *editable,
                          gint                start_pos,
                          gint                end_pos)
{
    if (start_pos == end_pos)
        return;

    g_return_if_fail (start_pos >= 0);

    if (end_pos < 0)
    {
        end_pos = gtk_entry_get_text_length (GTK_ENTRY (editable));
    }
    else if (start_pos > end_pos)
    {
        int tmp = start_pos;
        start_pos = end_pos;
        end_pos = tmp;
    }

    if (start_pos < end_pos)
    {
        moo_undo_stack_add_action (MOO_ENTRY(editable)->priv->undo_stack,
                                   DELETE_ACTION_TYPE,
                                   delete_action_new (editable, start_pos, end_pos));
        parent_editable_iface->do_delete_text (editable, start_pos, end_pos);
        moo_undo_ops_can_undo_changed (G_OBJECT (editable));
    }
}


static void
moo_entry_delete_to_start (MooEntry *entry)
{
    if (entry->priv->use_ctrl_u)
        gtk_editable_delete_text (GTK_EDITABLE (entry),
                                  0, gtk_editable_get_position (GTK_EDITABLE (entry)));
}


/*********************************************************************/
/* Working around gtk selection business
 * TODO: make stealing primary optional, independent of
 *       clearing selection
 */

/* GtkEdiatble::delete_text and GtkWidget::realize might also require this hack */

static void
moo_entry_set_selection_bounds (GtkEditable *editable,
                                gint         start_pos,
                                gint         end_pos)
{
    if (!MOO_ENTRY(editable)->priv->grab_selection)
        MOO_ENTRY(editable)->priv->fool_entry = TRUE;

    parent_editable_iface->set_selection_bounds (editable, start_pos, end_pos);
    MOO_ENTRY(editable)->priv->fool_entry = FALSE;
}

static gboolean
moo_entry_get_selection_bounds (GtkEditable *editable,
                                gint        *start_pos,
                                gint        *end_pos)
{
    if (MOO_ENTRY(editable)->priv->fool_entry)
        return FALSE;
    else
        return parent_editable_iface->get_selection_bounds (editable, start_pos, end_pos);
}

static gboolean
moo_entry_button_release (GtkWidget          *widget,
                          GdkEventButton     *event)
{
    gboolean result;

    if (!MOO_ENTRY(widget)->priv->grab_selection)
        MOO_ENTRY(widget)->priv->fool_entry = TRUE;

    result = GTK_WIDGET_CLASS(moo_entry_parent_class)->button_release_event (widget, event);
    MOO_ENTRY(widget)->priv->fool_entry = FALSE;

    return result;
}


/*********************************************************************/
/* Undo/redo
 */

typedef struct {
    int   pos;
    char *text;
    int   length;
    int   chars;
} InsertAction;

typedef struct {
    int   start;
    int   end;
    char *text;
    gboolean forward;
} DeleteAction;

static void     insert_action_undo      (InsertAction   *action,
                                         GtkEditable    *editable);
static void     insert_action_redo      (InsertAction   *action,
                                         GtkEditable    *editable);
static gboolean insert_action_merge     (InsertAction   *action,
                                         InsertAction   *what);
static void     insert_action_destroy   (InsertAction   *action);

static void     delete_action_undo      (DeleteAction   *action,
                                         GtkEditable    *editable);
static void     delete_action_redo      (DeleteAction   *action,
                                         GtkEditable    *editable);
static gboolean delete_action_merge     (DeleteAction   *action,
                                         DeleteAction   *what);
static void     delete_action_destroy   (DeleteAction   *action);


static MooUndoActionClass InsertActionClass = {
    (MooUndoActionUndo) insert_action_undo,
    (MooUndoActionRedo) insert_action_redo,
    (MooUndoActionMerge) insert_action_merge,
    (MooUndoActionDestroy) insert_action_destroy
};

static MooUndoActionClass DeleteActionClass = {
    (MooUndoActionUndo) delete_action_undo,
    (MooUndoActionRedo) delete_action_redo,
    (MooUndoActionMerge) delete_action_merge,
    (MooUndoActionDestroy) delete_action_destroy
};


static void
init_undo_actions (void)
{
    INSERT_ACTION_TYPE = moo_undo_action_register (&InsertActionClass);
    DELETE_ACTION_TYPE = moo_undo_action_register (&DeleteActionClass);
}


static MooUndoAction *
insert_action_new (G_GNUC_UNUSED GtkEditable *editable,
                   const gchar        *text,
                   gssize              length,
                   gint               *position)
{
    InsertAction *action;

    if (length < 0)
        length = strlen (text);

    g_return_val_if_fail (length > 0, NULL);

    action = g_new0 (InsertAction, 1);

    action->pos = *position;
    action->text = g_strndup (text, length);
    action->length = (int) length;
    action->chars = g_utf8_strlen (text, length);

    return (MooUndoAction*) action;
}


static MooUndoAction *
delete_action_new (GtkEditable        *editable,
                   gint                start_pos,
                   gint                end_pos)
{
    DeleteAction *action;

    g_return_val_if_fail (start_pos < end_pos, NULL);

    action = g_new0 (DeleteAction, 1);

    action->start = start_pos;
    action->end = end_pos;

    action->text = gtk_editable_get_chars (editable, start_pos, end_pos);

    /* figure out if the user used the Delete or the Backspace key */
    if (gtk_editable_get_position (editable) <= action->start)
        action->forward = TRUE;
    else
        action->forward = FALSE;

    return (MooUndoAction*) action;
}


static void
insert_action_undo (InsertAction   *action,
                    GtkEditable    *editable)
{
    gtk_editable_delete_text (editable, action->pos, action->pos + action->chars);
    gtk_editable_set_position (editable, action->pos);
}


static void
delete_action_undo (DeleteAction   *action,
                    GtkEditable    *editable)
{
    int pos_here = action->start;

    gtk_editable_insert_text (editable, action->text, -1, &pos_here);

    if (action->forward)
        gtk_editable_set_position (editable, action->start);
    else
        gtk_editable_set_position (editable, action->end);
}


static void
insert_action_redo (InsertAction   *action,
                    GtkEditable    *editable)
{
    int pos_here = action->pos;
    gtk_editable_insert_text (editable, action->text, action->length, &pos_here);
    gtk_editable_set_position (editable, action->pos + action->chars);
}


static void
delete_action_redo (DeleteAction   *action,
                    GtkEditable    *editable)
{
    gtk_editable_delete_text (editable, action->start, action->end);
    gtk_editable_set_position (editable, action->start);
}


static void
insert_action_destroy (InsertAction *action)
{
    if (action)
    {
        g_free (action->text);
        g_free (action);
    }
}


static void
delete_action_destroy (DeleteAction *action)
{
    if (action)
    {
        g_free (action->text);
        g_free (action);
    }
}


static gboolean
insert_action_merge (InsertAction   *last_action,
                     InsertAction   *action)
{
    char *tmp;

    if (action->pos != (last_action->pos + last_action->chars) ||
        (action->text[0] != ' ' && action->text[0] != '\t' &&
        (last_action->text[last_action->length-1] == ' ' ||
        last_action->text[last_action->length-1] == '\t')))
    {
        return FALSE;
    }

    tmp = g_strconcat (last_action->text, action->text, nullptr);
    g_free (last_action->text);
    last_action->length += action->length;
    last_action->text = tmp;
    last_action->chars += action->chars;

    return TRUE;
}


static gboolean
delete_action_merge (DeleteAction   *last_action,
                     DeleteAction   *action)
{
    char *tmp;

    if (last_action->forward != action->forward ||
        (last_action->start != action->start &&
        last_action->start != action->end))
    {
        return FALSE;
    }

    if (last_action->start == action->start)
    {
        char *text_end = g_utf8_offset_to_pointer (last_action->text,
                last_action->end - last_action->start - 1);

        /* Deleted with the delete key */
        if (action->text[0] != ' ' && action->text[0] != '\t' &&
            (*text_end == ' ' || *text_end  == '\t'))
        {
            return FALSE;
        }

        tmp = g_strconcat (last_action->text, action->text, nullptr);
        g_free (last_action->text);
        last_action->end += (action->end - action->start);
        last_action->text = tmp;
    }
    else
    {
        /* Deleted with the backspace key */
        if (action->text[0] != ' ' && action->text[0] != '\t' &&
            (last_action->text[0] == ' ' || last_action->text[0] == '\t'))
        {
            return FALSE;
        }

        tmp = g_strconcat (action->text, last_action->text, nullptr);
        g_free (last_action->text);
        last_action->start = action->start;
        last_action->text = tmp;
    }

    return TRUE;
}
