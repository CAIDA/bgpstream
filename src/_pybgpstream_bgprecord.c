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

  /* BGP Stream Record instance Handle */
  bgpstream_record_t *rec;
} BGPRecordObject;

#define BGPRecordDocstring "BGPRecord object"


static void
BGPRecord_dealloc(BGPRecordObject *self)
{
  if(self->rec != NULL)
    {
      bgpstream_destroy_record(self->rec);
    }
  self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
BGPRecord_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  BGPRecordObject *self;

  self = (BGPRecordObject *)type->tp_alloc(type, 0);
  if(self == NULL) {
    return NULL;
  }

  if ((self->rec = bgpstream_create_record()) == NULL)
    {
      Py_DECREF(self);
      return NULL;
    }

  return (PyObject *)self;
}

static int
BGPRecord_init(BGPRecordObject *self,
	       PyObject *args, PyObject *kwds)
{
  return 0;
}


static PyMethodDef BGPRecord_methods[] = {
  {NULL}  /* Sentinel */
};

static PyTypeObject BGPRecordType = {
  PyObject_HEAD_INIT(NULL)
  0,                                    /* ob_size */
  "_pybgpstream.BGPRecord",             /* tp_name */
  sizeof(BGPRecordObject), /* tp_basicsize */
  0,                                    /* tp_itemsize */
  (destructor)BGPRecord_dealloc,        /* tp_dealloc */
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
  BGPRecordDocstring,      /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  BGPRecord_methods,             /* tp_methods */
  0,             /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)BGPRecord_init,  /* tp_init */
  0,                         /* tp_alloc */
  BGPRecord_new,             /* tp_new */
};

PyObject *_pybgpstream_bgpstream_get_BGPRecordType()
{
  if (PyType_Ready(&BGPRecordType) < 0)
    return NULL;

  Py_INCREF(&BGPRecordType);
  return (PyObject *)&BGPRecordType;
}
