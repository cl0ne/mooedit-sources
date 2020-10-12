/*
 *   moofilelist.c
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

#include "config.h"
#include "plugins/mooplugin-builtin.h"
#include "mooedit/mooplugin-macro.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mootype-macros.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooutils-treeview.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/mooatom.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_ASSERT(expr) g_assert (expr)

#define FILE_LIST_PLUGIN_ID "MooFileList"
#define WindowPlugin FileListWindowPlugin

#define FILE_LIST(model) ((FileList*)model)
#define GROUP_ITEM(itm) ((Group*)itm)
#define FILE_ITEM(itm) ((File*)itm)
#define ITEM(itm) ((Item*)itm)
#define ITEM_IS_FILE(itm) ((itm) && ((Item*)(itm))->type == ITEM_FILE)
#define ITEM_IS_GROUP(itm) ((itm) && ((Item*)(itm))->type == ITEM_GROUP)

#define CONFIG_FILE "file-list-config.xml"
#define ELM_CONFIG "file-list-config"
#define PROP_VERSION "version"
#define VALUE_VERSION "1.0"
#define ELM_GROUP "group"
#define ELM_FILE "file"
#define ELM_UI "ui"
#define PROP_NAME "name"
#define PROP_URI "uri"
#define PROP_EXPANDED_ROWS "expanded-rows"
#define PROP_SELECTED_ROW "selected-row"

typedef struct Item Item;
typedef struct Group Group;
typedef struct File File;
typedef struct FileListWindowPlugin FileListWindowPlugin;

enum {
    COLUMN_ITEM,
    COLUMN_TOOLTIP
};

typedef enum {
    ITEM_FILE,
    ITEM_GROUP
} ItemType;

struct Item {
    ItemType type;
    guint ref_count;
};

struct File {
    Item base;
    char *uri;
    char *display_basename;
    char *display_name;
    MooEdit *doc;
};

struct Group {
    Item base;
    char *name;
};

typedef struct {
    GtkTreeStore base;
    int n_user_items;
    GSList *docs;
    FileListWindowPlugin *plugin;
} FileList;

typedef struct {
    GtkTreeStoreClass base_class;
} FileListClass;

typedef struct {
    MooPlugin parent;
} FileListPlugin;

typedef struct {
    GSList *expanded_rows;
    GtkTreePath *selected_row;
} UIConfig;

struct FileListWindowPlugin {
    MooWinPlugin parent;
    FileList *list;
    GtkTreeView *treeview;
    GtkTreeViewColumn *column;
    GtkCellRenderer *text_cell;
    gboolean first_time_show;
    guint update_idle;
    guint update_ui_idle;
    char *filename;
    UIConfig *ui_config;
};

#define TREE_MODEL_ROW_ATOM (tree_model_row_atom ())
MOO_DEFINE_ATOM (GTK_TREE_MODEL_ROW, tree_model_row)

MOO_DEFINE_QUARK_STATIC (moo-file-list-plugin-model-row, file_list_row_quark)
#define FILE_LIST_QUARK (file_list_quark ())
MOO_DEFINE_QUARK_STATIC (moo-file-list-plugin, file_list_quark)

const ObjectDataAccessor<MooEdit, GtkTreeRowReference*> file_list_row_data(file_list_row_quark());


static GType         item_get_type              (void) G_GNUC_CONST;
static Item         *item_ref                   (Item           *item);
static void          item_unref                 (Item           *item);

static void          file_list_load_config      (FileList       *list,
                                                 const char     *filename,
                                                 UIConfig      **ui_config);
static void          file_list_save_config      (FileList       *list,
                                                 const char     *filename,
                                                 UIConfig       *ui_config);
static void          file_list_update           (FileList       *list,
                                                 GSList         *docs);
static void          file_list_shutdown         (FileList       *list);

static void          window_plugin_queue_update (WindowPlugin   *plugin);
static void          window_plugin_queue_update_ui (WindowPlugin *plugin);

static void file_list_drag_source_iface_init    (GtkTreeDragSourceIface *iface);
static void file_list_drag_dest_iface_init      (GtkTreeDragDestIface   *iface);

static gboolean drag_source_row_draggable       (GtkTreeDragSource  *drag_source,
                                                 GtkTreePath        *path);
static gboolean drag_source_drag_data_get       (GtkTreeDragSource  *drag_source,
                                                 GtkTreePath        *path,
                                                 GtkSelectionData   *selection_data);
static gboolean drag_source_drag_data_delete    (GtkTreeDragSource  *drag_source,
                                                 GtkTreePath        *path);
static gboolean drag_dest_drag_data_received    (GtkTreeDragDest    *drag_dest,
                                                 GtkTreePath        *dest,
                                                 GtkSelectionData   *selection_data);
static gboolean drag_dest_row_drop_possible     (GtkTreeDragDest    *drag_dest,
                                                 GtkTreePath        *dest_path,
                                                 GtkSelectionData   *selection_data);

MOO_DEFINE_BOXED_TYPE_STATIC_R (MooFileListItem, item)
MOO_DEFINE_TYPE_STATIC_WITH_CODE (FileList, file_list, GTK_TYPE_TREE_STORE,
                                  G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_SOURCE,
                                                         file_list_drag_source_iface_init)
                                  G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_DEST,
                                                         file_list_drag_dest_iface_init))


static void
file_list_drag_source_iface_init (GtkTreeDragSourceIface *iface)
{
    iface->row_draggable = drag_source_row_draggable;
    iface->drag_data_get = drag_source_drag_data_get;
    iface->drag_data_delete = drag_source_drag_data_delete;
}

static void
file_list_drag_dest_iface_init (GtkTreeDragDestIface *iface)
{
    iface->drag_data_received = drag_dest_drag_data_received;
    iface->row_drop_possible = drag_dest_row_drop_possible;
}


static void
file_list_init (FileList *list)
{
    GType types[2];

    types[COLUMN_ITEM] = item_get_type ();
    types[COLUMN_TOOLTIP] = G_TYPE_STRING;

    gtk_tree_store_set_column_types (GTK_TREE_STORE (list), 2, types);

    list->n_user_items = 0;
    list->docs = nullptr;
}

static void
file_list_finalize (GObject *object)
{
    DEBUG_ASSERT (!FILE_LIST (object)->docs);
    G_OBJECT_CLASS (file_list_parent_class)->finalize (object);
}

static void
file_list_class_init (FileListClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = file_list_finalize;
}

static Item *
get_item_at_iter (FileList    *list,
                  GtkTreeIter *iter)
{
    Item *item = nullptr;

    gtk_tree_model_get (GTK_TREE_MODEL (list), iter, COLUMN_ITEM, &item, -1);

    if (item)
        item_unref (item);

    return item;
}

static Item *
get_item_at_path (FileList    *list,
                  GtkTreePath *path)
{
    GtkTreeIter iter;

    if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &iter, path))
        return nullptr;
    else
        return get_item_at_iter (list, &iter);
}


static Group *
group_new (const char *name)
{
    Group *grp = g_slice_new0 (Group);

    ITEM (grp)->ref_count = 1;
    ITEM (grp)->type = ITEM_GROUP;

    grp->name = g_strdup (name);

    return grp;
}

static void
group_free (Group *grp)
{
    if (grp)
    {
        g_free (grp->name);
        g_slice_free (Group, grp);
    }
}

static char *
uri_get_basename (const char *uri)
{
    const char *last_slash = strrchr (uri, '/');
    if (last_slash && last_slash[1])
        return g_strdup (last_slash + 1);
    else
        return g_strdup (uri);
}

static gstr
file_get_uri (File *file)
{
    if (file->uri)
        return gstr(file->uri);
    else
        return gstr::take(moo_edit_get_uri (file->doc));
}

static void
file_update (File *file)
{
    if (!file->uri && file->doc)
    {
        g_free (file->display_name);
        g_free (file->display_basename);
        file->display_name = g_strdup (moo_edit_get_display_name (file->doc));
        file->display_basename = g_strdup (moo_edit_get_display_basename (file->doc));
    }
}

static void
file_set_doc (File    *file,
              MooEdit *doc)
{
    if (doc)
        g_object_ref (doc);
    if (file->doc)
        g_object_unref (file->doc);

    file->doc = doc;
    file_update (file);
}

static File *
file_new (void)
{
    File *file = g_slice_new0 (File);

    ITEM (file)->ref_count = 1;
    ITEM (file)->type = ITEM_FILE;

    file->uri = nullptr;
    file->doc = nullptr;
    file->display_name = nullptr;
    file->display_basename = nullptr;

    return file;
}

static Item *
file_new_doc (MooEdit *doc)
{
    File *file;

    g_return_val_if_fail (MOO_IS_EDIT (doc), nullptr);

    file = file_new ();
    file_set_doc (file, doc);

    return ITEM (file);
}

static void
file_set_uri (File       *file,
              const char *uri)
{
    char *tmp;
    char *filename;

    g_return_if_fail (file != nullptr);
    g_return_if_fail (uri != nullptr);

    tmp = file->uri;
    file->uri = g_strdup (uri);
    g_free (tmp);

    g_free (file->display_name);
    g_free (file->display_basename);

    filename = g_filename_from_uri (uri, nullptr, nullptr);

    if (filename)
    {
        file->display_name = g_filename_display_name (filename);
        file->display_basename = g_filename_display_basename (filename);
    }
    else
    {
        file->display_name = g_strdup (uri);
        file->display_basename = uri_get_basename (uri);
    }

    g_free (filename);
}

static Item *
file_new_uri (const char *uri)
{
    File *file;

    g_return_val_if_fail (uri != nullptr, nullptr);

    file = file_new ();
    file_set_uri (file, uri);

    return ITEM (file);
}

static void
file_free (File *file)
{
    if (file)
    {
        if (file->doc)
            g_object_unref (file->doc);
        g_free (file->uri);
        g_free (file->display_name);
        g_free (file->display_basename);
        g_slice_free (File, file);
    }
}

static const char *
item_get_tooltip (Item *item)
{
    if (ITEM_IS_FILE (item))
        return FILE_ITEM (item)->display_name;
    else
        return nullptr;
}

static Item *
item_ref (Item *item)
{
    if (item)
        item->ref_count += 1;
    return item;
}

static void
item_unref (Item *item)
{
    if (item && !--item->ref_count)
    {
        switch (item->type)
        {
            case ITEM_FILE:
                file_free (FILE_ITEM (item));
                break;
            case ITEM_GROUP:
                group_free (GROUP_ITEM (item));
                break;
        }
    }
}


static gboolean
file_list_iter_is_auto (FileList    *list,
                        GtkTreeIter *iter)
{
    gboolean retval;
    GtkTreePath *path;

    path = gtk_tree_model_get_path (GTK_TREE_MODEL (list), iter);

    retval = gtk_tree_path_get_depth (path) == 1 &&
             gtk_tree_path_get_indices (path)[0] >= list->n_user_items;

    gtk_tree_path_free (path);
    return retval;
}


static void
check_list (G_GNUC_UNUSED FileList *list)
{
#ifdef DEBUG
    GtkTreeIter iter;
    int index = 0;

    if (gtk_tree_model_iter_children (GTK_TREE_MODEL (list), &iter, nullptr))
    {
        do
        {
            Item *item = get_item_at_iter (list, &iter);

            if (list->n_user_items)
            {
                g_assert (index == list->n_user_items || item != nullptr);
                g_assert (index != list->n_user_items || item == nullptr);
            }
            else
            {
                g_assert (item != nullptr);
            }

            index += 1;
        }
        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list), &iter));
    }

    g_assert (list->n_user_items == 0 ||
              list->n_user_items < gtk_tree_model_iter_n_children (GTK_TREE_MODEL (list), nullptr));
#endif
}


static gboolean
iter_find_uri (FileList     *list,
               const char   *uri,
               GtkTreeIter  *parent,
               GtkTreeIter  *dest)
{
    GtkTreeIter iter;

    if (parent)
    {
        Item *item = get_item_at_iter (list, parent);

        if (ITEM_IS_FILE (item) &&
            FILE_ITEM (item)->uri &&
            strcmp (FILE_ITEM (item)->uri, uri) == 0)
        {
            *dest = *parent;
            return TRUE;
        }
    }

    if (gtk_tree_model_iter_children (GTK_TREE_MODEL (list), &iter, parent))
        do
        {
            if (iter_find_uri (list, uri, &iter, dest))
                return TRUE;
        }
        while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list), &iter));

    return FALSE;
}

static gboolean
file_list_find_uri (FileList    *list,
                    const char  *uri,
                    GtkTreeIter *iter)
{
    return iter_find_uri (list, uri, nullptr, iter);
}


static void
file_list_remove_row (FileList    *list,
                      GtkTreeIter *iter)
{
    GtkTreeIter parent;
    gboolean last_user_item = FALSE;

    if (!gtk_tree_model_iter_parent (GTK_TREE_MODEL (list), &parent, iter) &&
        !file_list_iter_is_auto (list, iter))
    {
        DEBUG_ASSERT (list->n_user_items > 0);
        list->n_user_items -= 1;
        last_user_item = list->n_user_items == 0;
    }

    gtk_tree_store_remove (GTK_TREE_STORE (list), iter);

    if (last_user_item)
    {
        GtkTreeIter sep;
        gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (list), &sep, nullptr, 0);
        gtk_tree_store_remove (GTK_TREE_STORE (list), &sep);
    }

    check_list (list);
}

static void
file_list_append_row (FileList    *list,
                      Item        *item,
                      GtkTreeIter *iter)
{
    gtk_tree_store_append (GTK_TREE_STORE (list), iter, nullptr);
    gtk_tree_store_set (GTK_TREE_STORE (list), iter,
                        COLUMN_ITEM, item,
                        COLUMN_TOOLTIP, item_get_tooltip (item),
                        -1);
}

static void
file_list_insert_row (FileList    *list,
                      Item        *item,
                      GtkTreeIter *iter,
                      GtkTreeIter *parent_iter,
                      int          index)
{
    gboolean first_user_item = FALSE;

    if (index < 0)
    {
        if (!parent_iter)
            index = list->n_user_items;
        else
            index = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (list),
                                                    parent_iter);
    }

    if (!parent_iter)
    {
        index = MIN (index, list->n_user_items);
        list->n_user_items += 1;
        first_user_item = list->n_user_items == 1;
    }

    gtk_tree_store_insert (GTK_TREE_STORE (list), iter, parent_iter, index);
    gtk_tree_store_set (GTK_TREE_STORE (list), iter,
                        COLUMN_ITEM, item,
                        COLUMN_TOOLTIP, item_get_tooltip (item),
                        -1);

    if (first_user_item)
    {
        GtkTreeIter sep;
        gtk_tree_store_insert (GTK_TREE_STORE (list), &sep, nullptr, list->n_user_items);
        gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (list), iter, parent_iter, index);
    }

    check_list (list);
}


static void
doc_filename_changed (FileList *list)
{
    if (list->plugin)
        window_plugin_queue_update (list->plugin);
}

static void
connect_doc (FileList *list,
             MooEdit  *doc)
{
    DEBUG_ASSERT (!g_slist_find (list->docs, doc));
    list->docs = g_slist_prepend (list->docs, g_object_ref (doc));
    g_signal_connect_swapped (doc, "filename-changed",
                              G_CALLBACK (doc_filename_changed),
                              list);
}

static void
disconnect_doc (FileList *list,
                MooEdit  *doc)
{
    DEBUG_ASSERT (g_slist_find (list->docs, doc));
    list->docs = g_slist_remove (list->docs, doc);
    g_signal_handlers_disconnect_by_func (doc,
                                          (gpointer) doc_filename_changed,
                                          list);
    g_object_unref (doc);
}

static GtkTreeRowReference *
get_doc_row (FileList *list, MooEdit *doc)
{
    GtkTreeRowReference *row = file_list_row_data.get(doc);
    if (row && g_object_get_qdata (G_OBJECT (doc), FILE_LIST_QUARK) != list->plugin)
        row = nullptr;
    return row;
}

static void
file_list_add_doc (FileList *list,
                   MooEdit  *doc,
                   gboolean  new_)
{
    GtkTreeIter iter;
    Item *item;
    GtkTreeRowReference *row;
    GtkTreePath *path;

    DEBUG_ASSERT (!new_ || !get_doc_row (list, doc));
    DEBUG_ASSERT (new_ == !g_slist_find (list->docs, doc));

    gstr uri = gstr::take (moo_edit_get_uri (doc));

    if (!uri.empty() && file_list_find_uri (list, uri.get(), &iter))
    {
        item = get_item_at_iter (list, &iter);
        DEBUG_ASSERT (ITEM_IS_FILE (item) && !FILE_ITEM (item)->doc);
        file_set_doc (FILE_ITEM (item), doc);
    }
    else
    {
        item = file_new_doc (doc);
        file_list_append_row (list, item, &iter);
        item_unref (item);
    }

    path = gtk_tree_model_get_path (GTK_TREE_MODEL (list), &iter);
    row = gtk_tree_row_reference_new (GTK_TREE_MODEL (list), path);
    file_list_row_data.set(doc, row, (GDestroyNotify) gtk_tree_row_reference_free);
    g_object_set_qdata (G_OBJECT (doc), FILE_LIST_QUARK, list->plugin);

    if (new_)
        connect_doc (list, doc);

    gtk_tree_path_free (path);
}

static gboolean
doc_get_list_iter (FileList    *list,
                   MooEdit     *doc,
                   GtkTreeIter *iter)
{
    GtkTreeRowReference *row;
    GtkTreePath *path;

    row = get_doc_row (list, doc);

    if (!row || !gtk_tree_row_reference_valid (row))
        return FALSE;

    path = gtk_tree_row_reference_get_path (row);
    gtk_tree_model_get_iter (GTK_TREE_MODEL (list), iter, path);
    gtk_tree_path_free (path);
    return TRUE;
}

static void
file_list_update_doc (FileList *list,
                      MooEdit  *doc)
{
    GtkTreeIter iter;
    Item *item;

    if (!g_slist_find (list->docs, doc))
    {
        file_list_add_doc (list, doc, TRUE);
        return;
    }

    if (!doc_get_list_iter (list, doc, &iter))
    {
        file_list_add_doc (list, doc, FALSE);
        return;
    }

    DEBUG_ASSERT (!file_list_iter_is_auto (list, &iter));

    item = get_item_at_iter (list, &iter);
    DEBUG_ASSERT (ITEM_IS_FILE (item) && FILE_ITEM (item)->doc == doc);
    DEBUG_ASSERT (FILE_ITEM (item)->uri != nullptr);

    gstr new_uri = gstr::take (moo_edit_get_uri (doc));

    if (new_uri.empty() || strcmp (new_uri.get(), FILE_ITEM (item)->uri) != 0)
    {
        file_list_row_data.set(doc, nullptr);
        file_set_doc (FILE_ITEM (item), nullptr);
        file_list_add_doc (list, doc, FALSE);
    }
}

static void
file_list_remove_doc (FileList *list,
                      MooEdit  *doc)
{
    GtkTreeIter iter;

    DEBUG_ASSERT (g_slist_find (list->docs, doc) != nullptr);

    if (doc_get_list_iter (list, doc, &iter))
    {
        Item *item;

        item = get_item_at_iter (list, &iter);
        DEBUG_ASSERT (ITEM_IS_FILE (item) && FILE_ITEM (item)->doc == doc);
        DEBUG_ASSERT (file_list_iter_is_auto (list, &iter) == !FILE_ITEM (item)->uri);

        if (file_list_iter_is_auto (list, &iter))
            file_list_remove_row (list, &iter);
        else
            file_set_doc (FILE_ITEM (item), nullptr);
    }

    if (get_doc_row (list, doc))
    {
        file_list_row_data.set(doc, nullptr);
        g_object_set_qdata (G_OBJECT (doc), FILE_LIST_QUARK, nullptr);
    }

    disconnect_doc (list, doc);
}

static void
remove_auto_items (FileList *list)
{
    GtkTreeIter iter;

    if (list->n_user_items)
        while (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (list), &iter,
                                              nullptr, list->n_user_items + 1))
            gtk_tree_store_remove (GTK_TREE_STORE (list), &iter);
    else
        while (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (list), &iter, nullptr, 0))
            gtk_tree_store_remove (GTK_TREE_STORE (list), &iter);
}

static void
file_list_update (FileList *list,
                  GSList   *docs)
{
    GSList *l;
    GSList *old_docs;

    remove_auto_items (list);

    for (l = docs; l != nullptr; l = l->next)
        file_list_update_doc (list, (MooEdit*) l->data);

    old_docs = g_slist_copy (list->docs);
    for (l = docs; l != nullptr; l = l->next)
        old_docs = g_slist_remove (old_docs, l->data);
    for (l = old_docs; l != nullptr; l = l->next)
        file_list_remove_doc (list, (MooEdit*) l->data);

    check_list (list);

    g_slist_free (old_docs);
}

static void
file_list_shutdown (FileList *list)
{
    while (list->docs)
    {
        GtkTreeIter iter;
        MooEdit *doc;

        doc = (MooEdit*) list->docs->data;

        if (doc_get_list_iter (list, doc, &iter))
        {
            Item *item = get_item_at_iter (list, &iter);
            DEBUG_ASSERT (ITEM_IS_FILE (item));
            file_set_doc (FILE_ITEM (item), nullptr);
        }

        if (get_doc_row (list, doc))
        {
            file_list_row_data.set(doc, nullptr);
            g_object_set_qdata (G_OBJECT (doc), FILE_LIST_QUARK, nullptr);
        }

        disconnect_doc (list, doc);
    }
}


static UIConfig *
ui_config_new (void)
{
    UIConfig *cfg = g_new0 (UIConfig, 1);
    cfg->expanded_rows = nullptr;
    cfg->selected_row = nullptr;
    return cfg;
}

static void
ui_config_free (UIConfig *cfg)
{
    if (cfg)
    {
        g_slist_foreach (cfg->expanded_rows, (GFunc) gtk_tree_path_free, nullptr);
        g_slist_free (cfg->expanded_rows);
        gtk_tree_path_free (cfg->selected_row);
        g_free (cfg);
    }
}


static void
parse_node (FileList      *list,
            MooMarkupNode *elm,
            GtkTreeIter   *parent,
            const char    *filename,
            UIConfig     **ui_config)
{
    if (strcmp (elm->name, ELM_GROUP) == 0)
    {
        GtkTreeIter iter;
        const char *name;
        MooMarkupNode *child;
        Group *group;

        if (!(name = moo_markup_get_prop (elm, PROP_NAME)) || !name[0])
        {
            g_critical ("in file %s, element %s: name missing",
                        filename, elm->name);
            return;
        }

        group = group_new (name);
        file_list_insert_row (list, ITEM (group), &iter, parent, -1);
        item_unref (ITEM (group));

        for (child = elm->children; child != nullptr; child = child->next)
            if (MOO_MARKUP_IS_ELEMENT (child))
                parse_node (list, child, &iter, filename, ui_config);
    }
    else if (strcmp (elm->name, ELM_FILE) == 0)
    {
        const char *uri;
        Item *item;
        GtkTreeIter iter;

        if (!(uri = moo_markup_get_prop (elm, PROP_URI)) || !uri[0])
        {
            g_critical ("in file %s, element %s: uri missing",
                        filename, elm->name);
            return;
        }

        item = file_new_uri (uri);
        file_list_insert_row (list, item, &iter, parent, -1);
        item_unref (item);
    }
    else if (strcmp (elm->name, ELM_UI) == 0)
    {
        const char *expanded_rows;
        const char *selected_row;
        UIConfig *config;
        char **rows = nullptr, **p;

        if (*ui_config)
        {
            g_critical ("in file %s, duplicated element %s",
                        filename, elm->name);
            return;
        }

        *ui_config = config = ui_config_new ();

        expanded_rows = moo_markup_get_prop (elm, PROP_EXPANDED_ROWS);
        selected_row = moo_markup_get_prop (elm, PROP_SELECTED_ROW);

        if (expanded_rows)
            rows = g_strsplit (expanded_rows, ";", 0);

        for (p = rows; p && *p; ++p)
        {
            GtkTreePath *path = gtk_tree_path_new_from_string (*p);
            if (path)
                config->expanded_rows = g_slist_prepend (config->expanded_rows, path);
        }

        config->expanded_rows = g_slist_reverse (config->expanded_rows);

        if (selected_row)
            config->selected_row = gtk_tree_path_new_from_string (selected_row);

        g_strfreev (rows);
    }
    else
    {
        g_critical ("in file %s: unexpected element '%s'",
                    filename, elm->name);
    }
}

static void
file_list_load_config (FileList    *list,
                       const char  *filename,
                       UIConfig   **ui_configp)
{
    MooMarkupDoc *doc;
    GError *error = nullptr;
    MooMarkupNode *root, *node;
    const char *version;

    *ui_configp = nullptr;

    if (!g_file_test (filename, G_FILE_TEST_EXISTS))
        return;

    doc = moo_markup_parse_file (filename, &error);

    if (!doc)
    {
        g_critical ("%s: could not open file %s: %s",
                    G_STRFUNC, filename,
                    moo_error_message (error));
        g_error_free (error);
        return;
    }

    if (!(root = moo_markup_get_root_element (doc, ELM_CONFIG)))
    {
        g_critical ("%s: in file %s: missing element '%s'",
                    G_STRFUNC, filename, ELM_CONFIG);
        goto out;
    }

    if (!(version = moo_markup_get_prop (root, PROP_VERSION)) ||
        strcmp (version, VALUE_VERSION) != 0)
    {
        g_critical ("%s: in file %s: invalid version '%s'",
                    G_STRFUNC, filename, VALUE_VERSION);
        goto out;
    }

    for (node = root->children; node != nullptr; node = node->next)
    {
        if (!MOO_MARKUP_IS_ELEMENT (node))
            continue;

        parse_node (list, node, nullptr, filename, ui_configp);
    }

out:
    moo_markup_doc_unref (doc);
}

static void
format_item (FileList    *list,
             GtkTreeIter *iter,
             GString     *buffer,
             guint        indent)
{
    Item *item;
    char *indent_s;

    item = get_item_at_iter (list, iter);
    indent_s = g_strnfill (indent, ' ');

    if (ITEM_IS_FILE (item))
    {
        char *uri_escaped = g_markup_escape_text (FILE_ITEM (item)->uri, -1);
        if (uri_escaped)
            g_string_append_printf (buffer, "%s<%s %s=\"%s\"/>\n",
                                    indent_s, ELM_FILE, PROP_URI, uri_escaped);
        g_free (uri_escaped);
    }
    else if (ITEM_IS_GROUP (item))
    {
        char *name_escaped = g_markup_escape_text (GROUP_ITEM (item)->name, -1);

        if (name_escaped)
        {
            GtkTreeIter child;

            if (gtk_tree_model_iter_children (GTK_TREE_MODEL (list), &child, iter))
            {
                g_string_append_printf (buffer, "%s<%s %s=\"%s\">\n",
                                        indent_s, ELM_GROUP, PROP_NAME, name_escaped);

                do
                {
                    format_item (list, &child, buffer, indent + 2);
                }
                while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list), &child));

                g_string_append_printf (buffer, "%s</%s>\n", indent_s, ELM_GROUP);
            }
            else
            {
                g_string_append_printf (buffer, "%s<%s %s=\"%s\"/>\n",
                                        indent_s, ELM_GROUP, PROP_NAME, name_escaped);
            }
        }

        g_free (name_escaped);
    }
    else
    {
        g_critical ("oops");
    }

    g_free (indent_s);
}

static void
file_list_save_config (FileList   *list,
                       const char *filename,
                       UIConfig   *ui_config)
{
    GtkTreeIter iter;
    GString *buffer;
    GError *error = nullptr;

    if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list), &iter))
    {
        mgw_errno_t err;
        mgw_unlink (filename, &err);
        return;
    }

    buffer = g_string_new ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    g_string_append (buffer, "<" ELM_CONFIG " " PROP_VERSION "=\"" VALUE_VERSION "\">\n");

    if (ui_config && (ui_config->expanded_rows || ui_config->selected_row))
    {
        g_string_append (buffer, "  <" ELM_UI);

        if (ui_config->expanded_rows)
        {
            GSList *l;

            g_string_append (buffer, " " PROP_EXPANDED_ROWS "=\"");

            for (l = ui_config->expanded_rows; l != nullptr; l = l->next)
            {
                GtkTreePath *path = (GtkTreePath*) l->data;
                char *tmp = gtk_tree_path_to_string (path);
                if (l != ui_config->expanded_rows)
                    g_string_append (buffer, ";");
                if (tmp)
                    g_string_append (buffer, tmp);
                g_free (tmp);
            }

            g_string_append (buffer, "\"");
        }

        if (ui_config->selected_row)
        {
            char *tmp = gtk_tree_path_to_string (ui_config->selected_row);

            if (tmp)
                g_string_append_printf (buffer, " " PROP_SELECTED_ROW "=\"%s\"", tmp);

            g_free (tmp);
        }

        g_string_append (buffer, "/>\n");
    }

    do
    {
        if (file_list_iter_is_auto (list, &iter))
            break;

        format_item (list, &iter, buffer, 2);
    }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list), &iter));

    g_string_append (buffer, "</" ELM_CONFIG ">\n");

    if (!moo_save_config_file (filename, buffer->str, buffer->len, &error))
    {
        g_critical ("could not save file %s: %s",
                    filename, moo_error_message (error));
        g_error_free (error);
    }

    g_string_free (buffer, TRUE);
}

static GtkTreePath *
file_list_add_group (FileList    *list,
                     GtkTreePath *path)
{
    GtkTreePath *parent = nullptr;
    GtkTreeIter parent_iter, new_iter;
    GtkTreeIter *piter = nullptr;
    Group *group;
    int index = -1;

    if (path)
    {
        GtkTreeIter iter;
        Item *item;

        gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &iter, path);
        item = get_item_at_iter (list, &iter);

        if (ITEM_IS_GROUP (item))
        {
            parent = gtk_tree_path_copy (path);
            index = 0;
        }
        else if (!file_list_iter_is_auto (list, &iter))
        {
            GtkTreeIter parent_iter;

            if (gtk_tree_model_iter_parent (GTK_TREE_MODEL (list), &parent_iter, &iter))
            {
                parent = gtk_tree_path_copy (path);
                gtk_tree_path_up (parent);
                index = gtk_tree_path_get_indices (path)[gtk_tree_path_get_depth (path)-1] + 1;
            }
            else
            {
                index = gtk_tree_path_get_indices (path)[0] + 1;
            }
        }
    }

    if (parent)
    {
        gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &parent_iter, parent);
        piter = &parent_iter;
    }

    if (index < 0)
        index = list->n_user_items;

    group = group_new ("Group");
    file_list_insert_row (list, ITEM (group), &new_iter, piter, index);
    item_unref (ITEM (group));

    return gtk_tree_model_get_path (GTK_TREE_MODEL (list), &new_iter);
}

static void
file_list_try_remove_item (FileList    *list,
                           GtkTreePath *path)
{
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &iter, path) &&
        !file_list_iter_is_auto (list, &iter))
    {
        file_list_remove_row (list, &iter);
        window_plugin_queue_update (list->plugin);
    }
}

static void
file_list_remove_items (FileList *list,
                        GList    *paths)
{
    GSList *rows = nullptr;

    while (paths)
    {
        GtkTreeRowReference *row;

        row = gtk_tree_row_reference_new (GTK_TREE_MODEL (list), (GtkTreePath*) paths->data);

        if (row)
            rows = g_slist_prepend (rows, row);

        paths = paths->next;
    }

    rows = g_slist_reverse (rows);

    while (rows)
    {
        GtkTreePath *path = nullptr;

        if (gtk_tree_row_reference_valid ((GtkTreeRowReference*) rows->data))
            path = gtk_tree_row_reference_get_path ((GtkTreeRowReference*) rows->data);

        if (path)
        {
            file_list_try_remove_item (list, path);
            gtk_tree_path_free (path);
        }

        gtk_tree_row_reference_free ((GtkTreeRowReference*) rows->data);
        rows = g_slist_delete_link (rows, rows);
    }
}

static gboolean
drag_source_row_draggable (G_GNUC_UNUSED GtkTreeDragSource *drag_source,
                           G_GNUC_UNUSED GtkTreePath       *path)
{
    return TRUE;
}

static gboolean
drag_source_drag_data_get (GtkTreeDragSource *drag_source,
                           GtkTreePath       *path,
                           GtkSelectionData  *selection_data)
{
    if (selection_data->target == TREE_MODEL_ROW_ATOM)
    {
        gtk_tree_set_row_drag_data (selection_data, GTK_TREE_MODEL (drag_source), path);
        return TRUE;
    }
    else if (selection_data->target == moo_atom_uri_list ())
    {
        Item *item = get_item_at_path (FILE_LIST (drag_source), path);

        gstr uri;
        const char *uris[2] = { nullptr, nullptr };

        if (ITEM_IS_FILE (item))
        {
            uri = file_get_uri (FILE_ITEM (item));
            uris[0] = !uri.empty() ? uri.get() : nullptr;
        }

        if (uris[0])
        {
            gtk_selection_data_set_uris (selection_data, (char**) uris);
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean
drag_source_drag_data_delete (G_GNUC_UNUSED GtkTreeDragSource *drag_source,
                              G_GNUC_UNUSED GtkTreePath       *path)
{
    return FALSE;
}

static void
copy_row_children (FileList    *list,
                   GtkTreeIter *source,
                   GtkTreeIter *dest)
{
    GtkTreeIter child;

    if (gtk_tree_model_iter_children (GTK_TREE_MODEL (list), &child, source))
    do
    {
        GtkTreeIter iter;
        Item *item = get_item_at_iter (list, &child);
        gtk_tree_store_append (GTK_TREE_STORE (list), &iter, dest);
        gtk_tree_store_set (GTK_TREE_STORE (list), &iter,
                            COLUMN_ITEM, item,
                            COLUMN_TOOLTIP, item_get_tooltip (item),
                            -1);
        copy_row_children (list, &child, &iter);
    }
    while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list), &child));
}

static void
copy_row (FileList    *list,
          GtkTreePath *source,
          GtkTreePath *parent,
          int          index)
{
    GtkTreeRowReference *source_row;
    GtkTreeIter iter, parent_iter;
    GtkTreeIter *piter = nullptr;
    Item *item;

    gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &iter, source);
    item = get_item_at_iter (list, &iter);
    g_return_if_fail (item != nullptr);

    source_row = gtk_tree_row_reference_new (GTK_TREE_MODEL (list), source);

    if (parent)
    {
        gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &parent_iter, parent);
        piter = &parent_iter;
    }

    file_list_insert_row (list, item, &iter, piter, index);

    parent_iter = iter;
    source = gtk_tree_row_reference_get_path (source_row);
    gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &iter, source);
    copy_row_children (list, &iter, &parent_iter);
    gtk_tree_path_free (source);
}

static gboolean
move_row (FileList    *list,
          GtkTreePath *source,
          GtkTreePath *parent,
          int          index)
{
    GtkTreePath *source_parent = nullptr;
    gboolean same_parent = FALSE;
    GtkTreeIter iter;
    Item *item;

    gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &iter, source);

    item = get_item_at_iter (list, &iter);

    if (ITEM_IS_FILE (item) && FILE_ITEM (item)->doc)
    {
        gstr uri = file_get_uri (FILE_ITEM (item));

        if (uri.empty())
            return FALSE;

        if (!FILE_ITEM (item)->uri)
            file_set_uri (FILE_ITEM (item), uri.get());

        file_list_row_data.set(FILE_ITEM (item)->doc, nullptr);
        file_set_doc (FILE_ITEM (item), nullptr);
    }

    if (gtk_tree_path_get_depth (source) > 1)
    {
        source_parent = gtk_tree_path_copy (source);
        gtk_tree_path_up (source_parent);
    }

    if (!source_parent && !parent)
        same_parent = TRUE;
    else if (source_parent && parent && gtk_tree_path_compare (source_parent, parent) == 0)
        same_parent = TRUE;

    if (same_parent)
    {
        GtkTreeIter *piter = nullptr;
        gboolean first_user = FALSE;

        if (parent)
        {
            gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &iter, parent);
            piter = &iter;
        }
        else if (file_list_iter_is_auto (list, &iter))
        {
            list->n_user_items += 1;
            first_user = list->n_user_items == 1;
        }

        if (index >= gtk_tree_model_iter_n_children (GTK_TREE_MODEL (list), piter) || index < 0)
        {
            gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &iter, source);
            gtk_tree_store_move_before (GTK_TREE_STORE (list), &iter, nullptr);
        }
        else
        {
            GtkTreeIter ch_iter;
            gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (list), &ch_iter, piter, index);
            gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &iter, source);
            gtk_tree_store_move_before (GTK_TREE_STORE (list), &iter, &ch_iter);
        }

        if (first_user)
            gtk_tree_store_insert (GTK_TREE_STORE (list), &iter,
                                   nullptr, list->n_user_items);
    }
    else
    {
        GtkTreeRowReference *row;

        row = gtk_tree_row_reference_new (GTK_TREE_MODEL (list), source);

        copy_row (list, source, parent, index);

        source = gtk_tree_row_reference_get_path (row);
        gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &iter, source);

        file_list_remove_row (list, &iter);

        gtk_tree_path_free (source);
        gtk_tree_row_reference_free (row);
    }

    if (source_parent)
        gtk_tree_path_free (source_parent);

    check_list (list);

    return TRUE;
}

static gboolean
uri_is_directory (const char *uri)
{
    char *filename;
    gboolean retval = FALSE;

    filename = g_filename_from_uri (uri, nullptr, nullptr);

    if (filename)
        retval = g_file_test (filename, G_FILE_TEST_IS_DIR);

    g_free (filename);
    return retval;
}

static void
add_row_from_dir_uri (G_GNUC_UNUSED FileList    *list,
                      G_GNUC_UNUSED const char  *uri,
                      G_GNUC_UNUSED GtkTreeIter *iter,
                      G_GNUC_UNUSED GtkTreeIter *parent,
                      G_GNUC_UNUSED int          index)
{
#if 0
    /* TODO read files */
    Group *grp;
    char *basename;

    return;

    basename = uri_get_basename (uri);
    grp = group_new (basename);

    file_list_insert_row (list, ITEM (grp), iter, parent, index);

    item_unref (ITEM (grp));
    g_free (basename);
