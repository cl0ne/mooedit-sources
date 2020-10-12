/*
 *   moopython.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <Python.h>
#define NO_IMPORT_PYGOBJECT
#define NO_IMPORT_PYGTK
#include <pygobject.h>
G_BEGIN_DECLS
#include <pygtk/pygtk.h>
G_END_DECLS
#include "mooedit/mooplugin-loader.h"
#include "moopython/moopython-builtin.h"
#include "moopython/moopython-api.h"
#include "moopython/moopython-loader.h"
#include "moopython/pygtk/moo-pygtk.h"
#include "moopython/moopython-pygtkmod.h"
#include "mooutils/mooutils-misc.h"
#include "moopython/pygtk/moo-mod.h"

static gboolean create_moo_module (void)
{
    PyObject *moo_module;
    PyObject *code;

    code = Py_CompileString (MOO_PY, "moo.py", Py_file_input);

    if (!code)
        return FALSE;

    moo_module = PyImport_ExecCodeModule ((char*) "moo", code);

    if (!moo_module)
        PyErr_Print ();

    Py_DECREF (code);

    return !PyErr_Occurred ();
}

gboolean
_moo_python_builtin_init (void)
{
    if (!Py_IsInitialized ())
    {
        if (!moo_python_api_init ())
        {
            g_warning ("oops");
            return FALSE;
        }

        if (!_moo_module_init ())
        {
            g_warning ("could not initialize _moo module");
            PyErr_Print ();
            moo_python_api_deinit ();
            return FALSE;
        }

        reset_log_func ();

        if (!create_moo_module ())
        {
            g_warning ("could not initialize moo module");
            PyErr_Print ();
            moo_python_api_deinit ();
            return FALSE;
        }
    }

    if (!moo_plugin_loader_lookup (MOO_PYTHON_PLUGIN_LOADER_ID))
    {
        MooPluginLoader *loader = _moo_python_get_plugin_loader ();
        moo_plugin_loader_register (loader, MOO_PYTHON_PLUGIN_LOADER_ID);
        _moo_python_plugin_loader_free (loader);
    }

    return TRUE;
}
