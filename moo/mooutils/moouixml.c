/*
 *   moouixml.c
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
 * class:MooUiXml: (parent GObject) (constructable) (moo.private 1)
 **/

/**
 * pointer:MooUiNode: (moo.private 1)
 */

/**
 * enum:MooUiWidgetType: (moo.private 1)
 */

#include "mooutils/mooaction-private.h"
#include "mooutils/moouixml.h"
#include "marshals.h"
#include "mooutils/moomenutoolbutton.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mooi18n.h"
#include "mooutils/moomenu.h"
#include "mooutils/mootype-macros.h"
#include "mooutils/moocompat.h"
#include <gtk/gtk.h>
#include <string.h>


#define REPORT_UNKNOWN_ACTIONS 0


/* XXX weak ref actions */

typedef MooUiNodeType NodeType;
typedef MooUiNode Node;
typedef MooUiWidgetNode Widget;
typedef MooUiItemNode Item;
typedef MooUiSeparatorNode Separator;
typedef MooUiPlaceholderNode Placeholder;

#define CONTAINER MOO_UI_NODE_CONTAINER
#define WIDGET MOO_UI_NODE_WIDGET
#define TOOLBAR MOO_UI_NODE_TOOLBAR
#define ITEM MOO_UI_NODE_ITEM
#define SEPARATOR MOO_UI_NODE_SEPARATOR
#define PLACEHOLDER MOO_UI_NODE_PLACEHOLDER

static const char *NODE_TYPE_NAME[] = {
    NULL,
    "MOO_UI_NODE_CONTAINER",
    "MOO_UI_NODE_WIDGET",
    "MOO_UI_NODE_ITEM",
    "MOO_UI_NODE_SEPARATOR",
    "MOO_UI_NODE_PLACEHOLDER"
};

struct _MooUiXmlPrivate {
    Node *ui;
    GSList *toplevels;  /* Toplevel* */
    guint last_merge_id;
    GSList *merged_ui; /* Merge* */
};

typedef struct {
    Node *node;
    GtkWidget *widget;
    GHashTable *children; /* Node* -> GtkWidget* */
    MooActionCollection *actions;
    GtkAccelGroup *accel_group;
    gboolean in_creation;
} Toplevel;

typedef struct {
    guint id;
    GSList *nodes;
} Merge;

typedef enum {
    UPDATE_ADD_NODE,
    UPDATE_REMOVE_NODE,
    UPDATE_CHANGE_NODE
} UpdateType;

#define TOPLEVEL_QUARK (toplevel_quark ())
#define NODE_QUARK (node_quark ())
MOO_DEFINE_QUARK_STATIC (moo-ui-xml-toplevel, toplevel_quark)
MOO_DEFINE_QUARK_STATIC (moo-ui-xml-node, node_quark)

#define SLIST_FOREACH(list,var)                     \
G_STMT_START {                                      \
    GSList *var;                                    \
    for (var = list; var != NULL; var = var->next)  \

#define SLIST_FOREACH_END                           \
} G_STMT_END


/* walking nodes stops when func returns TRUE */
typedef gboolean (*NodeForeachFunc) (Node       *node,
                                     gpointer    data);

static void     moo_ui_xml_finalize     (GObject        *object);

static void     xml_add_markup          (MooUiXml       *xml,
                                         MooMarkupNode  *mnode);

static void     update_widgets          (MooUiXml       *xml,
                                         UpdateType      type,
                                         Node           *node);

static Node    *parse_markup            (MooMarkupNode  *mnode);
static Node    *parse_object            (MooMarkupNode  *mnode);
static Node    *parse_widget            (MooMarkupNode  *mnode);
static Node    *parse_placeholder       (MooMarkupNode  *mnode);
static Node    *parse_item              (MooMarkupNode  *mnode);
static Node    *parse_separator         (MooMarkupNode  *mnode);
static gboolean node_check              (Node           *node);
static gboolean placeholder_check       (Node           *node);
static gboolean item_check              (Node           *node);
static gboolean widget_check            (Node           *node);
static gboolean container_check         (Node           *node);

static Merge   *lookup_merge            (MooUiXml       *xml,
                                         guint           id);

static Node    *node_new                (NodeType        type,
                                         const char     *name);
static gboolean node_is_ancestor        (Node           *node,
                                         Node           *ancestor);
static gboolean node_is_empty           (Node           *node);
static GSList  *node_list_children      (Node           *ndoe);
static void     node_free               (Node           *node);
static void     node_foreach            (Node           *node,
                                         NodeForeachFunc func,
                                         gpointer        data);

static void     merge_add_node          (Merge          *merge,
                                         Node           *node);
static void     merge_remove_node       (Merge          *merge,
                                         Node           *node);

static Toplevel *toplevel_new           (Node           *node,
                                         MooActionCollection *actions,
                                         GtkAccelGroup  *accel_group);
static void     toplevel_free           (Toplevel       *toplevel);
static GtkWidget *toplevel_get_widget   (Toplevel       *toplevel,
                                         Node           *node);

static void     xml_add_item_widget     (MooUiXml       *xml,
                                         GtkWidget      *widget);
static void     xml_add_widget          (MooUiXml       *xml,
                                         GtkWidget      *widget,
                                         Toplevel       *toplevel,
                                         Node           *node);
static void     xml_remove_widget       (MooUiXml       *xml,
                                         GtkWidget      *widget);
static void     xml_delete_toplevel     (MooUiXml       *xml,
                                         Toplevel       *toplevel);
static void     xml_connect_toplevel    (MooUiXml       *xml,
                                         Toplevel       *toplevel);

static gboolean create_menu_separator   (MooUiXml       *xml,
                                         Toplevel       *toplevel,
                                         GtkMenuShell   *menu,
                                         Node           *node,
                                         int             index);
static void     create_menu_item        (MooUiXml       *xml,
                                         Toplevel       *toplevel,
                                         GtkMenuShell   *menu,
                                         Node           *node,
                                         int             index);
static gboolean create_menu_shell       (MooUiXml       *xml,
                                         Toplevel       *toplevel,
                                         MooUiWidgetType type);
static gboolean fill_menu_shell         (MooUiXml       *xml,
                                         Toplevel       *toplevel,
                                         Node           *menu_node,
                                         GtkMenuShell   *menu);

static void     check_separators        (Node           *parent,
                                         Toplevel       *toplevel);
static void     check_empty             (Node           *parent,
                                         GtkWidget      *widget,
                                         Toplevel       *toplevel);

static gboolean create_tool_separator   (MooUiXml       *xml,
                                         Toplevel       *toplevel,
                                         GtkToolbar     *toolbar,
                                         Node           *node,
                                         int             index);
static gboolean create_tool_item        (MooUiXml       *xml,
                                         Toplevel       *toplevel,
                                         GtkToolbar     *toolbar,
                                         Node           *node,
                                         int             index);
static gboolean fill_toolbar            (MooUiXml       *xml,
                                         Toplevel       *toplevel,
                                         Node           *toolbar_node,
                                         GtkToolbar     *toolbar);
static gboolean create_toolbar          (MooUiXml       *xml,
                                         Toplevel       *toplevel);

static MooUiNode *moo_ui_xml_find_node  (MooUiXml       *xml,
                                         const char     *path_or_placeholder);


/* MOO_TYPE_UI_XML */
G_DEFINE_TYPE (MooUiXml, moo_ui_xml, G_TYPE_OBJECT)


static void
moo_ui_xml_class_init (MooUiXmlClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = moo_ui_xml_finalize;
}


static Node*
node_new (NodeType type, const char *name)
{
    Node *node;
    gsize size = 0;

    switch (type)
    {
        case MOO_UI_NODE_CONTAINER:
            size = sizeof (MooUiNode);
            break;
        case MOO_UI_NODE_WIDGET:
            size = sizeof (MooUiWidgetNode);
            break;
        case MOO_UI_NODE_ITEM:
            size = sizeof (MooUiItemNode);
            break;
        case MOO_UI_NODE_SEPARATOR:
            size = sizeof (MooUiSeparatorNode);
            break;
        case MOO_UI_NODE_PLACEHOLDER:
            size = sizeof (MooUiPlaceholderNode);
            break;
    }

    node = g_malloc0 (size);
    node->type = type;
    node->name = name ? g_strdup (name) : g_strdup ("");
    return node;
}


static void
moo_ui_xml_init (MooUiXml *xml)
{
    xml->priv = g_new0 (MooUiXmlPrivate, 1);
    xml->priv->ui = node_new (CONTAINER, "ui");
}


