/*
 *   mootextbuffer.h
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

#ifndef MOO_TEXT_BUFFER_H
#define MOO_TEXT_BUFFER_H

#include <gtk/gtk.h>
#include <mooedit/moolang.h>
#include <mooedit/moolinemark.h>

G_BEGIN_DECLS


#define MOO_TYPE_TEXT_BUFFER              (moo_text_buffer_get_type ())
#define MOO_TEXT_BUFFER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_TEXT_BUFFER, MooTextBuffer))
#define MOO_TEXT_BUFFER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_TEXT_BUFFER, MooTextBufferClass))
#define MOO_IS_TEXT_BUFFER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_TEXT_BUFFER))
#define MOO_IS_TEXT_BUFFER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_TEXT_BUFFER))
#define MOO_TEXT_BUFFER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_TEXT_BUFFER, MooTextBufferClass))


typedef struct MooTextBufferPrivate  MooTextBufferPrivate;
typedef struct MooTextBufferClass    MooTextBufferClass;

struct MooTextBuffer
{
    GtkTextBuffer  parent;

    MooTextBufferPrivate *priv;
};

struct MooTextBufferClass
{
    GtkTextBufferClass parent_class;

    gboolean (* undo)           (MooTextBuffer      *buffer);
    gboolean (* redo)           (MooTextBuffer      *buffer);

    void (*cursor_moved)        (MooTextBuffer      *buffer,
                                 const GtkTextIter  *iter);
    void (*selection_changed)   (MooTextBuffer      *buffer);

    void (*line_mark_added)     (MooTextBuffer      *buffer,
                                 MooLineMark        *mark);
    void (*line_mark_deleted)   (MooTextBuffer      *buffer,
                                 MooLineMark        *mark);
    void (*line_mark_moved)     (MooTextBuffer      *buffer,
                                 MooLineMark        *mark);

    void (*fold_added)          (MooTextBuffer      *buffer,
                                 MooFold            *fold);
    void (*fold_deleted)        (MooTextBuffer      *buffer,
                                 MooFold            *fold);
    void (*fold_toggled)        (MooTextBuffer      *buffer,
                                 MooFold            *fold);
};


GType       moo_text_buffer_get_type                    (void) G_GNUC_CONST;

GtkTextBuffer *moo_text_buffer_new                      (GtkTextTagTable    *table);

void        moo_text_buffer_set_lang                    (MooTextBuffer      *buffer,
                                                         MooLang            *lang);
MooLang    *moo_text_buffer_get_lang                    (MooTextBuffer      *buffer);

void        moo_text_buffer_set_highlight               (MooTextBuffer      *buffer,
                                                         gboolean            highlight);
gboolean    moo_text_buffer_get_highlight               (MooTextBuffer      *buffer);

void        moo_text_buffer_set_brackets                (MooTextBuffer      *buffer,
                                                         const char         *brackets);

gboolean    moo_text_buffer_can_redo                    (MooTextBuffer      *buffer);
gboolean    moo_text_buffer_can_undo                    (MooTextBuffer      *buffer);
void        moo_text_buffer_begin_non_undoable_action   (MooTextBuffer      *buffer);
void        moo_text_buffer_end_non_undoable_action     (MooTextBuffer      *buffer);

void        moo_text_buffer_freeze                      (MooTextBuffer      *buffer);
void        moo_text_buffer_thaw                        (MooTextBuffer      *buffer);
void        moo_text_buffer_begin_non_interactive_action(MooTextBuffer      *buffer);
void        moo_text_buffer_end_non_interactive_action  (MooTextBuffer      *buffer);

gboolean    moo_text_buffer_has_text                    (MooTextBuffer      *buffer);
gboolean    moo_text_buffer_has_selection               (MooTextBuffer      *buffer);

void        moo_text_buffer_add_line_mark               (MooTextBuffer      *buffer,
                                                         MooLineMark        *mark,
                                                         int                 line);
void        moo_text_buffer_delete_line_mark            (MooTextBuffer      *buffer,
                                                         MooLineMark        *mark);
void        moo_text_buffer_move_line_mark              (MooTextBuffer      *buffer,
                                                         MooLineMark        *mark,
                                                         int                 line);
GSList     *moo_text_buffer_get_line_marks_in_range     (MooTextBuffer      *buffer,
                                                         int                 first_line,
                                                         int                 last_line);
GSList     *moo_text_buffer_get_line_marks_at_line      (MooTextBuffer      *buffer,
                                                         int                 line);

MooFold    *moo_text_buffer_add_fold                    (MooTextBuffer      *buffer,
                                                         int                 first_line,
                                                         int                 end_line);
void        moo_text_buffer_delete_fold                 (MooTextBuffer      *buffer,
                                                         MooFold            *fold);
MooFold    *moo_text_buffer_get_fold_at_line            (MooTextBuffer      *buffer,
                                                         int                 line);
void        moo_text_buffer_toggle_fold                 (MooTextBuffer      *buffer,
                                                         MooFold            *fold);
void        moo_text_buffer_toggle_folds                (MooTextBuffer      *buffer);


G_END_DECLS

#endif /* MOO_TEXT_BUFFER_H */
