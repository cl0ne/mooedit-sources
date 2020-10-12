/*
 *   moobookmarkmgr.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "moofileview/moobookmarkmgr.h"
#include "moofileview/moofileentry.h"
#include "mooutils/mooprefs.h"
#include "marshals.h"
#include "mooutils/mooactionfactory.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/moostock.h"
#include "mooutils/mootype-macros.h"
#include "moofileview/moobookmark-editor-gxml.h"
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <gtk/gtk.h>
#include <mooglib/moo-glib.h>

#define COLUMN_BOOKMARK MOO_BOOKMARK_MGR_COLUMN_BOOKMARK

struct _MooBookmarkMgrPrivate {
    GtkListStore *store;
    GSList *users;
    GtkWidget *editor;
    gboolean loading;
    guint last_user_id;
    guint update_idle;
};

typedef struct _UserInfo UserInfo;


static void moo_bookmark_mgr_finalize       (GObject        *object);
static void moo_bookmark_mgr_changed        (MooBookmarkMgr *mgr);
static void emit_changed                    (MooBookmarkMgr *mgr);
static void moo_bookmark_mgr_add_separator  (MooBookmarkMgr *mgr);

static void moo_bookmark_mgr_load           (MooBookmarkMgr *mgr);
static void moo_bookmark_mgr_save           (MooBookmarkMgr *mgr);

static void mgr_remove_user                 (MooBookmarkMgr *mgr,
                                             UserInfo       *info);
static gboolean mgr_update_menus            (MooBookmarkMgr *mgr);

static MooBookmark *_moo_bookmark_copy      (MooBookmark    *bookmark);


MOO_DEFINE_BOXED_TYPE_C (MooBookmark, _moo_bookmark)
G_DEFINE_TYPE (MooBookmarkMgr, _moo_bookmark_mgr, G_TYPE_OBJECT)

enum {
    CHANGED,
    ACTIVATE,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void
_moo_bookmark_mgr_class_init (MooBookmarkMgrClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = moo_bookmark_mgr_finalize;

    klass->changed = moo_bookmark_mgr_changed;

    signals[CHANGED] =
            g_signal_new ("changed",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooBookmarkMgrClass, changed),
                          NULL, NULL,
                          _moo_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

    signals[ACTIVATE] =
            g_signal_new ("activate",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooBookmarkMgrClass, activate),
                          NULL, NULL,
                          _moo_marshal_VOID__BOXED_OBJECT,
                          G_TYPE_NONE, 2,
                          MOO_TYPE_BOOKMARK, G_TYPE_OBJECT);
}


static void
_moo_bookmark_mgr_init (MooBookmarkMgr *mgr)
{
    mgr->priv = g_new0 (MooBookmarkMgrPrivate, 1);

    mgr->priv->store = gtk_list_store_new (1, MOO_TYPE_BOOKMARK);

    g_signal_connect_swapped (mgr->priv->store, "row-changed",
                              G_CALLBACK (emit_changed), mgr);
    g_signal_connect_swapped (mgr->priv->store, "rows-reordered",
                              G_CALLBACK (emit_changed), mgr);
    g_signal_connect_swapped (mgr->priv->store, "row-inserted",
                              G_CALLBACK (emit_changed), mgr);
    g_signal_connect_swapped (mgr->priv->store, "row-deleted",
                              G_CALLBACK (emit_changed), mgr);
}


static void
moo_bookmark_mgr_finalize (GObject *object)
{
    GSList *l, *users;
    MooBookmarkMgr *mgr = MOO_BOOKMARK_MGR (object);

    g_object_unref (mgr->priv->store);

    if (mgr->priv->update_idle)
        g_source_remove (mgr->priv->update_idle);

    if (mgr->priv->editor)
    {
        gtk_widget_destroy (mgr->priv->editor);
        g_object_unref (mgr->priv->editor);
    }

    users = g_slist_copy (mgr->priv->users);
    for (l = users; l != NULL; l = l->next)
        mgr_remove_user (mgr, l->data);
    g_assert (mgr->priv->users == NULL);
    g_slist_free (users);

    g_free (mgr->priv);
    mgr->priv = NULL;

    G_OBJECT_CLASS (_moo_bookmark_mgr_parent_class)->finalize (object);
}


static void
emit_changed (MooBookmarkMgr *mgr)
{
    g_signal_emit (mgr, signals[CHANGED], 0);
}


static void
moo_bookmark_mgr_changed (MooBookmarkMgr *mgr)
{
    if (!mgr->priv->loading)
        moo_bookmark_mgr_save (mgr);
    if (!mgr->priv->update_idle)
        mgr->priv->update_idle = g_idle_add((GSourceFunc) mgr_update_menus, mgr);
}


void
_moo_bookmark_mgr_add (MooBookmarkMgr *mgr,
                       MooBookmark    *bookmark)
{
    GtkTreeIter iter;

    g_return_if_fail (MOO_IS_BOOKMARK_MGR (mgr));
    g_return_if_fail (bookmark != NULL);

    /* XXX validate bookmark */

    gtk_list_store_append (mgr->priv->store, &iter);
    gtk_list_store_set (mgr->priv->store, &iter,
                        COLUMN_BOOKMARK, bookmark, -1);
}


static void
moo_bookmark_mgr_add_separator (MooBookmarkMgr *mgr)
{
    GtkTreeIter iter;
    g_return_if_fail (MOO_IS_BOOKMARK_MGR (mgr));
    gtk_list_store_append (mgr->priv->store, &iter);
}


MooBookmarkMgr *
_moo_bookmark_mgr_new (void)
{
    MooBookmarkMgr *mgr = MOO_BOOKMARK_MGR (g_object_new (MOO_TYPE_BOOKMARK_MGR, (const char*) NULL));
    moo_bookmark_mgr_load (mgr);
    return mgr;
}


GtkTreeModel *
_moo_bookmark_mgr_get_model (MooBookmarkMgr *mgr)
{
    g_return_val_if_fail (MOO_IS_BOOKMARK_MGR (mgr), NULL);
    return GTK_TREE_MODEL (mgr->priv->store);
}


MooBookmark*
_moo_bookmark_new (const char     *label,
                   const char     *path,
                   const char     *icon)
{
    MooBookmark *bookmark;

    bookmark = g_new0 (MooBookmark, 1);

    bookmark->path = g_strdup (path);
    bookmark->display_path = path ? g_filename_display_name (path) : NULL;
    bookmark->label = g_strdup (label);
    bookmark->icon_stock_id = g_strdup (icon);

    return bookmark;
}