MooUiXml*
moo_ui_xml_new (void)
{
    return g_object_new (MOO_TYPE_UI_XML, (const char*) NULL);
}


static Node*
parse_object (MooMarkupNode *mnode)
{
    Node *node;
    const char *name;
    MooMarkupNode *mchild;

    name = moo_markup_get_prop (mnode, "name");

    if (!name || !name[0])
    {
        g_warning ("object name missing");
        return NULL;
    }

    node = node_new (CONTAINER, name);

    for (mchild = mnode->children; mchild != NULL; mchild = mchild->next)
    {
        if (MOO_MARKUP_IS_ELEMENT (mchild))
        {
            Node *child = parse_markup (mchild);

            if (!child)
            {
                node_free (node);
                return NULL;
            }
            else
            {
                child->parent = node;
                node->children = g_slist_append (node->children, child);
            }
        }
    }

    return node;
}


static Node*
parse_widget (MooMarkupNode *mnode)
{
    Node *node;
    const char *name;
    MooMarkupNode *mchild;

    name = moo_markup_get_prop (mnode, "name");

    if (!name || !name[0])
    {
        g_warning ("widget name missing");
        return NULL;
    }

    node = node_new (WIDGET, name);

    for (mchild = mnode->children; mchild != NULL; mchild = mchild->next)
    {
        if (MOO_MARKUP_IS_ELEMENT (mchild))
        {
            Node *child = parse_markup (mchild);

            if (!child)
            {
                node_free (node);
                return NULL;
            }
            else
            {
                child->parent = node;
                node->children = g_slist_append (node->children, child);
            }
        }
    }

    return node;
}


static Node*
parse_placeholder (MooMarkupNode *mnode)
{
    Node *node;
    const char *name;
    MooMarkupNode *mchild;

    name = moo_markup_get_prop (mnode, "name");

    if (!name || !name[0])
    {
        g_warning ("placeholder name missing");
        return NULL;
    }

    node = node_new (PLACEHOLDER, name);

    for (mchild = mnode->children; mchild != NULL; mchild = mchild->next)
    {
        if (MOO_MARKUP_IS_ELEMENT (mchild))
        {
            Node *child = parse_markup (mchild);

            if (!child)
            {
                node_free (node);
                return NULL;
            }
            else
            {
                child->parent = node;
                node->children = g_slist_append (node->children, child);
            }
        }
    }

    return node;
}


static Item *
item_new (const char *name,
          const char *action)
{
    Item *item = (Item*) node_new (ITEM, name);
    item->action = g_strdup (action);
    return item;
}


static char *
translate_string (const char *string,
                  const char *translation_domain,
                  gboolean    translatable)
{
    const char *translated;

    if (!string || !string[0] || !translatable)
        return g_strdup (string);

    if (translation_domain)
        translated = D_(string, translation_domain);
    else
        translated = gettext (string);

    if (translated == string)
        translated = _(string);
    if (translated == string)
        translated = D_(string, "gtk20");

    return g_strdup (translated);
}

static Item *
item_new_from_node (MooMarkupNode *node,
                    const char    *translation_domain)
{
    Item *item;
    const char *name;
    const char *action;
    const char *stock_id;
    const char *stock_label;
    const char *label;
    const char *tooltip;
    const char *icon_stock_id;
    gboolean translatable = FALSE;

    name = moo_markup_get_prop (node, "name");

    if (!name)
        name = moo_markup_get_prop (node, "action");

    label = moo_markup_get_prop (node, "_label");
    if (label)
        translatable = TRUE;
    else
        label = moo_markup_get_prop (node, "label");

    tooltip = moo_markup_get_prop (node, "_tooltip");
    if (tooltip)
        translatable = TRUE;
    else
        tooltip = moo_markup_get_prop (node, "tooltip");

    action = moo_markup_get_prop (node, "action");
    stock_id = moo_markup_get_prop (node, "stock-id");
    stock_label = moo_markup_get_prop (node, "stock-label");
    icon_stock_id = moo_markup_get_prop (node, "icon-stock-id"),

    item = item_new (name, action);

    item->stock_id = g_strdup (stock_id);
    item->icon_stock_id = g_strdup (icon_stock_id);

    if (stock_label)
    {
        GtkStockItem stock_item;

        if (!gtk_stock_lookup (stock_label, &stock_item))
            g_warning ("could not find stock item '%s'", stock_label);
        else if (!stock_item.label)
            g_warning ("stock item '%s' does not have a label", stock_label);
        else
            item->label = g_strdup (stock_item.label);
    }
    else
    {
        item->label = translate_string (label, translation_domain, translatable);
    }

    item->tooltip = translate_string (tooltip, translation_domain, translatable);

    return item;
}


static Node*
parse_item (MooMarkupNode *mnode)
{
    Item *item;
    MooMarkupNode *mchild;

    item = item_new_from_node (mnode, NULL);

    for (mchild = mnode->children; mchild != NULL; mchild = mchild->next)
    {
        if (MOO_MARKUP_IS_ELEMENT (mchild))
        {
            Node *child = parse_markup (mchild);

            if (!child)
            {
                node_free ((Node*) item);
                return NULL;
            }
            else
            {
                child->parent = (Node*) item;
                item->children = g_slist_append (item->children, child);
            }
        }
    }

    return (Node*) item;
}


static Node*
parse_separator (G_GNUC_UNUSED MooMarkupNode *mnode)
{
    return node_new (SEPARATOR, NULL);
}


static Node*
parse_markup (MooMarkupNode *mnode)
{
    g_return_val_if_fail (MOO_MARKUP_IS_ELEMENT (mnode), NULL);
    g_return_val_if_fail (mnode->name != NULL, NULL);

    if (!strcmp (mnode->name, "object"))
        return parse_object (mnode);
    else if (!strcmp (mnode->name, "widget"))
        return parse_widget (mnode);
    else if (!strcmp (mnode->name, "item"))
        return parse_item (mnode);
    else if (!strcmp (mnode->name, "separator"))
        return parse_separator (mnode);
    else if (!strcmp (mnode->name, "placeholder"))
        return parse_placeholder (mnode);

    g_warning ("unknown element '%s'", mnode->name);
    return NULL;
}


static void
container_free (G_GNUC_UNUSED Node *node)
{
}

static void
widget_free (G_GNUC_UNUSED Node *node)
{
}

static void
separator_free (G_GNUC_UNUSED Node *node)
{
}

static void
placeholder_free (G_GNUC_UNUSED Node *node)
{
}

static void
item_free (Item *item)
{
    g_free (item->action);
    g_free (item->stock_id);
    g_free (item->label);
    g_free (item->tooltip);
    g_free (item->icon_stock_id);
}

static void
node_free (Node *node)
{
    if (node)
    {
        g_slist_foreach (node->children, (GFunc) node_free, NULL);
        g_slist_free (node->children);
        node->children = NULL;

        switch (node->type)
        {
            case CONTAINER:
                container_free (node);
                break;
            case WIDGET:
                widget_free (node);
                break;
            case ITEM:
                item_free ((Item*) node);
                break;
            case SEPARATOR:
                separator_free (node);
                break;
            case PLACEHOLDER:
                placeholder_free (node);
                break;
        }

        g_free (node->name);
        g_free (node);
    }
}


/**
 * moo_ui_xml_add_ui_from_string:
 *
 * @xml:
 * @buffer: (type const-utf8)
 * @length: (default -1)
 */
void
moo_ui_xml_add_ui_from_string (MooUiXml   *xml,
                               const char *buffer,
                               int         length)
{
    MooMarkupDoc *doc;
    MooMarkupNode *ui_node, *child;
    GError *error = NULL;

    g_return_if_fail (MOO_IS_UI_XML (xml));
    g_return_if_fail (buffer != NULL);

    doc = moo_markup_parse_memory (buffer, length, &error);

    if (!doc)
    {
        g_critical ("could not parse markup: %s", moo_error_message (error));
        g_error_free (error);
        return;
    }

    ui_node = moo_markup_get_root_element (doc, "ui");

    if (!ui_node)
        ui_node = MOO_MARKUP_NODE (doc);

    for (child = ui_node->children; child != NULL; child = child->next)
        if (MOO_MARKUP_IS_ELEMENT (child))
            xml_add_markup (xml, child);

    moo_markup_doc_unref (doc);
}


