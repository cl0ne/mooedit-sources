/*
 *   mooundo.c
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

#include "mooutils/mooundo.h"
#include "marshals.h"
#include <string.h>


/*
A bit of how and why:
    1) Instead of keeping one list of actions and pointer to 'current' one,
       it keeps two stacks - undo and redo. On Undo, action from undo stack is
       'undoed' and pushed into redo stack, and vice versa. When new action
       is added, redo stack is erased.
       These stacks are lists now, but they should be queues if we want to limit
       number of actions in the stack.
    2) Those stacks contain ActionGroup's, each ActionGroup is a queue of
       UndoAction instances.
    3) How actions are added:
        UndoMgr may be 'frozen' - then it discards added actions, it's
        gtk_source_undo_manager_(begin|end)_not_undoable_action().
        One may tell UndoMgr not to split action groups - analog of what
        GtkSourceUndoManager did for user_action signal. It's done by incrementing
        continue_group count.
        One may force UndoMgr to create new group - by new_group flag.

        Now, in the undo_stack_add_action() call it does the following:
            if it's frozen, free the action, and do nothing;
            if new_group is requested, create new undo group and push it on the stack;
            otherwise, try to merge given action into existing group. If it can be
            merged, fine. If it can't be merged, then create new group.
        Here's a problem with continue_group thing: when we ask for it, we want
        to merge following actions with this one. But what do we want to do with this one?
        ATM I made a do_continue flag which means "don't create new group" and is set
        _after_ adding first action. It works fine for text buffer, and for entry, but
        it's ugly and not clear.
*/

typedef struct {
    GQueue *actions;
} ActionGroup;

typedef struct {
    guint type;
    MooUndoAction *action;
} Wrapper;


static MooUndoActionClass *types;
static guint last_type;

#define WRAPPER_VTABLE(wrapper__)   (&types[(wrapper__)->type])
#define TYPE_VTABLE(type__)         (&types[type__])
#define TYPE_IS_REGISTERED(type__)  ((type__) && (type__) <= last_type)


static void     moo_undo_stack_finalize     (GObject        *object);
static void     moo_undo_stack_set_property (GObject        *object,
                                             guint           prop_id,
                                             const GValue   *value,
                                             GParamSpec     *pspec);
static void     moo_undo_stack_get_property (GObject        *object,
                                             guint           prop_id,
                                             GValue         *value,
                                             GParamSpec     *pspec);

static void     moo_undo_stack_undo_real    (MooUndoStack   *stack);
static void     moo_undo_stack_redo_real    (MooUndoStack   *stack);

static void     action_stack_free           (GSList        **stack,
                                             gpointer        doc);


G_DEFINE_TYPE(MooUndoStack, moo_undo_stack, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_DOCUMENT,
    PROP_CAN_UNDO,
    PROP_CAN_REDO
};

