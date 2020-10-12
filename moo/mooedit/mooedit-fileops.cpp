/*
 *   mooedit-fileops.c
 *
 *   Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
 *   Copyright (C) 2014 by Ulrich Eckhardt <ulrich.eckhardt@base-42.de>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mooedit/mooedit-private.h"
#include "mooedit/mooeditor-impl.h"
#include "mooedit/mooedit-fileops.h"
#include "mooedit/mooeditdialogs.h"
#include "mooedit/mootextbuffer.h"
#include "mooedit/mooeditprefs.h"
#include "mooutils/moofileicon.h"
#include "mooutils/moofilewatch.h"
#include "mooutils/mooencodings.h"
#include "mooutils/mooi18n.h"
#include "mooutils/mootype-macros.h"
#include "mooutils/mooutils.h"
#include "mooutils/mooutils-fs.h"
#include "mooutils/moocompat.h"
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>

#include <mooglib/moo-glib.h>

#define ENCODING_LOCALE "LOCALE"

MOO_DEFINE_QUARK (MooEditFileErrorQuark, _moo_edit_file_error_quark)

static GSList *UNTITLED = NULL;
static GHashTable *UNTITLED_NO = NULL;

static void     block_buffer_signals        (MooEdit        *edit);
static void     unblock_buffer_signals      (MooEdit        *edit);
static void     check_file_status           (MooEdit        *edit);
static void     file_modified_on_disk       (MooEdit        *edit);
static void     file_deleted                (MooEdit        *edit);
static void     add_status                  (MooEdit        *edit,
                                             MooEditStatus   s);

static void     moo_edit_load_text          (MooEdit        *edit,
                                             GFile          *file,
                                             const char     *encoding,
                                             const char     *text);
static gboolean moo_edit_reload_local       (MooEdit        *edit,
                                             const char     *encoding,
                                             GError        **error);
static gboolean moo_edit_save_local         (MooEdit        *edit,
                                             GFile          *file,
                                             const char     *encoding,
                                             MooEditSaveFlags flags,
                                             GError        **error);
static gboolean moo_edit_save_copy_local    (MooEdit        *edit,
                                             GFile          *file,
                                             const char     *encoding,
                                             MooEditSaveFlags flags,
                                             GError        **error);
static void     _moo_edit_start_file_watch  (MooEdit        *edit);

static char    *moo_convert_file_data_to_utf8   (const char     *data,
                                                 gsize           len,
                                                 const char     *encoding,
                                                 const char     *cached_encoding,
                                                 char          **used_enc);
static gboolean encoding_needs_bom_save         (const char     *enc,
                                                 const char    **enc_no_bom,
                                                 const char    **bom,
                                                 gsize          *bom_len);
static gboolean encoding_is_utf8                (const char     *encoding);

static gboolean check_regular               (GFile          *file,
                                             GError        **error);

gboolean
_moo_is_file_error_cancelled (GError *error)
{
    return error && error->domain == MOO_EDIT_FILE_ERROR &&
           error->code == MOO_EDIT_FILE_ERROR_CANCELLED;
}


static const char *
normalize_encoding (const char *encoding,
                    gboolean    for_save)
{
    if (!encoding || !encoding[0] || !strcmp (encoding, MOO_ENCODING_AUTO))
        encoding = for_save ? MOO_ENCODING_UTF8 : NULL;
    return encoding;
}


gboolean
_moo_edit_file_is_new (GFile *file)
{
    gboolean is_new;
    char *filename;
    g_return_val_if_fail (G_IS_FILE (file), FALSE);
    filename = g_file_get_path (file);
    g_return_val_if_fail (filename != NULL, FALSE);
    is_new = !g_file_test (filename, G_FILE_TEST_EXISTS);
    g_free (filename);
    return is_new;
}


static gboolean
load_file_contents (GFile   *gfile,
                    char   **data,
                    gsize   *data_len,
                    GError **error)
{
    char *path = NULL;
    gboolean retval = FALSE;

    if (!check_regular (gfile, error))
        return FALSE;

    if (!(path = g_file_get_path (gfile)))
    {
        g_set_error (error, MOO_EDIT_FILE_ERROR,
                     MOO_EDIT_FILE_ERROR_NOT_IMPLEMENTED,
                     "Loading remote files is not implemented");
        return FALSE;
    }

    retval = g_file_get_contents (path, data, data_len, error);

    g_free (path);
    return retval;
}

static char *
convert_file_data_to_utf8_with_prompt (const char *data,
                                       gsize       data_len,
                                       GFile      *file,
                                       const char *encoding,
                                       const char *cached_encoding,
                                       char      **used_encoding)
{
    char *freeme = NULL;
    char *text_utf8 = NULL;
    char *new_encoding = NULL;

    while (TRUE)
    {
        MooEditTryEncodingResponse response;

        text_utf8 = moo_convert_file_data_to_utf8 (data, data_len, encoding, cached_encoding, &new_encoding);

        if (text_utf8)
            break;

        g_free (new_encoding);
        new_encoding = NULL;

        response = _moo_edit_try_encoding_dialog (file, encoding, &new_encoding);

        switch (response)
        {
            case MOO_EDIT_TRY_ENCODING_RESPONSE_CANCEL:
                g_free (new_encoding);
                new_encoding = NULL;
                break;

            case MOO_EDIT_TRY_ENCODING_RESPONSE_TRY_ANOTHER:
                g_assert (new_encoding != NULL);
                break;
        }

        if (!new_encoding)
            break;

        g_free (freeme);
        freeme = new_encoding;
        encoding = normalize_encoding (new_encoding, FALSE);
        cached_encoding = NULL;
    }

    *used_encoding = g_strdup (new_encoding);

    g_free (freeme);
    return text_utf8;
}

gboolean
_moo_edit_load_file (MooEdit      *edit,
                     GFile        *file,
                     const char   *encoding,
                     const char   *cached_encoding,
                     GError      **error)
{
    char *freeme1 = NULL;
    char *freeme2 = NULL;
    gboolean result = FALSE;
    GError *error_here = NULL;
    char *data = NULL;
    gsize data_len = 0;
    char *data_utf8 = NULL;
    char *used_encoding = NULL;

    moo_return_error_if_fail (MOO_IS_EDIT (edit));
    moo_return_error_if_fail (G_IS_FILE (file));
    moo_return_error_if_fail (!MOO_EDIT_IS_BUSY (edit));

    encoding = freeme1 = g_strdup (normalize_encoding (encoding, FALSE));
    cached_encoding = freeme2 = cached_encoding ? g_strdup (normalize_encoding (cached_encoding, FALSE)) : NULL;

    if (!load_file_contents (file, &data, &data_len, &error_here))
        goto done;

    data_utf8 = convert_file_data_to_utf8_with_prompt (data, data_len, file, encoding, cached_encoding, &used_encoding);

    if (data_utf8 == NULL)
    {
        error_here = g_error_new (MOO_EDIT_FILE_ERROR,
                                  MOO_EDIT_FILE_ERROR_CANCELLED,
                                  "Cancelled");
        goto done;
    }

    moo_edit_load_text (edit, file, used_encoding, data_utf8);
    result = TRUE;

done:
    if (!result)
        _moo_edit_stop_file_watch (edit);

    if (error_here)
        g_propagate_error (error, error_here);

    g_free (used_encoding);
    g_free (data_utf8);
    g_free (data);
    g_free (freeme1);
    g_free (freeme2);
    return result;
}


gboolean
_moo_edit_reload_file (MooEdit    *edit,
                       const char *encoding,
                       GError    **error)
{
    GError *error_here = NULL;
    gboolean result;

    moo_return_error_if_fail (MOO_IS_EDIT (edit));

    result = moo_edit_reload_local (edit, encoding, &error_here);

    if (error_here)
        g_propagate_error (error, error_here);

    return result;
}


gboolean
_moo_edit_save_file (MooEdit        *edit,
                     GFile          *file,
                     const char     *encoding,
                     MooEditSaveFlags flags,
                     GError        **error)
{
    GError *error_here = NULL;
    char *encoding_copy;
    gboolean result;

    moo_return_error_if_fail (MOO_IS_EDIT (edit));
    moo_return_error_if_fail (G_IS_FILE (file));

    encoding_copy = g_strdup (normalize_encoding (encoding, TRUE));

    result = moo_edit_save_local (edit, file, encoding_copy, flags, &error_here);

    if (error_here)
        g_propagate_error (error, error_here);

    g_free (encoding_copy);
    return result;
}


gboolean
_moo_edit_save_file_copy (MooEdit        *edit,
                          GFile          *file,
                          const char     *encoding,
                          MooEditSaveFlags flags,
                          GError        **error)
{
    char *encoding_copy;
    gboolean result;

    moo_return_error_if_fail (MOO_IS_EDIT (edit));
    moo_return_error_if_fail (G_IS_FILE (file));

    encoding_copy = g_strdup (normalize_encoding (encoding, TRUE));

    result = moo_edit_save_copy_local (edit, file, encoding_copy, flags, error);

    g_free (encoding_copy);
    return result;
}


static void
set_encoding_error (GError **error)
{
    GError *tmp_error = NULL;
    g_set_error (&tmp_error, MOO_EDIT_FILE_ERROR,
                 MOO_EDIT_FILE_ERROR_ENCODING,
                 "%s", *error ? (*error)->message : "ERROR");
    g_clear_error (error);
    g_propagate_error (error, tmp_error);
}


/**
 * moo_edit_get_line_end_type:
 **/
