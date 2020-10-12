/*
 *   mootextbtree.c
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

#include "mooedit/mootext-private.h"
#include "mooutils/mooutils-mem.h"


static BTNode  *bt_node_new         (BTNode     *parent,
                                     guint       n_children,
                                     guint       count,
                                     guint       n_marks);
static BTData  *bt_data_new         (BTNode     *parent);
static void     bt_node_free_rec    (BTNode     *node,
                                     GSList    **removed_marks);
static void     bt_data_free        (BTData     *data,
                                     GSList    **removed_marks);

#define NODE_IS_ROOT(node__) (!(node__)->parent)

#if 0 && defined(MOO_DEBUG)
#define WANT_CHECK_INTEGRITY
static void CHECK_INTEGRITY (BTree *tree, gboolean check_capacity);
#else
#define CHECK_INTEGRITY(tree,check_capacity)
#endif


BTree*
_moo_text_btree_new (void)
{
    BTree *tree = g_new0 (BTree, 1);

    tree->stamp = 1;
    tree->depth = 0;
    tree->root = bt_node_new (NULL, 1, 1, 0);
    tree->root->is_bottom = TRUE;
    tree->root->u.data[0] = bt_data_new (tree->root);

    CHECK_INTEGRITY (tree, TRUE);

    return tree;
}


guint
_moo_text_btree_size (BTree *tree)
{
    g_return_val_if_fail (tree != 0, 0);
    return tree->root->count;
}


void
_moo_text_btree_free (BTree *tree)
{
    if (tree)
    {
        bt_node_free_rec (tree->root, NULL);
        g_free (tree);
    }
}


BTData*
_moo_text_btree_get_data (BTree *tree,
                          guint  index_)
{
    BTNode *node;

    g_assert (tree != NULL);
    g_assert (index_ < tree->root->count);

    node = tree->root;

    while (!node->is_bottom)
    {
        BTNode **child;

        for (child = node->u.children; index_ >= (*child)->count; child++)
            index_ -= (*child)->count;

        node = *child;
    }

    return node->u.data[index_];
}


static BTNode*
bt_node_new (BTNode *parent,
             guint   n_children,
             guint   count,
             guint   n_marks)
{
    BTNode *node = g_slice_new0 (BTNode);

    node->parent = parent;
    node->count = count;
    node->n_children = n_children;
    node->n_marks = n_marks;

    return node;
}


static void
bt_node_free_rec (BTNode  *node,
                  GSList **removed_marks)
{
    if (node)
    {
        guint i;

        if (node->is_bottom)
        {
            for (i = 0; i < node->n_children; ++i)
                bt_data_free (node->u.data[i], removed_marks);
        }
        else
        {
            for (i = 0; i < node->n_children; ++i)
                bt_node_free_rec (node->u.children[i], removed_marks);
        }

        g_slice_free (BTNode, node);
    }
}


static BTData*
bt_data_new (BTNode *parent)
{
    BTData *data = g_slice_new0 (BTData);
    data->parent = parent;
    return data;
}


static void
bt_data_free (BTData  *data,
              GSList **removed_marks)
{
    if (data)
    {
        if (data->n_marks)
        {
            guint i;

            for (i = 0; i < data->n_marks; ++i)
            {
                if (removed_marks)
                {
                    *removed_marks = g_slist_prepend (*removed_marks, data->marks[i]);
                    _moo_line_mark_set_line (data->marks[i], NULL, -1, 0);
                }
                else
                {
                    _moo_line_mark_deleted (data->marks[i]);
                    g_object_unref (data->marks[i]);
                }
            }
        }

        g_free (data->marks);
        g_slice_free (BTData, data);
    }
}


static guint
node_get_index (BTNode *node, gpointer child)
{
    guint i;
    for (i = 0; ; ++i)
        if (node->u.children[i] == child)
            return i;
}


static void
node_insert__ (BTNode   *node,
               gpointer  data,
               guint     index)
{
    g_assert (node != NULL);
    g_assert (node->n_children < BTREE_NODE_MAX_CAPACITY);
    g_assert (index <= node->n_children);

    if (index < node->n_children)
        MOO_ELMMOVE (node->u.children + index + 1,
                     node->u.children + index,
                     node->n_children - index);

    node->u.children[index] = data;
    node->n_children++;
}


static void
node_remove__ (BTNode *node, gpointer data)
{
    guint index;

    g_assert (node != NULL);
    g_assert (node->n_children > 0);

    index = node_get_index (node, data);

    if (index + 1 < node->n_children)
        MOO_ELMMOVE (node->u.data + index,
                     node->u.data + index + 1,
                     node->n_children - index - 1);

    node->n_children--;
}


BTData*
_moo_text_btree_insert (BTree   *tree,
                        guint    index_)
{
    BTNode *node, *tmp;
    BTData *data;
    guint index_orig = index_;
    (void) index_orig;

    g_assert (tree != NULL);
    g_assert (index_ <= tree->root->count);

    tree->stamp++;

    node = tree->root;

    while (!node->is_bottom)
    {
        BTNode **child;

        for (child = node->u.children; index_ > (*child)->count; child++)
            index_ -= (*child)->count;

        node = *child;
    }

    g_assert (node->n_children < BTREE_NODE_MAX_CAPACITY);
    g_assert (index_ <= node->n_children);

    data = bt_data_new (node);
    node_insert__ (node, data, index_);

    for (tmp = node; tmp != NULL; tmp = tmp->parent)
        tmp->count++;

    while (node->n_children == BTREE_NODE_MAX_CAPACITY)
    {
        BTNode *new_node;
        guint new_count, i, node_index;

        if (NODE_IS_ROOT (node))
        {
            tree->depth++;
            node->parent = bt_node_new (NULL, 1, node->count, node->n_marks);
            node->parent->u.children[0] = node;
            tree->root = node->parent;
        }

        if (node->is_bottom)
            new_count = BTREE_NODE_MIN_CAPACITY;
        else
            for (new_count = 0, i = BTREE_NODE_MIN_CAPACITY; i < BTREE_NODE_MAX_CAPACITY; ++i)
                new_count += node->u.children[i]->count;

        node_index = node_get_index (node->parent, node);
        g_assert (node_index < node->parent->n_children);

        new_node = bt_node_new (node->parent, BTREE_NODE_MIN_CAPACITY, new_count, 0);
        new_node->is_bottom = node->is_bottom;
        node->count -= new_count;
        node_insert__ (node->parent, new_node, node_index + 1);
        g_assert (node_get_index (node->parent, new_node) == node_index + 1);
        g_assert (node_get_index (node->parent, node) == node_index);
        MOO_ELMCPY (new_node->u.children,
                    node->u.children + BTREE_NODE_MIN_CAPACITY,
                    BTREE_NODE_MIN_CAPACITY);
        node->n_children = BTREE_NODE_MIN_CAPACITY;
        for (i = 0; i < new_node->n_children; ++i)
            new_node->u.children[i]->parent = new_node;

        if (node->n_marks)
        {
            node->n_marks = 0;
            new_node->n_marks = 0;

            for (i = 0; i < node->n_children; ++i)
                node->n_marks += node->u.children[i]->n_marks;
            for (i = 0; i < new_node->n_children; ++i)
                new_node->n_marks += new_node->u.children[i]->n_marks;
        }

        node = node->parent;
    }

    CHECK_INTEGRITY (tree, TRUE);
    g_assert (data == _moo_text_btree_get_data (tree, index_orig));

    return data;
}


static void
merge_nodes (BTNode *parent, guint first)
{
    BTNode *node, *next;
    guint i;

    g_assert (first + 1 < parent->n_children);

    node = parent->u.children[first];
    next = parent->u.children[first+1];
    g_assert (node->n_children + next->n_children < BTREE_NODE_MAX_CAPACITY);

    MOO_ELMCPY (node->u.children + node->n_children,
                next->u.children, next->n_children);

    for (i = node->n_children; i < (guint) node->n_children + (guint) next->n_children; ++i)
        node->u.children[i]->parent = node;

    node->n_children += next->n_children;
    node->count += next->count;
    node->n_marks += next->n_marks;

    node_remove__ (parent, next);
    g_slice_free (BTNode, next);
}


static void
_moo_text_btree_delete (BTree          *tree,
                        guint           index_,
                        GSList        **deleted_marks)
{
    BTNode *node, *tmp;
    BTData *data;

    g_assert (tree != NULL);
    g_assert (tree->root->count > 1);
    g_assert (index_ < tree->root->count);

    tree->stamp++;

    data = _moo_text_btree_get_data (tree, index_);
    g_assert (data != NULL);

    node = data->parent;
    g_assert (node->count == node->n_children);
    node_remove__ (node, data);

    for (tmp = node; tmp != NULL; tmp = tmp->parent)
    {
        tmp->count--;
        g_assert (tmp->n_marks >= data->n_marks);
        tmp->n_marks -= data->n_marks;
    }

    bt_data_free (data, deleted_marks);

    while (node->n_children < BTREE_NODE_MIN_CAPACITY)
    {
        guint node_index;
        BTNode *parent = node->parent;

        if (!parent)
        {
            if (node->n_children > 1 || node->is_bottom)
                break;

            tree->depth--;
            tree->root = node->u.children[0];
            tree->root->parent = NULL;
            g_slice_free (BTNode, node);
            break;
        }
        else if (parent->n_children == 1)
        {
            g_assert (NODE_IS_ROOT (parent));
            tree->depth--;
            tree->root = node;
            node->parent = NULL;
            g_slice_free (BTNode, parent);
            break;
        }
        else
        {
            node_index = node_get_index (parent, node);

            if (node_index)
            {
                BTNode *sib = parent->u.children[node_index-1];

                if (sib->n_children > BTREE_NODE_MIN_CAPACITY)
                {
                    BTNode *child = sib->u.children[sib->n_children-1];
                    node_insert__ (node, child, 0);
                    node_remove__ (sib, child);
                    child->parent = node;

                    node->n_marks += child->n_marks;
                    sib->n_marks -= child->n_marks;

                    if (!node->is_bottom)
                    {
                        node->count += child->count;
                        sib->count -= child->count;
                    }
                    else
                    {
                        node->count++;
                        sib->count--;
                    }
                }
                else
                {
                    merge_nodes (parent, node_index-1);
                }
            }
            else
            {
                BTNode *sib = parent->u.children[1];

                if (sib->n_children > BTREE_NODE_MIN_CAPACITY)
                {
                    BTNode *child = sib->u.children[0];
                    node_insert__ (node, child, node->n_children);
                    node_remove__ (sib, child);
                    g_assert (node->n_children == BTREE_NODE_MIN_CAPACITY);
                    child->parent = node;

                    node->n_marks += child->n_marks;
                    sib->n_marks -= child->n_marks;

                    if (!node->is_bottom)
                    {
                        node->count += child->count;
                        sib->count -= child->count;
                    }
                    else
                    {
                        node->count++;
                        sib->count--;
                    }
                }
                else
                {
                    merge_nodes (parent, 0);
                }
            }
        }

        node = parent;
    }

    CHECK_INTEGRITY (tree, TRUE);
}


/* XXX */
void
_moo_text_btree_insert_range (BTree      *tree,
                              int         first,
                              int         num)
{
    int i;

    g_assert (tree != NULL);
    g_assert (first >= 0 && first <= (int) tree->root->count);
    g_assert (num > 0);

    for (i = 0; i < num; ++i)
        _moo_text_btree_insert (tree, first);
}


