/*
 *   moofiltermgr.c
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

#include "mooutils/moofiltermgr.h"
#include "mooutils/mooprefs.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooi18n.h"
#include <string.h>
#include <gtk/gtk.h>

#define NUM_USER_FILTERS  5
#define ALL_FILES_GLOB    "*"

typedef struct {
    GtkFileFilter   *filter;
    GtkFileFilter   *aux;
    char            *description;
    char            *glob;
    gboolean         user;
} Filter;

static Filter       *filter_new             (const char *description,
                                             const char *glob,
                                             gboolean    user);
static void          filter_free            (Filter     *filter);

static const char   *filter_get_glob        (Filter     *filter);
static const char   *filter_get_description (Filter     *filter);
static GtkFileFilter *filter_get_gtk_filter (Filter     *filter);

typedef struct {
    GtkListStore    *filters;
    Filter          *last_filter;
    guint            num_user;
    guint            num_builtin;
    gboolean         has_separator;
    guint            max_num_user;
} FilterStore;

static FilterStore  *filter_store_new               (MooFilterMgr   *mgr);
static void          filter_store_free              (FilterStore    *store);

static void          filter_store_add_builtin       (FilterStore    *store,
                                                     Filter         *filter,
                                                     int             position);
static void          filter_store_add_user          (FilterStore    *store,
                                                     Filter         *filter);
static Filter       *filter_store_get_last          (FilterStore    *store);
static GSList       *filter_store_list_user_filters (FilterStore    *store);


struct _MooFilterMgrPrivate {
    FilterStore *default_store;
    GHashTable  *named_stores;  /* user_id -> FilterStore* */
    GHashTable  *filters;       /* glob -> Filter* */
    gboolean     loaded;
    gboolean     changed;
    guint        save_idle_id;
};

enum {
    COLUMN_DESCRIPTION,
    COLUMN_FILTER,
    NUM_COLUMNS
};


static void     moo_filter_mgr_class_init   (MooFilterMgrClass    *klass);
static void     moo_filter_mgr_init         (MooFilterMgr         *mgr);
static void     moo_filter_mgr_finalize     (GObject                *object);


static gboolean      combo_row_separator_func   (GtkTreeModel  *model,
                                                 GtkTreeIter   *iter,
                                                 gpointer       data);
static FilterStore  *mgr_get_store              (MooFilterMgr   *mgr,
                                                 const char     *user_id,
                                                 gboolean        create);
static Filter       *mgr_get_null_filter        (MooFilterMgr   *mgr);
static Filter       *mgr_new_filter             (MooFilterMgr   *mgr,
                                                 const char     *description,
                                                 const char     *glob,
                                                 gboolean        user);
static Filter       *mgr_get_last_filter        (MooFilterMgr   *mgr,
                                                 const char     *user_id);
static void          mgr_set_last_filter        (MooFilterMgr   *mgr,
                                                 const char     *user_id,
                                                 Filter         *filter);
static Filter       *mgr_new_user_filter        (MooFilterMgr   *mgr,
                                                 const char     *glob,
                                                 const char     *user_id);
static GSList       *mgr_list_user_ids          (MooFilterMgr   *mgr);

static void          mgr_load                   (MooFilterMgr   *mgr);
static void          mgr_save                   (MooFilterMgr   *mgr);


/* MOO_TYPE_FILTER_MGR */
G_DEFINE_TYPE (MooFilterMgr, moo_filter_mgr, G_TYPE_OBJECT)


static void
moo_filter_mgr_class_init (MooFilterMgrClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = moo_filter_mgr_finalize;
}


static void
moo_filter_mgr_init (MooFilterMgr *mgr)
{
    mgr->priv = g_new0 (MooFilterMgrPrivate, 1);

    mgr->priv->filters =
            g_hash_table_new_full (g_str_hash, g_str_equal,
                                   g_free, (GDestroyNotify) filter_free);
    g_hash_table_insert (mgr->priv->filters,
                         g_strdup (ALL_FILES_GLOB),
                         filter_new ("All Files", ALL_FILES_GLOB, FALSE));

    mgr->priv->default_store = NULL;
    mgr->priv->named_stores =
            g_hash_table_new_full (g_str_hash, g_str_equal,
                                   g_free, (GDestroyNotify) filter_store_free);
}


