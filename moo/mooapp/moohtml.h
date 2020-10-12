/*
 *   moohtml.h
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

#ifndef MOO_HTML_H
#define MOO_HTML_H

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define MOO_TYPE_HTML                   (_moo_html_get_type ())
#define MOO_HTML(object)                (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_HTML, MooHtml))
#define MOO_HTML_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_HTML, MooHtmlClass))
#define MOO_IS_HTML(object)             (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_HTML))
#define MOO_IS_HTML_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_HTML))
#define MOO_HTML_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_HTML, MooHtmlClass))

#define MOO_TYPE_HTML_TAG               (_moo_html_tag_get_type ())
#define MOO_HTML_TAG(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_HTML_TAG, MooHtmlTag))
#define MOO_HTML_TAG_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_HTML_TAG, MooHtmlTagClass))
#define MOO_IS_HTML_TAG(object)         (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_HTML_TAG))
#define MOO_IS_HTML_TAG_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_HTML_TAG))
#define MOO_HTML_TAG_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_HTML_TAG, MooHtmlTagClass))

typedef struct _MooHtml             MooHtml;
typedef struct _MooHtmlData         MooHtmlData;
typedef struct _MooHtmlClass        MooHtmlClass;
typedef struct _MooHtmlTag          MooHtmlTag;
typedef struct _MooHtmlTagClass     MooHtmlTagClass;
typedef struct _MooHtmlAttr         MooHtmlAttr;

struct _MooHtml
{
    GtkTextView parent;
    MooHtmlData *data;
};

struct _MooHtmlClass
{
    GtkTextViewClass parent_class;

    gboolean (*load_url)    (MooHtml    *html,
                             const char *url);
    void     (*hover_link)  (MooHtml    *html,
                             const char *link);
};

struct _MooHtmlTag
{
    GtkTextTag base;

    MooHtmlTag *parent;
    GHashTable *child_tags; /* char* -> MooHtmlTag* */

    MooHtmlAttr *attr;
    char *href;
};

struct _MooHtmlTagClass
{
    GtkTextTagClass base_class;
};


GType           _moo_html_get_type          (void) G_GNUC_CONST;
GType           _moo_html_tag_get_type      (void) G_GNUC_CONST;

gboolean        _moo_html_load_memory       (GtkTextView    *view,
                                             const char     *buffer,
                                             int             size,
                                             const char     *url,
                                             const char     *encoding);
gboolean        _moo_html_load_file         (GtkTextView    *view,
                                             const char     *file,
                                             const char     *encoding);

void            moo_text_view_set_markup    (GtkTextView    *view,
                                             const char     *markup);


G_END_DECLS

#endif /* MOO_HTML_H */
