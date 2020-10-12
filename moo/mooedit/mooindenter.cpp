/*
 *   mooindenter.c
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

#include "mooedit/mooindenter.h"
#include "mooedit/mooedit.h"
#include "marshals.h"
#include <string.h>


/* XXX this doesn't take unicode control chars into account */

static void     sync_settings               (MooIndenter    *indenter);

enum {
    SETTING_USE_TABS,
    SETTING_INDENT_WIDTH,
    LAST_SETTING
};

static guint settings[LAST_SETTING];

/* MOO_TYPE_INDENTER */
G_DEFINE_TYPE (MooIndenter, moo_indenter, G_TYPE_OBJECT)


static void
moo_indenter_class_init (G_GNUC_UNUSED MooIndenterClass *klass)
{
    settings[SETTING_USE_TABS] = moo_edit_config_install_setting (
        g_param_spec_boolean ("indent-use-tabs", "indent-use-tabs", "indent-use-tabs",
                              TRUE, (GParamFlags) G_PARAM_READWRITE));
    moo_edit_config_install_alias ("indent-use-tabs", "use-tabs");

    settings[SETTING_INDENT_WIDTH] = moo_edit_config_install_setting (
        g_param_spec_uint ("indent-width", "indent-width", "indent-width",
                           1, G_MAXUINT, 8, (GParamFlags) G_PARAM_READWRITE));
}


static void
moo_indenter_init (MooIndenter *indent)
{
    indent->tab_width = 8;
    indent->use_tabs = TRUE;
    indent->indent = 8;
}


MooIndenter *
moo_indenter_new (MooEdit *doc)
{
    MooIndenter *indent;

    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);

    indent = g::object_new<MooIndenter>();
    indent->doc = doc;

    return indent;
}


/*****************************************************************************/
/* Default implementation
 */

char *
moo_indenter_make_space (MooIndenter    *indenter,
                         guint           len,
                         guint           start)
{
    guint tabs, spaces, delta;
    guint tab_width;
    char *string;

    g_return_val_if_fail (MOO_IS_INDENTER (indenter), NULL);

    sync_settings (indenter);

    tab_width = indenter->tab_width;

    if (!len)
        return NULL;

    if (!indenter->use_tabs)
        return g_strnfill (len, ' ');

    delta = start % tab_width;

    if (!delta)
    {
        tabs = len / tab_width;
        spaces = len % tab_width;
    }
    else if (len < tab_width - delta)
    {
        tabs = 0;
        spaces = len;
    }
    else if (len == tab_width - delta)
    {
        tabs = 1;
        spaces = 0;
    }
    else
    {
        len -= tab_width - delta;
        tabs = len / tab_width + 1;
        spaces = len % tab_width;
    }

    string = g_new (char, tabs + spaces + 1);
    string[tabs + spaces] = 0;

    if (tabs)
        memset (string, '\t', tabs);
    if (spaces)
        memset (string + tabs, ' ', spaces);

    return string;
}


/* computes amount of leading white space on the given line;
   returns TRUE if line contains some non-whitespace chars;
   if returns TRUE, then iter points to the first non-white-space char,
   otherwise it points to the end of line */
static gboolean
compute_line_offset (GtkTextIter    *dest,
                     guint           tab_width,
                     guint          *offsetp)
{
    guint offset = 0;
    GtkTextIter iter = *dest;
    gboolean retval = FALSE;

    while (!gtk_text_iter_ends_line (&iter))
    {
        gunichar c = gtk_text_iter_get_char (&iter);

        if (c == ' ')
        {
            offset += 1;
        }
        else if (c == '\t')
        {
            guint add = tab_width - offset % tab_width;
            offset += add;
        }
        else
        {
            retval = TRUE;
            break;
        }

        gtk_text_iter_forward_char (&iter);
    }

    *offsetp = offset;
    *dest = iter;
    return retval;
}


void
moo_indenter_character (MooIndenter *indenter,
                        const char  *inserted_char,
                        GtkTextIter *where)
{
    char *indent_string = NULL;
    GtkTextBuffer *buffer = gtk_text_iter_get_buffer (where);
    guint offset;
    GtkTextIter iter;

    g_return_if_fail (MOO_IS_INDENTER (indenter));
    g_return_if_fail (inserted_char != NULL);

    if (*inserted_char != '\n')
        return;

    buffer = gtk_text_iter_get_buffer (where);
    g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));

    sync_settings (indenter);

    iter = *where;
    gtk_text_iter_backward_line (&iter);
    compute_line_offset (&iter, indenter->tab_width, &offset);

    if (offset)
    {
        indent_string = moo_indenter_make_space (indenter, offset, 0);
        gtk_text_buffer_insert (buffer, where, indent_string, -1);
        g_free (indent_string);
    }
}


/* computes offset of start and returns offset or -1 if there are
   non-whitespace characters before start */
int
moo_iter_get_blank_offset (const GtkTextIter  *start,
                           guint               tab_width)
{
    GtkTextIter iter;
    guint offset;

    if (gtk_text_iter_starts_line (start))
        return 0;

    iter = *start;
    gtk_text_iter_set_line_offset (&iter, 0);
    offset = 0;

    while (gtk_text_iter_compare (&iter, start))
    {
        gunichar c = gtk_text_iter_get_char (&iter);

        if (c == ' ')
        {
            offset += 1;
        }
        else if (c == '\t')
        {
            guint add = tab_width - offset % tab_width;
            offset += add;
        }
        else
        {
            return -1;
        }

        gtk_text_iter_forward_char (&iter);
    }

    return offset;
}


