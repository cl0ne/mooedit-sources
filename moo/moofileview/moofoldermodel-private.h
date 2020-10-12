/*
 *   moofoldermodel-private.h
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

#ifndef MOO_FOLDER_MODEL_PRIVATE_H
#define MOO_FOLDER_MODEL_PRIVATE_H

#include <mooglib/moo-glib.h>
#include <string.h>

G_BEGIN_DECLS

typedef struct _FileList FileList;

typedef int (*MooFileCmp) (MooFile *file1, MooFile *file2);

struct _FileList {
    GList       *list;                  /* MooFile*, sorted by name */
    int          size;
    GHashTable  *name_to_file;          /* char* -> MooFile* */
    GHashTable  *display_name_to_file;  /* char* -> MooFile* */
    GHashTable  *file_to_link;          /* MooFile* -> GList* */
    MooFileCmp   cmp_func;
};


static FileList *file_list_new          (MooFileCmp  cmp_func);
static void      file_list_destroy      (FileList   *flist);

static void      file_list_set_cmp_func (FileList   *flist,
                                         MooFileCmp  cmp_func,
                                         int       **new_order);

static int       file_list_add          (FileList   *flist,
                                         MooFile    *file);
static int       file_list_remove       (FileList   *flist,
                                         MooFile    *file);

static MooFile  *file_list_nth          (FileList   *flist,
                                         int         index_);
static gboolean  file_list_contains     (FileList   *flist,
                                         MooFile    *file);
static int       file_list_position     (FileList   *flist,
                                         MooFile    *file);

static MooFile  *file_list_find_name    (FileList   *flist,
                                         const char *name);
static MooFile  *file_list_find_display_name
                                        (FileList   *flist,
                                         const char *display_name);

static MooFile  *file_list_first        (FileList   *flist);
static MooFile  *file_list_next         (FileList   *flist,
                                         MooFile    *file);

static GSList   *file_list_get_slist    (FileList   *flist);

static void      _list_sort             (GList         **list,
                                         guint           list_len,
                                         MooFileCmp      cmp_func,
                                         int           **new_order);
static GList    *_list_insert_sorted    (GList         **list,
                                         int            *list_len,
                                         MooFileCmp      cmp_func,
                                         MooFile        *file,
                                         int            *position);
static void      _list_delete_link      (GList         **list,
                                         GList          *link,
                                         int            *list_len);
static GList    *_list_find             (FileList       *flist,
                                         MooFile        *file,
                                         int            *index_);

static void      _hash_table_insert     (FileList       *flist,
                                         MooFile        *file,
                                         GList          *link);
static void      _hash_table_remove     (FileList       *flist,
                                         MooFile        *file);


#ifdef MOO_DEBUG
#if 0
#define DEFINE_CHECK_FILE_LIST_INTEGRITY
static void CHECK_FILE_LIST_INTEGRITY (FileList *flist)
{
    GList *link;

    g_assert ((int)g_list_length (flist->list) == flist->size);
    g_assert ((int)g_hash_table_size (flist->name_to_file) == flist->size);
    g_assert ((int)g_hash_table_size (flist->display_name_to_file) == flist->size);
    g_assert ((int)g_hash_table_size (flist->file_to_link) == flist->size);

    for (link = flist->list; link != NULL; link = link->next)
    {
        MooFile *file = link->data;
        g_assert (file != NULL);
        g_assert (file == g_hash_table_lookup (flist->name_to_file,
                _moo_file_name (file)));
        g_assert (file == g_hash_table_lookup (flist->display_name_to_file,
                _moo_file_display_name (file)));
        g_assert (link == g_hash_table_lookup (flist->file_to_link, file));
    }
}
#endif
#endif /* MOO_DEBUG */

#ifndef DEFINE_CHECK_FILE_LIST_INTEGRITY
#define CHECK_FILE_LIST_INTEGRITY(flist)
#endif


