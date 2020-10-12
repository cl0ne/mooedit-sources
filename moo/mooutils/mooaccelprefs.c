/*
 *   mooaccelprefs.c
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

#include "mooutils/mooaccelprefs.h"
#include "mooutils/mooaccel.h"
#include "mooutils/mooprefsdialog.h"
#include "mooutils/mooaccelbutton.h"
#include "mooutils/mooglade.h"
#include "mooutils/moodialogs.h"
#include "mooutils/mooi18n.h"
#include "mooutils/moostock.h"
#include "mooutils/mooaction-private.h"
#include "mooutils/mooactiongroup.h"
#include "mooutils/mootype-macros.h"
#include "mooutils/moohelp.h"
#include "mooutils/moocompat.h"
#include "mooutils/mooaccelprefs-gxml.h"
#ifdef MOO_ENABLE_HELP
#include "moo-help-sections.h"
#endif
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>


typedef struct {
    MooPrefsPage base;

    GtkAction *current_action;
    GtkTreeRowReference *current_row;

    AccelPrefsPageXml *gxml;
    GtkTreeSelection *selection;
    GtkTreeStore *store;

    GHashTable *changed;    /* Gtkction* -> Shortcut* */
    GPtrArray *actions;     /* GtkActionGroup* */
    GHashTable *groups;     /* char* -> GtkTreeRowReference* */
} MooAccelPrefsPage;

typedef MooPrefsPageClass MooAccelPrefsPageClass;

#define MOO_TYPE_ACCEL_PREFS_PAGE    (_moo_accel_prefs_page_get_type ())
#define MOO_ACCEL_PREFS_PAGE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_ACCEL_PREFS_PAGE, MooAccelPrefsPage))
MOO_DEFINE_TYPE_STATIC (MooAccelPrefsPage, _moo_accel_prefs_page, MOO_TYPE_PREFS_PAGE)


typedef enum {
    NONE,
    DEFAULT,
    CUSTOM
} ChoiceType;

typedef struct {
    char *accel;
    ChoiceType choice;
} Shortcut;


enum {
    COLUMN_ACTION_NAME,
    COLUMN_ACTION,
    COLUMN_ACCEL,
    COLUMN_GLOBAL,
    N_COLUMNS
};


static void moo_accel_prefs_page_init   (MooPrefsPage       *page);
static void moo_accel_prefs_page_apply  (MooPrefsPage       *page);
static void tree_selection_changed      (MooAccelPrefsPage  *page);
static void accel_set                   (MooAccelPrefsPage  *page);
static void shortcut_none_toggled       (MooAccelPrefsPage  *page);
static void shortcut_default_toggled    (MooAccelPrefsPage  *page);
static void shortcut_custom_toggled     (MooAccelPrefsPage  *page);

static void global_cell_data_func       (GtkTreeViewColumn  *column,
                                         GtkCellRenderer    *cell,
                                         GtkTreeModel       *model,
                                         GtkTreeIter        *iter);
static void global_cell_toggled         (GtkTreeStore       *store,
                                         char               *path_string);

static Shortcut *
shortcut_new (ChoiceType  choice,
              const char *accel)
{
    Shortcut *s = g_new (Shortcut, 1);
    s->choice = choice;
    s->accel = g_strdup (accel);
    return s;
}

static void
shortcut_free (Shortcut *s)
{
    g_free (s->accel);
    g_free (s);
}


static void
_moo_accel_prefs_page_finalize (GObject *object)
{
    MooAccelPrefsPage *page = MOO_ACCEL_PREFS_PAGE (object);

    g_hash_table_destroy (page->changed);
    g_hash_table_destroy (page->groups);
    g_ptr_array_free (page->actions, TRUE);

    G_OBJECT_CLASS (_moo_accel_prefs_page_parent_class)->finalize (object);
}


static void
_moo_accel_prefs_page_class_init (MooAccelPrefsPageClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = _moo_accel_prefs_page_finalize;
    MOO_PREFS_PAGE_CLASS (klass)->init = moo_accel_prefs_page_init;
    MOO_PREFS_PAGE_CLASS (klass)->apply = moo_accel_prefs_page_apply;
}


static void
row_activated (MooAccelPrefsPage *page)
{
    if (GTK_WIDGET_IS_SENSITIVE (page->gxml->shortcut))
        gtk_button_clicked (GTK_BUTTON (page->gxml->shortcut));
}


