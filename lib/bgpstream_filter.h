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

#define BGPSTREAM_FILTER_ELEM_TYPE_RIB 0x1
#define BGPSTREAM_FILTER_ELEM_TYPE_ANNOUNCEMENT 0x2
#define BGPSTREAM_FILTER_ELEM_TYPE_WITHDRAWAL 0x4
#define BGPSTREAM_FILTER_ELEM_TYPE_PEERSTATE 0x8

/* hash table community filter:
 * community -> filter mask (asn only, value only, both) */
KHASH_INIT(bgpstream_community_filter, bgpstream_community_t, uint8_t, 1,
           bgpstream_community_hash_value, bgpstream_community_equal_value);
typedef khash_t(bgpstream_community_filter) bgpstream_community_filter_t;

typedef struct struct_bgpstream_interval_filter_t {
  uint32_t begin_time;
  uint32_t end_time;
  struct struct_bgpstream_interval_filter_t *next;
} bgpstream_interval_filter_t;

KHASH_INIT(collector_ts, char *, uint32_t, 1, kh_str_hash_func,
           kh_str_hash_equal);

typedef khash_t(collector_ts) collector_ts_t;

typedef struct struct_bgpstream_filter_mgr_t {
  bgpstream_str_set_t *projects;
  bgpstream_str_set_t *collectors;
  bgpstream_str_set_t *bgp_types;
  bgpstream_str_set_t *aspath_exprs;
  bgpstream_id_set_t *peer_asns;
  bgpstream_patricia_tree_t *prefixes;
  bgpstream_community_filter_t *communities;
  bgpstream_interval_filter_t *time_intervals;
  collector_ts_t *last_processed_ts;
  uint32_t rib_period;
  uint8_t ipversion;
  uint8_t elemtype_mask;
} bgpstream_filter_mgr_t;

/* allocate memory for a new bgpstream filter */
bgpstream_filter_mgr_t *bgpstream_filter_mgr_create();

/* configure filters in order to select a subset of the bgp data available */
void bgpstream_filter_mgr_filter_add(bgpstream_filter_mgr_t *bs_filter_mgr,
                                     bgpstream_filter_type_t filter_type,
                                     const char *filter_value);

void bgpstream_filter_mgr_rib_period_filter_add(
  bgpstream_filter_mgr_t *bs_filter_mgr, uint32_t period);

void bgpstream_filter_mgr_interval_filter_add(
  bgpstream_filter_mgr_t *bs_filter_mgr, uint32_t begin_time,
  uint32_t end_time);

/* validate the current filters */
int bgpstream_filter_mgr_validate(bgpstream_filter_mgr_t *mgr);

/* destroy the memory allocated for bgpstream filter */
void bgpstream_filter_mgr_destroy(bgpstream_filter_mgr_t *bs_filter_mgr);

#endif /* _BGPSTREAM_FILTER_H */
