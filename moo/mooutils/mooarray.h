#ifndef MOO_ARRAY_H
#define MOO_ARRAY_H

#include <mooutils/mooutils-mem.h>
#include <glib-object.h>

#define MOO_DECLARE_PTR_ARRAY_FULL(ArrayType, array_type, ElmType)      \
                                                                        \
typedef struct ArrayType ArrayType;                                     \
                                                                        \
struct ArrayType {                                                      \
    MOO_IP_ARRAY_ELMS (ElmType*, elms);                                 \
};                                                                      \
                                                                        \
ArrayType *array_type##_new (void);                                     \
void array_type##_free (ArrayType *ar);                                 \
void array_type##_append (ArrayType *ar, ElmType *elm);                 \
void array_type##_take (ArrayType *ar, ElmType *elm);                   \
void array_type##_append_array (ArrayType *ar, ArrayType *ar2);         \
ArrayType *array_type##_copy (ArrayType *ar);                           \
                                                                        \
typedef void (*ArrayType##Foreach) (ElmType *elm,                       \
                                    gpointer data);                     \
                                                                        \
G_INLINE_FUNC void                                                      \
array_type##_foreach (const ArrayType *ar,                              \
                      ArrayType##Foreach func,                          \
                      gpointer data)                                    \
{                                                                       \
    gsize i;                                                            \
    g_return_if_fail (ar != NULL && func != NULL);                      \
    for (i = 0; i < ar->n_elms; ++i)                                    \
        func (ar->elms[i], data);                                       \
}                                                                       \
                                                                        \
void array_type##_sort (ArrayType *ar, GCompareFunc func);              \
gssize array_type##_find (const ArrayType *ar, ElmType *elm);           \
void array_type##_remove (ArrayType *ar, ElmType *elm);                 \
void array_type##_clear (ArrayType *ar);                                \
gsize array_type##_insert_sorted (ArrayType *ar, ElmType *elm,          \
                                    GCompareFunc func);                 \
                                                                        \
G_INLINE_FUNC gboolean array_type##_is_empty (ArrayType *ar)            \
{                                                                       \
    return !ar || !ar->n_elms;                                          \
}                                                                       \
                                                                        \
G_INLINE_FUNC gsize array_type##_get_size (ArrayType *ar)               \
{                                                                       \
    return ar ? ar->n_elms : 0;                                         \
}

#define MOO_DECLARE_PTR_ARRAY(Element, element)                         \
    MOO_DECLARE_PTR_ARRAY_FULL(Element##Array, element##_array, Element)

#define MOO_DEFINE_PTR_ARRAY_FULL(ArrayType, array_type, ElmType,       \
                                  copy_elm, free_elm)                   \
                                                                        \
ArrayType *                                                             \
array_type##_new (void)                                                 \
{                                                                       \
    ArrayType *ar = g_slice_new0 (ArrayType);                           \
    MOO_IP_ARRAY_INIT (ElmType*, ar, elms, 0);                          \
    return ar;                                                          \
}                                                                       \
                                                                        \
void                                                                    \
array_type##_free (ArrayType *ar)                                       \
{                                                                       \
    if (ar)                                                             \
    {                                                                   \
        gsize i;                                                        \
        for (i = 0; i < ar->n_elms; ++i)                                \
            free_elm (ar->elms[i]);                                     \
        MOO_IP_ARRAY_DESTROY (ar, elms);                                \
        g_slice_free (ArrayType, ar);                                   \
    }                                                                   \
}                                                                       \
                                                                        \
void                                                                    \
array_type##_append (ArrayType *ar, ElmType *elm)                       \
{                                                                       \
    g_return_if_fail (ar != NULL && elm != NULL);                       \
    MOO_IP_ARRAY_GROW (ElmType*, ar, elms, 1);                          \
    ar->elms[ar->n_elms - 1] = copy_elm (elm);                          \
}                                                                       \
                                                                        \
void array_type##_append_array (ArrayType *ar, ArrayType *ar2)          \
{                                                                       \
    gsize i, old_size;                                                  \
    g_return_if_fail (ar != NULL && ar2 != NULL);                       \
    if (!ar2->n_elms)                                                   \
        return;                                                         \
    old_size = ar->n_elms;                                              \
    MOO_IP_ARRAY_GROW (ElmType*, ar, elms, ar2->n_elms);                \
    for (i = 0; i < ar2->n_elms; ++i)                                   \
        ar->elms[old_size + i] = copy_elm (ar2->elms[i]);               \
}                                                                       \
                                                                        \
void                                                                    \
array_type##_take (ArrayType *ar, ElmType *elm)                         \
{                                                                       \
    g_return_if_fail (ar != NULL && elm != NULL);                       \
    MOO_IP_ARRAY_GROW (ElmType*, ar, elms, 1);                          \
    ar->elms[ar->n_elms - 1] = elm;                                     \
}                                                                       \
                                                                        \
