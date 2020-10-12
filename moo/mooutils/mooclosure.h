/*
 *   mooclosure.h
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

#ifndef MOO_CLOSURE_H
#define MOO_CLOSURE_H

#include <glib-object.h>

G_BEGIN_DECLS


#define MOO_TYPE_CLOSURE (moo_closure_get_type ())

typedef struct _MooClosure MooClosure;
typedef struct _MooObjectPtr MooObjectPtr;

typedef void (*MooClosureCall)      (MooClosure *closure);
typedef void (*MooClosureDestroy)   (MooClosure *closure);

struct _MooClosure
{
    MooClosureCall call;
    MooClosureDestroy destroy;
    guint ref_count : 16;
    guint valid : 1;
    guint floating : 1;
    guint in_call : 1;
};

struct _MooObjectPtr
{
    GObject *target;
    GWeakNotify notify;
    gpointer notify_data;
};


GType       moo_closure_get_type            (void) G_GNUC_CONST;

MooClosure *moo_closure_alloc               (gsize size,
                                             MooClosureCall call,
                                             MooClosureDestroy destroy);
#define moo_closure_new(Type__,call__,destroy__) \
    ((Type__*)moo_closure_alloc (sizeof(Type__), call__, destroy__))

MooClosure *moo_closure_ref                 (MooClosure *closure);
MooClosure *moo_closure_ref_sink            (MooClosure *closure);
void        moo_closure_unref               (MooClosure *closure);
void        moo_closure_sink                (MooClosure *closure);

void        moo_closure_invoke              (MooClosure *closure);
void        moo_closure_invalidate          (MooClosure *closure);


#define MOO_OBJECT_PTR_GET(ptr_) ((ptr_) && (ptr_)->target ? (ptr_)->target : NULL)

MooObjectPtr *_moo_object_ptr_new           (GObject    *object,
                                             GWeakNotify notify,
                                             gpointer    data);
void        _moo_object_ptr_free            (MooObjectPtr *ptr);


MooClosure *_moo_closure_new_simple         (gpointer    object,
                                             const char *signal,
                                             GCallback   callback,
                                             GCallback   proxy_func);


G_END_DECLS

#endif /* MOO_CLOSURE_H */
