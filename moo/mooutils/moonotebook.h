/*
 *   moonotebook.h
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

#ifndef MOO_NOTEBOOK_H
#define MOO_NOTEBOOK_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_NOTEBOOK              (moo_notebook_get_type ())
#define MOO_NOTEBOOK(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_NOTEBOOK, MooNotebook))
#define MOO_NOTEBOOK_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_NOTEBOOK, MooNotebookClass))
#define MOO_IS_NOTEBOOK(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_NOTEBOOK))
#define MOO_IS_NOTEBOOK_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_NOTEBOOK))
#define MOO_NOTEBOOK_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_NOTEBOOK, MooNotebookClass))

typedef struct _MooNotebook         MooNotebook;
typedef struct _MooNotebookPrivate  MooNotebookPrivate;
typedef struct _MooNotebookClass    MooNotebookClass;

struct _MooNotebook
{
    GtkNotebook base;
    MooNotebookPrivate *priv;
};

struct _MooNotebookClass
{
    GtkNotebookClass base_class;

    gboolean    (*populate_popup)   (MooNotebook    *notebook,
                                     GtkWidget      *child,
                                     GtkMenu        *menu);

    void        (*switch_page)      (MooNotebook    *notebook,
                                     guint           page_num);
};


GType       moo_notebook_get_type          (void) G_GNUC_CONST;

GtkWidget  *moo_notebook_new                (void);

gint        moo_notebook_insert_page        (MooNotebook    *notebook,
                                             GtkWidget      *child,
                                             GtkWidget      *tab_label,
                                             gint            position);
void        moo_notebook_remove_page        (MooNotebook    *notebook,
                                             gint            page_num);


void        moo_notebook_set_action_widget  (MooNotebook    *notebook,
                                             GtkWidget      *widget,
                                             gboolean        right);
GtkWidget  *moo_notebook_get_action_widget  (MooNotebook    *notebook,
                                             gboolean        right);


gint        moo_notebook_get_current_page   (MooNotebook    *notebook);
GtkWidget*  moo_notebook_get_nth_page       (MooNotebook    *notebook,
                                             gint            page_num);
gint        moo_notebook_get_n_pages        (MooNotebook    *notebook);
gint        moo_notebook_page_num           (MooNotebook    *notebook,
                                             GtkWidget      *child);
void        moo_notebook_set_current_page   (MooNotebook    *notebook,
                                             gint            page_num);


void        moo_notebook_set_show_border    (MooNotebook    *notebook,
                                             gboolean        show_border);
gboolean    moo_notebook_get_show_border    (MooNotebook    *notebook);
void        moo_notebook_set_show_tabs      (MooNotebook    *notebook,
                                             gboolean        show_tabs);
gboolean    moo_notebook_get_show_tabs      (MooNotebook    *notebook);


GtkWidget  *moo_notebook_get_tab_label      (MooNotebook    *notebook,
                                             GtkWidget      *child);
void        moo_notebook_set_tab_label      (MooNotebook    *notebook,
                                             GtkWidget      *child,
                                             GtkWidget      *tab_label);
void        moo_notebook_set_tab_label_text (MooNotebook    *notebook,
                                             GtkWidget      *child,
                                             const char     *tab_text);
const char *moo_notebook_get_tab_label_text (MooNotebook    *notebook,
                                             GtkWidget      *child);

int         moo_notebook_get_event_tab      (MooNotebook    *notebook,
                                             GdkEvent       *event);

void        moo_notebook_reorder_child      (MooNotebook    *notebook,
                                             GtkWidget      *child,
                                             gint            position);


void        moo_notebook_enable_popup               (MooNotebook    *notebook,
                                                     gboolean        enable);

void        moo_notebook_enable_reordering          (MooNotebook    *notebook,
                                                     gboolean        enable);


G_END_DECLS

#endif /* MOO_NOTEBOOK_H */