static MooBookmark*
_moo_bookmark_copy (MooBookmark *bookmark)
{
    MooBookmark *copy;

    g_return_val_if_fail (bookmark != NULL, NULL);

    copy = g_new0 (MooBookmark, 1);

    copy->path = g_strdup (bookmark->path);
    copy->display_path = g_strdup (bookmark->display_path);
    copy->label = g_strdup (bookmark->label);
    copy->icon_stock_id = g_strdup (bookmark->icon_stock_id);
    if (bookmark->pixbuf)
        copy->pixbuf = g_object_ref (bookmark->pixbuf);

    return copy;
}


void
_moo_bookmark_free (MooBookmark *bookmark)
{
    if (bookmark)
    {
        g_free (bookmark->path);
        g_free (bookmark->display_path);
        g_free (bookmark->label);
        g_free (bookmark->icon_stock_id);
        if (bookmark->pixbuf)
            g_object_unref (bookmark->pixbuf);
        g_free (bookmark);
    }
}


#if 0
void
_moo_bookmark_set_path (MooBookmark    *bookmark,
                        const char     *path)
{
    char *display_path;
    g_return_if_fail (bookmark != NULL);
    g_return_if_fail (path != NULL);

    display_path = g_filename_display_name (path);
    g_return_if_fail (display_path != NULL);

    g_free (bookmark->path);
    g_free (bookmark->display_path);
    bookmark->display_path = display_path;
    bookmark->path = g_strdup (path);
}
#endif


static void
_moo_bookmark_set_display_path (MooBookmark  *bookmark,
                                const char   *display_path)
{
    char *path;
    g_return_if_fail (bookmark != NULL);
    g_return_if_fail (display_path != NULL);

    path = g_filename_from_utf8 (display_path, -1, NULL, NULL, NULL);
    g_return_if_fail (path != NULL);

    g_free (bookmark->path);
    g_free (bookmark->display_path);
    bookmark->path = path;
    bookmark->display_path = g_strdup (display_path);
}


/***************************************************************************/
/* Loading and saving
 */
#define BOOKMARKS_ROOT "FileSelector/bookmarks"
#define ELEMENT_BOOKMARK "bookmark"
#define ELEMENT_SEPARATOR "separator"
#define PROP_LABEL "label"
#define PROP_ICON "icon"

static void
moo_bookmark_mgr_load (MooBookmarkMgr *mgr)
{
    MooMarkupNode *xml;
    MooMarkupNode *root, *node;

    xml = moo_prefs_get_markup (MOO_PREFS_RC);
    g_return_if_fail (xml != NULL);

    root = moo_markup_get_element (xml, BOOKMARKS_ROOT);

    if (!root)
        return;

    mgr->priv->loading = TRUE;

    for (node = root->children; node != NULL; node = node->next)
    {
        if (!MOO_MARKUP_IS_ELEMENT (node))
            continue;

        if (!strcmp (node->name, ELEMENT_BOOKMARK))
        {
            MooBookmark *bookmark;
            const char *label = moo_markup_get_prop (node, PROP_LABEL);
            const char *icon = moo_markup_get_prop (node, PROP_ICON);
            const char *path_utf8 = moo_markup_get_content (node);
            char *path;

            if (!path_utf8 || !path_utf8[0])
            {
                g_warning ("empty path in bookmark");
                continue;
            }

            path = g_filename_from_utf8 (path_utf8, -1, NULL, NULL, NULL);

            if (!path)
            {
                g_warning ("could not convert '%s' to filename encoding", path_utf8);
                continue;
            }

            bookmark = _moo_bookmark_new (label ? label : path_utf8, path, icon);
            _moo_bookmark_mgr_add (mgr, bookmark);

            _moo_bookmark_free (bookmark);
            g_free (path);
        }
        else if (!strcmp (node->name, ELEMENT_SEPARATOR))
        {
            moo_bookmark_mgr_add_separator (mgr);
        }
        else
        {
            g_warning ("invalid '%s' element", node->name);
        }
    }

    mgr->priv->loading = FALSE;
}


static void
moo_bookmark_mgr_save (MooBookmarkMgr *mgr)
{
    MooMarkupNode *xml;
    MooMarkupNode *root;
    GtkTreeModel *model;
    GtkTreeIter iter;

    xml = moo_prefs_get_markup (MOO_PREFS_RC);
    g_return_if_fail (xml != NULL);

    model = GTK_TREE_MODEL (mgr->priv->store);

    root = moo_markup_get_element (xml, BOOKMARKS_ROOT);

    if (root)
        moo_markup_delete_node (root);

    if (!gtk_tree_model_get_iter_first (model, &iter))
        return;

    root = moo_markup_create_element (xml, BOOKMARKS_ROOT);
    g_return_if_fail (root != NULL);

    do
    {
        MooBookmark *bookmark = NULL;
        MooMarkupNode *elm;

        gtk_tree_model_get (model, &iter, COLUMN_BOOKMARK, &bookmark, -1);

        if (!bookmark)
        {
            moo_markup_create_element (root, ELEMENT_SEPARATOR);
            continue;
        }

        /* XXX validate bookmark */
        elm = moo_markup_create_text_element (root, ELEMENT_BOOKMARK,
                                              bookmark->display_path);
        moo_markup_set_prop (elm, PROP_LABEL, bookmark->label);
        if (bookmark->icon_stock_id)
            moo_markup_set_prop (elm, PROP_ICON, bookmark->icon_stock_id);

        _moo_bookmark_free (bookmark);
    }
    while (gtk_tree_model_iter_next (model, &iter));
}


/***************************************************************************/
/* Bookmarks menu
 */

struct _UserInfo {
    GObject *user;
    MooUiXml *xml;
    MooActionCollection *actions;
    char *path;
    guint user_id;
    guint merge_id;
    GSList *bm_actions;
};

static UserInfo*
user_info_new (GObject             *user,
               MooActionCollection *actions,
               MooUiXml            *xml,
               const char          *path,
               guint                user_id)
{
    UserInfo *info;

    g_return_val_if_fail (G_IS_OBJECT (user), NULL);
    g_return_val_if_fail (MOO_IS_ACTION_COLLECTION (actions), NULL);
    g_return_val_if_fail (MOO_IS_UI_XML (xml), NULL);
    g_return_val_if_fail (path, NULL);
    g_return_val_if_fail (user_id > 0, NULL);

    info = g_new0 (UserInfo, 1);
    info->user = user;
    info->actions = actions;
    info->xml = xml;
    info->path = g_strdup (path);
    info->user_id = user_id;

    return info;
}