enum {
    UNDO,
    REDO,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void
moo_undo_stack_class_init (MooUndoStackClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = moo_undo_stack_set_property;
    gobject_class->get_property = moo_undo_stack_get_property;
    gobject_class->finalize = moo_undo_stack_finalize;

    klass->undo = moo_undo_stack_undo_real;
    klass->redo = moo_undo_stack_redo_real;

    g_object_class_install_property (gobject_class,
                                     PROP_DOCUMENT,
                                     g_param_spec_pointer ("document",
                                             "document",
                                             "document",
                                             (GParamFlags) (G_PARAM_CONSTRUCT | G_PARAM_READWRITE)));

    g_object_class_install_property (gobject_class,
                                     PROP_CAN_UNDO,
                                     g_param_spec_boolean ("can-undo",
                                             "can-undo",
                                             "can-undo",
                                             FALSE,
                                             G_PARAM_READABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_CAN_REDO,
                                     g_param_spec_boolean ("can-redo",
                                             "can-redo",
                                             "can-redo",
                                             FALSE,
                                             G_PARAM_READABLE));

    signals[UNDO] =
            g_signal_new ("undo",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                          G_STRUCT_OFFSET (MooUndoStackClass, undo),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[REDO] =
            g_signal_new ("redo",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                          G_STRUCT_OFFSET (MooUndoStackClass, redo),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);
}


static void
moo_undo_stack_init (G_GNUC_UNUSED MooUndoStack *stack)
{
}


MooUndoStack*
moo_undo_stack_new (gpointer document)
{
    return g_object_new (MOO_TYPE_UNDO_STACK,
                         "document", document,
                         (const char*) NULL);
}


static void
moo_undo_stack_finalize (GObject *object)
{
    MooUndoStack *stack = MOO_UNDO_STACK (object);

    action_stack_free (&stack->undo_stack, stack->document);
    action_stack_free (&stack->redo_stack, stack->document);

    G_OBJECT_CLASS(moo_undo_stack_parent_class)->finalize (object);
}


static void
moo_undo_stack_set_property (GObject        *object,
                             guint           prop_id,
                             const GValue   *value,
                             GParamSpec     *pspec)
{
    MooUndoStack *stack = MOO_UNDO_STACK (object);

    switch (prop_id)
    {
        case PROP_DOCUMENT:
            stack->document = g_value_get_pointer (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_undo_stack_get_property (GObject        *object,
                             guint           prop_id,
                             GValue         *value,
                             GParamSpec     *pspec)
{
    MooUndoStack *stack = MOO_UNDO_STACK (object);

    switch (prop_id)
    {
        case PROP_DOCUMENT:
            g_value_set_pointer (value, stack->document);
            break;

        case PROP_CAN_UNDO:
            g_value_set_boolean (value, moo_undo_stack_can_undo (stack));
            break;

        case PROP_CAN_REDO:
            g_value_set_boolean (value, moo_undo_stack_can_redo (stack));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


guint
moo_undo_action_register (MooUndoActionClass *klass)
{
    g_return_val_if_fail (klass != NULL, 0);
    g_return_val_if_fail (klass->undo != NULL && klass->redo != NULL, 0);
    g_return_val_if_fail (klass->merge != NULL && klass->destroy != NULL, 0);

    types = g_renew (MooUndoActionClass, types, last_type + 2);
    types[last_type+1] = *klass;

    return ++last_type;
}


void
moo_undo_stack_undo (MooUndoStack *stack)
{
    g_return_if_fail (MOO_IS_UNDO_STACK (stack));
    g_signal_emit (stack, signals[UNDO], 0);
}


void
moo_undo_stack_redo (MooUndoStack *stack)
{
    g_return_if_fail (MOO_IS_UNDO_STACK (stack));
    g_signal_emit (stack, signals[REDO], 0);
}


gboolean
moo_undo_stack_can_undo (MooUndoStack *stack)
{
    g_return_val_if_fail (MOO_IS_UNDO_STACK (stack), FALSE);
    return stack->undo_stack != NULL;
}


gboolean
moo_undo_stack_can_redo (MooUndoStack *stack)
{
    g_return_val_if_fail (MOO_IS_UNDO_STACK (stack), FALSE);
    return stack->redo_stack != NULL;
}


static void
action_group_undo (ActionGroup    *group,
                   MooUndoStack   *stack)
{
    GList *l;

    for (l = group->actions->head; l != NULL; l = l->next)
    {
        Wrapper *wrapper = l->data;
        WRAPPER_VTABLE(wrapper)->undo (wrapper->action, stack->document);
    }
}


static void
action_group_redo (ActionGroup    *group,
                   MooUndoStack   *stack)
{
    GList *l;

    for (l = group->actions->tail; l != NULL; l = l->prev)
    {
        Wrapper *wrapper = l->data;
        WRAPPER_VTABLE(wrapper)->redo (wrapper->action, stack->document);
    }
}


static void
moo_undo_stack_undo_real (MooUndoStack *stack)
{
    ActionGroup *group;
    GSList *link;
    gboolean notify_redo;

    g_return_if_fail (stack->undo_stack != NULL);

    stack->frozen++;

    link = stack->undo_stack;
    group = link->data;
    stack->undo_stack = g_slist_delete_link (stack->undo_stack, link);
    notify_redo = stack->redo_stack == NULL;
    stack->redo_stack = g_slist_prepend (stack->redo_stack, group);
    stack->new_group = TRUE;

    action_group_undo (group, stack);

    stack->frozen--;

    g_object_freeze_notify (G_OBJECT (stack));

    if (!stack->undo_stack)
        g_object_notify (G_OBJECT (stack), "can-undo");
    if (notify_redo)
        g_object_notify (G_OBJECT (stack), "can-redo");

    g_object_thaw_notify (G_OBJECT (stack));
}


static void
moo_undo_stack_redo_real (MooUndoStack *stack)
{
    ActionGroup *group;
    GSList *link;
    gboolean notify_undo;

    g_return_if_fail (stack->redo_stack != NULL);

    stack->frozen++;

    link = stack->redo_stack;
    group = link->data;
    stack->redo_stack = g_slist_delete_link (stack->redo_stack, link);
    notify_undo = stack->undo_stack == NULL;
    stack->undo_stack = g_slist_prepend (stack->undo_stack, group);
    stack->new_group = TRUE;

    action_group_redo (group, stack);

    stack->frozen--;

    g_object_freeze_notify (G_OBJECT (stack));

    if (!stack->redo_stack)
        g_object_notify (G_OBJECT (stack), "can-redo");
    if (notify_undo)
        g_object_notify (G_OBJECT (stack), "can-undo");

    g_object_thaw_notify (G_OBJECT (stack));
}


static Wrapper *
wrapper_new (guint          type,
             MooUndoAction *action)
{
    Wrapper *w = g_new0 (Wrapper, 1);
    w->type = type;
    w->action = action;
    return w;
}


static void
wrapper_free (Wrapper  *wrapper,
              gpointer  doc)
{
    if (wrapper)
    {
        WRAPPER_VTABLE(wrapper)->destroy (wrapper->action, doc);
        g_free (wrapper);
    }
}


static void
action_group_free (ActionGroup *group,
                   gpointer     doc)
{
    if (group)
    {
        g_queue_foreach (group->actions, (GFunc) wrapper_free, doc);
        g_queue_free (group->actions);
        g_free (group);
    }
}


static ActionGroup*
action_group_new (void)
{
    ActionGroup *group = g_new0 (ActionGroup, 1);
    group->actions = g_queue_new ();
    return group;
}


static void
action_stack_free (GSList **stack,
                   gpointer doc)
{
    g_slist_foreach (*stack, (GFunc) action_group_free, doc);
    g_slist_free (*stack);
    *stack = NULL;
}


void
moo_undo_stack_clear (MooUndoStack *stack)
{
    gboolean notify_redo, notify_undo;

    g_return_if_fail (MOO_IS_UNDO_STACK (stack));

    notify_undo = stack->undo_stack != NULL;
    notify_redo = stack->redo_stack != NULL;

    action_stack_free (&stack->undo_stack, stack->document);
    action_stack_free (&stack->redo_stack, stack->document);
    stack->new_group = FALSE;

    g_object_freeze_notify (G_OBJECT (stack));

    if (notify_undo)
        g_object_notify (G_OBJECT (stack), "can-undo");
    if (notify_redo)
        g_object_notify (G_OBJECT (stack), "can-redo");

    g_object_thaw_notify (G_OBJECT (stack));
}


static gboolean
action_group_merge (ActionGroup    *group,
                    guint           type,
                    MooUndoAction  *action,
                    gpointer        doc)
{
    Wrapper *old;

    old = group->actions->head ? group->actions->head->data : NULL;

    if (!old || old->type != type)
        return FALSE;

    if (WRAPPER_VTABLE(old)->merge (old->action, action, doc))
    {
        TYPE_VTABLE(type)->destroy (action, doc);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void
action_group_add (ActionGroup    *group,
                  guint           type,
                  MooUndoAction  *action,
                  gboolean        try_merge,
                  gpointer        doc)
{
    if (!try_merge || !action_group_merge (group, type, action, doc))
    {
        Wrapper *wrapper = wrapper_new (type, action);
        g_queue_push_head (group->actions, wrapper);
    }
}


void
moo_undo_stack_add_action (MooUndoStack   *stack,
                           guint           type,
                           MooUndoAction  *action)
{
    gboolean notify_redo, notify_undo;
    ActionGroup *group;

    g_return_if_fail (MOO_IS_UNDO_STACK (stack));
    g_return_if_fail (action != NULL);
    g_return_if_fail (TYPE_IS_REGISTERED (type));

    if (stack->frozen)
    {
        TYPE_VTABLE(type)->destroy (action, stack->document);
        return;
    }

    notify_undo = stack->undo_stack == NULL;
    notify_redo = stack->redo_stack != NULL;

    if (!stack->undo_stack || stack->new_group)
    {
        group = action_group_new ();
        stack->undo_stack = g_slist_prepend (stack->undo_stack, group);
        action_group_add (group, type, action, FALSE, stack->document);
    }
    else if (stack->do_continue)
    {
        group = stack->undo_stack->data;
        action_group_add (group, type, action, TRUE, stack->document);
    }
    else
    {
        group = stack->undo_stack->data;

        if (!action_group_merge (group, type, action, stack->document))
        {
            group = action_group_new ();
            stack->undo_stack = g_slist_prepend (stack->undo_stack, group);
            action_group_add (group, type, action, TRUE, stack->document);
        }
    }

    stack->new_group = FALSE;
    if (stack->continue_group)
        stack->do_continue = TRUE;

    action_stack_free (&stack->redo_stack, stack->document);

    g_object_freeze_notify (G_OBJECT (stack));

    if (notify_undo)
        g_object_notify (G_OBJECT (stack), "can-undo");
    if (notify_redo)
        g_object_notify (G_OBJECT (stack), "can-redo");

    g_object_thaw_notify (G_OBJECT (stack));
}


void
moo_undo_stack_start_group (MooUndoStack *stack)
{
    g_return_if_fail (MOO_IS_UNDO_STACK (stack));
    stack->continue_group++;
}


void
moo_undo_stack_end_group (MooUndoStack *stack)
{
    g_return_if_fail (MOO_IS_UNDO_STACK (stack));
    g_return_if_fail (stack->continue_group > 0);
    if (!--stack->continue_group)
        stack->do_continue = FALSE;
}


void
moo_undo_stack_new_group (MooUndoStack *stack)
{
    g_return_if_fail (MOO_IS_UNDO_STACK (stack));
    stack->new_group = TRUE;
}


void
moo_undo_stack_freeze (MooUndoStack *stack)
{
    g_return_if_fail (MOO_IS_UNDO_STACK (stack));

    if (!stack->frozen++)
        moo_undo_stack_clear (stack);
}


void
moo_undo_stack_thaw (MooUndoStack *stack)
{
    g_return_if_fail (MOO_IS_UNDO_STACK (stack));
    g_return_if_fail (stack->frozen > 0);
    stack->frozen--;
}


gboolean
moo_undo_stack_frozen (MooUndoStack *stack)
{
    return stack->frozen > 0;
}