static void
block_accel_set (MooAccelPrefsPage *page)
{
    g_signal_handlers_block_matched (page->gxml->shortcut, G_SIGNAL_MATCH_FUNC,
                                     0, 0, 0, (gpointer) accel_set, 0);
}

static void
unblock_accel_set (MooAccelPrefsPage *page)
{
    g_signal_handlers_unblock_matched (page->gxml->shortcut, G_SIGNAL_MATCH_FUNC,
                                       0, 0, 0, (gpointer) accel_set, 0);
}

static void
block_radio (MooAccelPrefsPage *page)
{
    g_signal_handlers_block_matched (page->gxml->shortcut_none, G_SIGNAL_MATCH_FUNC,
                                     0, 0, 0, (gpointer) shortcut_none_toggled, 0);
    g_signal_handlers_block_matched (page->gxml->shortcut_default, G_SIGNAL_MATCH_FUNC,
                                     0, 0, 0, (gpointer) shortcut_default_toggled, 0);
    g_signal_handlers_block_matched (page->gxml->shortcut_custom, G_SIGNAL_MATCH_FUNC,
                                     0, 0, 0, (gpointer) shortcut_custom_toggled, 0);
}

static void
unblock_radio (MooAccelPrefsPage *page)
{
    g_signal_handlers_unblock_matched (page->gxml->shortcut_none, G_SIGNAL_MATCH_FUNC,
                                       0, 0, 0, (gpointer) shortcut_none_toggled, 0);
    g_signal_handlers_unblock_matched (page->gxml->shortcut_default, G_SIGNAL_MATCH_FUNC,
                                       0, 0, 0, (gpointer) shortcut_default_toggled, 0);
    g_signal_handlers_unblock_matched (page->gxml->shortcut_custom, G_SIGNAL_MATCH_FUNC,
                                       0, 0, 0, (gpointer) shortcut_custom_toggled, 0);
}


static void
global_cell_data_func (G_GNUC_UNUSED GtkTreeViewColumn *column,
                       GtkCellRenderer    *cell,
                       GtkTreeModel       *model,
                       GtkTreeIter        *iter)
{
    GtkAction *action = NULL;
    char *accel = NULL;

    gtk_tree_model_get (model, iter,
                        COLUMN_ACTION, &action,
                        COLUMN_ACCEL, &accel,
                        -1);

    if (action)
        g_object_set (cell,
                      "visible", TRUE,
                      "sensitive", accel && accel[0],
                      "activatable", accel && accel[0],
                      NULL);
    else
        g_object_set (cell,
                      "visible", FALSE,
                      NULL);

    if (action)
        g_object_unref (action);
    g_free (accel);
}

static void
global_cell_toggled (GtkTreeStore *store,
                     char         *path_string)
{
    GtkTreePath *path;
    GtkTreeIter iter;

    path = gtk_tree_path_new_from_string (path_string);

    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path))
    {
        gboolean value;
        gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, COLUMN_GLOBAL, &value, -1);
        gtk_tree_store_set (store, &iter, COLUMN_GLOBAL, !value, -1);
    }

    gtk_tree_path_free (path);
}


