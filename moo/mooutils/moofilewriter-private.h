/*
 *   moofilewriter-private.h
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

#ifndef MOO_FILE_WRITER_PRIVATE_H
#define MOO_FILE_WRITER_PRIVATE_H

#include "mooutils/moofilewriter.h"

G_BEGIN_DECLS

#define MOO_TYPE_FILE_READER            (moo_file_reader_get_type ())
#define MOO_FILE_READER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_FILE_READER, MooFileReader))
#define MOO_FILE_READER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_FILE_READER, MooFileReaderClass))
#define MOO_IS_FILE_READER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_FILE_READER))
#define MOO_IS_FILE_READER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_FILE_READER))
#define MOO_FILE_READER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_FILE_READER, MooFileReaderClass))

typedef struct _MooFileReaderClass MooFileReaderClass;

struct _MooFileReaderClass {
    GObjectClass base_class;
};


#define MOO_TYPE_FILE_WRITER            (moo_file_writer_get_type ())
#define MOO_FILE_WRITER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_FILE_WRITER, MooFileWriter))
#define MOO_FILE_WRITER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_FILE_WRITER, MooFileWriterClass))
#define MOO_IS_FILE_WRITER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_FILE_WRITER))
#define MOO_IS_FILE_WRITER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_FILE_WRITER))
#define MOO_FILE_WRITER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_FILE_WRITER, MooFileWriterClass))

typedef struct _MooFileWriterClass MooFileWriterClass;

struct _MooFileWriter {
    GObject base;
};

struct _MooFileWriterClass {
    GObjectClass base_class;

    gboolean (*meth_write)  (MooFileWriter *writer,
                             const char    *data,
                             gsize          len);
    gboolean (*meth_printf) (MooFileWriter  *writer,
                             const char     *fmt,
                             va_list         args) G_GNUC_PRINTF (2, 0);
    gboolean (*meth_close)  (MooFileWriter  *writer,
                             GError        **error);
};


#define MOO_TYPE_LOCAL_FILE_WRITER (moo_local_file_writer_get_type ())

typedef struct _MooLocalFileWriter MooLocalFileWriter;
typedef struct _MooLocalFileWriterClass MooLocalFileWriterClass;

struct _MooLocalFileWriterClass {
    MooFileWriterClass base_class;
};


#define MOO_TYPE_STRING_WRITER (moo_string_writer_get_type ())

typedef struct _MooStringWriter MooStringWriter;
typedef struct _MooStringWriterClass MooStringWriterClass;

struct _MooStringWriterClass {
    MooFileWriterClass base_class;
};

G_END_DECLS

#endif /* MOO_FILE_WRITER_PRIVATE_H */
