/*
 *   mooeditops.c
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

#include "mooeditops.h"
#include "marshals.h"
#include <gtk/gtk.h>

static void
moo_edit_ops_class_init (G_GNUC_UNUSED MooEditOpsIface *iface)
{
    g_signal_new ("moo-edit-ops-can-do-op-changed",
                  MOO_TYPE_EDIT_OPS,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _moo_marshal_VOID__UINT,
                  G_TYPE_NONE, 1,
                  G_TYPE_UINT);
}

GType
moo_edit_ops_get_type (void)
{
    static GType type;

    if (G_UNLIKELY (!type))
    {
        GTypeInfo type_info = {
            sizeof (MooEditOpsIface), /* class_size */
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) moo_edit_ops_class_init, /* class_init */
            NULL
        };

        type = g_type_register_static (G_TYPE_INTERFACE, "MooEditOps",
                                       &type_info, (GTypeFlags) 0);

        g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

    return type;
}


void
moo_edit_ops_can_do_op_changed (GObject       *obj,
                                MooEditOpType  type)
{
    g_return_if_fail (type < MOO_N_EDIT_OPS);
    g_signal_emit_by_name (obj, "moo-edit-ops-can-do-op-changed", type);
}


static void
emit_can_do_op_changed (GObject *obj)
{
    int i;
    for (i = 0; i < MOO_N_EDIT_OPS; i++)
        moo_edit_ops_can_do_op_changed (obj, (MooEditOpType) i);
}


static void
editable_do_op (GtkEditable   *obj,
                MooEditOpType  type)
{
    switch (type)
    {
        case MOO_EDIT_OP_CUT:
            gtk_editable_cut_clipboard (obj);
            break;
        case MOO_EDIT_OP_COPY:
            gtk_editable_copy_clipboard (obj);
            break;
        case MOO_EDIT_OP_PASTE:
            gtk_editable_paste_clipboard (obj);
            break;
        case MOO_EDIT_OP_DELETE:
            gtk_editable_delete_selection (obj);
            break;
        case MOO_EDIT_OP_SELECT_ALL:
            gtk_editable_select_region (obj, 0, -1);
            break;
        default:
            g_return_if_reached ();
    }
}

static gboolean
entry_can_do_op (GtkEntry      *obj,
                 MooEditOpType  type)
{
    switch (type)
    {
        case MOO_EDIT_OP_CUT:
            return gtk_editable_get_editable (GTK_EDITABLE (obj)) &&
                    gtk_editable_get_selection_bounds (GTK_EDITABLE (obj), NULL, NULL);
        case MOO_EDIT_OP_COPY:
            return gtk_editable_get_selection_bounds (GTK_EDITABLE (obj), NULL, NULL);
        case MOO_EDIT_OP_PASTE:
            return gtk_editable_get_editable (GTK_EDITABLE (obj));
        case MOO_EDIT_OP_DELETE:
            return gtk_editable_get_selection_bounds (GTK_EDITABLE (obj), NULL, NULL);

        case MOO_EDIT_OP_SELECT_ALL:
            {
                const char *text = gtk_entry_get_text (obj);
                return text && text[0];
            }
            break;

        default:
            g_return_val_if_reached (FALSE);
    }
}

static void
entry_connect (GtkEntry *obj)
{
    g_signal_connect (obj, "notify::selection-bound",
                      G_CALLBACK (emit_can_do_op_changed), NULL);
    g_signal_connect (obj, "notify::text",
                      G_CALLBACK (emit_can_do_op_changed), NULL);
}

static void
entry_disconnect (GtkEntry *obj)
{
    g_signal_handlers_disconnect_by_func (obj, (gpointer) emit_can_do_op_changed, NULL);
}


static void
textview_do_op (GtkTextView   *obj,
                MooEditOpType  type)
{
    switch (type)
    {
        case MOO_EDIT_OP_CUT:
            g_signal_emit_by_name (obj, "cut-clipboard");
            break;
        case MOO_EDIT_OP_COPY:
            g_signal_emit_by_name (obj, "copy-clipboard");
            break;
        case MOO_EDIT_OP_PASTE:
            g_signal_emit_by_name (obj, "paste-clipboard");
            break;
        case MOO_EDIT_OP_DELETE:
            g_signal_emit_by_name (obj, "delete-from-cursor", GTK_DELETE_CHARS, 1);
            break;
        case MOO_EDIT_OP_SELECT_ALL:
            g_signal_emit_by_name (obj, "select-all", TRUE);
            break;
        default:
            g_return_if_reached ();
    }
}