static FileList *file_list_new          (MooFileCmp cmp_func)
{
    FileList *flist = g_new0 (FileList, 1);

    flist->name_to_file =
            g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    flist->display_name_to_file =
            g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    flist->file_to_link = g_hash_table_new (g_direct_hash, g_direct_equal);
    flist->cmp_func = cmp_func;

    CHECK_FILE_LIST_INTEGRITY (flist);

    return flist;
}


static void      file_list_destroy      (FileList   *flist)
{
    g_return_if_fail (flist != NULL);

    g_hash_table_destroy (flist->display_name_to_file);
    g_hash_table_destroy (flist->name_to_file);
    g_hash_table_destroy (flist->file_to_link);

    g_list_foreach (flist->list, (GFunc) _moo_file_unref, NULL);
    g_list_free (flist->list);

    g_free (flist);
}


static int       file_list_add          (FileList   *flist,
                                         MooFile    *file)
{
    int index_;
    GList *link;

    link = _list_insert_sorted (&flist->list,
                                &flist->size,
                                flist->cmp_func,
                                _moo_file_ref (file),
                                &index_);
    _hash_table_insert (flist, file, link);

    CHECK_FILE_LIST_INTEGRITY (flist);

    return index_;
}


static int       file_list_remove       (FileList   *flist,
                                         MooFile    *file)
{
    int index_ = 0;
    GList *link;

    link = _list_find (flist, file, &index_);
    g_assert (link != NULL);

    _hash_table_remove (flist, file);
    _list_delete_link (&flist->list, link, &flist->size);

    CHECK_FILE_LIST_INTEGRITY (flist);

    _moo_file_unref (file);
    return index_;
}


static gboolean  file_list_contains     (FileList   *flist,
                                         MooFile    *file)
{
    return g_hash_table_lookup (flist->file_to_link, file) != NULL;
}


static MooFile  *file_list_nth          (FileList   *flist,
                                         int         index_)
{
    MooFile *file;
    g_assert (0 <= index_ && index_ < flist->size);
    file = g_list_nth_data (flist->list, index_);
    g_assert (file != NULL);
    return file;
}


/* TODO */
static int       file_list_position     (FileList   *flist,
                                         MooFile    *file)
{
    GList *link;
    int position;
    g_assert (file != NULL);
    link = g_hash_table_lookup (flist->file_to_link, file);
    g_assert (link != NULL);
    position = g_list_position (flist->list, link);
    g_assert (position >= 0);
    return position;
}


static MooFile  *file_list_find_name    (FileList   *flist,
                                         const char *name)
{
    return g_hash_table_lookup (flist->name_to_file, name);
}


static MooFile  *file_list_find_display_name
                                        (FileList   *flist,
                                         const char *display_name)
{
    return g_hash_table_lookup (flist->display_name_to_file,
                                display_name);
}


static MooFile  *file_list_first        (FileList   *flist)
{
    if (flist->list)
        return flist->list->data;
    else
        return NULL;
}


static MooFile  *file_list_next         (FileList   *flist,
                                         MooFile    *file)
{
    GList *link = g_hash_table_lookup (flist->file_to_link, file);
    g_assert (link != NULL);
    if (link->next)
        return link->next->data;
    else
        return NULL;
}


static GSList   *file_list_get_slist    (FileList   *flist)
{
    GList *l;
    GSList *slist = NULL;

    for (l = flist->list; l != NULL; l = l->next)
        slist = g_slist_prepend (slist, _moo_file_ref (l->data));

    return g_slist_reverse (slist);
}


static void      file_list_set_cmp_func (FileList   *flist,
                                         MooFileCmp  cmp_func,
                                         int       **new_order)
{
    flist->cmp_func = cmp_func;

    if (flist->size)
    {
        _list_sort (&flist->list, flist->size, cmp_func, new_order);
        CHECK_FILE_LIST_INTEGRITY (flist);
    }
}


