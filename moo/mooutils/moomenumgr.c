/*
 *   moomenumgr.c
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
 * class:MooMenuMgr: (parent GObject) (constructable) (moo.private 1)
 **/

#include "mooutils/moomenumgr.h"
#include "marshals.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/moolist.h"
#include <gtk/gtk.h>
#include <string.h>


typedef struct {
    char *id;
    char *label;
    char *tip;
    gpointer data;
    GDestroyNotify destroy;
    MooMenuItemFlags flags;
} Item;

typedef struct {
    GtkWidget *top_widget; /* GtkMenu or GtkMenuItem */
    GHashTable *items; /* Item* -> GtkMenuItem* */
    /* TODO: it breaks if last added radio item is deleted */
    GSList *radio_group;
    gpointer data;
    GDestroyNotify destroy;
} Menu;

MOO_DEFINE_SLIST(MenuList, menu_list, Menu)

struct _MooMenuMgrPrivate {
    GSList *top_nodes; /* GNode* */
    GHashTable *named_nodes; /* char* -> GNode* */
    MenuList *menus;
    Item *active_item;
    guint use_mnemonic : 1;
    guint show_tooltips : 1;
    guint frozen : 1;
    guint active_item_removed : 1;
};


static void         moo_menu_mgr_finalize   (GObject    *object);

static Item        *item_new                (const char         *id,
                                             const char         *label,
                                             const char         *tip,
                                             gpointer            data,
                                             GDestroyNotify      destroy,
                                             MooMenuItemFlags    flags);
static void         item_free               (Item               *item);

static Menu        *menu_new                (gpointer            data,
                                             GDestroyNotify      destroy);
static void         menu_free               (Menu               *menu);

static int          mgr_insert              (MooMenuMgr         *mgr,
                                             GNode              *parent_node,
                                             int                 position,
                                             Item               *item);
static void         mgr_remove              (MooMenuMgr         *mgr,
                                             GNode              *node);

static GtkWidget   *menu_item_new           (MooMenuMgr         *mgr,
                                             Menu               *menu,
                                             Item               *item);
static void         construct_menus         (MooMenuMgr         *mgr,
                                             Menu               *menu,
                                             GNode              *node,
                                             GtkWidget          *menu_shell);
static void         object_set_data         (GObject            *object,
                                             MooMenuMgr         *mgr,
                                             Menu               *menu,
                                             Item               *item);
static MooMenuMgr  *object_get_mgr          (GObject            *object);
static Menu        *object_get_menu         (GObject            *object);
static Item        *object_get_item         (GObject            *object);

static gboolean     cleanup_node            (GNode              *node,
                                             MooMenuMgr         *mgr);

static void         ensure_active_item      (MooMenuMgr         *mgr);

static void         emit_radio_set_active   (MooMenuMgr         *mgr,
                                             Item               *item);
static void         emit_toggle_set_active  (MooMenuMgr         *mgr,
                                             Item               *item,
                                             gboolean            active);
static void         emit_item_activated     (MooMenuMgr         *mgr,
                                             Item               *item,
                                             Menu               *menu);

static void         check_item_toggled      (GtkCheckMenuItem   *menu_item,
                                             MooMenuMgr         *mgr);
static void         radio_item_toggled      (GtkCheckMenuItem   *menu_item,
                                             MooMenuMgr         *mgr);
static void         item_activated          (GtkWidget          *menu_item,
                                             MooMenuMgr         *mgr);
static void         top_widget_destroyed    (GtkWidget          *widget);


enum {
    RADIO_SET_ACTIVE,
    TOGGLE_SET_ACTIVE,
    ITEM_ACTIVATED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


/* MOO_TYPE_MENU_MGR */
G_DEFINE_TYPE (MooMenuMgr, moo_menu_mgr, G_TYPE_OBJECT)


static void
moo_menu_mgr_class_init (MooMenuMgrClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = moo_menu_mgr_finalize;

    g_type_class_add_private (klass, sizeof (MooMenuMgrPrivate));

    signals[RADIO_SET_ACTIVE] =
            g_signal_new ("radio-set-active",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooMenuMgrClass, radio_set_active),
                          NULL, NULL,
                          _moo_marshal_VOID__POINTER,
                          G_TYPE_NONE, 1,
                          G_TYPE_POINTER);