static void
moo_filter_mgr_finalize (GObject *object)
{
    MooFilterMgr *mgr = MOO_FILTER_MGR (object);

    if (mgr->priv->save_idle_id)
        g_source_remove (mgr->priv->save_idle_id);

    filter_store_free (mgr->priv->default_store);
    g_hash_table_destroy (mgr->priv->named_stores);
    g_hash_table_destroy (mgr->priv->filters);

    g_free (mgr->priv);

    G_OBJECT_CLASS (moo_filter_mgr_parent_class)->finalize (object);
}


MooFilterMgr *
moo_filter_mgr_new (void)
{
    return MOO_FILTER_MGR (g_object_new (MOO_TYPE_FILTER_MGR, (const char*) NULL));
}

MooFilterMgr *
moo_filter_mgr_default (void)
{
    static MooFilterMgr *instance;
    if (!instance)
        instance = moo_filter_mgr_new ();
    return instance;
}


void
moo_filter_mgr_init_filter_combo (MooFilterMgr *mgr,
                                  GtkComboBox  *combo,
                                  const char   *user_id)
{
    FilterStore *store;

    g_return_if_fail (MOO_IS_FILTER_MGR (mgr));
    g_return_if_fail (GTK_IS_COMBO_BOX_ENTRY (combo));

    mgr_load (mgr);

    store = mgr_get_store (mgr, user_id, TRUE);
    g_return_if_fail (store != NULL);

    gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store->filters));
    gtk_combo_box_set_row_separator_func (combo, combo_row_separator_func, NULL, NULL);

    if (GTK_IS_COMBO_BOX_ENTRY (combo))
        gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (combo), COLUMN_DESCRIPTION);
}


static gboolean
combo_row_separator_func (GtkTreeModel  *model,
                          GtkTreeIter   *iter,
                          G_GNUC_UNUSED gpointer data)
{
    Filter *filter = NULL;
    gtk_tree_model_get (model, iter, COLUMN_FILTER, &filter, -1);
    return !filter;
}


static FilterStore*
mgr_get_store (MooFilterMgr   *mgr,
               const char     *user_id,
               gboolean        create)
{
    FilterStore *store;

    if (!user_id)
    {
        if (!mgr->priv->default_store && create)
            mgr->priv->default_store = filter_store_new (mgr);
        return mgr->priv->default_store;
    }

    store = (FilterStore*) g_hash_table_lookup (mgr->priv->named_stores, user_id);

    if (!store && create)
    {
        g_return_val_if_fail (g_utf8_validate (user_id, -1, NULL), NULL);
        store = filter_store_new (mgr);
        g_hash_table_insert (mgr->priv->named_stores,
                             g_strdup (user_id),
                             store);
    }

    return store;
}


static void
prepend_id (const char *user_id,
            G_GNUC_UNUSED gpointer store,
            GSList   **list)
{
    *list = g_slist_prepend (*list, g_strdup (user_id));
}

static GSList*
mgr_list_user_ids (MooFilterMgr *mgr)
{
    GSList *list = NULL;
    g_hash_table_foreach (mgr->priv->named_stores, (GHFunc) prepend_id, &list);
    return g_slist_reverse (list);
}


GtkFileFilter*
moo_filter_mgr_get_filter (MooFilterMgr   *mgr,
                           GtkTreeIter    *iter,
                           const char     *user_id)
{
    FilterStore *store;
    Filter *filter = NULL;

    g_return_val_if_fail (MOO_IS_FILTER_MGR (mgr), NULL);
    g_return_val_if_fail (iter != NULL, NULL);

    mgr_load (mgr);

    store = mgr_get_store (mgr, user_id, FALSE);
    g_return_val_if_fail (store != NULL, NULL);

    gtk_tree_model_get (GTK_TREE_MODEL (store->filters), iter,
                        COLUMN_FILTER, &filter, -1);

    return filter_get_gtk_filter (filter);
}


