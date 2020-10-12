/*
 *   moofold.c
 *
 *   Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
 *   Copyright (C) 2008 by Daniel Poelzleithner <mooedit@poelzi.org>
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
#include "marshals.h"


#ifdef MOO_DEBUG
#define WANT_CHECKS 1
#else
#define WANT_CHECKS 0
#endif

static void     moo_fold_get_property       (GObject    *object,
                                             guint       prop_id,
                                             GValue     *value,
                                             GParamSpec *pspec);
static void     moo_fold_finalize           (GObject    *object);
static void     moo_fold_free_recursively   (MooFold    *fold);

static int      _moo_fold_get_end           (MooFold    *fold);

enum {
    PROP_0,
    PROP_PARENT,
    PROP_NEXT,
    PROP_PREV,
    PROP_CHILDREN,
    PROP_N_CHILDREN,
    PROP_MARK_START,
    PROP_MARK_END,
    PROP_COLLAPSED
};

/* MOO_TYPE_FOLD */
G_DEFINE_TYPE (MooFold, moo_fold, G_TYPE_OBJECT)

static void
moo_fold_class_init (MooFoldClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = moo_fold_finalize;

    gobject_class->get_property = moo_fold_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_PARENT,
                                     g_param_spec_object ("parent",
                                             "parent",
                                             "parent",
                                             MOO_TYPE_FOLD,
                                             G_PARAM_READABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_NEXT,
                                     g_param_spec_object ("next",
                                             "next",
                                             "next",
                                             MOO_TYPE_FOLD,
                                             G_PARAM_READABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_PREV,
                                     g_param_spec_object ("previous",
                                             "previous",
                                             "previous",
                                             MOO_TYPE_FOLD,
                                             G_PARAM_READABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_MARK_START,
                                     g_param_spec_object ("mark_start",
                                             "Start Marker",
                                             "Marker of the start Marker",
                                             MOO_TYPE_LINE_MARK,
                                             G_PARAM_READABLE));


    g_object_class_install_property (gobject_class,
                                     PROP_MARK_END,
                                     g_param_spec_object ("mark_end",
                                             "End Marker",
                                             "Marker of the end Marker",
                                             MOO_TYPE_LINE_MARK,
                                             G_PARAM_READABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_CHILDREN,
                                     g_param_spec_object ("children",
                                             "Children",
                                             "Children ordered by Line",
                                             MOO_TYPE_FOLD,
                                             G_PARAM_READABLE));

    g_object_class_install_property (gobject_class,
                                     PROP_N_CHILDREN,
                                     g_param_spec_int ("n_children",
                                             "Children Number",
                                             "Number of children",
                                             0, G_MAXINT, 0,
                                             G_PARAM_READABLE));


    g_object_class_install_property (gobject_class,
                                     PROP_COLLAPSED,
                                     g_param_spec_boolean ("collapsed",
                                             "Fold Collapsed",
                                             "True if the fold is collapsed",
                                             FALSE,
                                             G_PARAM_READABLE));
}

static void
moo_fold_get_property (GObject        *object,
                            guint           prop_id,
                            GValue         *value,
                            GParamSpec     *pspec)
{
    MooFold *fold = MOO_FOLD (object);

    switch (prop_id)
    {
        case PROP_PARENT:
            g_value_set_object (value, fold->parent);
            break;

        case PROP_NEXT:
            g_value_set_object (value, fold->next);
            break;

        case PROP_PREV:
            g_value_set_object (value, fold->prev);
            break;

        case PROP_MARK_START:
            g_value_set_object (value, fold->start);
            break;

        case PROP_CHILDREN:
            g_value_set_object (value, fold->children);
            break;

        case PROP_N_CHILDREN:
            g_value_set_int (value, fold->n_children);
            break;

        case PROP_MARK_END:
            g_value_set_object (value, fold->end);
            break;

        case PROP_COLLAPSED:
            g_value_set_boolean (value, fold->collapsed);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_fold_init (MooFold *fold)
{
    fold->deleted = TRUE;
}


static void
moo_fold_finalize (GObject *object)
{
    MooFold *fold = MOO_FOLD (object);

    if (!fold->deleted)
        g_critical ("oops, crash pending...");

    G_OBJECT_CLASS (moo_fold_parent_class)->finalize (object);
}


static void
moo_fold_free_recursively (MooFold *fold)
{
    MooFold *child;

    g_return_if_fail (MOO_IS_FOLD (fold));

    fold->deleted = TRUE;

    child = fold->children;
    fold->children = NULL;

    for ( ; child != NULL; child = child->next)
        moo_fold_free_recursively (child);

    if (fold->start)
        g_object_unref (fold->start);
    if (fold->end)
        g_object_unref (fold->end);
    g_object_unref (fold);
}


int
_moo_fold_get_start (MooFold *fold)
{
    g_return_val_if_fail (MOO_IS_FOLD (fold), -1);
    g_return_val_if_fail (!_moo_fold_is_deleted (fold), -1);
    return moo_line_mark_get_line (fold->start);
}


static int
_moo_fold_get_end (MooFold *fold)
{
    g_return_val_if_fail (MOO_IS_FOLD (fold), -1);
    g_return_val_if_fail (!_moo_fold_is_deleted (fold), -1);
    return moo_line_mark_get_line (fold->end);
}


gboolean
_moo_fold_is_deleted (MooFold *fold)
{
    g_return_val_if_fail (MOO_IS_FOLD (fold), TRUE);
    return fold->deleted != 0;
}


#if WANT_CHECKS

static void
CHECK_LIST_MEMBER__ (MooFold *list,
                     MooFold *fold)
{
    while (list)
    {
        if (list == fold)
            return;
        list = list->next;
    }

    g_assert_not_reached ();
}


static void
CHECK_FOLD (MooFoldTree *tree,
            MooFold     *fold)
{
    g_assert (tree != NULL);
    g_assert (MOO_IS_FOLD (fold));
    g_assert (!fold->deleted);

    while (fold->parent)
    {
        CHECK_LIST_MEMBER__ (fold->parent->children, fold);
        fold = fold->parent;
    }

    CHECK_LIST_MEMBER__ (tree->folds, fold);
}

#else /* WANT_CHECKS */
#define CHECK_FOLD(tree,fold)
#endif


MooFoldTree *
_moo_fold_tree_new (MooTextBuffer *buffer)
{
    MooFoldTree *tree = g_new0 (MooFoldTree, 1);

    tree->buffer = buffer;
    tree->consistent = TRUE;

    return tree;
}


void
_moo_fold_tree_free (MooFoldTree *tree)
{
    MooFold *child;

    g_return_if_fail (tree != NULL);

    for (child = tree->folds; child != NULL; child = child->next)
        moo_fold_free_recursively (child);

    g_free (tree);
}


inline static int
get_line_count (MooFoldTree *tree)
{
    return gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (tree->buffer));
}


static MooLineMark *
fold_mark_new (MooFold *fold)
{
    MooLineMark *mark = MOO_LINE_MARK (g_object_new (MOO_TYPE_LINE_MARK, (const char*) NULL));
    _moo_line_mark_set_fold (mark, fold);
    return mark;
}


static MooFold *
insert_fold (MooFoldTree *tree,
             MooFold     *parent,
             int          first_line,
             int          last_line)
{
    MooFold *fold, *last, *new_fold;

    g_assert (!parent || MOO_IS_FOLD (parent));
    g_assert (!parent || _moo_fold_get_start (parent) < first_line);
    g_assert (!parent || _moo_fold_get_end (parent) >= last_line);

    last = NULL;

    for (fold = (parent ? parent->children : tree->folds); fold != NULL; fold = fold->next)
    {
        int start, end;

        start = _moo_fold_get_start (fold);
        end = _moo_fold_get_end (fold);

        if (!fold->next)
            last = fold;

        if (end < first_line)
            continue;

        if (start > last_line)
            break;

        if (start == first_line || end == first_line || start == last_line)
            return NULL;

        if (start < first_line)
        {
            if (end < last_line)
                return NULL;

            /* new fold is inserted into the old one */
            return insert_fold (tree, fold, first_line, last_line);
        }
        else /* start > first_line */
        {
            if (end > last_line)
                return NULL;

            /* old fold is inserted into the new one */

            new_fold = MOO_FOLD (g_object_new (MOO_TYPE_FOLD, (const char*) NULL));
            new_fold->deleted = FALSE;

            if (parent)
            {
                if (!parent->children || fold == parent->children)
                    parent->children = new_fold;
            }
            else
            {
                if (!tree->folds || fold == tree->folds)
                    tree->folds = new_fold;
            }

            new_fold->parent = parent;
            fold->parent = new_fold;
            new_fold->children = fold;
            new_fold->n_children = 1;

            if (fold->next)
            {
                fold->next->prev = new_fold;
                new_fold->next = fold->next;
                fold->next = NULL;
            }

            if (fold->prev)
            {
                fold->prev->next = new_fold;
                new_fold->prev = fold->prev;
                fold->prev = NULL;
            }

            new_fold->start = fold_mark_new (new_fold);
            new_fold->end = fold_mark_new (new_fold);

            moo_text_buffer_add_line_mark (tree->buffer, new_fold->start, first_line);
            moo_text_buffer_add_line_mark (tree->buffer, new_fold->end, last_line);

            CHECK_FOLD (tree, new_fold);
            return new_fold;
        }
    }

    /* insert new fold before <fold> or in the end if <fold> is NULL */

    new_fold = MOO_FOLD (g_object_new (MOO_TYPE_FOLD, (const char*) NULL));
    new_fold->deleted = FALSE;

    new_fold->parent = parent;
    new_fold->next = fold;

    if (parent)
    {
        if (fold == parent->children)
            parent->children = new_fold;
        parent->n_children++;
    }
    else
    {
        if (fold == tree->folds)
            tree->folds = new_fold;
        tree->n_folds++;
    }

    if (fold)
    {
        if (fold->prev)
            new_fold->prev = fold->prev;
        fold->prev = new_fold;
    }
    else if (last)
    {
        g_assert (last->next == NULL);
        last->next = new_fold;
        new_fold->prev = last;
    }

    new_fold->start = fold_mark_new (new_fold);
    new_fold->end = fold_mark_new (new_fold);

    moo_text_buffer_add_line_mark (tree->buffer, new_fold->start, first_line);
    moo_text_buffer_add_line_mark (tree->buffer, new_fold->end, last_line);

    CHECK_FOLD (tree, new_fold);
    return new_fold;
}



MooFold *
_moo_fold_tree_add (MooFoldTree *tree,
                    int          first_line,
                    int          last_line)
{
    g_assert (tree != NULL);
    g_assert (first_line >= 0 && first_line < get_line_count (tree));
    g_assert (last_line >= 0 && last_line < get_line_count (tree));
    g_assert (last_line > first_line);
    return insert_fold (tree, NULL, first_line, last_line);
}


static void
fold_free (MooFold *fold)
{
    fold->deleted = TRUE;

    if (fold->start)
    {
        _moo_line_mark_set_fold (fold->start, NULL);

        if (!moo_line_mark_get_deleted (fold->start))
            moo_text_buffer_delete_line_mark (moo_line_mark_get_buffer (fold->start),
                                              fold->start);

        g_object_unref (fold->start);
        fold->start = NULL;
    }

    if (fold->end)
    {
        _moo_line_mark_set_fold (fold->end, NULL);

        if (!moo_line_mark_get_deleted (fold->end))
            moo_text_buffer_delete_line_mark (moo_line_mark_get_buffer (fold->end),
                                              fold->end);

        g_object_unref (fold->end);
        fold->end = NULL;
    }

    g_object_unref (fold);
}


void
_moo_fold_tree_remove (MooFoldTree *tree,
                       MooFold     *fold)
{
    guint n_children;
    MooFold *children, *parent, *last;

    CHECK_FOLD (tree, fold);

    _moo_fold_tree_expand (tree, fold);

    n_children = fold->n_children;
    last = children = fold->children;
    parent = fold->parent;

    if (children)
    {
        while (TRUE)
        {
            g_assert (last->parent == fold);
            last->parent = fold->parent;
            if (!last->next)
                break;
        }
    }

    g_assert ((last && children) || (!last && !children));

    if (parent)
    {
        if (fold == parent->children)
            parent->children = children ? children : parent->children->next;
        parent->n_children = parent->n_children + n_children - 1;
    }
    else
    {
        if (fold == tree->folds)
            tree->folds = children ? children : tree->folds->next;
        tree->n_folds = tree->n_folds + n_children - 1;
    }

    if (last)
        last->next = fold->next;
    if (fold->next)
        fold->next->prev = last ? last : fold->prev;
    if (children)
        children->prev = fold->prev;
    if (fold->prev)
        fold->prev->next = children ? children : fold->next;

    fold_free (fold);
}


static void
expand_check_visible (MooFoldTree *tree,
                      MooFold     *fold)
{
    GtkTextIter start, end;
    GtkTextBuffer *buffer;
    MooFold *child;

    CHECK_FOLD (tree, fold);

    buffer = GTK_TEXT_BUFFER (tree->buffer);
    gtk_text_buffer_get_iter_at_line (buffer, &start,
                                      _moo_fold_get_start (fold));
    end = start;
    gtk_text_iter_forward_line (&end);
    gtk_text_buffer_remove_tag_by_name (buffer, MOO_FOLD_TAG, &start, &end);

    if (fold->collapsed)
        return;

    for (child = fold->children; child != NULL; child = child->next)
    {
        int start_line, end_line;

        if (child->prev)
            start_line = _moo_fold_get_end (child->prev) + 1;
        else
            start_line = _moo_fold_get_start (fold) + 1;

        end_line = _moo_fold_get_start (child);

        gtk_text_buffer_get_iter_at_line (buffer, &start, start_line);
        gtk_text_buffer_get_iter_at_line (buffer, &end, end_line);
        gtk_text_iter_forward_line (&end);
        gtk_text_buffer_remove_tag_by_name (buffer, MOO_FOLD_TAG, &start, &end);

        expand_check_visible (tree, child);

        if (!child->next)
            break;
    }

    if (child)
    {
        if (_moo_fold_get_end (child) < _moo_fold_get_end (fold))
        {
            gtk_text_buffer_get_iter_at_line (buffer, &start,
                                              _moo_fold_get_end (child) + 1);
            gtk_text_buffer_get_iter_at_line (buffer, &end,
                                              _moo_fold_get_end (fold));
            gtk_text_iter_forward_line (&end);
            gtk_text_buffer_remove_tag_by_name (buffer, MOO_FOLD_TAG, &start, &end);
        }
    }
    else
    {
        start = end;
        gtk_text_buffer_get_iter_at_line (buffer, &end,
                                          _moo_fold_get_end (fold));
        gtk_text_iter_forward_line (&end);
        gtk_text_buffer_remove_tag_by_name (buffer, MOO_FOLD_TAG, &start, &end);
    }
}


void
_moo_fold_tree_expand (MooFoldTree *tree,
                       MooFold     *fold)
{
    CHECK_FOLD (tree, fold);

    if (!fold->collapsed)
        return;

    fold->collapsed = FALSE;
    expand_check_visible (tree, fold);
}

#define MOO_FOLD_FOREACH_BEGIN(start, iter)             \
do {                                                    \
    MooFold *iter = (start);                            \
    while (iter)                                        \
    {

#define MOO_FOLD_FOREACH_END(iter)                      \
        if (iter->children)                             \
            iter = iter->children;                      \
        else if (iter->next)                            \
            iter = iter->next;                          \
        else                                            \
        {                                               \
            while (iter->parent && !iter->parent->next) \
                iter = iter->parent;                    \
            iter = iter && iter->parent ?               \
                iter->parent->next : NULL;              \
        }                                               \
    }                                                   \
} while (0)

gboolean
_moo_fold_tree_expand_all (MooFoldTree *tree)
{
    gboolean ret = FALSE;
    MooFold *fold_first;

    fold_first = NULL;

    MOO_FOLD_FOREACH_BEGIN(tree->folds, fold)
    {
        if (!fold->deleted && fold->collapsed)
        {
            if (!fold_first)
                fold_first = fold;
            fold->collapsed = FALSE;
        }
    }
    MOO_FOLD_FOREACH_END(fold);

    if (fold_first)
    {
        GtkTextIter start, end;
        GtkTextBuffer *buffer = GTK_TEXT_BUFFER (tree->buffer);
        int line_first;

        line_first = _moo_fold_get_start (fold_first);

        gtk_text_buffer_get_iter_at_line (buffer, &start, line_first);
        gtk_text_buffer_get_end_iter (buffer, &end);

        gtk_text_buffer_remove_tag_by_name (buffer, MOO_FOLD_TAG, &start, &end);

        ret = TRUE;
    }

    return ret;
}

void
_moo_fold_tree_collapse (MooFoldTree *tree,
                         MooFold     *fold)
{
    GtkTextIter start, end;
    GtkTextBuffer *buffer;

    CHECK_FOLD (tree, fold);

    if (fold->collapsed)
        return;

    fold->collapsed = TRUE;

    buffer = GTK_TEXT_BUFFER (tree->buffer);
    gtk_text_buffer_get_iter_at_line (buffer, &start,
                                      _moo_fold_get_start (fold) + 1);
    gtk_text_buffer_get_iter_at_line (buffer, &end,
                                      _moo_fold_get_end (fold));
    gtk_text_iter_forward_line (&end);
    gtk_text_buffer_apply_tag_by_name (buffer, MOO_FOLD_TAG, &start, &end);
}

gboolean
_moo_fold_tree_collapse_all (MooFoldTree *tree)
{
    MooFold *fold;
    gboolean ret = FALSE;

    for (fold = tree->folds; fold != NULL; fold = fold->next)
    {
        if (!fold->collapsed)
            ret = TRUE;
        _moo_fold_tree_collapse (tree, fold);
    }

    return ret;
}

gboolean
_moo_fold_tree_toggle (MooFoldTree *tree)
{
    gboolean seen_expanded = FALSE;
    gboolean seen_collapsed = FALSE;

    MOO_FOLD_FOREACH_BEGIN(tree->folds, fold)
    {
        if (!fold->deleted)
        {
            if (fold->collapsed)
                seen_collapsed = TRUE;
            else
                seen_expanded = TRUE;
        }

        if (seen_collapsed && seen_expanded)
            break;
    }
    MOO_FOLD_FOREACH_END(fold);

    if (seen_collapsed)
    {
        _moo_fold_tree_expand_all (tree);
        return TRUE;
    }
    else if (seen_expanded)
    {
        _moo_fold_tree_collapse_all (tree);
        return TRUE;
    }

    return FALSE;
}


static GSList *
get_folds_in_range (MooFoldTree    *tree,
                    MooFold        *parent,
                    int             first_line,
                    int             last_line,
                    GSList         *list)
{
    MooFold *child;

    if (parent && first_line <= _moo_fold_get_start (parent) && last_line >= _moo_fold_get_start (parent))
        list = g_slist_prepend (list, parent);

    for (child = parent ? parent->children : tree->folds; child != NULL; child = child->next)
    {
        if (last_line < _moo_fold_get_start (child))
            break;
        if (first_line >= _moo_fold_get_end (child))
            continue;
        list = get_folds_in_range (tree, child, first_line, last_line, list);
    }

    return list;
}


GSList *
_moo_fold_tree_get (MooFoldTree    *tree,
                    int             first_line,
                    int             last_line)
{
    GSList *list;

    g_assert (tree != NULL);
    g_assert (first_line >= 0 && first_line < get_line_count (tree));
    g_assert (last_line >= 0 && last_line < get_line_count (tree));
    g_assert (last_line >= first_line);

    list = get_folds_in_range (tree, NULL, first_line, last_line, NULL);
    return g_slist_reverse (list);
}