static void
user_info_free (UserInfo *info)
{
    if (info)
    {
        g_free (info->path);
        g_free (info);
    }
}


static void
item_activated (GtkAction      *action,
                MooBookmarkMgr *mgr)
{
    MooBookmark *bookmark;
    gpointer user;

    g_return_if_fail (GTK_IS_ACTION (action));
    g_return_if_fail (MOO_IS_BOOKMARK_MGR (mgr));

    bookmark = g_object_get_data (G_OBJECT (action), "moo-bookmark");
    user = g_object_get_data (G_OBJECT (action), "moo-bookmark-user");

    g_return_if_fail (bookmark != NULL && user != NULL);

    g_signal_emit (mgr, signals[ACTIVATE], 0, bookmark, user);
}


static void
make_menu (MooBookmarkMgr *mgr,
           UserInfo       *info)
{
    GtkTreeModel *model = GTK_TREE_MODEL (mgr->priv->store);
    GtkTreeIter iter;
    GString *markup;
    GtkActionGroup *group;

    if (!gtk_tree_model_get_iter_first (model, &iter))
        return;

    info->merge_id = moo_ui_xml_new_merge_id (info->xml);
    markup = g_string_new (NULL);

    group = moo_action_collection_get_group (info->actions, NULL);

    do
    {
        MooBookmark *bookmark = NULL;
        GtkAction *action;
        char *action_id;

        gtk_tree_model_get (model, &iter, COLUMN_BOOKMARK, &bookmark, -1);

        if (!bookmark)
        {
            g_string_append (markup, "<separator/>");
            continue;
        }

        action_id = g_strdup_printf ("MooBookmarkAction-%p", (gpointer) bookmark);

        action = moo_action_group_add_action (group, action_id,
                                              "label", bookmark->label ? bookmark->label : bookmark->display_path,
                                              "stock-id", bookmark->icon_stock_id,
                                              "tooltip", bookmark->display_path,
                                              "no-accel", TRUE,
                                              NULL);
        g_object_ref (action);
        g_object_set_data_full (G_OBJECT (action), "moo-bookmark",
                                bookmark, (GDestroyNotify) _moo_bookmark_free);
        g_object_set_data (G_OBJECT (action), "moo-bookmark-user", info->user);
        g_signal_connect (action, "activate", G_CALLBACK (item_activated), mgr);

        info->bm_actions = g_slist_prepend (info->bm_actions, action);

        g_string_append_printf (markup, "<item action=\"%s\"/>", action_id);

        g_free (action_id);
    }
    while (gtk_tree_model_iter_next (model, &iter));

    moo_ui_xml_insert_markup (info->xml, info->merge_id,
                              info->path, -1, markup->str);
    g_string_free (markup, TRUE);
}


static void
destroy_menu (UserInfo *info)
{
    GSList *l;

    for (l = info->bm_actions; l != NULL; l = l->next)
    {
        GtkAction *action = l->data;
        moo_action_collection_remove_action (info->actions, action);
        g_object_unref (action);
    }

    g_slist_free (info->bm_actions);
    info->bm_actions = NULL;

    if (info->merge_id > 0)
    {
        moo_ui_xml_remove_ui (info->xml, info->merge_id);
        info->merge_id = 0;
    }
}


static gboolean
mgr_update_menus (MooBookmarkMgr *mgr)
{
    GSList *l;
    GtkTreeIter first;
    GtkTreeModel *model = GTK_TREE_MODEL (mgr->priv->store);
    gboolean empty;

    mgr->priv->update_idle = 0;

    empty = !gtk_tree_model_get_iter_first (model, &first);

    for (l = mgr->priv->users; l != NULL; l = l->next)
    {
        UserInfo *info = l->data;

        destroy_menu (info);

        if (!empty)
            make_menu (mgr, info);
    }

    return FALSE;
}


void
_moo_bookmark_mgr_add_user (MooBookmarkMgr *mgr,
                            gpointer        user,
                            MooActionCollection *actions,
                            MooUiXml       *xml,
                            const char     *path)
{
    UserInfo *info;

    g_return_if_fail (MOO_IS_BOOKMARK_MGR (mgr));
    g_return_if_fail (G_IS_OBJECT (user));
    g_return_if_fail (MOO_IS_ACTION_COLLECTION (actions));
    g_return_if_fail (MOO_IS_UI_XML (xml));
    g_return_if_fail (path != NULL);

    info = user_info_new (user, actions, xml, path,
                          ++mgr->priv->last_user_id);
    mgr->priv->users = g_slist_prepend (mgr->priv->users, info);

    make_menu (mgr, info);
}


static void
mgr_remove_user (MooBookmarkMgr *mgr,
                 UserInfo       *info)
{
    destroy_menu (info);
    user_info_free (info);
    mgr->priv->users = g_slist_remove (mgr->priv->users, info);
}


void
_moo_bookmark_mgr_remove_user (MooBookmarkMgr *mgr,
                               gpointer        user)
{
    GSList *l, *infos = NULL;

    g_return_if_fail (MOO_IS_BOOKMARK_MGR (mgr));

    for (l = mgr->priv->users; l != NULL; l = l->next)
    {
        UserInfo *info = l->data;

        if (info->user == user)
            infos = g_slist_prepend (infos, info);
    }

    for (l = infos; l != NULL; l = l->next)
    {
        UserInfo *info = l->data;
        mgr_remove_user (mgr, info);
    }

    g_slist_free (infos);
}