static void      _hash_table_insert     (FileList       *flist,
                                         MooFile        *file,
                                         GList          *link)
{
    g_hash_table_insert (flist->file_to_link, file, link);
    g_hash_table_insert (flist->name_to_file,
                         g_strdup (_moo_file_name (file)),
                         file);
    g_hash_table_insert (flist->display_name_to_file,
                         g_strdup (_moo_file_display_name (file)),
                         file);
}


static void      _hash_table_remove     (FileList       *flist,
                                         MooFile        *file)
{
    g_hash_table_remove (flist->file_to_link, file);
    g_hash_table_remove (flist->name_to_file,
                         _moo_file_name (file));
    g_hash_table_remove (flist->display_name_to_file,
                         _moo_file_display_name (file));
}


static void      _list_delete_link      (GList         **list,
                                         GList          *link,
                                         int            *list_len)
{
    g_assert (*list_len == (int)g_list_length (*list));
    *list = g_list_delete_link (*list, link);
    (*list_len)--;
    g_assert (*list_len == (int)g_list_length (*list));
}


/* TODO */
static GList    *_list_find             (FileList       *flist,
                                         MooFile        *file,
                                         int            *index_)
{
    GList *link = g_hash_table_lookup (flist->file_to_link, file);
    g_return_val_if_fail (link != NULL, NULL);
    *index_ = g_list_position (flist->list, link);
    g_return_val_if_fail (*index_ >= 0, NULL);
    return link;
}


static int
moo_file_case_cmp (MooFile *f1,
                   MooFile *f2)
{
    if (!strcmp (_moo_file_name (f1), ".."))
        return strcmp (_moo_file_name (f2), "..") ? -1 : 0;
    else if (!strcmp (_moo_file_name (f2), ".."))
        return 1;
    else if (MOO_FILE_IS_DIR (f1) && !MOO_FILE_IS_DIR (f2))
        return -1;
    else if (!MOO_FILE_IS_DIR (f1) && MOO_FILE_IS_DIR (f2))
        return 1;
    else
        return _moo_collation_key_cmp (_moo_file_collation_key (f1),
                                       _moo_file_collation_key (f2));
}

static int
moo_file_cmp (MooFile *f1,
              MooFile *f2)
{
    if (!strcmp (_moo_file_name (f1), ".."))
        return strcmp (_moo_file_name (f2), "..") ? -1 : 0;
    else if (!strcmp (_moo_file_name (f2), ".."))
        return 1;
    else if (MOO_FILE_IS_DIR (f1) && !MOO_FILE_IS_DIR (f2))
        return -1;
    else if (!MOO_FILE_IS_DIR (f1) && MOO_FILE_IS_DIR (f2))
        return 1;
    else
        return strcmp (_moo_file_display_name (f1),
                       _moo_file_display_name (f2));
}

static int
moo_file_case_cmp_fi (MooFile *f1,
                      MooFile *f2)
{
    if (!strcmp (_moo_file_name (f1), ".."))
        return strcmp (_moo_file_name (f2), "..") ? -1 : 0;
    else if (!strcmp (_moo_file_name (f2), ".."))
        return 1;
    else
        return _moo_collation_key_cmp (_moo_file_collation_key (f1),
                                       _moo_file_collation_key (f2));
}

static int
moo_file_cmp_fi (MooFile *f1,
                 MooFile *f2)
{
    if (!strcmp (_moo_file_name (f1), ".."))
        return strcmp (_moo_file_name (f2), "..") ? -1 : 0;
    else if (!strcmp (_moo_file_name (f2), ".."))
        return 1;
    else
        return strcmp (_moo_file_display_name (f1),
                       _moo_file_display_name (f2));
}