static void
_moo_accel_prefs_page_init (MooAccelPrefsPage *page)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    page->changed = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL,
                                           (GDestroyNotify) shortcut_free);
    page->actions = g_ptr_array_new ();
    page->groups = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          (GDestroyNotify) g_free,
                                          (GDestroyNotify) gtk_tree_row_reference_free);

    page->gxml = accel_prefs_page_xml_new_with_root (GTK_WIDGET (page));
    g_object_set (page, "label", "Shortcuts", "icon-stock-id", MOO_STOCK_KEYBOARD, NULL);

    gtk_tree_view_set_search_column (page->gxml->treeview, 0);

    g_signal_connect_swapped (page->gxml->treeview, "row-activated",
                              G_CALLBACK (row_activated),
                              page);

    page->store = gtk_tree_store_new (N_COLUMNS,
                                      G_TYPE_STRING,
                                      GTK_TYPE_ACTION,
                                      G_TYPE_STRING,
                                      G_TYPE_BOOLEAN);
    gtk_tree_view_set_model (page->gxml->treeview, GTK_TREE_MODEL (page->store));
    g_object_unref (page->store);

    renderer = gtk_cell_renderer_text_new ();
    /* Column label in Configure Shortcuts dialog */
    column = gtk_tree_view_column_new_with_attributes (C_("accel-editor-column", "Action"),
                                                       renderer,
                                                       "text", COLUMN_ACTION_NAME,
                                                       NULL);
    gtk_tree_view_append_column (page->gxml->treeview, column);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ACTION_NAME);

    renderer = gtk_cell_renderer_text_new ();
    /* Column label in Configure Shortcuts dialog */
    column = gtk_tree_view_column_new_with_attributes (C_("accel-editor-column", "Shortcut"),
                                                       renderer,
                                                       "text", COLUMN_ACCEL,
                                                       NULL);
    gtk_tree_view_append_column (page->gxml->treeview, column);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_ACCEL);

    renderer = gtk_cell_renderer_toggle_new ();
    /* Column label in Configure Shortcuts dialog */
    column = gtk_tree_view_column_new_with_attributes (C_("accel-editor-column", "Global"),
                                                       renderer,
                                                       "active", COLUMN_GLOBAL,
                                                       NULL);
    gtk_tree_view_append_column (page->gxml->treeview, column);
    gtk_tree_view_column_set_sort_column_id (column, COLUMN_GLOBAL);
    gtk_tree_view_column_set_cell_data_func (column, renderer,
                                             (GtkTreeCellDataFunc) global_cell_data_func,
                                             NULL, NULL);
    g_signal_connect_swapped (renderer, "toggled",
                              G_CALLBACK (global_cell_toggled),
                              page->store);

    page->selection = gtk_tree_view_get_selection (page->gxml->treeview);
    gtk_tree_selection_set_mode (page->selection, GTK_SELECTION_SINGLE);

    g_signal_connect_swapped (page->selection, "changed",
                              G_CALLBACK (tree_selection_changed),
                              page);

    g_signal_connect_swapped (page->gxml->shortcut, "accel-set",
                              G_CALLBACK (accel_set),
                              page);
    g_signal_connect_swapped (page->gxml->shortcut_none, "toggled",
                              G_CALLBACK (shortcut_none_toggled),
                              page);
    g_signal_connect_swapped (page->gxml->shortcut_default, "toggled",
                              G_CALLBACK (shortcut_default_toggled),
                              page);
    g_signal_connect_swapped (page->gxml->shortcut_custom, "toggled",
                              G_CALLBACK (shortcut_custom_toggled),
                              page);
}


static const char *
get_action_accel (GtkAction *action)
{
    const char *accel_path = _moo_action_get_accel_path (action);
    return _moo_get_accel (accel_path);
}


static void
apply_one (GtkAction *action,
           Shortcut  *shortcut)
{
    const char *accel_path = _moo_action_get_accel_path (action);
    const char *default_accel = _moo_get_default_accel (accel_path);
    const char *new_accel = "";

    switch (shortcut->choice)
    {
        case NONE:
            new_accel = "";
            break;
        case CUSTOM:
            new_accel = shortcut->accel;
            break;
        case DEFAULT:
            new_accel = default_accel;
            break;
        default:
            g_return_if_reached ();
    }

    _moo_modify_accel (accel_path, new_accel);
}

static gboolean
apply_global (GtkTreeModel *model,
              G_GNUC_UNUSED GtkTreePath *path,
              GtkTreeIter  *iter)
{
    GtkAction *action = NULL;
    gboolean global = FALSE;

    gtk_tree_model_get (model, iter,
                        COLUMN_ACTION, &action,
                        COLUMN_GLOBAL, &global,
                        -1);

    if (action)
    {
        _moo_accel_prefs_set_global (_moo_action_get_accel_path (action), global);
        g_object_unref (action);
    }

    return FALSE;
}

static void
moo_accel_prefs_page_apply (MooPrefsPage *prefs_page)
{
    MooAccelPrefsPage *page = MOO_ACCEL_PREFS_PAGE (prefs_page);
    g_hash_table_foreach (page->changed, (GHFunc) apply_one, NULL);
    g_hash_table_foreach_remove (page->changed, (GHRFunc) gtk_true, NULL);
    gtk_tree_model_foreach (GTK_TREE_MODEL (page->store),
                            (GtkTreeModelForeachFunc) apply_global,
                            NULL);
}


static char *
get_accel_label_for_path (const char *accel_path)
{
    g_return_val_if_fail (accel_path != NULL, g_strdup (""));
    return _moo_get_accel_label (_moo_get_accel (accel_path));
}


