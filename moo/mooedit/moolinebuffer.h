/*
 *   moolinebuffer.h
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

#ifndef MOO_LINE_BUFFER_H
#define MOO_LINE_BUFFER_H

#include <gtk/gtk.h>
#include "mooedit/mootextbtree.h"
#include "mooedit/mootextbuffer.h"

G_BEGIN_DECLS


typedef struct LineBuffer LineBuffer;
typedef struct BTData Line;

struct LineBuffer {
    BTree *tree;
};


LineBuffer *_moo_line_buffer_new        (void);
void     _moo_line_buffer_free          (LineBuffer     *line_buf);

Line    *_moo_line_buffer_get_line      (LineBuffer     *line_buf,
                                         int             index);

guint    _moo_line_buffer_get_stamp     (LineBuffer     *line_buf);
int      _moo_line_buffer_get_line_index (LineBuffer    *line_buf,
                                         Line           *line);

void     _moo_line_buffer_add_mark      (LineBuffer     *line_buf,
                                         MooLineMark    *mark,
                                         int             line);
void     _moo_line_buffer_remove_mark   (LineBuffer     *line_buf,
                                         MooLineMark    *mark);
void     _moo_line_buffer_move_mark     (LineBuffer     *line_buf,
                                         MooLineMark    *mark,
                                         int             line);
GSList  *_moo_line_buffer_get_marks_in_range (LineBuffer *line_buf,
                                         int             first_line,
                                         int             last_line);

void     _moo_line_buffer_split_line    (LineBuffer     *line_buf,
                                         int             line,
                                         int             num_new_lines);
void     _moo_line_buffer_delete        (LineBuffer     *line_buf,
                                         int             first,
                                         int             num,
                                         int             move_to,
                                         GSList        **moved_marks,
                                         GSList        **deleted_marks);


G_END_DECLS

#endif /* MOO_LINE_BUFFER_H */
