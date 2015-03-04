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

#ifndef ___PYBGPSTREAM_BGPELEM_H
#define ___PYBGPSTREAM_BGPELEM_H

#include <Python.h>

#include <bgpstream_utils.h>

typedef struct {
  PyObject_HEAD

  bgpstream_elem_t *elem;

} BGPElemObject;

/** Expose the BGPElemType structure */
PyTypeObject *_pybgpstream_bgpstream_get_BGPElemType();

/** Expose our new function as it is not exposed to Python */
PyObject *BGPElem_new(bgpstream_elem_t *elem);

#endif /* ___PYBGPSTREAM_BGPELEM_H */
