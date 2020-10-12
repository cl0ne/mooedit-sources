/*
 *   moouixml.h
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

#ifndef MOO_UI_XML_H
#define MOO_UI_XML_H

#include <mooutils/moomarkup.h>
#include <mooutils/mooactioncollection.h>

G_BEGIN_DECLS


typedef enum {
    MOO_UI_NODE_CONTAINER = 1,
    MOO_UI_NODE_WIDGET,
    MOO_UI_NODE_ITEM,
    MOO_UI_NODE_SEPARATOR,
    MOO_UI_NODE_PLACEHOLDER
} MooUiNodeType;

typedef enum {
    MOO_UI_NODE_ENABLE_EMPTY = 1 << 0
} MooUiNodeFlags;

typedef struct MooUiNode            MooUiNode;
typedef struct MooUiWidgetNode      MooUiWidgetNode;
typedef struct MooUiItemNode        MooUiItemNode;
typedef struct MooUiSeparatorNode   MooUiSeparatorNode;
typedef struct MooUiPlaceholderNode MooUiPlaceholderNode;


#define MOO_UI_NODE_STRUCT      \
    MooUiNodeType type;         \
    char *name;                 \
    MooUiNode *parent;          \
    GSList *children;           \
    guint flags : 1

struct MooUiNode {
    MOO_UI_NODE_STRUCT;
};

struct MooUiWidgetNode {
    MOO_UI_NODE_STRUCT;
};

struct MooUiSeparatorNode {
    MOO_UI_NODE_STRUCT;
};

struct MooUiPlaceholderNode {
    MOO_UI_NODE_STRUCT;
};

struct MooUiItemNode {
    MOO_UI_NODE_STRUCT;

    char *action;

    char *label;
    char *tooltip;
    char *stock_id;
    char *icon_stock_id;
};

typedef enum {
    MOO_UI_MENUBAR = 1,
    MOO_UI_MENU,
    MOO_UI_TOOLBAR
} MooUiWidgetType;


#define MOO_TYPE_UI_NODE             (moo_ui_node_get_type ())

#define MOO_TYPE_UI_XML              (moo_ui_xml_get_type ())
#define MOO_UI_XML(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_UI_XML, MooUiXml))
#define MOO_UI_XML_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_UI_XML, MooUiXmlClass))
#define MOO_IS_UI_XML(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_UI_XML))
#define MOO_IS_UI_XML_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_UI_XML))
#define MOO_UI_XML_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_UI_XML, MooUiXmlClass))


typedef struct _MooUiXml        MooUiXml;
typedef struct _MooUiXmlPrivate MooUiXmlPrivate;
typedef struct _MooUiXmlClass   MooUiXmlClass;

struct _MooUiXml
{
    GObject          object;
    MooUiXmlPrivate *priv;
};

struct _MooUiXmlClass
{
    GObjectClass parent_class;
};


GType       moo_ui_xml_get_type             (void) G_GNUC_CONST;
GType       moo_ui_node_get_type            (void) G_GNUC_CONST;

MooUiXml   *moo_ui_xml_new                  (void);

void        moo_ui_xml_add_ui_from_string   (MooUiXml       *xml,
                                             const char     *buffer,
                                             int             length);

MooUiNode  *moo_ui_xml_get_node             (MooUiXml       *xml,
                                             const char     *path);
MooUiNode  *moo_ui_xml_find_placeholder     (MooUiXml       *xml,
                                             const char     *name);
char       *moo_ui_node_get_path            (MooUiNode      *node);
MooUiNode  *moo_ui_node_get_child           (MooUiNode      *node,
                                             const char     *path);

GtkWidget  *moo_ui_xml_create_widget        (MooUiXml       *xml,
                                             MooUiWidgetType type,
                                             const char     *path,
                                             MooActionCollection *actions,
                                             GtkAccelGroup  *accel_group);
GtkWidget  *moo_ui_xml_get_widget           (MooUiXml       *xml,
                                             GtkWidget      *toplevel,
                                             const char     *path);

guint       moo_ui_xml_new_merge_id         (MooUiXml       *xml);

MooUiNode  *moo_ui_xml_add_item             (MooUiXml       *xml,
                                             guint           merge_id,
                                             const char     *parent_path,
                                             const char     *name,
                                             const char     *action,
                                             int             position);

void        moo_ui_xml_insert_after         (MooUiXml       *xml,
                                             guint           merge_id,
                                             MooUiNode      *parent,
                                             MooUiNode      *after,
                                             const char     *markup);
void        moo_ui_xml_insert_before        (MooUiXml       *xml,
                                             guint           merge_id,
                                             MooUiNode      *parent,
                                             MooUiNode      *before,
                                             const char     *markup);
void        moo_ui_xml_insert               (MooUiXml       *xml,
                                             guint           merge_id,
                                             MooUiNode      *parent,
                                             int             position,
                                             const char     *markup);

void        moo_ui_xml_insert_markup_after  (MooUiXml       *xml,
                                             guint           merge_id,
                                             const char     *parent_path,
                                             const char     *after,
                                             const char     *markup);
void        moo_ui_xml_insert_markup_before (MooUiXml       *xml,
                                             guint           merge_id,
                                             const char     *parent_path,
                                             const char     *before,
                                             const char     *markup);
void        moo_ui_xml_insert_markup        (MooUiXml       *xml,
                                             guint           merge_id,
                                             const char     *parent_path,
                                             int             position,
                                             const char     *markup);

void        moo_ui_xml_remove_ui            (MooUiXml       *xml,
                                             guint           merge_id);

void        moo_ui_xml_remove_node          (MooUiXml       *xml,
                                             MooUiNode      *node);


G_END_DECLS

#endif /* MOO_UI_XML_H */
