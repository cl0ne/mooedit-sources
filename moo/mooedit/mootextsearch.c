/*
 *   mootextsearch.c
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

#include "mooedit/mootextsearch-private.h"
#include "gtksourceview/gtksourceview-api.h"
#include "mooutils/mooutils-misc.h"
#include <mooglib/moo-glib.h>
#include <string.h>


/* XXX lame */
static int
get_n_lines (GRegex *regex)
{
    const char *pattern;
    gboolean found = FALSE;
    guint bs = 0;

    pattern = g_regex_get_pattern (regex);

    while (*pattern && !found)
    {
        switch (*pattern++)
        {
            case '\\':
                bs ^= 1;
                break;
            case 'n':
            case 'r':
            case 'R':
                if (bs)
                    found = TRUE;
                else
                    bs = 0;
                break;
            default:
                bs = 0;
                break;
        }
    }

    return found ? 3 : 1;
}

MooRegex *
_moo_regex_new (GRegex *re)
{
    MooRegex *regex;

    g_return_val_if_fail (re != NULL, NULL);

    regex = g_slice_new (MooRegex);
    regex->re = g_regex_ref (re);
    regex->ref_count_ = 1;

    regex->n_lines = get_n_lines (re);

    return regex;
}

MooRegex *
_moo_regex_compile (const char         *pattern,
                    GRegexCompileFlags  compile_options,
                    GRegexMatchFlags    match_options,
                    GError            **error)
{
    GRegex *re;
    MooRegex *regex = NULL;

    g_return_val_if_fail (pattern != NULL, NULL);

    if ((re = g_regex_new (pattern, compile_options, match_options, error)))
    {
        regex = _moo_regex_new (re);
        g_regex_unref (re);
    }

    return regex;
}

MooRegex *
_moo_regex_ref (MooRegex *regex)
{
    g_return_val_if_fail (regex != NULL, NULL);
    regex->ref_count_++;
    return regex;
}

void
_moo_regex_unref (MooRegex *regex)
{
    g_return_if_fail (regex != NULL);

    if (!--regex->ref_count_)
    {
        g_regex_unref (regex->re);
        g_slice_free (MooRegex, regex);
    }
}


gboolean
_moo_text_search_regex_forward (const GtkTextIter      *search_start,
                                const GtkTextIter      *search_end,
                                MooRegex               *regex,
                                GtkTextIter            *match_start,
                                GtkTextIter            *match_end,
                                char                  **string,
                                int                    *match_offset,
                                int                    *match_len,
                                GMatchInfo            **match_infop)
{
    GtkTextIter start, end;
    GtkTextBuffer *buffer;
    int start_offset;
    char *text, *text_start;

    g_return_val_if_fail (search_start != NULL, FALSE);
    g_return_val_if_fail (match_start != NULL && match_end != NULL, FALSE);
    g_return_val_if_fail (regex != NULL, FALSE);

    buffer = gtk_text_iter_get_buffer (search_start);

    start = *search_start;
    start_offset = gtk_text_iter_get_line_offset (&start);
    if (start_offset)
        gtk_text_iter_set_line_offset (&start, 0);

    end = *search_start;
    gtk_text_iter_forward_lines (&end, regex->n_lines - 1);
    if (!gtk_text_iter_ends_line (&end))
        gtk_text_iter_forward_to_line_end (&end);

    while (TRUE)
    {
        GMatchInfo *match_info = NULL;

        text = gtk_text_buffer_get_slice (buffer, &start, &end, TRUE);
        text_start = g_utf8_offset_to_pointer (text, start_offset);

        if (g_regex_match_full (regex->re, text, -1, text_start - text, 0, &match_info, NULL))
        {
            int start_pos, end_pos;

            g_match_info_fetch_pos (match_info, 0, &start_pos, &end_pos);

            *match_start = start;
            gtk_text_iter_forward_chars (match_start, g_utf8_pointer_to_offset (text, text + start_pos));

            if (search_end && gtk_text_iter_compare (match_start, search_end) > 0)
            {
                g_match_info_free (match_info);
                g_free (text);
                return FALSE;
            }

            *match_end = *match_start;
            gtk_text_iter_forward_chars (match_end, g_utf8_pointer_to_offset (text + start_pos, text + end_pos));

            if (string)
                *string = text;
            else
                g_free (text);

            if (match_offset)
                *match_offset = start_pos;
            if (match_len)
                *match_len = end_pos - start_pos;

            if (match_infop)
                *match_infop = match_info;
            else
                g_match_info_free (match_info);

            return TRUE;
        }

        g_match_info_free (match_info);
        g_free (text);

        start = end;
        start_offset = 0;

        if (!gtk_text_iter_forward_line (&start))
            break;

        if (search_end && gtk_text_iter_compare (&start, search_end) > 0)
            break;

        end = start;
        gtk_text_iter_forward_lines (&end, regex->n_lines - 1);
        if (!gtk_text_iter_ends_line (&end))
            gtk_text_iter_forward_to_line_end (&end);
    }

    return FALSE;
}