    signals[TOGGLE_SET_ACTIVE] =
            g_signal_new ("toggle-set-active",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooMenuMgrClass, toggle_set_active),
                          NULL, NULL,
                          _moo_marshal_VOID__POINTER_BOOLEAN,
                          G_TYPE_NONE, 2,
                          G_TYPE_POINTER, G_TYPE_BOOLEAN);

    signals[ITEM_ACTIVATED] =
            g_signal_new ("item-activated",
                          G_OBJECT_CLASS_TYPE (klass),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MooMenuMgrClass, item_activated),
                          NULL, NULL,
                          _moo_marshal_VOID__POINTER_POINTER,
                          G_TYPE_NONE, 2,
                          G_TYPE_POINTER, G_TYPE_POINTER);
}


static void
moo_menu_mgr_init (MooMenuMgr *mgr)
{
    mgr->priv = G_TYPE_INSTANCE_GET_PRIVATE (mgr, MOO_TYPE_MENU_MGR, MooMenuMgrPrivate);
    mgr->priv->named_nodes =
            g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    mgr->priv->use_mnemonic = TRUE;
}


static void
moo_menu_mgr_finalize (GObject *object)
{
    GSList *l;
    MooMenuMgr *mgr = MOO_MENU_MGR (object);

    if (mgr->priv->menus)
    {
        g_critical ("oops");
        menu_list_foreach (mgr->priv->menus, (MenuListFunc) menu_free, NULL);
        menu_list_free_links (mgr->priv->menus);
    }

    for (l = mgr->priv->top_nodes; l != NULL; l = l->next)
    {
        GNode *node = (GNode*) l->data;
        g_node_traverse (node, G_IN_ORDER, G_TRAVERSE_ALL, -1,
                         (GNodeTraverseFunc) cleanup_node, NULL);
        g_node_destroy (node);
    }

    g_hash_table_destroy (mgr->priv->named_nodes);

    G_OBJECT_CLASS(moo_menu_mgr_parent_class)->finalize (object);
}


static GtkWidget*
menu_item_get_submenu (GtkWidget *item)
{
    GtkWidget *menu;

    g_return_val_if_fail (GTK_IS_MENU_ITEM (item), NULL);

    menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (item));

    if (!menu)
    {
        menu = gtk_menu_new ();
        gtk_widget_show (menu);
        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
    }

    return menu;
}


static int
mgr_insert (MooMenuMgr         *mgr,
            GNode              *parent_node,
            int                 position,
            Item               *item)
{
    GNode *node;
    MenuList *l;

    if (parent_node)
    {
        node = g_node_insert_data (parent_node, position, item);
        position = g_node_child_position (parent_node, node);
    }
    else
    {
        node = g_node_new (item);
        mgr->priv->top_nodes = g_slist_insert (mgr->priv->top_nodes, node, position);
        position = g_slist_index (mgr->priv->top_nodes, node);
    }

    if (item->id)
        g_hash_table_insert (mgr->priv->named_nodes,
                             g_strdup (item->id), node);

    for (l = mgr->priv->menus; l != NULL; l = l->next)
    {
        Menu *menu = l->data;
        GtkWidget *parent_item = NULL, *menu_shell;
        GtkWidget *menu_item;

        if (parent_node)
        {
            parent_item = (GtkWidget*) g_hash_table_lookup (menu->items, parent_node->data);
            g_return_val_if_fail (GTK_IS_MENU_ITEM (parent_item), -1);
            menu_shell = menu_item_get_submenu (parent_item);
        }
        else
        {
            if (GTK_IS_MENU_SHELL (menu->top_widget))
                menu_shell = menu->top_widget;
            else
                menu_shell = menu_item_get_submenu (menu->top_widget);
        }

        g_return_val_if_fail (menu_shell != NULL, -1);
        menu_item = menu_item_new (mgr, menu, item);
        g_return_val_if_fail (menu_item != NULL, -1);

        gtk_menu_shell_insert (GTK_MENU_SHELL (menu_shell), menu_item, position);
    }

    return position;
}


static gboolean
collect_items (GNode   *node,
               GSList **list)
{
    *list = g_slist_prepend (*list, node->data);
    return FALSE;
}

