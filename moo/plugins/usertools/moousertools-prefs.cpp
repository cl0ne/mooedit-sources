/*
 *   moousertools-prefs.c
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

#include "moousertools-prefs.h"
#include "moousertools.h"
#include "moocommand.h"
#include "moocommanddisplay.h"
#include "mooutils/mooprefspage.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mooutils-treeview.h"
#include "mooutils/mooutils.h"
#include "mooutils/moohelp.h"
#include "plugins/usertools/moousertools-gxml.h"
#ifdef MOO_ENABLE_HELP
#include "moo-help-sections.h"
#endif
#include <string.h>

enum {
    COLUMN_INFO,
    N_COLUMNS
};


const ObjectDataAccessor<MooPrefsPage, CommandXml*> command_xml_data("moo-user-tools-prefs-xml");


static MooUserToolType
page_get_type (MooPrefsPage *page)
{
    return (MooUserToolType) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (page), "moo-user-tool-type"));
}


static void
page_set_type (MooPrefsPage    *page,
               MooUserToolType  type)
{
    g_object_set_data (G_OBJECT (page), "moo-user-tool-type", GINT_TO_POINTER (type));
}


static gboolean
get_changed (MooPrefsPage *page)
{
    return g_object_get_data (G_OBJECT (page), "moo-changed") != NULL;
}


static void
set_changed (MooPrefsPage *page,
             gboolean      changed)
{
    g_object_set_data (G_OBJECT (page), "moo-changed", GINT_TO_POINTER (changed));
}


static MooCommandDisplay *
get_helper (MooPrefsPage *page)
{
    return (MooCommandDisplay*) g_object_get_data (G_OBJECT (page), "moo-tree-helper");
}


static void
populate_store (GtkListStore   *store,
                MooUserToolType type)
{
    GtkTreeIter iter;
    GSList *list;

    list = _moo_edit_parse_user_tools (type, FALSE);

    while (list)
    {
        MooUserToolInfo *info = (MooUserToolInfo*) list->data;

        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, COLUMN_INFO, info, -1);

        _moo_user_tool_info_unref (info);
        list = g_slist_delete_link (list, list);
    }
}


static gboolean
new_row (MooPrefsPage *page,
         GtkTreeModel *model,
         GtkTreePath  *path)
{
    GtkTreeIter iter;
    MooUserToolInfo *info;
    GtkTreeViewColumn *column;
    CommandXml *gxml;

    gxml = command_xml_data.get(page);

    info = _moo_user_tool_info_new ();
    info->cmd_factory = moo_command_factory_lookup ("lua");
    info->cmd_data = info->cmd_factory ? moo_command_data_new (info->cmd_factory->n_keys) : NULL;
    info->name = g_strdup (_("New Command"));
    info->options = g_strdup ("need-doc");
    info->enabled = TRUE;

    set_changed (page, TRUE);
    gtk_list_store_insert (GTK_LIST_STORE (model), &iter,
                           gtk_tree_path_get_indices(path)[0]);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_INFO, info, -1);

    column = gtk_tree_view_get_column (gxml->treeview, 0);
    gtk_tree_view_set_cursor (gxml->treeview, path, column, TRUE);

    _moo_user_tool_info_unref (info);
    return TRUE;
}

static gboolean
delete_row (MooPrefsPage *page)
{
    set_changed (page, TRUE);
    return FALSE;
}

static gboolean
move_row (MooPrefsPage *page)
{
    set_changed (page, TRUE);
    return FALSE;
}


enum {
    ROW_REQUIRES_NOTHING,
    ROW_REQUIRES_DOC,
    ROW_REQUIRES_FILE,
    ROW_REQUIRES_INVALID
};

enum {
    ROW_SAVE_NOTHING,
    ROW_SAVE_ONE,
    ROW_SAVE_ALL,
    ROW_SAVE_INVALID
};

static void
setup_options_widgets (CommandXml *gxml)
{
    GtkListStore *store;
    GtkCellRenderer *cell;

    store = gtk_list_store_new (1, G_TYPE_STRING);
    /* 'Requires' combo box entry on Tools prefs page */
    gtk_list_store_insert_with_values (store, NULL, G_MAXINT, 0, C_("Requires-combo", "Nothing"), -1);
    /* 'Requires' combo box entry on Tools prefs page */
    gtk_list_store_insert_with_values (store, NULL, G_MAXINT, 0, C_("Requires-combo", "Document"), -1);
    /* 'Requires' combo box entry on Tools prefs page */
    gtk_list_store_insert_with_values (store, NULL, G_MAXINT, 0, C_("Requires-combo", "File on disk"), -1);
    gtk_combo_box_set_model (gxml->combo_requires, GTK_TREE_MODEL (store));
    g_object_unref (store);

    gtk_cell_layout_clear (GTK_CELL_LAYOUT (gxml->combo_requires));
    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (gxml->combo_requires), cell, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (gxml->combo_requires), cell, "text", 0);

    store = gtk_list_store_new (1, G_TYPE_STRING);
    // 'Save' combo box entry on Tools prefs page
    gtk_list_store_insert_with_values (store, NULL, G_MAXINT, 0, C_("Save-combo", "Nothing"), -1);
    // 'Save' combo box entry on Tools prefs page
    gtk_list_store_insert_with_values (store, NULL, G_MAXINT, 0, C_("Save-combo", "Current document"), -1);
    // 'Save' combo box entry on Tools prefs page
    gtk_list_store_insert_with_values (store, NULL, G_MAXINT, 0, C_("Save-combo", "All documents"), -1);
    gtk_combo_box_set_model (gxml->combo_save, GTK_TREE_MODEL (store));
    g_object_unref (store);

    gtk_cell_layout_clear (GTK_CELL_LAYOUT (gxml->combo_save));
    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (gxml->combo_save), cell, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (gxml->combo_save), cell, "text", 0);
}

