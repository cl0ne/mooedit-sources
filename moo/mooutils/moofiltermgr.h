/*
 *   moofiltermgr.h
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

#ifndef MOO_FILTER_MGR_H
#define MOO_FILTER_MGR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_FILTER_MGR              (moo_filter_mgr_get_type ())
#define MOO_FILTER_MGR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_FILTER_MGR, MooFilterMgr))
#define MOO_FILTER_MGR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_FILTER_MGR, MooFilterMgrClass))
#define MOO_IS_FILTER_MGR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_FILTER_MGR))
#define MOO_IS_FILTER_MGR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_FILTER_MGR))
#define MOO_FILTER_MGR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_FILTER_MGR, MooFilterMgrClass))


typedef struct _MooFilterMgr        MooFilterMgr;
typedef struct _MooFilterMgrPrivate MooFilterMgrPrivate;
typedef struct _MooFilterMgrClass   MooFilterMgrClass;

struct _MooFilterMgr
{
    GObject      parent;

    MooFilterMgrPrivate *priv;
};

struct _MooFilterMgrClass
{
    GObjectClass parent_class;
};


GType            moo_filter_mgr_get_type            (void) G_GNUC_CONST;

MooFilterMgr    *moo_filter_mgr_new                 (void);
MooFilterMgr    *moo_filter_mgr_default             (void);

void             moo_filter_mgr_init_filter_combo   (MooFilterMgr   *mgr,
                                                     GtkComboBox    *combo,
                                                     const char     *user_id);
void             moo_filter_mgr_attach              (MooFilterMgr   *mgr,
                                                     GtkFileChooser *filechooser,
                                                     GtkWidget      *box,
                                                     const char     *user_id);

GtkFileFilter   *moo_filter_mgr_get_filter          (MooFilterMgr   *mgr,
                                                     GtkTreeIter    *iter,
                                                     const char     *user_id);
void             moo_filter_mgr_set_last_filter     (MooFilterMgr   *mgr,
                                                     GtkTreeIter    *iter,
                                                     const char     *user_id);

GtkFileFilter   *moo_filter_mgr_get_null_filter     (MooFilterMgr   *mgr);
GtkFileFilter   *moo_filter_mgr_get_last_filter     (MooFilterMgr   *mgr,
                                                     const char     *user_id);
GtkFileFilter   *moo_filter_mgr_new_user_filter     (MooFilterMgr   *mgr,
                                                     const char     *glob,
                                                     const char     *user_id);
GtkFileFilter   *moo_filter_mgr_new_builtin_filter  (MooFilterMgr   *mgr,
                                                     const char     *description,
                                                     const char     *glob,
                                                     const char     *user_id,
                                                     int             position);


G_END_DECLS

#endif /* MOO_FILTER_MGR_H */
