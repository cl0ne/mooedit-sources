/*
 *   mooundo.h
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

#ifndef MOO_UNDO_H
#define MOO_UNDO_H

#include <glib-object.h>

G_BEGIN_DECLS


#define MOO_TYPE_UNDO_STACK              (moo_undo_stack_get_type ())
#define MOO_UNDO_STACK(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_UNDO_STACK, MooUndoStack))
#define MOO_UNDO_STACK_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_UNDO_STACK, MooUndoStackClass))
#define MOO_IS_UNDO_STACK(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_UNDO_STACK))
#define MOO_IS_UNDO_STACK_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_UNDO_STACK))
#define MOO_UNDO_STACK_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_UNDO_STACK, MooUndoStackClass))


typedef struct _MooUndoStack          MooUndoStack;
typedef struct _MooUndoStackPrivate   MooUndoStackPrivate;
typedef struct _MooUndoStackClass     MooUndoStackClass;
typedef struct _MooUndoAction         MooUndoAction;
typedef struct _MooUndoActionClass    MooUndoActionClass;

typedef void     (*MooUndoActionUndo)   (MooUndoAction  *action,
                                         gpointer        document);
typedef void     (*MooUndoActionRedo)   (MooUndoAction  *action,
                                         gpointer        document);
typedef gboolean (*MooUndoActionMerge)  (MooUndoAction  *action,
                                         MooUndoAction  *what,
                                         gpointer        document);
typedef void     (*MooUndoActionDestroy)(MooUndoAction  *action,
                                         gpointer        document);

struct _MooUndoActionClass
{
    MooUndoActionUndo undo;
    MooUndoActionRedo redo;
    MooUndoActionMerge merge;
    MooUndoActionDestroy destroy;
};

struct _MooUndoStack
{
    GObject base;

    gpointer document;
    GSList *undo_stack; /* ActionGroup* */
    GSList *redo_stack; /* ActionGroup* */

    guint frozen;
    guint continue_group;
    gboolean do_continue;
    gboolean new_group;
};

struct _MooUndoStackClass
{
    GObjectClass base_class;

    void (*undo) (MooUndoStack    *stack);
    void (*redo) (MooUndoStack    *stack);
};


GType           moo_undo_stack_get_type     (void) G_GNUC_CONST;

MooUndoStack   *moo_undo_stack_new          (gpointer            document);

guint           moo_undo_action_register    (MooUndoActionClass *klass);

void            moo_undo_stack_add_action   (MooUndoStack       *stack,
                                             guint               type,
                                             MooUndoAction      *action);

void            moo_undo_stack_clear        (MooUndoStack       *stack);
void            moo_undo_stack_freeze       (MooUndoStack       *stack);
void            moo_undo_stack_thaw         (MooUndoStack       *stack);
gboolean        moo_undo_stack_frozen       (MooUndoStack       *stack);

void            moo_undo_stack_new_group    (MooUndoStack       *stack);
void            moo_undo_stack_start_group  (MooUndoStack       *stack);
void            moo_undo_stack_end_group    (MooUndoStack       *stack);

void            moo_undo_stack_undo         (MooUndoStack       *stack);
void            moo_undo_stack_redo         (MooUndoStack       *stack);
gboolean        moo_undo_stack_can_undo     (MooUndoStack       *stack);
gboolean        moo_undo_stack_can_redo     (MooUndoStack       *stack);


G_END_DECLS

#endif /* MOO_UNDO_H */