static gboolean
find_last_match (GRegex            *regex,
                 const char        *text,
                 GRegexMatchFlags   flags,
                 int               *start_pos,
                 int               *end_pos,
                 GMatchInfo       **match_infop)
{
    gssize len, start;
    GMatchInfo *match_info = NULL;

    *start_pos = -1;
    len = strlen (text);
    start = 0;

    while (g_regex_match_full (regex, text, len, start, flags, &match_info, NULL))
    {
        g_match_info_fetch_pos (match_info, 0, start_pos, end_pos);

        start = *start_pos + 1;

        if (start >= len)
            break;

        g_match_info_free (match_info);
        match_info = NULL;
    }

    if (*start_pos >= 0 && match_infop)
        *match_infop = match_info;
    else if (match_info)
        g_match_info_free (match_info);

    return *start_pos >= 0;
}


gboolean
_moo_text_search_regex_backward (const GtkTextIter      *search_start,
                                 const GtkTextIter      *search_end,
                                 MooRegex               *regex,
                                 GtkTextIter            *match_start,
                                 GtkTextIter            *match_end,
                                 char                  **string,
                                 int                    *match_offset,
                                 int                    *match_len,
                                 GMatchInfo            **match_info)
{
    GtkTextIter slice_start, slice_end;
    GtkTextBuffer *buffer;
    char *text;
    GRegexMatchFlags flags;

    g_return_val_if_fail (search_start != NULL, FALSE);
    g_return_val_if_fail (match_start != NULL && match_end != NULL, FALSE);
    g_return_val_if_fail (regex != NULL, FALSE);

    buffer = gtk_text_iter_get_buffer (search_start);
    slice_start = *search_start;
    slice_end = slice_start;
    gtk_text_iter_backward_lines (&slice_start, regex->n_lines);
    flags = 0;

    if (!gtk_text_iter_ends_line (&slice_end))
        flags |= G_REGEX_MATCH_NOTEOL;

    while (TRUE)
    {
        int start_pos, end_pos;

        text = gtk_text_buffer_get_slice (buffer, &slice_start, &slice_end, TRUE);

        if (find_last_match (regex->re, text, flags, &start_pos, &end_pos, match_info))
        {
            *match_start = slice_start;
            gtk_text_iter_forward_chars (match_start, g_utf8_pointer_to_offset (text, text + start_pos));

            /* XXX how about not last match? */
            if (search_end && gtk_text_iter_compare (match_start, search_end) < 0)
            {
                if (match_info && *match_info)
                {
                    g_match_info_free (*match_info);
                    *match_info = NULL;
                }

                g_free (text);
                return FALSE;
            }

            *match_end = *match_start;
            gtk_text_iter_forward_chars (match_end, g_utf8_pointer_to_offset (text + start_pos, text + end_pos));

            if (string)
                *string = text;
            else
                g_free (text);

            if (match_offset)
                *match_offset = start_pos;
            if (match_len)
                *match_len = end_pos - start_pos;

            return TRUE;
        }

        g_free (text);

        slice_end = slice_start;
        flags = 0;

        if (gtk_text_iter_is_start (&slice_end))
            break;

        if (search_end && gtk_text_iter_compare (&slice_end, search_end) < 0)
            break;

        gtk_text_iter_backward_lines (&slice_start, regex->n_lines);
    }

    return FALSE;
}