/* XXX */
void
_moo_text_btree_delete_range (BTree      *tree,
                              int         first,
                              int         num,
                              GSList    **deleted_marks)
{
    int i;

    g_assert (tree != NULL);
    g_assert (first >= 0 && first < (int) tree->root->count);
    g_assert (num > 0 && first + num <= (int) tree->root->count);

    for (i = 0; i < num; ++i)
        _moo_text_btree_delete (tree, first, deleted_marks);
}


void
_moo_text_btree_update_n_marks (G_GNUC_UNUSED BTree *tree,
                                BTData     *data,
                                int         add)
{
    BTNode *node;

    for (node = (BTNode*) data; node != NULL; node = node->parent)
    {
        g_assert (add > 0 || (add < 0 && (int) node->n_marks >= -add));
        node->n_marks += add;
    }

    CHECK_INTEGRITY (tree, FALSE);
}


#ifdef WANT_CHECK_INTEGRITY

static void
node_check_count (BTNode *node)
{
    guint real_count = 0, mark_count = 0, i;

    if (node->is_bottom)
    {
        real_count = node->n_children;
        mark_count = node->n_marks;
    }
    else
    {
        for (i = 0; i < node->n_children; ++i)
        {
            real_count += node->u.children[i]->count;
            mark_count += node->u.children[i]->n_marks;
        }
    }

    g_assert (real_count == node->count);
    g_assert (mark_count == node->n_marks);
}


