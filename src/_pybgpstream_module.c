/*
 * This file is part of bgpstream
 *
 * Copyright (C) 2015 The Regents of the University of California.
 * Authors: Alistair King, Chiara Orsini
 *
 * All rights reserved.
 *
 * This code has been developed by CAIDA at UC San Diego.
 * For more information, contact bgpstream-info@caida.org
 *
 * This source code is proprietary to the CAIDA group at UC San Diego and may
 * not be redistributed, published or disclosed without prior permission from
 * CAIDA.
 *
 * Report any bugs, questions or comments to bgpstream-info@caida.org
 *
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
