/*
 *   mooprefsdialog.c
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
 * class:MooPrefsDialog: (parent GtkDialog) (constructable) (moo.private 1)
 **/

#include "config.h"
#include "marshals.h"
#include "mooutils/mooprefsdialog.h"
#include "mooutils/moodialogs.h"
#include "mooutils/moohelp.h"
#include "mooutils/mooutils-treeview.h"
#include "mooutils/moocompat.h"
#ifdef MOO_ENABLE_HELP
#include "moo-help-sections.h"
#endif


enum {
    ICON_COLUMN,
    ICON_ID_COLUMN,
    LABEL_COLUMN,
    PAGE_COLUMN,
    N_COLUMNS
};


static void moo_prefs_dialog_set_property   (GObject        *object,
                                             guint           prop_id,
                                             const GValue   *value,
                                             GParamSpec     *pspec);
static void moo_prefs_dialog_get_property   (GObject        *object,
                                             guint           prop_id,
                                             GValue         *value,
                                             GParamSpec     *pspec);

static void moo_prefs_dialog_destroy        (GtkObject      *object);
static void moo_prefs_dialog_response       (GtkDialog      *dialog,
                                             int             response);

static void moo_prefs_dialog_apply          (MooPrefsDialog *dialog);

static void setup_pages_list                (MooPrefsDialog *dialog);
static void pages_list_selection_changed    (MooPrefsDialog *dialog,
                                             GtkTreeSelection *selection);

static void init_page                       (MooPrefsPage   *page);

static gboolean moo_prefs_dialog_help       (GtkWidget      *widget);


enum {
    PROP_0,
    PROP_HIDE_ON_DELETE
};

enum {
    INIT,
    APPLY,
    LAST_SIGNAL
};


static G_GNUC_UNUSED guint prefs_dialog_signals[LAST_SIGNAL] = {0};


/* MOO_TYPE_PREFS_DIALOG */
G_DEFINE_TYPE (MooPrefsDialog, moo_prefs_dialog, GTK_TYPE_DIALOG)


static void
moo_prefs_dialog_class_init (MooPrefsDialogClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
    GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

    gobject_class->set_property = moo_prefs_dialog_set_property;
    gobject_class->get_property = moo_prefs_dialog_get_property;
    gtkobject_class->destroy = moo_prefs_dialog_destroy;
    dialog_class->response = moo_prefs_dialog_response;

    klass->apply = moo_prefs_dialog_apply;

    g_object_class_install_property (gobject_class,
                                     PROP_HIDE_ON_DELETE,
                                     g_param_spec_boolean
                                           ("hide_on_delete",
                                            "hide_on_delete",
                                            "Hide on delete",
                                            FALSE,
                                            (GParamFlags) G_PARAM_READWRITE));

    prefs_dialog_signals[APPLY] =
        g_signal_new ("apply",
                      G_OBJECT_CLASS_TYPE (klass),
                      (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                      G_STRUCT_OFFSET (MooPrefsDialogClass, apply),
                      NULL, NULL,
                      _moo_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    prefs_dialog_signals[INIT] =
        g_signal_new ("init",
                      G_OBJECT_CLASS_TYPE (klass),
                      (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                      G_STRUCT_OFFSET (MooPrefsDialogClass, init),
                      NULL, NULL,
                      _moo_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
}


static void
moo_prefs_dialog_init (MooPrefsDialog *dialog)
{
    GtkWidget *hbox, *scrolledwindow, *notebook;

    moo_help_set_func (GTK_WIDGET (dialog), moo_prefs_dialog_help);
    moo_help_connect_keys (GTK_WIDGET (dialog));

    _moo_window_set_remember_size (GTK_WINDOW (dialog), "Dialogs/Preferences", -1, -1, FALSE);

    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                            GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                            GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK, GTK_RESPONSE_OK,
                            NULL);
#if GTK_MINOR_VERSION >= 6
    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_HELP,
                                             GTK_RESPONSE_OK,
                                             GTK_RESPONSE_CANCEL,
                                             GTK_RESPONSE_APPLY,
                                            -1);
#endif /* GTK_MINOR_VERSION >= 6 */
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (GTK_WIDGET (hbox));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 0);

    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledwindow);
    gtk_box_pack_start (GTK_BOX (hbox), scrolledwindow, FALSE, FALSE, 0);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    setup_pages_list (dialog);
    gtk_widget_show (GTK_WIDGET (dialog->pages_list));
    gtk_container_add (GTK_CONTAINER (scrolledwindow), GTK_WIDGET (dialog->pages_list));

    notebook = gtk_notebook_new ();
    gtk_widget_show (notebook);
    gtk_box_pack_start (GTK_BOX (hbox), notebook, TRUE, TRUE, 0);
    GTK_WIDGET_UNSET_CAN_FOCUS (notebook);
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

    dialog->notebook = GTK_NOTEBOOK (notebook);
}