#endif
}

static void
add_row_from_file_uri (FileList    *list,
                       const char  *uri,
                       GtkTreeIter *iter,
                       GtkTreeIter *parent,
                       int          index)
{
    Item *item = file_new_uri (uri);
    file_list_insert_row (list, item, iter, parent, index);
    item_unref (item);
}

static gboolean
add_row_from_uri (FileList    *list,
                  const char  *uri,
                  GtkTreePath *parent,
                  int          index)
{
    GtkTreeIter iter, dummy;
    GtkTreeIter *piter = nullptr;

    g_return_val_if_fail (uri != nullptr, FALSE);

    if (parent)
    {
        gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &iter, parent);
        piter = &iter;
    }

    if (uri_is_directory (uri))
        add_row_from_dir_uri (list, uri, &dummy, piter, index);
    else
        add_row_from_file_uri (list, uri, &dummy, piter, index);

    return TRUE;
}

static gboolean
find_drop_destination (FileList     *list,
                       GtkTreePath  *dest,
                       GtkTreePath **parent_path,
                       int          *index)
{
    int n_children;
    Group *parent_group = nullptr;
    GtkTreeIter parent_iter;

    *parent_path = nullptr;
    *index = 0;

    if (gtk_tree_path_get_depth (dest) > 1)
    {
        GtkTreePath *parent;
        Item *parent_item;

        parent = gtk_tree_path_copy (dest);
        gtk_tree_path_up (parent);

        if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &parent_iter, parent))
        {
            gtk_tree_path_free (parent);
            return FALSE;
        }

        parent_item = get_item_at_iter (list, &parent_iter);

        if (ITEM_IS_GROUP (parent_item))
        {
            parent_group = GROUP_ITEM (parent_item);
            *index = gtk_tree_path_get_indices (dest)[gtk_tree_path_get_depth (dest) - 1];
        }
        else if (gtk_tree_path_get_depth (parent) > 1)
        {
            *index = gtk_tree_path_get_indices (parent)[gtk_tree_path_get_depth (parent) - 1];
            gtk_tree_path_up (parent);

            if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &parent_iter, parent))
            {
                gtk_tree_path_free (parent);
                g_return_val_if_reached (FALSE);
            }

            parent_item = get_item_at_iter (list, &parent_iter);

            if (!ITEM_IS_GROUP (parent_item))
            {
                gtk_tree_path_free (parent);
                g_return_val_if_reached (FALSE);
            }

            parent_group = GROUP_ITEM (parent_item);
        }
        else
        {
            parent_group = nullptr;
            *index = gtk_tree_path_get_indices (parent)[0];
        }

        gtk_tree_path_free (parent);
    }
    else
    {
        *index = gtk_tree_path_get_indices (dest)[0];
    }

    if (parent_group)
    {
        n_children = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (list), &parent_iter);
        *parent_path = gtk_tree_model_get_path (GTK_TREE_MODEL (list), &parent_iter);
    }
    else
    {
        n_children = list->n_user_items;
    }

    *index = CLAMP (*index, 0, n_children);

    return TRUE;
}

