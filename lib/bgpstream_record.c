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

#include <assert.h>
#include <inttypes.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

#include "bgpdump_lib.h"
#include "bgpdump_process.h"

#include "bgpstream_elem_int.h"
#include "bgpstream_int.h"
#include "bgpstream_debug.h"
#include "bgpstream_elem_generator.h"
#include "bgpstream_record.h"

/* allocate memory for a bs_record */
bgpstream_record_t *bgpstream_record_create()
{
  bgpstream_debug("BS: create record start");
  bgpstream_record_t *bs_record;
  if ((bs_record = (bgpstream_record_t *)malloc(sizeof(bgpstream_record_t))) ==
      NULL) {
    return NULL; // can't allocate memory
  }

  bs_record->bs = NULL;
  bs_record->bd_entry = NULL;

  if ((bs_record->elem_generator = bgpstream_elem_generator_create()) == NULL) {
    bgpstream_record_destroy(bs_record);
    return NULL;
  }

  bgpstream_record_clear(bs_record);

  bgpstream_debug("BS: create record end");
  return bs_record;
}

/* free memory associated to a bs_record  */
void bgpstream_record_destroy(bgpstream_record_t *const bs_record)
{
  bgpstream_debug("BS: destroy record start");
  if (bs_record == NULL) {
    bgpstream_debug("BS: record destroy end");
    return; // nothing to do
  }
  if (bs_record->bd_entry != NULL) {
    bgpstream_debug("BS - free bs_record->bgpdump_entry");
    bgpdump_free_mem(bs_record->bd_entry);
    bs_record->bd_entry = NULL;
  }

  if (bs_record->elem_generator != NULL) {
    bgpstream_elem_generator_destroy(bs_record->elem_generator);
    bs_record->elem_generator = NULL;
  }

  bgpstream_debug("BS - free bs_record");
  free(bs_record);
  bgpstream_debug("BS: destroy record end");
}

void bgpstream_record_clear(bgpstream_record_t *record)
{
  record->status = BGPSTREAM_RECORD_STATUS_EMPTY_SOURCE;
  record->dump_pos = BGPSTREAM_DUMP_START;
  record->attributes.dump_project[0] = '\0';
  record->attributes.dump_collector[0] = '\0';
  record->attributes.dump_type = BGPSTREAM_UPDATE;
  record->attributes.dump_time = 0;
  record->attributes.record_time = 0;

  if (record->bd_entry != NULL) {
    bgpdump_free_mem(record->bd_entry);
    record->bd_entry = NULL;
  }
  // record->bs = NULL;

  bgpstream_elem_generator_clear(record->elem_generator);
}

void bgpstream_record_print_mrt_data(bgpstream_record_t *const bs_record)
{
  bgpdump_print_entry(bs_record->bd_entry);
}

static int bgpstream_elem_prefix_match(bgpstream_patricia_tree_t *prefixes,
                                       bgpstream_pfx_t *search)
{

  bgpstream_patricia_tree_result_set_t *res = NULL;
  bgpstream_patricia_node_t *it;
  int matched = 0;

  /* If this is an exact match, the allowable matches don't matter */
  if (bgpstream_patricia_tree_search_exact(prefixes, search)) {
    return 1;
  }

  bgpstream_patricia_node_t *n =
    bgpstream_patricia_tree_insert(prefixes, search);

  /* Check for less specific prefixes that have the "MORE" match flag */
  res = bgpstream_patricia_tree_result_set_create();
  bgpstream_patricia_tree_get_less_specifics(prefixes, n, res);

  while ((it = bgpstream_patricia_tree_result_set_next(res)) != NULL) {
    bgpstream_pfx_t *pfx = bgpstream_patricia_tree_get_pfx(it);

    if (pfx->allowed_matches == BGPSTREAM_PREFIX_MATCH_ANY ||
        pfx->allowed_matches == BGPSTREAM_PREFIX_MATCH_MORE) {
      matched = 1;
      goto endmatch;
    }
  }

  bgpstream_patricia_tree_get_more_specifics(prefixes, n, res);

  while ((it = bgpstream_patricia_tree_result_set_next(res)) != NULL) {
    bgpstream_pfx_t *pfx = bgpstream_patricia_tree_get_pfx(it);

    /* TODO maybe have a way of limiting the amount of bits we are allowed to
     * go back? or make it specifiable via the language? */
    if (pfx->allowed_matches == BGPSTREAM_PREFIX_MATCH_ANY ||
        pfx->allowed_matches == BGPSTREAM_PREFIX_MATCH_LESS) {
      matched = 1;
      goto endmatch;
    }
  }

endmatch:
  bgpstream_patricia_tree_result_set_destroy(&res);
  bgpstream_patricia_tree_remove_node(prefixes, n);
  return matched;
}

