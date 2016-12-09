#ifndef ___PYUTILS_H
#define ___PYUTILS_H

#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(type, size) \
	PyObject_HEAD_INIT(type) size,
#endif

#if PY_MAJOR_VERSION > 2
#define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
#else
#define MOD_INIT(name) PyMODINIT_FUNC init##name(void)
#endif

#ifndef Py_TYPE
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

#if PY_MAJOR_VERSION > 2
#define PYSTR_FROMSTR(str) PyUnicode_FromString(str)
#define PYNUM_FROMLONG(num) PyLong_FromLong(num)
#else
#define PYSTR_FROMSTR(str) PyString_FromString(str)
#define PYNUM_FROMLONG(num) PyInt_FromLong(num)
#endif

static inline int
add_to_dict(PyObject* dict, const char* key_str, PyObject* value)
{
    PyObject *key = PYSTR_FROMSTR(key_str);
    int err = PyDict_SetItem(dict, key, value);
    Py_DECREF(key);
    Py_DECREF(value);
    return err;
}

#endif