static gboolean
path_is_descendant (GtkTreePath *path,
                    GtkTreePath *ancestor)
{
    return gtk_tree_path_compare (path, ancestor) == 0 ||
            gtk_tree_path_is_descendant (path, ancestor);
}

static gboolean
drop_uris (FileList     *list,
           GtkTreePath  *dest,
           char        **uris)
{
    GtkTreePath *parent_path = nullptr;
    int index = 0;

    if (!find_drop_destination (list, dest, &parent_path, &index))
        return FALSE;

    for ( ; uris && *uris; ++uris)
    {
        GtkTreeIter iter;

        if (!file_list_find_uri (list, *uris, &iter) &&
            add_row_from_uri (list, *uris, parent_path, index))
        {
            index += 1;
        }
    }

    if (parent_path)
        gtk_tree_path_free (parent_path);

    return TRUE;
}

static gboolean
drop_tree_model_row (FileList    *list,
                     GtkTreePath *dest,
                     GtkTreePath *source)
{
    GtkTreePath *parent_path = nullptr;
    int index;
    gboolean retval;

    if (!get_item_at_path (list, source))
        return FALSE;

    if (!find_drop_destination (list, dest, &parent_path, &index))
        return FALSE;

    if (parent_path && path_is_descendant (parent_path, source))
    {
        gtk_tree_path_free (parent_path);
        return FALSE;
    }

    retval = move_row (list, source, parent_path, index);

    if (parent_path)
        gtk_tree_path_free (parent_path);

    return retval;
}

