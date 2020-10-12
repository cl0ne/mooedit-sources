/*
 *   moobookmarkview.h
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

#ifndef MOO_BOOKMARK_VIEW_H
#define MOO_BOOKMARK_VIEW_H

#include <moofileview/moobookmarkmgr.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_BOOKMARK_VIEW                (_moo_bookmark_view_get_type ())
#define MOO_BOOKMARK_VIEW(object)             (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_BOOKMARK_VIEW, MooBookmarkView))
#define MOO_BOOKMARK_VIEW_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_BOOKMARK_VIEW, MooBookmarkViewClass))
#define MOO_IS_BOOKMARK_VIEW(object)          (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_BOOKMARK_VIEW))
#define MOO_IS_BOOKMARK_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_BOOKMARK_VIEW))
#define MOO_BOOKMARK_VIEW_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_BOOKMARK_VIEW, MooBookmarkViewClass))


typedef struct _MooBookmarkView          MooBookmarkView;
typedef struct _MooBookmarkViewPrivate   MooBookmarkViewPrivate;
typedef struct _MooBookmarkViewClass     MooBookmarkViewClass;

struct _MooBookmarkView
{
    GtkTreeView tree_view;

    MooBookmarkMgr *mgr;
};

struct _MooBookmarkViewClass
{
    GtkTreeViewClass tree_view_class;

    void (*bookmark_activated) (MooBookmarkView *view,
                                MooBookmark     *bookmark);
};


GType           _moo_bookmark_view_get_type     (void) G_GNUC_CONST;

GtkWidget      *_moo_bookmark_view_new          (MooBookmarkMgr     *mgr);

void            _moo_bookmark_view_set_mgr      (MooBookmarkView    *view,
                                                 MooBookmarkMgr     *mgr);


G_END_DECLS

#endif /* MOO_BOOKMARK_VIEW_H */