static gboolean
destroy_page (GtkTreeModel  *model,
              G_GNUC_UNUSED GtkTreePath   *path,
              GtkTreeIter   *iter)
{
    GtkWidget *page = NULL;

    gtk_tree_model_get (model, iter, PAGE_COLUMN, &page, -1);
    g_return_val_if_fail (page != NULL, FALSE);

    g_object_set_data (G_OBJECT (page), "moo-prefs-dialog-row", NULL);
    gtk_widget_destroy (page);
    g_object_unref (page);

    return FALSE;
}

static void
moo_prefs_dialog_destroy (GtkObject *object)
{
    MooPrefsDialog *dialog = MOO_PREFS_DIALOG (object);

    if (dialog->store)
    {
        GtkTreeSelection *selection = gtk_tree_view_get_selection (dialog->pages_list);
        g_signal_handlers_disconnect_by_func (selection,
                                              (gpointer) pages_list_selection_changed,
                                              dialog);
        gtk_tree_view_set_model (dialog->pages_list, NULL);
        gtk_tree_model_foreach (GTK_TREE_MODEL (dialog->store),
                                (GtkTreeModelForeachFunc) destroy_page, NULL);
        g_object_unref (dialog->store);
        dialog->store = NULL;
    }

    GTK_OBJECT_CLASS(moo_prefs_dialog_parent_class)->destroy (object);
}


static void
setup_pages_list (MooPrefsDialog *dialog)
{
    GtkWidget *tree;
    GtkCellRenderer *icon_renderer, *label_renderer;
    GtkTreeViewColumn *icon_column, *label_column;
    GtkTreeSelection *selection;

    dialog->store = gtk_list_store_new (N_COLUMNS,
                                        GDK_TYPE_PIXBUF,
                                        G_TYPE_STRING,
                                        G_TYPE_STRING,
                                        MOO_TYPE_PREFS_PAGE);
    tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (dialog->store));
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (tree), FALSE);

    icon_renderer = gtk_cell_renderer_pixbuf_new ();
    icon_column =
        gtk_tree_view_column_new_with_attributes ("Icon",
                                                  icon_renderer,
                                                  "stock-id", ICON_ID_COLUMN,
                                                  "pixbuf", ICON_COLUMN,
                                                  NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), icon_column);

    label_renderer = gtk_cell_renderer_text_new ();
    label_column =
        gtk_tree_view_column_new_with_attributes ("Label",
                                                  label_renderer,
                                                  "markup", LABEL_COLUMN,
                                                  NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), label_column);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    g_signal_connect_swapped (G_OBJECT (selection), "changed",
                              G_CALLBACK (pages_list_selection_changed),
                              dialog);

    dialog->pages_list = GTK_TREE_VIEW (tree);
}