static gboolean
add_row (GtkActionGroup    *group,
         GtkAction         *action,
         MooAccelPrefsPage *page)
{
    const char *group_name;
    char *accel;
    const char *name;
    GtkTreeIter iter;
    GtkTreeRowReference *row;
    gboolean global;

    if (_moo_action_get_no_accel (action) || !_moo_action_get_accel_editable (action))
        return FALSE;

    group_name = _moo_action_group_get_display_name (MOO_ACTION_GROUP (group));

    if (!group_name)
        group_name = "";

    row = (GtkTreeRowReference*) g_hash_table_lookup (page->groups, group_name);

    if (row)
    {
        GtkTreeIter parent;
        GtkTreePath *path = gtk_tree_row_reference_get_path (row);
        if (!path || !gtk_tree_model_get_iter (GTK_TREE_MODEL (page->store), &parent, path))
        {
            g_critical ("got invalid path for group %s", group_name);
            gtk_tree_path_free (path);
            return FALSE;
        }
        gtk_tree_path_free (path);

        gtk_tree_store_append (page->store, &iter, &parent);
    }
    else
    {
        GtkTreeIter group_iter;
        GtkTreePath *path;

        gtk_tree_store_append (page->store, &group_iter, NULL);
        gtk_tree_store_set (page->store, &group_iter,
                            COLUMN_ACTION_NAME, group_name,
                            COLUMN_ACTION, NULL,
                            COLUMN_ACCEL, NULL,
                            -1);
        path = gtk_tree_model_get_path (GTK_TREE_MODEL (page->store), &group_iter);
        g_hash_table_insert (page->groups,
                             g_strdup (group_name),
                             gtk_tree_row_reference_new (GTK_TREE_MODEL (page->store), path));
        gtk_tree_path_free (path);

        gtk_tree_store_append (page->store, &iter, &group_iter);
    }

    accel = get_accel_label_for_path (_moo_action_get_accel_path (action));
    name = _moo_action_get_display_name (action);
    global = _moo_accel_prefs_get_global (gtk_action_get_accel_path (action));

    gtk_tree_store_set (page->store, &iter,
                        COLUMN_ACTION_NAME, name,
                        COLUMN_ACTION, action,
                        COLUMN_ACCEL, accel,
                        COLUMN_GLOBAL, global,
                        -1);
    g_free (accel);

    return FALSE;
}


static void
moo_accel_prefs_page_init (MooPrefsPage *prefs_page)
{
    guint i;
    MooAccelPrefsPage *page = MOO_ACCEL_PREFS_PAGE (prefs_page);

    for (i = 0; i < page->actions->len; ++i)
    {
        GtkActionGroup *group = (GtkActionGroup*) g_ptr_array_index (page->actions, i);
        GList *list = gtk_action_group_list_actions (group);

        while (list)
        {
            add_row (group, GTK_ACTION (list->data), page);
            list = g_list_delete_link (list, list);
        }
    }

    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (page->store),
                                          COLUMN_ACTION_NAME,
                                          GTK_SORT_ASCENDING);
    gtk_tree_view_expand_all (page->gxml->treeview);
    tree_selection_changed (page);
}


