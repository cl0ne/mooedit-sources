/*
 *   moopython-pygtkmod.h
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


#include "mooutils/mooutils-misc.h"


static void *
get_object_pointer_because_python_folks_are_funny (const char *module,
                                                   const char *key)
{
    PyObject *pymod;
    PyObject *mdict;
    PyObject *cobject;
    gpointer ret = NULL;

    if (!(pymod = PyImport_ImportModule (module)))
        return NULL;

    mdict = PyModule_GetDict (pymod);
    cobject = PyDict_GetItemString (mdict, key);

#if PY_VERSION_HEX >= 0x02070000
    if (cobject && PyCapsule_CheckExact(cobject))
    {
        char *name = g_strdup_printf ("%s.%s", module, key);
        ret = PyCapsule_GetPointer (cobject, name);
        g_free (name);
    }
    else
#endif
    if (cobject && PyCObject_Check(cobject))
    {
        ret = PyCObject_AsVoidPtr (cobject);
    }

    if (!ret)
    {
        char *message = g_strdup_printf ("could not find %s object", key);
        PyErr_SetString (PyExc_RuntimeError, message);
        g_free (message);
    }

    Py_XDECREF (pymod);
    return ret;
}


G_GNUC_UNUSED static void
init_pygtk_mod (void)
{
    _PyGObject_API = (struct _PyGObject_Functions *)
        get_object_pointer_because_python_folks_are_funny ("gobject", "_PyGObject_API");

    if (!_PyGObject_API)
        return;

    _PyGtk_API = (struct _PyGtk_FunctionStruct*)
        get_object_pointer_because_python_folks_are_funny ("gtk._gtk", "_PyGtk_API");
}


inline static gboolean
check_pygtk_version (const char *module,
                     int         req_major,
                     int         req_minor,
                     int         req_micro)
{
    PyObject *mod, *mdict, *version;
    int found_major, found_minor, found_micro;

    mod = PyImport_ImportModule ((char*) module);
    g_return_val_if_fail (mod != NULL, FALSE);

    mdict = PyModule_GetDict (mod);

    version = PyDict_GetItemString (mdict, "pygobject_version");

    if (!version)
        version = PyDict_GetItemString (mdict, "pygtk_version");

    if (!version)
        return FALSE;

    if (!PyArg_ParseTuple (version, "iii", &found_major, &found_minor, &found_micro))
    {
        PyErr_Print ();
        return FALSE;
    }

    if (req_major != found_major ||
        req_minor > found_minor ||
        (req_minor == found_minor && req_micro > found_micro))
            return FALSE;

    return TRUE;
}


G_GNUC_UNUSED static void
reset_log_func (void)
{
#ifdef pyg_disable_warning_redirections
    if (check_pygtk_version ("gobject", 2, 12, 0))
        pyg_disable_warning_redirections ();
    else
        moo_reset_log_func ();
#else
    moo_reset_log_func ();
#endif
}
