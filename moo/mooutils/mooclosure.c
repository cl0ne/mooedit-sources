/*
 *   mooclosure.c
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

#include "mooutils/mooclosure.h"
#include "mooutils/mootype-macros.h"
#include "marshals.h"


MOO_DEFINE_BOXED_TYPE_R (MooClosure, moo_closure)


MooClosure*
moo_closure_alloc (gsize size,
                   MooClosureCall call,
                   MooClosureDestroy destroy)
{
    MooClosure *cl;

    g_return_val_if_fail (size >= sizeof(MooClosure), NULL);
    g_return_val_if_fail (call != NULL, NULL);

    cl = (MooClosure*) g_malloc0 (size);
    cl->call = call;
    cl->destroy = destroy;
    cl->ref_count = 1;
    cl->floating = TRUE;
    cl->valid = TRUE;

    return cl;
}


MooClosure *
moo_closure_ref (MooClosure *closure)
{
    if (closure)
        closure->ref_count++;
    return closure;
}


void
moo_closure_unref (MooClosure *closure)
{
    if (closure && !--closure->ref_count)
    {
        moo_closure_invalidate (closure);
        g_free (closure);
    }
}


void
moo_closure_sink (MooClosure *closure)
{
    g_return_if_fail (closure != NULL);

    if (closure->floating)
    {
        closure->floating = FALSE;
        moo_closure_unref (closure);
    }
}


MooClosure *
moo_closure_ref_sink (MooClosure *closure)
{
    moo_closure_ref (closure);
    moo_closure_sink (closure);
    return closure;
}


void
moo_closure_invoke (MooClosure *closure)
{
    g_return_if_fail (closure != NULL);

    if (closure->valid)
    {
        moo_closure_ref (closure);

        closure->in_call = TRUE;
        closure->call (closure);
        closure->in_call = FALSE;

        if (!closure->valid && closure->destroy)
            closure->destroy (closure);

        moo_closure_unref (closure);
    }
}


void
moo_closure_invalidate (MooClosure *closure)
{
    if (closure && closure->valid)
    {
        closure->valid = FALSE;

        if (!closure->in_call && closure->destroy)
        {
            closure->destroy (closure);
            closure->destroy = (MooClosureDestroy) 0xdeadbeef;
        }
    }
}


/******************************************************************/
/* MooClosureSignal
 */

typedef struct
{
    MooClosure parent;
    MooObjectPtr *object;
    guint signal_id;
    char *signal;
    GType ret_type;
    gpointer (*proxy) (gpointer);
} MooClosureSignal;


static void
moo_closure_signal_call (MooClosure *cl)
{
    MooClosureSignal *closure = (MooClosureSignal*) cl;

    if (!closure->proxy)
    {
        gboolean ret;
        g_signal_emit (closure->object->target, closure->signal_id, 0, &ret);
    }
    else
    {
        gboolean ret;
        gpointer object = closure->proxy (closure->object->target);
        g_return_if_fail (object != NULL);
        g_signal_emit_by_name (object, closure->signal, &ret);
    }
}


static void
moo_closure_signal_destroy (MooClosure *closure)
{
    MooClosureSignal *cl = (MooClosureSignal*) closure;
    _moo_object_ptr_free (cl->object);
    g_free (cl->signal);
}


static void
object_died (MooClosureSignal *cl)
{
    _moo_object_ptr_free (cl->object);
    cl->object = NULL;
    moo_closure_invalidate ((MooClosure*) cl);
}


static MooClosure*
moo_closure_signal_new (gpointer    object,
                        const char *signal,
                        GCallback   proxy_func)
{
    guint signal_id = 0;
    GSignalQuery query = {0}; /* make MSVC happy */
    MooClosureSignal *cl;

    g_return_val_if_fail (G_IS_OBJECT (object), NULL);
    g_return_val_if_fail (signal != NULL, NULL);

    if (!proxy_func)
    {
        signal_id = g_signal_lookup (signal, G_OBJECT_TYPE (object));
        g_return_val_if_fail (signal_id != 0, NULL);

        g_signal_query (signal_id, &query);

        if (query.n_params > 0)
        {
            g_warning ("implement me");
            return NULL;
        }
    }

    cl = moo_closure_new (MooClosureSignal,
                          moo_closure_signal_call,
                          moo_closure_signal_destroy);

    cl->object = _moo_object_ptr_new (G_OBJECT (object), (GWeakNotify) object_died, cl);
    cl->proxy = (gpointer (*) (gpointer)) proxy_func;
    cl->signal = g_strdup (signal);

    if (!proxy_func)
    {
        cl->signal_id = signal_id;
        cl->ret_type = query.return_type & ~(G_SIGNAL_TYPE_STATIC_SCOPE);
    }

    return (MooClosure*) cl;
}


