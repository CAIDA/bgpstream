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

#ifndef ___PYBGPSTREAM_BGPRECORD_H
#define ___PYBGPSTREAM_BGPRECORD_H

#include "bgpstream.h"
#include <Python.h>

typedef struct {
  PyObject_HEAD

    /* BGP Stream Record instance Handle */
    bgpstream_record_t *rec;

} BGPRecordObject;

/** Expose the BGPRecordType structure */
PyTypeObject *_pybgpstream_bgpstream_get_BGPRecordType(void);

#endif /* ___PYBGPSTREAM_BGPRECORD_H */
