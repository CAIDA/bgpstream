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

#include <bgpstream.h>

#include "_pybgpstream_bgpelem.h"

#define BGPElemDocstring "BGPElem object"

#define ADD_STR_TO_DICT(key_str, value_exp)             \
  do {                                                  \
    PyObject *key = PyString_FromString(key_str);       \
    PyObject *value = (value_exp);                      \
    if(PyDict_SetItem(dict, key, value) == -1)          \
      return NULL;                                      \
    Py_DECREF(key);                                     \
    Py_DECREF(value);                                   \
  }                                                     \
  while(0)

static PyObject *
get_ip_pystr(bgpstream_ip_addr_t *ip) {
  char ip_str[INET6_ADDRSTRLEN] = "";
  bgpstream_addr_ntop(ip_str, INET6_ADDRSTRLEN, ip);
  return PyString_FromString(ip_str);
}

static PyObject *
get_pfx_pystr(bgpstream_pfx_t *pfx) {
  char pfx_str[INET6_ADDRSTRLEN+3] = "";
  if(bgpstream_pfx_snprintf(pfx_str, INET6_ADDRSTRLEN+3, pfx) == NULL)
    return NULL;
  return PyString_FromString(pfx_str);
}

static PyObject *
get_aspath_pystr(bgpstream_as_path_t *aspath) {
  // assuming 10 char per ASN, then this will hold >400 hops
  char buf[4096] = "";
  if(bgpstream_as_path_snprintf(buf, 4096, aspath) >= 4096)
    return NULL;
  return PyString_FromString(buf);
}

static PyObject *
get_communities_pylist(bgpstream_community_set_t *communities) {

  PyObject *list;
  bgpstream_community_t *c;
  Py_ssize_t len = bgpstream_community_set_size(communities);
  int i;
  /* create the dictionary */
  if((list = PyList_New(len)) == NULL)
    return NULL;
  
  PyObject *dict;

  for(i = 0; i< len; i++)
    {
      c = bgpstream_community_set_get(communities, i);

      /* create the dictionary */
      if((dict = PyDict_New()) == NULL)
        return NULL;
      /* add pair to dictionary */
      ADD_STR_TO_DICT("asn", Py_BuildValue("k", c->asn));
      ADD_STR_TO_DICT("value", Py_BuildValue("k", c->value));

      /* add dictionary to list*/
      PyList_SetItem(list, (Py_ssize_t)i, dict);
    }
  return list;
}

static PyObject *
get_peerstate_pystr(bgpstream_elem_peerstate_t state) {
  char buf[128] = "";
  if(bgpstream_elem_peerstate_snprintf(buf, 128, state) >= 128)
    return NULL;
  return PyString_FromString(buf);
}

static void
BGPElem_dealloc(BGPElemObject *self)
{
  bgpstream_elem_destroy(self->elem);
  self->ob_type->tp_free((PyObject*)self);
}

static int
BGPElem_init(BGPElemObject *self,
	       PyObject *args, PyObject *kwds)
{
  return 0;
}

/* type */
static PyObject *
BGPElem_get_type(BGPElemObject *self, void *closure)
{
  char buf[128] = "";
  if(bgpstream_elem_type_snprintf(buf, 128, self->elem->type) >= 128)
    return NULL;
  return PyString_FromString(buf);
}

/* timestamp */
static PyObject *
BGPElem_get_time(BGPElemObject *self, void *closure)
{
  return Py_BuildValue("k", self->elem->timestamp);
}

/* peer address */
/** @todo consider using something like netaddr
    (http://pythonhosted.org/netaddr/) */
static PyObject *
BGPElem_get_peer_address(BGPElemObject *self, void *closure)
{
  return get_ip_pystr((bgpstream_ip_addr_t *)&self->elem->peer_address);
}

/* peer as number */
static PyObject *
BGPElem_get_peer_asn(BGPElemObject *self, void *closure)
{
  return Py_BuildValue("k", self->elem->peer_asnumber);
}


/** Type-dependent field dict */
static PyObject *
BGPElem_get_fields(BGPElemObject *self, void *closure)
{
  PyObject *dict;

  /* create the dictionary */
  if((dict = PyDict_New()) == NULL)
    return NULL;

  switch(self->elem->type)
    {
    case BGPSTREAM_ELEM_TYPE_RIB:
    case BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT:
      /* next hop */
      ADD_STR_TO_DICT("next-hop",
                  get_ip_pystr((bgpstream_ip_addr_t *)&self->elem->nexthop));

      /* as path */
      ADD_STR_TO_DICT("as-path", get_aspath_pystr(self->elem->aspath));

      /* communities */
      ADD_STR_TO_DICT("communities", get_communities_pylist(self->elem->communities));

      /* FALLTHROUGH */

    case BGPSTREAM_ELEM_TYPE_WITHDRAWAL:
      /* prefix */
      ADD_STR_TO_DICT("prefix",
                  get_pfx_pystr((bgpstream_pfx_t *)&self->elem->prefix));
      break;

    case BGPSTREAM_ELEM_TYPE_PEERSTATE:
      /* old state */
      ADD_STR_TO_DICT("old-state", get_peerstate_pystr(self->elem->old_state));

      /* new state */
      ADD_STR_TO_DICT("new-state", get_peerstate_pystr(self->elem->new_state));
      break;

    case BGPSTREAM_ELEM_TYPE_UNKNOWN:
    default:
      break;
    }

  return Py_BuildValue("N", dict);
}

static PyMethodDef BGPElem_methods[] = {
  {NULL}  /* Sentinel */
};

static PyGetSetDef BGPElem_getsetters[] = {

  /* type */
  {
    "type",
    (getter)BGPElem_get_type, NULL,
    "Type",
    NULL
  },

  /* timestamp */
  {
    "time",
    (getter)BGPElem_get_time, NULL,
    "Time",
    NULL
  },

  /* Peer Address */
  {
    "peer_address",
    (getter)BGPElem_get_peer_address, NULL,
    "Peer IP Address",
    NULL
  },

  /* peer ASN */
  {
    "peer_asn",
    (getter)BGPElem_get_peer_asn, NULL,
    "Peer ASN",
    NULL
  },

  /* Type-Specific Fields */
  {
    "fields",
    (getter)BGPElem_get_fields, NULL,
    "Type-Specific Fields",
    NULL
  },

  {NULL} /* Sentinel */
};

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
  BGPElem_getsetters,                 /* tp_getset */
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
PyObject *BGPElem_new(bgpstream_elem_t *elem)
{
  BGPElemObject *self;

  self = (BGPElemObject *)(BGPElemType.tp_alloc(&BGPElemType, 0));
  if(self == NULL) {
    return NULL;
  }

  if ((self->elem = bgpstream_elem_create()) == NULL) {
    return NULL;
  }

  if(bgpstream_elem_copy(self->elem, elem) == NULL) {
    Py_DECREF(self);
    return NULL;
  }

  return (PyObject *)self;
}