/******************************************************************/
/* MooClosureSimple
 */

typedef struct
{
    MooClosure parent;
    MooObjectPtr *object;
    gpointer (*proxy) (gpointer);
    void (*callback) (gpointer);
} MooClosureSimple;


static void
moo_closure_simple_call (MooClosure *closure)
{
    MooClosureSimple *cl = (MooClosureSimple*) closure;
    gpointer data = cl->object->target;

    if (cl->proxy)
        data = cl->proxy (cl->object->target);

    g_object_ref (data);
    cl->callback (data);
    g_object_unref (data);
}


static void
moo_closure_simple_destroy (MooClosure *closure)
{
    MooClosureSimple *cl = (MooClosureSimple*) closure;
    _moo_object_ptr_free (cl->object);
    cl->object = NULL;
}


static void
closure_simple_object_died (MooClosureSimple *cl)
{
    MooObjectPtr *tmp = cl->object;
    cl->object = NULL;
    _moo_object_ptr_free (tmp);
    moo_closure_invalidate ((MooClosure*)cl);
}


static MooClosure*
moo_closure_simple_new (gpointer    object,
                        GCallback   callback,
                        GCallback   proxy_func)
{
    MooClosureSimple *cl;

    cl = moo_closure_new (MooClosureSimple,
                          moo_closure_simple_call,
                          moo_closure_simple_destroy);
    cl->object = _moo_object_ptr_new (G_OBJECT (object),
                                      (GWeakNotify) closure_simple_object_died,
                                      cl);
    cl->callback = (void (*) (gpointer)) callback;
    cl->proxy = (gpointer (*) (gpointer)) proxy_func;

    return (MooClosure*) cl;
}


MooClosure*
_moo_closure_new_simple (gpointer    object,
                         const char *signal,
                         GCallback   callback,
                         GCallback   proxy_func)
{
    g_return_val_if_fail (G_IS_OBJECT (object), NULL);
    g_return_val_if_fail (callback || signal, NULL);
    g_return_val_if_fail (!callback || !signal, NULL);

    if (signal)
        return moo_closure_signal_new (object, signal, proxy_func);
    else
        return moo_closure_simple_new (object, callback, proxy_func);
}


/******************************************************************/
/* MooObjectPtr
 */

static void
object_ptr_object_died (MooObjectPtr *ptr)
{
    GObject *object = ptr->target;

    ptr->target = NULL;

    if (ptr->notify)
        ptr->notify (ptr->notify_data, object);
}


MooObjectPtr*
_moo_object_ptr_new (GObject    *object,
                     GWeakNotify notify,
                     gpointer    data)
{
    MooObjectPtr *ptr;

    g_return_val_if_fail (G_IS_OBJECT (object), NULL);
    g_return_val_if_fail (notify != NULL || data == NULL, NULL);

    ptr = g_new (MooObjectPtr, 1);
    ptr->target = object;
    ptr->notify = notify;
    ptr->notify_data = data;

    g_object_weak_ref (object, (GWeakNotify) object_ptr_object_died, ptr);

    return ptr;
}


static void
_moo_object_ptr_die (MooObjectPtr *ptr)
{
    if (ptr)
    {
        if (ptr->target)
            g_object_weak_unref (ptr->target, (GWeakNotify) object_ptr_object_died, ptr);
        ptr->target = NULL;
    }
}


void
_moo_object_ptr_free (MooObjectPtr *ptr)
{
    _moo_object_ptr_die (ptr);
    g_free (ptr);
}


/* kate: strip on; indent-width 4; */
