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
#else
#define PYSTR_FROMSTR(str) PyString_FromString(str)
#endif

#endif