static void
mgr_remove (MooMenuMgr         *mgr,
            GNode              *node)
{
    MenuList *lm, *menus, *menus_to_free = NULL;
    GSList *li, *items_to_free = NULL;
    Item *item = (Item*) node->data;

    g_object_ref (mgr);
    mgr->priv->frozen = TRUE;

    menus = menu_list_copy_links (mgr->priv->menus);

    for (lm = menus; lm != NULL; lm = lm->next)
    {
        Menu *menu = lm->data;
        GtkWidget *menu_item;

        menu_item = (GtkWidget*) g_hash_table_lookup (menu->items, item);
        g_return_if_fail (menu_item != NULL);

        gtk_widget_destroy (menu_item);

        if (menu_item == menu->top_widget)
        {
            menus_to_free = menu_list_prepend (menus_to_free, menu);
            mgr->priv->menus = menu_list_remove (mgr->priv->menus, menu);
        }
    }

    g_node_traverse (node, G_POST_ORDER, G_TRAVERSE_ALL, -1,
                     (GNodeTraverseFunc) collect_items, &items_to_free);
    g_node_destroy (node);
    mgr->priv->top_nodes = g_slist_remove (mgr->priv->top_nodes, node);

    for (li = items_to_free; li != NULL; li = li->next)
    {
        Item *child_item = (Item*) li->data;
        if (child_item->id)
            g_hash_table_remove (mgr->priv->named_nodes, child_item->id);
    }

    mgr->priv->frozen = FALSE;

    for (li = items_to_free; li != NULL; li = li->next)
    {
        if (mgr->priv->active_item == li->data)
        {
            mgr->priv->active_item_removed = TRUE;
            mgr->priv->active_item = NULL;
        }

        item_free ((Item*) li->data);
    }

    for (lm = menus_to_free; lm != NULL; lm = lm->next)
        menu_free (lm->data);

    g_slist_free (items_to_free);
    menu_list_free_links (menus_to_free);
    menu_list_free_links (menus);

    ensure_active_item (mgr);

    g_object_unref (mgr);
}


static void
ensure_active_item (MooMenuMgr         *mgr)
{
    MenuList *l;
    gboolean need_signal = FALSE;

    if (!mgr->priv->active_item)
        return;

    for (l = mgr->priv->menus; l != NULL; l = l->next)
    {
        Menu *menu = l->data;
        GtkWidget *menu_item;

        menu_item = (GtkWidget*) g_hash_table_lookup (menu->items,
                                                      mgr->priv->active_item);
        g_return_if_fail (GTK_IS_RADIO_MENU_ITEM (menu_item));

        if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menu_item)))
        {
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
            need_signal = TRUE;
        }
    }

    if (need_signal)
        emit_radio_set_active (mgr, mgr->priv->active_item);
}


static GtkWidget*
menu_item_new (MooMenuMgr         *mgr,
               Menu               *menu,
               Item               *item)
{
    GtkWidget *menu_item;
    gboolean use_mnemonic = mgr->priv->use_mnemonic;

    if (item->flags & MOO_MENU_ITEM_USE_MNEMONIC)
        use_mnemonic = TRUE;
    else if (item->flags & MOO_MENU_ITEM_DONT_USE_MNEMONIC)
        use_mnemonic = FALSE;

    if (item->flags & MOO_MENU_ITEM_SEPARATOR)
    {
        menu_item = gtk_separator_menu_item_new ();
    }
    else if (item->flags & MOO_MENU_ITEM_RADIO)
    {
        if (use_mnemonic)
            menu_item = gtk_radio_menu_item_new_with_mnemonic (menu->radio_group, item->label);
        else
            menu_item = gtk_radio_menu_item_new_with_label (menu->radio_group, item->label);
        menu->radio_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_item));
        g_signal_connect (menu_item, "toggled", G_CALLBACK (radio_item_toggled), mgr);
    }
    else if (item->flags & MOO_MENU_ITEM_TOGGLE)
    {
        if (use_mnemonic)
            menu_item = gtk_check_menu_item_new_with_mnemonic (item->label);
        else
            menu_item = gtk_check_menu_item_new_with_label (item->label);

        g_signal_connect (menu_item, "toggled", G_CALLBACK (check_item_toggled), mgr);
    }
    else
    {
        if (use_mnemonic)
            menu_item = gtk_menu_item_new_with_mnemonic (item->label);
        else
            menu_item = gtk_menu_item_new_with_label (item->label);

        if (item->flags & MOO_MENU_ITEM_ACTIVATABLE)
            g_signal_connect (menu_item, "activate", G_CALLBACK (item_activated), mgr);
    }

    if (mgr->priv->show_tooltips && item->tip)
        _moo_widget_set_tooltip (menu_item, item->tip);

    gtk_widget_show (menu_item);
    object_set_data (G_OBJECT (menu_item), mgr, menu, item);
    g_hash_table_insert (menu->items, item, menu_item);

    return menu_item;
}