void
moo_filter_mgr_set_last_filter (MooFilterMgr   *mgr,
                                GtkTreeIter    *iter,
                                const char     *user_id)
{
    FilterStore *store;
    Filter *filter = NULL;

    g_return_if_fail (MOO_IS_FILTER_MGR (mgr));
    g_return_if_fail (iter != NULL);

    mgr_load (mgr);

    store = mgr_get_store (mgr, user_id, FALSE);
    g_return_if_fail (store != NULL);

    gtk_tree_model_get (GTK_TREE_MODEL (store->filters), iter,
                        COLUMN_FILTER, &filter, -1);
    g_return_if_fail (filter != NULL);

    mgr_set_last_filter (mgr, user_id, filter);
}


GtkFileFilter*
moo_filter_mgr_get_null_filter (MooFilterMgr *mgr)
{
    g_return_val_if_fail (MOO_IS_FILTER_MGR (mgr), NULL);
    return mgr_get_null_filter(mgr)->filter;
}


static Filter*
mgr_get_null_filter (MooFilterMgr *mgr)
{
    return (Filter*) g_hash_table_lookup (mgr->priv->filters, ALL_FILES_GLOB);
}


GtkFileFilter*
moo_filter_mgr_get_last_filter (MooFilterMgr   *mgr,
                                const char     *user_id)
{
    g_return_val_if_fail (MOO_IS_FILTER_MGR (mgr), NULL);

    mgr_load (mgr);

    return filter_get_gtk_filter (mgr_get_last_filter (mgr, user_id));
}


static Filter*
mgr_get_last_filter (MooFilterMgr   *mgr,
                     const char     *user_id)
{
    FilterStore *store;

    g_return_val_if_fail (MOO_IS_FILTER_MGR (mgr), NULL);

    store = mgr_get_store (mgr, user_id, FALSE);

    if (store)
        return filter_store_get_last (store);
    else
        return NULL;
}


static void
mgr_set_last_filter (MooFilterMgr   *mgr,
                     const char     *user_id,
                     Filter         *filter)
{
    FilterStore *store;

    g_return_if_fail (MOO_IS_FILTER_MGR (mgr));

    store = mgr_get_store (mgr, user_id, TRUE);

    if (store->last_filter != filter)
    {
        store->last_filter = filter;
        mgr->priv->changed = TRUE;
        mgr_save (mgr);
    }
}


GtkFileFilter*
moo_filter_mgr_new_user_filter (MooFilterMgr   *mgr,
                                const char     *glob,
                                const char     *user_id)
{
    g_return_val_if_fail (MOO_IS_FILTER_MGR (mgr), NULL);
    g_return_val_if_fail (glob != NULL, NULL);

    mgr_load (mgr);

    return filter_get_gtk_filter (mgr_new_user_filter (mgr, glob, user_id));
}


static Filter*
mgr_new_user_filter (MooFilterMgr   *mgr,
                     const char     *glob,
                     const char     *user_id)
{
    FilterStore *store;
    Filter *filter;

    filter = mgr_new_filter (mgr, glob, glob, TRUE);
    g_return_val_if_fail (filter != NULL, NULL);

    store = mgr_get_store (mgr, user_id, TRUE);
    filter_store_add_user (store, filter);

    mgr_save (mgr);

    return filter;
}


GtkFileFilter*
moo_filter_mgr_new_builtin_filter (MooFilterMgr   *mgr,
                                   const char     *description,
                                   const char     *glob,
                                   const char     *user_id,
                                   int             position)
{
    FilterStore *store;
    Filter *filter;

    g_return_val_if_fail (MOO_IS_FILTER_MGR (mgr), NULL);
    g_return_val_if_fail (glob != NULL, NULL);
    g_return_val_if_fail (description != NULL, NULL);

    mgr_load (mgr);

    filter = mgr_new_filter (mgr, description, glob, FALSE);
    g_return_val_if_fail (filter != NULL, NULL);

    store = mgr_get_store (mgr, user_id, TRUE);
    filter_store_add_builtin (store, filter, position);

    return filter->filter;
}


