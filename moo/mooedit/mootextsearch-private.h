/*
 *   mootextsearch-private.h
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

#ifndef MOO_TEXT_SEARCH_PRIVATE_H
#define MOO_TEXT_SEARCH_PRIVATE_H

#include <mooedit/mootextsearch.h>
#include <mooglib/moo-glib.h>

G_BEGIN_DECLS


typedef struct MooRegex {
    GRegex *re;
    int n_lines;
    int ref_count_;
} MooRegex;

typedef enum /*< enum >*/
{
    MOO_TEXT_REPLACE_STOP = 0,
    MOO_TEXT_REPLACE_SKIP = 1,
    MOO_TEXT_REPLACE_DO_REPLACE = 2,
    MOO_TEXT_REPLACE_ALL = 3
} MooTextReplaceResponse;

/* replacement is evaluated in case of regex */
typedef MooTextReplaceResponse (*MooTextReplaceFunc) (const char         *text,
                                                      MooRegex           *regex,
                                                      const char         *replacement,
                                                      const GtkTextIter  *to_replace_start,
                                                      const GtkTextIter  *to_replace_end,
                                                      gpointer            user_data);

MooRegex   *_moo_regex_new          (GRegex             *regex);
MooRegex   *_moo_regex_compile      (const char         *pattern,
                                     GRegexCompileFlags  compile_options,
                                     GRegexMatchFlags    match_options,
                                     GError            **error);
MooRegex   *_moo_regex_ref          (MooRegex           *regex);
void        _moo_regex_unref        (MooRegex           *regex);

gboolean _moo_text_search_regex_forward     (const GtkTextIter      *start,
                                             const GtkTextIter      *end,
                                             MooRegex               *regex,
                                             GtkTextIter            *match_start,
                                             GtkTextIter            *match_end,
                                             char                  **string,
                                             int                    *match_offset,
                                             int                    *match_len,
                                             GMatchInfo            **match_info);
gboolean _moo_text_search_regex_backward    (const GtkTextIter      *start,
                                             const GtkTextIter      *end,
                                             MooRegex               *regex,
                                             GtkTextIter            *match_start,
                                             GtkTextIter            *match_end,
                                             char                  **string,
                                             int                    *match_offset,
                                             int                    *match_len,
                                             GMatchInfo            **match_info);

int      _moo_text_replace_regex_all        (GtkTextIter            *start,
                                             GtkTextIter            *end,
                                             MooRegex               *regex,
                                             const char             *replacement,
                                             gboolean                replacement_literal);

int      _moo_text_replace_regex_all_interactive
                                            (GtkTextIter            *start,
                                             GtkTextIter            *end,
                                             MooRegex               *regex,
                                             const char             *replacement,
                                             gboolean                replacement_literal,
                                             MooTextReplaceFunc      func,
                                             gpointer                func_data);
int      _moo_text_replace_all_interactive  (GtkTextIter            *start,
                                             GtkTextIter            *end,
                                             const char             *text,
                                             const char             *replacement,
                                             MooTextSearchFlags      flags,
                                             MooTextReplaceFunc      func,
                                             gpointer                func_data);


G_END_DECLS

#endif /* MOO_TEXT_SEARCH_PRIVATE_H */