static MooRegex *
get_regex (const char            *pattern,
           MooTextSearchFlags     flags,
           GError               **error)
{
    static MooRegex *saved_regex;
    static char *saved_pattern;
    static MooTextSearchFlags saved_flags;

    if (!saved_pattern || strcmp (saved_pattern, pattern) || saved_flags != flags)
    {
        GRegexCompileFlags re_flags = 0;

        if (saved_regex)
            _moo_regex_unref (saved_regex);
        g_free (saved_pattern);

        saved_pattern = g_strdup (pattern);
        saved_flags = flags;

        if (flags & MOO_TEXT_SEARCH_CASELESS)
            re_flags |= G_REGEX_CASELESS;

        saved_regex = _moo_regex_compile (saved_pattern,
                                          re_flags | G_REGEX_OPTIMIZE,
                                          0, error);

        if (!saved_regex)
        {
            g_free (saved_pattern);
            saved_pattern = NULL;
            return NULL;
        }
    }

    return saved_regex;
}


inline static gboolean
is_word_char (const GtkTextIter *iter)
{
    gunichar c = gtk_text_iter_get_char (iter);
    return c == '_' || g_unichar_isalnum (c);
}


static gboolean
is_whole_word (const GtkTextIter *start,
               const GtkTextIter *end)
{
    GtkTextIter s = *start;
    GtkTextIter e = *end;

    gtk_text_iter_order (&s, &e);

    if (!gtk_text_iter_starts_line (&s))
    {
        gtk_text_iter_backward_char (&s);
        if (is_word_char (&s))
            return FALSE;
    }

    if (!gtk_text_iter_ends_line (&e) && is_word_char (&e))
        return FALSE;

    return TRUE;
}


gboolean
moo_text_search_forward (const GtkTextIter      *start,
                         const char             *str,
                         MooTextSearchFlags      flags,
                         GtkTextIter            *match_start,
                         GtkTextIter            *match_end,
                         const GtkTextIter      *end)
{
    MooRegex *regex;
    GError *error = NULL;

    g_return_val_if_fail (start != NULL, FALSE);
    g_return_val_if_fail (str != NULL, FALSE);
    g_return_val_if_fail (match_start != NULL && match_end != NULL, FALSE);

    if (!(flags & MOO_TEXT_SEARCH_REGEX))
    {
        GtkSourceSearchFlags gs_flags = 0;
        GtkTextIter real_end, real_start;

        if (flags & MOO_TEXT_SEARCH_CASELESS)
            gs_flags |= GTK_SOURCE_SEARCH_CASE_INSENSITIVE;

        /* http://bugzilla.gnome.org/show_bug.cgi?id=321299 */
        if (!end || gtk_text_iter_is_end (end))
        {
            end = NULL;
        }
        else
        {
            real_end = *end;
            gtk_text_iter_forward_char (&real_end);
            end = &real_end;
        }

        if (!(flags & MOO_TEXT_SEARCH_WHOLE_WORDS))
            return gtk_source_iter_forward_search (start, str, gs_flags,
                                                   match_start, match_end, end);

        real_start = *start;

        while (gtk_source_iter_forward_search (&real_start, str, gs_flags,
                                               match_start, match_end, end))
        {
            if (is_whole_word (match_start, match_end))
                return TRUE;
            real_start = *match_end;
        }

        return FALSE;
    }

    regex = get_regex (str, flags, &error);

    if (!regex)
    {
        g_warning ("%s", moo_error_message (error));
        g_error_free (error);
        return FALSE;
    }

    return _moo_text_search_regex_forward (start, end, regex,
                                           match_start, match_end,
                                           NULL, NULL, NULL, NULL);
}