static void
object_set_data (GObject            *object,
                 MooMenuMgr         *mgr,
                 Menu               *menu,
                 Item               *item)
{
    if (mgr)
        g_object_set_data_full (object, "moo-menu-mgr",
                                g_object_ref (mgr), g_object_unref);
    else
        g_object_set_data (object, "moo-menu-mgr", NULL);

    g_object_set_data (object, "moo-menu-mgr-menu", menu);
    g_object_set_data (object, "moo-menu-mgr-item", item);
}

static MooMenuMgr*
object_get_mgr (GObject *object)
{
    return (MooMenuMgr*) g_object_get_data (object, "moo-menu-mgr");
}

static Menu*
object_get_menu (GObject *object)
{
    return (Menu*) g_object_get_data (object, "moo-menu-mgr-menu");
}

static Item*
object_get_item (GObject *object)
{
    return (Item*) g_object_get_data (object, "moo-menu-mgr-item");
}


static Menu*
menu_new (gpointer            data,
          GDestroyNotify      destroy)
{
    Menu *menu = g_new0 (Menu, 1);

    menu->items = g_hash_table_new (g_direct_hash, g_direct_equal);
    menu->data = data;
    menu->destroy = destroy;

    return menu;
}


static void
menu_free (Menu               *menu)
{
    if (menu)
    {
        g_hash_table_destroy (menu->items);
        if (menu->destroy)
            menu->destroy (menu->data);
        g_free (menu);
    }
}


GtkWidget*
moo_menu_mgr_create_item (MooMenuMgr         *mgr,
                          const char         *label,
                          MooMenuItemFlags    flags,
                          gpointer            user_data,
                          GDestroyNotify      destroy)
{
    Menu *menu;
    GtkWidget *menu_shell;
    gboolean make_item;

    g_return_val_if_fail (MOO_IS_MENU_MGR (mgr), NULL);
    g_return_val_if_fail (mgr->priv->top_nodes || label, NULL);

    menu = menu_new (user_data, destroy);
    mgr->priv->menus = menu_list_prepend (mgr->priv->menus, menu);

    make_item = (label == NULL && mgr->priv->top_nodes && !mgr->priv->top_nodes->next);

    if (!make_item)
    {
        menu_shell = menu->top_widget = gtk_menu_new ();
        gtk_widget_show (menu->top_widget);
        object_set_data (G_OBJECT (menu->top_widget), mgr, menu, NULL);
    }
    else
    {
        GNode *top_node = (GNode*) mgr->priv->top_nodes->data;
        Item *item = (Item*) top_node->data;
        menu->top_widget = menu_item_new (mgr, menu, item);
        menu_shell = menu_item_get_submenu (menu->top_widget);
    }

    g_signal_connect (menu->top_widget, "destroy",
                      G_CALLBACK (top_widget_destroyed), NULL);

    if (!make_item)
    {
        GSList *l;

        for (l = mgr->priv->top_nodes; l != NULL; l = l->next)
        {
            GNode *node = (GNode*) l->data;
            construct_menus (mgr, menu, node, menu_shell);
        }
    }
    else
    {
        GNode *top_node = (GNode*) mgr->priv->top_nodes->data;
        GNode *node;

        for (node = top_node->children; node != NULL; node = node->next)
            construct_menus (mgr, menu, node, menu_shell);
    }

    if (mgr->priv->active_item)
    {
        GtkCheckMenuItem *active_item = (GtkCheckMenuItem*)
            g_hash_table_lookup (menu->items, mgr->priv->active_item);
        gtk_check_menu_item_set_active (active_item, TRUE);
    }

    if (!make_item)
    {
        GtkWidget *item;
        gboolean use_mnemonic = mgr->priv->use_mnemonic;

        if (flags & MOO_MENU_ITEM_USE_MNEMONIC)
            use_mnemonic = TRUE;
        else if (flags & MOO_MENU_ITEM_DONT_USE_MNEMONIC)
            use_mnemonic = FALSE;

        if (use_mnemonic)
            item = gtk_menu_item_new_with_mnemonic (label);
        else
            item = gtk_menu_item_new_with_label (label);

        gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu_shell);
        return item;
    }
    else
    {
        return menu->top_widget;
    }
}