static void
tree_selection_changed (MooAccelPrefsPage *page)
{
    gboolean selected_action = FALSE;
    GtkTreeIter iter;
    GtkAction *action = NULL;
    GtkTreePath *path;
    char *default_label;
    const char *default_accel, *new_accel = "";
    GtkRadioButton *new_button = NULL;
    Shortcut *shortcut;

    if (gtk_tree_selection_get_selected (page->selection, NULL, &iter))
    {
        gtk_tree_model_get (GTK_TREE_MODEL (page->store), &iter,
                            COLUMN_ACTION, &action, -1);
        if (action)
            selected_action = TRUE;
    }

    page->current_action = action;
    gtk_tree_row_reference_free (page->current_row);
    page->current_row = NULL;

    if (!selected_action)
    {
        gtk_label_set_text (page->gxml->default_label, "");
        gtk_widget_set_sensitive (GTK_WIDGET (page->gxml->shortcut_frame), FALSE);
        return;
    }

    path = gtk_tree_model_get_path (GTK_TREE_MODEL (page->store), &iter);
    page->current_row = gtk_tree_row_reference_new (GTK_TREE_MODEL (page->store), path);
    gtk_tree_path_free (path);

    gtk_widget_set_sensitive (GTK_WIDGET (page->gxml->shortcut_frame), TRUE);

    default_accel = _moo_get_default_accel (_moo_action_get_accel_path (action));
    default_label = _moo_get_accel_label (default_accel);

    if (!default_label || !default_label[0])
        gtk_label_set_text (page->gxml->default_label, "None");
    else
        gtk_label_set_text (page->gxml->default_label, default_label);

    block_radio (page);
    block_accel_set (page);

    shortcut = (Shortcut*) g_hash_table_lookup (page->changed, action);

    if (shortcut)
    {
        switch (shortcut->choice)
        {
            case NONE:
                new_button = page->gxml->shortcut_none;
                new_accel = NULL;
                break;

            case DEFAULT:
                new_button = page->gxml->shortcut_default;
                new_accel = default_accel;
                break;

            case CUSTOM:
                new_button = page->gxml->shortcut_custom;
                new_accel = shortcut->accel;
                break;

            default:
                g_assert_not_reached ();
        }
    }
    else
    {
        const char *accel = get_action_accel (action);

        if (!strcmp (accel, default_accel))
        {
            new_button = page->gxml->shortcut_default;
            new_accel = default_accel;
        }
        else if (!accel[0])
        {
            new_button = page->gxml->shortcut_none;
            new_accel = NULL;
        }
        else
        {
            new_button = page->gxml->shortcut_custom;
            new_accel = accel;
        }
    }

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (new_button), TRUE);
    _moo_accel_button_set_accel (page->gxml->shortcut, new_accel);

    unblock_radio (page);
    unblock_accel_set (page);

    g_free (default_label);
    g_object_unref (action);
}


static void
accel_set (MooAccelPrefsPage *page)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    const char *accel;
    const char *label;

    g_return_if_fail (page->current_row != NULL && page->current_action != NULL);

    block_radio (page);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (page->gxml->shortcut_custom), TRUE);
    unblock_radio (page);

    path = gtk_tree_row_reference_get_path (page->current_row);
    if (!path || !gtk_tree_model_get_iter (GTK_TREE_MODEL (page->store), &iter, path))
    {
        g_critical ("got invalid path");
        gtk_tree_path_free (path);
        return;
    }
    gtk_tree_path_free (path);

    accel = _moo_accel_button_get_accel (page->gxml->shortcut);
    label = gtk_button_get_label (GTK_BUTTON (page->gxml->shortcut));
    gtk_tree_store_set (page->store, &iter, COLUMN_ACCEL, label, -1);

    if (accel && accel[0])
        g_hash_table_insert (page->changed,
                             page->current_action,
                             shortcut_new (CUSTOM, accel));
    else
        g_hash_table_insert (page->changed,
                             page->current_action,
                             shortcut_new (NONE, ""));
}


static void
shortcut_none_toggled (MooAccelPrefsPage *page)
{
    GtkTreeIter iter;
    GtkTreePath *path;

    g_return_if_fail (page->current_row != NULL && page->current_action != NULL);

    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (page->gxml->shortcut_none)))
        return;

    path = gtk_tree_row_reference_get_path (page->current_row);
    if (!path || !gtk_tree_model_get_iter (GTK_TREE_MODEL (page->store), &iter, path))
    {
        g_critical ("got invalid path");
        gtk_tree_path_free (path);
        return;
    }
    gtk_tree_path_free (path);

    gtk_tree_store_set (page->store, &iter, COLUMN_ACCEL, NULL, -1);
    g_hash_table_insert (page->changed,
                         page->current_action,
                         shortcut_new (NONE, ""));
    block_accel_set (page);
    _moo_accel_button_set_accel (page->gxml->shortcut, "");
    unblock_accel_set (page);
}