/* computes where cursor should jump when backspace is pressed

<-- result -->
              blah blah blah
                      blah
                     | offset
*/
guint
moo_text_iter_get_prev_stop (const GtkTextIter *start,
                             guint              tab_width,
                             guint              offset,
                             gboolean           same_line)
{
    GtkTextIter iter;
    guint indent;

    iter = *start;
    gtk_text_iter_set_line_offset (&iter, 0);

    if (!same_line && !gtk_text_iter_backward_line (&iter))
        return 0;

    do
    {
        if (compute_line_offset (&iter, tab_width, &indent) && indent <= offset)
            return indent;
        else if (!gtk_text_iter_get_line (&iter))
            return 0;
    }
    while (gtk_text_iter_backward_line (&iter));

    return 0;
}


/* computes visual offset at iter, amount of white space before the iter,
   and makes iter to point to the beginning of the white space, e.g. for
blah   wefwefw
       |
   it would set offset == 7, white_space == 3, and set iter to the first
   space after 'blah'
*/
static void
iter_get_visual_offset (GtkTextIter *iter,
                        guint        tab_width,
                        int         *offsetp,
                        int         *white_spacep)
{
    GtkTextIter start, white_space_start = {0};
    guint offset, white_space;

    start = *iter;
    gtk_text_iter_set_line_offset (&start, 0);
    offset = 0;
    white_space = 0;

    while (gtk_text_iter_compare (&start, iter))
    {
        gunichar c = gtk_text_iter_get_char (&start);

        if (c == ' ' || c == '\t')
        {
            if (!white_space)
                white_space_start = start;
        }
        else
        {
            white_space = 0;
        }

        if (c == '\t')
        {
            guint add = tab_width - offset % tab_width;
            offset += add;
            white_space += add;
        }
        else if (c == ' ')
        {
            offset += 1;
            white_space += 1;
        }
        else
        {
            offset += 1;
        }

        gtk_text_iter_forward_char (&start);
    }

    *offsetp = offset;
    *white_spacep = white_space;
    if (white_space)
        *iter = white_space_start;
}


void
moo_indenter_tab (MooIndenter    *indenter,
                  GtkTextBuffer  *buffer)
{
    GtkTextIter insert, start;
    int offset, new_offset, white_space;
    guint tab_width;
    guint indent;
    char *text = NULL;

    sync_settings (indenter);

    tab_width = indenter->tab_width;
    indent = indenter->indent;

    gtk_text_buffer_get_iter_at_mark (buffer, &insert, gtk_text_buffer_get_insert (buffer));

    start = insert;
    iter_get_visual_offset (&start, tab_width, &offset, &white_space);

    new_offset = offset + (indent - offset % indent);
    text = moo_indenter_make_space (indenter,
                                    new_offset - offset + white_space,
                                    offset - white_space);

    gtk_text_buffer_delete (buffer, &start, &insert);
    gtk_text_buffer_insert (buffer, &start, text, -1);

    g_free (text);
}


static void
shift_line_forward (MooIndenter   *indenter,
                    GtkTextBuffer *buffer,
                    GtkTextIter   *iter)
{
    char *text;
    guint offset;
    GtkTextIter start;

    if (!compute_line_offset (iter, indenter->tab_width, &offset))
        return;

    if (offset)
    {
        start = *iter;
        gtk_text_iter_set_line_offset (&start, 0);
        gtk_text_buffer_delete (buffer, &start, iter);
    }

    text = moo_indenter_make_space (indenter, offset + indenter->indent, 0);

    if (text)
        gtk_text_buffer_insert (buffer, iter, text, -1);

    g_free (text);
}


static void
shift_line_backward (MooIndenter   *indenter,
                     GtkTextBuffer *buffer,
                     GtkTextIter   *iter)
{
    GtkTextIter end;
    int deleted;
    gunichar c;

    gtk_text_iter_set_line_offset (iter, 0);
    end = *iter;
    deleted = 0;

    while (TRUE)
    {
        if (gtk_text_iter_ends_line (&end))
            break;

        c = gtk_text_iter_get_char (&end);

        if (c == ' ')
        {
            gtk_text_iter_forward_char (&end);
            deleted += 1;
        }
        else if (c == '\t')
        {
            gtk_text_iter_forward_char (&end);
            deleted += indenter->tab_width;
        }
        else
        {
            break;
        }

        if (deleted >= (int) indenter->indent)
            break;
    }

    gtk_text_buffer_delete (buffer, iter, &end);

    deleted -= indenter->indent;

    if (deleted > 0)
    {
        char *text = moo_indenter_make_space (indenter, deleted, 0);
        gtk_text_buffer_insert (buffer, iter, text, -1);
        g_free (text);
    }
}


void
moo_indenter_shift_lines (MooIndenter    *indenter,
                          GtkTextBuffer  *buffer,
                          guint           first_line,
                          guint           last_line,
                          int             direction)
{
    guint i;
    GtkTextIter iter;

    sync_settings (indenter);

    for (i = first_line; i <= last_line; ++i)
    {
        gtk_text_buffer_get_iter_at_line (buffer, &iter, i);
        if (direction > 0)
            shift_line_forward (indenter, buffer, &iter);
        else
            shift_line_backward (indenter, buffer, &iter);
    }
}


static void
sync_settings (MooIndenter *indenter)
{
    g_return_if_fail (MOO_IS_EDIT (indenter->doc));
    indenter->use_tabs = moo_edit_config_get_bool (indenter->doc->config, "indent-use-tabs");
    indenter->indent = moo_edit_config_get_uint (indenter->doc->config, "indent-width");
    indenter->tab_width = moo_edit_config_get_uint (indenter->doc->config, "tab-width");
}
