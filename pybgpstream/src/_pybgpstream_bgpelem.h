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

#ifndef ___PYBGPSTREAM_BGPELEM_H
#define ___PYBGPSTREAM_BGPELEM_H

#include "bgpstream_elem.h"
#include <Python.h>

typedef struct {
  PyObject_HEAD

    bgpstream_elem_t *elem;

} BGPElemObject;

/** Expose the BGPElemType structure */
PyTypeObject *_pybgpstream_bgpstream_get_BGPElemType(void);

/** Expose our new function as it is not exposed to Python */
PyObject *BGPElem_new(bgpstream_elem_t *elem);

#endif /* ___PYBGPSTREAM_BGPELEM_H */