#if 0
// static void     create_menu_items           (MooBookmarkMgr *mgr,
//                                              GtkMenuShell   *menu,
//                                              int             position,
//                                              MooBookmarkFunc func,
//                                              gpointer        data)
// {
//     GtkTreeIter iter;
//     GtkTreeModel *model = GTK_TREE_MODEL (mgr->priv->store);
//
//     if (!gtk_tree_model_get_iter_first (model, &iter))
//         return;
//
//     do
//     {
//         MooBookmark *bookmark = NULL;
//         GtkWidget *item, *icon;
//
//         gtk_tree_model_get (model, &iter, COLUMN_BOOKMARK, &bookmark, -1);
//
//         if (!bookmark)
//         {
//             item = gtk_separator_menu_item_new ();
//         }
//         else
//         {
//             if (bookmark->label)
//                 item = gtk_image_menu_item_new_with_label (bookmark->label);
//             else
//                 item = gtk_image_menu_item_new ();
//
//             if (bookmark->pixbuf)
//                 icon = gtk_image_new_from_pixbuf (bookmark->pixbuf);
//             else if (bookmark->icon_stock_id)
//                 icon = gtk_image_new_from_stock (bookmark->icon_stock_id,
//                     GTK_ICON_SIZE_MENU);
//             else
//                 icon = NULL;
//
//             if (icon)
//             {
//                 gtk_widget_show (icon);
//                 gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), icon);
//             }
//
//             g_signal_connect (item, "activate",
//                               G_CALLBACK (menu_item_activated), mgr);
//         }
//
//         g_object_set_data_full (G_OBJECT (item), "moo-bookmark-mgr",
//                                 g_object_ref (mgr), g_object_unref);
//         g_object_set_data_full (G_OBJECT (item), "moo-bookmark",
//                                 bookmark, (GDestroyNotify) _moo_bookmark_free);
//         g_object_set_data (G_OBJECT (item), "moo-bookmark-func", func);
//         g_object_set_data (G_OBJECT (item), "moo-bookmark-data", data);
//
//         gtk_widget_show (item);
//         gtk_menu_shell_insert (menu, item, position);
//         if (position >= 0)
//             position++;
//     }
//     while (gtk_tree_model_iter_next (model, &iter));
// }


static void moo_bookmark_mgr_update_menu(GtkMenuShell   *menu,
                                         MooBookmarkMgr *mgr)
{
    MooBookmarkFunc func;
    gpointer data;
    GList *items;
    int position = 0;

    g_return_if_fail (MOO_IS_BOOKMARK_MGR (mgr));
    g_return_if_fail (GTK_IS_MENU_SHELL (menu));

    g_return_if_fail (g_object_get_data (G_OBJECT (menu), "moo-bookmark-mgr") == mgr);

    func = g_object_get_data (G_OBJECT (menu), "moo-bookmark-func");
    data = g_object_get_data (G_OBJECT (menu), "moo-bookmark-data");

    items = gtk_container_get_children (GTK_CONTAINER (menu));

    if (items)
    {
        GList *l;
        GList *my_items = NULL;

        for (l = items; l != NULL; l = l->next)
        {
            if (g_object_get_data (G_OBJECT (l->data), "moo-bookmark-mgr") == mgr)
            {
                my_items = g_list_prepend (my_items, l->data);
                if (position < 0)
                    position = g_list_position (items, l);
            }
        }

        for (l = my_items; l != NULL; l = l->next)
            gtk_container_remove (GTK_CONTAINER (menu), l->data);

        g_list_free (items);
        g_list_free (my_items);
    }

    create_menu_items (mgr, menu, position, func, data);
}
#endif


/***************************************************************************/
/* Bookmark editor
 */

static GtkTreeModel *copy_bookmarks         (GtkListStore   *store);
static void          copy_bookmarks_back    (GtkListStore   *store,
                                             GtkTreeModel   *model);
static void          init_editor_dialog     (BkEditorXml    *xml);
static void          dialog_response        (GtkWidget      *dialog,
                                             int             response,
                                             MooBookmarkMgr *mgr);
static void          dialog_show            (GtkWidget      *dialog,
                                             MooBookmarkMgr *mgr);