static void
top_widget_destroyed (GtkWidget          *widget)
{
    MooMenuMgr *mgr = object_get_mgr (G_OBJECT (widget));
    Menu *menu = object_get_menu (G_OBJECT (widget));

    g_return_if_fail (MOO_IS_MENU_MGR (mgr));
    g_return_if_fail (menu != NULL && menu->top_widget == widget);

    mgr->priv->menus = menu_list_remove (mgr->priv->menus, menu);
    menu_free (menu);
}


static void
construct_menus (MooMenuMgr         *mgr,
                 Menu               *menu,
                 GNode              *node,
                 GtkWidget          *menu_shell)
{
    Item *item;
    GtkWidget *menu_item;

    g_return_if_fail (node != NULL);
    g_return_if_fail (menu != NULL);
    g_return_if_fail (GTK_IS_MENU_SHELL (menu_shell));

    item = (Item*) node->data;

    menu_item = menu_item_new (mgr, menu, item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_shell), menu_item);

    if (node->children)
    {
        GtkWidget *child_menu_shell;
        GNode *child;

        child_menu_shell = menu_item_get_submenu (menu_item);

        for (child = node->children; child != NULL; child = child->next)
            construct_menus (mgr, menu, child, child_menu_shell);
    }
}


static Item*
item_new (const char         *id,
          const char         *label,
          const char         *tip,
          gpointer            data,
          GDestroyNotify      destroy,
          MooMenuItemFlags    flags)
{
    Item *item = g_new0 (Item, 1);

    item->id = g_strdup (id);
    item->label = g_strdup (label);
    item->tip = g_strdup (tip);
    item->data = data;
    item->destroy = destroy;
    item->flags = flags;

    return item;
}


static void
item_free (Item               *item)
{
    if (item)
    {
        g_free (item->id);
        g_free (item->label);
        g_free (item->tip);
        if (item->destroy)
            item->destroy (item->data);
        g_free (item);
    }
}


static gboolean
cleanup_node (GNode              *node,
              MooMenuMgr         *mgr)
{
    Item *item;

    g_return_val_if_fail (node != NULL, FALSE);

    item = (Item*) node->data;

    if (mgr && item->id)
        g_hash_table_remove (mgr->priv->named_nodes, item->id);

    item_free (item);
    return FALSE;
}


static void
check_item_toggled (GtkCheckMenuItem   *menu_item,
                    MooMenuMgr         *mgr)
{
    MenuList *l;
    gboolean active = gtk_check_menu_item_get_active (menu_item);
    Menu *menu = object_get_menu (G_OBJECT (menu_item));
    Item *item = object_get_item (G_OBJECT (menu_item));

    g_return_if_fail (MOO_IS_MENU_MGR (mgr));
    g_return_if_fail (menu != NULL && item != NULL);
    g_return_if_fail (g_hash_table_lookup (menu->items, item) == menu_item);

    for (l = mgr->priv->menus; l != NULL; l = l->next)
    {
        GtkCheckMenuItem *m_item;
        Menu *m = l->data;

        if (m == menu)
            continue;

        m_item = (GtkCheckMenuItem*) g_hash_table_lookup (m->items, item);
        g_return_if_fail (m_item != NULL);

        g_signal_handlers_block_by_func (m_item, (gpointer) check_item_toggled, mgr);
        gtk_check_menu_item_set_active (m_item, active);
        g_signal_handlers_unblock_by_func (m_item, (gpointer) check_item_toggled, mgr);
    }

    emit_toggle_set_active (mgr, item, active);
}


