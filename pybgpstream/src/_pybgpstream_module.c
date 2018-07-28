/*
 * This file is part of pybgpstream
 *
 * CAIDA, UC San Diego
 * bgpstream-info@caida.org
 *
 * Copyright (C) 2012 The Regents of the University of California.
 * Authors: Alistair King, Chiara Orsini
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "_pybgpstream_bgpelem.h"
#include "_pybgpstream_bgprecord.h"
#include "_pybgpstream_bgpstream.h"
#include <Python.h>

static PyMethodDef module_methods[] = {
  {NULL} /* Sentinel */
};

#define ADD_OBJECT(objname)                                                    \
  do {                                                                         \
    if ((obj = _pybgpstream_bgpstream_get_##objname##Type()) == NULL)          \
      return NULL;                                                             \
    if (PyType_Ready(obj) < 0)                                                 \
      return NULL;                                                             \
    Py_INCREF(obj);                                                            \
    PyModule_AddObject(m, #objname, (PyObject *)obj);                          \
  } while (0)

#define MODULE_DOCSTRING                                                       \
  "Module that provides a low-level interface to libbgpstream"

#if PY_MAJOR_VERSION > 2
static struct PyModuleDef module_def = {
  PyModuleDef_HEAD_INIT,
  "_pybgpstream",
  MODULE_DOCSTRING,
  -1,
  module_methods,
  NULL,
  NULL,
  NULL,
  NULL,
};
#endif

#ifndef PyMODINIT_FUNC /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

static PyObject *moduleinit(void)
{
  PyObject *m;
  PyTypeObject *obj;

#if PY_MAJOR_VERSION > 2
  m = PyModule_Create(&module_def);
#else
  m = Py_InitModule3("_pybgpstream", module_methods, MODULE_DOCSTRING);
#endif

  if (m == NULL)
    return NULL;

  /* BGPStream object */
  ADD_OBJECT(BGPStream);

  /* BGPRecord object */
  ADD_OBJECT(BGPRecord);

  /* BGPRecord object */
  ADD_OBJECT(BGPElem);

  return m;
}

#if PY_MAJOR_VERSION > 2
PyMODINIT_FUNC PyInit__pybgpstream(void)
{
  return moduleinit();
}
#else
PyMODINIT_FUNC init_pybgpstream(void)
{
  moduleinit();
}
#endif