static void
set_options (CommandXml *gxml,
             const char *string)
{
    int row_requires = ROW_REQUIRES_NOTHING;
    int row_save = ROW_SAVE_NOTHING;
    MooCommandOptions options = moo_parse_command_options (string);

    if (options & MOO_COMMAND_NEED_FILE)
        row_requires = ROW_REQUIRES_FILE;
    else if (options & MOO_COMMAND_NEED_DOC)
        row_requires = ROW_REQUIRES_DOC;

    if (options & MOO_COMMAND_NEED_SAVE)
        row_save = ROW_SAVE_ONE;
    else if (options & MOO_COMMAND_NEED_SAVE_ALL)
        row_save = ROW_SAVE_ALL;

    gtk_combo_box_set_active (gxml->combo_requires, row_requires);
    gtk_combo_box_set_active (gxml->combo_save, row_save);
}

static gboolean
get_options (CommandXml  *gxml,
             char       **dest)
{
    char *string;
    int row_requires;
    int row_save;
    const char *requires = NULL;
    const char *save = NULL;

    row_requires = gtk_combo_box_get_active (gxml->combo_requires);
    if (row_requires < 0 || row_requires >= ROW_REQUIRES_INVALID)
    {
        g_critical ("oops");
        row_requires = ROW_REQUIRES_DOC;
    }

    switch (row_requires)
    {
        case ROW_REQUIRES_NOTHING:
            break;
        case ROW_REQUIRES_DOC:
            requires = "need-doc";
            break;
        case ROW_REQUIRES_FILE:
            requires = "need-file";
            break;
        default:
            g_critical ("oops");
            break;
    }

    row_save = gtk_combo_box_get_active (gxml->combo_save);
    if (row_save < 0 || row_save >= ROW_SAVE_INVALID)
    {
        g_critical ("oops");
        row_save = ROW_SAVE_NOTHING;
    }

    switch (row_save)
    {
        case ROW_SAVE_NOTHING:
            break;
        case ROW_SAVE_ONE:
            save = "need-save";
            break;
        case ROW_SAVE_ALL:
            save = "need-save-all";
            break;
        default:
            g_critical ("oops");
            break;
    }

    if (requires && save)
        string = g_strdup_printf ("%s,%s", requires, save);
    else if (requires)
        string = g_strdup (requires);
    else
        string = g_strdup (save);

    if (!moo_str_equal (*dest, string))
    {
        g_free (*dest);
        *dest = string;
        return TRUE;
    }
    else
    {
        g_free (string);
        return FALSE;
    }
}