ArrayType *                                                             \
array_type##_copy (ArrayType *ar)                                       \
{                                                                       \
    ArrayType *copy;                                                    \
                                                                        \
    g_return_val_if_fail (ar != NULL, NULL);                            \
                                                                        \
    copy = array_type##_new ();                                         \
                                                                        \
    if (ar->n_elms)                                                     \
    {                                                                   \
        gsize i;                                                        \
        MOO_IP_ARRAY_GROW (ElmType*, copy, elms, ar->n_elms);           \
        for (i = 0; i < ar->n_elms; ++i)                                \
            copy->elms[i] = copy_elm (ar->elms[i]);                     \
    }                                                                   \
                                                                        \
    return copy;                                                        \
}                                                                       \
                                                                        \
void array_type##_remove (ArrayType *ar, ElmType *elm)                  \
{                                                                       \
    gsize i;                                                            \
                                                                        \
    g_return_if_fail (ar != NULL);                                      \
                                                                        \
    for (i = 0; i < ar->n_elms; ++i)                                    \
    {                                                                   \
        if (ar->elms[i] == elm)                                         \
        {                                                               \
            MOO_IP_ARRAY_REMOVE_INDEX (ar, elms, i);                    \
            free_elm (elm);                                             \
            return;                                                     \
        }                                                               \
    }                                                                   \
}                                                                       \
                                                                        \
void array_type##_clear (ArrayType *ar)                                 \
{                                                                       \
    g_return_if_fail (ar != NULL);                                      \
                                                                        \
    if (ar->n_elms)                                                     \
    {                                                                   \
        gsize i;                                                        \
        gsize n_elms = ar->n_elms;                                      \
        ElmType **elms = ar->elms;                                      \
        MOO_IP_ARRAY_INIT (ElmType*, ar, elms, 0);                      \
                                                                        \
        for (i = 0; i < n_elms; ++i)                                    \
            free_elm (elms[i]);                                         \
                                                                        \
        g_free (elms);                                                  \
    }                                                                   \
}                                                                       \
                                                                        \
static int                                                              \
array_type##_gcompare_data_func (gconstpointer a,                       \
                                 gconstpointer b,                       \
                                 gpointer      user_data)               \
{                                                                       \
    GCompareFunc func = (GCompareFunc) user_data;                       \
    ElmType **ea = (ElmType **) a;                                      \
    ElmType **eb = (ElmType **) b;                                      \
    return func (*ea, *eb);                                             \
}                                                                       \
                                                                        \
void                                                                    \
array_type##_sort (ArrayType *ar, GCompareFunc func)                    \
{                                                                       \
    g_return_if_fail (ar != NULL && func != NULL);                      \
                                                                        \
    if (ar->n_elms <= 1)                                                \
        return;                                                         \
                                                                        \
    g_qsort_with_data (ar->elms, ar->n_elms, sizeof (*ar->elms),        \
                       array_type##_gcompare_data_func,                 \
                       (gpointer) func);                                \
}                                                                       \
                                                                        \
gssize array_type##_find (const ArrayType *ar, ElmType *elm)            \
{                                                                       \
    gsize i;                                                            \
    g_return_val_if_fail (ar != NULL && elm != NULL, -1);               \
    for (i = 0; i < ar->n_elms; ++i)                                    \
        if (ar->elms[i] == elm)                                         \
            return i;                                                   \
    return -1;                                                          \
}

#define MOO_DEFINE_PTR_ARRAY_C(Element, element)                        \
    MOO_DEFINE_PTR_ARRAY_FULL (Element##Array, element##_array, Element, element##_copy, element##_free)

#define MOO_DECLARE_OBJECT_ARRAY_FULL(ArrayType, array_type, ElmType)   \
    MOO_DECLARE_PTR_ARRAY_FULL (ArrayType, array_type, ElmType)

#define MOO_DECLARE_OBJECT_ARRAY(Element, element)                      \
    MOO_DECLARE_OBJECT_ARRAY_FULL (Element##Array, element##_array, Element)

#define MOO_DEFINE_OBJECT_ARRAY_FULL(ArrayType, array_type, ElmType)    \
    G_INLINE_FUNC ElmType *                                             \
    array_type##_ref_elm__ (ElmType *elm)                               \
    {                                                                   \
        return (ElmType*) g_object_ref (elm);                           \
    }                                                                   \
                                                                        \
    G_INLINE_FUNC void                                                  \
    array_type##_unref_elm__ (ElmType *elm)                             \
    {                                                                   \
        g_object_unref (elm);                                           \
    }                                                                   \
                                                                        \
    MOO_DEFINE_PTR_ARRAY_FULL (ArrayType, array_type, ElmType,          \
                               array_type##_ref_elm__,                  \
                               array_type##_unref_elm__)

#define MOO_DEFINE_OBJECT_ARRAY(Element, element)                       \
    MOO_DEFINE_OBJECT_ARRAY_FULL (Element##Array, element##_array, Element)

G_BEGIN_DECLS

MOO_DECLARE_OBJECT_ARRAY_FULL (MooObjectArray, moo_object_array, GObject)
MOO_DECLARE_PTR_ARRAY_FULL (MooPtrArray, moo_ptr_array, gpointer)

G_END_DECLS

#endif /* MOO_ARRAY_H */
