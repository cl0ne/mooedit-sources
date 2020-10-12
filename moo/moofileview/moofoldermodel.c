/*
 *   moofoldermodel.c
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

#include "moofoldermodel.h"
#include "moofile-private.h"
#include "moofolder-private.h"
#include "moofoldermodel-private.h"
#include <gtk/gtk.h>


struct _MooFolderModelPrivate {
    MooFolder   *folder;
    FileList    *files;
    MooFolderModelSortFlags sort_flags;
    MooFileCmp   cmp_func;
};


static void moo_folder_model_class_init         (MooFolderModelClass    *klass);
static void moo_folder_model_init               (MooFolderModel         *model);

static void moo_folder_model_tree_iface_init    (GtkTreeModelIface      *iface);

static void moo_folder_model_finalize           (GObject                *object);
static void moo_folder_model_set_property       (GObject                *object,
                                                 guint                   property_id,
                                                 const GValue           *value,
                                                 GParamSpec             *pspec);
static void moo_folder_model_get_property       (GObject                *object,
                                                 guint                   property_id,
                                                 GValue                 *value,
                                                 GParamSpec             *pspec);

static gpointer moo_folder_model_parent_class = NULL;

GType
_moo_folder_model_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (!type))
    {
        static GTypeInfo info = {
            /* interface types, classed types, instantiated types */
            sizeof (MooFolderClass),
            NULL, /* base_init; */
            NULL, /* base_finalize; */
            (GClassInitFunc) moo_folder_model_class_init,
            NULL, /* class_finalize; */
            NULL, /* class_data; */
            sizeof (MooFolder),
            0, /* n_preallocs; */
            (GInstanceInitFunc) moo_folder_model_init,
            NULL /* value_table; */
        };

        static GInterfaceInfo tree_model_info = {
            (GInterfaceInitFunc) moo_folder_model_tree_iface_init,
            NULL, /* interface_finalize; */
            NULL /* interface_data; */
        };

        type = g_type_register_static (G_TYPE_OBJECT,
                                       "MooFolderModel",
                                       &info, 0);
        g_type_add_interface_static (type, GTK_TYPE_TREE_MODEL,
                                     &tree_model_info);
    }

    return type;
}

enum {
    PROP_0,
    PROP_FOLDER
};


static void
moo_folder_model_class_init (MooFolderModelClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    moo_folder_model_parent_class = g_type_class_peek_parent (klass);

    gobject_class->finalize = moo_folder_model_finalize;
    gobject_class->set_property = moo_folder_model_set_property;
    gobject_class->get_property = moo_folder_model_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_FOLDER,
                                     g_param_spec_object ("folder",
                                             "folder",
                                             "folder",
                                             MOO_TYPE_FOLDER,
                                             (GParamFlags) G_PARAM_READWRITE));
}


static void         moo_folder_model_add_files      (MooFolderModel *model,
                                                     GSList         *files);
static void         moo_folder_model_change_files   (MooFolderModel *model,
                                                     GSList         *files);
static void         moo_folder_model_remove_files   (MooFolderModel *model,
                                                     GSList         *files);
static void         moo_folder_model_folder_deleted (MooFolderModel *model);
static void         moo_folder_model_disconnect_folder (MooFolderModel *model);


static MooFolder   *_moo_folder_model_get_folder    (MooFolderModel *model);

static GtkTreeModelFlags moo_folder_model_get_flags (GtkTreeModel *tree_model);
static gint         moo_folder_model_get_n_columns  (GtkTreeModel *tree_model);
static GType        moo_folder_model_get_column_type(GtkTreeModel *tree_model,
                                                     gint          index_);