static gboolean
node_check (Node *node)
{
    g_return_val_if_fail (node != NULL, FALSE);

    switch (node->type)
    {
        case CONTAINER:
            return container_check (node);
        case WIDGET:
            return widget_check (node);
        case ITEM:
            return item_check (node);
        case SEPARATOR:
            return TRUE;
        case PLACEHOLDER:
            return placeholder_check (node);
    }

    g_return_val_if_reached (FALSE);
}


static gboolean
placeholder_check (Node *node)
{
    g_return_val_if_fail (node != NULL, FALSE);
    g_return_val_if_fail (node->type == PLACEHOLDER, FALSE);
    g_return_val_if_fail (node->name && node->name[0], FALSE);

    SLIST_FOREACH (node->children, l)
    {
        Node *child = l->data;

        switch (child->type)
        {
            case SEPARATOR:
            case ITEM:
            case PLACEHOLDER:
                if (!node_check (child))
                    return FALSE;
                break;

            default:
                g_warning ("invalid placeholder child type %s",
                           NODE_TYPE_NAME[child->type]);
                return FALSE;
        }
    }
    SLIST_FOREACH_END;

    return TRUE;
}


static gboolean
item_check (Node *node)
{
    g_return_val_if_fail (node != NULL, FALSE);
    g_return_val_if_fail (node->type == ITEM, FALSE);
    g_return_val_if_fail (node->name != NULL, FALSE);

    SLIST_FOREACH (node->children, l)
    {
        Node *child = l->data;

        switch (child->type)
        {
            case SEPARATOR:
            case ITEM:
            case PLACEHOLDER:
                if (!node_check (child))
                    return FALSE;
                break;

            default:
                g_warning ("invalid item child type %s",
                           NODE_TYPE_NAME[child->type]);
                return FALSE;
        }
    }
    SLIST_FOREACH_END;

    return TRUE;
}


static gboolean
widget_check (Node *node)
{
    g_return_val_if_fail (node != NULL, FALSE);
    g_return_val_if_fail (node->type == WIDGET, FALSE);
    g_return_val_if_fail (node->name && node->name[0], FALSE);

    SLIST_FOREACH (node->children, l)
    {
        Node *child = l->data;

        switch (child->type)
        {
            case SEPARATOR:
            case ITEM:
            case PLACEHOLDER:
                if (!node_check (child))
                    return FALSE;
                break;

            default:
                g_warning ("invalid widget child type %s",
                           NODE_TYPE_NAME[child->type]);
                return FALSE;
        }
    }
    SLIST_FOREACH_END;

    return TRUE;
}


static gboolean
container_check (Node *node)
{
    g_return_val_if_fail (node != NULL, FALSE);
    g_return_val_if_fail (node->type == CONTAINER, FALSE);
    g_return_val_if_fail (node->name && node->name[0], FALSE);

    SLIST_FOREACH (node->children, l)
    {
        Node *child = l->data;

        switch (child->type)
        {
            case CONTAINER:
            case WIDGET:
                if (!node_check (child))
                    return FALSE;
                break;

            default:
                g_warning ("invalid widget child type %s",
                           NODE_TYPE_NAME[child->type]);
                return FALSE;
        }
    }
    SLIST_FOREACH_END;

    return TRUE;
}


static void
xml_add_markup (MooUiXml       *xml,
                MooMarkupNode  *mnode)
{
    Node *node = parse_markup (mnode);

    if (!node)
        return;

    switch (node->type)
    {
        case CONTAINER:
            if (!container_check (node))
            {
                node_free (node);
                return;
            }
            break;

        case WIDGET:
            if (!widget_check (node))
            {
                node_free (node);
                return;
            }
            break;

        case ITEM:
        case SEPARATOR:
        case PLACEHOLDER:
            g_warning ("invalid toplevel type %s",
                       NODE_TYPE_NAME[node->type]);
            node_free (node);
            return;
    }

    if (moo_ui_xml_get_node (xml, node->name))
    {
        g_warning ("implement me?");
        node_free (node);
        return;
    }

    node->parent = xml->priv->ui;
    xml->priv->ui->children = g_slist_append (xml->priv->ui->children, node);
}


/**
 * moo_ui_xml_new_merge_id:
 */
guint
moo_ui_xml_new_merge_id (MooUiXml *xml)
{
    Merge *merge;

    g_return_val_if_fail (MOO_IS_UI_XML (xml), 0);

    xml->priv->last_merge_id++;
    merge = g_new0 (Merge, 1);
    merge->id = xml->priv->last_merge_id;
    merge->nodes = NULL;

    xml->priv->merged_ui = g_slist_prepend (xml->priv->merged_ui, merge);

    return merge->id;
}


static Merge*
lookup_merge (MooUiXml *xml,
              guint     merge_id)
{
    GSList *l;

    for (l = xml->priv->merged_ui; l != NULL; l = l->next)
    {
        Merge *merge = l->data;
        if (merge->id == merge_id)
            return merge;
    }

    return NULL;
}


/**
 * moo_ui_xml_add_item:
 *
 * @xml:
 * @merge_id:
 * @parent_path: (type const-utf8)
 * @name: (type const-utf8) (allow-none) (default NULL)
 * @action: (type const-utf8)
 * @position: (type index) (default -1)
 */
MooUiNode*
moo_ui_xml_add_item (MooUiXml       *xml,
                     guint           merge_id,
                     const char     *parent_path,
                     const char     *name,
                     const char     *action,
                     int             position)
{
    Merge *merge;
    MooUiNode *parent;
    Item *item;

    g_return_val_if_fail (MOO_IS_UI_XML (xml), NULL);
    g_return_val_if_fail (parent_path != NULL, NULL);

    if (!name || !name[0])
        name = action;

    merge = lookup_merge (xml, merge_id);
    g_return_val_if_fail (merge != NULL, NULL);

    parent = moo_ui_xml_find_node (xml, parent_path);
    g_return_val_if_fail (parent != NULL, NULL);

    switch (parent->type)
    {
        case MOO_UI_NODE_WIDGET:
        case MOO_UI_NODE_ITEM:
        case MOO_UI_NODE_PLACEHOLDER:
            break;

        case MOO_UI_NODE_CONTAINER:
        case MOO_UI_NODE_SEPARATOR:
            g_warning ("can't add item to node of type %s",
                       NODE_TYPE_NAME[parent->type]);
    }

    item = item_new (name, action);
    item->parent = parent;
    parent->children = g_slist_insert (parent->children, item, position);

    merge_add_node (merge, (Node*) item);
    update_widgets (xml, UPDATE_ADD_NODE, (Node*) item);

    return (Node*) item;
}


/**
 * moo_ui_xml_insert:
 *
 * @xml:
 * @merge_id:
 * @parent:
 * @position: (type index) (default -1)
 * @markup: (type const-utf8)
 */
void
moo_ui_xml_insert (MooUiXml       *xml,
                   guint           merge_id,
                   MooUiNode      *parent,
                   int             position,
                   const char     *markup)
{
    Merge *merge;
    GError *error = NULL;
    MooMarkupDoc *doc;
    MooMarkupNode *mchild;
    int children_length;

    g_return_if_fail (MOO_IS_UI_XML (xml));
    g_return_if_fail (markup != NULL);
    g_return_if_fail (!parent || node_is_ancestor (parent, xml->priv->ui));

    merge = lookup_merge (xml, merge_id);
    g_return_if_fail (merge != NULL);

    if (!parent)
        parent = xml->priv->ui;

    if (parent->type == MOO_UI_NODE_SEPARATOR)
    {
        g_warning ("can't add stuff to node of type %s",
                   NODE_TYPE_NAME[parent->type]);
        return;
    }

    doc = moo_markup_parse_memory (markup, -1, &error);

    if (!doc)
    {
        g_warning ("could not parse markup: %s", moo_error_message (error));
        g_error_free (error);
        return;
    }

    children_length = g_slist_length (parent->children);
    if (position < 0 || position > children_length)
        position = children_length;

    for (mchild = doc->last; mchild != NULL; mchild = mchild->prev)
    {
        Node *node;

        if (!MOO_MARKUP_IS_ELEMENT (mchild))
            continue;

        node = parse_markup (mchild);

        if (!node)
            continue;

        if (!node_check (node))
        {
            node_free (node);
            continue;
        }

        switch (node->type)
        {
            case WIDGET:
            case CONTAINER:
                if (parent->type != CONTAINER)
                {
                    g_warning ("can not add node of type %s to node of type %s",
                               NODE_TYPE_NAME[node->type],
                               NODE_TYPE_NAME[parent->type]);
                    node_free (node);
                    continue;
                }
                break;

            case ITEM:
            case PLACEHOLDER:
            case SEPARATOR:
                if (parent->type == SEPARATOR || parent->type == CONTAINER)
                {
                    g_warning ("can not add node of type %s to node of type %s",
                               NODE_TYPE_NAME[node->type],
                               NODE_TYPE_NAME[parent->type]);
                    node_free (node);
                    continue;
                }
                break;
        }

        /* XXX check names? */
        node->parent = parent;
        parent->children = g_slist_insert (parent->children, node, position);

        merge_add_node (merge, node);
        update_widgets (xml, UPDATE_ADD_NODE, node);
    }

    moo_markup_doc_unref (doc);
}


