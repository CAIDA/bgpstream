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

#include "bgpstream_filter.h"
#include "bgpstream_debug.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

/* allocate memory for a new bgpstream filter */
bgpstream_filter_mgr_t *bgpstream_filter_mgr_create() {
  bgpstream_debug("\tBSF_MGR: create start");
  bgpstream_filter_mgr_t *bs_filter_mgr = (bgpstream_filter_mgr_t*) malloc(sizeof(bgpstream_filter_mgr_t));
  if(bs_filter_mgr == NULL) {
    return NULL; // can't allocate memory
  }
  bs_filter_mgr->projects = NULL;
  bs_filter_mgr->collectors = NULL;
  bs_filter_mgr->bgp_types = NULL;
  bs_filter_mgr->time_intervals = NULL;
  bs_filter_mgr->last_processed_ts = NULL;
  bs_filter_mgr->rib_period = 0;
  bgpstream_debug("\tBSF_MGR: create end");
  return bs_filter_mgr;
}

void bgpstream_filter_mgr_filter_add(bgpstream_filter_mgr_t *bs_filter_mgr,
				     bgpstream_filter_type_t filter_type,
				     const char* filter_value) {
  bgpstream_debug("\tBSF_MGR:: add_filter start");
  if(bs_filter_mgr == NULL) {
    return; // nothing to customize
  }
  // create a new filter structure
  bgpstream_string_filter_t *f = (bgpstream_string_filter_t*) malloc(sizeof(bgpstream_string_filter_t));
  if(f == NULL) {
    bgpstream_debug("\tBSF_MGR:: add_filter malloc failed");
    bgpstream_log_warn("\tBSF_MGR: can't allocate memory");       
    return; 
  }
  // copying filter value
  strcpy(f->value, filter_value);
  // add filter to the appropriate list
  switch(filter_type) {    
  case BGPSTREAM_FILTER_TYPE_PROJECT:
    f->next = bs_filter_mgr->projects;
    bs_filter_mgr->projects = f;
    break;
  case BGPSTREAM_FILTER_TYPE_COLLECTOR:
    f->next = bs_filter_mgr->collectors;
    bs_filter_mgr->collectors = f;
    break;
  case BGPSTREAM_FILTER_TYPE_RECORD_TYPE:
    f->next = bs_filter_mgr->bgp_types;
    bs_filter_mgr->bgp_types = f;
    break;
  default:
    free(f);
    bgpstream_log_warn("\tBSF_MGR: unknown filter - ignoring");   
    return;
  }
  bgpstream_debug("\tBSF_MGR:: add_filter stop");  
}


void bgpstream_filter_mgr_rib_period_filter_add(bgpstream_filter_mgr_t *bs_filter_mgr,
                                               uint32_t period)
{
  bgpstream_debug("\tBSF_MGR:: add_filter start");
  assert(bs_filter_mgr != NULL);
  if(period != 0 && bs_filter_mgr->last_processed_ts == NULL)
    {
      if((bs_filter_mgr->last_processed_ts = kh_init(collector_ts)) == NULL)
        {
          bgpstream_log_warn("\tBSF_MGR: can't allocate memory for collectortype map"); 
        }
    }
  bs_filter_mgr->rib_period = period;
  bgpstream_debug("\tBSF_MGR:: add_filter end");

}


void bgpstream_filter_mgr_interval_filter_add(bgpstream_filter_mgr_t *bs_filter_mgr,
					      uint32_t begin_time,
                                              uint32_t end_time){  
  bgpstream_debug("\tBSF_MGR:: add_filter start");
  if(bs_filter_mgr == NULL) {
    return; // nothing to customize
  }
  // create a new filter structure
  bgpstream_interval_filter_t *f = (bgpstream_interval_filter_t*) malloc(sizeof(bgpstream_interval_filter_t));
  if(f == NULL) {
    bgpstream_debug("\tBSF_MGR:: add_filter malloc failed");
    bgpstream_log_warn("\tBSF_MGR: can't allocate memory");       
    return; 
  }
  // copying filter values
  f->begin_time = begin_time;
  f->end_time = end_time;
  f->next = bs_filter_mgr->time_intervals;
  bs_filter_mgr->time_intervals = f;

  bgpstream_debug("\tBSF_MGR:: add_filter stop");  
}

int bgpstream_filter_mgr_validate(bgpstream_filter_mgr_t *filter_mgr) {
  bgpstream_interval_filter_t * tif;
  /* currently we only validate the intervals */
  if(filter_mgr->time_intervals != NULL) {
    tif = filter_mgr->time_intervals;

    while(tif != NULL) {
      if(tif->begin_time > tif->end_time) {
        /* invalid interval */
        fprintf(stderr, "ERROR: Interval %"PRIu32",%"PRIu32" is invalid\n",
                tif->begin_time, tif->end_time);
        return -1;
      }

      tif = tif->next;
    }
  }

  return 0;
}

/* destroy the memory allocated for bgpstream filter */
void bgpstream_filter_mgr_destroy(bgpstream_filter_mgr_t *bs_filter_mgr) {
  bgpstream_debug("\tBSF_MGR:: destroy start");
  if(bs_filter_mgr == NULL) {
    return; // nothing to destroy
  }
  // destroying filters
  bgpstream_string_filter_t * sf;
  bgpstream_interval_filter_t * tif;
  khiter_t k;
  // projects
  sf = NULL;
  while(bs_filter_mgr->projects != NULL) {
    sf =  bs_filter_mgr->projects;
    bs_filter_mgr->projects =  bs_filter_mgr->projects->next;
    free(sf);
  }
  // collectors
  sf = NULL;
  while(bs_filter_mgr->collectors != NULL) {
    sf =  bs_filter_mgr->collectors;
    bs_filter_mgr->collectors =  bs_filter_mgr->collectors->next;
    free(sf);
  }
  // bgp_types
  sf = NULL;
  while(bs_filter_mgr->bgp_types != NULL) {
    sf =  bs_filter_mgr->bgp_types;
    bs_filter_mgr->bgp_types =  bs_filter_mgr->bgp_types->next;
    free(sf);
  }
  // time_intervals
  tif = NULL;
  while(bs_filter_mgr->time_intervals != NULL) {
    tif =  bs_filter_mgr->time_intervals;
    bs_filter_mgr->time_intervals =  bs_filter_mgr->time_intervals->next;
    free(tif);
  }
  // rib/update frequency
  if(bs_filter_mgr->last_processed_ts != NULL)
    {
      for(k=kh_begin(bs_filter_mgr->last_processed_ts); k!=kh_end(bs_filter_mgr->last_processed_ts); ++k)
        {
          if(kh_exist(bs_filter_mgr->last_processed_ts,k))
            {
              free(kh_key(bs_filter_mgr->last_processed_ts,k));
            }
        }
      kh_destroy(collector_ts, bs_filter_mgr->last_processed_ts);
    }
  // free the mgr structure
  free(bs_filter_mgr);
  bs_filter_mgr = NULL;
  bgpstream_debug("\tBSF_MGR:: destroy end");
}


