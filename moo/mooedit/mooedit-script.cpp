#include "mooedit/mooedit-script.h"
#include "mooedit/mootextview.h"
#include "mooedit/mootextbuffer.h"
#include "mooutils/mooutils.h"

/**
 * moo_edit_can_undo:
 **/
gboolean
moo_edit_can_undo (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), FALSE);
    return moo_text_buffer_can_undo (MOO_TEXT_BUFFER (moo_edit_get_buffer (doc)));
}

/**
 * moo_edit_can_redo:
 **/
gboolean
moo_edit_can_redo (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), FALSE);
    return moo_text_buffer_can_redo (MOO_TEXT_BUFFER (moo_edit_get_buffer (doc)));
}

/**
 * moo_edit_undo:
 **/
gboolean
moo_edit_undo (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), FALSE);
    return moo_text_view_undo (MOO_TEXT_VIEW (moo_edit_get_view (doc)));
}

/**
 * moo_edit_redo:
 **/
gboolean
moo_edit_redo (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), FALSE);
    return moo_text_view_redo (MOO_TEXT_VIEW (moo_edit_get_view (doc)));
}

/**
 * moo_edit_begin_non_undoable_action:
 **/
void
moo_edit_begin_non_undoable_action (MooEdit *doc)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    moo_text_buffer_begin_non_undoable_action (MOO_TEXT_BUFFER (moo_edit_get_buffer (doc)));
}

/**
 * moo_edit_begin_user_action:
 **/
void
moo_edit_begin_user_action (MooEdit *doc)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    gtk_text_buffer_begin_user_action (moo_edit_get_buffer (doc));
}

/**
 * moo_edit_end_user_action:
 **/
void
moo_edit_end_user_action (MooEdit *doc)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    gtk_text_buffer_end_user_action (moo_edit_get_buffer (doc));
}

/**
 * moo_edit_end_non_undoable_action:
 **/
void
moo_edit_end_non_undoable_action (MooEdit *doc)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    moo_text_buffer_end_non_undoable_action (MOO_TEXT_BUFFER (moo_edit_get_buffer (doc)));
}

/**
 * moo_edit_get_start_pos:
 *
 * Returns: (transfer full): Iterator which points to the beginning of the document.
 **/
GtkTextIter *
moo_edit_get_start_pos (MooEdit *doc)
{
    GtkTextIter iter;
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    gtk_text_buffer_get_start_iter (moo_edit_get_buffer (doc), &iter);
    return gtk_text_iter_copy (&iter);
}

/**
 * moo_edit_get_end_pos:
 *
 * Returns: (transfer full): Iterator which points to the end of the document,
 * i.e. the position past the last character in the document.
 **/
GtkTextIter *
moo_edit_get_end_pos (MooEdit *doc)
{
    GtkTextIter iter;
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    gtk_text_buffer_get_end_iter (moo_edit_get_buffer (doc), &iter);
    return gtk_text_iter_copy (&iter);
}

static void
get_iter_at_cursor (MooEdit     *doc,
                    GtkTextIter *iter)
{
    GtkTextBuffer *buffer = moo_edit_get_buffer (doc);
    gtk_text_buffer_get_iter_at_mark (buffer, iter, gtk_text_buffer_get_insert (buffer));
}

/**
 * moo_edit_get_cursor_pos:
 *
 * Returns: (transfer full): Iterator which points to the current cursor (insertion point) position.
 * In the case when text selection is not empty, it points to one of the ends of the selection.
 **/
GtkTextIter *
moo_edit_get_cursor_pos (MooEdit *doc)
{
    GtkTextIter iter;
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    get_iter_at_cursor (doc, &iter);
    return gtk_text_iter_copy (&iter);
}

/**
 * moo_edit_get_selection_start_pos:
 *
 * Returns: (transfer full): Iterator which points to the beginning of the current
 * text selection. If the selection is empty, it returns the current cursor position.
 **/
GtkTextIter *
moo_edit_get_selection_start_pos (MooEdit *doc)
{
    GtkTextIter iter;
    GtkTextBuffer *buffer;
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    buffer = moo_edit_get_buffer (doc);
    gtk_text_buffer_get_selection_bounds (buffer, &iter, NULL);
    return gtk_text_iter_copy (&iter);
}

/**
 * moo_edit_get_selection_end_pos:
 *
 * Returns: (transfer full): Iterator which points to the end of the current
 * text selection. If the selection is empty, it returns the current cursor position.
 **/