static int
cmp_uris (const void *p1,
          const void *p2)
{
    const char *const *s1 = (const char *const *) p1;
    const char *const *s2 = (const char *const *) p2;
    return strcmp (*s1, *s2);
}

static gboolean
drag_dest_drag_data_received (GtkTreeDragDest  *drag_dest,
                              GtkTreePath      *dest,
                              GtkSelectionData *selection_data)
{
    if (selection_data->target == TREE_MODEL_ROW_ATOM)
    {
        GtkTreePath *path = nullptr;
        gboolean retval;

        if (!gtk_tree_get_row_drag_data (selection_data, nullptr, &path))
            return FALSE;

        if ((retval = drop_tree_model_row (FILE_LIST (drag_dest), dest, path)))
            window_plugin_queue_update (FILE_LIST (drag_dest)->plugin);

        gtk_tree_path_free (path);
        return retval;
    }
    else if (selection_data->target == moo_atom_uri_list ())
    {
        char **uris;
        gboolean retval = FALSE;

        if (!(uris = gtk_selection_data_get_uris (selection_data)))
            return FALSE;

        if (uris[0])
        {
            qsort (uris, g_strv_length (uris), sizeof (char*), cmp_uris);

            if ((retval = drop_uris (FILE_LIST (drag_dest), dest, uris)))
                window_plugin_queue_update (FILE_LIST (drag_dest)->plugin);
        }

        g_strfreev (uris);
        return retval;
    }

    return FALSE;
}

