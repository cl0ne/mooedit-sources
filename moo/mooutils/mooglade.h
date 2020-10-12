/*
 *   mooglade.h
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

#ifndef MOO_GLADE_H
#define MOO_GLADE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_GLADE_XML              (moo_glade_xml_get_type ())
#define MOO_GLADE_XML(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_GLADE_XML, MooGladeXML))
#define MOO_GLADE_XML_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_GLADE_XML, MooGladeXMLClass))
#define MOO_IS_GLADE_XML(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_GLADE_XML))
#define MOO_IS_GLADE_XML_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_GLADE_XML))
#define MOO_GLADE_XML_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_GLADE_XML, MooGladeXMLClass))

typedef struct _MooGladeXML        MooGladeXML;
typedef struct _MooGladeXMLPrivate MooGladeXMLPrivate;
typedef struct _MooGladeXMLClass   MooGladeXMLClass;


struct _MooGladeXML {
    GObject base;
    MooGladeXMLPrivate *priv;
};

struct _MooGladeXMLClass {
    GObjectClass base_class;
};


typedef GtkWidget* (*MooGladeCreateFunc)    (MooGladeXML    *xml,
                                             const char     *id,
                                             gpointer        data);
typedef gboolean   (*MooGladeSignalFunc)    (MooGladeXML    *xml,
                                             const char     *widget_id,
                                             GtkWidget      *widget,
                                             const char     *signal,
                                             const char     *handler,
                                             const char     *object,
                                             gpointer        data);
typedef gboolean   (*MooGladePropFunc)      (MooGladeXML    *xml,
                                             const char     *widget_id,
                                             GtkWidget      *widget,
                                             const char     *property,
                                             const char     *value,
                                             gpointer        data);


GType        moo_glade_xml_get_type         (void) G_GNUC_CONST;

MooGladeXML *moo_glade_xml_new_empty        (const char     *domain);

void         moo_glade_xml_register_type    (GType           type);
void         moo_glade_xml_map_class        (MooGladeXML    *xml,
                                             const char     *class_name,
                                             GType           use_type);
void         moo_glade_xml_map_id           (MooGladeXML    *xml,
                                             const char     *id,
                                             GType           use_type);
void         moo_glade_xml_map_custom       (MooGladeXML    *xml,
                                             const char     *id,
                                             MooGladeCreateFunc func,
                                             gpointer        data);
void         moo_glade_xml_set_signal_func  (MooGladeXML    *xml,
                                             MooGladeSignalFunc func,
                                             gpointer        data,
                                             GDestroyNotify  notify);
void         moo_glade_xml_set_prop_func    (MooGladeXML    *xml,
                                             MooGladePropFunc func,
                                             gpointer        data,
                                             GDestroyNotify  notify);

void         moo_glade_xml_set_property     (MooGladeXML    *xml,
                                             const char     *widget,
                                             const char     *prop_name,
                                             const char     *value);

gboolean     moo_glade_xml_parse_file       (MooGladeXML    *xml,
                                             const char     *file,
                                             const char     *root,
                                             GError        **error);
gboolean     moo_glade_xml_parse_memory     (MooGladeXML    *xml,
                                             const char     *buffer,
                                             int             size,
                                             const char     *root,
                                             GError        **error);
gboolean     moo_glade_xml_fill_widget      (MooGladeXML    *xml,
                                             GtkWidget      *target,
                                             const char     *buffer,
                                             int             size,
                                             const char     *target_name,
                                             GError        **error);

MooGladeXML *moo_glade_xml_new              (const char     *file,
                                             const char     *root,
                                             const char     *domain,
                                             GError        **error);
MooGladeXML *moo_glade_xml_new_from_buf     (const char     *buffer,
                                             int             size,
                                             const char     *root,
                                             const char     *domain,
                                             GError        **error);

gpointer     moo_glade_xml_get_widget       (MooGladeXML    *xml,
                                             const char     *id);
GtkWidget   *moo_glade_xml_get_root         (MooGladeXML    *xml);


G_END_DECLS

#endif /* MOO_GLADE_H */