/**
 * moo_ui_xml_insert_after:
 *
 * @xml:
 * @merge_id:
 * @parent:
 * @after:
 * @markup: (type const-utf8)
 */
void
moo_ui_xml_insert_after (MooUiXml       *xml,
                         guint           merge_id,
                         MooUiNode      *parent,
                         MooUiNode      *after,
                         const char     *markup)
{
    int position;

    g_return_if_fail (MOO_IS_UI_XML (xml));

    if (!parent)
        parent = xml->priv->ui;

    g_return_if_fail (!after || after->parent == parent);

    if (!after)
        position = 0;
    else
        position = g_slist_index (parent->children, after) + 1;

    moo_ui_xml_insert (xml, merge_id, parent, position, markup);
}


/**
 * moo_ui_xml_insert_before:
 *
 * @xml:
 * @merge_id:
 * @parent:
 * @before:
 * @markup: (type const-utf8)
 */
void
moo_ui_xml_insert_before (MooUiXml       *xml,
                          guint           merge_id,
                          MooUiNode      *parent,
                          MooUiNode      *before,
                          const char     *markup)
{
    int position;

    g_return_if_fail (MOO_IS_UI_XML (xml));

    if (!parent)
        parent = xml->priv->ui;

    g_return_if_fail (!before || before->parent == parent);

    if (!before)
        position = g_slist_length (parent->children);
    else
        position = g_slist_index (parent->children, before);

    moo_ui_xml_insert (xml, merge_id, parent, position, markup);
}


/**
 * moo_ui_xml_insert_markup_after:
 *
 * @xml:
 * @merge_id:
 * @parent_path: (type const-utf8)
 * @after_name: (type const-utf8)
 * @markup: (type const-utf8)
 */
void
moo_ui_xml_insert_markup_after (MooUiXml       *xml,
                                guint           merge_id,
                                const char     *parent_path,
                                const char     *after_name,
                                const char     *markup)
{
    Node *parent = NULL, *after = NULL;

    g_return_if_fail (MOO_IS_UI_XML (xml));
    g_return_if_fail (markup != NULL);

    if (parent_path)
    {
        parent = moo_ui_xml_find_node (xml, parent_path);
        g_return_if_fail (parent != NULL);
    }
    else
    {
        parent = xml->priv->ui;
    }

    if (after_name)
    {
        after = moo_ui_node_get_child (parent, after_name);
        g_return_if_fail (after != NULL);
    }

    moo_ui_xml_insert_after (xml, merge_id, parent, after, markup);
}


/**
 * moo_ui_xml_insert_markup_before:
 *
 * @xml:
 * @merge_id:
 * @parent_path: (type const-utf8)
 * @before_name: (type const-utf8)
 * @markup: (type const-utf8)
 */
void
moo_ui_xml_insert_markup_before (MooUiXml       *xml,
                                 guint           merge_id,
                                 const char     *parent_path,
                                 const char     *before_name,
                                 const char     *markup)
{
    Node *parent = NULL, *before = NULL;

    g_return_if_fail (MOO_IS_UI_XML (xml));
    g_return_if_fail (markup != NULL);

    if (parent_path)
    {
        parent = moo_ui_xml_find_node (xml, parent_path);
        g_return_if_fail (parent != NULL);
    }
    else
    {
        parent = xml->priv->ui;
    }

    if (before_name)
    {
        before = moo_ui_node_get_child (parent, before_name);
        g_return_if_fail (before != NULL);
    }

    moo_ui_xml_insert_before (xml, merge_id, parent, before, markup);
}


/**
 * moo_ui_xml_insert_markup:
 *
 * @xml:
 * @merge_id:
 * @parent_path: (type const-utf8)
 * @position: (type index) (default -1)
 * @markup: (type const-utf8)
 */
void
moo_ui_xml_insert_markup (MooUiXml       *xml,
                          guint           merge_id,
                          const char     *parent_path,
                          int             position,
                          const char     *markup)
{
    Node *parent = NULL;

    g_return_if_fail (MOO_IS_UI_XML (xml));
    g_return_if_fail (markup != NULL);

    if (parent_path)
    {
        parent = moo_ui_xml_find_node (xml, parent_path);
        g_return_if_fail (parent != NULL);
    }
    else
    {
        parent = xml->priv->ui;
    }

    moo_ui_xml_insert (xml, merge_id, parent, position, markup);
}


static gboolean
node_is_ancestor (Node           *node,
                  Node           *ancestor)
{
    Node *n;

    g_return_val_if_fail (node != NULL, FALSE);
    g_return_val_if_fail (ancestor != NULL, FALSE);

    for (n = node; n != NULL; n = n->parent)
        if (n == ancestor)
            return TRUE;

    return FALSE;
}


/**
 * moo_ui_xml_remove_ui:
 */
void
moo_ui_xml_remove_ui (MooUiXml       *xml,
                      guint           merge_id)
{
    Merge *merge;
    GSList *nodes;

    g_return_if_fail (MOO_IS_UI_XML (xml));

    merge = lookup_merge (xml, merge_id);
    g_return_if_fail (merge != NULL);

    nodes = g_slist_copy (merge->nodes);
    while (nodes)
    {
        moo_ui_xml_remove_node (xml, nodes->data);
        nodes = g_slist_delete_link (nodes, nodes);
    }

    g_return_if_fail (merge->nodes == NULL);
    xml->priv->merged_ui = g_slist_remove (xml->priv->merged_ui, merge);
    g_free (merge);
}


/**
 * moo_ui_xml_remove_node:
 */
void
moo_ui_xml_remove_node (MooUiXml       *xml,
                        MooUiNode      *node)
{
    Node *parent;

    g_return_if_fail (MOO_IS_UI_XML (xml));
    g_return_if_fail (node != NULL);
    g_return_if_fail (node_is_ancestor (node, xml->priv->ui));

    SLIST_FOREACH (xml->priv->merged_ui, l)
    {
        Merge *merge = l->data;
        GSList *merge_nodes = g_slist_copy (merge->nodes);

        SLIST_FOREACH (merge_nodes, n)
        {
            Node *merge_node = n->data;
            if (node_is_ancestor (merge_node, node))
                merge_remove_node (merge, merge_node);
        }
        SLIST_FOREACH_END;

        g_slist_free (merge_nodes);
    }
    SLIST_FOREACH_END;

    update_widgets (xml, UPDATE_REMOVE_NODE, node);

    parent = node->parent;
    parent->children = g_slist_remove (parent->children, node);
    node->parent = NULL;

    while (parent && parent->type == MOO_UI_NODE_PLACEHOLDER)
        parent = parent->parent;

    SLIST_FOREACH (xml->priv->toplevels, l)
    {
        Toplevel *toplevel = l->data;
        if (node_is_ancestor (parent, toplevel->node))
            check_separators (parent, toplevel);
    }
    SLIST_FOREACH_END;

    node_free (node);
}


static void
merge_add_node (Merge *merge,
                Node  *added)
{
    g_return_if_fail (merge != NULL);
    g_return_if_fail (added != NULL);

    SLIST_FOREACH (merge->nodes, l)
    {
        Node *node = l->data;

        if (node_is_ancestor (added, node))
            return;
    }
    SLIST_FOREACH_END;

    merge->nodes = g_slist_prepend (merge->nodes, added);
}


static void
merge_remove_node (Merge          *merge,
                   Node           *removed)
{
    g_return_if_fail (merge != NULL);
    g_return_if_fail (removed != NULL);

    merge->nodes = g_slist_remove (merge->nodes, removed);
}


/**
 * moo_ui_xml_get_node:
 *
 * @xml:
 * @path: (type const-utf8)
 */
