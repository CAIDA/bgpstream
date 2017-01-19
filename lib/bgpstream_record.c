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
#include <assert.h>
#include <inttypes.h>

#include "bgpdump_lib.h"
#include "bgpdump_process.h"

#include "bgpstream_record.h"
#include "bgpstream_elem_generator.h"
#include "bgpstream_elem_int.h"
#include "bgpstream_debug.h"
#include "bgpstream_int.h"

#include "bgpstream.h"
#include "utils/bgpstream_utils_rtr.h"


/* allocate memory for a bs_record */
bgpstream_record_t *bgpstream_record_create() {
  bgpstream_debug("BS: create record start");
  bgpstream_record_t *bs_record;
  if((bs_record =
      (bgpstream_record_t*)malloc(sizeof(bgpstream_record_t))) == NULL) {
    return NULL; // can't allocate memory
  }

  bs_record->bs = NULL;
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
  // record->bs = NULL;

  bgpstream_elem_generator_clear(record->elem_generator);
}

void bgpstream_record_print_mrt_data(bgpstream_record_t * const bs_record) {
  bgpdump_print_entry(bs_record->bd_entry);
}



static int bgpstream_elem_check_filters(bgpstream_filter_mgr_t *filter_mgr, bgpstream_elem_t *elem)
{
  int pass = 0;

  /* Checking peer ASNs: if the filter is on and the peer asn is not in the 
   * set, return 0 */
  if(filter_mgr->peer_asns &&
     bgpstream_id_set_exists(filter_mgr->peer_asns, elem->peer_asnumber) == 0)
    {
      return 0;
    }

  /* Checking prefixes (unless it is a peer state message) */
  if(elem->type == BGPSTREAM_ELEM_TYPE_PEERSTATE)
    {
      return 1;
    }

  if(filter_mgr->prefixes &&
    (bgpstream_patricia_tree_get_pfx_overlap_info(filter_mgr->prefixes,
                                                  (bgpstream_pfx_t *) &elem->prefix) &
     (BGPSTREAM_PATRICIA_EXACT_MATCH | BGPSTREAM_PATRICIA_LESS_SPECIFICS) ) == 0)
    {
      return 0;
    }

  /* Checking communities (unless it is a withdrawal message) */
  if(elem->type == BGPSTREAM_ELEM_TYPE_WITHDRAWAL)
    {
      return 1;
    }

  pass = (filter_mgr->communities != NULL) ? 0 : 1;
  if(filter_mgr->communities)
    {
      bgpstream_community_t *c;
      khiter_t k;
      for(k = kh_begin(filter_mgr->communities); k != kh_end(filter_mgr->communities); ++k)
        {
          if(kh_exist(filter_mgr->communities, k))
            {
              c = &(kh_key(filter_mgr->communities, k));
              if(bgpstream_community_set_match(elem->communities, c,
                                               kh_value(filter_mgr->communities, k)))
                {
                  pass = 1;
                  break;
                }
            }
        }
    }
  return pass;
}

bgpstream_elem_t *bgpstream_record_get_next_elem(bgpstream_record_t *record) {
  if(bgpstream_elem_generator_is_populated(record->elem_generator) == 0 &&
     bgpstream_elem_generator_populate(record->elem_generator, record) != 0)
    {
      return NULL;
    }
  bgpstream_elem_t *elem = bgpstream_elem_generator_get_next_elem(record->elem_generator);

  /* if the elem is compatible with the current filters
   * then return elem, otherwise run again
   * bgpstream_record_get_next_elem(record) */
  if(elem == NULL || bgpstream_elem_check_filters(record->bs->filter_mgr, elem) == 1)
  {
#if defined(FOUND_RTR)
    if (elem != NULL && bgpstream_get_rtr_config() != NULL) {
      elem->annotations.rpki_validation_status =
          BGPSTREAM_ELEM_RPKI_VALIDATION_STATUS_NOTVALIDATED;

      char prefix[INET6_ADDRSTRLEN];
      bgpstream_pfx_t *addr_pfx;

      switch (elem->prefix.address.version) {
      case BGPSTREAM_ADDR_VERSION_IPV4:
        addr_pfx = (bgpstream_pfx_t *)&(elem->prefix);
        bgpstream_addr_ntop(prefix, INET_ADDRSTRLEN, &(addr_pfx->address));
        break;

      case BGPSTREAM_ADDR_VERSION_IPV6:
        addr_pfx = (bgpstream_pfx_t *)&(elem->prefix);
        bgpstream_addr_ntop(prefix, INET6_ADDRSTRLEN, &(addr_pfx->address));
        break;

      default:
        addr_pfx = NULL;
        break;
      }
      if (addr_pfx != NULL) {
        uint32_t origin_asn = 0;
        bgpstream_as_path_seg_t *origin_seg =
            bgpstream_as_path_get_origin_seg(elem->aspath);
        if (origin_seg != NULL &&
            origin_seg->type == BGPSTREAM_AS_PATH_SEG_ASN) {
          origin_asn = ((bgpstream_as_path_seg_asn_t *)origin_seg)->asn;
        }

        bgpstream_elem_get_rpki_validation_result(elem, prefix, origin_asn,
                                                  elem->prefix.mask_len);
      }
    }
#endif
    return elem;
  }

  return bgpstream_record_get_next_elem(record);
}


