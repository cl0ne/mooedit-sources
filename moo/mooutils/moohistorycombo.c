/*
 *   moohistorycombo.c
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

/**
 * class:MooHistoryCombo: (parent MooCombo) (constructable) (moo.private 1)
 **/

#include "marshals.h"
#include "mooutils/moohistorycombo.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/moocompat.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>


struct _MooHistoryComboPrivate {
    gboolean enable_completion;
    gboolean called_popup;
    guint completion_popup_timeout_id;
    guint completion_popup_timeout;

    MooHistoryList *list;
    GtkTreeModel *model;
    GtkTreeModel *filter;
    char *completion_text;
    MooHistoryComboFilterFunc filter_func;
    gpointer filter_data;
    gboolean disable_filter;

    gboolean sort_history;
    gboolean sort_completion;
};


static void     moo_history_combo_dispose       (GObject            *object);
static void     moo_history_combo_finalize      (GObject            *object);
static void     moo_history_combo_set_property  (GObject            *object,
                                                 guint               prop_id,
                                                 const GValue       *value,
                                                 GParamSpec         *pspec);
static void     moo_history_combo_get_property  (GObject            *object,
                                                 guint               prop_id,
                                                 GValue             *value,
                                                 GParamSpec         *pspec);

static void     moo_history_combo_popup         (MooCombo           *combo);
static void     moo_history_combo_changed       (MooCombo           *combo);

static gboolean completion_visible_func         (GtkTreeModel       *model,
                                                 GtkTreeIter        *iter,
                                                 MooHistoryCombo    *combo);
static gboolean default_filter_func             (const char         *text,
                                                 GtkTreeModel       *model,
                                                 GtkTreeIter        *iter,
                                                 gpointer            data);
static int      default_sort_func               (GtkTreeModel       *model,
                                                 GtkTreeIter        *a,
                                                 GtkTreeIter        *b);
static void     cell_data_func                  (GtkCellLayout      *cell_layout,
                                                 GtkCellRenderer    *cell,
                                                 GtkTreeModel       *tree_model,
                                                 GtkTreeIter        *iter);
static gboolean row_separator_func              (GtkTreeModel       *model,
                                                 GtkTreeIter        *iter,
                                                 gpointer            data);
static char    *get_text_func                   (GtkTreeModel       *model,
                                                 GtkTreeIter        *iter,
                                                 gpointer            data);


/* MOO_TYPE_HISTORY_COMBO */
G_DEFINE_TYPE (MooHistoryCombo, moo_history_combo, MOO_TYPE_COMBO)


enum {
    PROP_0,
    PROP_LIST,
    PROP_SORT_HISTORY,
    PROP_SORT_COMPLETION,
    PROP_HISTORY_LIST_ID,
    PROP_ENABLE_COMPLETION
};