GtkTextIter *
moo_edit_get_selection_end_pos (MooEdit *doc)
{
    GtkTextIter iter;
    GtkTextBuffer *buffer;
    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);
    buffer = moo_edit_get_buffer (doc);
    gtk_text_buffer_get_selection_bounds (buffer, NULL, &iter);
    return gtk_text_iter_copy (&iter);
}

/**
 * moo_edit_set_cursor_pos:
 *
 * Move the text cursor to the given position. No text is selected
 * after this operation, use one of the select* methods if you need to select a range of text.
 **/
void
moo_edit_set_cursor_pos (MooEdit           *doc,
                         const GtkTextIter *pos)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    gtk_text_buffer_place_cursor (moo_edit_get_buffer (doc), pos);
}

/**
 * moo_edit_get_char_count:
 **/
int
moo_edit_get_char_count (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), 0);
    return gtk_text_buffer_get_char_count (moo_edit_get_buffer (doc));
}

/**
 * moo_edit_get_line_count:
 **/
int
moo_edit_get_line_count (MooEdit *doc)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), 0);
    return gtk_text_buffer_get_line_count (moo_edit_get_buffer (doc));
}

/**
 * moo_edit_get_line_at_cursor:
 *
 * Returns: (type index): %INDEXBASE-based number of the current cursor line.
 **/
int
moo_edit_get_line_at_cursor (MooEdit *doc)
{
    GtkTextIter iter;
    g_return_val_if_fail (MOO_IS_EDIT (doc), 0);
    get_iter_at_cursor (doc, &iter);
    return gtk_text_iter_get_line (&iter);
}

/**
 * moo_edit_get_line_at_pos:
 *
 * Returns: (type index): %INDEXBASE-based number of the line at the given position.
 **/
int
moo_edit_get_line_at_pos (MooEdit           *doc,
                          const GtkTextIter *pos)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), 0);
    return gtk_text_iter_get_line (pos);
}

/**
 * moo_edit_get_pos_at_line:
 *
 * @doc:
 * @line: (type index): %INDEXBASE-based line number.
 *
 * Returns: (transfer full): Iterator which points to the beginning of the given line.
 **/
GtkTextIter *
moo_edit_get_pos_at_line (MooEdit *doc,
                          int      line)
{
    GtkTextIter iter;
    GtkTextBuffer *buffer;

    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);

    buffer = moo_edit_get_buffer (doc);
    g_return_val_if_fail (line >= 0 && line <= gtk_text_buffer_get_line_count (buffer), NULL);

    gtk_text_buffer_get_iter_at_line (buffer, &iter, line);
    return gtk_text_iter_copy (&iter);
}

/**
 * moo_edit_get_pos_at_line_end:
 *
 * @doc:
 * @line: (type index): %INDEXBASE-based line number.
 *
 * Returns: (transfer full): Iterator which points to the end of the given line (i.e. the position
 * before the line end character(s)).
 **/
GtkTextIter *
moo_edit_get_pos_at_line_end (MooEdit *doc,
                              int      line)
{
    GtkTextIter iter;
    GtkTextBuffer *buffer;

    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);

    buffer = moo_edit_get_buffer (doc);
    g_return_val_if_fail (line >= 0 && line <= gtk_text_buffer_get_line_count (buffer), NULL);

    gtk_text_buffer_get_iter_at_line (buffer, &iter, line);
    if (!gtk_text_iter_ends_line (&iter))
        gtk_text_iter_forward_to_line_end (&iter);

    return gtk_text_iter_copy (&iter);
}

/**
 * moo_edit_get_char_at_pos:
 **/
gunichar
moo_edit_get_char_at_pos (MooEdit           *doc,
                          const GtkTextIter *pos)
{
    g_return_val_if_fail (MOO_IS_EDIT (doc), 0);
    return gtk_text_iter_get_char (pos);
}

/**
 * moo_edit_get_text:
 *
 * @doc:
 * @start: (allow-none) (default NULL)
 * @end: (allow-none) (default NULL)
 *
 * Returns: (type utf8): text between @start and @end. If @end is
 * missing then it returns text from @start to the end of document;
 * and if both @start and @end are missing then it returns whole
 * document content.
 **/
char *
moo_edit_get_text (MooEdit           *doc,
                   const GtkTextIter *start,
                   const GtkTextIter *end)
{
    GtkTextBuffer *buffer;
    GtkTextIter start_iter;
    GtkTextIter end_iter;

    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);

    buffer = moo_edit_get_buffer (doc);

    if (start)
        start_iter = *start;
    else
        gtk_text_buffer_get_start_iter (buffer, &start_iter);

    if (end)
        end_iter = *end;
    else
        gtk_text_buffer_get_end_iter (buffer, &end_iter);

    return gtk_text_buffer_get_slice (buffer, &start_iter, &end_iter, TRUE);
}

