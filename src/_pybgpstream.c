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

#include <bgpstream_lib.h>

typedef struct {
  PyObject_HEAD

  /* BGP Stream Instance Handle */
  bgpstream_t *bs;
} BGPStreamObject;

#define BGPStreamDocstring "BGPStream object"


static void
BGPStream_dealloc(BGPStreamObject *self)
{
  if(self->bs != NULL)
    {
      bgpstream_close(self->bs);
      bgpstream_destroy(self->bs);
    }
  self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
BGPStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  BGPStreamObject *self;

  self = (BGPStreamObject *)type->tp_alloc(type, 0);
  if(self == NULL) {
    return NULL;
  }

  if ((self->bs = bgpstream_create()) == NULL)
    {
      Py_DECREF(self);
      return NULL;
    }

  /** @todo init bgpstream_record here? */

  return (PyObject *)self;
}

static int
BGPStream_init(BGPStreamObject *self,
	       PyObject *args, PyObject *kwds)
{
  return 0;
}

/** @todo add methods to set filters */

/** @todo add methods to start stream */
/*
  if (bgpstream_init(self->bs) < 0) {
    return -1;
  }
*/

static PyObject *
BGPStream_next_record(BGPStreamObject *self)
{
  bgpstream_record_t *bsr;
  int ret;

  PyObject *result;

  /** @todo move into _start method! */
  if (bgpstream_init(self->bs) < 0) {
    PyErr_SetString(PyExc_RuntimeError, "Could not start stream");
    return NULL;
  }

  /** @todo actually create a BGPStreamRecord object and return that */

  if ((bsr = bgpstream_create_record()) == NULL) {
    PyErr_SetString(PyExc_MemoryError, "Could not create a new record");
    return NULL;
  }

  ret = bgpstream_get_next_record(self->bs, bsr);
  if (ret < 0) {
    PyErr_SetString(PyExc_RuntimeError, "Could not get next record");
    return NULL;
  } else if (ret == 0) {
    /* end of stream */
    Py_RETURN_NONE;
  }

  /* bsr is available */

  /* for now, just return a tuple of (PROJECT, COLLECTOR, TIME) */
  result = Py_BuildValue("ssl",
			 bsr->attributes.dump_project,
			 bsr->attributes.dump_collector,
			 bsr->attributes.record_time);
  return result; /* could be NULL, but an exception will have been raised */
}

static PyMethodDef BGPStream_methods[] = {
    {"next_record", (PyCFunction)BGPStream_next_record, METH_NOARGS,
     "Get the next BGPStreamRecord from the stream, or None if "
     "end-of-stream has been reached"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject BGPStreamType = {
  PyObject_HEAD_INIT(NULL)
  0,                                    /* ob_size */
  "_pybgpstream.BGPStream",             /* tp_name */
  sizeof(BGPStreamObject), /* tp_basicsize */
  0,                                    /* tp_itemsize */
  (destructor)BGPStream_dealloc,        /* tp_dealloc */
  0,                                    /* tp_print */
  0,                                    /* tp_getattr */
  0,                                    /* tp_setattr */
  0,                                    /* tp_compare */
  0,                                    /* tp_repr */
  0,                                    /* tp_as_number */
  0,                                    /* tp_as_sequence */
  0,                                    /* tp_as_mapping */
  0,                                    /* tp_hash */
  0,                                    /* tp_call */
  0,                                    /* tp_str */
  0,                                    /* tp_getattro */
  0,                                    /* tp_setattro */
  0,                                    /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
  BGPStreamDocstring,      /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  BGPStream_methods,             /* tp_methods */
  0,             /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)BGPStream_init,  /* tp_init */
  0,                         /* tp_alloc */
  BGPStream_new,             /* tp_new */
};

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
    PyObject* m;

    if (PyType_Ready(&BGPStreamType) < 0)
        return;

    m = Py_InitModule3("_pybgpstream", module_methods, module_docstring);

    if (m == NULL)
      return;

    Py_INCREF(&BGPStreamType);
    PyModule_AddObject(m, "BGPStream", (PyObject *)&BGPStreamType);
}