static gboolean
drag_dest_row_drop_possible (G_GNUC_UNUSED GtkTreeDragDest  *drag_dest,
                             G_GNUC_UNUSED GtkTreePath      *dest_path,
                             G_GNUC_UNUSED GtkSelectionData *selection_data)
{
    return TRUE;
}


gboolean
_moo_str_semicase_compare (const char *string,
                           const char *key)
{
    gboolean has_upper;
    const char *p;

    g_return_val_if_fail (string != nullptr, FALSE);
    g_return_val_if_fail (key != nullptr, FALSE);

    for (p = key, has_upper = FALSE; *p && !has_upper; ++p)
        has_upper = g_ascii_isupper (*p);

    if (has_upper)
        return strncmp (string, key, strlen (key)) == 0;
    else
        return g_ascii_strncasecmp (string, key, strlen (key)) == 0;
}


static gboolean
row_separator_func (GtkTreeModel *model,
                    GtkTreeIter  *iter)
{
    Item *item = get_item_at_iter (FILE_LIST (model), iter);
    return item == nullptr;
}

static gboolean
tree_view_search_equal_func (GtkTreeModel *model,
                             G_GNUC_UNUSED int column,
                             const char   *key,
                             GtkTreeIter  *iter)
{
    const char *compare_with = nullptr;
    Item *item;

    item = get_item_at_iter (FILE_LIST (model), iter);

    if (ITEM_IS_FILE (item))
        compare_with = FILE_ITEM (item)->display_basename;
    else if (ITEM_IS_GROUP (item))
        compare_with = GROUP_ITEM (item)->name;

    if (compare_with)
        return !_moo_str_semicase_compare (compare_with, key);
    else
        return TRUE;
}