gboolean
moo_text_search_backward (const GtkTextIter      *start,
                          const char             *str,
                          MooTextSearchFlags      flags,
                          GtkTextIter            *match_start,
                          GtkTextIter            *match_end,
                          const GtkTextIter      *end)
{
    MooRegex *regex;
    GError *error = NULL;

    g_return_val_if_fail (start != NULL, FALSE);
    g_return_val_if_fail (str != NULL, FALSE);
    g_return_val_if_fail (match_start != NULL && match_end != NULL, FALSE);

    if (!(flags & MOO_TEXT_SEARCH_REGEX))
    {
        GtkSourceSearchFlags gs_flags = 0;
        GtkTextIter real_start;

        if (flags & MOO_TEXT_SEARCH_CASELESS)
            gs_flags |= GTK_SOURCE_SEARCH_CASE_INSENSITIVE;

        if (!(flags & MOO_TEXT_SEARCH_WHOLE_WORDS))
            return gtk_source_iter_backward_search (start, str, gs_flags,
                                                    match_start, match_end, end);

        real_start = *start;

        while (gtk_source_iter_backward_search (&real_start, str, gs_flags,
                                                match_start, match_end, end))
        {
            if (is_whole_word (match_start, match_end))
                return TRUE;
            real_start = *match_start;
        }

        return FALSE;
    }

    regex = get_regex (str, flags, &error);

    if (!regex)
    {
        g_warning ("%s", moo_error_message (error));
        g_error_free (error);
        return FALSE;
    }

    return _moo_text_search_regex_backward (start, end, regex,
                                            match_start, match_end,
                                            NULL, NULL, NULL, NULL);
}


static int
moo_text_replace_regex_all_real (GtkTextIter            *start,
                                 GtkTextIter            *end,
                                 MooRegex               *regex,
                                 const char             *replacement,
                                 gboolean                replacement_literal,
                                 MooTextReplaceFunc      func,
                                 gpointer                func_data)
{
    int count = 0;
    GtkTextMark *end_mark;
    GtkTextBuffer *buffer;
    MooTextReplaceResponse response;
    gboolean need_end_user_action = FALSE;
    char *freeme = NULL;
    const char *const_replacement = NULL;
    GError *error = NULL;
    gboolean was_zero_match = FALSE;

    g_return_val_if_fail (start != NULL, 0);
    g_return_val_if_fail (regex != NULL, 0);
    g_return_val_if_fail (replacement != NULL, 0);

    if (replacement_literal)
    {
        const_replacement = replacement;
    }
    else
    {
        gboolean has_references = FALSE;

        if (!g_regex_check_replacement (replacement, &has_references, &error))
        {
            g_warning ("%s", moo_error_message (error));
            g_error_free (error);
            return 0;
        }

        if (!has_references)
        {
            freeme = g_match_info_expand_references (NULL, replacement, &error);

            if (error)
            {
                g_warning ("%s", moo_error_message (error));
                g_error_free (error);
                return 0;
            }

            const_replacement = freeme;
        }
    }

    buffer = gtk_text_iter_get_buffer (start);

    if (end && !gtk_text_iter_is_end (end))
    {
        end_mark = gtk_text_buffer_create_mark (buffer, NULL, end, TRUE);
    }
    else
    {
        end = NULL;
        end_mark = NULL;
    }

    if (func)
    {
        response = MOO_TEXT_REPLACE_DO_REPLACE;
    }
    else
    {
        gtk_text_buffer_begin_user_action (buffer);
        need_end_user_action = TRUE;
        response = MOO_TEXT_REPLACE_ALL;
    }

    while (TRUE)
    {
        GtkTextIter match_start, match_end;
        char *freeme_here = NULL;
        const char *real_replacement;
        char *string;
        int match_len;
        GMatchInfo *match_info = NULL;

        if (!_moo_text_search_regex_forward (start, end, regex,
                                             &match_start, &match_end,
                                             &string, NULL, &match_len, &match_info))
            goto out;

        if (!match_len)
        {
            if (was_zero_match && gtk_text_iter_equal (&match_start, start))
            {
                was_zero_match = FALSE;
                g_free (string);
                g_match_info_free (match_info);

                if (!gtk_text_iter_forward_char (start))
                    goto out;

                continue;
            }

            was_zero_match = TRUE;
        }
        else
        {
            was_zero_match = FALSE;
        }

        if (const_replacement)
        {
            real_replacement = const_replacement;
            g_free (string);
        }
        else
        {
            freeme_here = g_match_info_expand_references (match_info, replacement, &error);
            g_free (string);

            if (!freeme_here)
            {
                g_warning ("%s", moo_error_message (error));
                g_error_free (error);
                g_match_info_free (match_info);
                goto out;
            }

            real_replacement = freeme_here;
        }

        if (response != MOO_TEXT_REPLACE_ALL)
        {
            response = func (NULL, regex, real_replacement, &match_start, &match_end, func_data);

            if (!response)
            {
                g_free (freeme_here);
                g_match_info_free (match_info);
                goto out;
            }
        }

        if (response != MOO_TEXT_REPLACE_SKIP && (match_len || *real_replacement))
        {
            count++;

            if (response == MOO_TEXT_REPLACE_ALL)
            {
                if (!need_end_user_action)
                {
                    gtk_text_buffer_begin_user_action (buffer);
                    need_end_user_action = TRUE;
                }
            }
            else
            {
                gtk_text_buffer_begin_user_action (buffer);
            }

            gtk_text_buffer_delete (buffer, &match_start, &match_end);
            gtk_text_buffer_insert (buffer, &match_end, real_replacement, -1);

            if (response != MOO_TEXT_REPLACE_ALL)
                gtk_text_buffer_end_user_action (buffer);
        }

        *start = match_end;

        if (was_zero_match && !*real_replacement)
        {
            if (gtk_text_iter_is_end (start))
            {
                g_free (freeme_here);
                g_match_info_free (match_info);
                goto out;
            }

            gtk_text_iter_forward_char (start);
            was_zero_match = FALSE;
        }

        if (end)
            gtk_text_buffer_get_iter_at_mark (buffer, end, end_mark);

        g_match_info_free (match_info);
        g_free (freeme_here);
    }

out:
    if (end_mark)
        gtk_text_buffer_delete_mark (buffer, end_mark);
    if (need_end_user_action)
        gtk_text_buffer_end_user_action (buffer);
    g_free (freeme);
    return count;
}


