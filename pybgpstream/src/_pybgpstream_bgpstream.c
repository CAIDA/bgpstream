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
#include "pyutils.h"
#include <Python.h>
#include <bgpstream.h>

typedef struct {
  PyObject_HEAD

    /* BGP Stream Instance Handle */
    bgpstream_t *bs;
} BGPStreamObject;

#define BGPStreamDocstring "BGPStream object"

static void BGPStream_dealloc(BGPStreamObject *self)
{
  if (self->bs != NULL) {
    bgpstream_stop(self->bs);
    bgpstream_destroy(self->bs);
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *BGPStream_new(PyTypeObject *type, PyObject *args,
                               PyObject *kwds)
{
  BGPStreamObject *self;

  self = (BGPStreamObject *)type->tp_alloc(type, 0);
  if (self == NULL) {
    return NULL;
  }

  if ((self->bs = bgpstream_create()) == NULL) {
    Py_DECREF(self);
    return NULL;
  }

  return (PyObject *)self;
}

static int BGPStream_init(BGPStreamObject *self, PyObject *args, PyObject *kwds)
{
  return 0;
}

static PyObject *BGPStream_parse_filter_string(BGPStreamObject *self,
                                               PyObject *args)
{
  const char *fstring;
  if (!PyArg_ParseTuple(args, "s", &fstring)) {
    return NULL;
  }

  if (bgpstream_parse_filter_string(self->bs, fstring) == 0) {
    return PyErr_Format(PyExc_ValueError, "Invalid filter string: %s", fstring);
  }

  Py_RETURN_NONE;
}

/** Add a filter to the bgpstream. */
static PyObject *BGPStream_add_filter(BGPStreamObject *self, PyObject *args)
{
  /* args: FILTER_TYPE (string), FILTER_VALUE (string) */
  static char *filtertype_strs[] = {
    "project",   "collector",    "record-type", "peer-asn",    "prefix",
    "community", "prefix-exact", "prefix-more", "prefix-less", "prefix-any",
    "aspath",    "ipversion",    "elemtype",    NULL};

  static int filtertype_vals[] = {
    BGPSTREAM_FILTER_TYPE_PROJECT,
    BGPSTREAM_FILTER_TYPE_COLLECTOR,
    BGPSTREAM_FILTER_TYPE_RECORD_TYPE,
    BGPSTREAM_FILTER_TYPE_ELEM_PEER_ASN,
    BGPSTREAM_FILTER_TYPE_ELEM_PREFIX,
    BGPSTREAM_FILTER_TYPE_ELEM_COMMUNITY,
    BGPSTREAM_FILTER_TYPE_ELEM_PREFIX_EXACT,
    BGPSTREAM_FILTER_TYPE_ELEM_PREFIX_MORE,
    BGPSTREAM_FILTER_TYPE_ELEM_PREFIX_LESS,
    BGPSTREAM_FILTER_TYPE_ELEM_PREFIX_ANY,
    BGPSTREAM_FILTER_TYPE_ELEM_ASPATH,
    BGPSTREAM_FILTER_TYPE_ELEM_IP_VERSION,
    BGPSTREAM_FILTER_TYPE_ELEM_TYPE,
    -1,
  };

  const char *filter_type;
  const char *value;
  if (!PyArg_ParseTuple(args, "ss", &filter_type, &value)) {
    return NULL;
  }

  int i;
  int filter_val = -1;
  for (i = 0; filtertype_strs[i] != NULL; i++) {
    if (strcmp(filtertype_strs[i], filter_type) == 0) {
      filter_val = filtertype_vals[i];
      break;
    }
  }
  if (filter_val == -1) {
    return PyErr_Format(PyExc_ValueError, "Invalid filter type: %s",
                        filter_type);
  }

  bgpstream_add_filter(self->bs, filter_val, value);

  Py_RETURN_NONE;
}

/** Add a rib period filter to the bgpstream. */
static PyObject *BGPStream_add_rib_period_filter(BGPStreamObject *self,
                                                 PyObject *args)
{
  /* args: period (int) */

  uint32_t filter_period;
  if (!PyArg_ParseTuple(args, "I", &filter_period)) {
    return NULL;
  }

  bgpstream_add_rib_period_filter(self->bs, filter_period);

  Py_RETURN_NONE;
}

/** Add a time filter to the bgpstream. */
static PyObject *BGPStream_add_interval_filter(BGPStreamObject *self,
                                               PyObject *args)
{
  /* args: from (int), until (int) */

  uint32_t filter_start, filter_stop;
  if (!PyArg_ParseTuple(args, "II", &filter_start, &filter_stop)) {
    return NULL;
  }

  bgpstream_add_interval_filter(self->bs, filter_start, filter_stop);

  Py_RETURN_NONE;
}

static PyObject *BGPStream_add_recent_interval(BGPStreamObject *self,
                                               PyObject *args)
{
  const char *intstring;
  int islive;

  if (!PyArg_ParseTuple(args, "si", &intstring, &islive)) {
    return NULL;
  }

  bgpstream_add_recent_interval_filter(self->bs, intstring, islive);
  Py_RETURN_NONE;
}

/** Get information about available data interfaces */
static PyObject *BGPStream_get_data_interfaces(BGPStreamObject *self)
{
  PyObject *list;
  PyObject *dict;

  int id_cnt = 0;
  bgpstream_data_interface_id_t *ids = NULL;
  int i;
  bgpstream_data_interface_info_t *info = NULL;

  id_cnt = bgpstream_get_data_interfaces(self->bs, &ids);

  /* create the list */
  if ((list = PyList_New(0)) == NULL)
    return NULL;

  for (i = 0; i < id_cnt; i++) {
    /* create the dictionary */
    if ((dict = PyDict_New()) == NULL)
      return NULL;

    info = bgpstream_get_data_interface_info(self->bs, ids[i]);
    if (info == NULL) {
      /* this data interface is not available */
      continue;
    }

    /* add info to dict */

    if (add_to_dict(dict, "id", PYNUM_FROMLONG(ids[i])) ||
        add_to_dict(dict, "name", PYSTR_FROMSTR(info->name)) ||
        add_to_dict(dict, "description", PYSTR_FROMSTR(info->description))) {
      return NULL;
    }

    /* add dict to list */
    if (PyList_Append(list, dict) == -1)
      return NULL;
    Py_DECREF(dict);
  }

  return list;
}

/** Set the data interface */
static PyObject *BGPStream_set_data_interface(BGPStreamObject *self,
                                              PyObject *args)
{
  const char *name;
  if (!PyArg_ParseTuple(args, "s", &name)) {
    return NULL;
  }

  int id;
  if ((id = bgpstream_get_data_interface_id_by_name(self->bs, name)) == 0) {
    return PyErr_Format(PyExc_ValueError, "Invalid data interface: %s", name);
  }
  bgpstream_set_data_interface(self->bs, id);

  Py_RETURN_NONE;
}

/** Get the list of interface options that are available for the given
    interface */
static PyObject *BGPStream_get_data_interface_options(BGPStreamObject *self,
                                                      PyObject *args)
{
  const char *name;
  if (!PyArg_ParseTuple(args, "s", &name)) {
    return NULL;
  }

  int id;
  if ((id = bgpstream_get_data_interface_id_by_name(self->bs, name)) == 0) {
    return PyErr_Format(PyExc_ValueError, "Invalid data interface: %s", name);
  }

  int opt_cnt = 0;
  bgpstream_data_interface_option_t *options;
  opt_cnt = bgpstream_get_data_interface_options(self->bs, id, &options);

  /* create the list */
  PyObject *list;
  if ((list = PyList_New(0)) == NULL)
    return NULL;

  int i;
  PyObject *dict;
  for (i = 0; i < opt_cnt; i++) {
    /* create the dictionary */
    if ((dict = PyDict_New()) == NULL)
      return NULL;

    if (add_to_dict(dict, "name", PYSTR_FROMSTR(options[i].name)) ||
        add_to_dict(dict, "description",
                    PYSTR_FROMSTR(options[i].description))) {
      return NULL;
    }

    /* add dict to list */
    if (PyList_Append(list, dict) == -1)
      return NULL;
    Py_DECREF(dict);
  }

  return list;
}

/** Set a data interface option (takes interface, opt-name, opt-val) */
static PyObject *BGPStream_set_data_interface_option(BGPStreamObject *self,
                                                     PyObject *args)
{
  const char *interface_name;
  const char *opt_name;
  const char *opt_value;

  if (!PyArg_ParseTuple(args, "sss", &interface_name, &opt_name, &opt_value)) {
    return NULL;
  }

  int id;
  if ((id = bgpstream_get_data_interface_id_by_name(self->bs,
                                                    interface_name)) == 0) {
    return PyErr_Format(PyExc_ValueError, "Invalid data interface: %s",
                        interface_name);
  }

  bgpstream_data_interface_option_t *opt;
  if ((opt = bgpstream_get_data_interface_option_by_name(self->bs, id,
                                                         opt_name)) == NULL) {
    return PyErr_Format(PyExc_ValueError, "Invalid data interface option: %s",
                        opt_name);
  }

  bgpstream_set_data_interface_option(self->bs, opt, opt_value);

  Py_RETURN_NONE;
}

/** Enable blocking mode */
static PyObject *BGPStream_set_live_mode(BGPStreamObject *self)
{
  bgpstream_set_live_mode(self->bs);
  Py_RETURN_NONE;
}

/** Start the bgpstream.
 *
 * Corresponds to bgpstream_init (so as not to be confused with Python's
 * __init__ method)
 */
static PyObject *BGPStream_start(BGPStreamObject *self)
{
  if (bgpstream_start(self->bs) < 0) {
    PyErr_SetString(PyExc_RuntimeError, "Could not start stream");
    return NULL;
  }
  Py_RETURN_NONE;
}

/** Corresponds to bgpstream_get_next_record */
static PyObject *BGPStream_get_next_record(BGPStreamObject *self,
                                           PyObject *args)
{
  BGPRecordObject *pyrec = NULL;
  int ret;

  /* get the BGPRecord argument */
  if (!PyArg_ParseTuple(args, "O!", _pybgpstream_bgpstream_get_BGPRecordType(),
                        &pyrec)) {
    return NULL;
  }

  if (!pyrec->rec) {
    PyErr_SetString(PyExc_RuntimeError, "Invalid BGPRecord object");
    return NULL;
  }

  ret = bgpstream_get_next_record(self->bs, pyrec->rec);

  if (ret < 0) {
    PyErr_SetString(PyExc_RuntimeError,
                    "Could not get next record (is the stream started?)");
    return NULL;
  } else if (ret == 0) {
    /* end of stream */
    Py_RETURN_FALSE;
  }

  Py_RETURN_TRUE;
}

static PyMethodDef BGPStream_methods[] = {
  {"parse_filter_string", (PyCFunction)BGPStream_parse_filter_string,
   METH_VARARGS, "Parse a string to add filters to an un-started stream."},

  {"add_filter", (PyCFunction)BGPStream_add_filter, METH_VARARGS,
   "Add a filter to an un-started stream."},

  {"add_rib_period_filter", (PyCFunction)BGPStream_add_rib_period_filter,
   METH_VARARGS, "Set the RIB period filter for the current stream."},

  {"add_interval_filter", (PyCFunction)BGPStream_add_interval_filter,
   METH_VARARGS, "Add an interval filter to an un-started stream."},
  {"add_recent_interval_filter", (PyCFunction)BGPStream_add_recent_interval,
   METH_VARARGS, "Add a recentinterval filter to an un-started stream."},
  {
    "get_data_interfaces", (PyCFunction)BGPStream_get_data_interfaces,
    METH_NOARGS, "Get a list of data interfaces available",
  },
  {"set_data_interface", (PyCFunction)BGPStream_set_data_interface,
   METH_VARARGS, "Set the data interface used to discover dump files"},
  {"get_data_interface_options",
   (PyCFunction)BGPStream_get_data_interface_options, METH_VARARGS,
   "Get a list of options available for the given data interface"},
  {"set_data_interface_option",
   (PyCFunction)BGPStream_set_data_interface_option, METH_VARARGS,
   "Set a data interface option"},
  {"set_live_mode", (PyCFunction)BGPStream_set_live_mode, METH_NOARGS,
   "Enable live mode"},

  {"start", (PyCFunction)BGPStream_start, METH_NOARGS, "Start the BGPStream."},

  {"get_next_record", (PyCFunction)BGPStream_get_next_record, METH_VARARGS,
   "Get the next BGPStreamRecord from the stream, or None if end-of-stream "
   "has been reached"},

  {NULL} /* Sentinel */
};

static PyTypeObject BGPStreamType = {
  PyVarObject_HEAD_INIT(NULL, 0) "_pybgpstream.BGPStream", /* tp_name */
  sizeof(BGPStreamObject),                                 /* tp_basicsize */
  0,                                                       /* tp_itemsize */
  (destructor)BGPStream_dealloc,                           /* tp_dealloc */
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
  BGPStreamDocstring,                                      /* tp_doc */
  0,                                                       /* tp_traverse */
  0,                                                       /* tp_clear */
  0,                                                       /* tp_richcompare */
  0,                        /* tp_weaklistoffset */
  0,                        /* tp_iter */
  0,                        /* tp_iternext */
  BGPStream_methods,        /* tp_methods */
  0,                        /* tp_members */
  0,                        /* tp_getset */
  0,                        /* tp_base */
  0,                        /* tp_dict */
  0,                        /* tp_descr_get */
  0,                        /* tp_descr_set */
  0,                        /* tp_dictoffset */
  (initproc)BGPStream_init, /* tp_init */
  0,                        /* tp_alloc */
  BGPStream_new,            /* tp_new */
};

PyTypeObject *_pybgpstream_bgpstream_get_BGPStreamType()
{
  return &BGPStreamType;
}