static gboolean
textview_can_do_op (GtkTextView   *obj,
                    MooEditOpType  type)
{
    GtkTextBuffer *buffer = obj->buffer;

    switch (type)
    {
        case MOO_EDIT_OP_CUT:
            return gtk_text_view_get_editable (obj) &&
                    gtk_text_buffer_get_selection_bounds (buffer, NULL, NULL);
        case MOO_EDIT_OP_COPY:
            return gtk_text_buffer_get_selection_bounds (buffer, NULL, NULL);
        case MOO_EDIT_OP_PASTE:
            return gtk_text_view_get_editable (obj);
        case MOO_EDIT_OP_DELETE:
            return gtk_text_buffer_get_selection_bounds (buffer, NULL, NULL);

        case MOO_EDIT_OP_SELECT_ALL:
            return gtk_text_buffer_get_char_count (buffer) != 0;

        default:
            g_return_val_if_reached (FALSE);
    }
}

static void
textview_connect (GtkTextView *obj)
{
    /* XXX */
    GtkTextBuffer *buffer = obj->buffer;

    if (buffer)
    {
        g_signal_connect_swapped (buffer, "notify::has-selection",
                                  G_CALLBACK (emit_can_do_op_changed), obj);
        g_signal_connect_swapped (buffer, "changed",
                                  G_CALLBACK (emit_can_do_op_changed), obj);
    }
}

static void
textview_disconnect (GtkTextView *obj)
{
    /* XXX */
    GtkTextBuffer *buffer = obj->buffer;

    if (buffer)
        g_signal_handlers_disconnect_by_func (buffer, (gpointer) emit_can_do_op_changed, obj);
}


void
_moo_edit_ops_do_op (GObject       *obj,
                     MooEditOpType  type)
{
    g_return_if_fail (type < MOO_N_EDIT_OPS);

    if (MOO_IS_EDIT_OPS (obj))
    {
        g_return_if_fail (MOO_EDIT_OPS_GET_IFACE (obj)->do_op != NULL);
        MOO_EDIT_OPS_GET_IFACE (obj)->do_op (MOO_EDIT_OPS (obj), type);
    }
    else if (GTK_IS_ENTRY (obj))
    {
        editable_do_op (GTK_EDITABLE (obj), type);
    }
    else if (GTK_IS_TEXT_VIEW (obj))
    {
        textview_do_op (GTK_TEXT_VIEW (obj), type);
    }
    else
    {
        g_return_if_reached ();
    }
}

gboolean
_moo_edit_ops_can_do_op (GObject       *obj,
                         MooEditOpType  type)
{
    g_return_val_if_fail (type < MOO_N_EDIT_OPS, FALSE);

    if (MOO_IS_EDIT_OPS (obj))
    {
        g_return_val_if_fail (MOO_EDIT_OPS_GET_IFACE (obj)->can_do_op != NULL, FALSE);
        return MOO_EDIT_OPS_GET_IFACE (obj)->can_do_op (MOO_EDIT_OPS (obj), type);
    }
    else if (GTK_IS_TEXT_VIEW (obj))
    {
        return textview_can_do_op (GTK_TEXT_VIEW (obj), type);
    }
    else if (GTK_IS_ENTRY (obj))
    {
        return entry_can_do_op (GTK_ENTRY (obj), type);
    }
    else
    {
        g_return_val_if_reached (FALSE);
    }
}

void
_moo_edit_ops_connect (GObject *obj)
{
    if (!MOO_IS_EDIT_OPS (obj))
    {
        if (GTK_IS_TEXT_VIEW (obj))
            textview_connect (GTK_TEXT_VIEW (obj));
        else if (GTK_IS_ENTRY (obj))
            entry_connect (GTK_ENTRY (obj));
        else
            g_return_if_reached ();
    }
}