static void
radio_item_toggled (GtkCheckMenuItem   *menu_item,
                    MooMenuMgr         *mgr)
{
    MenuList *l;
    Menu *menu = object_get_menu (G_OBJECT (menu_item));
    Item *item = object_get_item (G_OBJECT (menu_item));

    g_return_if_fail (MOO_IS_MENU_MGR (mgr));
    g_return_if_fail (menu != NULL && item != NULL);
    g_return_if_fail (g_hash_table_lookup (menu->items, item) == menu_item);

    if (!gtk_check_menu_item_get_active (menu_item))
    {
        if (mgr->priv->active_item == item)
            mgr->priv->active_item = NULL;
        return;
    }

    mgr->priv->active_item = item;

    if (mgr->priv->frozen)
        return;

    for (l = mgr->priv->menus; l != NULL; l = l->next)
    {
        GtkCheckMenuItem *m_item;
        Menu *m = l->data;

        if (m == menu)
            continue;

        m_item = (GtkCheckMenuItem*) g_hash_table_lookup (m->items, item);
        g_return_if_fail (m_item != NULL);

        g_signal_handlers_block_by_func (m_item, (gpointer) radio_item_toggled, mgr);
        gtk_check_menu_item_set_active (m_item, TRUE);
        g_signal_handlers_unblock_by_func (m_item, (gpointer) radio_item_toggled, mgr);
    }

    emit_radio_set_active (mgr, item);
}


void
moo_menu_mgr_set_active (MooMenuMgr         *mgr,
                         const char         *item_id,
                         gboolean            active)
{
    GNode *node;
    Item *item;
    GCallback callback;
    MenuList *l;

    g_return_if_fail (MOO_IS_MENU_MGR (mgr));
    g_return_if_fail (item_id != NULL);

    node = (GNode*) g_hash_table_lookup (mgr->priv->named_nodes, item_id);
    g_return_if_fail (node != NULL);

    item = (Item*) node->data;
    g_return_if_fail (item->flags & (MOO_MENU_ITEM_TOGGLE | MOO_MENU_ITEM_RADIO));
    g_return_if_fail (!(item->flags & MOO_MENU_ITEM_RADIO) || active);

    if (item->flags & MOO_MENU_ITEM_RADIO)
    {
        mgr->priv->active_item = item;

        if (mgr->priv->frozen)
            return;

        callback = (GCallback) radio_item_toggled;
    }
    else
    {
        callback = (GCallback) check_item_toggled;
    }

    for (l = mgr->priv->menus; l != NULL; l = l->next)
    {
        GtkCheckMenuItem *m_item;
        Menu *m = l->data;

        m_item = (GtkCheckMenuItem*) g_hash_table_lookup (m->items, item);
        g_return_if_fail (m_item != NULL);

        g_signal_handlers_block_by_func (m_item, (gpointer) callback, mgr);
        gtk_check_menu_item_set_active (m_item, active);
        g_signal_handlers_unblock_by_func (m_item, (gpointer) callback, mgr);
    }

    if (item->flags & MOO_MENU_ITEM_RADIO)
        emit_radio_set_active (mgr, item);
    else
        emit_toggle_set_active (mgr, item, active);
}


static void
item_activated (GtkWidget          *menu_item,
                MooMenuMgr         *mgr)
{
    Menu *menu = object_get_menu (G_OBJECT (menu_item));
    Item *item = object_get_item (G_OBJECT (menu_item));

    g_return_if_fail (MOO_IS_MENU_MGR (mgr));
    g_return_if_fail (menu != NULL && item != NULL);
    g_return_if_fail (g_hash_table_lookup (menu->items, item) == menu_item);

    emit_item_activated (mgr, item, menu);
}


static void
emit_radio_set_active (MooMenuMgr         *mgr,
                       Item               *item)
{
    g_return_if_fail (MOO_IS_MENU_MGR (mgr));
    g_return_if_fail (item && (item->flags & MOO_MENU_ITEM_RADIO));
    g_signal_emit (mgr, signals[RADIO_SET_ACTIVE], 0, item->data);
}