static Filter*
mgr_new_filter (MooFilterMgr   *mgr,
                const char     *description,
                const char     *glob,
                gboolean        user)
{
    Filter *filter;

    g_return_val_if_fail (description != NULL, NULL);
    g_return_val_if_fail (glob != NULL, NULL);

    filter = (Filter*) g_hash_table_lookup (mgr->priv->filters, glob);

    if (!filter)
    {
        filter = filter_new (description, glob, user);
        g_return_val_if_fail (filter != NULL, NULL);
        g_hash_table_insert (mgr->priv->filters,
                             g_strdup (glob), filter);
        mgr->priv->changed = TRUE;
    }

    return filter;
}


/*****************************************************************************/
/* Filechooser
 */

static void
dialog_set_filter (MooFilterMgr   *mgr,
                   GtkFileChooser *dialog,
                   Filter         *filter)
{
    GtkEntry *entry = GTK_ENTRY (g_object_get_data (G_OBJECT (dialog), "moo-filter-entry"));
    gtk_entry_set_text (entry, filter_get_description (filter));
    gtk_file_chooser_set_filter (dialog, filter_get_gtk_filter (filter));
    mgr_set_last_filter (mgr,
                         (const char*) g_object_get_data (G_OBJECT (dialog), "moo-filter-user-id"),
                         filter);
}


static void
filter_entry_activated (GtkEntry       *entry,
                        GtkFileChooser *dialog)
{
    const char *text;
    Filter *filter = NULL;
    MooFilterMgr *mgr;

    mgr = (MooFilterMgr*) g_object_get_data (G_OBJECT (dialog), "moo-filter-mgr");
    g_return_if_fail (MOO_IS_FILTER_MGR (mgr));

    text = gtk_entry_get_text (entry);

    if (text && text[0])
        filter = mgr_new_user_filter (mgr, text,
                                      (const char*) g_object_get_data (G_OBJECT (dialog),
                                                                       "moo-filter-user-id"));

    if (!filter)
        filter = mgr_get_null_filter (mgr);

    dialog_set_filter (mgr, dialog, filter);
}


static void
combo_changed (GtkComboBox    *combo,
               GtkFileChooser *dialog)
{
    GtkTreeIter iter;
    Filter *filter;
    MooFilterMgr *mgr;
    FilterStore *store;

    if (!gtk_combo_box_get_active_iter (combo, &iter))
        return;

    mgr = (MooFilterMgr*) g_object_get_data (G_OBJECT (dialog), "moo-filter-mgr");
    g_return_if_fail (MOO_IS_FILTER_MGR (mgr));

    store = mgr_get_store (mgr,
                           (const char*) g_object_get_data (G_OBJECT (dialog),
                                                            "moo-filter-user-id"),
                           FALSE);
    g_return_if_fail (store != NULL);

    gtk_tree_model_get (GTK_TREE_MODEL (store->filters), &iter,
                        COLUMN_FILTER, &filter, -1);
    g_return_if_fail (filter != NULL);

    dialog_set_filter (mgr, dialog, filter);
}