int bgpstream_record_dump_type_snprintf(char *buf, size_t len,
                                        bgpstream_record_dump_type_t dump_type)
{
  /* ensure we have enough bytes to write our single character */
  if(len == 0)
    {
      return -1;
    }
  else if(len == 1)
    {
      buf[0] = '\0';
      return -1;
    }
  switch(dump_type)
    {
    case BGPSTREAM_RIB:
      buf[0] = 'R';
      break;
    case BGPSTREAM_UPDATE:
      buf[0] = 'U';
      break;
    default:
      buf[0] = '\0';
      break;
    }
  buf[1] = '\0';
  return 1;
}

int bgpstream_record_dump_pos_snprintf(char *buf, size_t len,
                                       bgpstream_dump_position_t dump_pos)
{
  /* ensure we have enough bytes to write our single character */
  if(len == 0)
    {
      return -1;
    }
  else if(len == 1)
    {
      buf[0] = '\0';
      return -1;
    }

  switch(dump_pos)
    {
    case BGPSTREAM_DUMP_START:
      buf[0] = 'B';
      break;
    case BGPSTREAM_DUMP_MIDDLE:
      buf[0] = 'M';
      break;
    case BGPSTREAM_DUMP_END:
      buf[0] = 'E';
      break;
    default:
      buf[0] = '\0';
      break;
    }
  buf[1] = '\0';
  return 1;
}

int bgpstream_record_status_snprintf(char *buf, size_t len,
                                     bgpstream_record_status_t status)
{
  /* ensure we have enough bytes to write our single character */
  if(len == 0)
    {
      return -1;
    }
  else if(len == 1)
    {
      buf[0] = '\0';
      return -1;
    }

  switch(status)
    {
    case BGPSTREAM_RECORD_STATUS_VALID_RECORD:
      buf[0] = 'V';
      break;
    case BGPSTREAM_RECORD_STATUS_FILTERED_SOURCE:
      buf[0] = 'F';
      break;
    case BGPSTREAM_RECORD_STATUS_EMPTY_SOURCE:
      buf[0] = 'E';
      break;
    case BGPSTREAM_RECORD_STATUS_CORRUPTED_SOURCE:
      buf[0] = 'S';
      break;
    case BGPSTREAM_RECORD_STATUS_CORRUPTED_RECORD:
      buf[0] = 'R';
      break;
    default:
      buf[0] = '\0';
      break;
    }
  buf[1] = '\0';
  return 1;
}

#define B_REMAIN (len-written)
#define B_FULL   (written >= len)
#define ADD_PIPE                                \
  do {                                          \
  if(B_REMAIN > 1)                              \
    {                                           \
      *buf_p = '|';                             \
      buf_p++;                                  \
      *buf_p = '\0';                            \
      written++;                                \
    }                                           \
  else                                          \
    {                                           \
      return NULL;                              \
    }                                           \
  } while(0)


char *bgpstream_record_elem_snprintf(char *buf, size_t len,
                                     const bgpstream_record_t * bs_record,
                                     const bgpstream_elem_t * elem)
{
  assert(bs_record);
  assert(elem);

  size_t written = 0; /* < how many bytes we wanted to write */
  ssize_t c = 0; /* < how many chars were written */
  char *buf_p = buf;

  /* Record type */
  if((c = bgpstream_record_dump_type_snprintf(buf_p, B_REMAIN, bs_record->attributes.dump_type)) < 0)
    {
      return NULL;
    }
  written += c;
  buf_p += c;
  ADD_PIPE;

  /* Elem type */
  if((c = bgpstream_elem_type_snprintf(buf_p, B_REMAIN, elem->type)) < 0)
    {
      return NULL;
    }
  written += c;
  buf_p += c;
  ADD_PIPE;

  /* Record timestamp, project, collector */
  c = snprintf(buf_p, B_REMAIN, "%ld|%s|%s",
               bs_record->attributes.record_time,
               bs_record->attributes.dump_project,
               bs_record->attributes.dump_collector);
  written += c;
  buf_p += c;
  ADD_PIPE;

  if(B_FULL)
    return NULL;

  if(bgpstream_elem_custom_snprintf(buf_p, B_REMAIN, elem, 0) == NULL)
    {
      return NULL;
    }

  written += c;
  buf_p += c;

  if(B_FULL)
    return NULL;

  return buf;
}