MooUiNode *
moo_ui_xml_get_node (MooUiXml       *xml,
                     const char     *path)
{
    g_return_val_if_fail (MOO_IS_UI_XML (xml), NULL);
    g_return_val_if_fail (path != NULL, NULL);

    return moo_ui_node_get_child (xml->priv->ui, path);
}


static MooUiNode *
moo_ui_xml_find_node (MooUiXml       *xml,
                      const char     *path)
{
    MooUiNode *node;

    g_return_val_if_fail (MOO_IS_UI_XML (xml), NULL);
    g_return_val_if_fail (path != NULL, NULL);

    node = moo_ui_xml_get_node (xml, path);

    if (!node)
        node = moo_ui_xml_find_placeholder (xml, path);

    return node;
}


static gboolean
find_placeholder_func (Node    *node,
                       gpointer user_data)
{
    struct {
        Node *found;
        const char *name;
    } *data = user_data;

    if (node->type != PLACEHOLDER)
        return FALSE;

    if (!strcmp (node->name, data->name))
    {
        data->found = node;
        return TRUE;
    }

    return FALSE;
}

/**
 * moo_ui_xml_find_placeholder:
 *
 * @xml:
 * @name: (type const-utf8)
 */
MooUiNode*
moo_ui_xml_find_placeholder (MooUiXml       *xml,
                             const char     *name)
{
    struct {
        Node *found;
        const char *name;
    } data;

    g_return_val_if_fail (MOO_IS_UI_XML (xml), NULL);
    g_return_val_if_fail (name != NULL, NULL);

    data.found = NULL;
    data.name = name;
    node_foreach (xml->priv->ui, find_placeholder_func, &data);

    return data.found;
}


/**
 * moo_ui_node_get_path:
 *
 * Returns: (type utf8)
 */
char *
moo_ui_node_get_path (MooUiNode *node)
{
    GString *path;

    g_return_val_if_fail (node != NULL, NULL);

    path = g_string_new (node->name);

    while (node->parent && node->parent->parent)
    {
        node = node->parent;
        g_string_prepend_c (path, '/');
        g_string_prepend (path, node->name);
    }

    g_print ("path: %s\n", path->str);
    return g_string_free (path, FALSE);
}


/**
 * moo_ui_node_get_child:
 *
 * @node:
 * @name: (type const-utf8)
 */
MooUiNode *
moo_ui_node_get_child (MooUiNode      *node,
                       const char     *path)
{
    char **pieces, **p;

    g_return_val_if_fail (node != NULL, NULL);
    g_return_val_if_fail (path != NULL, NULL);

    pieces = g_strsplit (path, "/", 0);

    if (!pieces)
        return node;

    for (p = pieces; *p != NULL; ++p)
    {
        Node *child = NULL;

        if (!**p)
            continue;

        SLIST_FOREACH (node->children, l)
        {
            child = l->data;
            if (!strcmp (child->name, *p))
                break;
            else
                child = NULL;
        }
        SLIST_FOREACH_END;

        if (child)
        {
            node = child;
        }
        else
        {
            node = NULL;
            goto out;
        }
    }

out:
    g_strfreev (pieces);
    return node;
}


static Toplevel*
toplevel_new (Node                *node,
              MooActionCollection *actions,
              GtkAccelGroup       *accel_group)
{
    Toplevel *top;

    g_return_val_if_fail (node != NULL, NULL);

    top = g_new0 (Toplevel, 1);
    top->node = node;
    top->widget = NULL;
    top->children = g_hash_table_new (g_direct_hash, g_direct_equal);
    top->actions = actions;
    top->accel_group = accel_group;

    return top;
}


static void
toplevel_free (Toplevel *toplevel)
{
    if (toplevel)
    {
        g_hash_table_destroy (toplevel->children);
        g_free (toplevel);
    }
}


static Node *
get_effective_parent (Node *node)
{
    Node *parent;

    g_return_val_if_fail (node != NULL && node->parent != NULL, NULL);

    for (parent = node->parent;
         parent && parent->type == PLACEHOLDER;
         parent = parent->parent) ;

    return parent;
}


static void
visibility_notify (GtkWidget *widget,
                   G_GNUC_UNUSED gpointer whatever,
                   MooUiXml  *xml)
{
    Toplevel *toplevel;
    Node *node;

    g_return_if_fail (GTK_IS_WIDGET (widget));
    g_return_if_fail (MOO_IS_UI_XML (xml));

    toplevel = g_object_get_qdata (G_OBJECT (widget), TOPLEVEL_QUARK);
    g_return_if_fail (toplevel != NULL);

    if (toplevel->in_creation)
        return;

    node = g_object_get_qdata (G_OBJECT (widget), NODE_QUARK);
    g_return_if_fail (node != NULL && node->parent != NULL);
    g_return_if_fail (node->type == ITEM);

    check_separators (get_effective_parent (node), toplevel);
}

static void
xml_add_item_widget (MooUiXml       *xml,
                     GtkWidget      *widget)
{
    g_signal_connect (widget, "notify::visible",
                      G_CALLBACK (visibility_notify), xml);
}


static void
widget_destroyed (GtkWidget *widget,
                  MooUiXml  *xml)
{
    g_return_if_fail (GTK_IS_WIDGET (widget));
    g_return_if_fail (MOO_IS_UI_XML (xml));
    xml_remove_widget (xml, widget);
}


static void
xml_remove_widget (MooUiXml  *xml,
                   GtkWidget *widget)
{
    Toplevel *toplevel;
    Node *node;

    g_return_if_fail (GTK_IS_WIDGET (widget));
    g_return_if_fail (MOO_IS_UI_XML (xml));

    toplevel = g_object_get_qdata (G_OBJECT (widget), TOPLEVEL_QUARK);
    g_return_if_fail (toplevel != NULL);
    g_return_if_fail (toplevel->widget != widget);

    node = g_object_get_qdata (G_OBJECT (widget), NODE_QUARK);
    g_hash_table_remove (toplevel->children, node);

    g_object_set_qdata (G_OBJECT (widget), NODE_QUARK, NULL);
    g_object_set_qdata (G_OBJECT (widget), TOPLEVEL_QUARK, NULL);
    g_signal_handlers_disconnect_by_func (widget,
                                          (gpointer) widget_destroyed,
                                          xml);
    g_signal_handlers_disconnect_by_func (widget,
                                          (gpointer) visibility_notify,
                                          xml);
}


static void
xml_add_widget (MooUiXml  *xml,
                GtkWidget *widget,
                Toplevel  *toplevel,
                Node      *node)
{
    g_signal_connect (widget, "destroy",
                      G_CALLBACK (widget_destroyed), xml);
    g_object_set_qdata (G_OBJECT (widget), TOPLEVEL_QUARK, toplevel);
    g_object_set_qdata (G_OBJECT (widget), NODE_QUARK, node);
    g_hash_table_insert (toplevel->children, node, widget);
}


static void
prepend_value (G_GNUC_UNUSED gpointer key,
               gpointer value,
               GSList **list)
{
    *list = g_slist_prepend (*list, value);
}

static GSList*
hash_table_list_values (GHashTable *hash_table)
{
    GSList *list = NULL;
    g_hash_table_foreach (hash_table, (GHFunc) prepend_value, &list);
    return list;
}


static void
toplevel_destroyed (GtkWidget *widget,
                    MooUiXml  *xml)
{
    Toplevel *toplevel;

    g_return_if_fail (GTK_IS_WIDGET (widget));
    g_return_if_fail (MOO_IS_UI_XML (xml));

    toplevel = g_object_get_qdata (G_OBJECT (widget), TOPLEVEL_QUARK);
    g_return_if_fail (toplevel != NULL);
    g_return_if_fail (toplevel->widget == widget);

    xml_delete_toplevel (xml, toplevel);
}


static void
xml_delete_toplevel (MooUiXml *xml,
                     Toplevel *toplevel)
{
    GSList *children, *l;

    children = hash_table_list_values (toplevel->children);

    for (l = children; l != NULL; l = l->next)
    {
        GtkWidget *child = GTK_WIDGET (l->data);
        if (child != toplevel->widget)
            xml_remove_widget (xml, child);
    }

    g_signal_handlers_disconnect_by_func (toplevel->widget,
                                          (gpointer) toplevel_destroyed,
                                          xml);
    g_object_set_qdata (G_OBJECT (toplevel->widget), TOPLEVEL_QUARK, NULL);
    g_object_set_qdata (G_OBJECT (toplevel->widget), NODE_QUARK, NULL);
    xml->priv->toplevels = g_slist_remove (xml->priv->toplevels, toplevel);

    g_slist_free (children);
    toplevel_free (toplevel);
}