/**
 * moo_edit_get_line_text:
 *
 * @doc:
 * @line: (type index) (default -1): %INDEXBASE-based line number.
 *
 * Returns: (type utf8): text at line @line, not including line
 * end characters. If @line is missing, returns text at cursor line.
 **/
char *
moo_edit_get_line_text (MooEdit *doc,
                        int      line)
{
    GtkTextBuffer *buffer;
    GtkTextIter start, end;

    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);

    buffer = moo_edit_get_buffer (doc);

    if (line >= 0)
    {
        g_return_val_if_fail (line < gtk_text_buffer_get_line_count (buffer), NULL);
        gtk_text_buffer_get_iter_at_line (buffer, &start, line);
    }
    else
    {
        get_iter_at_cursor (doc, &start);
        gtk_text_iter_set_line_offset (&start, 0);
    }

    end = start;

    if (!gtk_text_iter_ends_line (&end))
        gtk_text_iter_forward_to_line_end (&end);

    return moo_edit_get_text (doc, &start, &end);
}

/**
 * moo_edit_get_line_text_at_pos:
 *
 * Returns: (type utf8): text at line which contains position @pos,
 * not including the line end character(s).
 **/
char *
moo_edit_get_line_text_at_pos (MooEdit           *doc,
                               const GtkTextIter *pos)
{
    return moo_edit_get_line_text (doc, gtk_text_iter_get_line (pos));
}

/**
 * moo_edit_set_text:
 *
 * @doc:
 * @text: (type const-utf8)
 **/
void
moo_edit_set_text (MooEdit    *doc,
                   const char *text)
{
    GtkTextBuffer *buffer;
    GtkTextIter start, end;

    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (text != NULL);

    buffer = moo_edit_get_buffer (doc);
    gtk_text_buffer_get_bounds (buffer, &start, &end);
    gtk_text_buffer_delete (buffer, &start, &end);
    gtk_text_buffer_insert (buffer, &start, text, -1);
}

/**
 * moo_edit_insert_text:
 *
 * @doc:
 * @text: (type const-utf8)
 * @where: (allow-none) (default NULL)
 *
 * Insert @text at position @where or at cursor position if @where is %NULL.
 **/
void
moo_edit_insert_text (MooEdit     *doc,
                      const char  *text,
                      GtkTextIter *where)
{
    GtkTextIter iter;
    GtkTextBuffer *buffer;

    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (text != NULL);

    buffer = moo_edit_get_buffer (doc);

    if (where)
        iter = *where;
    else
        gtk_text_buffer_get_iter_at_mark (buffer, &iter, gtk_text_buffer_get_insert (buffer));

    gtk_text_buffer_insert (buffer, &iter, text, -1);

    if (where)
        *where = iter;
}

/**
 * moo_edit_replace_text:
 *
 * @doc:
 * @start:
 * @end:
 * @text: (type const-utf8)
 **/
void
moo_edit_replace_text (MooEdit     *doc,
                       GtkTextIter *start,
                       GtkTextIter *end,
                       const char  *text)
{
    GtkTextBuffer *buffer;

    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (start != NULL);
    g_return_if_fail (end != NULL);
    g_return_if_fail (text != NULL);

    buffer = moo_edit_get_buffer (doc);
    gtk_text_buffer_delete (buffer, start, end);
    gtk_text_buffer_insert (buffer, start, text, -1);
    *end = *start;
}

/**
 * moo_edit_delete_text:
 **/
void
moo_edit_delete_text (MooEdit     *doc,
                      GtkTextIter *start,
                      GtkTextIter *end)
{
    GtkTextBuffer *buffer;

    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (start != NULL);
    g_return_if_fail (end != NULL);

    buffer = moo_edit_get_buffer (doc);
    gtk_text_buffer_delete (buffer, start, end);
}

/**
 * moo_edit_append_text:
 *
 * @doc:
 * @text: (type const-utf8)
 *
 * Append text to the end of document.
 **/
void
moo_edit_append_text (MooEdit    *doc,
                      const char *text)
{
    GtkTextBuffer *buffer;
    GtkTextIter iter;

    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (text != NULL);

    buffer = moo_edit_get_buffer (doc);
    gtk_text_buffer_get_end_iter (buffer, &iter);
    gtk_text_buffer_insert (buffer, &iter, text, -1);
}

/**
 * moo_edit_clear:
 *
 * Remove all text from document.
 **/