static int bgpstream_elem_check_filters(bgpstream_filter_mgr_t *filter_mgr,
                                        bgpstream_elem_t *elem)
{
  int pass = 0;

  /* First up, check if this element is the right type */
  if (filter_mgr->elemtype_mask) {

    if (elem->type == BGPSTREAM_ELEM_TYPE_PEERSTATE &&
        !(filter_mgr->elemtype_mask & BGPSTREAM_FILTER_ELEM_TYPE_PEERSTATE)) {
      return 0;
    }

    if (elem->type == BGPSTREAM_ELEM_TYPE_RIB &&
        !(filter_mgr->elemtype_mask & BGPSTREAM_FILTER_ELEM_TYPE_RIB)) {
      return 0;
    }

    if (elem->type == BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT &&
        !(filter_mgr->elemtype_mask &
          BGPSTREAM_FILTER_ELEM_TYPE_ANNOUNCEMENT)) {
      return 0;
    }

    if (elem->type == BGPSTREAM_ELEM_TYPE_WITHDRAWAL &&
        !(filter_mgr->elemtype_mask & BGPSTREAM_FILTER_ELEM_TYPE_WITHDRAWAL)) {
      return 0;
    }
  }

  /* Checking peer ASNs: if the filter is on and the peer asn is not in the
   * set, return 0 */
  if (filter_mgr->peer_asns &&
      bgpstream_id_set_exists(filter_mgr->peer_asns, elem->peer_asnumber) ==
        0) {
    return 0;
  }

  if (filter_mgr->ipversion) {
    /* Determine address version for the element prefix */

    if (elem->type == BGPSTREAM_ELEM_TYPE_PEERSTATE) {
      return 0;
    }

    bgpstream_ip_addr_t *addr = &(((bgpstream_pfx_t *)&elem->prefix)->address);
    if (addr->version != filter_mgr->ipversion)
      return 0;
  }

  if (filter_mgr->prefixes) {
    if (elem->type == BGPSTREAM_ELEM_TYPE_PEERSTATE) {
      return 0;
    }
    return bgpstream_elem_prefix_match(filter_mgr->prefixes,
                                       (bgpstream_pfx_t *)&elem->prefix);
  }

  /* Checking AS Path expressions */
  if (filter_mgr->aspath_exprs) {
    char aspath[65536];
    char *regexstr;
    int pathlen;
    regex_t re;
    int result;
    int negatives = 0;
    int positives = 0;
    int totalpositives = 0;

    if (elem->type == BGPSTREAM_ELEM_TYPE_WITHDRAWAL ||
        elem->type == BGPSTREAM_ELEM_TYPE_PEERSTATE) {
      return 0;
    }

    pathlen = bgpstream_as_path_get_filterable(aspath, 65535, elem->aspath);

    if (pathlen == 65535) {
      bgpstream_log_warn("AS Path is too long? Filter may not work well.");
    }

    if (pathlen == 0) {
      return 0;
    }

    bgpstream_str_set_rewind(filter_mgr->aspath_exprs);
    while ((regexstr = bgpstream_str_set_next(filter_mgr->aspath_exprs)) !=
           NULL) {
      int negate = 0;

      if (strlen(regexstr) == 0)
        continue;

      if (*regexstr == '!') {
        negate = 1;
        regexstr++;
      } else {
        totalpositives += 1;
      }

      if (regcomp(&re, regexstr, 0) < 0) {
        /* XXX should really use regerror here for proper error reporting */
        bgpstream_log_err("Failed to compile AS path regex");
        break;
      }

      result = regexec(&re, aspath, 0, NULL, 0);
      if (result == 0) {
        if (!negate) {
          positives++;
        }
        if (negate) {
          negatives++;
        }
      }

      regfree(&re);
      if (result != REG_NOMATCH && result != 0) {
        bgpstream_log_err("Error while matching AS path regex");
        break;
      }
    }
    if (positives == totalpositives && negatives == 0) {
      return 1;
    } else {
      return 0;
    }
  }
  /* Checking communities (unless it is a withdrawal message) */
  pass = (filter_mgr->communities != NULL) ? 0 : 1;
  if (filter_mgr->communities) {
    if (elem->type == BGPSTREAM_ELEM_TYPE_WITHDRAWAL ||
        elem->type == BGPSTREAM_ELEM_TYPE_PEERSTATE) {
      return 0;
    }

    bgpstream_community_t *c;
    khiter_t k;
    for (k = kh_begin(filter_mgr->communities);
         k != kh_end(filter_mgr->communities); ++k) {
      if (kh_exist(filter_mgr->communities, k)) {
        c = &(kh_key(filter_mgr->communities, k));
        if (bgpstream_community_set_match(
              elem->communities, c, kh_value(filter_mgr->communities, k))) {
          pass = 1;
          break;
        }
      }
    }
    if (pass == 0) {
      return 0;
    }
  }

  return 1;
}

