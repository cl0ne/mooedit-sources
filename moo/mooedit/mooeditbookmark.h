/*
 *   mooeditbookmark.h
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

#ifndef MOO_EDIT_BOOKMARK_H
#define MOO_EDIT_BOOKMARK_H

#include <mooedit/mooedit.h>
#include <mooedit/moolinemark.h>

G_BEGIN_DECLS


#define MOO_TYPE_EDIT_BOOKMARK              (moo_edit_bookmark_get_type ())
#define MOO_EDIT_BOOKMARK(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_EDIT_BOOKMARK, MooEditBookmark))
#define MOO_EDIT_BOOKMARK_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_EDIT_BOOKMARK, MooEditBookmarkClass))
#define MOO_IS_EDIT_BOOKMARK(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_EDIT_BOOKMARK))
#define MOO_IS_EDIT_BOOKMARK_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_EDIT_BOOKMARK))
#define MOO_EDIT_BOOKMARK_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_EDIT_BOOKMARK, MooEditBookmarkClass))

typedef struct MooEditBookmark MooEditBookmark;
typedef struct MooEditBookmarkClass MooEditBookmarkClass;

struct MooEditBookmark
{
    MooLineMark mark;
    guint no;
};

struct MooEditBookmarkClass
{
    MooLineMarkClass mark_class;
};


GType            moo_edit_bookmark_get_type     (void) G_GNUC_CONST;

void             moo_edit_set_enable_bookmarks  (MooEdit        *edit,
                                                 gboolean        enable);
gboolean         moo_edit_get_enable_bookmarks  (MooEdit        *edit);
/* list must not be modified */
const GSList    *moo_edit_list_bookmarks        (MooEdit        *edit);
void             moo_edit_toggle_bookmark       (MooEdit        *edit,
                                                 guint           line);
void             moo_edit_remove_bookmark       (MooEdit        *edit,
                                                 MooEditBookmark *bookmark);
void             moo_edit_add_bookmark          (MooEdit        *edit,
                                                 guint           line,
                                                 guint           no);
MooEditBookmark *moo_edit_get_bookmark_at_line  (MooEdit        *edit,
                                                 guint           line);
MooEditBookmark *moo_edit_get_bookmark          (MooEdit        *edit,
                                                 guint           n);
GSList          *moo_edit_get_bookmarks_in_range(MooEdit        *edit,
                                                 int             first_line,
                                                 int             last_line);

void             moo_edit_view_goto_bookmark    (MooEditView     *view,
                                                 MooEditBookmark *bookmark);

char            *_moo_edit_bookmark_get_text    (MooEditBookmark *bookmark);


G_END_DECLS

#endif /* MOO_EDIT_BOOKMARK_H */
