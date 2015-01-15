/*
 * pybgpstream
 *
 * Alistair King, CAIDA, UC San Diego
 * corsaro-info@caida.org
 *
 * Copyright (C) 2012 The Regents of the University of California.
 *
 * This file is part of bgpwatcher.
 *
 * bgpwatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bgpwatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bgpwatcher.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <Python.h>

#include "_pybgpstream_bgpstream.h"
#include "_pybgpstream_bgprecord.h"

static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};

static char *module_docstring =
  "Module that provides an low-level interface to libbgpstream";

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
init_pybgpstream(void)
{
  PyObject *_pybgpstream;
  PyObject *obj;

  _pybgpstream = Py_InitModule3("_pybgpstream",
                                module_methods,
                                module_docstring);
  if (_pybgpstream == NULL)
    return;

  /* BGPStream object */
  if (!(obj = _pybgpstream_bgpstream_get_BGPStreamType()))
    return;
  PyModule_AddObject(_pybgpstream, "BGPStream", obj);

  /* BGPRecord object */
  if (!(obj = _pybgpstream_bgpstream_get_BGPRecordType()))
    return;
  PyModule_AddObject(_pybgpstream, "BGPRecord", obj);
}