GtkWidget *
_moo_bookmark_mgr_get_editor (MooBookmarkMgr *mgr)
{
    GtkWidget *dialog;
    BkEditorXml *xml;

    if (mgr->priv->editor)
        return mgr->priv->editor;

    xml = bk_editor_xml_new ();
    dialog = GTK_WIDGET (xml->BkEditor);

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

    init_editor_dialog (xml);

    g_signal_connect (dialog, "response",
                      G_CALLBACK (dialog_response), mgr);
    g_signal_connect (dialog, "delete-event",
                      G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    g_signal_connect (dialog, "show",
                      G_CALLBACK (dialog_show), mgr);

    mgr->priv->editor = dialog;
    g_object_ref_sink (dialog);

    return dialog;
}


static void
dialog_show (GtkWidget      *dialog,
             MooBookmarkMgr *mgr)
{
    GtkTreeModel *model;
    BkEditorXml *xml;

    xml = bk_editor_xml_get (dialog);
    g_return_if_fail (xml != NULL);

    model = copy_bookmarks (mgr->priv->store);
    gtk_tree_view_set_model (xml->treeview, model);
    g_object_unref (model);
}


static void
dialog_response (GtkWidget      *dialog,
                 int             response,
                 MooBookmarkMgr *mgr)
{
    GtkTreeModel *model;
    BkEditorXml *xml;

    xml = bk_editor_xml_get (dialog);
    g_return_if_fail (xml != NULL);

    if (response != GTK_RESPONSE_OK)
    {
        gtk_widget_hide (dialog);
        return;
    }

    model = gtk_tree_view_get_model (xml->treeview);
    copy_bookmarks_back (mgr->priv->store, model);
    gtk_widget_hide (dialog);
}


static gboolean
copy_value (GtkTreeModel    *src,
            G_GNUC_UNUSED GtkTreePath *path,
            GtkTreeIter     *iter,
            GtkListStore    *dest)
{
    GtkTreeIter dest_iter;
    MooBookmark *bookmark;

    gtk_tree_model_get (src, iter, COLUMN_BOOKMARK, &bookmark, -1);
    gtk_list_store_append (dest, &dest_iter);
    gtk_list_store_set (dest, &dest_iter, COLUMN_BOOKMARK, bookmark, -1);
    _moo_bookmark_free (bookmark);

    return FALSE;
}

static GtkTreeModel *
copy_bookmarks (GtkListStore *store)
{
    GtkListStore *copy;
    copy = gtk_list_store_new (1, MOO_TYPE_BOOKMARK);
    gtk_tree_model_foreach (GTK_TREE_MODEL (store),
                            (GtkTreeModelForeachFunc) copy_value,
                            copy);
    return GTK_TREE_MODEL (copy);
}


static void
copy_bookmarks_back (GtkListStore   *store,
                     GtkTreeModel   *model)
{
    gtk_list_store_clear (store);
    gtk_tree_model_foreach (model,
                            (GtkTreeModelForeachFunc) copy_value,
                            store);
}


static void     icon_data_func      (GtkTreeViewColumn  *column,
                                     GtkCellRenderer    *cell,
                                     GtkTreeModel       *model,
                                     GtkTreeIter        *iter);
static void     label_data_func     (GtkTreeViewColumn  *column,
                                     GtkCellRenderer    *cell,
                                     GtkTreeModel       *model,
                                     GtkTreeIter        *iter);
static void     path_data_func      (GtkTreeViewColumn  *column,
                                     GtkCellRenderer    *cell,
                                     GtkTreeModel       *model,
                                     GtkTreeIter        *iter);

static void     selection_changed   (GtkTreeSelection   *selection,
                                     BkEditorXml        *xml);
static void     new_clicked         (BkEditorXml        *xml);
static void     delete_clicked      (BkEditorXml        *xml);
static void     separator_clicked   (BkEditorXml        *xml);

static void     label_edited        (GtkCellRenderer    *cell,
                                     char               *path,
                                     char               *text,
                                     BkEditorXml        *xml);
static void     path_edited         (GtkCellRenderer    *cell,
                                     char               *path,
                                     char               *text,
                                     BkEditorXml        *xml);
static void     path_editing_started(GtkCellRenderer    *cell,
                                     GtkCellEditable    *editable);

static void     init_icon_combo     (GtkComboBox        *combo,
                                     BkEditorXml        *xml);
static void     combo_update_icon   (GtkComboBox        *combo,
                                     BkEditorXml        *xml);


static void
init_editor_dialog (BkEditorXml *xml)
{
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;
    GtkTreeSelection *selection;
    MooFileEntryCompletion *completion;

    init_icon_combo (xml->icon_combo, xml);

    selection = gtk_tree_view_get_selection (xml->treeview);
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
    g_signal_connect (selection, "changed",
                      G_CALLBACK (selection_changed), xml);
    selection_changed (selection, xml);

    g_signal_connect_swapped (xml->delete_button, "clicked",
                              G_CALLBACK (delete_clicked), xml);
    g_signal_connect_swapped (xml->new_button, "clicked",
                              G_CALLBACK (new_clicked), xml);
    g_signal_connect_swapped (xml->separator_button, "clicked",
                              G_CALLBACK (separator_clicked), xml);

    /* Icon */
    cell = gtk_cell_renderer_pixbuf_new ();
    /* Column label in file selector bookmark editor */
    column = gtk_tree_view_column_new_with_attributes (C_("fileview-bookmark-editor", "Icon"), cell, NULL);
    gtk_tree_view_column_set_cell_data_func (column, cell,
                                             (GtkTreeCellDataFunc) icon_data_func,
                                             NULL, NULL);
    gtk_tree_view_append_column (xml->treeview, column);
    g_object_set_data (G_OBJECT (xml->treeview),
                       "moo-bookmarks-icon-column",
                       column);

    /* Label */
    cell = gtk_cell_renderer_text_new ();
    g_object_set (cell, "editable", TRUE, NULL);
    g_signal_connect (cell, "edited",
                      G_CALLBACK (label_edited), xml);

    /* Column label in file selector bookmark editor */
    column = gtk_tree_view_column_new_with_attributes (C_("fileview-bookmark-editor", "Label"), cell, NULL);
    gtk_tree_view_column_set_cell_data_func (column, cell,
                                             (GtkTreeCellDataFunc) label_data_func,
                                             NULL, NULL);
    gtk_tree_view_append_column (xml->treeview, column);
    g_object_set_data (G_OBJECT (xml->treeview),
                       "moo-bookmarks-label-column",
                       column);

    /* Path */
    cell = gtk_cell_renderer_text_new ();
    g_object_set (cell, "editable", TRUE, NULL);
    g_signal_connect (cell, "edited",
                      G_CALLBACK (path_edited), xml);
    g_signal_connect (cell, "editing-started",
                      G_CALLBACK (path_editing_started), NULL);

    /* Column label in file selector bookmark editor */
    column = gtk_tree_view_column_new_with_attributes (C_("fileview-bookmark-editor", "Path"), cell, NULL);
    gtk_tree_view_column_set_cell_data_func (column, cell,
                                             (GtkTreeCellDataFunc) path_data_func,
                                             NULL, NULL);
    gtk_tree_view_append_column (xml->treeview, column);
    g_object_set_data (G_OBJECT (xml->treeview),
                       "moo-bookmarks-path-column",
                       column);

    completion = MOO_FILE_ENTRY_COMPLETION (
        g_object_new (MOO_TYPE_FILE_ENTRY_COMPLETION,
                      "directories-only", TRUE,
                      "show-hidden", FALSE,
                      (const char*) NULL));
    g_object_set_data_full (G_OBJECT (cell), "moo-file-entry-completion",
                            completion, g_object_unref);
}


static MooBookmark *
get_bookmark (GtkTreeModel       *model,
              GtkTreeIter        *iter)
{
    MooBookmark *bookmark = NULL;
    gtk_tree_model_get (model, iter, COLUMN_BOOKMARK, &bookmark, -1);
    return bookmark;
}


static void
set_bookmark (GtkListStore       *store,
              GtkTreeIter        *iter,
              MooBookmark        *bookmark)
{
    gtk_list_store_set (store, iter, COLUMN_BOOKMARK, bookmark, -1);
}


static void
icon_data_func (G_GNUC_UNUSED GtkTreeViewColumn *column,
                GtkCellRenderer    *cell,
                GtkTreeModel       *model,
                GtkTreeIter        *iter)
{
    MooBookmark *bookmark = get_bookmark (model, iter);

    if (!bookmark)
        g_object_set (cell,
                      "pixbuf", NULL,
                      "stock-id", NULL,
                      NULL);
    else
        g_object_set (cell,
                      "pixbuf", bookmark->pixbuf,
                      "stock-id", bookmark->icon_stock_id,
                      "stock-size", GTK_ICON_SIZE_MENU,
                      NULL);

    _moo_bookmark_free (bookmark);
}


static void
label_data_func (G_GNUC_UNUSED GtkTreeViewColumn *column,
                 GtkCellRenderer    *cell,
                 GtkTreeModel       *model,
                 GtkTreeIter        *iter)
{
    MooBookmark *bookmark = get_bookmark (model, iter);

    if (!bookmark)
        g_object_set (cell,
                      "text", "-------",
                      "editable", FALSE,
                      NULL);
    else
        g_object_set (cell,
                      "text", bookmark->label,
                      "editable", TRUE,
                      NULL);

    _moo_bookmark_free (bookmark);
}


static void
path_data_func (G_GNUC_UNUSED GtkTreeViewColumn *column,
                GtkCellRenderer    *cell,
                GtkTreeModel       *model,
                GtkTreeIter        *iter)
{
    MooBookmark *bookmark = get_bookmark (model, iter);

    if (!bookmark)
        g_object_set (cell,
                      "text", NULL,
                      "editable", FALSE,
                      NULL);
    else
        g_object_set (cell,
                      "text", bookmark->display_path,
                      "editable", TRUE,
                      NULL);

    _moo_bookmark_free (bookmark);
}


static void
selection_changed (GtkTreeSelection *selection,
                   BkEditorXml      *xml)
{
    GtkWidget *selected_hbox;
    int selected;

    selected_hbox = GTK_WIDGET (xml->selected_hbox);
    selected = gtk_tree_selection_count_selected_rows (selection);
    gtk_widget_set_sensitive (GTK_WIDGET (xml->delete_button), selected);

    if (selected == 1)
    {
        GtkTreeIter iter;
        GtkTreeModel *model;
        MooBookmark *bookmark;
        GList *rows = gtk_tree_selection_get_selected_rows (selection, &model);
        g_return_if_fail (rows != NULL);
        gtk_tree_model_get_iter (model, &iter, rows->data);
        bookmark = get_bookmark (model, &iter);
        if (bookmark)
        {
            gtk_widget_set_sensitive (selected_hbox, TRUE);
            combo_update_icon (xml->icon_combo, xml);
            _moo_bookmark_free (bookmark);
        }
        else
        {
            gtk_widget_set_sensitive (selected_hbox, FALSE);
        }
    }
    else
    {
        gtk_widget_set_sensitive (selected_hbox, FALSE);
    }
}


static void
new_clicked (BkEditorXml *xml)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    GtkTreeViewColumn *column;
    GtkListStore *store;
    MooBookmark *bookmark;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (xml->treeview));

    bookmark = _moo_bookmark_new ("New bookmark", NULL,
                                  MOO_STOCK_FOLDER);
    gtk_list_store_append (store, &iter);
    set_bookmark (store, &iter, bookmark);

    column = g_object_get_data (G_OBJECT (xml->treeview),
                                "moo-bookmarks-label-column");
    path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
    gtk_tree_view_set_cursor (xml->treeview, path, column, TRUE);

    g_object_set_data (G_OBJECT (store),
                       "moo-bookmarks-modified",
                       GINT_TO_POINTER (TRUE));

    gtk_tree_path_free (path);
    _moo_bookmark_free (bookmark);
}


