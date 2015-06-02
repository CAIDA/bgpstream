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

#ifndef ___PYBGPSTREAM_BGPELEM_H
#define ___PYBGPSTREAM_BGPELEM_H

#include <Python.h>

#include <bgpstream_utils.h>

typedef struct {
  PyObject_HEAD

  bgpstream_elem_t elem;

} BGPElemObject;

/** Expose the BGPElemType structure */
PyTypeObject *_pybgpstream_bgpstream_get_BGPElemType();

/** Expose our new function as it is not exposed to Python */
PyObject *BGPElem_new(bgpstream_elem_t *elem);

#endif /* ___PYBGPSTREAM_BGPELEM_H */
