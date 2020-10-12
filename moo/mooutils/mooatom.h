/*
 *   mooutils-misc.h
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

#ifndef MOO_ATOM_H
#define MOO_ATOM_H

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define MOO_DEFINE_ATOM_(Name)                                                              \
    static GdkAtom atom;                                                                    \
                                                                                            \
    if (!atom)                                                                              \
        atom = gdk_atom_intern_static_string (#Name);                                       \
                                                                                            \
    return atom;                                                                            \

#define MOO_DEFINE_ATOM(Name, name)                                                         \
static GdkAtom name ## _atom (void) G_GNUC_CONST;                                           \
static GdkAtom name ## _atom (void)                                                         \
{                                                                                           \
    MOO_DEFINE_ATOM_ (Name)                                                                 \
}

#define MOO_DECLARE_ATOM_GLOBAL(Name, name)                                                 \
GdkAtom name ## _atom (void) G_GNUC_CONST;

#define MOO_DEFINE_ATOM_GLOBAL(Name, name)                                                  \
GdkAtom name ## _atom (void)                                                                \
{                                                                                           \
    MOO_DEFINE_ATOM_ (Name)                                                                 \
}

GdkAtom     moo_atom_uri_list   (void) G_GNUC_CONST;

G_END_DECLS

#endif /* MOO_ATOM_H */