static void
xml_connect_toplevel (MooUiXml  *xml,
                      Toplevel  *toplevel)
{
    g_signal_connect (toplevel->widget, "destroy",
                      G_CALLBACK (toplevel_destroyed), xml);
    g_object_set_qdata (G_OBJECT (toplevel->widget), TOPLEVEL_QUARK, toplevel);
    g_object_set_qdata (G_OBJECT (toplevel->widget), NODE_QUARK, toplevel->node);
    g_hash_table_insert (toplevel->children, toplevel->node, toplevel->widget);
}


static gboolean
create_menu_separator (MooUiXml       *xml,
                       Toplevel       *toplevel,
                       GtkMenuShell   *menu,
                       Node           *node,
                       int             index)
{
    GtkWidget *item = gtk_separator_menu_item_new ();
    gtk_menu_shell_insert (menu, item, index);
    xml_add_widget (xml, item, toplevel, node);
    return TRUE;
}


static gboolean node_is_empty (Node *node)
{
    SLIST_FOREACH (node->children, l)
    {
        Node *child = l->data;

        if (child->type == SEPARATOR)
            return FALSE;

        if (child->type == PLACEHOLDER)
        {
            if (!node_is_empty (child))
                return FALSE;
        }
        else
        {
            return FALSE;
        }
    }
    SLIST_FOREACH_END;

    return TRUE;
}


static void
create_menu_item (MooUiXml       *xml,
                  Toplevel       *toplevel,
                  GtkMenuShell   *menu,
                  Node           *node,
                  int             index)
{
    GtkWidget *menu_item = NULL;
    Item *item;

    g_return_if_fail (node != NULL && node->type == ITEM);
    g_return_if_fail (node->name != NULL);

    item = (Item*) node;

    if (item->action)
    {
        GtkAction *action;

        g_return_if_fail (toplevel->actions != NULL);

        action = moo_action_collection_get_action (toplevel->actions, item->action);

        if (!action)
        {
#if REPORT_UNKNOWN_ACTIONS
            g_critical ("could not find action '%s'", item->action);
#endif
            return;
        }

        if (_moo_action_get_dead (action))
            return;

        gtk_action_set_accel_group (action, toplevel->accel_group);
        menu_item = gtk_action_create_menu_item (action);
    }
    else
    {
        if (item->stock_id)
        {
            menu_item = gtk_image_menu_item_new_from_stock (item->stock_id, NULL);
        }
        else if (item->label)
        {
            if (item->icon_stock_id)
            {
                GtkWidget *icon = gtk_image_new_from_stock (item->icon_stock_id,
                                                            GTK_ICON_SIZE_MENU);
                menu_item = gtk_image_menu_item_new_with_mnemonic (item->label);
                gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), icon);
            }
            else
            {
                menu_item = gtk_menu_item_new_with_mnemonic (item->label);
            }
        }
        else
        {
            g_warning ("item '%s' does not have an associated action, label, or stock id",
                       item->name);
        }

        if (menu_item)
            gtk_widget_show (menu_item);
    }

    g_return_if_fail (menu_item != NULL);

    gtk_menu_shell_insert (menu, menu_item, index);
    xml_add_widget (xml, menu_item, toplevel, node);
    xml_add_item_widget (xml, menu_item);

    if (!node_is_empty (node))
    {
        GtkWidget *submenu = gtk_menu_new ();
        gtk_widget_show (submenu);
        gtk_menu_set_accel_group (GTK_MENU (submenu), toplevel->accel_group);
        gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);
        fill_menu_shell (xml, toplevel, node,
                         GTK_MENU_SHELL (submenu));
    }

    /* XXX completely empty menus? */
    if (node->children)
        check_empty (node, menu_item, toplevel);
}


static GSList*
node_list_children (Node *parent)
{
    GSList *list = NULL, *l;

    for (l = parent->children; l != NULL; l = l->next)
    {
        GSList *tmp, *t;
        Node *node = l->data;

        switch (node->type)
        {
            case MOO_UI_NODE_ITEM:
            case MOO_UI_NODE_SEPARATOR:
                list = g_slist_prepend (list, node);
                break;
            case MOO_UI_NODE_PLACEHOLDER:
                tmp = node_list_children (node);
                for (t = tmp; t != NULL; t = t->next)
                    list = g_slist_prepend (list, t->data);
                g_slist_free (tmp);
                break;
            default:
                g_return_val_if_reached (g_slist_reverse (list));
        }
    }

    return g_slist_reverse (list);
}


static void
real_foreach (Node    *node,
              gpointer data)
{
    GSList *l;
    struct {
        NodeForeachFunc func;
        gpointer func_data;
        gboolean stop;
    } *foreach_data = data;

    if (foreach_data->stop)
        return;

    if (foreach_data->func (node, foreach_data->func_data))
    {
        foreach_data->stop = TRUE;
        return;
    }

    for (l = node->children; l != NULL; l = l->next)
    {
        Node *child = l->data;

        real_foreach (child, data);

        if (foreach_data->stop)
            return;
    }
}

static void
node_foreach (Node           *node,
              NodeForeachFunc func,
              gpointer        data)
{
    struct {
        NodeForeachFunc func;
        gpointer func_data;
        gboolean stop;
    } foreach_data;

    g_return_if_fail (node != NULL);
    g_return_if_fail (func != NULL);

    foreach_data.func = func;
    foreach_data.func_data = data;
    foreach_data.stop = FALSE;
    real_foreach (node, &foreach_data);
}


static GtkWidget*
toplevel_get_widget (Toplevel  *toplevel,
                     Node      *node)
{
    return g_hash_table_lookup (toplevel->children, node);
}


static void
check_empty (Node           *parent,
             GtkWidget      *widget,
             Toplevel       *toplevel)
{
    GSList *children, *l;
    gboolean has_children = FALSE;

    children = node_list_children (parent);

    for (l = children; l != NULL; l = l->next)
    {
        Node *node = l->data;

        if (node->type == MOO_UI_NODE_ITEM)
        {
            GtkWidget *nw = toplevel_get_widget (toplevel, node);

            if (nw && GTK_WIDGET_VISIBLE (nw))
            {
                has_children = TRUE;
                break;
            }
        }
    }

    /* XXX decide something on this stuff */
#if 0
//     if (!(parent->flags & MOO_UI_NODE_ENABLE_EMPTY))
//         gtk_widget_set_sensitive (widget, has_children);
#endif
    g_object_set (widget, "visible", has_children, NULL);

    g_slist_free (children);
}


static void
check_separators (Node           *parent,
                  Toplevel       *toplevel)
{
    GSList *children, *l;
    Node *separator = NULL;
    gboolean first = TRUE;
    GtkWidget *widget;

    if (!toplevel_get_widget (toplevel, parent))
        return;

    children = node_list_children (parent);

    for (l = children; l != NULL; l = l->next)
    {
        Node *node = l->data;

        switch (node->type)
        {
            case MOO_UI_NODE_ITEM:
                widget = toplevel_get_widget (toplevel, node);

                if (!widget || !GTK_WIDGET_VISIBLE (widget))
                    continue;

                if (!first)
                {
                    if (separator)
                    {
                        GtkWidget *sep_widget = toplevel_get_widget (toplevel, separator);
                        g_return_if_fail (sep_widget != NULL);
                        gtk_widget_show (sep_widget);
                    }
                }
                else
                {
                    first = FALSE;
                    separator = NULL;
                }
                break;

            case MOO_UI_NODE_SEPARATOR:
                widget = toplevel_get_widget (toplevel, node);
                g_return_if_fail (widget != NULL);
                gtk_widget_hide (widget);
                if (!first)
                    separator = node;
                break;

            default:
                g_return_if_reached ();
        }
    }

    widget = toplevel_get_widget (toplevel, parent);

    if (widget)
        check_empty (parent, widget, toplevel);

    g_slist_free (children);
}


static gboolean
fill_menu_shell (MooUiXml       *xml,
                 Toplevel       *toplevel,
                 Node           *menu_node,
                 GtkMenuShell   *menu)
{
    GSList *children;

    children = node_list_children (menu_node);

    SLIST_FOREACH (children, l)
    {
        Node *node = l->data;

        switch (node->type)
        {
            case MOO_UI_NODE_ITEM:
                create_menu_item (xml, toplevel, menu, node, -1);
                break;
            case MOO_UI_NODE_SEPARATOR:
                create_menu_separator (xml, toplevel, menu, node, -1);
                break;

            default:
                g_warning ("invalid menu item type %s",
                           NODE_TYPE_NAME[node->type]);
                return FALSE;
        }
    }
    SLIST_FOREACH_END;

    check_separators (menu_node, toplevel);

    g_slist_free (children);
    return TRUE;
}