static void
delete_clicked (BkEditorXml *xml)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    GtkTreeSelection *selection;
    GtkListStore *store;
    GList *paths, *rows = NULL, *l;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (xml->treeview));

    selection = gtk_tree_view_get_selection (xml->treeview);
    paths = gtk_tree_selection_get_selected_rows (selection, NULL);

    if (!paths)
        return;

    for (l = paths; l != NULL; l = l->next)
        rows = g_list_prepend (rows,
                               gtk_tree_row_reference_new (GTK_TREE_MODEL (store),
                                                           l->data));

    for (l = rows; l != NULL; l = l->next)
    {
        if (gtk_tree_row_reference_valid (l->data))
        {
            path = gtk_tree_row_reference_get_path (l->data);
            gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
            gtk_list_store_remove (store, &iter);
            gtk_tree_path_free (path);
        }
    }

    g_object_set_data (G_OBJECT (store),
                       "moo-bookmarks-modified",
                       GINT_TO_POINTER (TRUE));

    g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
    g_list_foreach (rows, (GFunc) gtk_tree_row_reference_free, NULL);
    g_list_free (paths);
    g_list_free (rows);
}


static void
separator_clicked (BkEditorXml *xml)
{
    GtkTreeIter iter;
    GtkListStore *store;
    store = GTK_LIST_STORE (gtk_tree_view_get_model (xml->treeview));
    gtk_list_store_append (store, &iter);
}


static void
label_edited (G_GNUC_UNUSED GtkCellRenderer *cell,
              char        *path_string,
              char        *text,
              BkEditorXml *xml)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    GtkListStore *store;
    MooBookmark *bookmark;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (xml->treeview));

    path = gtk_tree_path_new_from_string (path_string);

    if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path))
    {
        gtk_tree_path_free (path);
        return;
    }

    bookmark = get_bookmark (GTK_TREE_MODEL (store), &iter);
    g_return_if_fail (bookmark != NULL);

    if (!bookmark->label || strcmp (bookmark->label, text))
    {
        g_free (bookmark->label);
        bookmark->label = g_strdup (text);
        set_bookmark (store, &iter, bookmark);
        g_object_set_data (G_OBJECT (store),
                           "moo-bookmarks-modified",
                           GINT_TO_POINTER (TRUE));
    }

    _moo_bookmark_free (bookmark);
    gtk_tree_path_free (path);
}


static void
path_edited (G_GNUC_UNUSED GtkCellRenderer *cell,
             char        *path_string,
             char        *text,
             BkEditorXml *xml)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    GtkListStore *store;
    MooBookmark *bookmark;
    MooFileEntryCompletion *cmpl;

    store = GTK_LIST_STORE (gtk_tree_view_get_model (xml->treeview));

    path = gtk_tree_path_new_from_string (path_string);

    if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path))
    {
        gtk_tree_path_free (path);
        return;
    }

    bookmark = get_bookmark (GTK_TREE_MODEL (store), &iter);
    g_return_if_fail (bookmark != NULL);

    if (!bookmark->display_path || strcmp (bookmark->display_path, text))
    {
        _moo_bookmark_set_display_path (bookmark, text);
        set_bookmark (store, &iter, bookmark);
        g_object_set_data (G_OBJECT (store),
                           "moo-bookmarks-modified",
                           GINT_TO_POINTER (TRUE));
    }

    _moo_bookmark_free (bookmark);
    gtk_tree_path_free (path);

    cmpl = g_object_get_data (G_OBJECT (cell), "moo-file-entry-completion");
    g_return_if_fail (cmpl != NULL);
    g_object_set (cmpl, "entry", NULL, NULL);
}


