/*
 *   moolinebuffer.c
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


LineBuffer*
_moo_line_buffer_new (void)
{
    LineBuffer *buf = g_new0 (LineBuffer, 1);
    buf->tree = _moo_text_btree_new ();
    return buf;
}


#if 0
Line*
_moo_line_buffer_insert (LineBuffer     *line_buf,
                         int             index)
{
    Line *line = _moo_text_btree_insert (line_buf->tree, index);
    return line;
}
#endif


void
_moo_line_buffer_split_line (LineBuffer *line_buf,
                             int         line,
                             int         num_new_lines)
{
    g_assert (num_new_lines != 0);
    _moo_text_btree_insert_range (line_buf->tree, line + 1, num_new_lines);
}


void
_moo_line_buffer_delete (LineBuffer *line_buf,
                         int         first,
                         int         num,
                         int         move_to,
                         GSList    **moved_marks,
                         GSList    **deleted_marks)
{
    Line *line;
    MooLineMark **old_marks = NULL;
    guint i, n_old_marks = 0;

    if (move_to >= 0)
    {
        line = _moo_line_buffer_get_line (line_buf, first + num - 1);

        old_marks = line->marks;
        n_old_marks = line->n_marks;
        line->marks = NULL;

        if (n_old_marks)
            _moo_text_btree_update_n_marks (line_buf->tree, line, -((int) n_old_marks));

        g_assert (line->n_marks == 0);
    }

    _moo_text_btree_delete_range (line_buf->tree, first, num, deleted_marks);

    if (move_to >= 0)
    {
        line = _moo_line_buffer_get_line (line_buf, move_to);

        if (n_old_marks)
        {
            if (moved_marks)
                for (i = 0; i < n_old_marks; ++i)
                    *moved_marks = g_slist_prepend (*moved_marks, old_marks[i]);

            MOO_ARRAY_GROW (MooLineMark*, line->marks, n_old_marks + line->n_marks);
            MOO_ELMCPY (line->marks + line->n_marks, old_marks, n_old_marks);

            for (i = 0; i < n_old_marks; ++i)
                _moo_line_mark_set_line (old_marks[i], line, move_to, line_buf->tree->stamp);

            g_free (old_marks);

            _moo_text_btree_update_n_marks (line_buf->tree, line, n_old_marks);
        }
    }
}


Line*
_moo_line_buffer_get_line (LineBuffer *line_buf,
                           int         index)
{
    return _moo_text_btree_get_data (line_buf->tree, index);
}


void
_moo_line_buffer_free (LineBuffer *line_buf)
{
    if (line_buf)
    {
        _moo_text_btree_free (line_buf->tree);
        g_free (line_buf);
    }
}


static void
line_add_mark (LineBuffer  *line_buf,
               MooLineMark *mark,
               Line        *line)
{
    MOO_ARRAY_GROW (MooLineMark*, line->marks, line->n_marks + 1);
    line->marks[line->n_marks] = mark;
    _moo_text_btree_update_n_marks (line_buf->tree, line, 1);
}


void
_moo_line_buffer_add_mark (LineBuffer  *line_buf,
                           MooLineMark *mark,
                           int          index)
{
    Line *line;

    g_return_if_fail (index < (int) _moo_text_btree_size (line_buf->tree));
    g_assert (_moo_line_mark_get_line (mark) == NULL);

    line = _moo_line_buffer_get_line (line_buf, index);
    line_add_mark (line_buf, mark, line);
    _moo_line_mark_set_line (mark, line, index, line_buf->tree->stamp);
}


static void
line_remove_mark (LineBuffer  *line_buf,
                  MooLineMark *mark,
                  Line        *line)
{
    guint i;

    g_assert (line->marks);

    for (i = 0; i < line->n_marks; ++i)
        if (line->marks[i] == mark)
            break;

    g_assert (i < line->n_marks);
    g_assert (line->marks[i] == mark);

    if (line->n_marks == 1)
    {
        g_free (line->marks);
        line->marks = NULL;
    }
    else if (i < line->n_marks - 1)
    {
        MOO_ELMMOVE (line->marks + i, line->marks + i + 1, line->n_marks - i - 1);
        line->marks[line->n_marks - 1] = NULL;
    }
    else
    {
        line->marks[line->n_marks - 1] = NULL;
    }

    _moo_text_btree_update_n_marks (line_buf->tree, line, -1);
}


void
_moo_line_buffer_remove_mark (LineBuffer     *line_buf,
                              MooLineMark    *mark)
{
    Line *line;

    line = _moo_line_mark_get_line (mark);

    g_assert (line != NULL);
    g_assert (line == _moo_line_buffer_get_line (line_buf, moo_line_mark_get_line (mark)));

    _moo_line_mark_set_line (mark, NULL, -1, 0);
    line_remove_mark (line_buf, mark, line);
}


/* XXX */
void
_moo_line_buffer_move_mark (LineBuffer  *line_buf,
                            MooLineMark *mark,
                            int          line)
{
    g_return_if_fail (line < (int) _moo_text_btree_size (line_buf->tree));
    _moo_line_buffer_remove_mark (line_buf, mark);
    _moo_line_buffer_add_mark (line_buf, mark, line);
}


