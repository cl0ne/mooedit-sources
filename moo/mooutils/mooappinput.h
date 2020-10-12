/*
 *   mooapp/mooappinput.h
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

#ifndef MOO_APP_INPUT_H
#define MOO_APP_INPUT_H

#include <mooglib/moo-glib.h>

G_BEGIN_DECLS


typedef void (*MooAppInputCallback) (char        cmd,
                                     const char *data,
                                     gsize       len,
                                     gpointer    cb_data);

void         _moo_app_input_start       (const char     *name,
                                         gboolean        bind_default,
                                         MooAppInputCallback callback,
                                         gpointer        callback_data);
void         _moo_app_input_shutdown    (void);
gboolean     _moo_app_input_running     (void);

gboolean     _moo_app_input_send_msg    (const char     *name,
                                         const char     *data,
                                         gssize          len);
void         _moo_app_input_broadcast   (const char     *header,
                                         const char     *data,
                                         gssize          len);
const char  *_moo_app_input_get_path    (void);


G_END_DECLS

#endif /* MOO_APP_INPUT_H */
