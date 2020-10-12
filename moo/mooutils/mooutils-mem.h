/*
 *   mooutils-mem.h
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

#ifndef MOO_UTILS_MEM_H
#define MOO_UTILS_MEM_H

#include <mooglib/moo-glib.h>
#include <string.h>


#define _MOO_COPYELMS(func_,dest_,src_,n_)      \
G_STMT_START {                                  \
    gsize n__ = n_;                             \
    if ((dest_) == (src_))                      \
        (void) 0;                               \
    func_ ((dest_), (src_),                     \
           n__ * sizeof *(dest_));              \
} G_STMT_END

#define MOO_ELMCPY(dest_,src_,n_) _MOO_COPYELMS (memcpy, dest_, src_, n_)
#define MOO_ELMMOVE(dest_,src_,n_) _MOO_COPYELMS (memmove, dest_, src_, n_)

#define MOO_ARRAY_GROW(Type_,mem_,new_size_)                \
G_STMT_START {                                              \
    gsize ns__ = new_size_;                                 \
    (mem_) = g_renew (Type_, (mem_), ns__);                 \
} G_STMT_END


#define MOO_IP_ARRAY_ELMS(ElmType,name_)                            \
    ElmType *name_;                                                 \
    gsize n_##name_;                                                \
    gsize n_##name_##_allocd__

#define MOO_IP_ARRAY_INIT(ElmType,c_,name_,len_)                    \
G_STMT_START {                                                      \
    gsize use_len__ = len_;                                         \
    if (use_len__ == 0)                                             \
        use_len__ = 2;                                              \
    (c_)->name_ = (ElmType*)                                        \
        g_malloc0 (use_len__ * sizeof *(c_)->name_);                \
    (c_)->n_##name_ = len_;                                         \
    (c_)->n_##name_##_allocd__ = use_len__;                         \
} G_STMT_END

#define MOO_IP_ARRAY_DESTROY(c_,name_)                              \
G_STMT_START {                                                      \
    g_free ((c_)->name_);                                           \
    (c_)->name_ = NULL;                                             \
} G_STMT_END

#define MOO_IP_ARRAY_GROW(ElmType,c_,name_,howmuch_)                \
G_STMT_START {                                                      \
    if ((c_)->n_##name_ + howmuch_ > (c_)->n_##name_##_allocd__)    \
    {                                                               \
        gsize old_size__ = (c_)->n_##name_##_allocd__;              \
        gsize new_size__ = MAX((gsize) (old_size__ * 1.2),          \
                               (c_)->n_##name_ + howmuch_);         \
        (c_)->name_ = (ElmType*) g_realloc ((c_)->name_,            \
                                 new_size__ * sizeof *(c_)->name_); \
        (c_)->n_##name_##_allocd__ = new_size__;                    \
    }                                                               \
                                                                    \
    if (howmuch_ > 0)                                               \
        memset ((c_)->name_ + (c_)->n_##name_, 0,                   \
                howmuch_ * sizeof *(c_)->name_);                    \
    (c_)->n_##name_ += howmuch_;                                    \
} G_STMT_END

#define MOO_IP_ARRAY_REMOVE_INDEX(c_,name_,idx_)                    \
G_STMT_START {                                                      \
    gsize idx__ = idx_;                                             \
    g_assert ((c_)->n_##name_ > 0 && idx__ < (c_)->n_##name_);      \
    if (idx__ + 1 < (c_)->n_##name_)                                \
        MOO_ELMMOVE ((c_)->name_ + idx__,                           \
                     (c_)->name_ + idx__ + 1,                       \
                     (c_)->n_##name_ - idx__ - 1);                  \
    (c_)->n_##name_ -= 1;                                           \
} G_STMT_END


#endif /* MOO_UTILS_MEM_H */
