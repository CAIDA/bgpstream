/*
 * This file is part of bgpstream
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

/* This file written by Shane Alcock and freely contributed back to the
 * BGPStream project  <salcock@waikato.ac.nz>
 */

#ifndef BGPSTREAM_UTILS_TIME_H_
#define BGPSTREAM_UTILS_TIME_H_

#include <stdint.h>

int bgpstream_time_calc_recent_interval(uint32_t *start, uint32_t *end,
                                        const char *optval);

#endif
