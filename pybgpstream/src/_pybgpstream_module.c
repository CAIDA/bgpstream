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

#include <Python.h>

#include "_pybgpstream_bgpstream.h"
#include "_pybgpstream_bgprecord.h"
#include "_pybgpstream_bgpelem.h"

static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};

#define ADD_OBJECT(objname)                                             \
  do {                                                                  \
    if ((obj = _pybgpstream_bgpstream_get_##objname##Type()) == NULL)   \
      return;                                                           \
    if (PyType_Ready(obj) < 0)                                          \
      return;                                                           \
    Py_INCREF(obj);                                                     \
    PyModule_AddObject(_pybgpstream, #objname, (PyObject*)obj);         \
  } while(0)

static char *module_docstring =
  "Module that provides a low-level interface to libbgpstream";

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
init_pybgpstream(void)
{
  PyObject *_pybgpstream;
  PyTypeObject *obj;

  _pybgpstream = Py_InitModule3("_pybgpstream",
                                module_methods,
                                module_docstring);
  if (_pybgpstream == NULL)
    return;

  /* BGPStream object */
  ADD_OBJECT(BGPStream);

  /* BGPRecord object */
  ADD_OBJECT(BGPRecord);

  /* BGPRecord object */
  ADD_OBJECT(BGPElem);
}
