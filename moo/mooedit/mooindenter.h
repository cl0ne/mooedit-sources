/*
 *   mooindenter.h
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

#pragma once

#include <mooedit/mooedittypes.h>

G_BEGIN_DECLS


#define MOO_TYPE_INDENTER            (moo_indenter_get_type ())
#define MOO_INDENTER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_INDENTER, MooIndenter))
#define MOO_INDENTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_INDENTER, MooIndenterClass))
#define MOO_IS_INDENTER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_INDENTER))
#define MOO_IS_INDENTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_INDENTER))
#define MOO_INDENTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_INDENTER, MooIndenterClass))


typedef struct MooIndenter      MooIndenter;
typedef struct MooIndenterClass MooIndenterClass;

struct MooIndenter
{
    GObject parent;
    MooEdit *doc;
    gboolean use_tabs;
    guint tab_width;
    guint indent;
};

struct MooIndenterClass
{
    GObjectClass parent_class;

    void    (*config_changed)   (MooIndenter    *indenter,
                                 guint           setting_id,
                                 GParamSpec     *pspec);
    void    (*character)        (MooIndenter    *indenter,
                                 const char     *inserted_char,
                                 GtkTextIter    *where);
};


GType        moo_indenter_get_type              (void) G_GNUC_CONST;

MooIndenter *moo_indenter_new                   (MooEdit        *doc);

char        *moo_indenter_make_space            (MooIndenter    *indenter,
                                                 guint           len,
                                                 guint           start);

void         moo_indenter_character             (MooIndenter    *indenter,
                                                 const char     *inserted_char,
                                                 GtkTextIter    *where);
void         moo_indenter_tab                   (MooIndenter    *indenter,
                                                 GtkTextBuffer  *buffer);
void         moo_indenter_shift_lines           (MooIndenter    *indenter,
                                                 GtkTextBuffer  *buffer,
                                                 guint           first_line,
                                                 guint           last_line,
                                                 int             direction);

/* computes offset of start and returns offset or -1 if there are
   non-whitespace characters before start */
int          moo_iter_get_blank_offset          (const GtkTextIter  *iter,
                                                 guint               tab_width);

/* computes where cursor should jump when backspace is pressed

<-- result -->
              blah blah blah
                      blah
                     | offset
*/
guint        moo_text_iter_get_prev_stop        (const GtkTextIter *start,
                                                 guint              tab_width,
                                                 guint              offset,
                                                 gboolean           same_line);


G_END_DECLS

#ifdef __cplusplus

MOO_DEFINE_GOBJ_TRAITS(MooIndenter, MOO_TYPE_INDENTER);

#endif // __cplusplus
