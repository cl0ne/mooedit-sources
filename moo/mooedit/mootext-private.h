/*
 *   mootext-private.h
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

#ifndef MOO_TEXT_PRIVATE_H
#define MOO_TEXT_PRIVATE_H

#include "mooedit/moofold.h"
#include "mooedit/moolinebuffer.h"
#include "mooedit/mootextstylescheme.h"

G_BEGIN_DECLS


#define MOO_PLACEHOLDER_TAG "moo-placeholder-tag"


Line       *_moo_line_mark_get_line                 (MooLineMark        *mark);
void        _moo_line_mark_set_line                 (MooLineMark        *mark,
                                                     Line               *line,
                                                     int                 line_no,
                                                     guint               stamp);
void        _moo_line_mark_set_buffer               (MooLineMark        *mark,
                                                     MooTextBuffer      *buffer,
                                                     LineBuffer         *line_buf);
void        _moo_line_mark_deleted                  (MooLineMark        *mark);
void        _moo_line_mark_set_pretty               (MooLineMark        *mark,
                                                     gboolean            pretty);
gboolean    _moo_line_mark_get_pretty               (MooLineMark        *mark);

void        _moo_line_mark_realize                  (MooLineMark        *mark,
                                                     GtkWidget          *widget);
void        _moo_line_mark_unrealize                (MooLineMark        *mark);

void        _moo_line_mark_set_fold                 (MooLineMark        *mark,
                                                     MooFold            *fold);
MooFold    *_moo_line_mark_get_fold                 (MooLineMark        *mark);

void        _moo_text_buffer_update_highlight       (MooTextBuffer      *buffer,
                                                     const GtkTextIter  *start,
                                                     const GtkTextIter  *end,
                                                     gboolean            synchronous);
gpointer    _moo_text_buffer_get_undo_stack         (MooTextBuffer      *buffer);
gboolean    _moo_text_buffer_is_bracket_tag         (MooTextBuffer      *buffer,
                                                     GtkTextTag         *tag);
void        _moo_text_buffer_set_style_scheme       (MooTextBuffer      *buffer,
                                                     MooTextStyleScheme *scheme);


G_END_DECLS

#endif /* MOO_TEXT_PRIVATE_H */