void
moo_filter_mgr_attach (MooFilterMgr   *mgr,
                       GtkFileChooser *dialog,
                       GtkWidget      *parent,
                       const char     *user_id)
{
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *combo;
    GtkWidget *entry;
    Filter *filter;

    g_return_if_fail (MOO_IS_FILTER_MGR (mgr));
    g_return_if_fail (GTK_IS_FILE_CHOOSER (dialog));

    mgr_load (mgr);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_end (GTK_BOX (parent), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("_Show:"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    combo = gtk_combo_box_entry_new ();
    moo_filter_mgr_init_filter_combo (mgr, GTK_COMBO_BOX (combo), user_id);
    gtk_widget_show (combo);
    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);

    entry = GTK_BIN (combo)->child;
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

    g_signal_connect (entry, "activate",
                      G_CALLBACK (filter_entry_activated),
                      dialog);
    g_signal_connect (combo, "changed",
                      G_CALLBACK (combo_changed),
                      dialog);

    g_object_set_data (G_OBJECT (dialog), "moo-filter-combo", combo);
    g_object_set_data (G_OBJECT (dialog), "moo-filter-entry", entry);
    g_object_set_data_full (G_OBJECT (dialog), "moo-filter-mgr",
                            g_object_ref (mgr), g_object_unref);
    g_object_set_data_full  (G_OBJECT (dialog), "moo-filter-user-id",
                             g_strdup (user_id), g_free);

    filter = mgr_get_last_filter (mgr, user_id);

    if (filter)
        dialog_set_filter (mgr, GTK_FILE_CHOOSER (dialog), filter);
}


/*****************************************************************************/
/* FilterStore
 */

static FilterStore*
filter_store_new (MooFilterMgr *mgr)
{
    FilterStore *store = g_new0 (FilterStore, 1);

    store->filters = gtk_list_store_new (NUM_COLUMNS,
                                         G_TYPE_STRING,
                                         G_TYPE_POINTER);
    store->last_filter = NULL;
    store->num_user = 0;
    store->num_builtin = 0;
    store->has_separator = FALSE;
    store->max_num_user = NUM_USER_FILTERS;

    filter_store_add_builtin (store, mgr_get_null_filter (mgr), -1);

    return store;
}


static void
filter_store_free (FilterStore *store)
{
    if (store)
    {
        g_object_unref (store->filters);
        g_free (store);
    }
}


static GSList*
filter_store_list_user_filters (FilterStore *store)
{
    GSList *list = NULL;
    GtkTreeIter iter;
    guint offset;

    g_return_val_if_fail (store != NULL, NULL);

    if (!store->num_user)
        return NULL;

    offset = store->num_builtin + (store->has_separator ? 1 : 0);

    if (!gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store->filters), &iter, NULL, offset))
    {
        g_critical ("oops");
        return NULL;
    }

    do
    {
        Filter *filter = NULL;
        gtk_tree_model_get (GTK_TREE_MODEL (store->filters), &iter,
                            COLUMN_FILTER, &filter, -1);
        list = g_slist_prepend (list, filter);
    }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store->filters), &iter));

    return g_slist_reverse (list);
}


static gboolean
filter_store_find_builtin (FilterStore    *store,
                           Filter         *filter,
                           GtkTreeIter    *iter)
{
    guint i;
    Filter *filt = NULL;

    for (i = 0; i < store->num_builtin; ++i)
    {
        gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store->filters),
                                       iter, NULL, i);
        gtk_tree_model_get (GTK_TREE_MODEL (store->filters), iter,
                            COLUMN_FILTER, &filt, -1);
        if (filter == filt)
            return TRUE;
    }

    return FALSE;
}


static void
filter_store_add_builtin (FilterStore    *store,
                          Filter         *filter,
                          int             position)
{
    GtkTreeIter iter;

    g_return_if_fail (store != NULL);
    g_return_if_fail (filter != NULL);

    store->last_filter = filter;

    if (filter_store_find_builtin (store, filter, &iter))
        return;

    if (position < 0 || position > (int) store->num_builtin)
        position = store->num_builtin;

    gtk_list_store_insert (store->filters, &iter, position);
    gtk_list_store_set (store->filters, &iter,
                        COLUMN_DESCRIPTION, filter_get_description (filter),
                        COLUMN_FILTER, filter, -1);
    store->num_builtin++;

    if (store->num_user && !store->has_separator)
    {
        gtk_list_store_insert (store->filters, &iter, store->num_builtin);
        store->has_separator = TRUE;
    }
}


