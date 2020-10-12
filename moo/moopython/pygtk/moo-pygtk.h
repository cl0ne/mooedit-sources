/*
 *   moo-pygtk.h
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

#ifndef MOO_PYGTK_H
#define MOO_PYGTK_H

#include <Python.h>
#include <mooglib/moo-glib.h>

G_BEGIN_DECLS

gboolean    _moo_module_init            (void);

extern const PyMethodDef _moo_functions[];

void        _moo_register_classes       (PyObject       *dict);
void        _moo_add_constants          (PyObject       *module,
                                         const char     *strip_prefix);

G_END_DECLS

#endif /* MOO_PYGTK_H */
