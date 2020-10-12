/*
 *   moobigpaned.h
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

#ifndef MOO_BIG_PANED_H
#define MOO_BIG_PANED_H

#include "moopaned.h"

G_BEGIN_DECLS


#define MOO_TYPE_BIG_PANED              (moo_big_paned_get_type ())
#define MOO_BIG_PANED(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_BIG_PANED, MooBigPaned))
#define MOO_BIG_PANED_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_BIG_PANED, MooBigPanedClass))
#define MOO_IS_BIG_PANED(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_BIG_PANED))
#define MOO_IS_BIG_PANED_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_BIG_PANED))
#define MOO_BIG_PANED_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_BIG_PANED, MooBigPanedClass))


typedef struct _MooBigPaned        MooBigPaned;
typedef struct _MooBigPanedPrivate MooBigPanedPrivate;
typedef struct _MooBigPanedClass   MooBigPanedClass;

struct _MooBigPaned
{
    GtkFrame base;
    MooBigPanedPrivate *priv;
    GtkWidget *paned[4]; /* indexed by PanePos */
};

struct _MooBigPanedClass
{
    GtkFrameClass base_class;
};


GType           moo_big_paned_get_type          (void) G_GNUC_CONST;

GtkWidget      *moo_big_paned_new               (void);

void            moo_big_paned_set_pane_order    (MooBigPaned    *paned,
                                                 int            *order);
void            moo_big_paned_set_config        (MooBigPaned    *paned,
                                                 const char     *config_string);
char           *moo_big_paned_get_config        (MooBigPaned    *paned);

MooPane        *moo_big_paned_find_pane         (MooBigPaned    *paned,
                                                 GtkWidget      *pane_widget,
                                                 MooPaned      **child_paned);

void            moo_big_paned_add_child         (MooBigPaned    *paned,
                                                 GtkWidget      *widget);
void            moo_big_paned_remove_child      (MooBigPaned    *paned);
GtkWidget      *moo_big_paned_get_child         (MooBigPaned    *paned);

MooPane        *moo_big_paned_insert_pane       (MooBigPaned    *paned,
                                                 GtkWidget      *pane_widget,
                                                 const char     *pane_id,
                                                 MooPaneLabel   *pane_label,
                                                 MooPanePosition position,
                                                 int             index_);
gboolean        moo_big_paned_remove_pane       (MooBigPaned    *paned,
                                                 GtkWidget      *pane_widget);
MooPane        *moo_big_paned_lookup_pane       (MooBigPaned    *paned,
                                                 const char     *pane_id);

GtkWidget      *moo_big_paned_get_pane          (MooBigPaned    *paned,
                                                 MooPanePosition position,
                                                 int             index_);
void            moo_big_paned_reorder_pane      (MooBigPaned    *paned,
                                                 GtkWidget      *pane_widget,
                                                 MooPanePosition new_position,
                                                 int             new_index);

MooPaned       *moo_big_paned_get_paned         (MooBigPaned    *paned,
                                                 MooPanePosition position);

void            moo_big_paned_open_pane         (MooBigPaned    *paned,
                                                 GtkWidget      *pane_widget);
void            moo_big_paned_hide_pane         (MooBigPaned    *paned,
                                                 GtkWidget      *pane_widget);
void            moo_big_paned_present_pane      (MooBigPaned    *paned,
                                                 GtkWidget      *pane_widget);
void            moo_big_paned_attach_pane       (MooBigPaned    *paned,
                                                 GtkWidget      *pane_widget);
void            moo_big_paned_detach_pane       (MooBigPaned    *paned,
                                                 GtkWidget      *pane_widget);


G_END_DECLS

#endif /* MOO_BIG_PANED_H */