MooLineEndType
moo_edit_get_line_end_type (MooEdit *edit)
{
    g_return_val_if_fail (MOO_IS_EDIT (edit), MOO_LE_NATIVE);
    if (edit->priv->line_end_type == MOO_LE_NONE)
        return MOO_LE_NATIVE;
    else
        return edit->priv->line_end_type;
}

static void
moo_edit_set_line_end_type_full (MooEdit        *edit,
                                 MooLineEndType  le,
                                 gboolean        quiet)
{
    g_return_if_fail (MOO_IS_EDIT (edit));
    g_return_if_fail (le > 0);

    if (edit->priv->line_end_type != le)
    {
        edit->priv->line_end_type = le;
        if (!quiet)
            g_object_notify (G_OBJECT (edit), "line-end-type");
    }
}

/**
 * moo_edit_set_line_end_type:
 **/
void
moo_edit_set_line_end_type (MooEdit        *edit,
                            MooLineEndType  le)
{
    g_return_if_fail (MOO_IS_EDIT (edit));
    moo_edit_set_line_end_type_full (edit, le, FALSE);
}


/***************************************************************************/
/* File loading
 */

static void do_load_text    (MooEdit    *edit,
                             const char *text);

static GSList *
get_encodings (void)
{
    const char *encodings;
    char **raw, **p;
    GSList *result;

    encodings = moo_prefs_get_string (moo_edit_setting (MOO_EDIT_PREFS_ENCODINGS));

    if (!encodings || !encodings[0])
        encodings = _moo_get_default_encodings ();

    result = NULL;
    raw = g_strsplit (encodings, ",", 0);

    for (p = raw; p && *p; ++p)
    {
        const char *enc;

        if (!g_ascii_strcasecmp (*p, ENCODING_LOCALE))
        {
            if (g_get_charset (&enc))
                enc = "UTF-8";
        }
        else
        {
            enc = *p;
        }

        if (!g_slist_find_custom (result, enc, (GCompareFunc) g_ascii_strcasecmp))
            result = g_slist_prepend (result, g_strdup (enc));
    }

    if (!result)
    {
        g_critical ("oops");
        result = g_slist_prepend (NULL, g_strdup ("UTF-8"));
    }

    g_strfreev (raw);
    return g_slist_reverse (result);
}