static gboolean
filter_store_find_user (FilterStore    *store,
                        Filter         *filter,
                        GtkTreeIter    *iter)
{
    guint i, offset;
    Filter *filt = NULL;

    offset = store->num_builtin + (store->has_separator ? 1 : 0);

    for (i = offset; i < offset + store->num_user; ++i)
    {
        gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store->filters),
                                       iter, NULL, i);
        gtk_tree_model_get (GTK_TREE_MODEL (store->filters), iter,
                            COLUMN_FILTER, &filt, -1);
        if (filter == filt)
            return TRUE;
    }

    return FALSE;
}


static void
filter_store_add_user (FilterStore    *store,
                       Filter         *filter)
{
    GtkTreeIter iter;

    g_return_if_fail (store != NULL);
    g_return_if_fail (filter != NULL);

    store->last_filter = filter;

    if (filter_store_find_builtin (store, filter, &iter))
        return;

    if (filter_store_find_user (store, filter, &iter))
    {
        gtk_list_store_remove (store->filters, &iter);
        store->num_user--;
    }

    gtk_list_store_insert (store->filters, &iter,
                           store->num_builtin + (store->has_separator ? 1 : 0));
    gtk_list_store_set (store->filters, &iter,
                        COLUMN_DESCRIPTION, filter_get_description (filter),
                        COLUMN_FILTER, filter, -1);
    store->num_user++;

    if (store->num_builtin && !store->has_separator)
    {
        gtk_list_store_insert (store->filters, &iter, store->num_builtin);
        store->has_separator = TRUE;
    }

    if (store->num_user > store->max_num_user)
    {
        gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store->filters),
                                       &iter, NULL,
                                       store->num_builtin +
                                               (store->has_separator ? 1 : 0) +
                                               store->num_user - 1);
        gtk_list_store_remove (store->filters, &iter);
        store->num_user--;
    }
}


static Filter       *filter_store_get_last      (FilterStore    *store)
{
    if (!store->last_filter)
    {
        GtkTreeIter iter;
        Filter *filter = NULL;

        gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store->filters), &iter);
        gtk_tree_model_get (GTK_TREE_MODEL (store->filters), &iter,
                            COLUMN_FILTER, &filter, -1);
        store->last_filter = filter;
    }

    return store->last_filter;
}


/*****************************************************************************/
/* Filter
 */

#define NEGATE_CHAR     '!'
#define GLOB_SEPARATOR  ";"

static gboolean
neg_filter_func (const GtkFileFilterInfo *filter_info,
                 Filter *filter)
{
    return !gtk_file_filter_filter (filter->aux, filter_info);
}


static Filter*
filter_new (const char *description,
            const char *glob,
            gboolean    user)
{
    Filter *filter;
    char **globs, **p;
    gboolean negative;

    g_return_val_if_fail (description != NULL, NULL);
    g_return_val_if_fail (g_utf8_validate (description, -1, NULL), NULL);
    g_return_val_if_fail (g_utf8_validate (glob, -1, NULL), NULL);
    g_return_val_if_fail (glob != NULL && glob[0] != 0, NULL);
    g_return_val_if_fail (glob[0] != NEGATE_CHAR || glob[1] != 0, NULL);

    if (glob[0] == NEGATE_CHAR)
    {
        negative = TRUE;
        globs = g_strsplit (glob + 1, GLOB_SEPARATOR, 0);
    }
    else
    {
        negative = FALSE;
        globs = g_strsplit (glob, GLOB_SEPARATOR, 0);
    }

    g_return_val_if_fail (globs != NULL, NULL);

    filter = g_new0 (Filter, 1);

    filter->description = g_strdup (description);
    filter->glob = g_strdup (glob);
    filter->user = user;

    filter->filter = gtk_file_filter_new ();
    g_object_ref_sink (filter->filter);
    gtk_file_filter_set_name (filter->filter, description);

    if (negative)
    {
        filter->aux = gtk_file_filter_new ();
        g_object_ref_sink (filter->aux);

        for (p = globs; *p != NULL; p++)
            gtk_file_filter_add_pattern (filter->aux, *p);

        gtk_file_filter_add_custom (filter->filter,
                                    gtk_file_filter_get_needed (filter->aux),
                                    (GtkFileFilterFunc) neg_filter_func,
                                    filter, NULL);
    }
    else
    {
        for (p = globs; *p != NULL; p++)
            gtk_file_filter_add_pattern (filter->filter, *p);
    }

    g_strfreev (globs);
    return filter;
}


