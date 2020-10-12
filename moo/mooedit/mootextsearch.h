/*
 *   mootextsearch.h
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

#ifndef MOO_TEXT_SEARCH_H
#define MOO_TEXT_SEARCH_H

#include <mooedit/mootextiter.h>
#include <mooedit/mooedit-enums.h>

G_BEGIN_DECLS


gboolean moo_text_search_forward            (const GtkTextIter      *start,
                                             const char             *str,
                                             MooTextSearchFlags      flags,
                                             GtkTextIter            *match_start,
                                             GtkTextIter            *match_end,
                                             const GtkTextIter      *end);
gboolean moo_text_search_backward           (const GtkTextIter      *start,
                                             const char             *str,
                                             MooTextSearchFlags      flags,
                                             GtkTextIter            *match_start,
                                             GtkTextIter            *match_end,
                                             const GtkTextIter      *end);

int      moo_text_replace_all               (GtkTextIter            *start,
                                             GtkTextIter            *end,
                                             const char             *text,
                                             const char             *replacement,
                                             MooTextSearchFlags      flags);


G_END_DECLS

#endif /* MOO_TEXT_SEARCH_H */
