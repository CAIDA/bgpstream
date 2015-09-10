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

#include <stdio.h>
#include <stdlib.h>

#include "bgpdump_lib.h"
#include "bgpdump_process.h"

#include "bgpstream_record.h"
#include "bgpstream_elem_generator.h"
#include "bgpstream_debug.h"

/* allocate memory for a bs_record */
bgpstream_record_t *bgpstream_record_create() {
  bgpstream_debug("BS: create record start");
  bgpstream_record_t *bs_record;
  if((bs_record =
      (bgpstream_record_t*)malloc(sizeof(bgpstream_record_t))) == NULL) {
    return NULL; // can't allocate memory
  }

  bs_record->bd_entry = NULL;

  if((bs_record->elem_generator = bgpstream_elem_generator_create()) == NULL) {
    bgpstream_record_destroy(bs_record);
    return NULL;
  }

  bgpstream_record_clear(bs_record);

  bgpstream_debug("BS: create record end");
  return bs_record;
}

/* free memory associated to a bs_record  */
void bgpstream_record_destroy(bgpstream_record_t * const bs_record){
  bgpstream_debug("BS: destroy record start");
  if(bs_record == NULL) {
    bgpstream_debug("BS: record destroy end");
    return; // nothing to do
  }
  if(bs_record->bd_entry != NULL){
    bgpstream_debug("BS - free bs_record->bgpdump_entry");
    bgpdump_free_mem(bs_record->bd_entry);
    bs_record->bd_entry = NULL;
  }

  if(bs_record->elem_generator != NULL) {
    bgpstream_elem_generator_destroy(bs_record->elem_generator);
    bs_record->elem_generator = NULL;
  }

  bgpstream_debug("BS - free bs_record");
  free(bs_record);
  bgpstream_debug("BS: destroy record end");
}

void bgpstream_record_clear(bgpstream_record_t *record) {
  record->status = BGPSTREAM_RECORD_STATUS_EMPTY_SOURCE;
  record->dump_pos = BGPSTREAM_DUMP_START;
  record->attributes.dump_project[0] = '\0';
  record->attributes.dump_collector[0] = '\0';
  record->attributes.dump_type = BGPSTREAM_UPDATE;
  record->attributes.dump_time = 0;
  record->attributes.record_time = 0;

  if(record->bd_entry != NULL) {
    bgpdump_free_mem(record->bd_entry);
    record->bd_entry = NULL;
  }

  bgpstream_elem_generator_clear(record->elem_generator);
}

void bgpstream_record_print_mrt_data(bgpstream_record_t * const bs_record) {
  bgpdump_print_entry(bs_record->bd_entry);
}

bgpstream_elem_t *bgpstream_record_get_next_elem(bgpstream_record_t *record) {
  if(bgpstream_elem_generator_is_populated(record->elem_generator) == 0 &&
     bgpstream_elem_generator_populate(record->elem_generator, record) != 0)
    {
      return NULL;
    }
  return bgpstream_elem_generator_get_next_elem(record->elem_generator);
}
