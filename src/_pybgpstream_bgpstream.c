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

#include <bgpstream.h>

#include "_pybgpstream_bgprecord.h"

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
      bgpstream_stop(self->bs);
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

  return (PyObject *)self;
}

static int
BGPStream_init(BGPStreamObject *self,
	       PyObject *args, PyObject *kwds)
{
  return 0;
}

/** Add a filter to the bgpstream. */
static PyObject *
BGPStream_add_filter(BGPStreamObject *self, PyObject *args)
{
  /* args: FILTER_TYPE (string), FILTER_VALUE (string) */
  static char *filtertype_strs[] = {
    "project",
    "collector",
    "record-type",
    NULL
  };
  static int filtertype_vals[] = {
    BGPSTREAM_FILTER_TYPE_PROJECT,
    BGPSTREAM_FILTER_TYPE_COLLECTOR,
    BGPSTREAM_FILTER_TYPE_RECORD_TYPE,
    -1,
  };

  const char *filter_type;
  const char *value;
  if (!PyArg_ParseTuple(args, "ss", &filter_type, &value)) {
    return NULL;
  }

  int i;
  int filter_val = -1;
  for (i=0; filtertype_strs[i] != NULL; i++) {
    if (strcmp(filtertype_strs[i], filter_type) == 0) {
      filter_val = filtertype_vals[i];
      break;
    }
  }
  if (filter_val == -1) {
    return PyErr_Format(PyExc_ValueError,
			"Invalid filter type: %s", filter_type);
  }

  bgpstream_add_filter(self->bs, filter_val, value);

  Py_RETURN_NONE;
}

/** Add a time filter to the bgpstream. */
static PyObject *
BGPStream_add_interval_filter(BGPStreamObject *self, PyObject *args)
{
  /* args: from (int), until (int) */

  uint32_t filter_start, filter_stop;
  if (!PyArg_ParseTuple(args, "II", &filter_start, &filter_stop)) {
    return NULL;
  }

  bgpstream_add_interval_filter(self->bs, filter_start, filter_stop);

  Py_RETURN_NONE;
}

/** Get information about available data interfaces */
static PyObject *
BGPStream_get_data_interfaces(BGPStreamObject *self)
{
  PyObject *list;
  PyObject *dict;

  int id_cnt = 0;
  bgpstream_data_interface_id_t *ids = NULL;
  int i;
  bgpstream_data_interface_info_t *info = NULL;

  /* create the list */
  if((list = PyList_New(id_cnt)) == NULL)
    return NULL;

  id_cnt = bgpstream_get_data_interfaces(self->bs, &ids);
  for(i=0; i<id_cnt; i++) {

    /* create the dictionary */
    if((dict = PyDict_New()) == NULL)
      return NULL;

    info = bgpstream_get_data_interface_info(self->bs, ids[i]);

    /* add info to dict */

    /* id */
    if(PyDict_SetItem(dict, PyString_FromString("id"),
                      PyInt_FromLong(ids[i])) == -1)
      return NULL;

    /* name */
    if(PyDict_SetItem(dict, PyString_FromString("name"),
                      PyString_FromString(info->name)) == -1)
      return NULL;

    /* description */
    if(PyDict_SetItem(dict, PyString_FromString("description"),
                      PyString_FromString(info->description)) == -1)
      return NULL;

    /* add dict to list */
    if(PyList_Append(list, dict) == -1)
      return NULL;
  }

  return list;
}

/** Set the data interface */
static PyObject *
BGPStream_set_data_interface(BGPStreamObject *self, PyObject *args)
{
  int id;
  char *name;
  if (!PyArg_ParseTuple(args, "s", &name)) {
    return NULL;
  }

  if((id = bgpstream_get_data_interface_id_by_name(self->bs, name)) == 0) {
    return PyErr_Format(PyExc_ValueError,
			"Invalid data interface: %s", name);
  }

  bgpstream_set_data_interface(self->bs, id);

  Py_RETURN_NONE;
}

#if 0
/** Set a data interface option */
static PyObject *
BGPStream_set_data_interface_option(BGPStreamObject *self, PyObject *args)
{
  /* args: option_type (string), option_value (string) */
  static char *optiontype_strs[] = {
    "mysql-db",
    "mysql-user",
    "mysql-host",
    "csvfile-file",
    NULL
  };
  static int optiontype_vals[] = {
    BS_MYSQL_DB,
    BS_MYSQL_USER,
    BS_MYSQL_HOST,
    BS_CSVFILE_FILE,
    -1,
  };

  char *option_type;
  char *value;
  if (!PyArg_ParseTuple(args, "ss", &option_type, &value)) {
    return NULL;
  }

  int i;
  int option_val = -1;
  for (i=0; optiontype_strs[i] != NULL; i++) {
    if (strcmp(optiontype_strs[i], option_type) == 0) {
      option_val = optiontype_vals[i];
      break;
    }
  }
  if (option_val == -1) {
    return PyErr_Format(PyExc_ValueError,
			"Invalid data interface option type: %s", option_type);
  }

  bgpstream_set_data_interface_options(self->bs, option_val, value);

  Py_RETURN_NONE;
}
#endif

/** Enable blocking mode */
static PyObject *
BGPStream_set_blocking(BGPStreamObject *self)
{
  bgpstream_set_blocking(self->bs);
  Py_RETURN_NONE;
}

/** Start the bgpstream.
 *
 * Corresponds to bgpstream_init (so as not to be confused with Python's
 * __init__ method)
 */
static PyObject *
BGPStream_start(BGPStreamObject *self)
{
  if (bgpstream_start(self->bs) < 0) {
    PyErr_SetString(PyExc_RuntimeError, "Could not start stream");
    return NULL;
  }
  Py_RETURN_NONE;
}


/** Corresponds to bgpstream_get_next_record */
static PyObject *
BGPStream_get_next_record(BGPStreamObject *self, PyObject *args)
{
  BGPRecordObject *pyrec = NULL;
  int ret;

  /* get the BGPRecord argument */
  if (!PyArg_ParseTuple(args, "O!",
                        _pybgpstream_bgpstream_get_BGPRecordType(),
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
  {
    "add_filter",
    (PyCFunction)BGPStream_add_filter,
    METH_VARARGS,
    "Add a filter to an un-started stream."
  },

  {
    "add_interval_filter",
    (PyCFunction)BGPStream_add_interval_filter,
    METH_VARARGS,
   "Add an interval filter to an un-started stream."
  },
  {
    "get_data_interfaces",
    (PyCFunction)BGPStream_get_data_interfaces,
    METH_NOARGS,
    "Get a list of data interfaces available",
  },
  {
    "set_data_interface",
    (PyCFunction)BGPStream_set_data_interface,
    METH_VARARGS,
    "Set the data interface used to discover dump files"
  },
#if 0
  {
    "set_data_interface_option",
    (PyCFunction)BGPStream_set_data_interface_option,
    METH_VARARGS,
    "Set a data interface option"
  },
#endif
  {
    "set_blocking",
    (PyCFunction)BGPStream_set_blocking,
    METH_NOARGS,
    "Enable blocking mode"
  },

  {
    "start",
   (PyCFunction)BGPStream_start,
   METH_NOARGS,
   "Start the BGPStream."
  },

  {
    "get_next_record",
    (PyCFunction)BGPStream_get_next_record,
    METH_VARARGS,
    "Get the next BGPStreamRecord from the stream, or None if end-of-stream "
    "has been reached"
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

PyTypeObject *_pybgpstream_bgpstream_get_BGPStreamType()
{
  return &BGPStreamType;
}