static void
emit_toggle_set_active (MooMenuMgr         *mgr,
                        Item               *item,
                        gboolean            active)
{
    g_return_if_fail (MOO_IS_MENU_MGR (mgr));
    g_return_if_fail (item && (item->flags & MOO_MENU_ITEM_TOGGLE));
    g_signal_emit (mgr, signals[TOGGLE_SET_ACTIVE], 0, item->data, active);
}


static void
emit_item_activated (MooMenuMgr         *mgr,
                     Item               *item,
                     Menu               *menu)
{
    g_return_if_fail (MOO_IS_MENU_MGR (mgr));
    g_return_if_fail (item != NULL && menu != NULL);
    g_signal_emit (mgr, signals[ITEM_ACTIVATED], 0, item->data, menu->data);
}


int
moo_menu_mgr_insert (MooMenuMgr         *mgr,
                     const char         *parent_id,
                     int                 position,
                     const char         *item_id,
                     const char         *label,
                     const char         *tip,
                     MooMenuItemFlags    flags,
                     gpointer            data,
                     GDestroyNotify      destroy)
{
    Item *item;
    GNode *parent_node = NULL;

    g_return_val_if_fail (MOO_IS_MENU_MGR (mgr), -1);
    g_return_val_if_fail (flags & MOO_MENU_ITEM_SEPARATOR || label != NULL, -1);
    g_return_val_if_fail (!label || g_utf8_validate (label, -1, NULL), -1);

    if (parent_id)
    {
        parent_node = (GNode*) g_hash_table_lookup (mgr->priv->named_nodes, parent_id);
        g_return_val_if_fail (parent_node != NULL, -1);
    }

    item = item_new (item_id, label, tip, data, destroy, flags);
    return mgr_insert (mgr, parent_node, position, item);
}


void
moo_menu_mgr_remove (MooMenuMgr         *mgr,
                     const char         *parent_id,
                     guint               position)
{
    GNode *parent_node = NULL, *node;

    g_return_if_fail (MOO_IS_MENU_MGR (mgr));

    if (parent_id)
    {
        parent_node = (GNode*) g_hash_table_lookup (mgr->priv->named_nodes, parent_id);
        g_return_if_fail (parent_node != NULL);
    }

    if (parent_node)
        node = g_node_nth_child (parent_node, position);
    else
        node = (GNode*) g_slist_nth_data (mgr->priv->top_nodes, position);

    g_return_if_fail (node != NULL);
    mgr_remove (mgr, node);
}


#if 0
void
moo_menu_mgr_remove_named (MooMenuMgr         *mgr,
                           const char         *item_id)
{
    GNode *node;

    g_return_if_fail (MOO_IS_MENU_MGR (mgr));
    g_return_if_fail (item_id != NULL);

    node = g_hash_table_lookup (mgr->priv->named_nodes, item_id);
    g_return_if_fail (node != NULL);

    mgr_remove (mgr, node);
}
#endif


int
moo_menu_mgr_insert_separator (MooMenuMgr         *mgr,
                               const char         *parent_id,
                               int                 position)
{
    return moo_menu_mgr_insert (mgr, parent_id, position,
                                NULL, NULL, NULL,
                                MOO_MENU_ITEM_SEPARATOR,
                                NULL, NULL);
}


void
moo_menu_mgr_set_use_mnemonic (MooMenuMgr *mgr,
                               gboolean    use)
{
    g_return_if_fail (MOO_IS_MENU_MGR (mgr));
    mgr->priv->use_mnemonic = use != 0;
}


void
moo_menu_mgr_set_show_tooltips (MooMenuMgr *mgr,
                                gboolean    show)
{
    g_return_if_fail (MOO_IS_MENU_MGR (mgr));
    mgr->priv->show_tooltips = show != 0;
}


int
moo_menu_mgr_append (MooMenuMgr         *mgr,
                     const char         *parent_id,
                     const char         *item_id,
                     const char         *label,
                     const char         *tip,
                     MooMenuItemFlags    flags,
                     gpointer            data,
                     GDestroyNotify      destroy)
{
    return moo_menu_mgr_insert (mgr, parent_id, -1, item_id,
                                label, tip, flags, data, destroy);
}


MooMenuMgr *
moo_menu_mgr_new (void)
{
    return MOO_MENU_MGR (g_object_new (MOO_TYPE_MENU_MGR, (const char*) NULL));
}