static void
update_widgets (MooPrefsPage *page,
                GtkTreeModel *model,
                GtkTreePath  *path,
                GtkTreeIter  *iter)
{
    MooCommandDisplay *helper;
    CommandXml *gxml;

    gxml = command_xml_data.get(page);
    helper = get_helper (page);

    if (path)
    {
        MooUserToolInfo *info;

        gtk_tree_model_get (model, iter, COLUMN_INFO, &info, -1);
        g_return_if_fail (info != NULL);

        _moo_command_display_set (helper, info->cmd_factory, info->cmd_data);

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gxml->enabled), info->enabled);
        gtk_entry_set_text (gxml->filter, info->filter ? info->filter : "");
        set_options (gxml, info->options);

        _moo_user_tool_info_unref (info);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gxml->enabled), FALSE);
        gtk_entry_set_text (gxml->filter, "");
        _moo_command_display_set (helper, NULL, NULL);
    }

    gtk_widget_set_sensitive (GTK_WIDGET (gxml->tool_vbox), path != NULL);
}


static gboolean
get_text (GtkEntry  *entry,
          char     **dest)
{
    const char *text = gtk_entry_get_text (entry);

    if (strcmp (*dest ? *dest : "", text) != 0)
    {
        g_free (*dest);
        *dest = text[0] ? g_strdup (text) : NULL;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void
update_model (MooPrefsPage *page,
              GtkTreeModel *model,
              G_GNUC_UNUSED GtkTreePath *path,
              GtkTreeIter  *iter)
{
    MooCommandDisplay *helper;
    MooCommandFactory *factory;
    MooCommandData *data;
    MooUserToolInfo *info;
    gboolean changed = FALSE;
    CommandXml *gxml;

    gxml = command_xml_data.get(page);

    helper = get_helper (page);
    gtk_tree_model_get (model, iter, COLUMN_INFO, &info, -1);
    g_return_if_fail (info != NULL);

    if (_moo_command_display_get (helper, &factory, &data))
    {
        moo_command_data_unref (info->cmd_data);
        info->cmd_data = moo_command_data_ref (data);
        info->cmd_factory = factory;
        gtk_list_store_set (GTK_LIST_STORE (model), iter, COLUMN_INFO, info, -1);
        changed = TRUE;
    }

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gxml->enabled)) != info->enabled)
    {
        info->enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gxml->enabled));
        changed = TRUE;
    }

    changed = get_text (gxml->filter, &info->filter) || changed;
    changed = get_options (gxml, &info->options) || changed;

    if (changed)
    {
        info->builtin = FALSE;
        set_changed (page, TRUE);
    }

    _moo_user_tool_info_unref (info);
}


static void
name_data_func (G_GNUC_UNUSED GtkTreeViewColumn *column,
                GtkCellRenderer *cell,
                GtkTreeModel    *model,
                GtkTreeIter     *iter)
{
    MooUserToolInfo *info = NULL;

    gtk_tree_model_get (model, iter, COLUMN_INFO, &info, -1);
    g_return_if_fail (info != NULL);

    g_object_set (cell, "text", info->name, nullptr);

    _moo_user_tool_info_unref (info);
}

static void
name_cell_edited (MooPrefsPage *page,
                  char         *path_string,
                  char         *text)
{
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    MooUserToolInfo *info = NULL;
    CommandXml *gxml;

    gxml = command_xml_data.get(page);

    path = gtk_tree_path_new_from_string (path_string);
    model = gtk_tree_view_get_model (gxml->treeview);

    if (gtk_tree_model_get_iter (model, &iter, path))
    {
        gtk_tree_model_get (model, &iter, COLUMN_INFO, &info, -1);
        g_return_if_fail (info != NULL);

        if (strcmp (info->name ? info->name : "", text) != 0)
        {
            set_changed (page, TRUE);
            g_free (info->name);
            info->name = g_strdup (text);
        }
    }

    gtk_tree_path_free (path);
}

