/*
 *   moo-environ.h
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

#ifndef MOO_ENVIRON_H
#define MOO_ENVIRON_H

#include <config.h>

#if defined(MOO_OS_DARWIN)
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#elif !defined(MOO_OS_WIN32)
extern char **environ;
#endif

#endif /* MOO_ENVIRON_H */
