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

#ifndef _BGPSTREAM_DATASOURCE_BROKER_H
#define _BGPSTREAM_DATASOURCE_BROKER_H

#include "bgpstream_constants.h"
#include "bgpstream_filter.h"
#include "bgpstream_input.h"

#include <stdio.h>
#include <stdlib.h>

/** Opaque handle that represents the broker data source */
typedef struct struct_bgpstream_broker_datasource_t
  bgpstream_broker_datasource_t;

bgpstream_broker_datasource_t *
bgpstream_broker_datasource_create(bgpstream_filter_mgr_t *filter_mgr,
                                   char *broker_url, char **params,
                                   int params_cnt);

int bgpstream_broker_datasource_update_input_queue(
  bgpstream_broker_datasource_t *broker_ds, bgpstream_input_mgr_t *input_mgr);

void bgpstream_broker_datasource_destroy(
  bgpstream_broker_datasource_t *broker_ds);

#endif /* _BGPSTREAM_DATASOURCE_BROKER_H */