static gboolean
check_regular (GFile   *file,
               GError **error)
{
    GFileInfo *info;
    GFileType type;
    gboolean retval = TRUE;

    if (!g_file_is_native (file))
        return TRUE;

    if (!(info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_TYPE, (GFileQueryInfoFlags) 0, NULL, NULL)))
        return TRUE;

    type = g_file_info_get_file_type (info);
    if (type != G_FILE_TYPE_REGULAR && type != G_FILE_TYPE_UNKNOWN)
    {
        g_set_error (error, MOO_EDIT_FILE_ERROR,
                     MOO_EDIT_FILE_ERROR_FAILED,
                     "%s", D_("Not a regular file", "glib20"));
        retval = FALSE;
    }

    g_object_unref (info);
    return retval;
}

static void
moo_edit_load_text (MooEdit    *edit,
                    GFile      *file,
                    const char *encoding,
                    const char *text)
{
    GtkTextIter start;
    GtkTextBuffer *buffer;
    gboolean undo;
    char *freeme = NULL;
    MooLineEndType saved_le;

    if (moo_edit_is_empty (edit))
        undo = FALSE;
    else
        undo = TRUE;

    buffer = moo_edit_get_buffer (edit);

    block_buffer_signals (edit);

    if (undo)
        gtk_text_buffer_begin_user_action (buffer);
    else
        moo_text_buffer_begin_non_undoable_action (MOO_TEXT_BUFFER (buffer));

    moo_text_buffer_begin_non_interactive_action (MOO_TEXT_BUFFER (buffer));

    saved_le = edit->priv->line_end_type;

    {
        gboolean enable_highlight;
        buffer = moo_edit_get_buffer (edit);
        gtk_text_buffer_set_text (buffer, "", 0);
        g_object_get (buffer, "highlight-syntax", &enable_highlight, (char*) 0);
        g_object_set (buffer, "highlight-syntax", FALSE, (char*) 0);
        do_load_text (edit, text);
        g_object_set (buffer, "highlight-syntax", enable_highlight, (char*) 0);
    }

    unblock_buffer_signals (edit);

    if (undo)
        gtk_text_buffer_end_user_action (buffer);
    else
        moo_text_buffer_end_non_undoable_action (MOO_TEXT_BUFFER (buffer));

    moo_text_buffer_end_non_interactive_action (MOO_TEXT_BUFFER (buffer));

    gtk_text_buffer_get_start_iter (buffer, &start);
    gtk_text_buffer_place_cursor (buffer, &start);
    edit->priv->status = (MooEditStatus) 0;
    moo_edit_set_modified (edit, FALSE);
    _moo_edit_set_file (edit, file, encoding);
    if (edit->priv->line_end_type != saved_le)
        g_object_notify (G_OBJECT (edit), "line-end-type");
    _moo_edit_start_file_watch (edit);

    g_free (freeme);
}


static void
do_load_text (MooEdit    *edit,
              const char *text)
{
    GtkTextBuffer *buffer;
    MooLineEndType le = MOO_LE_NONE;
    gboolean mixed_le = FALSE;
    const char *line = NULL;
    MooLineReader lr;
    gsize line_len;
    gsize line_term_len;
    GString *strbuf;

    strbuf = g_string_new (NULL);
    buffer = moo_edit_get_buffer (edit);

    moo_line_reader_init (&lr, text, -1);

    while ((line = moo_line_reader_get_line (&lr, &line_len, &line_term_len)) != NULL)
    {
        gboolean insert_line_term = FALSE;
        MooLineEndType le_here = MOO_LE_NONE;
        gsize copy_len = line_len;

        if (line_term_len != 0)
        {
            const char *line_term = line + line_len;

            insert_line_term = TRUE;

            if (line_term_len == 1 && !strncmp (line_term, "\r", line_term_len))
            {
                le_here = MOO_LE_MAC;
            }
            else if (line_term_len == 1 && !strncmp (line_term, "\n", line_term_len))
            {
                le_here = MOO_LE_UNIX;
            }
            else if (line_term_len == 2 && !strncmp (line_term, "\r\n", line_term_len))
            {
                le_here = MOO_LE_WIN32;
            }
            else if (line_term_len == 3 && !strncmp ("\xe2\x80\xa9", line_term, line_term_len))
            {
                insert_line_term = FALSE;
                copy_len += line_term_len;
            }
            else
            {
                g_critical ("oops");
                copy_len += line_term_len;
            }

            if (le_here)
            {
                if (mixed_le || (le && le != le_here))
                    mixed_le = TRUE;
                else
                    le = le_here;
            }
        }

        g_string_append_len (strbuf, line, copy_len);

        if (insert_line_term)
            g_string_append_c (strbuf, '\n');
    }

    gtk_text_buffer_insert_at_cursor (buffer, strbuf->str, (int) strbuf->len);

    if (mixed_le)
        le = MOO_LE_NATIVE;

    if (le != MOO_LE_NONE)
        moo_edit_set_line_end_type_full (edit, le, TRUE);

    g_string_free (strbuf, TRUE);
}


