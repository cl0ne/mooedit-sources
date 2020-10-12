/*
 *   moopython-api.h
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

#ifndef MOO_PYTHON_API_H
#define MOO_PYTHON_API_H


#include "moopython/moopython-utils.h"
#include <Python.h>
#define NO_IMPORT_PYGOBJECT
#include "pygobject.h"
#include <glib/gprintf.h>


// static MooPyObject *
// moo_python_api_run_simple_string (const char *str)
// {
//     PyObject *dict, *main_mod;
//     g_return_val_if_fail (str != NULL, NULL);
//     main_mod = PyImport_AddModule ("__main__");
//     dict = PyModule_GetDict (main_mod);
//     return (MooPyObject*) PyRun_String (str, Py_file_input, dict, dict);
// }
//
//
// static MooPyObject *
// moo_python_api_create_script_dict (const char *name)
// {
//     PyObject *dict, *builtins;
//
//     builtins = PyImport_ImportModule ("__builtin__");
//     g_return_val_if_fail (builtins != NULL, NULL);
//
//     dict = PyDict_New ();
//     PyDict_SetItemString (dict, "__builtins__", builtins);
//
//     if (name)
//     {
//         PyObject *py_name = PyString_FromString (name);
//         PyDict_SetItemString (dict, "__name__", py_name);
//         Py_XDECREF (py_name);
//     }
//
//     Py_XDECREF (builtins);
//     return (MooPyObject*) dict;
// }
//
//
// static MooPyObject*
// moo_python_api_run_string (const char  *str,
//                            MooPyObject *globals,
//                            MooPyObject *locals)
// {
//     PyObject *ret;
//
//     g_return_val_if_fail (str != NULL, NULL);
//
//     if (!locals)
//         locals = moo_python_api_create_script_dict ("__script__");
//     else
//         Py_INCREF ((PyObject*) locals);
//
//     g_return_val_if_fail (locals != NULL, NULL);
//
//     if (!globals)
//         globals = locals;
//
//     ret = PyRun_String (str, Py_file_input, (PyObject*) globals, (PyObject*) locals);
//
//     Py_DECREF ((PyObject*) locals);
//     return (MooPyObject*) ret;
// }
//
//
// static MooPyObject *
// moo_python_api_run_file (gpointer     fp,
//                          const char  *filename,
//                          MooPyObject *globals,
//                          MooPyObject *locals)
// {
//     PyObject *ret;
//
//     g_return_val_if_fail (fp != NULL && filename != NULL, NULL);
//
//     if (!locals)
//         locals = moo_python_api_create_script_dict ("__script__");
//     else
//         Py_INCREF ((PyObject*) locals);
//
//     g_return_val_if_fail (locals != NULL, NULL);
//
//     if (!globals)
//         globals = locals;
//
//     ret = PyRun_File (fp, filename, Py_file_input, (PyObject*) globals, (PyObject*) locals);
//
//     Py_DECREF ((PyObject*) locals);
//     return (MooPyObject*) ret;
// }
//
//
// static MooPyObject *
// moo_python_api_run_code (const char  *str,
//                          MooPyObject *globals,
//                          MooPyObject *locals)
// {
//     PyObject *ret;
//
//     g_return_val_if_fail (str != NULL, NULL);
//
//     if (!locals)
//         locals = moo_python_api_create_script_dict ("__script__");
//     else
//         Py_INCREF ((PyObject*) locals);
//
//     g_return_val_if_fail (locals != NULL, NULL);
//
//     if (!globals)
//         globals = locals;
//
//     ret = PyRun_String (str, Py_file_input, (PyObject*) globals, (PyObject*) locals);
//
//     if (ret)
//     {
//         Py_DECREF (ret);
//
//         if (PyMapping_HasKeyString ((PyObject*) locals, "__retval__"))
//             ret = PyMapping_GetItemString ((PyObject*) locals, "__retval__");
//         else
//             ret = NULL;
//     }
//
//     Py_DECREF ((PyObject*) locals);
//     return (MooPyObject*) ret;
// }
//
//
// static MooPyObject*
// moo_python_api_incref (MooPyObject *obj)
// {
//     Py_XINCREF ((PyObject*) obj);
//     return obj;
// }
//
// static void
// moo_python_api_decref (MooPyObject *obj)
// {
//     Py_XDECREF ((PyObject*) obj);
// }
//
//
// static MooPyObject *
// moo_python_api_py_object_from_gobject (gpointer gobj)
// {
//     g_return_val_if_fail (!gobj || G_IS_OBJECT (gobj), NULL);
//     return (MooPyObject*) pygobject_new (gobj);
// }
//
// static gpointer
// moo_python_api_gobject_from_py_object (MooPyObject *pyobj)
// {
//     GValue val;
//
//     g_return_val_if_fail (pyobj != NULL, NULL);
//
//     val.g_type = 0;
//     g_value_init (&val, G_TYPE_OBJECT);
//
//     if (pyg_value_from_pyobject (&val, (PyObject*) pyobj) == 0)
//     {
//         gpointer ret = g_value_get_object (&val);
//         g_value_unset (&val);
//         return ret;
//     }
//
//     PyErr_Clear ();
//     return NULL;
// }
//
//
// static MooPyObject *
// moo_python_api_dict_get_item (MooPyObject *dict,
//                               const char  *key)
// {
//     g_return_val_if_fail (dict != NULL, NULL);
//     g_return_val_if_fail (key != NULL, NULL);
//     return (MooPyObject*) PyDict_GetItemString ((PyObject*) dict, (char*) key);
// }
//
// static gboolean
// moo_python_api_dict_set_item (MooPyObject *dict,
//                               const char  *key,
//                               MooPyObject *val)
// {
//     g_return_val_if_fail (dict != NULL, FALSE);
//     g_return_val_if_fail (key != NULL, FALSE);
//     g_return_val_if_fail (val != NULL, FALSE);
//
//     if (PyDict_SetItemString ((PyObject*) dict, (char*) key, (PyObject*) val))
//     {
//         PyErr_Print ();
//         return FALSE;
//     }
//
//     return TRUE;
// }
//
// static gboolean
// moo_python_api_dict_del_item (MooPyObject *dict,
//                               const char  *key)
// {
//     g_return_val_if_fail (dict != NULL, FALSE);
//     g_return_val_if_fail (key != NULL, FALSE);
//
//     if (PyDict_DelItemString ((PyObject*) dict, (char*) key))
//     {
//         PyErr_Print ();
//         return FALSE;
//     }
//
//     return TRUE;
// }
//
//
// static void G_GNUC_PRINTF(2,3)
// moo_python_api_set_error (int         type,
//                           const char *format,
//                           ...)
// {
//     PyObject *exception = NULL;
//     char *msg = NULL;
//     va_list args;
//
//     va_start (args, format);
//     g_vasprintf (&msg, format, args);
//     va_end (args);
//
//     g_return_if_fail (msg != NULL);
//
//     switch (type)
//     {
//         case MOO_PY_RUNTIME_ERROR:
//             exception = PyExc_RuntimeError;
//             break;
//         case MOO_PY_TYPE_ERROR:
//             exception = PyExc_TypeError;
//             break;
//         case MOO_PY_VALUE_ERROR:
//             exception = PyExc_ValueError;
//             break;
//         case MOO_PY_NOT_IMPLEMENTED_ERROR:
//             exception = PyExc_NotImplementedError;
//             break;
//     }
//
//     g_return_if_fail (exception != NULL);
//
//     PyErr_SetString (exception, (char*) msg);
// }
//
//
// static void
// moo_python_api_py_err_print (void)
// {
//     PyErr_Print ();
// }
//
//
// static char *
// moo_python_api_get_info (void)
// {
//     return g_strdup_printf ("%s %s", Py_GetVersion (), Py_GetPlatform ());
// }
//
//
// static MooPyObject *
// moo_python_api_import_exec (const char  *name,
//                             const char  *string)
// {
//     PyObject *code;
//     PyObject *mod;
//     char *filename;
//
//     g_return_val_if_fail (name != NULL, NULL);
//     g_return_val_if_fail (string != NULL, NULL);
//
//     filename = g_strdup_printf ("%s.py", name);
//     code = Py_CompileString (string, filename, Py_file_input);
//     g_free (filename);
//
//     if (!code)
//         return FALSE;
//
//     mod = PyImport_ExecCodeModule ((char*) name, code);
//
//     Py_DECREF (code);
//     return (MooPyObject*) mod;
// }
//
//
// static MooPyObject *
// call_function_valist (MooPyObject *callable,
//                       const char  *format,
//                       va_list      vargs)
// {
//     PyObject *args = NULL, *ret = NULL;
//
//     if (format && *format)
//         args = Py_VaBuildValue ((char*) format, vargs);
//     else
//         args = PyTuple_New (0);
//
//     if (!args)
//         goto out;
//
//     if (!PyTuple_Check (args))
//     {
//         PyObject *tmp = PyTuple_New (1);
//
//         if (!tmp)
//             goto out;
//
//         if (PyTuple_SetItem (tmp, 0, args) < 0)
//         {
//             Py_XDECREF (tmp);
//             goto out;
//         }
//
//         args = tmp;
//     }
//
//     ret = PyObject_Call ((PyObject*) callable, args, NULL);
//
// out:
//     Py_XDECREF (args);
//     return (MooPyObject*) ret;
// }
//
// static MooPyObject *
// moo_python_api_py_object_call_method (MooPyObject *object,
//                                       const char  *method,
//                                       const char  *format,
//                                       ...)
// {
//     va_list vargs;
//     MooPyObject *ret;
//     PyObject *callable;
//
//     callable = PyObject_GetAttrString ((PyObject*) object, (char*) method);
//
//     if (!callable)
//         return NULL;
//
//     va_start (vargs, format);
//     ret = call_function_valist ((MooPyObject*) callable, format, vargs);
//     va_end (vargs);
//
//     Py_XDECREF (callable);
//     return (MooPyObject*) ret;
// }
//
// static MooPyObject *
// moo_python_api_py_object_call_function (MooPyObject *callable,
//                                         const char  *format,
//                                         ...)
// {
//     va_list vargs;
//     MooPyObject *ret;
//
//     va_start (vargs, format);
//     ret = call_function_valist (callable, format, vargs);
//     va_end (vargs);
//
//     return (MooPyObject*) ret;
// }
//
//
// static int
// convert_meth_flags (int moo_flags)
// {
//     int flags = 0;
//
//     if (moo_flags & MOO_PY_METH_VARARGS)
//         flags |= METH_VARARGS;
//     if (moo_flags & MOO_PY_METH_KEYWORDS)
//         flags |= METH_KEYWORDS;
//     if (moo_flags & MOO_PY_METH_NOARGS)
//         flags |= METH_NOARGS;
//     if (moo_flags & MOO_PY_METH_O)
//         flags |= METH_O;
//     if (moo_flags & MOO_PY_METH_CLASS)
//         flags |= METH_CLASS;
//     if (moo_flags & MOO_PY_METH_STATIC)
//         flags |= METH_STATIC;
//
//     return flags;
// }
//
// static MooPyObject *
// moo_python_api_py_c_function_new (MooPyMethodDef *meth,
//                                   MooPyObject    *self)
// {
//     PyMethodDef *py_meth;
//
//     g_return_val_if_fail (meth != NULL, NULL);
//     g_return_val_if_fail (meth->ml_meth != NULL, NULL);
//     g_return_val_if_fail (meth->ml_name != NULL, NULL);
//
//     py_meth = g_new0 (PyMethodDef, 1);
//     moo_python_add_data (py_meth, g_free);
//
//     py_meth->ml_name = (char*) meth->ml_name;
//     py_meth->ml_meth = (PyCFunction) meth->ml_meth;
//     py_meth->ml_flags = convert_meth_flags (meth->ml_flags);
//     py_meth->ml_doc = (char*) meth->ml_doc;
//
//     return (MooPyObject*) PyCFunction_New (py_meth, (PyObject*) self);
// }
//
//
// static int
// moo_python_api_py_module_add_object (MooPyObject *mod,
//                                      const char  *name,
//                                      MooPyObject *obj)
// {
//     return PyModule_AddObject ((PyObject*) mod, (char*) name, (PyObject*) obj);
// }
//
//
// static gboolean
// moo_python_api_py_arg_parse_tuple (MooPyObject *args,
//                                    const char  *format,
//                                    ...)
// {
//     int ret;
//     va_list vargs;
//
//     va_start (vargs, format);
//     ret = PyArg_VaParse ((PyObject*) args, (char*) format, vargs);
//     va_end (vargs);
//
//     return ret;
// }


static gboolean
moo_python_api_init (void)
{
    static int argc;
    static char **argv;

//     static MooPyAPI api = {
//         NULL, NULL, NULL,
//         moo_python_api_incref,
//         moo_python_api_decref,
//         moo_python_api_get_info,
//         moo_python_api_run_simple_string,
//         moo_python_api_run_string,
//         moo_python_api_run_file,
//         moo_python_api_run_code,
//         moo_python_api_create_script_dict,
//         moo_python_api_py_object_from_gobject,
//         moo_python_api_gobject_from_py_object,
//         moo_python_api_dict_get_item,
//         moo_python_api_dict_set_item,
//         moo_python_api_dict_del_item,
//         moo_python_api_import_exec,
//
//         moo_python_api_set_error,
//         moo_python_api_py_err_print,
//         moo_python_api_py_object_call_method,
//         moo_python_api_py_object_call_function,
//         moo_python_api_py_c_function_new,
//         moo_python_api_py_module_add_object,
//         moo_python_api_py_arg_parse_tuple,
//         NULL
//     };
//
//     g_return_val_if_fail (!moo_python_running(), FALSE);
//
//     if (!moo_python_init (MOO_PY_API_VERSION, &api))
//     {
//         g_warning ("oops");
//         return FALSE;
//     }
//
//     g_assert (moo_python_running ());

    if (!Py_IsInitialized ())
    {
        if (!argv)
        {
            argc = 1;
            argv = g_new0 (char*, 2);
            argv[0] = g_strdup ("");
        }

        /* do not let python install signal handlers */
        Py_InitializeEx (FALSE);

        /* pygtk wants sys.argv */
        PySys_SetArgv (argc, argv);
        _moo_py_init_print_funcs ();
    }

//     api.py_none = (gpointer) Py_None;
//
// #ifndef MOO_DEBUG_ENABLED
//     api.py_true = (MooPyObject*) Py_True;
//     api.py_false = (MooPyObject*) Py_False;
// #else
//     /* avoid strict aliasing warnings */
//     api.py_true = (gpointer) &(_Py_TrueStruct);
//     api.py_false = (gpointer) &(_Py_ZeroStruct);
// #endif

    return TRUE;
}


inline static void
moo_python_api_deinit (void)
{
//     moo_python_init (MOO_PY_API_VERSION, NULL);
}


#endif /* MOO_PYTHON_API_H */
