/*
 *   mootextbtree.h
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

#ifndef MOO_TEXT_BTREE_H
#define MOO_TEXT_BTREE_H

#include <mooglib/moo-glib.h>

G_BEGIN_DECLS


#define BTREE_NODE_EXP 4
#define BTREE_NODE_MAX_CAPACITY (1 << BTREE_NODE_EXP)
#define BTREE_NODE_MIN_CAPACITY (BTREE_NODE_MAX_CAPACITY >> 1)
#define BTREE_MAX_DEPTH 9 /* 2^(3*(9-1)) == 2^24 > 16,777,216 - more than enough */
#define BTREE_MAX_DEPTH_EXP 4 /* 2^4 > 8 */

typedef struct BTNode BTNode;
typedef struct BTData BTData;
typedef struct BTIter BTIter;
typedef struct BTree BTree;

struct MooLineMark;


struct BTNode {
    BTNode *parent;
    guint n_marks;

    union {
        BTNode *children[BTREE_NODE_MAX_CAPACITY];
        BTData *data[BTREE_NODE_MAX_CAPACITY];
    } u;

    guint n_children : BTREE_NODE_EXP + 1;
    guint is_bottom : 1;
    guint count : (30 - BTREE_NODE_EXP);
};

struct BTData {
    BTNode *parent;
    guint n_marks;

    struct MooLineMark **marks;
};

struct BTree {
    BTNode *root;
    guint depth;
    guint stamp;
};


BTree      *_moo_text_btree_new             (void);
void        _moo_text_btree_free            (BTree      *tree);

guint       _moo_text_btree_size            (BTree      *tree);

BTData     *_moo_text_btree_get_data        (BTree      *tree,
                                             guint       index_);

BTData     *_moo_text_btree_insert          (BTree      *tree,
                                             guint       index_);

void        _moo_text_btree_insert_range    (BTree      *tree,
                                             int         first,
                                             int         num);
void        _moo_text_btree_delete_range    (BTree      *tree,
                                             int         first,
                                             int         num,
                                             GSList    **removed_marks);

void        _moo_text_btree_update_n_marks  (BTree      *tree,
                                             BTData     *data,
                                             int         add);


G_END_DECLS

#endif /* MOO_TEXT_BTREE_H */
