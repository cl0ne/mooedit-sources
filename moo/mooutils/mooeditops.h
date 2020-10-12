/*
 *   mooeditops.h
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

#ifndef MOO_EDIT_OPS_H
#define MOO_EDIT_OPS_H

#include <glib-object.h>

G_BEGIN_DECLS


#define MOO_TYPE_EDIT_OPS              (moo_edit_ops_get_type ())
#define MOO_EDIT_OPS(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_EDIT_OPS, MooEditOps))
#define MOO_IS_EDIT_OPS(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_EDIT_OPS))
#define MOO_EDIT_OPS_GET_IFACE(obj)    (G_TYPE_INSTANCE_GET_INTERFACE ((obj), MOO_TYPE_EDIT_OPS, MooEditOpsIface))

#define MOO_TYPE_UNDO_OPS              (moo_undo_ops_get_type ())
#define MOO_UNDO_OPS(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_UNDO_OPS, MooUndoOps))
#define MOO_IS_UNDO_OPS(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_UNDO_OPS))
#define MOO_UNDO_OPS_GET_IFACE(obj)    (G_TYPE_INSTANCE_GET_INTERFACE ((obj), MOO_TYPE_UNDO_OPS, MooUndoOpsIface))

typedef struct MooEditOps      MooEditOps;
typedef struct MooEditOpsIface MooEditOpsIface;
typedef struct MooUndoOps      MooUndoOps;
typedef struct MooUndoOpsIface MooUndoOpsIface;

typedef enum {
    MOO_EDIT_OP_CUT,
    MOO_EDIT_OP_COPY,
    MOO_EDIT_OP_PASTE,
    MOO_EDIT_OP_DELETE,
    MOO_EDIT_OP_SELECT_ALL
} MooEditOpType;

#define MOO_N_EDIT_OPS (MOO_EDIT_OP_SELECT_ALL+1)

struct MooEditOpsIface
{
    GTypeInterface g_iface;

    void     (*do_op)       (MooEditOps    *obj,
                             MooEditOpType  type);
    gboolean (*can_do_op)   (MooEditOps    *obj,
                             MooEditOpType  type);
};

struct MooUndoOpsIface
{
    GTypeInterface g_iface;

    void     (*undo)     (MooUndoOps *obj);
    void     (*redo)     (MooUndoOps *obj);
    gboolean (*can_undo) (MooUndoOps *obj);
    gboolean (*can_redo) (MooUndoOps *obj);
};

GType       moo_edit_ops_get_type           (void) G_GNUC_CONST;
GType       moo_undo_ops_get_type           (void) G_GNUC_CONST;

void       _moo_edit_ops_do_op              (GObject        *obj,
                                             MooEditOpType   type);
gboolean   _moo_edit_ops_can_do_op          (GObject        *obj,
                                             MooEditOpType   type);
void        moo_edit_ops_can_do_op_changed  (GObject        *obj,
                                             MooEditOpType   type);

gboolean   _moo_edit_ops_check              (GObject        *obj);
void       _moo_edit_ops_iface_install      (void);
void       _moo_edit_ops_connect            (GObject        *obj);
void       _moo_edit_ops_disconnect         (GObject        *obj);

void       _moo_undo_ops_undo               (GObject        *obj);
void       _moo_undo_ops_redo               (GObject        *obj);
gboolean   _moo_undo_ops_can_undo           (GObject        *obj);
gboolean   _moo_undo_ops_can_redo           (GObject        *obj);
void        moo_undo_ops_can_undo_changed   (GObject        *obj);
void        moo_undo_ops_can_redo_changed   (GObject        *obj);
gboolean   _moo_undo_ops_check              (GObject        *obj);


G_END_DECLS

#endif /* MOO_EDIT_OPS_H */