static GSList *
node_get_marks (BTree  *tree,
                BTNode *node,
                int     first_line,
                int     last_line,
                int     node_offset)
{
    GSList *total = NULL;
    guint i;

    if (!node->n_marks)
        return NULL;

    if (last_line < node_offset || first_line >= node_offset + (int) node->count)
        return NULL;

    if (node->is_bottom)
    {
        for (i = MAX (first_line - node_offset, 0); i < node->n_children; ++i)
        {
            Line *line;
            guint j;

            if ((int) i + node_offset > last_line)
                break;

            line = node->u.data[i];

            for (j = 0; j < line->n_marks; ++j)
            {
                MooLineMark *mark = line->marks[j];
                _moo_line_mark_set_line (mark, line, i + node_offset, tree->stamp);
                total = g_slist_prepend (total, line->marks[j]);
            }
        }
    }
    else
    {
        for (i = 0; i < node->n_children; ++i)
        {
            GSList *child_list;

            if (node->u.children[i]->n_marks)
            {
                child_list = node_get_marks (tree, node->u.children[i],
                                             first_line, last_line,
                                             node_offset);
                total = g_slist_concat (child_list, total);
            }

            node_offset += node->u.children[i]->count;

            if (last_line < node_offset)
                break;
        }
    }

    return total;
}


GSList *
_moo_line_buffer_get_marks_in_range (LineBuffer     *line_buf,
                                     int             first_line,
                                     int             last_line)
{
    int size = _moo_text_btree_size (line_buf->tree);

    g_assert (first_line >= 0);
    g_return_val_if_fail (first_line < size, NULL);

    if (last_line < 0 || last_line >= size)
        last_line = size - 1;

    g_return_val_if_fail (first_line <= last_line, NULL);

    return g_slist_reverse (node_get_marks (line_buf->tree,
                                            line_buf->tree->root,
                                            first_line, last_line, 0));
}


guint
_moo_line_buffer_get_stamp (LineBuffer *line_buf)
{
    return line_buf->tree->stamp;
}


static guint
line_get_index (BTData *line)
{
    guint index = 0;
    BTNode *node = (BTNode*) line;
    gboolean bottom = TRUE;

    while (node->parent)
    {
        guint i;

        for (i = 0; i < node->parent->n_children; ++i)
        {
            if (node->parent->u.children[i] != node)
            {
                if (bottom)
                    index += 1;
                else
                    index += node->parent->u.children[i]->count;
            }
            else
            {
                break;
            }
        }

        g_assert (i < node->parent->n_children);
        node = node->parent;
        bottom = FALSE;
    }

    return index;
}


int
_moo_line_buffer_get_line_index (G_GNUC_UNUSED LineBuffer *line_buf,
                                 Line           *line)
{
    guint index;
    index = line_get_index (line);
    g_assert (line == _moo_line_buffer_get_line (line_buf, index));
    return index;
}