static void
filter_free (Filter *filter)
{
    if (filter)
    {
        if (filter->filter)
            g_object_unref (filter->filter);
        filter->filter = NULL;
        if (filter->aux)
            g_object_unref (filter->aux);
        filter->aux = NULL;
        g_free (filter->description);
        filter->description = NULL;
        g_free (filter->glob);
        filter->glob = NULL;
        g_free (filter);
    }
}


static GtkFileFilter *filter_get_gtk_filter (Filter     *filter)
{
    g_return_val_if_fail (filter != NULL, NULL);
    return filter->filter;
}


static const char*
filter_get_glob (Filter *filter)
{
    g_return_val_if_fail (filter != NULL, NULL);
    return filter->glob;
}


static const char*
filter_get_description (Filter *filter)
{
    g_return_val_if_fail (filter != NULL, NULL);
    return filter->description;
}


/***************************************************************************/
/* Loading and saving
 */

#define FILTERS_ROOT            "Editor/filters"
#define ELEMENT_DEFAULT         "default"
#define ELEMENT_SET             "set"
#define ELEMENT_FILTER          "filter"
#define ELEMENT_LAST            "last"
#define PROP_USER_ID            "user-id"
#define PROP_USER               "user"
#define PROP_DESCRIPTION        "description"


static void
mgr_load_set (MooFilterMgr      *mgr,
              const char        *user_id,
              MooMarkupNode     *root)
{
    MooMarkupNode *elm;

    for (elm = root->last; elm != NULL; elm = elm->prev)
    {
        if (!MOO_MARKUP_IS_ELEMENT (elm))
            continue;

        if (!strcmp (elm->name, ELEMENT_FILTER))
        {
            const char *glob = moo_markup_get_content (elm);

            if (!glob || !glob[0])
            {
                g_warning ("empty glob in filter entry");
                continue;
            }

            moo_filter_mgr_new_user_filter (mgr, glob, user_id);
        }
        else if (!strcmp (elm->name, ELEMENT_LAST))
        {
            const char *glob = moo_markup_get_content (elm);
            const char *user_str = moo_markup_get_prop (elm, PROP_USER);
            const char *description = moo_markup_get_prop (elm, PROP_DESCRIPTION);
            gboolean user;

            if (!glob || !glob[0])
            {
                g_warning ("empty glob in filter entry");
                continue;
            }

            if (user_str)
                user = strcmp (user_str, "TRUE") ? FALSE : TRUE;
            else
                user = FALSE;

            if (!user && !description)
            {
                g_warning ("no description in filter entry");
                continue;
            }

            /* XXX */
            if (user)
            {
                moo_filter_mgr_new_user_filter (mgr, glob, user_id);
            }
            else
            {
                Filter *filter;
                moo_filter_mgr_new_builtin_filter (mgr, description, glob, user_id, -1);
                filter = mgr_new_filter (mgr, description, glob, FALSE);
                g_return_if_fail (filter != NULL);
                mgr_set_last_filter (mgr, user_id, filter);
            }
        }
        else
        {
            g_warning ("invalid '%s' element", elm->name);
        }
    }
}