static void
shortcut_default_toggled (MooAccelPrefsPage *page)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    Shortcut *current_shortcut;
    const char *default_accel;

    g_return_if_fail (page->current_row != NULL && page->current_action != NULL);

    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (page->gxml->shortcut_default)))
        return;

    path = gtk_tree_row_reference_get_path (page->current_row);

    if (!path || !gtk_tree_model_get_iter (GTK_TREE_MODEL (page->store), &iter, path))
    {
        g_critical ("got invalid path");
        gtk_tree_path_free (path);
        return;
    }

    gtk_tree_path_free (path);

    current_shortcut = (Shortcut*) g_hash_table_lookup (page->changed, page->current_action);

    if (!current_shortcut)
    {
        current_shortcut = shortcut_new (NONE, "");
        g_hash_table_insert (page->changed,
                             page->current_action,
                             current_shortcut);
    }

    default_accel = _moo_action_get_default_accel (page->current_action);

    if (default_accel[0])
    {
        char *label = _moo_get_accel_label (default_accel);
        gtk_tree_store_set (page->store, &iter, COLUMN_ACCEL, label, -1);
        g_free (label);
    }
    else
    {
        gtk_tree_store_set (page->store, &iter, COLUMN_ACCEL, NULL, -1);
    }

    current_shortcut->choice = DEFAULT;
    g_free (current_shortcut->accel);
    current_shortcut->accel = g_strdup (default_accel);

    block_accel_set (page);
    _moo_accel_button_set_accel (page->gxml->shortcut, default_accel);
    unblock_accel_set (page);
}


static void
shortcut_custom_toggled (MooAccelPrefsPage *page)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    Shortcut *shortcut;

    g_return_if_fail (page->current_row != NULL && page->current_action != NULL);

    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (page->gxml->shortcut_custom)))
        return;

    path = gtk_tree_row_reference_get_path (page->current_row);

    if (!path || !gtk_tree_model_get_iter (GTK_TREE_MODEL (page->store), &iter, path))
    {
        g_critical ("got invalid path");
        gtk_tree_path_free (path);
        return;
    }

    gtk_tree_path_free (path);

    shortcut = (Shortcut*) g_hash_table_lookup (page->changed, page->current_action);

    if (shortcut)
    {
        block_accel_set (page);
        _moo_accel_button_set_accel (page->gxml->shortcut, shortcut->accel);
        unblock_accel_set (page);

        gtk_tree_store_set (page->store, &iter, COLUMN_ACCEL,
                            gtk_button_get_label (GTK_BUTTON (page->gxml->shortcut)),
                            -1);

        shortcut->choice = CUSTOM;
    }
    else
    {
        gtk_button_clicked (GTK_BUTTON (page->gxml->shortcut));
    }
}


static void
dialog_response (GObject *page,
                 int response)
{
    switch (response)
    {
        case GTK_RESPONSE_OK:
            g_signal_emit_by_name (page, "apply", NULL);
            break;

        case GTK_RESPONSE_REJECT:
            g_signal_emit_by_name (page, "set_defaults", NULL);
            break;
    };
}


static MooAccelPrefsPage *
_moo_accel_prefs_page_new (MooActionCollection *collection)
{
    const GSList *groups;
    MooAccelPrefsPage *page;

    page = MOO_ACCEL_PREFS_PAGE (g_object_new (MOO_TYPE_ACCEL_PREFS_PAGE, (const char*) NULL));
    groups = moo_action_collection_get_groups (collection);

    while (groups)
    {
        g_ptr_array_add (page->actions, groups->data);
        groups = groups->next;
    }

    return page;
}


static GtkWidget *
_moo_accel_prefs_dialog_new (MooActionCollection *collection)
{
    MooAccelPrefsPage *page;
    GtkWidget *dialog;

    dialog = gtk_dialog_new_with_buttons (_("Configure Shortcuts"), NULL,
                                          (GtkDialogFlags) 0,
                                          GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                                          NULL);
    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_OK,
                                             GTK_RESPONSE_CANCEL,
                                             -1);
    gtk_window_set_default_size (GTK_WINDOW (dialog), -1, 400);

    page = _moo_accel_prefs_page_new (collection);
    gtk_widget_show (GTK_WIDGET (page));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), GTK_WIDGET (page), TRUE, TRUE, 0);

#ifdef MOO_ENABLE_HELP
    moo_help_set_id (dialog, HELP_SECTION_PREFS_ACCELS);
#endif
    moo_help_connect_keys (dialog);

    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (dialog_response),
                              page);
    g_signal_emit_by_name (page, "init", NULL);

    return dialog;
}


gboolean
_moo_accel_prefs_dialog_run (MooActionCollection *collection,
                             GtkWidget           *parent)
{
    GtkWidget *dialog;
    int response;

    dialog = _moo_accel_prefs_dialog_new (collection);
    moo_window_set_parent (dialog, parent);

    while ((response = gtk_dialog_run (GTK_DIALOG (dialog))) == GTK_RESPONSE_HELP)
        moo_help_open (dialog);

    gtk_widget_destroy (dialog);
    return response == GTK_RESPONSE_OK;
}