void
moo_edit_clear (MooEdit *doc)
{
    GtkTextBuffer *buffer;
    GtkTextIter start, end;

    g_return_if_fail (MOO_IS_EDIT (doc));

    buffer = moo_edit_get_buffer (doc);
    gtk_text_buffer_get_bounds (buffer, &start, &end);
    gtk_text_buffer_delete (buffer, &start, &end);
}

/**
 * moo_edit_cut:
 *
 * Cut selection to clipboard.
 **/
void
moo_edit_cut (MooEdit *doc)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    g_signal_emit_by_name (moo_edit_get_view (doc), "cut-clipboard");
}

/**
 * moo_edit_copy:
 *
 * Copy selection to clipboard.
 **/
void
moo_edit_copy (MooEdit        *doc)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    g_signal_emit_by_name (moo_edit_get_view (doc), "copy-clipboard");
}

/**
 * moo_edit_paste:
 *
 * Paste clipboard contents.
 **/
void
moo_edit_paste (MooEdit *doc)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    g_signal_emit_by_name (moo_edit_get_view (doc), "paste-clipboard");
}

/**
 * moo_edit_select_range:
 *
 * Select text from @start to @end.
 **/
void
moo_edit_select_range (MooEdit           *doc,
                       const GtkTextIter *start,
                       const GtkTextIter *end)
{
    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (start != NULL);
    g_return_if_fail (end != NULL);
    gtk_text_buffer_select_range (moo_edit_get_buffer (doc), start, end);
}

/**
 * moo_edit_select_lines:
 *
 * @doc:
 * @start: (type index)
 * @end: (type index) (default -1)
 **/
void
moo_edit_select_lines (MooEdit *doc,
                       int      start,
                       int      end)
{
    GtkTextBuffer *buffer;
    GtkTextIter start_iter, end_iter;

    g_return_if_fail (MOO_IS_EDIT (doc));

    buffer = moo_edit_get_buffer (doc);

    if (end < 0)
        end = start;

    if (start > end)
    {
        int tmp = start;
        start = end;
        end = tmp;
    }

    g_return_if_fail (start >= 0 && start < gtk_text_buffer_get_line_count (buffer));
    g_return_if_fail (end >= 0 && end < gtk_text_buffer_get_line_count (buffer));

    gtk_text_buffer_get_iter_at_line (buffer, &start_iter, start);
    gtk_text_buffer_get_iter_at_line (buffer, &end_iter, end);
    gtk_text_iter_forward_line (&end_iter);
    gtk_text_buffer_select_range (buffer, &start_iter, &end_iter);
}

/**
 * moo_edit_select_lines_at_pos:
 *
 * @doc:
 * @start:
 * @end: (allow-none) (default NULL)
 *
 * Select lines which span the range from @start to @end (including @end position).
 * If @end is %NULL, then it selects single line which contains position @start.
 **/
void
moo_edit_select_lines_at_pos (MooEdit           *doc,
                              const GtkTextIter *start,
                              const GtkTextIter *end)
{
    GtkTextBuffer *buffer;
    GtkTextIter start_iter, end_iter;

    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (start != NULL);

    buffer = moo_edit_get_buffer (doc);
    start_iter = *start;
    end_iter = end ? *end : *start;
    gtk_text_iter_order (&start_iter, &end_iter);
    gtk_text_iter_forward_line (&end_iter);
    gtk_text_buffer_select_range (buffer, &start_iter, &end_iter);
}

/**
 * moo_edit_select_all:
 **/
void
moo_edit_select_all (MooEdit *doc)
{
    GtkTextBuffer *buffer;
    GtkTextIter start, end;

    g_return_if_fail (MOO_IS_EDIT (doc));

    buffer = moo_edit_get_buffer (doc);
    gtk_text_buffer_get_bounds (buffer, &start, &end);
    gtk_text_buffer_select_range (buffer, &start, &end);
}

static void
get_selected_lines_bounds (GtkTextBuffer *buf,
                           GtkTextIter   *start,
                           GtkTextIter   *end,
                           gboolean      *cursor_at_next_line)
{
    if (cursor_at_next_line)
        *cursor_at_next_line = FALSE;

    gtk_text_buffer_get_selection_bounds (buf, start, end);

    gtk_text_iter_set_line_offset (start, 0);

    if (gtk_text_iter_starts_line (end) && !gtk_text_iter_equal (start, end))
    {
        gtk_text_iter_backward_line (end);
        if (cursor_at_next_line)
            *cursor_at_next_line = TRUE;
    }

    if (!gtk_text_iter_ends_line (end))
        gtk_text_iter_forward_to_line_end (end);

}