static void
pixbuf_data_func (G_GNUC_UNUSED GtkTreeViewColumn *column,
                  GtkCellRenderer   *cell,
                  GtkTreeModel      *model,
                  GtkTreeIter       *iter)
{
    Item *item = get_item_at_iter (FILE_LIST (model), iter);
    if (ITEM_IS_GROUP (item))
        g_object_set (cell, "stock-id", GTK_STOCK_DIRECTORY, nullptr);
    else if (ITEM_IS_FILE (item))
        g_object_set (cell, "stock-id", GTK_STOCK_FILE, nullptr);
}

static void
text_data_func (G_GNUC_UNUSED GtkTreeViewColumn *column,
                GtkCellRenderer   *cell,
                GtkTreeModel      *model,
                GtkTreeIter       *iter)
{
    Item *item = get_item_at_iter (FILE_LIST (model), iter);

    if (ITEM_IS_GROUP (item))
        g_object_set (cell, "text", GROUP_ITEM (item)->name, nullptr);
    else if (ITEM_IS_FILE (item))
        g_object_set (cell, "text", FILE_ITEM (item)->display_basename, nullptr);
}

static void
text_cell_edited (GtkCellRenderer *cell,
                  const char      *path_string,
                  const char      *new_text,
                  WindowPlugin    *plugin)
{
    GtkTreeIter iter;
    Item *item;
    GtkTreePath *path;

    g_object_set (cell, "editable", FALSE, nullptr);

    path = gtk_tree_path_new_from_string (path_string);
    g_return_if_fail (path != nullptr);

    gtk_tree_model_get_iter (GTK_TREE_MODEL (plugin->list), &iter, path);
    item = get_item_at_iter (plugin->list, &iter);
    g_return_if_fail (ITEM_IS_GROUP (item));

    MOO_ASSIGN_STRING (GROUP_ITEM (item)->name, new_text);
    gtk_tree_model_row_changed (GTK_TREE_MODEL (plugin->list), path, &iter);

    gtk_tree_path_free (path);
}

static void
text_cell_editing_canceled (GtkCellRenderer *cell)
{
    g_object_set (cell, "editable", FALSE, nullptr);
}

static void
start_edit (WindowPlugin *plugin,
            GtkTreePath  *path)
{
    g_object_set (plugin->text_cell, "editable", TRUE, nullptr);
    gtk_tree_view_set_cursor_on_cell (plugin->treeview,
                                      path, plugin->column,
                                      plugin->text_cell,
                                      TRUE);
}

static GList *
get_selected_rows (WindowPlugin *plugin)
{
    GtkTreeSelection *selection;
    selection = gtk_tree_view_get_selection (plugin->treeview);
    return gtk_tree_selection_get_selected_rows (selection, nullptr);
}

static void
path_list_free (GList *paths)
{
    g_list_foreach (paths, (GFunc) gtk_tree_path_free, nullptr);
    g_list_free (paths);
}

static void
rename_activated (G_GNUC_UNUSED GtkWidget *menuitem,
                  WindowPlugin *plugin)
{
    GList *selected;
    Item *item;

    selected = get_selected_rows (plugin);
    if (!selected || selected->next)
    {
        path_list_free (selected);
        g_return_if_fail (selected && !selected->next);
    }

    item = get_item_at_path (plugin->list, (GtkTreePath*) selected->data);
    if (!ITEM_IS_GROUP (item))
    {
        path_list_free (selected);
        g_return_if_fail (ITEM_IS_GROUP (item));
    }

    start_edit (plugin, (GtkTreePath*) selected->data);

    path_list_free (selected);
}