static void
mgr_load (MooFilterMgr *mgr)
{
    MooMarkupNode *xml;
    MooMarkupNode *root, *elm;

    if (mgr->priv->loaded)
        return;

    mgr->priv->loaded = TRUE;
    mgr->priv->changed = FALSE;

    xml = moo_prefs_get_markup (MOO_PREFS_RC);
    g_return_if_fail (xml != NULL);

    root = moo_markup_get_element (xml, FILTERS_ROOT);

    if (!root)
        return;

    for (elm = root->children; elm != NULL; elm = elm->next)
    {
        if (!MOO_MARKUP_IS_ELEMENT (elm))
            continue;

        if (!strcmp (elm->name, ELEMENT_DEFAULT))
        {
            mgr_load_set (mgr, NULL, elm);
        }
        else if (!strcmp (elm->name, ELEMENT_SET))
        {
            const char *user_id = moo_markup_get_prop (elm, PROP_USER_ID);

            if (!user_id || !user_id[0])
            {
                g_warning ("empty user id");
                continue;
            }

            mgr_load_set (mgr, user_id, elm);
        }
        else
        {
            g_warning ("invalid '%s' element", elm->name);
        }
    }

    mgr->priv->changed = FALSE;
}


/* XXX */
static void
mgr_save_set (MooFilterMgr      *mgr,
              const char        *user_id,
              MooMarkupNode     *root)
{
    MooMarkupNode *elm;
    GSList *list, *l;
    FilterStore *store;
    Filter *filter;

    store = mgr_get_store (mgr, user_id, FALSE);
    g_return_if_fail (store != NULL);

    if (!store->num_user && !store->last_filter)
        return;

    if (store->last_filter)
    {
        filter = store->last_filter;
        elm = moo_markup_create_text_element (root, ELEMENT_LAST,
                                              filter_get_glob (filter));
        moo_markup_set_prop (elm, PROP_USER, filter->user ? "TRUE" : "FALSE");
        if (!filter->user)
            moo_markup_set_prop (elm, PROP_DESCRIPTION, filter->description);
    }

    list = filter_store_list_user_filters (store);

    if (!list)
        return;

    for (l = list; l != NULL; l = l->next)
        moo_markup_create_text_element (root, ELEMENT_FILTER,
                                        filter_get_glob ((Filter*) l->data));

    g_slist_free (list);
}


static gboolean
mgr_do_save (MooFilterMgr *mgr)
{
    MooMarkupNode *xml;
    MooMarkupNode *root, *elm;
    GSList *user_ids, *l;

    mgr->priv->save_idle_id = 0;

    if (!mgr->priv->changed)
        return FALSE;

    xml = moo_prefs_get_markup (MOO_PREFS_RC);
    g_return_val_if_fail (xml != NULL, FALSE);

    root = moo_markup_get_element (xml, FILTERS_ROOT);

    if (root)
        moo_markup_delete_node (root);

    root = NULL;

    user_ids = mgr_list_user_ids (mgr);
    user_ids = g_slist_prepend (user_ids, NULL);

    for (l = user_ids; l != NULL; l = l->next)
    {
        FilterStore *store = mgr_get_store (mgr, (const char*) l->data, FALSE);
        g_return_val_if_fail (!l->data || store != NULL, FALSE);

        if (!store)
            continue;

        if (store->num_user || store->last_filter != mgr_get_null_filter (mgr))
        {
            if (!root)
            {
                root = moo_markup_create_element (xml, FILTERS_ROOT);
                g_return_val_if_fail (root != NULL, FALSE);
            }

            if (l->data)
            {
                elm = moo_markup_create_element (root, ELEMENT_SET);
                moo_markup_set_prop (elm, PROP_USER_ID, (const char*) l->data);
            }
            else
            {
                elm = moo_markup_create_element (root, ELEMENT_DEFAULT);
            }

            mgr_save_set (mgr, (const char*) l->data, elm);
        }
    }

    mgr->priv->changed = FALSE;

    g_slist_foreach (user_ids, (GFunc) g_free, NULL);
    g_slist_free (user_ids);
    return FALSE;
}


static void
mgr_save (MooFilterMgr *mgr)
{
    if (!mgr->priv->save_idle_id)
        mgr->priv->save_idle_id = g_idle_add ((GSourceFunc) mgr_do_save, mgr);
}
