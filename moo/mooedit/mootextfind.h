/*
 *   mootextfind.h
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

#ifndef MOO_TEXT_FIND_H
#define MOO_TEXT_FIND_H

#include <gtk/gtk.h>
#include <mooedit/mooedit-enums.h>
#include <mooutils/moohistorylist.h>
#include <mooglib/moo-glib.h>

G_BEGIN_DECLS


#define MOO_TYPE_FIND               (moo_find_get_type ())
#define MOO_FIND(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_FIND, MooFind))
#define MOO_FIND_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_FIND, MooFindClass))
#define MOO_IS_FIND(object)         (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_FIND))
#define MOO_IS_FIND_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_FIND))
#define MOO_FIND_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_FIND, MooFindClass))


typedef struct MooFind       MooFind;
typedef struct MooFindClass  MooFindClass;

struct MooFind
{
    GtkDialog base;
    struct MooFindBoxXml *xml;
    struct MooRegex *regex;
    guint replace : 1;
};

struct MooFindClass
{
    GtkDialogClass base_class;
};

typedef void (*MooFindMsgFunc) (const char *msg,
                                gpointer    data);


GType           moo_find_get_type           (void) G_GNUC_CONST;

void            moo_text_view_run_find      (GtkTextView    *view,
                                             MooFindMsgFunc  msg_func,
                                             gpointer        data);
void            moo_text_view_run_find_current_word
                                            (GtkTextView    *view,
                                             gboolean        forward,
                                             MooFindMsgFunc  msg_func,
                                             gpointer        data);
void            moo_text_view_run_replace   (GtkTextView    *view,
                                             MooFindMsgFunc  msg_func,
                                             gpointer        data);
void            moo_text_view_run_find_next (GtkTextView    *view,
                                             MooFindMsgFunc  msg_func,
                                             gpointer        data);
void            moo_text_view_run_find_prev (GtkTextView    *view,
                                             MooFindMsgFunc  msg_func,
                                             gpointer        data);
void            moo_text_view_run_goto_line (GtkTextView    *view);


G_END_DECLS

#endif /* MOO_TEXT_FIND_H */