static gboolean
create_menu_shell (MooUiXml       *xml,
                   Toplevel       *toplevel,
                   MooUiWidgetType type)
{
    g_return_val_if_fail (toplevel != NULL, FALSE);
    g_return_val_if_fail (toplevel->widget == NULL, FALSE);
    g_return_val_if_fail (toplevel->node != NULL, FALSE);

    if (type == MOO_UI_MENUBAR)
    {
        toplevel->widget = gtk_menu_bar_new ();
    }
    else
    {
        toplevel->widget = moo_menu_new ();
        gtk_menu_set_accel_group (GTK_MENU (toplevel->widget),
                                  toplevel->accel_group);
    }

    xml_connect_toplevel (xml, toplevel);

    return fill_menu_shell (xml, toplevel,
                            toplevel->node,
                            GTK_MENU_SHELL (toplevel->widget));
}


static gboolean
create_tool_separator (MooUiXml       *xml,
                       Toplevel       *toplevel,
                       GtkToolbar     *toolbar,
                       Node           *node,
                       int             index)
{
    GtkToolItem *item = gtk_separator_tool_item_new ();
    gtk_toolbar_insert (toolbar, item, index);
    xml_add_widget (xml, GTK_WIDGET (item), toplevel, node);
    return TRUE;
}


#define IS_MENU_TOOL_BUTTON(wid) (GTK_IS_MENU_TOOL_BUTTON (wid) || \
                                  MOO_IS_MENU_TOOL_BUTTON (wid))

static void
menu_tool_button_set_menu (GtkWidget *button,
                           GtkWidget *menu)
{
    if (MOO_IS_MENU_TOOL_BUTTON (button))
        moo_menu_tool_button_set_menu (MOO_MENU_TOOL_BUTTON (button), menu);
    else if (GTK_IS_MENU_TOOL_BUTTON (button))
        gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (button), menu);
    else
        g_return_if_reached ();
}


static GtkWidget*
menu_tool_button_get_menu (GtkWidget *button)
{
    if (MOO_IS_MENU_TOOL_BUTTON (button))
        return moo_menu_tool_button_get_menu (MOO_MENU_TOOL_BUTTON (button));
    else if (GTK_IS_MENU_TOOL_BUTTON (button))
        return gtk_menu_tool_button_get_menu (GTK_MENU_TOOL_BUTTON (button));
    else
        g_return_val_if_reached (NULL);
}


static gboolean
create_tool_item (MooUiXml       *xml,
                  Toplevel       *toplevel,
                  GtkToolbar     *toolbar,
                  Node           *node,
                  int             index)
{
    GtkWidget *tool_item = NULL;
    Item *item;

    g_return_val_if_fail (node != NULL && node->type == ITEM, FALSE);

    item = (Item*) node;

    if (item->action)
    {
        GtkAction *action;

        g_return_val_if_fail (toplevel->actions != NULL, FALSE);

        action = moo_action_collection_get_action (toplevel->actions, item->action);

        if (!action || _moo_action_get_dead (action))
            return TRUE;

        gtk_action_set_accel_group (action, toplevel->accel_group);

        if (_moo_action_get_has_submenu (action))
        {
            tool_item = GTK_WIDGET (gtk_menu_tool_button_new (NULL, NULL));
            gtk_action_connect_proxy (action, tool_item);
        }
        else
        {
            tool_item = gtk_action_create_tool_item (action);
        }

        if (index > gtk_toolbar_get_n_items (toolbar))
            index = -1;

        gtk_toolbar_insert (toolbar, GTK_TOOL_ITEM (tool_item), index);
        _moo_action_ring_the_bells_it_has_tooltip (action);

        if (node->children)
        {
            if (!IS_MENU_TOOL_BUTTON (tool_item))
            {
                g_critical ("oops");
            }
            else
            {
                GtkWidget *menu = gtk_menu_new ();
                /* XXX empty menu */
                gtk_widget_show (menu);
                menu_tool_button_set_menu (tool_item, menu);
                fill_menu_shell (xml, toplevel, node, GTK_MENU_SHELL (menu));
            }
        }

    }
    else
    {
        GtkWidget *menu;

        tool_item = moo_menu_tool_button_new ();
        gtk_widget_show (tool_item);

        if (item->icon_stock_id)
            gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (tool_item),
                                          item->icon_stock_id);
        if (item->stock_id)
            gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (tool_item),
                                          item->stock_id);
        if (item->label)
            gtk_tool_button_set_label (GTK_TOOL_BUTTON (tool_item),
                                       item->label);

        gtk_toolbar_insert (toolbar, GTK_TOOL_ITEM (tool_item), index);

        if (item->tooltip)
            _moo_widget_set_tooltip (tool_item, item->tooltip);

        menu = gtk_menu_new ();
        /* XXX empty menu */
        gtk_widget_show (menu);
        menu_tool_button_set_menu (tool_item, menu);
        fill_menu_shell (xml, toplevel, node, GTK_MENU_SHELL (menu));
    }

    g_return_val_if_fail (tool_item != NULL, FALSE);

    gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (tool_item), FALSE);
    xml_add_widget (xml, tool_item, toplevel, node);
    xml_add_item_widget (xml, tool_item);

    return TRUE;
}


static gboolean
fill_toolbar (MooUiXml       *xml,
              Toplevel       *toplevel,
              Node           *toolbar_node,
              GtkToolbar     *toolbar)
{
    gboolean result = TRUE;
    GSList *children;

    children = node_list_children (toolbar_node);

    SLIST_FOREACH (children, l)
    {
        Node *node = l->data;

        switch (node->type)
        {
            case MOO_UI_NODE_ITEM:
                create_tool_item (xml, toplevel, toolbar, node, -1);
                break;
            case MOO_UI_NODE_SEPARATOR:
                create_tool_separator (xml, toplevel, toolbar, node, -1);
                break;

            default:
                g_warning ("invalid tool item type %s",
                           NODE_TYPE_NAME[node->type]);
                return FALSE;
        }

        if (!result)
            return FALSE;
    }
    SLIST_FOREACH_END;

    check_separators (toolbar_node, toplevel);

    g_slist_free (children);
    return TRUE;
}


static gboolean
create_toolbar (MooUiXml       *xml,
                Toplevel       *toplevel)
{
    g_return_val_if_fail (toplevel != NULL, FALSE);
    g_return_val_if_fail (toplevel->widget == NULL, FALSE);
    g_return_val_if_fail (toplevel->node != NULL, FALSE);

    toplevel->widget = gtk_toolbar_new ();
    gtk_toolbar_set_tooltips (GTK_TOOLBAR (toplevel->widget), TRUE);
    xml_connect_toplevel (xml, toplevel);

    return fill_toolbar (xml, toplevel,
                         toplevel->node,
                         GTK_TOOLBAR (toplevel->widget));
}


/**
 * moo_ui_xml_create_widget:
 *
 * @xml:
 * @type:
 * @path: (type const-utf8)
 * @actions:
 * @accel_group:
 *
 * Returns: (type GtkWidget*)
 */
GtkWidget*
moo_ui_xml_create_widget (MooUiXml            *xml,
                          MooUiWidgetType      type,
                          const char          *path,
                          MooActionCollection *actions,
                          GtkAccelGroup       *accel_group)
{
    Node *node;
    Toplevel *toplevel;
    gboolean result = FALSE;

    g_return_val_if_fail (MOO_IS_UI_XML (xml), NULL);
    g_return_val_if_fail (path != NULL, NULL);
    g_return_val_if_fail (!actions || MOO_IS_ACTION_COLLECTION (actions), NULL);

    node = moo_ui_xml_get_node (xml, path);
    g_return_val_if_fail (node != NULL, NULL);

    if (node->type != WIDGET)
    {
        g_warning ("can create widgets only for nodes of type %s",
                   NODE_TYPE_NAME[WIDGET]);
        return NULL;
    }

    if (type < 1 || type > 3)
    {
        g_warning ("invalid widget type %u", type);
        return NULL;
    }

    toplevel = toplevel_new (node, actions, accel_group);
    toplevel->in_creation = TRUE;

    xml->priv->toplevels = g_slist_append (xml->priv->toplevels, toplevel);

    switch (type)
    {
        case MOO_UI_MENUBAR:
            result = create_menu_shell (xml, toplevel, MOO_UI_MENUBAR);
            break;
        case MOO_UI_MENU:
            result = create_menu_shell (xml, toplevel, MOO_UI_MENU);
            break;
        case MOO_UI_TOOLBAR:
            result = create_toolbar (xml, toplevel);
            break;
    }

    toplevel->in_creation = FALSE;

    if (!result)
    {
        xml_delete_toplevel (xml, toplevel);
        return NULL;
    }

    return toplevel->widget;
}