static void
path_editing_started (GtkCellRenderer    *cell,
                      GtkCellEditable    *editable)
{
    MooFileEntryCompletion *cmpl =
            g_object_get_data (G_OBJECT (cell), "moo-file-entry-completion");

    g_return_if_fail (cmpl != NULL);
    g_return_if_fail (GTK_IS_ENTRY (editable));

    g_object_set (cmpl, "entry", editable, NULL);

#if 0
    if (!g_object_get_data (G_OBJECT (editable), "moo-stupid-entry-workaround"))
    {
        g_signal_connect (editable, "realize",
                          G_CALLBACK (path_entry_realize), NULL);
        g_signal_connect (editable, "unrealize",
                          G_CALLBACK (path_entry_unrealize), NULL);
        g_object_set_data (G_OBJECT (editable), "moo-stupid-entry-workaround",
                           GINT_TO_POINTER (TRUE));
    }
#endif
}


#if 0
static void
path_entry_realize (GtkWidget *entry)
{
    GtkSettings *settings;
    gboolean value;

    g_return_if_fail (gtk_widget_has_screen (entry));
    settings = gtk_settings_get_for_screen (gtk_widget_get_screen (entry));
    g_return_if_fail (settings != NULL);

    g_object_get (settings, "gtk-entry-select-on-focus", &value, NULL);
    g_object_set (settings, "gtk-entry-select-on-focus", FALSE, NULL);
    g_object_set_data (G_OBJECT (settings),
                       "moo-stupid-entry-workaround",
                       GINT_TO_POINTER (TRUE));
    g_object_set_data (G_OBJECT (settings),
                       "moo-stupid-entry-workaround-value",
                       GINT_TO_POINTER (value));

    gtk_editable_set_position (GTK_EDITABLE (entry), -1);
}


static void
path_entry_unrealize (GtkWidget *entry)
{
    GtkSettings *settings;
    gboolean value;

    g_return_if_fail (gtk_widget_has_screen (entry));
    settings = gtk_settings_get_for_screen (gtk_widget_get_screen (entry));
    g_return_if_fail (settings != NULL);

    if (g_object_get_data (G_OBJECT (settings), "moo-stupid-entry-workaround"))
    {
        value = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (settings),
                                 "moo-stupid-entry-workaround-value"));
        g_object_set (settings, "gtk-entry-select-on-focus", value, NULL);
        g_object_set_data (G_OBJECT (settings), "moo-stupid-entry-workaround", NULL);
    }

    g_signal_handlers_disconnect_by_func (entry,
                                          (gpointer) path_entry_realize,
                                          NULL);
    g_signal_handlers_disconnect_by_func (entry,
                                          (gpointer) path_entry_unrealize,
                                          NULL);
    g_object_set_data (G_OBJECT (entry), "moo-stupid-entry-workaround", NULL);
}
#endif


static void combo_icon_data_func    (GtkCellLayout      *cell_layout,
                                     GtkCellRenderer    *cell,
                                     GtkTreeModel       *model,
                                     GtkTreeIter        *iter);
static void combo_label_data_func   (GtkCellLayout      *cell_layout,
                                     GtkCellRenderer    *cell,
                                     GtkTreeModel       *model,
                                     GtkTreeIter        *iter);
static void fill_icon_store         (GtkListStore       *store,
                                     GtkStyle           *style);
static void icon_store_find_pixbuf  (GtkListStore       *store,
                                     GtkTreeIter        *iter,
                                     GdkPixbuf          *pixbuf);
static void icon_store_find_stock   (GtkListStore       *store,
                                     GtkTreeIter        *iter,
                                     const char         *stock);
static void icon_store_find_empty   (GtkListStore       *store,
                                     GtkTreeIter        *iter);
static void icon_combo_changed      (GtkComboBox        *combo,
                                     BkEditorXml        *xml);

static void
init_icon_combo (GtkComboBox *combo,
                 BkEditorXml *xml)
{
    static GtkListStore *icon_store;
    GtkCellRenderer *cell;

    if (!icon_store)
    {
        GtkWidget *dialog = GTK_WIDGET (xml->BkEditor);
        gtk_widget_ensure_style (dialog);
        icon_store = gtk_list_store_new (3, GDK_TYPE_PIXBUF,
                                         G_TYPE_STRING, G_TYPE_STRING);
        fill_icon_store (icon_store, dialog->style);
    }

    gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
    gtk_combo_box_set_model (combo, GTK_TREE_MODEL (icon_store));
    gtk_combo_box_set_wrap_width (combo, 3);

    cell = gtk_cell_renderer_pixbuf_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, FALSE);
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo), cell,
                                        (GtkCellLayoutDataFunc) combo_icon_data_func,
                                        NULL, NULL);

    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, FALSE);
    gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo), cell,
                                        (GtkCellLayoutDataFunc) combo_label_data_func,
                                        NULL, NULL);

    g_signal_connect (combo, "changed",
                      G_CALLBACK (icon_combo_changed), xml);
}


static void
combo_update_icon (GtkComboBox *combo,
                   BkEditorXml *xml)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GList *rows;
    MooBookmark *bookmark;
    GtkListStore *icon_store;

    selection = gtk_tree_view_get_selection (xml->treeview);
    rows = gtk_tree_selection_get_selected_rows (selection, &model);
    g_return_if_fail (rows != NULL && rows->next == NULL);

    gtk_tree_model_get_iter (model, &iter, rows->data);
    bookmark = get_bookmark (model, &iter);
    g_return_if_fail (bookmark != NULL);

    icon_store = GTK_LIST_STORE (gtk_combo_box_get_model (combo));

    if (bookmark->pixbuf)
        icon_store_find_pixbuf (icon_store, &iter, bookmark->pixbuf);
    else if (bookmark->icon_stock_id)
        icon_store_find_stock (icon_store, &iter, bookmark->icon_stock_id);
    else
        icon_store_find_empty (icon_store, &iter);

    g_signal_handlers_block_by_func (combo, (gpointer) icon_combo_changed, xml);
    gtk_combo_box_set_active_iter (combo, &iter);
    g_signal_handlers_unblock_by_func (combo, (gpointer) icon_combo_changed, xml);

    _moo_bookmark_free (bookmark);
    gtk_tree_path_free (rows->data);
    g_list_free (rows);
}


enum {
    ICON_COLUMN_PIXBUF = 0,
    ICON_COLUMN_STOCK  = 1,
    ICON_COLUMN_LABEL  = 2
};