static void
node_check (BTNode *node, gboolean is_root, gboolean check_capacity)
{
    guint i;

    if (is_root)
    {
        g_assert (node->parent == NULL);
        g_assert (node->n_children >= 1);
    }
    else
    {
        g_assert (node->parent != NULL);
        if (check_capacity)
            g_assert (node->n_children >= BTREE_NODE_MIN_CAPACITY);
        else
            g_assert (node->n_children >= 1);
    }

    if (check_capacity)
        g_assert (node->n_children < BTREE_NODE_MAX_CAPACITY);

    g_assert (node->count >= node->n_children);

    if (!is_root)
    {
        guint index = node_get_index (node->parent, node);
        g_assert (index < node->parent->n_children);
    }

    node_check_count (node);

    if (!node->is_bottom)
        for (i = 0; i < node->n_children; ++i)
            node_check (node->u.children[i], FALSE, check_capacity);
}


static void CHECK_INTEGRITY (BTree *tree, gboolean check_capacity)
{
    guint i, p;
    BTNode *node;

    g_assert (tree != NULL);
    g_assert (tree->root != NULL);
    g_assert (tree->root->count != 0);

    for (i = 0, p = 1; i < BTREE_MAX_DEPTH - 1; ++i, p *= BTREE_NODE_MIN_CAPACITY) ;
    g_assert (p > 10000000);

    for (i = 0, node = tree->root; i < tree->depth; ++i, node = node->u.children[0])
        g_assert (!node->is_bottom);
    g_assert (node->is_bottom);

    node_check (tree->root, TRUE, check_capacity);
}

#endif /* WANT_CHECK_INTEGRITY */