/**
 * moo_edit_get_selected_lines:
 *
 * Returns selected lines as a list of strings, one string for each line,
 * line terminator characters not included. If nothing is selected, then
 * line at cursor is returned.
 *
 * Returns: (type strv)
 **/
char **
moo_edit_get_selected_lines (MooEdit *doc)
{
    GtkTextIter start, end;
    GtkTextBuffer *buf;
    char *text;
    char **lines;

    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);

    buf = moo_edit_get_buffer (doc);
    get_selected_lines_bounds (buf, &start, &end, NULL);
    text = gtk_text_buffer_get_slice (buf, &start, &end, TRUE);
    lines = moo_splitlines (text);

    if (!lines)
    {
        lines = g_new (char*, 2);
        lines[0] = g_strdup ("");
        lines[1] = NULL;
    }

    g_free (text);
    return lines;
}

static char *
join_lines (char **strv)
{
    char **p;
    GString *string = g_string_new (NULL);
    for (p = strv; p && *p; ++p)
    {
        if (p != strv)
            g_string_append_c (string, '\n');
        g_string_append (string, *p);
    }
    return g_string_free (string, FALSE);
}

/**
 * moo_edit_replace_selected_lines:
 *
 * @doc:
 * @replacement: (type strv) (allow-none): list of lines to replace
 * selected lines with, maybe empty
 *
 * replace selected lines with @replacement. Similar to
 * moo_edit_replace_selected_text(), but selection is extended to include
 * whole lines. If nothing is selected, then line at cursor is replaced.
 **/
void
moo_edit_replace_selected_lines (MooEdit  *doc,
                                 char    **replacement)
{
    GtkTextBuffer *buf;
    GtkTextIter start, end;
    gboolean cursor_at_next_line;

    g_return_if_fail (MOO_IS_EDIT (doc));

    buf = moo_edit_get_buffer (doc);
    get_selected_lines_bounds (buf, &start, &end, &cursor_at_next_line);
    gtk_text_buffer_delete (buf, &start, &end);

    if (replacement)
    {
        char *text = join_lines (replacement);
        gtk_text_buffer_insert (buf, &start, text, -1);
        g_free (text);
    }

    if (cursor_at_next_line)
    {
        gtk_text_iter_forward_line (&start);
        gtk_text_buffer_place_cursor (buf, &start);
    }
}

/**
 * moo_edit_get_selected_text:
 *
 * Returns: (type utf8): selected text.
 **/
char *
moo_edit_get_selected_text (MooEdit *doc)
{
    GtkTextBuffer *buf;
    GtkTextIter start, end;

    g_return_val_if_fail (MOO_IS_EDIT (doc), NULL);

    buf = moo_edit_get_buffer (doc);
    gtk_text_buffer_get_selection_bounds(buf, &start, &end);
    return gtk_text_buffer_get_slice(buf, &start, &end, TRUE);
}

/**
 * moo_edit_delete_selected_text:
 **/
void
moo_edit_delete_selected_text (MooEdit *doc)
{
    moo_edit_replace_selected_text (doc, "");
}

/**
 * moo_edit_delete_selected_lines:
 *
 * Delete selected lines. Similar to moo_edit_delete_selected_text() but
 * selection is extended to include whole lines. If no text is selected then
 * line at cursor is deleted.
 **/
void
moo_edit_delete_selected_lines (MooEdit *doc)
{
    moo_edit_replace_selected_lines (doc, NULL);
}

/**
 * moo_edit_replace_selected_text:
 *
 * @doc:
 * @replacement: (type const-utf8)
 *
 * Replace selected text with string @replacement. If nothing is selected,
 * then @replacement is inserted at cursor.
 **/
void
moo_edit_replace_selected_text (MooEdit    *doc,
                                const char *replacement)
{
    GtkTextBuffer *buf;
    GtkTextIter start, end;

    g_return_if_fail (MOO_IS_EDIT (doc));
    g_return_if_fail (replacement != NULL);

    buf = moo_edit_get_buffer (doc);
    gtk_text_buffer_get_selection_bounds (buf, &start, &end);
    gtk_text_buffer_delete (buf, &start, &end);
    if (*replacement)
        gtk_text_buffer_insert (buf, &start, replacement, -1);
    gtk_text_buffer_place_cursor (buf, &start);
}

/**
 * moo_edit_has_selection:
 **/
gboolean
moo_edit_has_selection (MooEdit *doc)
{
    return moo_text_buffer_has_selection (MOO_TEXT_BUFFER (moo_edit_get_buffer (doc)));
}