/* XXX */
static gboolean
moo_edit_reload_local (MooEdit    *edit,
                       const char *encoding,
                       GError    **error)
{
    gboolean result;
    GFile *file;

    file = moo_edit_get_file (edit);
    moo_return_error_if_fail (G_IS_FILE (file));

    result = _moo_edit_load_file (edit, file,
                                  encoding ? encoding : edit->priv->encoding,
                                  NULL,
                                  error);

    if (result)
    {
        edit->priv->status = (MooEditStatus) 0;
        moo_edit_set_modified (edit, FALSE);
        _moo_edit_start_file_watch (edit);
        g_clear_error (error);
    }

    g_object_unref (file);
    return result;
}


/***************************************************************************/
/* File saving
 */

static char *
get_contents_with_fixed_line_end (GtkTextBuffer *buffer, const char *le, gsize le_len)
{
    GtkTextIter line_start;
    GString *contents;

    contents = g_string_new (NULL);
    gtk_text_buffer_get_start_iter (buffer, &line_start);

    do
    {
        GtkTextIter line_end = line_start;

        if (!gtk_text_iter_ends_line (&line_start))
        {
            char *line;
            gsize len;

            gtk_text_iter_forward_to_line_end (&line_end);
            line = gtk_text_buffer_get_text (buffer, &line_start, &line_end, TRUE);
            len = strlen (line);

            g_string_append_len (contents, line, len);

            g_free (line);
        }

        if (!gtk_text_iter_is_end (&line_end))
            g_string_append_len (contents, le, le_len);
    }
    while (gtk_text_iter_forward_line (&line_start));

    return g_string_free (contents, FALSE);
}

static char *
get_contents (MooEdit *edit)
{
    const char *le = "\n";
    gsize le_len = 1;
    GtkTextBuffer *buffer;

    switch (moo_edit_get_line_end_type (edit))
    {
        case MOO_LE_UNIX:
            le = "\n";
            le_len = 1;
            break;
        case MOO_LE_WIN32:
            le = "\r\n";
            le_len = 2;
            break;
        case MOO_LE_MAC:
            le = "\r";
            le_len = 1;
            break;
        default:
            moo_assert_not_reached ();
    }

    buffer = moo_edit_get_buffer (edit);
    return get_contents_with_fixed_line_end (buffer, le, le_len);
}

static gboolean
do_write (GFile             *file,
          const char        *data1,
          gsize              len1,
          const char        *data2,
          gsize              len2,
          MooEditSaveFlags   flags,
          GError           **error)
{
    MooFileWriter *writer;
    MooFileWriterFlags writer_flags;
    gboolean success = FALSE;

    moo_return_error_if_fail (G_IS_FILE (file));

    writer_flags = (flags & MOO_EDIT_SAVE_BACKUP) ? MOO_FILE_WRITER_SAVE_BACKUP : (MooFileWriterFlags) 0;

    if ((writer = moo_file_writer_new_for_file (file, writer_flags, error)))
    {
        success = TRUE;
        if (success && len1 > 0)
            success = moo_file_writer_write (writer, data1, len1);
        if (success && len2 > 0)
            success = moo_file_writer_write (writer, data2, len2);
        if (success)
            success = moo_file_writer_close (writer, error);
    }

    return success;
}

static gboolean
do_save_local (MooEdit        *edit,
               GFile          *file,
               const char     *encoding,
               MooEditSaveFlags flags,
               GError        **error)
{
    char *utf8_contents;
    const char *to_save;
    gsize to_save_size;
    GError *encoding_error = NULL;
    char *freeme = NULL;
    gboolean success = TRUE;
    const char *enc_no_bom = NULL;
    const char *bom = NULL;
    gsize bom_len = 0;

    utf8_contents = get_contents (edit);

    moo_release_assert (utf8_contents != NULL);

    if (encoding_needs_bom_save (encoding, &enc_no_bom, &bom, &bom_len))
        encoding = enc_no_bom;

    if (encoding && encoding_is_utf8 (encoding))
        encoding = NULL;

    if (encoding)
    {
        gsize bytes_read;
        gsize bytes_written;
        char *encoded = g_convert (utf8_contents, -1,
                                   encoding, "UTF-8",
                                   &bytes_read, &bytes_written,
                                   &encoding_error);

        if (encoded)
        {
            to_save = encoded;
            to_save_size = bytes_written;
            freeme = encoded;
        }
        else
        {
            g_propagate_error (error, encoding_error);
            set_encoding_error (error);
            goto error;
        }
    }
    else
    {
        to_save = utf8_contents;
        to_save_size = strlen (utf8_contents);
    }

    if (!do_write (file, bom, bom_len, to_save, to_save_size, flags, error))
        goto error;

    goto out;

error:
    success = FALSE;
out:
    g_free (freeme);
    g_free (utf8_contents);
    return success;
}