static gboolean     moo_folder_model_get_iter_impl  (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreePath  *path);
static GtkTreePath *moo_folder_model_get_path       (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static void         moo_folder_model_get_value      (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     gint          column,
                                                     GValue       *value);
static gboolean     moo_folder_model_iter_next      (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static gboolean     moo_folder_model_iter_children  (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreeIter  *parent);
static gboolean     moo_folder_model_iter_has_child (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static gint         moo_folder_model_iter_n_children(GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static gboolean     moo_folder_model_iter_nth_child (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreeIter  *parent,
                                                     gint          n);
static gboolean     moo_folder_model_iter_parent    (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreeIter  *child);


static void
moo_folder_model_tree_iface_init (GtkTreeModelIface *iface)
{
    iface->get_flags = moo_folder_model_get_flags;
    iface->get_n_columns = moo_folder_model_get_n_columns;
    iface->get_column_type = moo_folder_model_get_column_type;
    iface->get_iter = moo_folder_model_get_iter_impl;
    iface->get_path = moo_folder_model_get_path;
    iface->get_value = moo_folder_model_get_value;
    iface->iter_next = moo_folder_model_iter_next;
    iface->iter_children = moo_folder_model_iter_children;
    iface->iter_has_child = moo_folder_model_iter_has_child;
    iface->iter_n_children = moo_folder_model_iter_n_children;
    iface->iter_nth_child = moo_folder_model_iter_nth_child;
    iface->iter_parent = moo_folder_model_iter_parent;
}


static void
moo_folder_model_set_property (GObject *object,
                               guint property_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
    MooFolderModel *model = MOO_FOLDER_MODEL (object);

    switch (property_id)
    {
        case PROP_FOLDER:
            _moo_folder_model_set_folder (model,
                                          g_value_get_object (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
moo_folder_model_get_property (GObject *object,
                               guint property_id,
                               GValue *value,
                               GParamSpec *pspec)
{
    MooFolderModel *model = MOO_FOLDER_MODEL (object);

    switch (property_id)
    {
        case PROP_FOLDER:
            g_value_set_object (value,
                                _moo_folder_model_get_folder (model));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


void
_moo_folder_model_set_folder (MooFolderModel *model,
                              MooFolder      *folder)
{
    GSList *files;

    g_return_if_fail (!folder || MOO_IS_FOLDER (folder));

    if (model->priv->folder == folder)
        return;

    moo_folder_model_disconnect_folder (model);

    if (folder)
    {
        model->priv->folder = g_object_ref (folder);

        g_signal_connect_swapped (folder, "files_added",
                                  G_CALLBACK (moo_folder_model_add_files),
                                  model);
        g_signal_connect_swapped (folder, "files_removed",
                                  G_CALLBACK (moo_folder_model_remove_files),
                                  model);
        g_signal_connect_swapped (folder, "files_changed",
                                  G_CALLBACK (moo_folder_model_change_files),
                                  model);
        g_signal_connect_swapped (folder, "deleted",
                                  G_CALLBACK (moo_folder_model_folder_deleted),
                                  model);

        files = _moo_folder_list_files (folder);
        moo_folder_model_add_files (model, files);
        g_slist_foreach (files, (GFunc) _moo_file_unref, NULL);
        g_slist_free (files);
    }

    g_object_notify (G_OBJECT (model), "folder");
}


static MooFileCmp
get_cmp_func (MooFolderModelSortFlags flags)
{
    if (flags & MOO_FOLDER_MODEL_SORT_FOLDERS_FIRST)
    {
        if (flags & MOO_FOLDER_MODEL_SORT_CASE_SENSITIVE)
            return moo_file_cmp;
        else
            return moo_file_case_cmp;
    }
    else
    {
        if (flags & MOO_FOLDER_MODEL_SORT_CASE_SENSITIVE)
            return moo_file_cmp_fi;
        else
            return moo_file_case_cmp_fi;
    }
}

static void
moo_folder_model_init (MooFolderModel *model)
{
    model->priv = g_new0 (MooFolderModelPrivate, 1);
    model->priv->sort_flags = MOO_FOLDER_MODEL_SORT_FLAGS_DEFAULT;
    model->priv->cmp_func = get_cmp_func (model->priv->sort_flags);
    model->priv->files = file_list_new (model->priv->cmp_func);
}


static void
moo_folder_model_finalize (GObject *object)
{
    MooFolderModel *model = MOO_FOLDER_MODEL (object);

    if (model->priv->folder)
    {
        g_signal_handlers_disconnect_by_func (model->priv->folder,
                                              (gpointer) moo_folder_model_add_files,
                                              model);
        g_signal_handlers_disconnect_by_func (model->priv->folder,
                                              (gpointer) moo_folder_model_remove_files,
                                              model);
        g_signal_handlers_disconnect_by_func (model->priv->folder,
                                              (gpointer) moo_folder_model_change_files,
                                              model);
        g_object_unref (model->priv->folder);
        model->priv->folder = NULL;
    }

    file_list_destroy (model->priv->files);

    g_free (model->priv);
    model->priv = NULL;

    G_OBJECT_CLASS(moo_folder_model_parent_class)->finalize (object);
}


void
_moo_folder_model_set_sort_flags (MooFolderModel          *model,
                                  MooFolderModelSortFlags  flags)
{
    g_return_if_fail (MOO_IS_FOLDER_MODEL (model));

    if (model->priv->sort_flags != flags)
    {
        int *new_order = NULL;
        GtkTreePath *path = gtk_tree_path_new ();

        model->priv->sort_flags = flags;
        model->priv->cmp_func = get_cmp_func (model->priv->sort_flags);
        file_list_set_cmp_func (model->priv->files, model->priv->cmp_func, &new_order);

        g_signal_emit_by_name (model, "rows-reordered", path, NULL, new_order);

        g_free (new_order);
        gtk_tree_path_free (path);
    }
}


/*****************************************************************************/
/* Files
 */

static void     model_add_moo_file      (MooFile        *file,
                                         MooFolderModel *model);
static void     model_change_moo_file   (MooFile        *file,
                                         MooFolderModel *model);
static void     model_remove_moo_file   (MooFile        *file,
                                         MooFolderModel *model);


inline static gboolean
model_contains_file (MooFolderModel *model,
                     MooFile        *file)
{
    return file_list_contains (model->priv->files, file);
}


#define ITER_MODEL(ip)      ((ip)->user_data)
#define ITER_FILE(ip)       ((ip)->user_data2)
#define ITER_GET_MODEL(ip)  ((MooFolderModel*) ITER_MODEL (ip))
#define ITER_GET_FILE(ip)   ((MooFile*) ITER_FILE (ip))

#define ITER_INIT(ip,model,file)        \
G_STMT_START {                          \
    ITER_MODEL(ip) = model;             \
    ITER_FILE(ip) = file;               \
} G_STMT_END

#ifdef MOO_DEBUG
#if 0
#define DEFINE_CHECK_ITER
static void CHECK_ITER (MooFolderModel *model, GtkTreeIter *iter);
#endif
#endif /* MOO_DEBUG */

#ifndef DEFINE_CHECK_ITER
#define CHECK_ITER(model,iter)
#endif


static void
moo_folder_model_disconnect_folder (MooFolderModel *model)
{
    GSList *files;

    if (model->priv->folder)
    {
        g_signal_handlers_disconnect_by_func (model->priv->folder,
                                              (gpointer) moo_folder_model_add_files,
                                              model);
        g_signal_handlers_disconnect_by_func (model->priv->folder,
                                              (gpointer) moo_folder_model_remove_files,
                                              model);
        g_signal_handlers_disconnect_by_func (model->priv->folder,
                                              (gpointer) moo_folder_model_change_files,
                                              model);
        g_signal_handlers_disconnect_by_func (model->priv->folder,
                                              (gpointer) moo_folder_model_folder_deleted,
                                              model);

        files = file_list_get_slist (model->priv->files);
        moo_folder_model_remove_files (model, files);
        g_slist_foreach (files, (GFunc) _moo_file_unref, NULL);
        g_slist_free (files);

        g_object_unref (model->priv->folder);
        model->priv->folder = NULL;
    }

}


static void
moo_folder_model_folder_deleted (MooFolderModel *model)
{
    moo_folder_model_disconnect_folder (model);
}

static void
moo_folder_model_add_files (MooFolderModel *model,
                            GSList         *files)
{
    g_slist_foreach (files, (GFunc) model_add_moo_file, model);
}

static void
moo_folder_model_change_files (MooFolderModel *model,
                               GSList         *files)
{
    g_slist_foreach (files, (GFunc) model_change_moo_file, model);
}

static void
moo_folder_model_remove_files (MooFolderModel *model,
                               GSList         *files)
{
    g_slist_foreach (files, (GFunc) model_remove_moo_file, model);
}


static void
model_add_moo_file (MooFile        *file,
                    MooFolderModel *model)
{
    int index_;
    GtkTreePath *path;
    GtkTreeIter iter;

    g_assert (!model_contains_file (model, file));
    g_assert (file != NULL);

    _moo_file_ref (file);

    index_ = file_list_add (model->priv->files, file);

    ITER_INIT (&iter, model, file);
    CHECK_ITER (model, &iter);

    path = gtk_tree_path_new_from_indices (index_, -1);
    gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
    gtk_tree_path_free (path);
}


static void
model_change_moo_file (MooFile        *file,
                       MooFolderModel *model)
{
    int index_;
    GtkTreePath *path;
    GtkTreeIter iter;

    g_assert (model_contains_file (model, file));
    g_assert (file != NULL);

    index_ = file_list_position (model->priv->files, file);
    g_assert (index_ >= 0);

    ITER_INIT (&iter, model, file);
    CHECK_ITER (model, &iter);

    path = gtk_tree_path_new_from_indices (index_, -1);
    gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
    gtk_tree_path_free (path);
}


static void
model_remove_moo_file (MooFile        *file,
                       MooFolderModel *model)
{
    int index;
    GtkTreePath *path;

    g_assert (model_contains_file (model, file));
    g_assert (file != NULL);

    _moo_file_ref (file);

    index = file_list_remove (model->priv->files, file);

    path = gtk_tree_path_new_from_indices (index, -1);
    gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
    gtk_tree_path_free (path);

    _moo_file_unref (file);
}


#ifdef MOO_DEBUG
#if 1
#define CHECK_ORDER(order,n)                \
G_STMT_START {                              \
    int *check = g_new0 (int, n);           \
    for (i = 0; i < n; ++i)                 \
    {                                       \
        g_assert (0 <= order[i]);           \
        g_assert (order[i] < n);            \
        g_assert (check[order[i]] == 0);    \
        check[order[i]] = 1;                \
    }                                       \
    g_free (check);                         \
} G_STMT_END
#endif
#endif /* MOO_DEBUG */
#ifndef CHECK_ORDER
#define CHECK_ORDER(order,n)
#endif


/***************************************************************************/
/* GtkTreeModel
 */

static GtkTreeModelFlags
moo_folder_model_get_flags (G_GNUC_UNUSED GtkTreeModel *tree_model)
{
    return GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY;
}


static gint
moo_folder_model_get_n_columns (G_GNUC_UNUSED GtkTreeModel *tree_model)
{
#if MOO_FOLDER_MODEL_N_COLUMNS != 1
#error "Fix me!"
#endif
    return MOO_FOLDER_MODEL_N_COLUMNS;
}


static GType
moo_folder_model_get_column_type (G_GNUC_UNUSED GtkTreeModel *tree_model,
                                  gint          index_)
{
    g_return_val_if_fail (index_ == 0, 0);
    return MOO_TYPE_FILE;
}


static gboolean
moo_folder_model_get_iter_impl (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter,
                                GtkTreePath  *path)
{
    MooFolderModel *model;
    int index;
    MooFile *file;

    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (tree_model), FALSE);
    g_return_val_if_fail (iter != NULL && path != NULL, FALSE);

    model = MOO_FOLDER_MODEL (tree_model);

    if (gtk_tree_path_get_depth (path) != 1)
    {
        g_critical ("oops");
        return FALSE;
    }

    index = gtk_tree_path_get_indices(path)[0];

    g_return_val_if_fail (index >= 0, FALSE);

    if (index >= model->priv->files->size)
        return FALSE;

    file = file_list_nth (model->priv->files, index);

    g_assert (file != NULL);
    ITER_INIT (iter, model, file);
    CHECK_ITER (model, iter);

    return TRUE;
}


static GtkTreePath *
moo_folder_model_get_path (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter)
{
    MooFolderModel *model;
    int index;

    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (tree_model), NULL);
    g_return_val_if_fail (iter != NULL, NULL);

    model = MOO_FOLDER_MODEL (tree_model);
    g_return_val_if_fail (ITER_MODEL (iter) == model, NULL);
    g_return_val_if_fail (ITER_FILE (iter) != NULL, NULL);

    index = file_list_position (model->priv->files, ITER_FILE (iter));
    g_return_val_if_fail (index >= 0, NULL);

    return gtk_tree_path_new_from_indices (index, -1);
}


static void
moo_folder_model_get_value (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter,
                            gint          column,
                            GValue       *value)
{
    g_return_if_fail (MOO_IS_FOLDER_MODEL (tree_model));
    g_return_if_fail (iter != NULL);
    g_return_if_fail (column == 0);
    g_return_if_fail (value != NULL);
    g_return_if_fail (ITER_MODEL (iter) == tree_model);
    g_return_if_fail (ITER_FILE (iter) != NULL);
    g_value_init (value, MOO_TYPE_FILE);
    g_value_set_boxed (value, ITER_FILE (iter));
}


static gboolean
moo_folder_model_iter_next (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter)
{
    MooFolderModel *model;
    MooFile *next;

    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (tree_model), FALSE);
    g_return_val_if_fail (iter != NULL, FALSE);

    model = MOO_FOLDER_MODEL (tree_model);
    g_return_val_if_fail (ITER_MODEL (iter) == model, FALSE);
    g_return_val_if_fail (ITER_FILE (iter) != NULL, FALSE);

    next = file_list_next (model->priv->files, ITER_FILE (iter));

    if (next)
    {
        ITER_INIT (iter, model, next);
        CHECK_ITER (model, iter);
        return TRUE;
    }
    else
    {
        ITER_INIT (iter, NULL, NULL);
        return FALSE;
    }
}


static gboolean
moo_folder_model_iter_children (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter,
                                GtkTreeIter  *parent)
{
    MooFolderModel *model;
    MooFile *first;

    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (tree_model), FALSE);

    model = MOO_FOLDER_MODEL (tree_model);

    if (!parent)
    {
        if ((first = file_list_first (model->priv->files)))
        {
            ITER_INIT (iter, model, first);
            CHECK_ITER (model, iter);
            return TRUE;
        }
        else
        {
            ITER_INIT (iter, NULL, NULL);
            return FALSE;
        }
    }
    else
    {
        g_return_val_if_fail (ITER_MODEL (parent) == model, FALSE);
        g_return_val_if_fail (ITER_FILE (parent) != NULL, FALSE);
        ITER_INIT (iter, NULL, NULL);
        return FALSE;
    }
}


static gboolean
moo_folder_model_iter_has_child (GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter)
{
    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (tree_model), FALSE);
    g_return_val_if_fail (iter != NULL, FALSE);
    g_return_val_if_fail (ITER_MODEL (iter) == tree_model, FALSE);
    g_return_val_if_fail (ITER_FILE (iter) != NULL, FALSE);
    return FALSE;
}


static gint
moo_folder_model_iter_n_children (GtkTreeModel *tree_model,
                                  GtkTreeIter  *iter)
{
    MooFolderModel *model;

    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (tree_model), 0);

    if (iter)
    {
        g_return_val_if_fail (ITER_MODEL (iter) == tree_model, 0);
        g_return_val_if_fail (ITER_FILE (iter) != NULL, 0);
        return 0;
    }

    model = MOO_FOLDER_MODEL (tree_model);
    return model->priv->files->size;
}


static gboolean
moo_folder_model_iter_nth_child (GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter,
                                 GtkTreeIter  *parent,
                                 gint          n)
{
    GtkTreePath *path;
    gboolean result;

    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (tree_model), FALSE);

    if (parent)
    {
        g_return_val_if_fail (ITER_MODEL (parent) == tree_model, FALSE);
        g_return_val_if_fail (ITER_FILE (parent) != NULL, FALSE);
        return FALSE;
    }

    /* TODO */
    path = gtk_tree_path_new_from_indices (n, -1);
    result = gtk_tree_model_get_iter (tree_model, iter, path);
    gtk_tree_path_free (path);
    return result;
}


static gboolean
moo_folder_model_iter_parent (GtkTreeModel *tree_model,
                              GtkTreeIter  *iter,
                              GtkTreeIter  *child)
{
    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (tree_model), FALSE);
    g_return_val_if_fail (child != NULL, FALSE);
    g_return_val_if_fail (ITER_MODEL (child) == tree_model, FALSE);
    g_return_val_if_fail (ITER_FILE (child) != NULL, FALSE);
    ITER_INIT (iter, NULL, NULL);
    return FALSE;
}


GtkTreeModel *
_moo_folder_model_new (MooFolder *folder)
{
    g_return_val_if_fail (!folder || MOO_IS_FOLDER (folder), NULL);
    return GTK_TREE_MODEL (g_object_new (MOO_TYPE_FOLDER_MODEL,
                                         "folder", folder,
                                         (const char*) NULL));
}


static MooFolder *
_moo_folder_model_get_folder (MooFolderModel *model)
{
    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (model), NULL);
    return model->priv->folder;
}


#if 0
static gboolean
_moo_folder_model_get_iter (MooFolderModel *model,
                            MooFile        *file,
                            GtkTreeIter    *iter)
{
    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (model), FALSE);
    g_return_val_if_fail (file != NULL, FALSE);

    if (file_list_contains (model->priv->dirs, file))
    {
        ITER_INIT (iter, model, file, TRUE);
        CHECK_ITER (model, iter);
        return TRUE;
    }
    else if (file_list_contains (model->priv->files, file))
    {
        ITER_INIT (iter, model, file, FALSE);
        CHECK_ITER (model, iter);
        return TRUE;
    }
    else
    {
        ITER_INIT (iter, NULL, NULL, FALSE);
        return FALSE;
    }
}
#endif


