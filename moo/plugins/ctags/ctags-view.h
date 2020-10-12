/*
 *   ctags-view.h
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

#ifndef CTAGS_VIEW_H
#define CTAGS_VIEW_H

#include <gtk/gtk.h>

#define MOO_TYPE_CTAGS_VIEW              (_moo_ctags_view_get_type ())
#define MOO_CTAGS_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_CTAGS_VIEW, MooCtagsView))
#define MOO_CTAGS_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_CTAGS_VIEW, MooCtagsViewClass))
#define MOO_IS_CTAGS_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_CTAGS_VIEW))
#define MOO_IS_CTAGS_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_CTAGS_VIEW))
#define MOO_CTAGS_VIEW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_CTAGS_VIEW, MooCtagsViewClass))

typedef struct _MooCtagsView MooCtagsView;
typedef struct _MooCtagsViewPrivate MooCtagsViewPrivate;
typedef struct _MooCtagsViewClass MooCtagsViewClass;

#define MOO_CTAGS_VIEW_COLUMN_ENTRY 0
#define MOO_CTAGS_VIEW_COLUMN_LABEL 1

struct _MooCtagsView
{
    GtkTreeView base;
    MooCtagsViewPrivate *priv;
};

struct _MooCtagsViewClass
{
    GtkTreeViewClass base_class;
};


GType       _moo_ctags_view_get_type    (void) G_GNUC_CONST;
GtkWidget  *_moo_ctags_view_new         (void);


#endif /* CTAGS_VIEW_H */
