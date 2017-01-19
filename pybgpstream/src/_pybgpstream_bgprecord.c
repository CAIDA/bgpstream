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

#include "_pybgpstream_bgprecord.h"
#include "_pybgpstream_bgpelem.h"
#include "pyutils.h"
#include <Python.h>
#include <bgpstream.h>

#define BGPRecordDocstring "BGPRecord object"

static void BGPRecord_dealloc(BGPRecordObject *self)
{
  if (self->rec != NULL) {
    bgpstream_record_destroy(self->rec);
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *BGPRecord_new(PyTypeObject *type, PyObject *args,
                               PyObject *kwds)
{
  BGPRecordObject *self;

  self = (BGPRecordObject *)type->tp_alloc(type, 0);
  if (self == NULL) {
    return NULL;
  }

  if ((self->rec = bgpstream_record_create()) == NULL) {
    Py_DECREF(self);
    return NULL;
  }

  return (PyObject *)self;
}

static int BGPRecord_init(BGPRecordObject *self, PyObject *args, PyObject *kwds)
{
  return 0;
}

/* attributes */

/* project */
static PyObject *BGPRecord_get_project(BGPRecordObject *self, void *closure)
{
  return Py_BuildValue("s", self->rec->attributes.dump_project);
}

/* collector */
static PyObject *BGPRecord_get_collector(BGPRecordObject *self, void *closure)
{
  return Py_BuildValue("s", self->rec->attributes.dump_collector);
}

/* type */
static PyObject *BGPRecord_get_type(BGPRecordObject *self, void *closure)
{
  switch (self->rec->attributes.dump_type) {
  case BGPSTREAM_UPDATE:
    return Py_BuildValue("s", "update");
    break;

  case BGPSTREAM_RIB:
    return Py_BuildValue("s", "rib");
    break;

  default:
    return Py_BuildValue("s", "unknown");
  }

  return NULL;
}

/* dump_time */
static PyObject *BGPRecord_get_dump_time(BGPRecordObject *self, void *closure)
{
  return Py_BuildValue("l", self->rec->attributes.dump_time);
}

/* record_time */
static PyObject *BGPRecord_get_record_time(BGPRecordObject *self, void *closure)
{
  return Py_BuildValue("l", self->rec->attributes.record_time);
}

/* get status */
static PyObject *BGPRecord_get_status(BGPRecordObject *self, void *closure)
{
  switch (self->rec->status) {
  case BGPSTREAM_RECORD_STATUS_VALID_RECORD:
    return Py_BuildValue("s", "valid");
    break;

  case BGPSTREAM_RECORD_STATUS_FILTERED_SOURCE:
    return Py_BuildValue("s", "filtered-source");
    break;

  case BGPSTREAM_RECORD_STATUS_EMPTY_SOURCE:
    return Py_BuildValue("s", "empty-source");
    break;

  case BGPSTREAM_RECORD_STATUS_CORRUPTED_SOURCE:
    return Py_BuildValue("s", "corrupted-source");
    break;

  case BGPSTREAM_RECORD_STATUS_CORRUPTED_RECORD:
    return Py_BuildValue("s", "corrupted-record");
    break;

  default:
    return Py_BuildValue("s", "unknown");
  }

  return NULL;
}

/* get dump position */
static PyObject *BGPRecord_get_dump_position(BGPRecordObject *self,
                                             void *closure)
{
  switch (self->rec->dump_pos) {
  case BGPSTREAM_DUMP_START:
    return Py_BuildValue("s", "start");
    break;

  case BGPSTREAM_DUMP_MIDDLE:
    return Py_BuildValue("s", "middle");
    break;

  case BGPSTREAM_DUMP_END:
    return Py_BuildValue("s", "end");
    break;

  default:
    return Py_BuildValue("s", "unknown");
  }

  return NULL;
}

/* get next elem */
static PyObject *BGPRecord_get_next_elem(BGPRecordObject *self)
{
  bgpstream_elem_t *elem;

  PyObject *pyelem;

  if ((elem = bgpstream_record_get_next_elem(self->rec)) == NULL)
    Py_RETURN_NONE;

  if ((pyelem = BGPElem_new(elem)) == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "Could not create BGPElem object");
    return NULL;
  }

  return pyelem;
}

static PyMethodDef BGPRecord_methods[] = {

  {"get_next_elem", (PyCFunction)BGPRecord_get_next_elem, METH_NOARGS,
   "Get next BGP Elem from the Record"},

  {NULL} /* Sentinel */
};

static PyGetSetDef BGPRecord_getsetters[] = {

  /* attributes.dump_project */
  {"project", (getter)BGPRecord_get_project, NULL, "Dump Project", NULL},

  /* attributes.dump_collector */
  {"collector", (getter)BGPRecord_get_collector, NULL, "Dump Collector", NULL},

  /* attributes.dump_type */
  {"type", (getter)BGPRecord_get_type, NULL, "Dump Type", NULL},

  /* attributes.dump_time */
  {"dump_time", (getter)BGPRecord_get_dump_time, NULL, "Dump Time", NULL},

  /* attributes.record_time */
  {"time", (getter)BGPRecord_get_record_time, NULL, "Record Time", NULL},

  /* status */
  {"status", (getter)BGPRecord_get_status, NULL, "Status", NULL},

  /* attributes.dump_position */
  {"dump_position", (getter)BGPRecord_get_dump_position, NULL, "Dump Position",
   NULL},

  {NULL} /* Sentinel */
};

static PyTypeObject BGPRecordType = {
  PyVarObject_HEAD_INIT(NULL, 0) "_pybgpstream.BGPRecord", /* tp_name */
  sizeof(BGPRecordObject),                                 /* tp_basicsize */
  0,                                                       /* tp_itemsize */
  (destructor)BGPRecord_dealloc,                           /* tp_dealloc */
  0,                                                       /* tp_print */
  0,                                                       /* tp_getattr */
  0,                                                       /* tp_setattr */
  0,                                                       /* tp_compare */
  0,                                                       /* tp_repr */
  0,                                                       /* tp_as_number */
  0,                                                       /* tp_as_sequence */
  0,                                                       /* tp_as_mapping */
  0,                                                       /* tp_hash */
  0,                                                       /* tp_call */
  0,                                                       /* tp_str */
  0,                                                       /* tp_getattro */
  0,                                                       /* tp_setattro */
  0,                                                       /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                /* tp_flags */
  BGPRecordDocstring,                                      /* tp_doc */
  0,                                                       /* tp_traverse */
  0,                                                       /* tp_clear */
  0,                                                       /* tp_richcompare */
  0,                        /* tp_weaklistoffset */
  0,                        /* tp_iter */
  0,                        /* tp_iternext */
  BGPRecord_methods,        /* tp_methods */
  0,                        /* tp_members */
  BGPRecord_getsetters,     /* tp_getset */
  0,                        /* tp_base */
  0,                        /* tp_dict */
  0,                        /* tp_descr_get */
  0,                        /* tp_descr_set */
  0,                        /* tp_dictoffset */
  (initproc)BGPRecord_init, /* tp_init */
  0,                        /* tp_alloc */
  BGPRecord_new,            /* tp_new */
};

PyTypeObject *_pybgpstream_bgpstream_get_BGPRecordType()
{
  return &BGPRecordType;
}
