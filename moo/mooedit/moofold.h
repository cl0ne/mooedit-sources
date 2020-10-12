/*
 *   moofold.h
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

#ifndef MOO_FOLD_H
#define MOO_FOLD_H

#include <mooedit/moolinemark.h>

G_BEGIN_DECLS


#define MOO_FOLD_TAG "moo-fold-invisible"

typedef struct MooFoldTree MooFoldTree;

struct MooFoldTree
{
    MooFold *folds;
    guint n_folds;
    MooTextBuffer *buffer;
    guint consistent : 1;
};

struct MooFold
{
    GObject object;

    MooFold *parent;
    MooFold *prev;
    MooFold *next;
    MooFold *children;  /* chlidren are sorted by line */
    guint n_children;

    MooLineMark *start;
    MooLineMark *end;   /* may be NULL */

    guint collapsed : 1;
    guint deleted : 1;  /* alive just because of reference count */
};

struct MooFoldClass
{
    GObjectClass object_class;
};


int          _moo_fold_get_start    (MooFold        *fold);

gboolean     _moo_fold_is_deleted   (MooFold        *fold);

MooFoldTree *_moo_fold_tree_new     (MooTextBuffer  *buffer);
void         _moo_fold_tree_free    (MooFoldTree    *tree);

MooFold     *_moo_fold_tree_add     (MooFoldTree    *tree,
                                     int             first_line,
                                     int             last_line);
void         _moo_fold_tree_remove  (MooFoldTree    *tree,
                                     MooFold        *fold);
GSList      *_moo_fold_tree_get     (MooFoldTree    *tree,
                                     int             first_line,
                                     int             last_line);

void         _moo_fold_tree_expand          (MooFoldTree    *tree,
                                             MooFold        *fold);
void         _moo_fold_tree_collapse        (MooFoldTree    *tree,
                                             MooFold        *fold);
gboolean     _moo_fold_tree_toggle          (MooFoldTree    *tree);
gboolean     _moo_fold_tree_expand_all      (MooFoldTree    *tree);
gboolean     _moo_fold_tree_collapse_all    (MooFoldTree    *tree);


G_END_DECLS

#endif /* MOO_FOLD_H */