gboolean
_moo_folder_model_get_iter_by_name (MooFolderModel *model,
                                    const char     *name,
                                    GtkTreeIter    *iter)
{
    MooFile *file;

    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (model), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);
    g_return_val_if_fail (iter != NULL, FALSE);

    if ((file = file_list_find_name (model->priv->files, name)))
    {
        ITER_INIT (iter, model, file);
        CHECK_ITER (model, iter);
        return TRUE;
    }
    else
    {
        ITER_INIT (iter, NULL, NULL);
        return FALSE;
    }
}


gboolean
_moo_folder_model_get_iter_by_display_name (MooFolderModel *model,
                                            const char     *name,
                                            GtkTreeIter    *iter)
{
    MooFile *file;

    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (model), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);
    g_return_val_if_fail (iter != NULL, FALSE);

    if ((file = file_list_find_display_name (model->priv->files, name)))
    {
        ITER_INIT (iter, model, file);
        CHECK_ITER (model, iter);
        return TRUE;
    }
    else
    {
        ITER_INIT (iter, NULL, NULL);
        return FALSE;
    }
}


#ifdef DEFINE_CHECK_ITER
static gboolean NO_CHECK_ITER = FALSE;

static void CHECK_ITER (MooFolderModel *model, GtkTreeIter *iter)
{
    GtkTreePath *path1, *path2;
    GtkTreeIter iter2;

    if (NO_CHECK_ITER)
    {
        NO_CHECK_ITER = FALSE;
        return;
    }
    else
    {
        NO_CHECK_ITER = TRUE;
    }

    g_assert (iter != NULL);
    g_assert (ITER_MODEL (iter) == model);
    g_assert (ITER_FILE (iter) != NULL);
    g_assert (model_contains_file (model, ITER_FILE (iter)));

    path1 = gtk_tree_model_get_path (GTK_TREE_MODEL (model), iter);
    g_assert (path1 != NULL);
    g_assert (gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter2, path1));

    g_assert (ITER_MODEL (&iter2) == model);
    g_assert (ITER_FILE (&iter2) == ITER_FILE (iter));
    g_assert (ITER_DIR (&iter2) == ITER_DIR (iter));

    path2 = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter2);
    g_assert (path2 != NULL);
    g_assert (!gtk_tree_path_compare (path1, path2));

    gtk_tree_path_free (path1);
    gtk_tree_path_free (path2);
}
#endif /* DEFINE_CHECK_ITER */


