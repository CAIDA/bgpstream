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

static void *get_in_addr(bl_addr_storage_t *ip) {
  assert(ip->version != BL_ADDR_TYPE_UNKNOWN);
  return ip->version == BL_ADDR_IPV4
    ? (void *) &(ip->ipv4)
    : (void *) &(ip->ipv6);
}

static PyObject *
get_ip_str(bl_addr_storage_t *ip) {
  char ip_str[INET6_ADDRSTRLEN] = "";

  inet_ntop(ip->version,
	    get_in_addr(ip),
	    ip_str, INET6_ADDRSTRLEN);

  return Py_BuildValue("s", ip_str);
}

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

/* type */
static PyObject *
BGPElem_get_type(BGPElemObject *self, void *closure)
{
  switch(self->elem->type)
    {
    case BL_RIB_ELEM:
      return Py_BuildValue("s", "rib");
      break;

    case BL_ANNOUNCEMENT_ELEM:
      return Py_BuildValue("s", "announcement");
      break;

    case BL_WITHDRAWAL_ELEM:
      return Py_BuildValue("s", "withdrawal");
      break;

    case BL_PEERSTATE_ELEM:
      return Py_BuildValue("s", "peerstate");
      break;

    case BL_UNKNOWN_ELEM:
    default:
      return Py_BuildValue("s", "unknown");
    }

  return NULL;
}

/* timestamp */
static PyObject *
BGPElem_get_time(BGPElemObject *self, void *closure)
{
  return Py_BuildValue("k", self->elem->timestamp);
}

/* peer address */
static PyObject *
BGPElem_get_peer_address(BGPElemObject *self, void *closure)
{
  return get_ip_str(&self->elem->peer_address);
}

/* peer as number */
static PyObject *
BGPElem_get_peer_asn(BGPElemObject *self, void *closure)
{
  return Py_BuildValue("k", self->elem->peer_asnumber);
}

/** TYPE DEPENDENT FIELDS (TODO) */

/* prefix */
/* next hop */
/* as path */
/* old state */
/* new state */


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
