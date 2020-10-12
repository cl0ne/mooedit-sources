/*
 *   moopython/moopython-utils.c
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
# include "config.h"
#endif
#include "moopython/moopython-utils.h"
#include "mooutils/mooutils-misc.h"
#include "mooutils/mootype-macros.h"
#define NO_IMPORT_PYGOBJECT
#include "pygobject.h"

MOO_DEFINE_BOXED_TYPE_R (MooPyObject, _moo_py_object)

gboolean
moo_python_add_path (const char *dir)
{
    PyObject *path;
    PyObject *s;

    g_return_val_if_fail (dir != NULL, FALSE);

    path = PySys_GetObject ((char*) "path");

    if (!path)
    {
        PyErr_Print ();
        return FALSE;
    }

    if (!PyList_Check (path))
    {
        g_critical ("sys.path is not a list");
        return FALSE;
    }

    s = PyString_FromString (dir);
    PyList_Append (path, s);

    Py_DECREF (s);
    return TRUE;
}

void
moo_python_remove_path (const char *dir)
{
    PyObject *path;
    int i;

    g_return_if_fail (dir != NULL);

    path = PySys_GetObject ((char*) "path");

    if (!path || !PyList_Check (path))
        return;

    for (i = PyList_GET_SIZE (path) - 1; i >= 0; --i)
    {
        PyObject *item = PyList_GET_ITEM (path, i);

        if (PyString_CheckExact (item) &&
            !strcmp (PyString_AsString (item), dir))
        {
            if (PySequence_DelItem (path, i) != 0)
                PyErr_Print ();
            break;
        }
    }
}

static PyTypeObject *
moo_get_pygobject_type (void)
{
    static PyTypeObject *type;

    if (G_UNLIKELY (!type))
    {
        PyObject *module = PyImport_ImportModule ("gobject");
        g_return_val_if_fail (module != NULL, NULL);
        type = (PyTypeObject*) PyObject_GetAttrString (module, "GObject");
        g_return_val_if_fail (type != NULL, NULL);
    }

    return type;
}

PyObject *
_moo_strv_to_pyobject (char **strv)
{
    PyObject *result;
    guint len, i;

    if (!strv)
        return_None;

    len = g_strv_length (strv);
    result = PyTuple_New (len);

    for (i = 0; i < len; ++i)
    {
        PyTuple_SET_ITEM (result, i,
                          PyString_FromString (strv[i]));
    }

    return result;
}


static char **
_moo_pyobject_to_strv_no_check (PyObject *seq,
                                int       len)
{
#define CACHE_SIZE 10
    static char **cache[CACHE_SIZE];
    static guint n;
    int i;
    char **ret;

    g_strfreev (cache[n]);

    cache[n] = ret = g_new (char*, len + 1);
    ret[len] = NULL;

    for (i = 0; i < len; ++i)
    {
        PyObject *item = PySequence_ITEM (seq, i);
        ret[i] = g_strdup (PyString_AS_STRING (item));
    }

    if (++n == CACHE_SIZE)
        n = 0;

    return ret;
#undef CACHE_SIZE
}


int _moo_pyobject_to_strv (PyObject *obj, char ***dest)
{
    int len, i;

    if (obj == Py_None)
    {
        *dest = NULL;
        return TRUE;
    }

    if (!PySequence_Check (obj))
    {
        PyErr_SetString (PyExc_TypeError,
                         "argument must be a sequence");
        return FALSE;
    }

    len = PySequence_Size (obj);

    if (len < 0)
    {
        PyErr_SetString (PyExc_RuntimeError,
                         "got negative length of a sequence");
        return FALSE;
    }

    for (i = 0; i < len; ++i)
    {
        PyObject *item = PySequence_ITEM (obj, i);

        g_return_val_if_fail (item != NULL, FALSE);

        if (!PyString_Check (item))
        {
            PyErr_SetString (PyExc_TypeError,
                             "argument must be a sequence of strings");
            return FALSE;
        }
    }

    *dest = _moo_pyobject_to_strv_no_check (obj, len);

    return TRUE;
}

int
_moo_pyobject_to_strv_no_null (PyObject  *obj,
                               char    ***dest)
{
    if (obj == Py_None)
    {
        PyErr_SetString (PyExc_TypeError,
                         "argument must be a sequence, not None");
        return FALSE;
    }

    return _moo_pyobject_to_strv (obj, dest);
}


static MooObjectArray *
_moo_pyobject_to_object_array_no_check (PyObject *seq,
                                        int       len)
{
    int i;
    MooObjectArray *ar;

    ar = moo_object_array_new ();

    for (i = 0; i < len; ++i)
    {
        PyObject *item = PySequence_ITEM (seq, i);
        moo_object_array_append (ar, pygobject_get (item));
    }

    return ar;
}

int
_moo_pyobject_to_object_array (PyObject        *obj,
                               MooObjectArray **dest)
{
    int len, i;
    PyTypeObject *type;

    type = moo_get_pygobject_type ();
    g_return_val_if_fail (type != NULL, FALSE);

    if (obj == Py_None)
    {
        *dest = NULL;
        return TRUE;
    }

    if (!PySequence_Check (obj))
    {
        PyErr_SetString (PyExc_TypeError,
                         "argument must be a sequence");
        return FALSE;
    }

    len = PySequence_Size (obj);

    if (len < 0)
    {
        PyErr_SetString (PyExc_RuntimeError,
                         "got negative length of a sequence");
        return FALSE;
    }

    for (i = 0; i < len; ++i)
    {
        PyObject *item = PySequence_ITEM (obj, i);

        g_return_val_if_fail (item != NULL, FALSE);

        if (!pygobject_check (item, type))
        {
            PyErr_SetString (PyExc_TypeError,
                             "argument must be a sequence of objects");
            return FALSE;
        }
    }

    *dest = _moo_pyobject_to_object_array_no_check (obj, len);

    return TRUE;
}

int
_moo_pyobject_to_object_array_no_null (PyObject        *obj,
                                       MooObjectArray **dest)
{
    if (obj == Py_None)
    {
        PyErr_SetString (PyExc_TypeError,
                         "argument must be a sequence, not None");
        return FALSE;
    }

    return _moo_pyobject_to_object_array (obj, dest);
}


PyObject *_moo_gvalue_to_pyobject (const GValue *val)
{
    PyObject *obj;

    obj = pyg_value_as_pyobject (val, TRUE);

    if (!obj && !PyErr_Occurred ())
    {
        g_critical ("oops");
        PyErr_Format (PyExc_TypeError, "could not convert value of type %s",
                      g_type_name (G_VALUE_TYPE (val)));
    }

    return obj;
}


void
_moo_pyobject_to_gvalue (PyObject *object,
                         GValue   *value)
{
    GType type;

    g_return_if_fail (value != NULL && !G_IS_VALUE (value));
    g_return_if_fail (object != NULL);

    type = pyg_type_from_object (object);

    if (type != 0 && type != G_TYPE_NONE)
    {
        g_value_init (value, type);

        if (pyg_value_from_pyobject (value, object) == 0)
            return;

        g_critical ("oops");
        g_value_unset (value);
    }

    PyErr_Clear ();
    g_value_init (value, MOO_TYPE_PY_OBJECT);

    if (object != Py_None)
        g_value_set_boxed (value, object);
    else
        g_value_set_boxed (value, NULL);
}



typedef PyObject *(*PtrToPy) (gpointer  ptr,
                              gpointer  data);

static PyObject *
slist_to_pyobject (GSList  *list,
                   PtrToPy  func,
                   gpointer data)
{
    int i;
    GSList *l;
    PyObject *result;

    result = PyList_New (g_slist_length (list));

    for (i = 0, l = list; l != NULL; l = l->next, ++i)
    {
        PyObject *item = func (l->data, data);

        if (!item)
        {
            Py_DECREF (result);
            return NULL;
        }

        PyList_SetItem (result, i, item);
    }

    return result;
}


PyObject *
_moo_object_slist_to_pyobject (GSList *list)
{
    return slist_to_pyobject (list, (PtrToPy) pygobject_new, NULL);
}


PyObject *
_moo_object_array_to_pyobject (MooObjectArray *array)
{
    guint i;
    PyObject *result;

    result = PyList_New (moo_object_array_get_size (array));

    for (i = 0; array && i < array->n_elms; ++i)
    {
        PyObject *item = pygobject_new (array->elms[i]);

        if (!item)
        {
            Py_DECREF (result);
            return NULL;
        }

        PyList_SetItem (result, i, item);
    }

    return result;
}


static PyObject *
string_to_pyobject (gpointer str)
{
    if (!str)
        return_RuntimeError ("got NULL string");
    else
        return PyString_FromString (str);
}

PyObject *
_moo_string_slist_to_pyobject (GSList *list)
{
    return slist_to_pyobject (list, (PtrToPy) string_to_pyobject, NULL);
}


static PyObject *
boxed_to_pyobject (gpointer boxed,
                   gpointer type)
{
    return pyg_boxed_new (GPOINTER_TO_SIZE (type), boxed, TRUE, TRUE);
}

PyObject *
_moo_boxed_slist_to_pyobject (GSList *list,
                              GType   type)
{
    g_return_val_if_fail (g_type_is_a (type, G_TYPE_BOXED), NULL);
    return slist_to_pyobject (list, boxed_to_pyobject,
                              GSIZE_TO_POINTER (type));
}


PyObject *
_moo_py_object_ref (PyObject *obj)
{
    Py_XINCREF (obj);
    return obj;
}


void
_moo_py_object_unref (PyObject *obj)
{
    Py_XDECREF (obj);
}


char *
_moo_py_err_string (void)
{
    PyObject *exc, *value, *tb;
    PyObject *str_exc, *str_value, *str_tb;
    GString *string;

    PyErr_Fetch (&exc, &value, &tb);
    PyErr_NormalizeException (&exc, &value, &tb);

    if (!exc)
        return NULL;

    string = g_string_new (NULL);

    str_exc = PyObject_Str (exc);
    str_value = PyObject_Str (value);
    str_tb = PyObject_Str (tb);

    if (str_exc)
        g_string_append_printf (string, "%s\n", PyString_AS_STRING (str_exc));
    if (str_value)
        g_string_append_printf (string, "%s\n", PyString_AS_STRING (str_value));
    if (str_tb)
        g_string_append_printf (string, "%s\n", PyString_AS_STRING (str_tb));

    Py_XDECREF(exc);
    Py_XDECREF(value);
    Py_XDECREF(tb);
    Py_XDECREF(str_exc);
    Py_XDECREF(str_value);
    Py_XDECREF(str_tb);

    return g_string_free (string, FALSE);
}


/***********************************************************************/
/* File-like object for sys.stdout and sys.stderr
 */