void
_moo_edit_ops_disconnect (GObject *obj)
{
    if (!MOO_IS_EDIT_OPS (obj))
    {
        if (GTK_IS_TEXT_VIEW (obj))
            textview_disconnect (GTK_TEXT_VIEW (obj));
        else if (GTK_IS_ENTRY (obj))
            entry_disconnect (GTK_ENTRY (obj));
        else
            g_return_if_reached ();
    }
}

gboolean
_moo_edit_ops_check (GObject *obj)
{
    return MOO_IS_EDIT_OPS (obj) ||
            GTK_IS_TEXT_VIEW (obj) ||
            GTK_IS_ENTRY (obj);
}

void
_moo_edit_ops_iface_install (void)
{
    static gboolean been_here;

    if (!been_here)
    {
        g_signal_new ("moo-edit-ops-can-do-op-changed",
                      GTK_TYPE_TEXT_VIEW,
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      _moo_marshal_VOID__UINT,
                      G_TYPE_NONE, 1,
                      G_TYPE_UINT);
        g_signal_new ("moo-edit-ops-can-do-op-changed",
                      GTK_TYPE_ENTRY,
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      _moo_marshal_VOID__UINT,
                      G_TYPE_NONE, 1,
                      G_TYPE_UINT);
        been_here = TRUE;
    }
}


/************************************************************************/
/* MooUndoOps
 */

static void
moo_undo_ops_class_init (G_GNUC_UNUSED MooUndoOpsIface *iface)
{
    g_signal_new ("moo-undo-ops-can-undo-changed",
                  MOO_TYPE_UNDO_OPS,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _moo_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
    g_signal_new ("moo-undo-ops-can-redo-changed",
                  MOO_TYPE_UNDO_OPS,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _moo_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

GType
moo_undo_ops_get_type (void)
{
    static GType type;

    if (G_UNLIKELY (!type))
    {
        GTypeInfo type_info = {
            sizeof (MooUndoOpsIface), NULL, NULL,
            (GClassInitFunc) moo_undo_ops_class_init,
            NULL
        };

        type = g_type_register_static (G_TYPE_INTERFACE, "MooUndoOps",
                                       &type_info, (GTypeFlags) 0);

        g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

    return type;
}


void
moo_undo_ops_can_undo_changed (GObject *obj)
{
    g_signal_emit_by_name (obj, "moo-undo-ops-can-undo-changed");
}

void
moo_undo_ops_can_redo_changed (GObject *obj)
{
    g_signal_emit_by_name (obj, "moo-undo-ops-can-redo-changed");
}


void
_moo_undo_ops_undo (GObject *obj)
{
    if (MOO_IS_UNDO_OPS (obj))
    {
        g_return_if_fail (MOO_UNDO_OPS_GET_IFACE (obj)->undo != NULL);
        MOO_UNDO_OPS_GET_IFACE (obj)->undo (MOO_UNDO_OPS (obj));
    }
    else
    {
        g_return_if_reached ();
    }
}

void
_moo_undo_ops_redo (GObject *obj)
{
    if (MOO_IS_UNDO_OPS (obj))
    {
        g_return_if_fail (MOO_UNDO_OPS_GET_IFACE (obj)->redo != NULL);
        MOO_UNDO_OPS_GET_IFACE (obj)->redo (MOO_UNDO_OPS (obj));
    }
    else
    {
        g_return_if_reached ();
    }
}

gboolean
_moo_undo_ops_can_undo (GObject *obj)
{
    if (MOO_IS_UNDO_OPS (obj))
    {
        g_return_val_if_fail (MOO_UNDO_OPS_GET_IFACE (obj)->can_undo != NULL, FALSE);
        return MOO_UNDO_OPS_GET_IFACE (obj)->can_undo (MOO_UNDO_OPS (obj));
    }
    else
    {
        g_return_val_if_reached (FALSE);
    }
}

gboolean
_moo_undo_ops_can_redo (GObject *obj)
{
    if (MOO_IS_UNDO_OPS (obj))
    {
        g_return_val_if_fail (MOO_UNDO_OPS_GET_IFACE (obj)->can_redo != NULL, FALSE);
        return MOO_UNDO_OPS_GET_IFACE (obj)->can_redo (MOO_UNDO_OPS (obj));
    }
    else
    {
        g_return_val_if_reached (FALSE);
    }
}

gboolean
_moo_undo_ops_check (GObject *obj)
{
    return MOO_IS_UNDO_OPS (obj);
}
