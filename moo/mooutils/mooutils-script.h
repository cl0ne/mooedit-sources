/*
 *   mooutils-script.h
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

#ifndef MOO_UTILS_SCRIPT_H
#define MOO_UTILS_SCRIPT_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

void         moo_spin_main_loop     (double      sec);

char        *moo_tempdir            (void);
char        *moo_tempnam            (const char *extension);
void         moo_cleanup            (void);

G_END_DECLS

#endif /* MOO_UTILS_SCRIPT_H */