bgpstream_elem_t *bgpstream_record_get_next_elem(bgpstream_record_t *record)
{
  if (bgpstream_elem_generator_is_populated(record->elem_generator) == 0 &&
      bgpstream_elem_generator_populate(record->elem_generator, record) != 0) {
    return NULL;
  }
  bgpstream_elem_t *elem =
    bgpstream_elem_generator_get_next_elem(record->elem_generator);

  /* if the elem is compatible with the current filters
   * then return elem, otherwise run again
   * bgpstream_record_get_next_elem(record) */
  if (elem == NULL ||
      bgpstream_elem_check_filters(record->bs->filter_mgr, elem) == 1) {
    return elem;
  }

  return bgpstream_record_get_next_elem(record);
}

int bgpstream_record_dump_type_snprintf(char *buf, size_t len,
                                        bgpstream_record_dump_type_t dump_type)
{
  /* ensure we have enough bytes to write our single character */
  if (len == 0) {
    return -1;
  } else if (len == 1) {
    buf[0] = '\0';
    return -1;
  }
  switch (dump_type) {
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
  if (len == 0) {
    return -1;
  } else if (len == 1) {
    buf[0] = '\0';
    return -1;
  }

  switch (dump_pos) {
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
  if (len == 0) {
    return -1;
  } else if (len == 1) {
    buf[0] = '\0';
    return -1;
  }

  switch (status) {
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

#define B_REMAIN (len - written)
#define B_FULL (written >= len)
#define ADD_PIPE                                                               \
  do {                                                                         \
    if (B_REMAIN > 1) {                                                        \
      *buf_p = '|';                                                            \
      buf_p++;                                                                 \
      *buf_p = '\0';                                                           \
      written++;                                                               \
    } else {                                                                   \
      return NULL;                                                             \
    }                                                                          \
  } while (0)

char *bgpstream_record_elem_snprintf(char *buf, size_t len,
                                     const bgpstream_record_t *bs_record,
                                     const bgpstream_elem_t *elem)
{
  assert(bs_record);
  assert(elem);

  size_t written = 0; /* < how many bytes we wanted to write */
  ssize_t c = 0;      /* < how many chars were written */
  char *buf_p = buf;

  /* Record type */
  if ((c = bgpstream_record_dump_type_snprintf(
         buf_p, B_REMAIN, bs_record->attributes.dump_type)) < 0) {
    return NULL;
  }
  written += c;
  buf_p += c;
  ADD_PIPE;

  /* Elem type */
  if ((c = bgpstream_elem_type_snprintf(buf_p, B_REMAIN, elem->type)) < 0) {
    return NULL;
  }
  written += c;
  buf_p += c;
  ADD_PIPE;

  /* Record timestamp, project, collector */
  c = snprintf(buf_p, B_REMAIN, "%ld|%s|%s", bs_record->attributes.record_time,
               bs_record->attributes.dump_project,
               bs_record->attributes.dump_collector);
  written += c;
  buf_p += c;
  ADD_PIPE;

  if (B_FULL)
    return NULL;

  if (bgpstream_elem_custom_snprintf(buf_p, B_REMAIN, elem, 0) == NULL) {
    return NULL;
  }

  written += c;
  buf_p += c;

  if (B_FULL)
    return NULL;

  return buf;
}