int
_moo_text_replace_regex_all (GtkTextIter            *start,
                             GtkTextIter            *end,
                             MooRegex               *regex,
                             const char             *replacement,
                             gboolean                replacement_literal)
{
    g_return_val_if_fail (start != NULL, 0);
    g_return_val_if_fail (regex != NULL, 0);
    g_return_val_if_fail (replacement != NULL, 0);

    return moo_text_replace_regex_all_real (start, end, regex, replacement,
                                            replacement_literal, NULL, NULL);
}


int
_moo_text_replace_regex_all_interactive (GtkTextIter            *start,
                                         GtkTextIter            *end,
                                         MooRegex               *regex,
                                         const char             *replacement,
                                         gboolean                replacement_literal,
                                         MooTextReplaceFunc      func,
                                         gpointer                func_data)
{
    g_return_val_if_fail (start != NULL, 0);
    g_return_val_if_fail (regex != NULL, 0);
    g_return_val_if_fail (replacement != NULL, 0);
    g_return_val_if_fail (func != NULL, 0);

    return moo_text_replace_regex_all_real (start, end, regex,
                                            replacement, replacement_literal,
                                            func, func_data);
}


int
moo_text_replace_all (GtkTextIter            *start,
                      GtkTextIter            *end,
                      const char             *text,
                      const char             *replacement,
                      MooTextSearchFlags      flags)
{
    int count = 0;
    GtkTextMark *end_mark;
    GtkTextBuffer *buffer;

    g_return_val_if_fail (start != NULL, 0);
    g_return_val_if_fail (text != NULL, 0);
    g_return_val_if_fail (text[0] != 0, 0);
    g_return_val_if_fail (replacement != NULL, 0);

    if (flags & MOO_TEXT_SEARCH_REGEX)
    {
        GError *error = NULL;
        MooRegex *regex = get_regex (text, flags, &error);

        if (!regex)
        {
            g_warning ("%s", moo_error_message (error));
            g_error_free (error);
            return 0;
        }

        return _moo_text_replace_regex_all (start, end, regex, replacement,
                                            flags & MOO_TEXT_SEARCH_REPL_LITERAL);
    }

    buffer = gtk_text_iter_get_buffer (start);
    gtk_text_buffer_begin_user_action (buffer);

    if (!end || gtk_text_iter_is_end (end))
        end = NULL;
    else
        gtk_text_iter_forward_char (end);

    if (end)
        end_mark = gtk_text_buffer_create_mark (buffer, NULL, end, TRUE);
    else
        end_mark = NULL;

    while (TRUE)
    {
        GtkTextIter match_start, match_end;

        if (!moo_text_search_forward (start, text, flags, &match_start, &match_end, end))
            goto out;

        count++;
        gtk_text_buffer_delete (buffer, &match_start, &match_end);
        gtk_text_buffer_insert (buffer, &match_end, replacement, -1);

        *start = match_end;

        if (end)
            gtk_text_buffer_get_iter_at_mark (buffer, end, end_mark);
    }

out:
    if (end_mark)
        gtk_text_buffer_delete_mark (buffer, end_mark);

    gtk_text_buffer_end_user_action (buffer);
    return count;
}


