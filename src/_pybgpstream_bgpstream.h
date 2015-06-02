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

#include <Python.h>

#ifndef ___PYBGPSTREAM_BGPSTREAM_H
#define ___PYBGPSTREAM_BGPSTREAM_H

/** Expose the BGPStreamType structure */
PyTypeObject *_pybgpstream_bgpstream_get_BGPStreamType();

#endif /* ___PYBGPSTREAM_BGPSTREAM_H */
