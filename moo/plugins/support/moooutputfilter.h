/*
 *   moooutputfilter.h
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

#ifndef MOO_OUTPUT_FILTER_H
#define MOO_OUTPUT_FILTER_H

#include "moolineview.h"

G_BEGIN_DECLS


#define MOO_TYPE_FILE_LINE_DATA             (moo_file_line_data_get_type ())

#define MOO_TYPE_OUTPUT_FILTER              (moo_output_filter_get_type ())
#define MOO_OUTPUT_FILTER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_OUTPUT_FILTER, MooOutputFilter))
#define MOO_OUTPUT_FILTER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_OUTPUT_FILTER, MooOutputFilterClass))
#define MOO_IS_OUTPUT_FILTER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_OUTPUT_FILTER))
#define MOO_IS_OUTPUT_FILTER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_OUTPUT_FILTER))
#define MOO_OUTPUT_FILTER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_OUTPUT_FILTER, MooOutputFilterClass))

typedef struct _MooOutputFilterPrivate  MooOutputFilterPrivate;
typedef struct _MooFileLineData         MooFileLineData;

struct _MooFileLineData {
    char *file;
    int line;
    int character;
};

struct MooOutputFilter
{
    GObject parent;
    MooOutputFilterPrivate *priv;
};

struct MooOutputFilterClass
{
    GObjectClass parent_class;

    void     (*attach)      (MooOutputFilter *filter);
    void     (*detach)      (MooOutputFilter *filter);

    void     (*activate)    (MooOutputFilter *filter,
                             int              line);

    void     (*cmd_start)   (MooOutputFilter *filter);
    gboolean (*cmd_exit)    (MooOutputFilter *filter,
                             int              status);
    gboolean (*stdout_line) (MooOutputFilter *filter,
                             const char      *line);
    gboolean (*stderr_line) (MooOutputFilter *filter,
                             const char      *line);
};


GType            moo_output_filter_get_type         (void) G_GNUC_CONST;
GType            moo_file_line_data_get_type        (void) G_GNUC_CONST;

MooFileLineData *moo_file_line_data_new             (const char         *file,
                                                     int                 line,
                                                     int                 character);
void             moo_file_line_data_free            (MooFileLineData    *data);

void             moo_output_filter_set_view         (MooOutputFilter    *filter,
                                                     MooLineView        *view);
MooLineView     *moo_output_filter_get_view         (MooOutputFilter    *filter);

gboolean         moo_output_filter_stdout_line      (MooOutputFilter    *filter,
                                                     const char         *line);
gboolean         moo_output_filter_stderr_line      (MooOutputFilter    *filter,
                                                     const char         *line);
void             moo_output_filter_cmd_start        (MooOutputFilter    *filter,
                                                     const char         *working_dir);
gboolean         moo_output_filter_cmd_exit         (MooOutputFilter    *filter,
                                                     int                 status);

void             moo_output_filter_set_active_file  (MooOutputFilter    *filter,
                                                     const char         *path);
void             moo_output_filter_add_active_dirs  (MooOutputFilter    *filter,
                                                     char              **dirs);
const char      *moo_output_filter_get_active_file  (MooOutputFilter    *filter);
const char*const*moo_output_filter_get_active_dirs  (MooOutputFilter    *filter);
void             moo_output_filter_set_window       (MooOutputFilter    *filter,
                                                     gpointer            window);


G_END_DECLS

#endif /* MOO_OUTPUT_FILTER_H */