int
_moo_text_replace_all_interactive (GtkTextIter            *start,
                                   GtkTextIter            *end,
                                   const char             *text,
                                   const char             *replacement,
                                   MooTextSearchFlags      flags,
                                   MooTextReplaceFunc      func,
                                   gpointer                func_data)
{
    int count = 0;
    GtkTextMark *end_mark;
    GtkTextBuffer *buffer;
    MooTextReplaceResponse response = MOO_TEXT_REPLACE_DO_REPLACE;
    gboolean need_end_user_action = FALSE;

    g_return_val_if_fail (start != NULL, 0);
    g_return_val_if_fail (text != NULL, 0);
    g_return_val_if_fail (text[0] != 0, 0);
    g_return_val_if_fail (replacement != NULL, 0);
    g_return_val_if_fail (func != NULL, 0);

    if (flags & MOO_TEXT_SEARCH_REGEX)
    {
        GError *error = NULL;
        MooRegex *regex = get_regex (text, flags, &error);

        if (!regex)
        {
            g_warning ("%s", moo_error_message (error));
            g_error_free (error);
            return 0;
        }

        return _moo_text_replace_regex_all_interactive (start, end, regex, replacement,
                                                        flags & MOO_TEXT_SEARCH_REPL_LITERAL,
                                                        func, func_data);
    }

    buffer = gtk_text_iter_get_buffer (start);

    if (!end || gtk_text_iter_is_end (end))
        end = NULL;
    else
        gtk_text_iter_forward_char (end);

    if (end)
        end_mark = gtk_text_buffer_create_mark (buffer, NULL, end, TRUE);
    else
        end_mark = NULL;

    while (TRUE)
    {
        GtkTextIter match_start, match_end;

        if (!moo_text_search_forward (start, text, flags, &match_start, &match_end, end))
            goto out;

        if (response != MOO_TEXT_REPLACE_ALL)
        {
            response = func (text, NULL, replacement, &match_start, &match_end, func_data);

            if (!response)
                goto out;
        }

        if (response != MOO_TEXT_REPLACE_SKIP)
        {
            count++;

            if (response == MOO_TEXT_REPLACE_ALL)
            {
                if (!need_end_user_action)
                {
                    gtk_text_buffer_begin_user_action (buffer);
                    need_end_user_action = TRUE;
                }
            }
            else
            {
                gtk_text_buffer_begin_user_action (buffer);
            }

            gtk_text_buffer_delete (buffer, &match_start, &match_end);
            gtk_text_buffer_insert (buffer, &match_end, replacement, -1);

            if (response != MOO_TEXT_REPLACE_ALL)
                gtk_text_buffer_end_user_action (buffer);
        }

        *start = match_end;

        if (end)
            gtk_text_buffer_get_iter_at_mark (buffer, end, end_mark);
    }

out:
    if (end_mark)
        gtk_text_buffer_delete_mark (buffer, end_mark);
    if (need_end_user_action)
        gtk_text_buffer_end_user_action (buffer);
    return count;
}