/**
 * moo_ui_xml_get_widget:
 *
 * @xml:
 * @widget:
 * @path: (type const-utf8)
 */
GtkWidget*
moo_ui_xml_get_widget (MooUiXml       *xml,
                       GtkWidget      *widget,
                       const char     *path)
{
    Toplevel *toplevel;
    MooUiNode *node;

    g_return_val_if_fail (MOO_IS_UI_XML (xml), NULL);
    g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
    g_return_val_if_fail (path != NULL, NULL);

    node = moo_ui_xml_get_node (xml, path);
    g_return_val_if_fail (node != NULL, NULL);

    toplevel = g_object_get_qdata (G_OBJECT (widget), TOPLEVEL_QUARK);
    g_return_val_if_fail (toplevel != NULL, NULL);

    return toplevel_get_widget (toplevel, node);
}


static Node*
effective_parent (Node *node)
{
    Node *parent;
    g_return_val_if_fail (node != NULL, NULL);
    parent = node->parent;
    while (parent && parent->type == PLACEHOLDER)
        parent = parent->parent;
    return parent;
}


static int
effective_index (Node *parent,
                 Node *node)
{
    GSList *children;
    int index;
    g_return_val_if_fail (effective_parent (node) == parent, -1);
    children = node_list_children (parent);
    index = g_slist_index (children, node);
    g_slist_free (children);
    g_return_val_if_fail (index >= 0, -1);
    return index;
}


static void
toplevel_add_node (MooUiXml *xml,
                   Toplevel *toplevel,
                   Node     *node)
{
    g_return_if_fail (GTK_IS_WIDGET (toplevel->widget));
    g_return_if_fail (node->type == ITEM || node->type == SEPARATOR);
    g_return_if_fail (node_is_ancestor (node, toplevel->node));

    if (GTK_IS_TOOLBAR (toplevel->widget))
    {
        GtkWidget *parent_widget;
        Node *parent = effective_parent (node);

        g_return_if_fail (parent != NULL);
        parent_widget = toplevel_get_widget (toplevel, parent);
        g_return_if_fail (parent_widget != NULL);

        if (GTK_IS_TOOLBAR (parent_widget))
        {
            switch (node->type)
            {
                case ITEM:
                    create_tool_item (xml, toplevel,
                                      GTK_TOOLBAR (parent_widget),
                                      node, effective_index (parent, node));
                    break;
                case SEPARATOR:
                    create_tool_separator (xml, toplevel,
                                           GTK_TOOLBAR (parent_widget),
                                           node, effective_index (parent, node));
                    break;
                default:
                    g_return_if_reached ();
            }

            check_separators (parent, toplevel);
        }
        else if (IS_MENU_TOOL_BUTTON (parent_widget))
        {
            GtkWidget *menu;

            menu = menu_tool_button_get_menu (parent_widget);

            if (!menu)
            {
                menu = gtk_menu_new ();
                gtk_widget_show (menu);
                menu_tool_button_set_menu (parent_widget, menu);
            }

            switch (node->type)
            {
                case ITEM:
                    create_menu_item (xml, toplevel, GTK_MENU_SHELL (menu),
                                      node, effective_index (parent, node));
                    break;
                case SEPARATOR:
                    create_menu_separator (xml, toplevel, GTK_MENU_SHELL (menu),
                                           node, effective_index (parent, node));
                    break;
                default:
                    g_return_if_reached ();
            }

            check_separators (parent, toplevel);
        }
        else
        {
            g_return_if_reached ();
        }
    }
    else if (GTK_IS_MENU_SHELL (toplevel->widget))
    {
        GtkWidget *parent_widget, *menu_shell;
        Node *parent = effective_parent (node);

        g_return_if_fail (parent != NULL);
        parent_widget = toplevel_get_widget (toplevel, parent);
        g_return_if_fail (parent_widget != NULL);

        if (GTK_IS_MENU_SHELL (parent_widget))
        {
            menu_shell = parent_widget;
        }
        else
        {
            g_return_if_fail (GTK_IS_MENU_ITEM (parent_widget));
            menu_shell = gtk_menu_item_get_submenu (GTK_MENU_ITEM (parent_widget));
            if (!menu_shell)
            {
                menu_shell = gtk_menu_new ();
                gtk_widget_show (menu_shell);
                gtk_menu_item_set_submenu (GTK_MENU_ITEM (parent_widget), menu_shell);
            }
        }

        switch (node->type)
        {
            case ITEM:
                create_menu_item (xml, toplevel, GTK_MENU_SHELL (menu_shell),
                                  node, effective_index (parent, node));
                break;
            case SEPARATOR:
                create_menu_separator (xml, toplevel, GTK_MENU_SHELL (menu_shell),
                                       node, effective_index (parent, node));
                break;
            default:
                g_return_if_reached ();
        }

        check_separators (parent, toplevel);
    }
    else
    {
        g_return_if_reached ();
    }
}


static void
toplevel_remove_node (G_GNUC_UNUSED MooUiXml *xml,
                      Toplevel *toplevel,
                      Node     *node)
{
    GtkWidget *widget;

    g_return_if_fail (node != toplevel->node);
    g_return_if_fail (node_is_ancestor (node, toplevel->node));

    widget = g_hash_table_lookup (toplevel->children, node);

    if (widget)
        gtk_widget_destroy (widget);
}


static void
update_widgets (MooUiXml       *xml,
                UpdateType      type,
                Node           *node)
{
    switch (type)
    {
        case UPDATE_ADD_NODE:
            SLIST_FOREACH (xml->priv->toplevels, l)
            {
                Toplevel *toplevel = l->data;

                if (node_is_ancestor (node, toplevel->node))
                    toplevel_add_node (xml, toplevel, node);
            }
            SLIST_FOREACH_END;
            break;

        case UPDATE_REMOVE_NODE:
            SLIST_FOREACH (xml->priv->toplevels, l)
            {
                Toplevel *toplevel = l->data;

                if (node_is_ancestor (toplevel->node, node))
                    xml_delete_toplevel (xml, toplevel);
                else if (node_is_ancestor (node, toplevel->node))
                    toplevel_remove_node (xml, toplevel, node);
            }
            SLIST_FOREACH_END;
            break;

        case UPDATE_CHANGE_NODE:
            g_warning ("implement me");
            break;

        default:
            g_return_if_reached ();
    }
}


static void
moo_ui_xml_finalize (GObject *object)
{
    MooUiXml *xml = MOO_UI_XML (object);

    SLIST_FOREACH (xml->priv->toplevels, t)
    {
        Toplevel *toplevel = t->data;
        GSList *widgets = hash_table_list_values (toplevel->children);

        SLIST_FOREACH (widgets, w)
        {
            GObject *widget = G_OBJECT (w->data);

            g_object_set_qdata (widget, NODE_QUARK, NULL);
            g_object_set_qdata (widget, TOPLEVEL_QUARK, NULL);
            g_signal_handlers_disconnect_by_func (widget, (gpointer) widget_destroyed, xml);
            g_signal_handlers_disconnect_by_func (widget, (gpointer) visibility_notify, xml);
        }
        SLIST_FOREACH_END;

        g_slist_free (widgets);
        toplevel_free (toplevel);
    }
    SLIST_FOREACH_END;

    SLIST_FOREACH (xml->priv->merged_ui, m)
    {
        Merge *merge = m->data;
        g_slist_free (merge->nodes);
        g_free (merge);
    }
    SLIST_FOREACH_END;

    g_slist_free (xml->priv->toplevels);
    g_slist_free (xml->priv->merged_ui);
    node_free (xml->priv->ui);

    g_free (xml->priv);
    xml->priv = NULL;

    G_OBJECT_CLASS(moo_ui_xml_parent_class)->finalize (object);
}


GType
moo_ui_node_get_type (void)
{
    static GType type = 0;

    if (G_UNLIKELY (!type))
        type = g_pointer_type_register_static ("MooUiNode");

    return type;
}
