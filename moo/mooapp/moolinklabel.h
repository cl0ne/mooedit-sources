/*
 *   moolinklabel.h
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

#ifndef MOO_LINK_LABEL_H
#define MOO_LINK_LABEL_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_LINK_LABEL             (_moo_link_label_get_type ())
#define MOO_LINK_LABEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOO_TYPE_LINK_LABEL, MooLinkLabel))
#define MOO_LINK_LABEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_LINK_LABEL, MooLinkLabelClass))
#define MOO_IS_LINK_LABEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOO_TYPE_LINK_LABEL))
#define MOO_IS_LINK_LABEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_LINK_LABEL))
#define MOO_LINK_LABEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_LINK_LABEL, MooLinkLabelClass))

typedef struct _MooLinkLabel        MooLinkLabel;
typedef struct _MooLinkLabelPrivate MooLinkLabelPrivate;
typedef struct _MooLinkLabelClass   MooLinkLabelClass;

struct _MooLinkLabel {
    GtkLabel parent;
    MooLinkLabelPrivate *priv;
};

struct _MooLinkLabelClass {
    GtkLabelClass parent_class;

    void (*activate) (MooLinkLabel  *label,
                      const char    *url);
};


GType        _moo_link_label_get_type   (void) G_GNUC_CONST;

GtkWidget   *_moo_link_label_new        (const char     *text,
                                         const char     *url);

void         _moo_link_label_set_text   (MooLinkLabel   *label,
                                         const char     *text);
void         _moo_link_label_set_url    (MooLinkLabel   *label,
                                         const char     *url);


G_END_DECLS

#endif /* MOO_LINK_LABEL_H */
