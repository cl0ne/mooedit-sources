/*
 *   mootype-macros.h
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

#ifndef MOO_TYPE_MACROS_H
#define MOO_TYPE_MACROS_H

#include <glib-object.h>
#include <mooutils/mooonce.h>

#define _MOO_REGISTER_TYPE(TypeName,type_name,TYPE_PARENT,flags)                            \
    g_define_type_id =                                                                      \
        g_type_register_static_simple (TYPE_PARENT,                                         \
                                       g_intern_static_string (#TypeName),                  \
                                       sizeof (TypeName##Class),                            \
                                       (GClassInitFunc) type_name##_class_intern_init,      \
                                       sizeof (TypeName),                                   \
                                       (GInstanceInitFunc) type_name##_init,                \
                                       (GTypeFlags) flags);

#define MOO_DEFINE_TYPE_STATIC_WITH_CODE(TypeName,type_name,TYPE_PARENT,code)               \
                                                                                            \
static void     type_name##_init              (TypeName        *self);                      \
static void     type_name##_class_init        (TypeName##Class *klass);                     \
static gpointer type_name##_parent_class = NULL;                                            \
                                                                                            \
static void     type_name##_class_intern_init (gpointer klass)                              \
{                                                                                           \
    type_name##_parent_class = g_type_class_peek_parent (klass);                            \
    type_name##_class_init ((TypeName##Class*) klass);                                      \
}                                                                                           \
                                                                                            \
static GType G_GNUC_CONST                                                                   \
type_name##_get_type (void)                                                                 \
{                                                                                           \
    static GType g_define_type_id;                                                          \
                                                                                            \
    MOO_DO_ONCE_BEGIN                                                                       \
                                                                                            \
    _MOO_REGISTER_TYPE(TypeName,type_name,TYPE_PARENT,0)                                    \
    code                                                                                    \
                                                                                            \
    MOO_DO_ONCE_END                                                                         \
                                                                                            \
    return g_define_type_id;                                                                \
}

#define MOO_DEFINE_TYPE_STATIC(TypeName,type_name,TYPE_PARENT)                              \
    MOO_DEFINE_TYPE_STATIC_WITH_CODE (TypeName, type_name, TYPE_PARENT, {})


#define MOO_DEFINE_BOXED_TYPE__(TypeName,type_name,copy_func,free_func)                     \
{                                                                                           \
    static GType g_define_type_id;                                                          \
                                                                                            \
    MOO_DO_ONCE_BEGIN                                                                       \
                                                                                            \
    g_define_type_id =                                                                      \
        g_boxed_type_register_static (#TypeName,                                            \
                                      (GBoxedCopyFunc) copy_func,                           \
                                      (GBoxedFreeFunc) free_func);                          \
                                                                                            \
    MOO_DO_ONCE_END                                                                         \
                                                                                            \
    return g_define_type_id;                                                                \
}

#define MOO_DEFINE_BOXED_TYPE(TypeName,type_name,copy_func,free_func)                       \
GType type_name##_get_type (void)                                                           \
    MOO_DEFINE_BOXED_TYPE__(TypeName,type_name,copy_func,free_func)

#define MOO_DEFINE_BOXED_TYPE_C(TypeName,type_name) \
    MOO_DEFINE_BOXED_TYPE(TypeName,type_name,type_name##_copy,type_name##_free)

#define MOO_DEFINE_BOXED_TYPE_R(TypeName,type_name) \
    MOO_DEFINE_BOXED_TYPE(TypeName,type_name,type_name##_ref,type_name##_unref)

#define MOO_DEFINE_BOXED_TYPE_STATIC(TypeName,type_name,copy_func,free_func)                \
static GType G_GNUC_CONST type_name##_get_type (void)                                       \
    MOO_DEFINE_BOXED_TYPE__(TypeName,type_name,copy_func,free_func)

#define MOO_DEFINE_BOXED_TYPE_STATIC_C(TypeName,type_name) \
    MOO_DEFINE_BOXED_TYPE_STATIC(TypeName,type_name,type_name##_copy,type_name##_free)

#define MOO_DEFINE_BOXED_TYPE_STATIC_R(TypeName,type_name) \
    MOO_DEFINE_BOXED_TYPE_STATIC(TypeName,type_name,type_name##_ref,type_name##_unref)


#define MOO_DEFINE_POINTER_TYPE(TypeName,type_name)                                         \
GType type_name##_get_type (void)                                                           \
{                                                                                           \
    static GType g_define_type_id;                                                          \
    MOO_DO_ONCE_BEGIN                                                                       \
    g_define_type_id = g_pointer_type_register_static (#TypeName);                          \
    MOO_DO_ONCE_END                                                                         \
    return g_define_type_id;                                                                \
}


#define MOO_DEFINE_QUARK__(QuarkName)                                                       \
{                                                                                           \
    static GQuark q;                                                                        \
    MOO_DO_ONCE_BEGIN                                                                       \
    q = g_quark_from_static_string (#QuarkName);                                            \
    MOO_DO_ONCE_END                                                                         \
    return q;                                                                               \
}

#define MOO_DECLARE_QUARK(QuarkName,quark_func)                                             \
GQuark quark_func (void) G_GNUC_CONST;

#define MOO_DEFINE_QUARK(QuarkName,quark_func)                                              \
GQuark quark_func (void)                                                                    \
    MOO_DEFINE_QUARK__(QuarkName)

#define MOO_DEFINE_QUARK_STATIC(QuarkName,quark_func)                                       \
static GQuark G_GNUC_CONST quark_func (void)                                                \
    MOO_DEFINE_QUARK__(QuarkName)


#endif /* MOO_TYPE_MACROS_H */