static gboolean
moo_edit_save_local (MooEdit        *edit,
                     GFile          *file,
                     const char     *encoding,
                     MooEditSaveFlags flags,
                     GError        **error)
{
    moo_return_error_if_fail (MOO_IS_EDIT (edit));
    moo_return_error_if_fail (G_IS_FILE (file));

    if (!do_save_local (edit, file, encoding, flags, error))
        return FALSE;

    edit->priv->status = (MooEditStatus) 0;
    _moo_edit_set_file (edit, file, encoding);
    moo_edit_set_modified (edit, FALSE);
    _moo_edit_start_file_watch (edit);
    return TRUE;
}


static gboolean
moo_edit_save_copy_local (MooEdit        *edit,
                          GFile          *file,
                          const char     *encoding,
                          MooEditSaveFlags flags,
                          GError        **error)
{
    moo_return_error_if_fail (MOO_IS_EDIT (edit));
    moo_return_error_if_fail (G_IS_FILE (file));
    return do_save_local (edit, file, encoding, flags, error);
}


/***************************************************************************/
/* Aux stuff
 */

static void
block_buffer_signals (MooEdit *edit)
{
    g_signal_handler_block (moo_edit_get_buffer (edit), edit->priv->modified_changed_handler_id);
}


static void
unblock_buffer_signals (MooEdit *edit)
{
    g_signal_handler_unblock (moo_edit_get_buffer (edit), edit->priv->modified_changed_handler_id);
}


static void
file_watch_callback (G_GNUC_UNUSED MooFileWatch *watch,
                     MooFileEvent  *event,
                     gpointer       data)
{
    MooEdit *edit = MOO_EDIT (data);

    g_return_if_fail (MOO_IS_EDIT (data));
    g_return_if_fail (event->monitor_id == edit->priv->file_monitor_id);
    g_return_if_fail (edit->priv->filename != NULL);
    g_return_if_fail (!(edit->priv->status & MOO_EDIT_STATUS_CHANGED_ON_DISK));

    switch (event->code)
    {
        case MOO_FILE_EVENT_CHANGED:
            edit->priv->modified_on_disk = TRUE;
            break;

        case MOO_FILE_EVENT_DELETED:
            edit->priv->deleted_from_disk = TRUE;
            edit->priv->file_monitor_id = 0;
            break;

        case MOO_FILE_EVENT_ERROR:
            /* XXX and what to do now? */
            break;

        case MOO_FILE_EVENT_CREATED:
            g_critical ("oops");
            break;
    }

    check_file_status (edit);
}


static void
_moo_edit_start_file_watch (MooEdit *edit)
{
    MooFileWatch *watch;
    GError *error = NULL;

    watch = _moo_editor_get_file_watch (edit->priv->editor);
    g_return_if_fail (watch != NULL);

    if (edit->priv->file_monitor_id)
        moo_file_watch_cancel_monitor (watch, edit->priv->file_monitor_id);
    edit->priv->file_monitor_id = 0;

    g_return_if_fail ((edit->priv->status & MOO_EDIT_STATUS_CHANGED_ON_DISK) == 0);
    g_return_if_fail (edit->priv->filename != NULL);

    edit->priv->file_monitor_id =
        moo_file_watch_create_monitor (watch, edit->priv->filename,
                                       file_watch_callback,
                                       edit, NULL, &error);

    if (!edit->priv->file_monitor_id)
    {
        g_warning ("could not start watch for '%s': %s",
                   edit->priv->filename, moo_error_message (error));
        g_error_free (error);
        return;
    }
}


void
_moo_edit_stop_file_watch (MooEdit *edit)
{
    MooFileWatch *watch;

    watch = _moo_editor_get_file_watch (edit->priv->editor);
    g_return_if_fail (watch != NULL);

    if (edit->priv->file_monitor_id)
        moo_file_watch_cancel_monitor (watch, edit->priv->file_monitor_id);
    edit->priv->file_monitor_id = 0;
}


static void
check_file_status (MooEdit *edit)
{
    g_return_if_fail (edit->priv->filename != NULL);
    g_return_if_fail (!(edit->priv->status & MOO_EDIT_STATUS_CHANGED_ON_DISK));

    if (edit->priv->deleted_from_disk)
        file_deleted (edit);
    else if (edit->priv->modified_on_disk)
        file_modified_on_disk (edit);
}


static void
file_modified_on_disk (MooEdit *edit)
{
    g_return_if_fail (edit->priv->filename != NULL);

    if (moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_AUTO_SYNC)))
    {
        moo_edit_reload (edit, NULL, NULL);
    }
    else
    {
        edit->priv->modified_on_disk = FALSE;
        edit->priv->deleted_from_disk = FALSE;
        _moo_edit_stop_file_watch (edit);
        add_status (edit, MOO_EDIT_STATUS_MODIFIED_ON_DISK);
    }
}