/* TODO XXX cmp_func may return 0. what then? */
static void     _find_insert_position   (GList          *list,
                                         int             list_len,
                                         MooFileCmp      cmp_func,
                                         MooFile        *file,
                                         GList         **prev,
                                         GList         **next,
                                         int            *position)
{
    GList *left = NULL, *right = NULL;
    int pos = -1;

    while (list_len)
    {
        int cmp;

        if (!left)
        {
            left = list;
            pos = 0;
        }

        cmp = cmp_func (file, left->data);
        g_assert (cmp != 0);

        if (cmp < 0)
        {
            right = left;
            left = left->prev;
            pos--;
            break;
        }
        else
        {
            if (list_len == 1)
            {
                right = left->next;
                break;
            }
            else if (list_len == 2)
            {
                g_assert (left->next != NULL);
                cmp = cmp_func (file, left->next->data);
                g_assert (cmp != 0);

                if (cmp < 0)
                {
                    right = left->next;
                }
                else
                {
                    right = left->next->next;
                    left = left->next;
                    pos++;
                }

                break;
            }
            else if (list_len % 2)
            {
                right = g_list_nth (left, list_len / 2);
                g_assert (right != NULL);

                cmp = cmp_func (file, right->data);
                g_assert (cmp != 0);

                if (cmp > 0)
                {
                    left = right;
                    pos += list_len / 2;
                }

                list_len = list_len / 2 + 1;
            }
            else
            {
                right = g_list_nth (left, list_len / 2);
                g_assert (right != NULL);

                cmp = cmp_func (file, right->data);
                g_assert (cmp != 0);

                if (cmp < 0)
                {
                    list_len = list_len / 2 + 1;
                }
                else
                {
                    left = right;
                    pos += list_len / 2;
                    list_len = list_len / 2;
                }
            }
        }
    }

    *next = right;
    *prev = left;
    *position = pos + 1;
}

/* TODO */
static GList    *_list_insert_sorted    (GList         **list,
                                         int            *list_len,
                                         MooFileCmp      cmp_func,
                                         MooFile        *file,
                                         int            *position)
{
    GList *link;
    GList *prev, *next;

    g_assert (*list_len == (int)g_list_length (*list));
    g_assert (g_list_find (*list, file) == NULL);

    _find_insert_position (*list, *list_len, cmp_func, file, &prev, &next, position);
    g_assert (*position >= 0);

    link = g_list_alloc ();
    link->data = file;
    link->prev = prev;
    link->next = next;

    if (prev)
        prev->next = link;
    else
        *list = link;

    if (next)
        next->prev = link;

    (*list_len)++;
    g_assert (g_list_nth (*list, *position) == link);
    g_assert (*list_len == (int)g_list_length (*list));
    return link;
}


static int       _compare_links         (int            *a,
                                         int            *b,
                                         gpointer        user_data)
{
    struct {
        MooFileCmp cmp_func;
        GList **links;
    } *data = user_data;

    return data->cmp_func (data->links[*a]->data, data->links[*b]->data);
}


static void      _list_sort             (GList         **list,
                                         guint           list_len,
                                         MooFileCmp      cmp_func,
                                         int           **new_order_p)
{
    guint i;
    GList **links;
    int *order;
    GList *l;
    struct {
        MooFileCmp cmp_func;
        GList **links;
    } data;

    g_return_if_fail (list_len != 0);

    links = g_new (GList*, list_len);
    order = g_new (int, list_len);

    for (i = 0, l = *list; i < list_len; ++i, l = l->next)
    {
        g_assert (i > 0 || l->prev == NULL);
        g_assert (i < list_len - 1 || l->next == NULL);
        g_assert (i == 0 || i == list_len - 1 || (l->next != NULL && l->prev != NULL));

        order[i] = i;
        links[i] = l;
    }

    data.cmp_func = cmp_func;
    data.links = links;
    g_qsort_with_data (order, list_len, sizeof (int),
                       (GCompareDataFunc) _compare_links, &data);

    for (i = 0; i < list_len; ++i)
    {
        if (i == 0)
            links[i]->prev = NULL;
        else
            links[i]->prev = links[i-1];
        if (i == list_len - 1)
            links[i]->next = NULL;
        else
            links[i]->next = links[i+1];
    }

    *list = links[0];
    *new_order_p = order;

    g_free (links);
}


G_END_DECLS

#endif /* MOO_FOLDER_MODEL_PRIVATE_H */
