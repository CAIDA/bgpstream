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

#ifndef _BGPSTREAM_INPUT_H
#define _BGPSTREAM_INPUT_H

#include "bgpstream_constants.h"
#include <stdbool.h>

typedef struct struct_bgpstream_input_t {
  struct struct_bgpstream_input_t *next;
  char *filename;      // name of bgpdump
  char *fileproject;   // bgpdump project
  char *filecollector; // bgpdump collector
  char *filetype;      // type of bgpdump (rib or update)
  int epoch_filetime;  // timestamp associated to the time the bgp data was
                       // generated
  int time_span;
} bgpstream_input_t;

typedef enum {
  BGPSTREAM_INPUT_MGR_STATUS_EMPTY_INPUT_QUEUE,
  BGPSTREAM_INPUT_MGR_STATUS_NON_EMPTY_INPUT_QUEUE
} bgpstream_input_mgr_status_t;

typedef struct struct_bgpstream_input_mgr_t {
  bgpstream_input_t *head;
  bgpstream_input_t *tail;
  bgpstream_input_t *last_to_process;
  bgpstream_input_mgr_status_t status;
  int epoch_minimum_date;
  int epoch_last_ts_input;
} bgpstream_input_mgr_t;

/* prototypes */
bgpstream_input_mgr_t *bgpstream_input_mgr_create();
bool bgpstream_input_mgr_is_empty(
  const bgpstream_input_mgr_t *const bs_input_mgr);
int bgpstream_input_mgr_push_sorted_input(
  bgpstream_input_mgr_t *const bs_input_mgr, char *filename, char *fileproject,
  char *filecollector, char *const filetype, const int epoch_filetime,
  const int time_span);
bgpstream_input_t *bgpstream_input_mgr_get_queue_to_process(
  bgpstream_input_mgr_t *const bs_input_mgr);
void bgpstream_input_mgr_destroy_queue(bgpstream_input_t *queue);
void bgpstream_input_mgr_destroy(bgpstream_input_mgr_t *bs_input_mgr);

#endif /* _BGPSTREAM_INPUT_H */
