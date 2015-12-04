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

#ifndef _BGPSTREAM_FILTER_H
#define _BGPSTREAM_FILTER_H

#include "bgpstream.h"
#include "bgpstream_constants.h"
#include "khash.h"

typedef struct struct_bgpstream_string_filter_t {
  char value[BGPSTREAM_PAR_MAX_LEN];
  struct struct_bgpstream_string_filter_t * next;
} bgpstream_string_filter_t;

typedef struct struct_bgpstream_asn_filter_t {
  uint32_t value;
  struct struct_bgpstream_asn_filter_t * next;
} bgpstream_asn_filter_t;

typedef struct struct_bgpstream_pfx_filter_t {
  bgpstream_pfx_storage_t value;
  struct struct_bgpstream_pfx_filter_t * next;
} bgpstream_pfx_filter_t;

typedef struct struct_bgpstream_interval_filter_t {
  uint32_t begin_time;
  uint32_t end_time;
  struct struct_bgpstream_interval_filter_t * next;
} bgpstream_interval_filter_t;

KHASH_INIT(collector_ts, char*, uint32_t, 1,
	   kh_str_hash_func, kh_str_hash_equal);

typedef khash_t(collector_ts) collector_ts_t;
                                   
typedef struct struct_bgpstream_filter_mgr_t {
  
  bgpstream_string_filter_t * projects;
  bgpstream_string_filter_t * collectors;
  bgpstream_string_filter_t * bgp_types;
  bgpstream_asn_filter_t * peer_asns;
  bgpstream_pfx_filter_t * prefixes;
  bgpstream_interval_filter_t * time_intervals;
  collector_ts_t *last_processed_ts;
  uint32_t rib_period;
} bgpstream_filter_mgr_t;


/* allocate memory for a new bgpstream filter */
bgpstream_filter_mgr_t *bgpstream_filter_mgr_create();

/* configure filters in order to select a subset of the bgp data available */
void bgpstream_filter_mgr_filter_add(bgpstream_filter_mgr_t *bs_filter_mgr,
				     bgpstream_filter_type_t filter_type,
				     const char* filter_value);

void bgpstream_filter_mgr_rib_period_filter_add(bgpstream_filter_mgr_t *bs_filter_mgr,
                                                uint32_t period);

void bgpstream_filter_mgr_interval_filter_add(bgpstream_filter_mgr_t *bs_filter_mgr,
					      uint32_t begin_time,
					      uint32_t end_time);

/* validate the current filters */
int bgpstream_filter_mgr_validate(bgpstream_filter_mgr_t *mgr);


/* destroy the memory allocated for bgpstream filter */
void bgpstream_filter_mgr_destroy(bgpstream_filter_mgr_t *bs_filter_mgr);


#endif /* _BGPSTREAM_FILTER_H */
