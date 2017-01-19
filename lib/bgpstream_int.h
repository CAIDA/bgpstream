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

#ifndef _BGPSTREAM_INT_H
#define _BGPSTREAM_INT_H

#include "bgpstream.h"

#include "bgpstream_datasource.h"
#include "bgpstream_filter.h"
#include "bgpstream_input.h"
#include "bgpstream_reader.h"

typedef enum {
  BGPSTREAM_STATUS_ALLOCATED,
  BGPSTREAM_STATUS_ON,
  BGPSTREAM_STATUS_OFF
} bgpstream_status;

struct struct_bgpstream_t {
  bgpstream_input_mgr_t *input_mgr;
  bgpstream_reader_mgr_t *reader_mgr;
  bgpstream_filter_mgr_t *filter_mgr;
  bgpstream_datasource_mgr_t *datasource_mgr;
  bgpstream_status status;
};

#endif /* _BGPSTREAM_INT_H */