typedef struct {
    PyObject_HEAD
    GPrintFunc write_func;
} MooPyFile;


static PyObject *
_moo_py_file_close (G_GNUC_UNUSED PyObject *self)
{
    return_None;
}


static PyObject *
_moo_py_file_flush (G_GNUC_UNUSED PyObject *self)
{
    return_None;
}


static PyObject *
_moo_py_file_write (PyObject *self, PyObject *args)
{
    char *string;
    MooPyFile *file = (MooPyFile *) self;

    if (!PyArg_ParseTuple (args, "s", &string))
        return NULL;

    if (!file->write_func)
        return_RuntimeError ("no write function installed");

    file->write_func (string);
    return_None;
}


static PyMethodDef MooPyFile_methods[] = {
    { "close", (PyCFunction) _moo_py_file_close, METH_NOARGS, NULL },
    { "flush", (PyCFunction) _moo_py_file_flush, METH_NOARGS, NULL },
    { "write", (PyCFunction) _moo_py_file_write, METH_VARARGS, NULL },
    { NULL, NULL, 0, NULL }
};


static PyTypeObject MooPyFile_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "MooPyFile",                /* tp_name */
    sizeof (MooPyFile),                 /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor) 0,                     /* tp_dealloc */
    (printfunc) 0,                      /* tp_print */
    (getattrfunc) 0,                    /* tp_getattr */
    (setattrfunc) 0,                    /* tp_setattr */
    (cmpfunc) 0,                        /* tp_compare */
    (reprfunc) 0,                       /* tp_repr */
    (PyNumberMethods*) 0,               /* tp_as_number */
    (PySequenceMethods*) 0,             /* tp_as_sequence */
    (PyMappingMethods*) 0,              /* tp_as_mapping */
    (hashfunc) 0,                       /* tp_hash */
    (ternaryfunc) 0,                    /* tp_call */
    (reprfunc) 0,                       /* tp_str */
    (getattrofunc) 0,                   /* tp_getattro */
    (setattrofunc) 0,                   /* tp_setattro */
    (PyBufferProcs*) 0,                 /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    NULL,                               /* Documentation string */
    (traverseproc) 0,                   /* tp_traverse */
    (inquiry) 0,                        /* tp_clear */
    (richcmpfunc) 0,                    /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    (getiterfunc) 0,                    /* tp_iter */
    (iternextfunc) 0,                   /* tp_iternext */
    MooPyFile_methods,                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    NULL,                               /* tp_base */
    NULL,                               /* tp_dict */
    (descrgetfunc) 0,                   /* tp_descr_get */
    (descrsetfunc) 0,                   /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc) 0,         /* tp_init */
    (allocfunc) 0,                      /* tp_alloc */
    (newfunc) 0,                        /* tp_new */
    (freefunc) 0,                       /* tp_free */
    (inquiry) 0,                        /* tp_is_gc */
    NULL, NULL, NULL, NULL, NULL, NULL
};


static void
set_file (const char *name,
          GPrintFunc  func)
{
    MooPyFile *file;

    file = PyObject_New (MooPyFile, &MooPyFile_Type);

    if (!file)
    {
        PyErr_Print ();
        return;
    }

    file->write_func = func;

    if (PySys_SetObject ((char*) name, (PyObject*) file))
        PyErr_Print ();

    Py_DECREF (file);
}

static void
print_func (const char *string)
{
    g_print ("%s", string);
}

static void
printerr_func (const char *string)
{
    g_printerr ("%s", string);
}

void
_moo_py_init_print_funcs (void)
{
    static gboolean done;

    if (done)
        return;

    done = TRUE;

    if (PyType_Ready (&MooPyFile_Type) < 0)
    {
        g_critical ("could not init MooPyFile type");
        return;
    }

    set_file ("stdout", print_func);
    set_file ("stderr", printerr_func);
}
