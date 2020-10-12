/*
 *   mooapp-ipc.h
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

#ifndef MOO_APP_IPC_H
#define MOO_APP_IPC_H

#include <glib-object.h>

G_BEGIN_DECLS


typedef void (*MooDataCallback) (GObject    *object,
                                 const char *data,
                                 gsize       len);

void    moo_ipc_register_client     (GObject        *object,
                                     const char     *id,
                                     MooDataCallback callback);
void    moo_ipc_unregister_client   (GObject        *object,
                                     const char     *id);

void    moo_ipc_send                (GObject        *sender,
                                     const char     *id,
                                     const char     *data,
                                     gssize          len);

void   _moo_ipc_dispatch            (const char *data,
                                     gsize       len);


G_END_DECLS

#endif /* MOO_APP_IPC_H */