/*****************************************************************************/
/* MooFolderFilter
 */

G_DEFINE_TYPE (MooFolderFilter, _moo_folder_filter, GTK_TYPE_TREE_MODEL_FILTER)

static void
_moo_folder_filter_class_init (G_GNUC_UNUSED MooFolderFilterClass *klass)
{
}

static void
_moo_folder_filter_init (G_GNUC_UNUSED MooFolderFilter *filter)
{
}


GtkTreeModel *
_moo_folder_filter_new (MooFolderModel *model)
{
    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (model), NULL);
    return GTK_TREE_MODEL (g_object_new (MOO_TYPE_FOLDER_FILTER,
                                         "child-model", model,
                                         (const char*) NULL));
}


void
_moo_folder_filter_set_folder (MooFolderFilter    *filter,
                               MooFolder          *folder)
{
    GtkTreeModel *model;
    g_return_if_fail (MOO_IS_FOLDER_FILTER (filter));
    model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));
    g_return_if_fail (MOO_IS_FOLDER_MODEL (model));
    _moo_folder_model_set_folder (MOO_FOLDER_MODEL (model), folder);
}


#if 0
MooFolder *
_moo_folder_filter_get_folder (MooFolderFilter *filter)
{
    GtkTreeModel *model;
    g_return_val_if_fail (MOO_IS_FOLDER_FILTER (filter), NULL);
    model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));
    g_return_val_if_fail (MOO_IS_FOLDER_MODEL (model), NULL);
    return _moo_folder_model_get_folder (MOO_FOLDER_MODEL (model));
}
#endif
