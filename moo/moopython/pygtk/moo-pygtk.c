/*
 *   moo-pygtk.c
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

#include <Python.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "moopython/pygtk/moo-pygtk.h"
#include "moopython/moopython-utils.h"
#include <pygobject.h>  /* _PyGObjectAPI lives here */
G_BEGIN_DECLS
#include <pygtk/pygtk.h>
G_END_DECLS
#include <mooglib/moo-glib.h>
#include "moopython/moopython-pygtkmod.h"
#include <mooutils/moostock.h>

/**
 * moo_window_class_add_action: (moo.lua 0) (moo.private 1)
 **/

static void     init_moo_utils          (PyObject       *module);

static PyObject *
moo_version (void)
{
    return PyString_FromString (MOO_VERSION);
}

static PyObject *
moo_detailed_version (void)
{
    PyObject *res = PyDict_New ();
    g_return_val_if_fail (res != NULL, NULL);

    PyDict_SetItemString (res, "full", PyString_FromString (MOO_VERSION));
    PyDict_SetItemString (res, "major", PyInt_FromLong (MOO_MAJOR_VERSION));
    PyDict_SetItemString (res, "minor", PyInt_FromLong (MOO_MINOR_VERSION));
    PyDict_SetItemString (res, "micro", PyInt_FromLong (MOO_MICRO_VERSION));

    return res;
}

static const char _moo_module_doc[] = "_moo module.";

static PyObject *
py_object_from_moo_py_object (const GValue *value)
{
    PyObject *obj;

    g_return_val_if_fail (G_VALUE_TYPE (value) == MOO_TYPE_PY_OBJECT, NULL);

    obj = g_value_get_boxed (value);

    if (!obj)
        obj = Py_None;

    return _moo_py_object_ref (obj);
}

static int
py_object_to_moo_py_object (GValue *value, PyObject *obj)
{
    g_value_set_boxed (value, obj == Py_None ? obj : NULL);
    return 0;
}

gboolean
_moo_module_init (void)
{
    PyObject *_moo_module = NULL;

    init_pygtk_mod ();

    if (PyErr_Occurred ())
        return FALSE;

    pyg_register_boxed_custom (MOO_TYPE_PY_OBJECT,
                               py_object_from_moo_py_object,
                               py_object_to_moo_py_object);

    if (PyErr_Occurred ())
        return FALSE;

    _moo_module = Py_InitModule3 ("_moo", (PyMethodDef*) _moo_functions, (char*) _moo_module_doc);

    if (!_moo_module)
        return FALSE;

    PyModule_AddObject (_moo_module, "version", moo_version());
    if (PyErr_Occurred ())
        return FALSE;

    PyModule_AddObject (_moo_module, "detailed_version", moo_detailed_version());
    if (PyErr_Occurred ())
        return FALSE;

    init_moo_utils (_moo_module);
    if (PyErr_Occurred ())
        return FALSE;

    _moo_add_constants (_moo_module, "MOO_");
    if (PyErr_Occurred ())
        return FALSE;

    _moo_register_classes (PyModule_GetDict (_moo_module));
    if (PyErr_Occurred ())
        return FALSE;

    return TRUE;
}

static void
init_moo_utils (PyObject *module)
{
    PyModule_AddStringConstant (module, "GETTEXT_PACKAGE", GETTEXT_PACKAGE);

    PyModule_AddStringConstant (module, "STOCK_TERMINAL", MOO_STOCK_TERMINAL);
    PyModule_AddStringConstant (module, "STOCK_KEYBOARD", MOO_STOCK_KEYBOARD);
    PyModule_AddStringConstant (module, "STOCK_RESTART", MOO_STOCK_RESTART);
    PyModule_AddStringConstant (module, "STOCK_DOC_DELETED", MOO_STOCK_DOC_DELETED);
    PyModule_AddStringConstant (module, "STOCK_DOC_MODIFIED_ON_DISK", MOO_STOCK_DOC_MODIFIED_ON_DISK);
    PyModule_AddStringConstant (module, "STOCK_DOC_DELETED", MOO_STOCK_DOC_DELETED);
    PyModule_AddStringConstant (module, "STOCK_DOC_MODIFIED", MOO_STOCK_DOC_MODIFIED);
    PyModule_AddStringConstant (module, "STOCK_FILE_SELECTOR", MOO_STOCK_FILE_SELECTOR);
    PyModule_AddStringConstant (module, "STOCK_SAVE_NONE", MOO_STOCK_SAVE_NONE);
    PyModule_AddStringConstant (module, "STOCK_SAVE_SELECTED", MOO_STOCK_SAVE_SELECTED);
    PyModule_AddStringConstant (module, "STOCK_NEW_PROJECT", MOO_STOCK_NEW_PROJECT);
    PyModule_AddStringConstant (module, "STOCK_OPEN_PROJECT", MOO_STOCK_OPEN_PROJECT);
    PyModule_AddStringConstant (module, "STOCK_CLOSE_PROJECT", MOO_STOCK_CLOSE_PROJECT);
    PyModule_AddStringConstant (module, "STOCK_PROJECT_OPTIONS", MOO_STOCK_PROJECT_OPTIONS);
    PyModule_AddStringConstant (module, "STOCK_BUILD", MOO_STOCK_BUILD);
    PyModule_AddStringConstant (module, "STOCK_COMPILE", MOO_STOCK_COMPILE);
    PyModule_AddStringConstant (module, "STOCK_EXECUTE", MOO_STOCK_EXECUTE);
    PyModule_AddStringConstant (module, "STOCK_FIND_IN_FILES", MOO_STOCK_FIND_IN_FILES);
    PyModule_AddStringConstant (module, "STOCK_FIND_FILE", MOO_STOCK_FIND_FILE);
    PyModule_AddStringConstant (module, "STOCK_PLUGINS", MOO_STOCK_PLUGINS);
}