static void
file_deleted (MooEdit *edit)
{
    g_return_if_fail (edit->priv->filename != NULL);

    if (moo_prefs_get_bool (moo_edit_setting (MOO_EDIT_PREFS_AUTO_SYNC)))
    {
        moo_edit_close(edit);
    }
    else
    {
        edit->priv->modified_on_disk = FALSE;
        edit->priv->deleted_from_disk = FALSE;
        _moo_edit_stop_file_watch (edit);
        add_status (edit, MOO_EDIT_STATUS_DELETED);
    }
}


static void
add_status (MooEdit        *edit,
            MooEditStatus   s)
{
    edit->priv->status = (MooEditStatus) (edit->priv->status | s);
    g_signal_emit_by_name (edit, "doc-status-changed", NULL);
}


void
_moo_edit_remove_untitled (MooEdit *doc)
{
    gpointer n = g_hash_table_lookup (UNTITLED_NO, doc);

    if (n)
    {
        UNTITLED = g_slist_remove (UNTITLED, n);
        g_hash_table_remove (UNTITLED_NO, doc);
    }
}


static int
add_untitled (MooEdit *edit)
{
    int n;

    if (!(n = GPOINTER_TO_INT (g_hash_table_lookup (UNTITLED_NO, edit))))
    {
        for (n = 1; ; ++n)
        {
            if (!g_slist_find (UNTITLED, GINT_TO_POINTER (n)))
            {
                UNTITLED = g_slist_prepend (UNTITLED, GINT_TO_POINTER (n));
                break;
            }
        }

        g_hash_table_insert (UNTITLED_NO, edit, GINT_TO_POINTER (n));
    }

    return n;
}


static char *
moo_file_get_display_basename (GFile *file)
{
    char *name;
    const char *slash;

    g_return_val_if_fail (G_IS_FILE (file), NULL);

    name = moo_file_get_display_name (file);
    g_return_val_if_fail (name != NULL, NULL);

    slash = strrchr (name, '/');

#ifdef G_OS_WIN32
    {
        const char *backslash = strrchr (name, '\\');
        if (backslash && (!slash || backslash > slash))
            slash = backslash;
    }
#endif

    if (slash)
        memmove (name, slash + 1, strlen (slash + 1) + 1);

    return name;
}

char *
_moo_edit_normalize_filename_for_comparison (const char *filename)
{
#ifdef __WIN32__
    /* XXX */
    char *tmp;
    char *ret;
    g_return_val_if_fail (filename != NULL, NULL);
    tmp = g_utf8_normalize (filename, -1, G_NORMALIZE_ALL_COMPOSE);
    ret = g_utf8_strdown (tmp, -1);
    g_free (tmp);
    return ret;
#else
    g_return_val_if_fail (filename != NULL, NULL);
    return g_strdup (filename);
#endif
}

char *_moo_edit_normalize_uri_for_comparison (const char *uri)
{
    return _moo_edit_normalize_filename_for_comparison (uri);
}

char *
_moo_file_get_normalized_name (GFile *file)
{
    char *ret;
    char *tmp = NULL;
    char *tmp2 = NULL;

    g_return_val_if_fail (G_IS_FILE (file), NULL);

    tmp = g_file_get_path (file);

    if (tmp)
    {
        tmp2 = _moo_normalize_file_path (tmp);
        ret = _moo_edit_normalize_filename_for_comparison (tmp2);
    }
    else
    {
        tmp = g_file_get_uri (file);
        g_return_val_if_fail (tmp != NULL, NULL);
        ret = _moo_edit_normalize_uri_for_comparison (tmp);
    }

    g_free (tmp2);
    g_free (tmp);
    return ret;
}

void
_moo_edit_set_file (MooEdit    *edit,
                    GFile      *file,
                    const char *encoding)
{
    GFile *tmp;
    GSList *free_list = NULL;

    tmp = edit->priv->file;

    free_list = g_slist_prepend (free_list, edit->priv->filename);
    free_list = g_slist_prepend (free_list, edit->priv->norm_name);
    free_list = g_slist_prepend (free_list, edit->priv->display_filename);
    free_list = g_slist_prepend (free_list, edit->priv->display_basename);

    if (!UNTITLED_NO)
        UNTITLED_NO = g_hash_table_new (g_direct_hash, g_direct_equal);

    if (!file)
    {
        int n = add_untitled (edit);

        edit->priv->file = NULL;
        edit->priv->filename = NULL;
        edit->priv->norm_name = NULL;

        if (n == 1)
            edit->priv->display_filename = g_strdup (_("Untitled"));
        else
            edit->priv->display_filename = g_strdup_printf (_("Untitled %d"), n);

        edit->priv->display_basename = g_strdup (edit->priv->display_filename);
    }
    else
    {
        _moo_edit_remove_untitled (edit);
        edit->priv->file = g_file_dup (file);
        edit->priv->filename = g_file_get_path (file);
        edit->priv->norm_name = _moo_file_get_normalized_name (file);
        edit->priv->display_filename = moo_file_get_display_name (file);
        edit->priv->display_basename = moo_file_get_display_basename (file);
    }

    if (!encoding)
        moo_edit_set_encoding (edit, _moo_edit_get_default_encoding ());
    else
        moo_edit_set_encoding (edit, encoding);

    g_signal_emit_by_name (edit, "filename-changed", NULL);
    _moo_edit_status_changed (edit);
    _moo_edit_queue_recheck_config (edit);

    moo_file_free (tmp);

    g_slist_foreach (free_list, (GFunc) g_free, NULL);
    g_slist_free (free_list);
}


