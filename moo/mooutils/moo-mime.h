/*
 *   moo-mime.h
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

#ifndef MOO_MIME_H
#define MOO_MIME_H

#include <mooglib/moo-glib.h>

G_BEGIN_DECLS

/* All public functions here are thread-safe */

typedef struct MgwStatBuf MgwStatBuf;

#define MOO_MIME_TYPE_UNKNOWN (moo_mime_type_unknown ())

const char  *moo_mime_type_unknown              (void) G_GNUC_CONST;

const char  *moo_get_mime_type_for_file         (const char     *filename,
                                                 MgwStatBuf     *statbuf);
const char  *moo_get_mime_type_for_filename     (const char     *filename);
gboolean     moo_mime_type_is_subclass          (const char     *mime_type,
                                                 const char     *base);
const char **moo_mime_type_list_parents         (const char     *mime_type);
void         moo_mime_shutdown                  (void);

const char *const *_moo_get_mime_data_dirs      (void);


G_END_DECLS

#endif /* MOO_MIME_H */