static void
command_page_init (MooPrefsPage    *page,
                   MooUserToolType  type)
{
    GtkListStore *store;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;
    MooCommandDisplay *helper;
    CommandXml *gxml;

    gxml = command_xml_data.get(page);

    page_set_type (page, type);

    setup_options_widgets (gxml);

    store = gtk_list_store_new (N_COLUMNS, MOO_TYPE_USER_TOOL_INFO);

    cell = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_pack_start (column, cell, TRUE);
    gtk_tree_view_column_set_cell_data_func (column, cell,
                                             (GtkTreeCellDataFunc) name_data_func,
                                             NULL, NULL);
    gtk_tree_view_append_column (gxml->treeview, column);
    g_object_set (cell, "editable", TRUE, nullptr);
    g_signal_connect_swapped (cell, "edited", G_CALLBACK (name_cell_edited), page);

    populate_store (store, type);

    helper = _moo_command_display_new (gxml->combo_type,
                                       gxml->type_notebook,
                                       GTK_WIDGET (gxml->treeview),
                                       GTK_WIDGET (gxml->new_),
                                       GTK_WIDGET (gxml->delete_),
                                       GTK_WIDGET (gxml->up),
                                       GTK_WIDGET (gxml->down));
    g_object_set_data_full (G_OBJECT (page), "moo-tree-helper", helper, g_object_unref);
    g_signal_connect_swapped (page, "destroy", G_CALLBACK (gtk_object_destroy), helper);

    g_signal_connect_swapped (helper, "new-row", G_CALLBACK (new_row), page);
    g_signal_connect_swapped (helper, "delete-row", G_CALLBACK (delete_row), page);
    g_signal_connect_swapped (helper, "move-row", G_CALLBACK (move_row), page);
    g_signal_connect_swapped (helper, "update-widgets", G_CALLBACK (update_widgets), page);
    g_signal_connect_swapped (helper, "update-model", G_CALLBACK (update_model), page);

    gtk_tree_view_set_model (gxml->treeview, GTK_TREE_MODEL (store));

    _moo_tree_view_select_first (gxml->treeview);
    _moo_tree_helper_update_widgets (MOO_TREE_HELPER (helper));

    g_object_unref (store);
}


static void
command_page_apply (MooPrefsPage *page)
{
    MooTreeHelper *helper;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GSList *list = NULL;
    CommandXml *gxml;

    gxml = command_xml_data.get(page);

    helper = (MooTreeHelper*) g_object_get_data (G_OBJECT (page), "moo-tree-helper");
    _moo_tree_helper_update_model (helper, NULL, NULL);

    if (!get_changed (page))
        return;

    model = gtk_tree_view_get_model (gxml->treeview);

    if (gtk_tree_model_get_iter_first (model, &iter))
    {
        do
        {
            MooUserToolInfo *info;
            gtk_tree_model_get (model, &iter, COLUMN_INFO, &info, -1);
            list = g_slist_prepend (list, info);
        }
        while (gtk_tree_model_iter_next (model, &iter));
    }

    list = g_slist_reverse (list);

    _moo_edit_save_user_tools (page_get_type (page), list);
    _moo_edit_load_user_tools_type (page_get_type (page));

    g_slist_foreach (list, (GFunc) _moo_user_tool_info_unref, NULL);
    g_slist_free (list);

    set_changed (page, FALSE);
}


static void
main_page_init (PrefsPageXml *gxml)
{
    command_page_init (gxml->page_menu, MOO_USER_TOOL_MENU);
    command_page_init (gxml->page_context, MOO_USER_TOOL_CONTEXT);
}


static void
main_page_apply (PrefsPageXml *gxml)
{
    command_page_apply (gxml->page_menu);
    command_page_apply (gxml->page_context);
}


GtkWidget *
moo_user_tools_prefs_page_new (void)
{
    GtkWidget *page;
    PrefsPageXml *gxml;
    CommandXml *cxml;

    page = moo_prefs_page_new (_("Tools"), GTK_STOCK_EXECUTE);
    gxml = prefs_page_xml_new_with_root (page);

    g_signal_connect_swapped (page, "init", G_CALLBACK (main_page_init), gxml);
    g_signal_connect_swapped (page, "apply", G_CALLBACK (main_page_apply), gxml);
#ifdef MOO_ENABLE_HELP
    moo_help_set_id (page, HELP_SECTION_PREFS_USER_TOOLS);
#endif

    cxml = command_xml_new_with_root (GTK_WIDGET (gxml->page_menu));
    command_xml_data.set(gxml->page_menu, cxml);

    cxml = command_xml_new_with_root (GTK_WIDGET (gxml->page_context));
    command_xml_data.set(gxml->page_context, cxml);

    return page;
}
