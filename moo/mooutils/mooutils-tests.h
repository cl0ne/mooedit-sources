/*
 *   mooutils-tests.h
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

#ifndef MOO_UTILS_TESTS_H
#define MOO_UTILS_TESTS_H

#include "moo-test-macros.h"

G_BEGIN_DECLS

void    moo_test_gobject            (void);
void    moo_test_mooaccel           (void);
void    moo_test_mooutils_fs        (void);
void    moo_test_moo_file_writer    (void);
void    moo_test_mooutils_misc      (void);
void    moo_test_i18n               (MooTestOptions opts);

#ifdef __WIN32__
void    moo_test_mooutils_win32     (void);
#endif

G_END_DECLS

#endif /* MOO_UTILS_TESTS_H */