static void
combo_icon_data_func (G_GNUC_UNUSED GtkCellLayout *cell_layout,
                      GtkCellRenderer    *cell,
                      GtkTreeModel       *model,
                      GtkTreeIter        *iter)
{
    char *stock = NULL;
    GdkPixbuf *pixbuf = NULL;

    gtk_tree_model_get (model, iter, ICON_COLUMN_PIXBUF, &pixbuf, -1);
    g_object_set (cell, "pixbuf", pixbuf, NULL);
    if (pixbuf)
    {
        g_object_unref (pixbuf);
        return;
    }

    gtk_tree_model_get (model, iter, ICON_COLUMN_STOCK, &stock, -1);
    g_object_set (cell, "stock-id", stock,
                  "stock-size", GTK_ICON_SIZE_MENU, NULL);
    g_free (stock);
}


static void
combo_label_data_func (G_GNUC_UNUSED GtkCellLayout *cell_layout,
                       GtkCellRenderer    *cell,
                       GtkTreeModel       *model,
                       GtkTreeIter        *iter)
{
    char *label = NULL;
    gtk_tree_model_get (model, iter, ICON_COLUMN_LABEL, &label, -1);
    g_object_set (cell, "text", label, NULL);
    g_free (label);
}


static void
fill_icon_store (GtkListStore       *store,
                 GtkStyle           *style)
{
    GtkTreeIter iter;
    GSList *stock_ids, *l;

    stock_ids = gtk_stock_list_ids ();

    for (l = stock_ids; l != NULL; l = l->next)
    {
        GtkStockItem item;

        if (!gtk_style_lookup_icon_set (style, l->data))
            continue;

        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, ICON_COLUMN_STOCK,
                            l->data, -1);

        if (gtk_stock_lookup (l->data, &item))
        {
            char *label = g_strdup (item.label);
            char *und = strchr (label, '_');

            if (und)
            {
                if (und[1] == 0)
                    *und = 0;
                else
                    memmove (und, und + 1, strlen (label) - (und - label));
            }

            gtk_list_store_set (store, &iter, ICON_COLUMN_LABEL,
                                label, -1);
            g_free (label);
        }
        else
        {
            gtk_list_store_set (store, &iter, ICON_COLUMN_LABEL,
                                l->data, -1);
        }
    }

    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                          ICON_COLUMN_LABEL,
                                          GTK_SORT_ASCENDING);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                          GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                                          GTK_SORT_ASCENDING);

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, ICON_COLUMN_LABEL, "None", -1);

    g_slist_foreach (stock_ids, (GFunc) g_free, NULL);
    g_slist_free (stock_ids);
}


static void
icon_combo_changed (GtkComboBox *combo,
                    BkEditorXml *xml)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter, icon_iter;
    GList *rows;
    MooBookmark *bookmark;
    GtkTreeModel *icon_model;
    GdkPixbuf *pixbuf = NULL;
    char *stock = NULL;

    selection = gtk_tree_view_get_selection (xml->treeview);
    rows = gtk_tree_selection_get_selected_rows (selection, &model);
    g_return_if_fail (rows != NULL && rows->next == NULL);

    gtk_tree_model_get_iter (model, &iter, rows->data);
    bookmark = get_bookmark (model, &iter);
    g_return_if_fail (bookmark != NULL);

    gtk_combo_box_get_active_iter (combo, &icon_iter);
    icon_model = gtk_combo_box_get_model (combo);

    if (bookmark->pixbuf)
        g_object_unref (bookmark->pixbuf);
    bookmark->pixbuf = NULL;
    g_free (bookmark->icon_stock_id);
    bookmark->icon_stock_id = NULL;

    gtk_tree_model_get (icon_model, &icon_iter, ICON_COLUMN_PIXBUF,
                        &pixbuf, -1);

    if (pixbuf)
        bookmark->pixbuf = pixbuf;

    if (!pixbuf)
    {
        gtk_tree_model_get (icon_model, &icon_iter, ICON_COLUMN_STOCK,
                            &stock, -1);
        bookmark->icon_stock_id = stock;
    }

    set_bookmark (GTK_LIST_STORE (model), &iter, bookmark);

    _moo_bookmark_free (bookmark);
    gtk_tree_path_free (rows->data);
    g_list_free (rows);
}


static void
icon_store_find_pixbuf (GtkListStore       *store,
                        GtkTreeIter        *iter,
                        GdkPixbuf          *pixbuf)
{
    GtkTreeModel *model = GTK_TREE_MODEL (store);

    g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

    if (gtk_tree_model_get_iter_first (model, iter)) do
    {
        GdkPixbuf *pix = NULL;
        gtk_tree_model_get (model, iter, ICON_COLUMN_PIXBUF, &pix, -1);

        if (pix == pixbuf)
        {
            if (pix)
                g_object_unref (pix);
            return;
        }

        if (pix)
            g_object_unref (pix);
    }
    while (gtk_tree_model_iter_next (model, iter));

    gtk_list_store_append (store, iter);
    gtk_list_store_set (store, iter, ICON_COLUMN_PIXBUF, pixbuf, -1);
}


static void
icon_store_find_stock (GtkListStore       *store,
                       GtkTreeIter        *iter,
                       const char         *stock)
{
    GtkTreeModel *model = GTK_TREE_MODEL (store);

    g_return_if_fail (stock != NULL);

    if (gtk_tree_model_get_iter_first (model, iter)) do
    {
        char *id = NULL;
        gtk_tree_model_get (model, iter, ICON_COLUMN_STOCK, &id, -1);

        if (id && !strcmp (id, stock))
        {
            g_free (id);
            return;
        }

        g_free (id);
    }
    while (gtk_tree_model_iter_next (model, iter));

    gtk_list_store_append (store, iter);
    gtk_list_store_set (store, iter, ICON_COLUMN_STOCK, stock, -1);
}


static void
icon_store_find_empty (GtkListStore       *store,
                       GtkTreeIter        *iter)
{
    GtkTreeModel *model = GTK_TREE_MODEL (store);

    if (gtk_tree_model_get_iter_first (model, iter)) do
    {
        char *id = NULL;
        GdkPixbuf *pixbuf = NULL;

        gtk_tree_model_get (model, iter, ICON_COLUMN_STOCK, &id,
                            ICON_COLUMN_PIXBUF, &pixbuf, -1);

        if (!id && !pixbuf)
            return;

        g_free (id);
        if (pixbuf)
            g_object_unref (pixbuf);
    }
    while (gtk_tree_model_iter_next (model, iter));

    gtk_list_store_append (store, iter);
    gtk_list_store_set (store, iter, ICON_COLUMN_LABEL, "None", -1);
}