static void
pages_list_selection_changed (MooPrefsDialog *dialog,
                              GtkTreeSelection *selection)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        MooPrefsPage *page = NULL;
        gtk_tree_model_get (model, &iter, PAGE_COLUMN, &page, -1);
        g_return_if_fail (page != NULL);

        init_page (page);

        gtk_notebook_set_current_page (dialog->notebook,
                                       gtk_notebook_page_num (dialog->notebook, GTK_WIDGET (page)));
        g_object_unref (page);
    }
    else
    {
        g_critical ("nothing selected");

        if (gtk_tree_model_get_iter_first (model, &iter))
            gtk_tree_selection_select_iter (selection, &iter);
        else
            g_critical ("list is empty");
    }
}


static gboolean
page_initialized (MooPrefsPage *page)
{
    return g_object_get_data (G_OBJECT (page), "moo-prefs-dialog-page-initialized") != NULL;
}

static void
init_page (MooPrefsPage *page)
{
    if (!g_object_get_data (G_OBJECT (page), "moo-prefs-dialog-page-initialized"))
    {
        g_object_set_data (G_OBJECT (page), "moo-prefs-dialog-page-initialized",
                           GINT_TO_POINTER (1));
        g_signal_emit_by_name (page, "init");
    }
}


static void
moo_prefs_dialog_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
    MooPrefsDialog *dialog = MOO_PREFS_DIALOG (object);
    g_return_if_fail (dialog != NULL);

    switch (prop_id)
    {
        case PROP_HIDE_ON_DELETE:
            dialog->hide_on_delete = g_value_get_boolean (value);
            g_object_notify (object, "hide-on-delete");
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
moo_prefs_dialog_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
    MooPrefsDialog *dialog = MOO_PREFS_DIALOG (object);
    g_return_if_fail (dialog != NULL);

    switch (prop_id)
    {
        case PROP_HIDE_ON_DELETE:
            g_value_set_boolean (value, dialog->hide_on_delete);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


/**************************************************************************/
/* MooPrefsDialog methods
 */

GtkWidget*
moo_prefs_dialog_new (const char *title)
{
    return GTK_WIDGET (g_object_new (MOO_TYPE_PREFS_DIALOG,
                                     "title", title,
                                     (const char*) NULL));
}


void
moo_prefs_dialog_run (MooPrefsDialog *dialog,
                      GtkWidget      *parent)
{
    g_return_if_fail (MOO_IS_PREFS_DIALOG (dialog));

    moo_window_set_parent (GTK_WIDGET (dialog), parent);
    _moo_tree_view_select_first (dialog->pages_list);

    dialog->running = TRUE;
    g_signal_emit_by_name (dialog, "init");
    gtk_window_present (GTK_WINDOW (dialog));
}


static void
moo_prefs_dialog_response (GtkDialog *dialog,
                           int        response)
{
    switch (response)
    {
        case GTK_RESPONSE_HELP:
            moo_help_open (GTK_WIDGET (dialog));
            break;

        case GTK_RESPONSE_APPLY:
            g_signal_emit_by_name (dialog, "apply");
            break;

        case GTK_RESPONSE_OK:
            g_signal_emit_by_name (dialog, "apply");
            /* fallthrough */

        default:
            MOO_PREFS_DIALOG(dialog)->running = FALSE;

            if (MOO_PREFS_DIALOG(dialog)->hide_on_delete)
                gtk_widget_hide (GTK_WIDGET (dialog));
            else
                gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}


static int
get_n_pages (MooPrefsDialog *dialog)
{
    return gtk_notebook_get_n_pages (dialog->notebook);
}

static GtkWidget *
get_nth_page (MooPrefsDialog *dialog,
              int             n)
{
    return gtk_notebook_get_nth_page (dialog->notebook, n);
}


static void
moo_prefs_dialog_apply (MooPrefsDialog *dialog)
{
    int n, i;
    GSList *list = NULL;

    g_return_if_fail (MOO_IS_PREFS_DIALOG (dialog));

    n = get_n_pages (dialog);

    for (i = 0; i < n; ++i)
    {
        GtkWidget *page = get_nth_page (dialog, i);
        list = g_slist_prepend (list, g_object_ref (page));
    }

    list = g_slist_reverse (list);

    while (list)
    {
        MooPrefsPage *page = list->data;

        if (page->auto_apply && page_initialized (page))
            g_signal_emit_by_name (page, "apply");

        g_object_unref (page);
        list = g_slist_delete_link (list, list);
    }
}


void
moo_prefs_dialog_append_page (MooPrefsDialog     *dialog,
                              GtkWidget          *page)
{
    moo_prefs_dialog_insert_page (dialog, page, -1);
}


void
moo_prefs_dialog_insert_page (MooPrefsDialog     *dialog,
                              GtkWidget          *page,
                              int                 position)
{
    char *label = NULL, *icon_id = NULL;
    GdkPixbuf *icon = NULL;
    GtkTreeIter iter;
    GtkTreeRowReference *ref;
    GtkTreePath *path = NULL;

    g_return_if_fail (MOO_IS_PREFS_DIALOG (dialog));
    g_return_if_fail (MOO_IS_PREFS_PAGE (page));
    g_return_if_fail (page->parent == NULL);

    if (position < 0)
        position = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (dialog->store), NULL);

    gtk_list_store_insert (dialog->store, &iter, position);

    gtk_widget_show (page);
    gtk_notebook_insert_page (dialog->notebook, page, NULL, -1);

    g_object_get (page,
                  "label", &label,
                  "icon", &icon,
                  "icon-stock-id", &icon_id,
                  NULL);

    if (!icon_id)
        icon_id = g_strdup (GTK_STOCK_PREFERENCES);

    gtk_list_store_set (dialog->store, &iter,
                        ICON_ID_COLUMN, icon_id,
                        ICON_COLUMN, icon,
                        LABEL_COLUMN, label,
                        PAGE_COLUMN, page,
                        -1);

    path = gtk_tree_model_get_path (GTK_TREE_MODEL (dialog->store), &iter);
    ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (dialog->store), path);
    g_object_set_data_full (G_OBJECT (page), "moo-prefs-dialog-row",
                            ref, (GDestroyNotify) gtk_tree_row_reference_free);

    if (dialog->running)
        g_signal_emit_by_name (page, "init");

    gtk_tree_path_free (path);

    g_free (label);
    g_free (icon_id);

    if (icon)
        g_object_unref (icon);
}


void
moo_prefs_dialog_remove_page (MooPrefsDialog     *dialog,
                              GtkWidget          *page)
{
    GtkTreeRowReference *ref;
    GtkTreePath *path;
    GtkTreeIter iter;

    g_return_if_fail (MOO_IS_PREFS_DIALOG (dialog));
    g_return_if_fail (MOO_IS_PREFS_PAGE (page));

    ref = g_object_get_data (G_OBJECT (page), "moo-prefs-dialog-row");
    g_return_if_fail (ref && gtk_tree_row_reference_valid (ref));

    path = gtk_tree_row_reference_get_path (ref);
    gtk_tree_model_get_iter (GTK_TREE_MODEL (dialog->store), &iter, path);
    gtk_list_store_remove (dialog->store, &iter);

    g_object_set_data (G_OBJECT (page), "moo-prefs-dialog-row", NULL);
    gtk_tree_path_free (path);

    gtk_notebook_remove_page (dialog->notebook,
                              gtk_notebook_page_num (dialog->notebook, page));
}


static gboolean
moo_prefs_dialog_help (GtkWidget *widget)
{
    int index;
    GtkWidget *current_page = NULL;
    MooPrefsDialog *dialog = MOO_PREFS_DIALOG (widget);

    if ((index = gtk_notebook_get_current_page (dialog->notebook)) >= 0)
        current_page = gtk_notebook_get_nth_page (dialog->notebook, index);

    if (!current_page || !moo_help_open (current_page))
    {
#ifdef MOO_ENABLE_HELP
        moo_help_open_id (HELP_SECTION_PREFS_DIALOG, widget);
#endif
    }

    return TRUE;
}