static void
add_group_activated (G_GNUC_UNUSED GtkWidget *menuitem,
                     WindowPlugin *plugin)
{
    GtkTreePath *new_path, *path;
    GList *selected;

    selected = get_selected_rows (plugin);
    if (selected)
        path = (GtkTreePath*) g_list_last (selected)->data;
    else
        path = nullptr;

    new_path = file_list_add_group (plugin->list, path);

    if (new_path)
    {
        GtkTreeIter iter, parent;

        gtk_tree_model_get_iter (GTK_TREE_MODEL (plugin->list), &iter, new_path);

        if (gtk_tree_model_iter_parent (GTK_TREE_MODEL (plugin->list), &parent, &iter))
        {
            GtkTreePath *parent_path = gtk_tree_model_get_path (GTK_TREE_MODEL (plugin->list),
                                                                &parent);
            if (!gtk_tree_view_row_expanded (plugin->treeview, parent_path))
                gtk_tree_view_expand_row (plugin->treeview, parent_path, FALSE);
            gtk_tree_path_free (parent_path);
        }

        start_edit (plugin, new_path);
        gtk_tree_path_free (new_path);
    }

    path_list_free (selected);
}

static void
remove_activated (G_GNUC_UNUSED GtkWidget *menuitem,
                  WindowPlugin *plugin)
{
    GList *selected;
    selected = get_selected_rows (plugin);
    file_list_remove_items (plugin->list, selected);
    window_plugin_queue_update (plugin);
    path_list_free (selected);
}

static void
open_file (WindowPlugin *plugin,
           GtkTreePath  *path)
{
    Item *item;

    item = get_item_at_path (plugin->list, path);
    g_return_if_fail (item != nullptr);

    if (ITEM_IS_FILE (item))
    {
        if (FILE_ITEM (item)->doc)
        {
            moo_editor_set_active_doc (moo_editor_instance (),
                                       FILE_ITEM (item)->doc);
            gtk_widget_grab_focus (GTK_WIDGET (FILE_ITEM (item)->doc));
        }
        else
        {
            moo_editor_open_uri (moo_editor_instance (),
                                 FILE_ITEM (item)->uri, nullptr, -1,
                                 MOO_WIN_PLUGIN (plugin)->window);
        }
    }
    else if (ITEM_IS_GROUP (item))
    {
        GtkTreeIter iter, child;

        gtk_tree_model_get_iter (GTK_TREE_MODEL (plugin->list), &iter, path);

        if (gtk_tree_model_iter_children (GTK_TREE_MODEL (plugin->list), &child, &iter))
        {
            do
            {
                GtkTreePath *child_path;
                child_path = gtk_tree_model_get_path (GTK_TREE_MODEL (plugin->list), &child);
                open_file (plugin, child_path);
                gtk_tree_path_free (child_path);
            }
            while (gtk_tree_model_iter_next (GTK_TREE_MODEL (plugin->list), &child));
        }
    }
    else
    {
        g_return_if_reached ();
    }
}

static void
open_activated (G_GNUC_UNUSED GtkWidget *menuitem,
                WindowPlugin *plugin)
{
    GList *selected, *l;

    selected = get_selected_rows (plugin);

    for (l = selected; l != nullptr; l = l->next)
        open_file (plugin, (GtkTreePath*) l->data);

    path_list_free (selected);
}

static gboolean
can_open (G_GNUC_UNUSED FileList *list,
          GList *paths)
{
    /* XXX */
    return paths != nullptr;
}

static gboolean
can_remove (FileList *list,
            GList    *paths)
{
    while (paths)
    {
        GtkTreeIter iter;
        GtkTreePath *path = (GtkTreePath*) paths->data;
        gtk_tree_model_get_iter (GTK_TREE_MODEL (list), &iter, path);
        if (!file_list_iter_is_auto (list, &iter))
            return TRUE;
        paths = paths->next;
    }

    return FALSE;
}

static void
popup_menu (WindowPlugin *plugin,
            GList        *selected,
            int           button,
            guint32       time)
{
    GtkWidget *menu, *menuitem;
    GtkTreePath *single_path;
    Item *single_item;

    single_path = (selected && !selected->next) ? (GtkTreePath*) selected->data : nullptr;
    single_item = single_path ? get_item_at_path (plugin->list, single_path) : nullptr;

    menu = gtk_menu_new ();

    if (can_open (plugin->list, selected))
    {
        menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_OPEN, nullptr);
        g_signal_connect (menuitem, "activate", G_CALLBACK (open_activated), plugin);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    }

    menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_ADD, nullptr);
    gtk_label_set_text (GTK_LABEL (GTK_BIN (menuitem)->child), "Add Group");
    g_signal_connect (menuitem, "activate", G_CALLBACK (add_group_activated), plugin);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

    if (selected)
    {
        menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_REMOVE, nullptr);
        g_signal_connect (menuitem, "activate", G_CALLBACK (remove_activated), plugin);
        gtk_widget_set_sensitive (menuitem, can_remove (plugin->list, selected));
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    }

    if (single_item && ITEM_IS_GROUP (single_item))
    {
        menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_EDIT, nullptr);
        gtk_label_set_text (GTK_LABEL (GTK_BIN (menuitem)->child), "Rename");
        g_signal_connect (menuitem, "activate", G_CALLBACK (rename_activated), plugin);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    }

    gtk_widget_show_all (menu);
    gtk_menu_popup (GTK_MENU (menu), nullptr, nullptr, nullptr, nullptr, button, time);
}

static gboolean
treeview_button_press (GtkTreeView    *treeview,
                       GdkEventButton *event,
                       WindowPlugin   *plugin)
{
    GtkTreeSelection *selection;
    GtkTreePath *path = nullptr;
    GList *selected;
    int x, y;

    if (event->type != GDK_BUTTON_PRESS || event->button != 3)
        return FALSE;

    gtk_tree_view_get_path_at_pos (treeview, (int) event->x, (int) event->y,
                                   &path, nullptr, &x, &y);

    selection = gtk_tree_view_get_selection (treeview);

    if (!path)
        gtk_tree_selection_unselect_all (selection);
    else if (!gtk_tree_selection_path_is_selected (selection, path))
        gtk_tree_view_set_cursor (treeview, path, plugin->column, FALSE);

    selected = gtk_tree_selection_get_selected_rows (selection, nullptr);
    popup_menu (plugin, selected, event->button, event->time);

    if (path)
        gtk_tree_path_free (path);

    g_list_foreach (selected, (GFunc) gtk_tree_path_free, nullptr);
    g_list_free (selected);

    return TRUE;
}

static void
treeview_row_activated (WindowPlugin *plugin,
                        GtkTreePath  *path)
{
    Item *item;

    item = get_item_at_path (plugin->list, path);
    g_return_if_fail (item != nullptr);

    if (ITEM_IS_FILE (item))
        open_file (plugin, path);
}