static void
moo_history_combo_class_init (MooHistoryComboClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    MooComboClass *combo_class = MOO_COMBO_CLASS (klass);

    moo_history_combo_parent_class = g_type_class_peek_parent (klass);

    g_type_class_add_private (klass, sizeof (MooHistoryComboPrivate));

    gobject_class->dispose = moo_history_combo_dispose;
    gobject_class->finalize = moo_history_combo_finalize;
    gobject_class->set_property = moo_history_combo_set_property;
    gobject_class->get_property = moo_history_combo_get_property;

    combo_class->popup = moo_history_combo_popup;
    combo_class->changed = moo_history_combo_changed;

    g_object_class_install_property (gobject_class,
                                     PROP_LIST,
                                     g_param_spec_object ("list",
                                             "list",
                                             "list",
                                             MOO_TYPE_HISTORY_LIST,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_SORT_HISTORY,
                                     g_param_spec_boolean ("sort-history",
                                             "sort-history",
                                             "sort-history",
                                             TRUE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_SORT_COMPLETION,
                                     g_param_spec_boolean ("sort-completion",
                                             "sort-completion",
                                             "sort-completion",
                                             FALSE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_ENABLE_COMPLETION,
                                     g_param_spec_boolean ("enable-completion",
                                             "enable-completion",
                                             "enable-completion",
                                             TRUE,
                                             (GParamFlags) G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class,
                                     PROP_HISTORY_LIST_ID,
                                     g_param_spec_string ("history-list-id",
                                             "history-list-id",
                                             "history-list-id",
                                             NULL,
                                             G_PARAM_WRITABLE));
}


static void
moo_history_combo_init (MooHistoryCombo *combo)
{
    GtkCellRenderer *cell;

    combo->priv = G_TYPE_INSTANCE_GET_PRIVATE (combo, MOO_TYPE_HISTORY_COMBO, MooHistoryComboPrivate);

    combo->priv->filter_func = default_filter_func;
    combo->priv->filter_data = combo;
    combo->priv->completion_text = g_strdup ("");
    combo->priv->enable_completion = TRUE;

    gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
    cell = gtk_cell_renderer_text_new ();
    g_object_set (cell, "single-paragraph-mode", TRUE, NULL);
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo), cell,
                                        (GtkCellLayoutDataFunc) cell_data_func,
                                        combo, NULL);

    combo->priv->list = moo_history_list_new (NULL);
    combo->priv->model = gtk_tree_model_sort_new_with_model (moo_history_list_get_model (combo->priv->list));
    combo->priv->filter = gtk_tree_model_filter_new (combo->priv->model, NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (combo->priv->filter),
                                            (GtkTreeModelFilterVisibleFunc) completion_visible_func,
                                            combo, NULL);
    moo_combo_set_model (MOO_COMBO (combo), combo->priv->filter);
    moo_combo_set_row_separator_func (MOO_COMBO (combo), row_separator_func, NULL);
    moo_combo_set_get_text_func (MOO_COMBO (combo), get_text_func, NULL);
}


static void
set_history_list_id (MooHistoryCombo *combo,
                     const char      *id)
{
    if (id)
    {
        MooHistoryList *list = moo_history_list_get (id);
        moo_history_combo_set_list (combo, list);
    }
}


static void
moo_history_combo_set_property (GObject        *object,
                                guint           prop_id,
                                const GValue   *value,
                                GParamSpec     *pspec)
{
    MooHistoryCombo *combo = MOO_HISTORY_COMBO (object);

    switch (prop_id)
    {
        case PROP_LIST:
            moo_history_combo_set_list (combo, MOO_HISTORY_LIST (g_value_get_object (value)));
            break;

        case PROP_SORT_HISTORY:
            combo->priv->sort_history = g_value_get_boolean (value);
            g_object_notify (object, "sort-history");
            break;

        case PROP_SORT_COMPLETION:
            combo->priv->sort_completion = g_value_get_boolean (value);
            g_object_notify (object, "sort-completion");
            break;

        case PROP_ENABLE_COMPLETION:
            combo->priv->enable_completion = g_value_get_boolean (value);
            g_object_notify (object, "enable-completion");
            break;

        case PROP_HISTORY_LIST_ID:
            set_history_list_id (combo, g_value_get_string (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_history_combo_get_property (GObject        *object,
                                guint           prop_id,
                                GValue         *value,
                                GParamSpec     *pspec)
{
    MooHistoryCombo *combo = MOO_HISTORY_COMBO (object);

    switch (prop_id)
    {
        case PROP_LIST:
            g_value_set_object (value, combo->priv->list);
            break;

        case PROP_SORT_HISTORY:
            g_value_set_boolean (value, combo->priv->sort_history);
            break;

        case PROP_SORT_COMPLETION:
            g_value_set_boolean (value, combo->priv->sort_completion);
            break;

        case PROP_ENABLE_COMPLETION:
            g_value_set_boolean (value, combo->priv->enable_completion);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


static void
moo_history_combo_dispose (GObject *object)
{
    MooHistoryCombo *combo = MOO_HISTORY_COMBO (object);

    if (combo->priv->filter)
    {
        g_object_unref (combo->priv->filter);
        g_object_unref (combo->priv->model);
        combo->priv->filter = NULL;
        combo->priv->model = NULL;
    }

    if (combo->priv->list)
    {
        g_object_unref (combo->priv->list);
        combo->priv->list = NULL;
    }

    G_OBJECT_CLASS (moo_history_combo_parent_class)->dispose (object);
}


static void
moo_history_combo_finalize (GObject *object)
{
    MooHistoryCombo *combo = MOO_HISTORY_COMBO (object);

    g_free (combo->priv->completion_text);

    G_OBJECT_CLASS (moo_history_combo_parent_class)->finalize (object);
}


static gboolean
model_is_empty (GtkTreeModel *model)
{
    GtkTreeIter iter;
    return !gtk_tree_model_get_iter_first (model, &iter);
}


static void
get_entry_text (MooHistoryCombo *combo)
{
    g_free (combo->priv->completion_text);
    combo->priv->completion_text = g_strdup (moo_combo_entry_get_text (MOO_COMBO (combo)));
}


static void
moo_history_combo_popup (MooCombo *combo)
{
    MooHistoryCombo *entry = MOO_HISTORY_COMBO (combo);
    gboolean do_sort = FALSE;

    if (!entry->priv->called_popup)
    {
        entry->priv->disable_filter = TRUE;
        if (entry->priv->sort_history)
            do_sort = TRUE;
    }
    else
    {
        entry->priv->completion_popup_timeout_id = 0;
        entry->priv->disable_filter = FALSE;
        entry->priv->called_popup = FALSE;

        if (entry->priv->sort_completion)
            do_sort = TRUE;

        if (moo_combo_popup_shown (MOO_COMBO (entry)))
            return;

        if (!entry->priv->filter)
            return;

        get_entry_text (entry);
    }

    if (do_sort)
        gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (entry->priv->model),
                                                 (GtkTreeIterCompareFunc) default_sort_func,
                                                 NULL, NULL);
    else
        gtk_tree_model_sort_reset_default_sort_func (GTK_TREE_MODEL_SORT (entry->priv->model));

    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (entry->priv->filter));
    entry->priv->disable_filter = FALSE;

    MOO_COMBO_CLASS(moo_history_combo_parent_class)->popup (combo);
}


static int
default_sort_func (GtkTreeModel       *model,
                   GtkTreeIter        *a,
                   GtkTreeIter        *b)
{
    MooHistoryListItem *e1 = NULL, *e2 = NULL;
    int result;

    /* XXX 0 is not good */
    gtk_tree_model_get (model, a, 0, &e1, -1);
    gtk_tree_model_get (model, b, 0, &e2, -1);

    if (!e1 && !e2)
        result = 0;
    else if (!e1)
        result = e2->builtin ? 1 : -1;
    else if (!e2)
        result = e1->builtin ? -1 : 1;
    else if (e1->builtin != e2->builtin)
        result = e1->builtin ? -1 : 1;
    else
        result = strcmp (e1->data, e2->data);

    moo_history_list_item_free (e1);
    moo_history_list_item_free (e2);
    return result;
}


static void
refilter (MooHistoryCombo *combo)
{
    get_entry_text (combo);
    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (combo->priv->filter));

    if (model_is_empty (combo->priv->filter))
        moo_combo_popdown (MOO_COMBO (combo));
    else
        moo_combo_update_popup (MOO_COMBO (combo));
}


static gboolean
popup (MooHistoryCombo *combo)
{
    combo->priv->called_popup = TRUE;
    moo_combo_popup (MOO_COMBO (combo));
    return FALSE;
}


static void
moo_history_combo_changed (MooCombo *combo)
{
    MooHistoryCombo *hist_combo = MOO_HISTORY_COMBO (combo);

    if (!GTK_WIDGET_MAPPED (combo))
        return;

    if (!hist_combo->priv->enable_completion &&
        !moo_combo_popup_shown (combo))
            return;

    get_entry_text (hist_combo);

    if (!hist_combo->priv->completion_text[0] ||
         moo_combo_get_active_iter (combo, NULL))
    {
        if (hist_combo->priv->completion_popup_timeout_id)
            g_source_remove (hist_combo->priv->completion_popup_timeout_id);

        moo_combo_popdown (combo);
    }
    else if (moo_combo_popup_shown (combo))
    {
        refilter (hist_combo);
    }
    else if (hist_combo->priv->completion_popup_timeout)
    {
        if (hist_combo->priv->completion_popup_timeout_id)
            g_source_remove (hist_combo->priv->completion_popup_timeout_id);
        hist_combo->priv->completion_popup_timeout_id =
                g_timeout_add (hist_combo->priv->completion_popup_timeout,
                               (GSourceFunc) popup,
                               combo);
    }
    else
    {
        popup (hist_combo);
    }
}


static gboolean
completion_visible_func (GtkTreeModel    *model,
                         GtkTreeIter     *iter,
                         MooHistoryCombo *combo)
{
    if (combo->priv->disable_filter)
        return TRUE;
    else
        return combo->priv->filter_func (combo->priv->completion_text,
                                         model, iter, combo->priv->filter_data);
}


static gboolean
default_filter_func (const char         *entry_text,
                     GtkTreeModel       *model,
                     GtkTreeIter        *iter,
                     G_GNUC_UNUSED gpointer data)
{
    gboolean visible;
    MooHistoryListItem *e = NULL;

    gtk_tree_model_get (model, iter, 0, &e, -1);

    if (entry_text[0])
    {
        if (!e)
            visible = FALSE;
        else
            visible = !strncmp (entry_text, e->data, strlen (entry_text));
    }
    else
    {
        visible = TRUE;
    }

    moo_history_list_item_free (e);
    return visible ? TRUE : FALSE;
}


GtkWidget*
moo_history_combo_new (const char *user_id)
{
    MooHistoryCombo *combo;

    combo = MOO_HISTORY_COMBO (g_object_new (MOO_TYPE_HISTORY_COMBO, (const char*) NULL));

    if (user_id)
    {
        MooHistoryList *list = moo_history_list_get (user_id);
        moo_history_combo_set_list (combo, list);
    }

    return GTK_WIDGET (combo);
}


void
moo_history_combo_add_text (MooHistoryCombo *combo,
                            const char      *text)
{
    g_return_if_fail (MOO_IS_HISTORY_COMBO (combo));
    g_return_if_fail (text != NULL);
    moo_history_list_add (combo->priv->list, text);
}


void
moo_history_combo_commit (MooHistoryCombo *combo)
{
    GtkTreeIter iter;
    const char *text;
    MooHistoryListItem *freeme = NULL;

    g_return_if_fail (MOO_IS_HISTORY_COMBO (combo));

    text = moo_combo_entry_get_text (MOO_COMBO (combo));

    if (!text || !*text)
        return;

    if (moo_history_list_find (combo->priv->list, text, &iter))
    {
        freeme = moo_history_list_get_item (combo->priv->list, &iter);
        text = freeme->data;
    }

    moo_history_combo_add_text (combo, text);

    if (freeme)
        moo_history_list_item_free (freeme);
}


void
moo_history_combo_set_list (MooHistoryCombo *combo,
                            MooHistoryList  *list)
{
    g_return_if_fail (MOO_IS_HISTORY_COMBO (combo));
    g_return_if_fail (!list || MOO_IS_HISTORY_LIST (list));

    if (combo->priv->list == list)
        return;

    if (!list)
    {
        list = moo_history_list_new (NULL);
        moo_history_combo_set_list (combo, list);
        g_object_unref (list);
        return;
    }

    g_object_unref (combo->priv->list);
    moo_combo_set_model (MOO_COMBO (combo), NULL);
    g_object_unref (combo->priv->filter);

    combo->priv->list = list;
    g_object_ref (combo->priv->list);
    combo->priv->model = gtk_tree_model_sort_new_with_model (moo_history_list_get_model (combo->priv->list));
    combo->priv->filter = gtk_tree_model_filter_new (combo->priv->model, NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (combo->priv->filter),
                                            (GtkTreeModelFilterVisibleFunc) completion_visible_func,
                                            combo, NULL);
    moo_combo_set_model (MOO_COMBO (combo), combo->priv->filter);

    _moo_history_list_load (list);

    g_object_notify (G_OBJECT (combo), "list");
}


MooHistoryList*
moo_history_combo_get_list (MooHistoryCombo    *combo)
{
    g_return_val_if_fail (MOO_IS_HISTORY_COMBO (combo), NULL);
    return combo->priv->list;
}


static void
cell_data_func (G_GNUC_UNUSED GtkCellLayout *cell_layout,
                GtkCellRenderer    *cell,
                GtkTreeModel       *model,
                GtkTreeIter        *iter)
{
    MooHistoryListItem *e = NULL;

    gtk_tree_model_get (model, iter, 0, &e, -1);

    if (e)
    {
        g_object_set (cell, "text", e->display, NULL);
        moo_history_list_item_free (e);
    }
}


static gboolean
row_separator_func (GtkTreeModel       *model,
                    GtkTreeIter        *iter,
                    G_GNUC_UNUSED gpointer data)
{
    gboolean separator;
    MooHistoryListItem *item = NULL;
    gtk_tree_model_get (model, iter, 0, &item, -1);
    separator = item == NULL;
    moo_history_list_item_free (item);
    return separator;
}


static char*
get_text_func (GtkTreeModel       *model,
               GtkTreeIter        *iter,
               G_GNUC_UNUSED gpointer data)
{
    MooHistoryListItem *e = NULL;
    char *text;

    gtk_tree_model_get (model, iter, 0, &e, -1);

    g_return_val_if_fail (e != NULL, NULL);
    g_return_val_if_fail (g_utf8_validate (e->data, -1, NULL), NULL);

    text = g_strdup (e->data);
    moo_history_list_item_free (e);
    return text;
}
