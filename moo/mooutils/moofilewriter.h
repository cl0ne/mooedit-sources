/*
 *   mooutils-file.h
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

#ifndef MOO_FILE_WRITER_H
#define MOO_FILE_WRITER_H

#include <gio/gio.h>

G_BEGIN_DECLS


typedef struct _MooFileReader MooFileReader;

MooFileReader  *moo_file_reader_new             (const char     *filename,
                                                 GError        **error);
MooFileReader  *moo_text_reader_new             (const char     *filename,
                                                 GError        **error);
gboolean        moo_file_reader_read            (MooFileReader  *reader,
                                                 char           *buf,
                                                 gsize           buf_size,
                                                 gsize          *size_read,
                                                 GError        **error);
void            moo_file_reader_close           (MooFileReader  *reader);

typedef struct _MooFileWriter MooFileWriter;

typedef enum /*< flags >*/
{
    MOO_FILE_WRITER_SAVE_BACKUP = 1 << 0,
    MOO_FILE_WRITER_CONFIG_MODE = 1 << 1,
    MOO_FILE_WRITER_TEXT_MODE   = 1 << 2
} MooFileWriterFlags;

MooFileWriter  *moo_file_writer_new             (const char     *filename,
                                                 MooFileWriterFlags flags,
                                                 GError        **error);
MooFileWriter  *moo_file_writer_new_for_file    (GFile          *file,
                                                 MooFileWriterFlags flags,
                                                 GError        **error);
MooFileWriter  *moo_config_writer_new           (const char     *filename,
                                                 gboolean        save_backup,
                                                 GError        **error);
MooFileWriter  *moo_string_writer_new           (void);

gboolean        moo_file_writer_write           (MooFileWriter  *writer,
                                                 const char     *data,
                                                 gssize          len);
gboolean        moo_file_writer_printf          (MooFileWriter  *writer,
                                                 const char     *fmt,
                                                 ...) G_GNUC_PRINTF (2, 3);
gboolean        moo_file_writer_printf_markup   (MooFileWriter  *writer,
                                                 const char     *fmt,
                                                 ...) G_GNUC_PRINTF (2, 3);
gboolean        moo_file_writer_close           (MooFileWriter  *writer,
                                                 GError        **error);

const char     *moo_string_writer_get_string    (MooFileWriter  *writer,
                                                 gsize          *len);


G_END_DECLS

#endif /* MOO_FILE_WRITER_H */
