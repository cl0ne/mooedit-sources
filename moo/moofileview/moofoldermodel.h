/*
 *   moofoldermodel.h
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

#ifndef MOO_FOLDER_MODEL_H
#define MOO_FOLDER_MODEL_H

#include <moofileview/moofile.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MOO_TYPE_FOLDER_MODEL            (_moo_folder_model_get_type ())
#define MOO_FOLDER_MODEL(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_FOLDER_MODEL, MooFolderModel))
#define MOO_FOLDER_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_FOLDER_MODEL, MooFolderModelClass))
#define MOO_IS_FOLDER_MODEL(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_FOLDER_MODEL))
#define MOO_IS_FOLDER_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_FOLDER_MODEL))
#define MOO_FOLDER_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_FOLDER_MODEL, MooFolderModelClass))

typedef struct _MooFolderModel        MooFolderModel;
typedef struct _MooFolderModelPrivate MooFolderModelPrivate;
typedef struct _MooFolderModelClass   MooFolderModelClass;

struct _MooFolderModel
{
    GObject parent;
    MooFolderModelPrivate *priv;
};

struct _MooFolderModelClass
{
    GObjectClass    parent_class;
};


typedef enum {
    MOO_FOLDER_MODEL_COLUMN_FILE = 0
} MooFolderModelColumn;

#define MOO_FOLDER_MODEL_N_COLUMNS 1


typedef enum {
    MOO_FOLDER_MODEL_SORT_CASE_SENSITIVE = 1 << 0,
    MOO_FOLDER_MODEL_SORT_FOLDERS_FIRST  = 1 << 1,
#if defined(GDK_WINDOWING_QUARTZ) && 0
    MOO_FOLDER_MODEL_SORT_FLAGS_DEFAULT  = 0
#else
    MOO_FOLDER_MODEL_SORT_FLAGS_DEFAULT  = MOO_FOLDER_MODEL_SORT_FOLDERS_FIRST
#endif
} MooFolderModelSortFlags;


GType            _moo_folder_model_get_type      (void) G_GNUC_CONST;
GtkTreeModel    *_moo_folder_model_new           (MooFolder      *folder);
void             _moo_folder_model_set_folder    (MooFolderModel *model,
                                                  MooFolder      *folder);
void             _moo_folder_model_set_sort_flags(MooFolderModel *model,
                                                  MooFolderModelSortFlags flags);

gboolean         _moo_folder_model_get_iter_by_name
                                                (MooFolderModel *model,
                                                 const char     *name,
                                                 GtkTreeIter    *iter);
gboolean         _moo_folder_model_get_iter_by_display_name
                                                (MooFolderModel *model,
                                                 const char     *name,
                                                 GtkTreeIter    *iter);

#define MOO_TYPE_FOLDER_FILTER            (_moo_folder_filter_get_type ())
#define MOO_FOLDER_FILTER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_FOLDER_FILTER, MooFolderFilter))
#define MOO_FOLDER_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_FOLDER_FILTER, MooFolderFilterClass))
#define MOO_IS_FOLDER_FILTER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_FOLDER_FILTER))
#define MOO_IS_FOLDER_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_FOLDER_FILTER))
#define MOO_FOLDER_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_FOLDER_FILTER, MooFolderFilterClass))

typedef struct _MooFolderFilter      MooFolderFilter;
typedef struct _MooFolderFilterClass MooFolderFilterClass;

struct _MooFolderFilter
{
    GtkTreeModelFilter parent;
};

struct _MooFolderFilterClass
{
    GtkTreeModelFilterClass parent_class;
};


GType            _moo_folder_filter_get_type        (void) G_GNUC_CONST;
GtkTreeModel    *_moo_folder_filter_new             (MooFolderModel     *model);
MooFolderModel  *_moo_folder_filter_get_model       (MooFolderFilter    *filter);

void             _moo_folder_filter_set_folder      (MooFolderFilter    *filter,
                                                     MooFolder          *folder);

gboolean         _moo_folder_filter_get_iter        (MooFolderFilter    *filter,
                                                     MooFile            *file,
                                                     GtkTreeIter        *filter_iter);
gboolean         _moo_folder_filter_get_iter_by_name(MooFolderFilter    *filter,
                                                     const char         *name,
                                                     GtkTreeIter        *filter_iter);
gboolean         _moo_folder_filter_get_iter_by_display_name
                                                    (MooFolderFilter    *filter,
                                                     const char         *name,
                                                     GtkTreeIter        *filter_iter);


G_END_DECLS

#endif /* MOO_FOLDER_MODEL_H */