GdkPixbuf *
_moo_edit_get_icon (MooEdit     *doc,
                    GtkWidget   *widget,
                    GtkIconSize  size)
{
    if (doc->priv->filename)
        return moo_get_icon_for_path (doc->priv->filename, widget, size);
    else if (doc->priv->file)
        return moo_get_icon_for_path (doc->priv->display_basename, widget, size);
    else
        return moo_get_icon_for_path (NULL, widget, size);
}


/***************************************************************************
 *
 * Character encoding conversion
 *
 */

#define BOM_UTF8        "\xEF\xBB\xBF"
#define BOM_UTF8_LEN    3
#define BOM_UTF16_LE    "\xFF\xFE"
#define BOM_UTF16_BE    "\xFE\xFF"
#define BOM_UTF16_LEN   2
#define BOM_UTF32_LE    "\xFF\xFE\x00\x00"
#define BOM_UTF32_BE    "\x00\x00\xFE\xFF"
#define BOM_UTF32_LEN   4

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define BOM_UTF16 BOM_UTF16_LE
#define BOM_UTF32 BOM_UTF32_LE
#else
#define BOM_UTF16 BOM_UTF16_BE
#define BOM_UTF32 BOM_UTF32_BE
#endif

static char *
try_convert_to_utf8_from_utf8 (const char *data,
                               gsize       len)
{
    const char *invalid;
    gboolean valid_utf8;

//     g_print ("try_convert_to_utf8_from_utf8()\n");

    if (len >= BOM_UTF8_LEN && memcmp (data, BOM_UTF8, BOM_UTF8_LEN) == 0)
    {
        data += BOM_UTF8_LEN;
        len -= BOM_UTF8_LEN;
    }

    valid_utf8 = g_utf8_validate (data, len, &invalid);

    // allow trailing zero byte
    if (!valid_utf8 && invalid + 1 == data + len && *invalid == 0)
        valid_utf8 = TRUE;

    return valid_utf8 ? g_strdup (data) : NULL;
}