static void
create_treeview (WindowPlugin *plugin)
{
    GtkTreeSelection *selection;
    GtkCellRenderer *cell;
    GtkTargetEntry targets[] = {
        { (char*) "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, 0 },
        { (char*) "text/uri-list", 0, 0 }
    };

    plugin->treeview = GTK_TREE_VIEW (gtk_tree_view_new ());
    gtk_tree_view_set_headers_visible (plugin->treeview, FALSE);
    gtk_tree_view_set_row_separator_func (plugin->treeview,
                                          (GtkTreeViewRowSeparatorFunc) row_separator_func,
                                          nullptr, nullptr);
    gtk_tree_view_set_search_equal_func (plugin->treeview,
                                         (GtkTreeViewSearchEqualFunc) tree_view_search_equal_func,
                                         nullptr, nullptr);
    gtk_tree_view_set_tooltip_column (plugin->treeview, COLUMN_TOOLTIP);

    selection = gtk_tree_view_get_selection (plugin->treeview);
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

    g_signal_connect (plugin->treeview, "button-press-event",
                      G_CALLBACK (treeview_button_press), plugin);
    g_signal_connect_swapped (plugin->treeview, "row-activated",
                              G_CALLBACK (treeview_row_activated), plugin);
    g_signal_connect_swapped (plugin->treeview, "row-expanded",
                              G_CALLBACK (window_plugin_queue_update_ui), plugin);
    g_signal_connect_swapped (plugin->treeview, "row-collapsed",
                              G_CALLBACK (window_plugin_queue_update_ui), plugin);

    gtk_tree_view_enable_model_drag_dest (plugin->treeview,
                                          targets, G_N_ELEMENTS (targets),
                                          GDK_ACTION_COPY | GDK_ACTION_MOVE |
                                            GDK_ACTION_LINK | GDK_ACTION_PRIVATE);
    gtk_tree_view_enable_model_drag_source (plugin->treeview,
                                            GDK_BUTTON1_MASK,
                                            targets, G_N_ELEMENTS (targets),
                                            GDK_ACTION_COPY | GDK_ACTION_MOVE |
                                                GDK_ACTION_LINK | GDK_ACTION_PRIVATE);

    plugin->column = gtk_tree_view_column_new ();
    gtk_tree_view_append_column (plugin->treeview, plugin->column);

    _moo_tree_view_setup_expander (plugin->treeview, plugin->column);

    cell = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (plugin->column, cell, FALSE);
    gtk_tree_view_column_set_cell_data_func (plugin->column, cell,
                                             (GtkTreeCellDataFunc) pixbuf_data_func,
                                             nullptr, nullptr);

    plugin->text_cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (plugin->column, plugin->text_cell, TRUE);
    gtk_tree_view_column_set_cell_data_func (plugin->column, plugin->text_cell,
                                             (GtkTreeCellDataFunc) text_data_func,
                                             nullptr, nullptr);
    g_signal_connect (plugin->text_cell, "edited",
                      G_CALLBACK (text_cell_edited), plugin);
    g_signal_connect (plugin->text_cell, "editing-canceled",
                      G_CALLBACK (text_cell_editing_canceled), plugin);
}

static void
create_model (WindowPlugin *plugin)
{
    plugin->list = (FileList*) g_object_new (file_list_get_type (), (const char*) nullptr);
    plugin->list->plugin = plugin;

    file_list_load_config (plugin->list, plugin->filename, &plugin->ui_config);

    gtk_tree_view_set_model (plugin->treeview, GTK_TREE_MODEL (plugin->list));
}

static int
compare_docs_by_name (MooEdit *doc1,
                      MooEdit *doc2)
{
    const char *name1, *name2;
    char *key1, *key2;
    int retval;

    name1 = moo_edit_get_display_basename (doc1);
    name2 = moo_edit_get_display_basename (doc2);
    key1 = g_utf8_collate_key_for_filename (name1, -1);
    key2 = g_utf8_collate_key_for_filename (name2, -1);

    retval = strcmp (key1, key2);

    g_free (key2);
    g_free (key1);
    return retval;
}

static gboolean
do_update (WindowPlugin *plugin)
{
    MooEditArray *docs;
    GSList *list;
    guint i;

    plugin->update_idle = 0;

    docs = moo_edit_window_get_docs (MOO_WIN_PLUGIN (plugin)->window);

    for (i = 0, list = nullptr; i < docs->n_elms; ++i)
        list = g_slist_prepend (list, docs->elms[i]);

    list = g_slist_sort (list, (GCompareFunc) compare_docs_by_name);

    file_list_update (plugin->list, list);

    g_slist_free (list);
    moo_edit_array_free (docs);
    return FALSE;
}

static void
window_plugin_queue_update (WindowPlugin *plugin)
{
    g_return_if_fail (plugin != nullptr);

    if (!plugin->update_idle)
        plugin->update_idle = g_idle_add ((GSourceFunc) do_update,
                                          plugin);

    window_plugin_queue_update_ui (plugin);
}


static void
load_ui_config (WindowPlugin *plugin)
{
    if (plugin->ui_config)
    {
        UIConfig *cfg = plugin->ui_config;

        plugin->ui_config = nullptr;

        while (cfg->expanded_rows)
        {
            GtkTreePath *path = (GtkTreePath*) cfg->expanded_rows->data;
            gtk_tree_view_expand_row (plugin->treeview, path, FALSE);
            gtk_tree_path_free (path);
            cfg->expanded_rows = g_slist_delete_link (cfg->expanded_rows,
                                                      cfg->expanded_rows);
        }

        if (cfg->selected_row)
            gtk_tree_view_set_cursor (plugin->treeview, cfg->selected_row, nullptr, FALSE);
        else
            _moo_tree_view_select_first (plugin->treeview);

        gtk_tree_path_free (cfg->selected_row);
        g_free (cfg);
    }
    else
    {
        _moo_tree_view_select_first (plugin->treeview);
    }
}

static gboolean
check_row_expanded (G_GNUC_UNUSED GtkTreeModel *model,
                    GtkTreePath  *path,
                    G_GNUC_UNUSED GtkTreeIter  *iter,
                    WindowPlugin *plugin)
{
    if (gtk_tree_view_row_expanded (plugin->treeview, path))
        plugin->ui_config->expanded_rows =
            g_slist_prepend (plugin->ui_config->expanded_rows,
                             gtk_tree_path_copy (path));
    return FALSE;
}

static gboolean
do_update_ui (WindowPlugin *plugin)
{
    plugin->update_ui_idle = 0;

    if (plugin->first_time_show)
    {
        plugin->first_time_show = FALSE;
        load_ui_config (plugin);
    }
    else
    {
        ui_config_free (plugin->ui_config);
        plugin->ui_config = ui_config_new ();

        gtk_tree_model_foreach (GTK_TREE_MODEL (plugin->list),
                                (GtkTreeModelForeachFunc) check_row_expanded,
                                plugin);
        plugin->ui_config->expanded_rows =
            g_slist_reverse (plugin->ui_config->expanded_rows);
    }

    return FALSE;
}

static void
window_plugin_queue_update_ui (WindowPlugin *plugin)
{
    g_return_if_fail (plugin != nullptr);
    if (!plugin->update_ui_idle)
        plugin->update_ui_idle = g_idle_add ((GSourceFunc) do_update_ui,
                                             plugin);
}


static gboolean
file_list_window_plugin_create (WindowPlugin *plugin)
{
    MooPane *pane;
    MooPaneLabel *label;
    GtkWidget *scrolled_window;
    MooEditWindow *window;

    window = MOO_WIN_PLUGIN (plugin)->window;
    g_return_val_if_fail (MOO_IS_EDIT_WINDOW (window), FALSE);

    plugin->filename = moo_get_named_user_data_file (CONFIG_FILE);

    create_treeview (plugin);
    create_model (plugin);

    scrolled_window = gtk_scrolled_window_new (nullptr, nullptr);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (plugin->treeview));
    gtk_widget_show_all (scrolled_window);

    label = moo_pane_label_new (GTK_STOCK_DIRECTORY,
                                nullptr, _("File List"),
                                _("File List"));
    moo_edit_window_add_pane (window,
                              FILE_LIST_PLUGIN_ID,
                              scrolled_window, label,
                              MOO_PANE_POS_RIGHT);
    moo_pane_label_free (label);

    pane = moo_big_paned_find_pane (window->paned,
                                    GTK_WIDGET (scrolled_window), nullptr);
    moo_pane_set_drag_dest (pane);

    plugin->first_time_show = TRUE;
    g_signal_connect_swapped (window, "new-doc",
                              G_CALLBACK (window_plugin_queue_update), plugin);
    g_signal_connect_swapped (window, "close-doc",
                              G_CALLBACK (window_plugin_queue_update), plugin);
    window_plugin_queue_update (plugin);

    return TRUE;
}

static void
file_list_window_plugin_destroy (WindowPlugin *plugin)
{
    if (plugin->update_idle)
        g_source_remove (plugin->update_idle);
    if (plugin->update_ui_idle)
        g_source_remove (plugin->update_ui_idle);

    file_list_save_config (plugin->list, plugin->filename, plugin->ui_config);
    file_list_shutdown (plugin->list);
    g_object_unref (plugin->list);

    moo_edit_window_remove_pane (MOO_WIN_PLUGIN (plugin)->window,
                                 FILE_LIST_PLUGIN_ID);

    g_signal_handlers_disconnect_by_func (MOO_WIN_PLUGIN (plugin)->window,
                                          (gpointer) window_plugin_queue_update,
                                          plugin);

    ui_config_free (plugin->ui_config);
    g_free (plugin->filename);
}

static gboolean
file_list_plugin_init (G_GNUC_UNUSED FileListPlugin *plugin)
{
    return TRUE;
}

static void
file_list_plugin_deinit (G_GNUC_UNUSED FileListPlugin *plugin)
{
}


MOO_PLUGIN_DEFINE_INFO (file_list,
                        N_("File List"), N_("List of files"),
                        "Yevgen Muntyan <emuntyan@users.sourceforge.net>",
                        MOO_VERSION)
MOO_WIN_PLUGIN_DEFINE (FileList, file_list)
MOO_PLUGIN_DEFINE (FileList, file_list,
                   nullptr, nullptr, nullptr, nullptr, nullptr,
                   file_list_window_plugin_get_type (), 0)


gboolean
_moo_file_list_plugin_init (void)
{
    MooPluginParams params = {TRUE, TRUE};
    return moo_plugin_register (FILE_LIST_PLUGIN_ID,
                                file_list_plugin_get_type (),
                                &file_list_plugin_info,
                                &params);
}
