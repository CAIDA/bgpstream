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

#include "_pybgpstream_bgpelem.h"

#define BGPElemDocstring "BGPElem object"

static void
BGPElem_dealloc(BGPElemObject *self)
{
  if(self->elem != NULL)
    {
      bgpstream_destroy_elem_queue(self->elem);
    }
  self->ob_type->tp_free((PyObject*)self);
}

static int
BGPElem_init(BGPElemObject *self,
	       PyObject *args, PyObject *kwds)
{
  return 0;
}

#if 0
/* attributes */

/* project */
static PyObject *
BGPElem_get_project(BGPElemObject *self, void *closure)
{
  return Py_BuildValue("s", self->rec->attributes.dump_project);
}

/* collector */
static PyObject *
BGPElem_get_collector(BGPElemObject *self, void *closure)
{
  return Py_BuildValue("s", self->rec->attributes.dump_collector);
}

/* type */
static PyObject *
BGPElem_get_type(BGPElemObject *self, void *closure)
{
  switch(self->rec->attributes.dump_type)
    {
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
static PyObject *
BGPElem_get_dump_time(BGPElemObject *self, void *closure)
{
  return Py_BuildValue("l", self->rec->attributes.dump_time);
}

/* elem_time */
static PyObject *
BGPElem_get_elem_time(BGPElemObject *self, void *closure)
{
  return Py_BuildValue("l", self->rec->attributes.elem_time);
}

/* get status */
static PyObject *
BGPElem_get_status(BGPElemObject *self, void *closure)
{
  switch(self->rec->status)
    {
    case VALID_ELEM:
      return Py_BuildValue("s", "valid");
      break;

    case FILTERED_SOURCE:
      return Py_BuildValue("s", "filtered-source");
      break;

    case EMPTY_SOURCE:
      return Py_BuildValue("s", "empty-source");
      break;

    case CORRUPTED_SOURCE:
      return Py_BuildValue("s", "corrupted-source");
      break;

    case CORRUPTED_ELEM:
      return Py_BuildValue("s", "corrupted-elem");
      break;

    default:
      return Py_BuildValue("s", "unknown");
    }

  return NULL;
}

/* get dump position */
static PyObject *
BGPElem_get_dump_position(BGPElemObject *self, void *closure)
{
  switch(self->rec->dump_pos)
    {
    case DUMP_START:
      return Py_BuildValue("s", "start");
      break;

    case DUMP_MIDDLE:
      return Py_BuildValue("s", "middle");
      break;

    case DUMP_END:
      return Py_BuildValue("s", "end");
      break;

    default:
      return Py_BuildValue("s", "unknown");
    }

  return NULL;
}

/* get elems */
/** @todo! */
#endif

static PyMethodDef BGPElem_methods[] = {
  {NULL}  /* Sentinel */
};

#if 0
static PyGetSetDef BGPElem_getsetters[] = {

  /* attributes.dump_project */
  {
    "project",
    (getter)BGPElem_get_project, NULL,
    "Dump Project",
    NULL
  },

  /* attributes.dump_collector */
  {
    "collector",
    (getter)BGPElem_get_collector, NULL,
    "Dump Collector",
    NULL
  },

  /* attributes.dump_type */
  {
    "type",
    (getter)BGPElem_get_type, NULL,
    "Dump Type",
    NULL
  },

  /* attributes.dump_time */
  {
    "dump_time",
    (getter)BGPElem_get_dump_time, NULL,
    "Dump Time",
    NULL
  },

  /* attributes.elem_time */
  {
    "elem_time",
    (getter)BGPElem_get_elem_time, NULL,
    "Elem Time",
    NULL
  },

  /* status */
  {
    "status",
    (getter)BGPElem_get_status, NULL,
    "Status",
    NULL
  },

  /* attributes.dump_position */
  {
    "dump_position",
    (getter)BGPElem_get_dump_position, NULL,
    "Dump Position",
    NULL
  },

  {NULL} /* Sentinel */
};
#endif

static PyTypeObject BGPElemType = {
  PyObject_HEAD_INIT(NULL)
  0,                                    /* ob_size */
  "_pybgpstream.BGPElem",             /* tp_name */
  sizeof(BGPElemObject), /* tp_basicsize */
  0,                                    /* tp_itemsize */
  (destructor)BGPElem_dealloc,        /* tp_dealloc */
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
  BGPElemDocstring,      /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  BGPElem_methods,             /* tp_methods */
  0,             /* tp_members */
  0,//BGPElem_getsetters,                 /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)BGPElem_init,  /* tp_init */
  0,                         /* tp_alloc */
  0,             /* tp_new */
};

PyTypeObject *_pybgpstream_bgpstream_get_BGPElemType()
{
  return &BGPElemType;
}

/* only available to c code */
PyObject *BGPElem_new(bl_elem_t *elem)
{
  BGPElemObject *self;

  self = (BGPElemObject *)(BGPElemType.tp_alloc(&BGPElemType, 0));
  if(self == NULL) {
    return NULL;
  }

  self->elem = elem;

  return (PyObject *)self;
}