static gboolean
encoding_needs_bom_load (const char  *enc,
                         gboolean    *bom_optional,
                         const char **enc_no_bom,
                         const char **bom,
                         gsize       *bom_len)
{
    guint i;

    static const struct {
        const char *enc_bom;
        const char *enc_no_bom;
        const char *bom;
        gsize bom_len;
        gboolean optional;
    } encs[] = {
        { "UTF-8-BOM",    "UTF-8",    BOM_UTF8,     BOM_UTF8_LEN,  FALSE },
        { "UTF-16",       "UTF-16",   BOM_UTF16,    BOM_UTF16_LEN, TRUE },
        { "UTF-16-BOM",   "UTF-16",   BOM_UTF16,    BOM_UTF16_LEN, FALSE },
        { "UTF-16LE-BOM", "UTF-16LE", BOM_UTF16_LE, BOM_UTF16_LEN, FALSE },
        { "UTF-16BE-BOM", "UTF-16BE", BOM_UTF16_BE, BOM_UTF16_LEN, FALSE },
        { "UTF-32",       "UTF-32",   BOM_UTF32,    BOM_UTF32_LEN, TRUE },
        { "UTF-32-BOM",   "UTF-32",   BOM_UTF32,    BOM_UTF32_LEN, FALSE },
        { "UTF-32LE-BOM", "UTF-32LE", BOM_UTF32_LE, BOM_UTF32_LEN, FALSE },
        { "UTF-32BE-BOM", "UTF-32BE", BOM_UTF32_BE, BOM_UTF32_LEN, FALSE },
    };

    for (i = 0; i < G_N_ELEMENTS (encs); ++i)
    {
        if (!g_ascii_strcasecmp (enc, encs[i].enc_bom))
        {
            *enc_no_bom = encs[i].enc_no_bom;
            *bom = encs[i].bom;
            *bom_len = encs[i].bom_len;
            *bom_optional = encs[i].optional;
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean
encoding_needs_bom_save (const char  *enc,
                         const char **enc_no_bom,
                         const char **bom,
                         gsize       *bom_len)
{
    guint i;

    static const struct {
        const char *enc_bom;
        const char *enc_no_bom;
        const char *bom;
        gsize bom_len;
    } encs[] = {
        { "UTF-8-BOM",    "UTF-8",    BOM_UTF8,     BOM_UTF8_LEN },
        { "UTF-16LE-BOM", "UTF-16LE", BOM_UTF16_LE, BOM_UTF16_LEN },
        { "UTF-16BE-BOM", "UTF-16BE", BOM_UTF16_BE, BOM_UTF16_LEN },
        { "UTF-32LE-BOM", "UTF-32LE", BOM_UTF32_LE, BOM_UTF32_LEN },
        { "UTF-32BE-BOM", "UTF-32BE", BOM_UTF32_BE, BOM_UTF32_LEN },
    };

    for (i = 0; i < G_N_ELEMENTS (encs); ++i)
    {
        if (!g_ascii_strcasecmp (enc, encs[i].enc_bom))
        {
            *enc_no_bom = encs[i].enc_no_bom;
            *bom = encs[i].bom;
            *bom_len = encs[i].bom_len;
            return TRUE;
        }
    }

    return FALSE;
}

static char *
try_convert_to_utf8_from_non_utf8_encoding (const char *data,
                                            gsize       len,
                                            const char *enc)
{
    const char *enc_no_bom = NULL;
    const char *bom = NULL;
    gsize bom_len = 0;
    gsize bytes_read = 0;
    gsize bytes_written = 0;
    char *result = NULL;
    gsize result_len = 0;
    gboolean bom_optional = FALSE;

//     g_print ("try_convert_to_utf8_from_non_utf8_encoding(%s)\n",
//              enc ? enc : "<null>");

    if (encoding_needs_bom_load (enc, &bom_optional, &enc_no_bom, &bom, &bom_len))
    {
        if (len < bom_len || memcmp (bom, data, bom_len) != 0)
        {
            if (!bom_optional)
                return NULL;
        }
        else
        {
            data += bom_len;
            len -= bom_len;
            enc = enc_no_bom;
        }
    }

    if (encoding_is_utf8 (enc))
        return try_convert_to_utf8_from_utf8 (data, len);

    result = g_convert (data, len, "UTF-8", enc, &bytes_read, &bytes_written, NULL);

    if (!result)
        return NULL;

    if (bytes_read < len)
    {
        g_free (result);
        return NULL;
    }

    result_len = strlen (result);

    // ignore trailing zero
    if (bytes_written == result_len + 1)
        bytes_written -= 1;

    if (result_len < bytes_written)
    {
        g_free (result);
        return NULL;
    }

    return result;
}

static char *
try_convert_to_utf8_from_encoding (const char *data,
                                   gsize       len,
                                   const char *enc)
{
    if (encoding_is_utf8 (enc))
        return try_convert_to_utf8_from_utf8 (data, len);
    else
        return try_convert_to_utf8_from_non_utf8_encoding (data, len, enc);
}

static gboolean
data_has_bom (const char  *data,
              gsize        len,
              const char **bom_enc)
{
    guint i;

    static const struct {
        const char *enc;
        const char *bom;
        gsize bom_len;
    } encs[] = {
        { "UTF-8-BOM",      BOM_UTF8,     BOM_UTF8_LEN },
        { "UTF-16-BOM",     BOM_UTF16,    BOM_UTF16_LEN },
        { "UTF-32-BOM",     BOM_UTF32,    BOM_UTF32_LEN },
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
        { "UTF-16BE-BOM",   BOM_UTF16_BE, BOM_UTF16_LEN },
        { "UTF-32BE-BOM",   BOM_UTF32_BE, BOM_UTF32_LEN },
#else
        { "UTF-16LE-BOM",   BOM_UTF16_LE, BOM_UTF16_LEN },
        { "UTF-32LE-BOM",   BOM_UTF32_LE, BOM_UTF32_LEN },
#endif
    };

    for (i = 0; i < G_N_ELEMENTS (encs); ++i)
    {
        const char *bom = encs[i].bom;
        gsize bom_len = encs[i].bom_len;

        if (len >= bom_len && memcmp (data, bom, bom_len) == 0)
        {
            *bom_enc = encs[i].enc;
            return TRUE;
        }
    }

    return FALSE;
}

static char *
moo_convert_file_data_to_utf8 (const char  *data,
                               gsize        len,
                               const char  *encoding,
                               const char  *cached_encoding,
                               char       **used_enc)
{
    char *freeme = NULL;
    char *result = NULL;
    const char *bom_enc = NULL;

//     g_print ("moo_convert_file_data_to_utf8(%s, %s)\n",
//              encoding ? encoding : "<null>",
//              cached_encoding ? cached_encoding : "<null>");

    *used_enc = NULL;

    if (!encoding && data_has_bom (data, len, &bom_enc))
    {
        encoding = bom_enc;
        result = try_convert_to_utf8_from_encoding (data, len, encoding);
        *used_enc = g_strdup (bom_enc);
    }
    else if (!encoding)
    {
        GSList *encodings;

        encodings = get_encodings ();
        if (cached_encoding)
            encodings = g_slist_prepend (encodings, g_strdup (cached_encoding));

        while (encodings)
        {
            char *enc;

            enc = (char*) encodings->data;
            encodings = g_slist_delete_link (encodings, encodings);

            result = try_convert_to_utf8_from_encoding (data, len, enc);

            if (result != NULL)
            {
                encoding = freeme = enc;
                break;
            }

            g_free (enc);
        }

        g_slist_foreach (encodings, (GFunc) g_free, NULL);
        g_slist_free (encodings);
    }
    else
    {
        result = try_convert_to_utf8_from_encoding (data, len, encoding);
    }

    if (result)
        *used_enc = g_strdup (encoding);
    g_free (freeme);
    return result;
}

static gboolean
encoding_is_utf8 (const char *encoding)
{
    return !g_ascii_strcasecmp (encoding, "UTF-8") ||
           !g_ascii_strcasecmp (encoding, "UTF8");
}
