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

/*
  Some code adapted from libbgpdump:

  Copyright (c) 2007 - 2010 RIPE NCC - All Rights Reserved

  Permission to use, copy, modify, and distribute this software and its
  documentation for any purpose and without fee is hereby granted, provided
  that the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation, and that the name of the author not be used in advertising or
  publicity pertaining to distribution of the software without specific,
  written prior permission.

  THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS; IN NO EVENT SHALL
  AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
  DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  Parts of this code have been engineered after analiyzing GNU Zebra's
  source code and therefore might contain declarations/code from GNU
  Zebra, Copyright (C) 1999 Kunihiro Ishiguro. Zebra is a free routing
  software, distributed under the GNU General Public License. A copy of
  this license is included with libbgpdump.
  Original Author: Shufu Mao(msf98@mails.tsinghua.edu.cn)
*/

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "bgpdump_lib.h"
#include "utils.h"

#include "bgpstream_utils_as_path_int.h"
#include "bgpstream_utils_community_int.h"

#include "bgpstream_elem_int.h"
#include "bgpstream_debug.h"
#include "bgpstream_elem_generator.h"

struct bgpstream_elem_generator {

  /** Array of elems */
  bgpstream_elem_t **elems;

  /** Number of elems that are active in the elems list */
  int elems_cnt;

  /** Number of allocated elems in the elems list */
  int elems_alloc_cnt;

  /* Current iterator position (iter == cnt means end-of-list) */
  int iter;
};

/* ==================== PRIVATE FUNCTIONS ==================== */

static bgpstream_elem_t *get_new_elem(bgpstream_elem_generator_t *self)
{
  bgpstream_elem_t *elem = NULL;

  /* check if we need to alloc more elems */
  if (self->elems_cnt + 1 >= self->elems_alloc_cnt) {

    /* alloc more memory */
    if ((self->elems = realloc(self->elems, sizeof(bgpstream_elem_t *) *
                                              (self->elems_alloc_cnt + 1))) ==
        NULL) {
      return NULL;
    }

    /* create an elem */
    if ((self->elems[self->elems_alloc_cnt] = bgpstream_elem_create()) ==
        NULL) {
      return NULL;
    }

    self->elems_alloc_cnt++;
  }

  elem = self->elems[self->elems_cnt++];
  bgpstream_elem_clear(elem);
  return elem;
}

/* ==================== BGPDUMP JUNK ==================== */

#if 0
/** @todo consider moving this code into bgpstream_utils_as */
static void get_aspath_struct(struct aspath *ap,
                              bgpstream_as_path_t *bs_ap)
{
  const char *invalid_characters = "([{}])";
  // char origin_copy[16];
  uint32_t it;
  char * tok = NULL;
  char *c = ap->str;
  char *next;
  bs_ap->hop_count = ap->count;
  bs_ap->type = BGPSTREAM_AS_TYPE_UNKNOWN;

  if(ap->str == NULL || bs_ap->hop_count == 0) {
    // aspath is empty, if it is an internal AS bgp info that is fine
    return;
  }
  // check if there are sets or confederations
  while (*c) {
    if(strchr(invalid_characters, *c)) {
      bs_ap->type = BGPSTREAM_AS_TYPE_STRING;
      bs_ap->str_aspath = NULL;
      break;
    }
    c++;
  }
  /* if sets or confederations are present, then
   * the AS_PATH is of type STRING */
  if(bs_ap->type == BGPSTREAM_AS_TYPE_STRING) {
    /* if the type is STR then we allocate the memory
     * required for the path - we do not copy ap->str
     * it is a fixed length array which is unreasonably
     long (8000)*/
    bs_ap->str_aspath = strdup(ap->str);

    /** @todo fix this function to return -1 on failure */
    assert(bs_ap->str_aspath);
  }

  /* if type has not been changed, then it is  numeric, then  */

  if(bs_ap->type == BGPSTREAM_AS_TYPE_UNKNOWN)
    {
      bs_ap->type =  BGPSTREAM_AS_TYPE_NUMERIC;
      it = 0;
      bs_ap->numeric_aspath = (uint32_t *)malloc(bs_ap->hop_count * sizeof(uint32_t));
      /** @todo fix this function to return -1 on failure */
      if(bs_ap->numeric_aspath == NULL) {
	bgpstream_log_err("get_aspath_struct: can't malloc aspath numeric array");
        assert(0);
	return;
      }

      next = c = strdup(ap->str);
      while((tok = strsep(&next, " ")) != NULL) {
        if(it == bs_ap->hop_count)
          {            
            fprintf(stderr, "Wrong hop count %d - %s\n", bs_ap->hop_count, ap->str);
            assert(0);
          }
	// strcpy(origin_copy, tok);
	bs_ap->numeric_aspath[it] = strtoul(tok, NULL, 10);
	it++;
      }
      free(c);
    }

}
#endif

/* ribs related functions */
static int table_line_mrtd_route(bgpstream_elem_generator_t *self,
                                 BGPDUMP_ENTRY *entry)
{

  bgpstream_elem_t *ri;
  if ((ri = get_new_elem(self)) == NULL) {
    return -1;
  }

  // general
  ri->type = BGPSTREAM_ELEM_TYPE_RIB;
  ri->timestamp = entry->time;

  // peer and prefix
  if (entry->subtype == AFI_IP6) {
    ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV6;
    ri->peer_address.ipv6 = entry->body.mrtd_table_dump.peer_ip.v6_addr;
    ri->prefix.address.version = BGPSTREAM_ADDR_VERSION_IPV6;
    ri->prefix.address.ipv6 = entry->body.mrtd_table_dump.prefix.v6_addr;
  } else {
    ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV4;
    ri->peer_address.ipv4 = entry->body.mrtd_table_dump.peer_ip.v4_addr;
    ri->prefix.address.version = BGPSTREAM_ADDR_VERSION_IPV4;
    ri->prefix.address.ipv4 = entry->body.mrtd_table_dump.prefix.v4_addr;
  }
  ri->prefix.mask_len = entry->body.mrtd_table_dump.mask;
  ri->peer_asnumber = entry->body.mrtd_table_dump.peer_as;

  // as path
  if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_AS_PATH) &&
      entry->attr->aspath) {
    bgpstream_as_path_populate(ri->aspath, entry->attr->aspath);
  }

  // communities
  if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES) &&
      entry->attr->community) {
    bgpstream_community_set_populate(ri->communities, entry->attr->community);
  }

  // nextop
  if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_REACH_NLRI)) &&
      entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]) {
    ri->nexthop.version = BGPSTREAM_ADDR_VERSION_IPV6;
    ri->nexthop.ipv6 =
      entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]->nexthop.v6_addr;
  } else {
    ri->nexthop.version = BGPSTREAM_ADDR_VERSION_IPV4;
    ri->nexthop.ipv4 = entry->attr->nexthop;
  }
  return 0;
}

int table_line_dump_v2_prefix(bgpstream_elem_generator_t *self,
                              BGPDUMP_ENTRY *entry)
{
  bgpstream_elem_t *ri;

  BGPDUMP_TABLE_DUMP_V2_PREFIX *e = &(entry->body.mrtd_table_dump_v2_prefix);
  int i;

  for (i = 0; i < e->entry_count; i++) {
    attributes_t *attr = e->entries[i].attr;
    if (!attr)
      continue;

    if ((ri = get_new_elem(self)) == NULL) {
      return -1;
    }

    // general info
    ri->type = BGPSTREAM_ELEM_TYPE_RIB;
    ri->timestamp = entry->time;

    // peer
    if (e->entries[i].peer.afi == AFI_IP) {
      ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV4;
      ri->peer_address.ipv4 = e->entries[i].peer.peer_ip.v4_addr;
    } else {
      if (e->entries[i].peer.afi == AFI_IP6) {
        ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV6;
        ri->peer_address.ipv6 = e->entries[i].peer.peer_ip.v6_addr;
      } else {
        return -1;
      }
    }

    ri->peer_asnumber = e->entries[i].peer.peer_as;

    // prefix
    if (e->afi == AFI_IP) {
      ri->prefix.address.version = BGPSTREAM_ADDR_VERSION_IPV4;
      ri->prefix.address.ipv4 = e->prefix.v4_addr;
    } else {
      if (e->afi == AFI_IP6) {
        ri->prefix.address.version = BGPSTREAM_ADDR_VERSION_IPV6;
        ri->prefix.address.ipv6 = e->prefix.v6_addr;
      }
    }
    ri->prefix.mask_len = e->prefix_length;
    // as path
    if (attr->aspath) {
      bgpstream_as_path_populate(ri->aspath, attr->aspath);
    }
    // communities
    if (attr->community) {
      bgpstream_community_set_populate(ri->communities, attr->community);
    }
    // next hop
    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_REACH_NLRI)) &&
        attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]) {
      ri->nexthop.version = BGPSTREAM_ADDR_VERSION_IPV6;
      ri->nexthop.ipv6 =
        attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]->nexthop.v6_addr;
    } else {
      ri->nexthop.version = BGPSTREAM_ADDR_VERSION_IPV4;
      ri->nexthop.ipv4 = attr->nexthop;
    }
  }
  return 0;
}

static int table_line_announce(bgpstream_elem_generator_t *self,
                               struct prefix *prefix, int count,
                               BGPDUMP_ENTRY *entry)
{
  bgpstream_elem_t *ri;
  int idx;
  for (idx = 0; idx < count; idx++) {
    if ((ri = get_new_elem(self)) == NULL) {
      return -1;
    }

    // general info
    ri->type = BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT;
    ri->timestamp = entry->time;
    // peer
    if (entry->body.zebra_message.address_family == AFI_IP6) {
      ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV6;
      ri->peer_address.ipv6 = entry->body.zebra_message.source_ip.v6_addr;
    }
    if (entry->body.zebra_message.address_family == AFI_IP) {
      ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV4;
      ri->peer_address.ipv4 = entry->body.zebra_message.source_ip.v4_addr;
    }
    ri->peer_asnumber = entry->body.zebra_message.source_as;
    // prefix (ipv4)
    ri->prefix.address.version = BGPSTREAM_ADDR_VERSION_IPV4;
    ri->prefix.address.ipv4 = prefix[idx].address.v4_addr;
    ri->prefix.mask_len = prefix[idx].len;
    // nexthop (ipv4)
    ri->nexthop.version = BGPSTREAM_ADDR_VERSION_IPV4;
    ri->nexthop.ipv4 = entry->attr->nexthop;
    // as path
    if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_AS_PATH) &&
        entry->attr->aspath) {
      bgpstream_as_path_populate(ri->aspath, entry->attr->aspath);
    }
    // communities
    if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES) &&
        entry->attr->community) {
      bgpstream_community_set_populate(ri->communities, entry->attr->community);
    }
  }
  return 0;
}

static int table_line_announce_1(bgpstream_elem_generator_t *self,
                                 struct mp_nlri *prefix, int count,
                                 BGPDUMP_ENTRY *entry)
{
  bgpstream_elem_t *ri;
  int idx;
  for (idx = 0; idx < count; idx++) {
    if ((ri = get_new_elem(self)) == NULL) {
      return -1;
    }

    // general info
    ri->type = BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT;
    ri->timestamp = entry->time;
    // peer
    if (entry->body.zebra_message.address_family == AFI_IP6) {
      ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV6;
      ri->peer_address.ipv6 = entry->body.zebra_message.source_ip.v6_addr;
    }
    if (entry->body.zebra_message.address_family == AFI_IP) {
      ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV4;
      ri->peer_address.ipv4 = entry->body.zebra_message.source_ip.v4_addr;
    }
    ri->peer_asnumber = entry->body.zebra_message.source_as;
    // prefix (ipv4)
    ri->prefix.address.version = BGPSTREAM_ADDR_VERSION_IPV4;
    ri->prefix.address.ipv4 = prefix->nlri[idx].address.v4_addr;
    ri->prefix.mask_len = prefix->nlri[idx].len;
    // nexthop (ipv4)
    ri->nexthop.version = BGPSTREAM_ADDR_VERSION_IPV4;
    ri->nexthop.ipv4 = entry->attr->nexthop;
    // as path
    if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_AS_PATH) &&
        entry->attr->aspath) {
      bgpstream_as_path_populate(ri->aspath, entry->attr->aspath);
    }
    // communities
    if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES) &&
        entry->attr->community) {
      bgpstream_community_set_populate(ri->communities, entry->attr->community);
    }
  }
  return 0;
}

static int table_line_announce6(bgpstream_elem_generator_t *self,
                                struct mp_nlri *prefix, int count,
                                BGPDUMP_ENTRY *entry)
{
  bgpstream_elem_t *ri;
  int idx;
  for (idx = 0; idx < count; idx++) {
    if ((ri = get_new_elem(self)) == NULL) {
      return -1;
    }

    // general info
    ri->type = BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT;
    ri->timestamp = entry->time;
    // peer
    if (entry->body.zebra_message.address_family == AFI_IP6) {
      ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV6;
      ri->peer_address.ipv6 = entry->body.zebra_message.source_ip.v6_addr;
    }
    if (entry->body.zebra_message.address_family == AFI_IP) {
      ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV4;
      ri->peer_address.ipv4 = entry->body.zebra_message.source_ip.v4_addr;
    }
    ri->peer_asnumber = entry->body.zebra_message.source_as;
    // prefix (ipv6)
    ri->prefix.address.version = BGPSTREAM_ADDR_VERSION_IPV6;
    ri->prefix.address.ipv6 = prefix->nlri[idx].address.v6_addr;
    ri->prefix.mask_len = prefix->nlri[idx].len;
    // nexthop (ipv6)
    ri->nexthop.version = BGPSTREAM_ADDR_VERSION_IPV6;
    ri->nexthop.ipv6 = prefix->nexthop.v6_addr;
    // aspath
    if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_AS_PATH) &&
        entry->attr->aspath) {
      bgpstream_as_path_populate(ri->aspath, entry->attr->aspath);
    }
    // communities
    if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES) &&
        entry->attr->community) {
      bgpstream_community_set_populate(ri->communities, entry->attr->community);
    }
  }
  return 0;
}

static int table_line_withdraw(bgpstream_elem_generator_t *self,
                               struct prefix *prefix, int count,
                               BGPDUMP_ENTRY *entry)
{
  bgpstream_elem_t *ri;
  int idx;
  for (idx = 0; idx < count; idx++) {
    if ((ri = get_new_elem(self)) == NULL) {
      return -1;
    }

    // general info
    ri->type = BGPSTREAM_ELEM_TYPE_WITHDRAWAL;
    ri->timestamp = entry->time;
    // peer
    if (entry->body.zebra_message.address_family == AFI_IP6) {
      ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV6;
      ri->peer_address.ipv6 = entry->body.zebra_message.source_ip.v6_addr;
    }
    if (entry->body.zebra_message.address_family == AFI_IP) {
      ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV4;
      ri->peer_address.ipv4 = entry->body.zebra_message.source_ip.v4_addr;
    }
    ri->peer_asnumber = entry->body.zebra_message.source_as;
    // prefix (ipv4)
    ri->prefix.address.version = BGPSTREAM_ADDR_VERSION_IPV4;
    ri->prefix.address.ipv4 = prefix[idx].address.v4_addr;
    ri->prefix.mask_len = prefix[idx].len;
  }
  return 0;
}

static int table_line_withdraw6(bgpstream_elem_generator_t *self,
                                struct prefix *prefix, int count,
                                BGPDUMP_ENTRY *entry)
{
  bgpstream_elem_t *ri;
  int idx;
  for (idx = 0; idx < count; idx++) {
    if ((ri = get_new_elem(self)) == NULL) {
      return -1;
    }

    // general info
    ri->type = BGPSTREAM_ELEM_TYPE_WITHDRAWAL;
    ri->timestamp = entry->time;
    // peer
    if (entry->body.zebra_message.address_family == AFI_IP6) {
      ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV6;
      ri->peer_address.ipv6 = entry->body.zebra_message.source_ip.v6_addr;
    }
    if (entry->body.zebra_message.address_family == AFI_IP) {
      ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV4;
      ri->peer_address.ipv4 = entry->body.zebra_message.source_ip.v4_addr;
    }
    ri->peer_asnumber = entry->body.zebra_message.source_as;
    // prefix (ipv6)
    ri->prefix.address.version = BGPSTREAM_ADDR_VERSION_IPV6;
    ri->prefix.address.ipv6 = prefix[idx].address.v6_addr;
    ri->prefix.mask_len = prefix[idx].len;
  }
  return 0;
}

static int table_line_update(bgpstream_elem_generator_t *self,
                             BGPDUMP_ENTRY *entry)
{
  struct prefix *prefix;
  struct mp_nlri *prefix_mp;
  int count;

  // withdrawals (IPv4)
  if ((entry->body.zebra_message.withdraw_count) ||
      (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_UNREACH_NLRI))) {
    prefix = entry->body.zebra_message.withdraw;
    count = entry->body.zebra_message.withdraw_count;
    if (table_line_withdraw(self, prefix, count, entry) != 0) {
      return -1;
    }
  }
  if (entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST] &&
      entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST]->prefix_count) {
    prefix = entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST]->nlri;
    count = entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST]->prefix_count;
    if (table_line_withdraw(self, prefix, count, entry) != 0) {
      return -1;
    }
  }
  if (entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST] &&
      entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST]->prefix_count) {
    prefix = entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST]->nlri;
    count =
      entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST]->prefix_count;
    if (table_line_withdraw(self, prefix, count, entry) != 0) {
      return -1;
    }
  }
  if (entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST] &&
      entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST]
        ->prefix_count) {
    prefix =
      entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST]->nlri;
    count = entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST]
              ->prefix_count;
    if (table_line_withdraw(self, prefix, count, entry) != 0) {
      return -1;
    }
  }

  // withdrawals (IPv6)
  if (entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST] &&
      entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST]->prefix_count) {
    prefix = entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST]->nlri;
    count = entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST]->prefix_count;
    if (table_line_withdraw6(self, prefix, count, entry) != 0) {
      return -1;
    }
  }
  if (entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST] &&
      entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST]->prefix_count) {
    prefix = entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST]->nlri;
    count =
      entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST]->prefix_count;
    if (table_line_withdraw6(self, prefix, count, entry) != 0) {
      return -1;
    }
  }
  if (entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST] &&
      entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST]
        ->prefix_count) {
    prefix =
      entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST]->nlri;
    count = entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST]
              ->prefix_count;
    if (table_line_withdraw6(self, prefix, count, entry) != 0) {
      return -1;
    }
  }

  // announce
  if ((entry->body.zebra_message.announce_count) ||
      (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_REACH_NLRI))) {
    prefix = entry->body.zebra_message.announce;
    count = entry->body.zebra_message.announce_count;
    if (table_line_announce(self, prefix, count, entry) != 0) {
      return -1;
    }
  }

  // announce 1
  if (entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST] &&
      entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST]->prefix_count) {
    prefix_mp = entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST];
    count = entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST]->prefix_count;
    if (table_line_announce_1(self, prefix_mp, count, entry) != 0) {
      return -1;
    }
  }
  if (entry->attr->mp_info->announce[AFI_IP][SAFI_MULTICAST] &&
      entry->attr->mp_info->announce[AFI_IP][SAFI_MULTICAST]->prefix_count) {
    prefix_mp = entry->attr->mp_info->announce[AFI_IP][SAFI_MULTICAST];
    count =
      entry->attr->mp_info->announce[AFI_IP][SAFI_MULTICAST]->prefix_count;
    if (table_line_announce_1(self, prefix_mp, count, entry) != 0) {
      return -1;
    }
  }
  if (entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST] &&
      entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST]
        ->prefix_count) {
    prefix_mp = entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST];
    count = entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST]
              ->prefix_count;
    if (table_line_announce_1(self, prefix_mp, count, entry) != 0) {
      return -1;
    }
  }

  // announce ipv6
  if (entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST] &&
      entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]->prefix_count) {
    prefix_mp = entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST];
    count = entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]->prefix_count;
    if (table_line_announce6(self, prefix_mp, count, entry) != 0) {
      return -1;
    }
  }
  if (entry->attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST] &&
      entry->attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST]->prefix_count) {
    prefix_mp = entry->attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST];
    count =
      entry->attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST]->prefix_count;
    if (table_line_announce6(self, prefix_mp, count, entry) != 0) {
      return -1;
    }
  }
  if (entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST] &&
      entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST]
        ->prefix_count) {
    prefix_mp = entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST];
    count = entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST]
              ->prefix_count;
    if (table_line_announce6(self, prefix_mp, count, entry) != 0) {
      return -1;
    }
  }

  return 0;
}

static int bgp_state_change(bgpstream_elem_generator_t *self,
                            BGPDUMP_ENTRY *entry)
{
  bgpstream_elem_t *ri;
  if ((ri = get_new_elem(self)) == NULL) {
    return -1;
  }

  // general information
  ri->type = BGPSTREAM_ELEM_TYPE_PEERSTATE;
  ri->timestamp = entry->time;
  // peer
  if (entry->body.zebra_message.address_family == AFI_IP6) {
    ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV6;
    ri->peer_address.ipv6 = entry->body.zebra_state_change.source_ip.v6_addr;
  }
  if (entry->body.zebra_message.address_family == AFI_IP) {
    ri->peer_address.version = BGPSTREAM_ADDR_VERSION_IPV4;
    ri->peer_address.ipv4 = entry->body.zebra_state_change.source_ip.v4_addr;
  }
  ri->peer_asnumber = entry->body.zebra_message.source_as;
  ri->old_state = entry->body.zebra_state_change.old_state;
  ri->new_state = entry->body.zebra_state_change.new_state;
  return 0;
}

/* ==================== PROTECTED FUNCTIONS ==================== */

bgpstream_elem_generator_t *bgpstream_elem_generator_create()
{
  bgpstream_elem_generator_t *self;

  if ((self = malloc_zero(sizeof(bgpstream_elem_generator_t))) == NULL) {
    return NULL;
  }

  /* indicates not populated */
  self->elems_cnt = -1;

  return self;
}

void bgpstream_elem_generator_destroy(bgpstream_elem_generator_t *self)
{
  int i;
  if (self == NULL) {
    return;
  }

  /* free all the alloc'd elems */
  for (i = 0; i < self->elems_alloc_cnt; i++) {
    bgpstream_elem_destroy(self->elems[i]);
    self->elems[i] = NULL;
  }

  free(self->elems);

  self->elems_cnt = self->elems_alloc_cnt = self->iter = 0;

  free(self);
}

void bgpstream_elem_generator_clear(bgpstream_elem_generator_t *self)
{
  /* explicit clear is done by get_next_elem */

  self->elems_cnt = -1;
  self->iter = 0;
}

int bgpstream_elem_generator_is_populated(bgpstream_elem_generator_t *self)
{
  return self->elems_cnt != -1;
}

int bgpstream_elem_generator_populate(bgpstream_elem_generator_t *self,
                                      bgpstream_record_t *record)
{
  assert(record != NULL);

  /* bgpstream_record must have cleared already */
  assert(self->elems_cnt == -1);

  /* start with an empty set */
  self->elems_cnt = 0;

  if (record->bd_entry == NULL ||
      record->status != BGPSTREAM_RECORD_STATUS_VALID_RECORD) {
    return 0;
  }

  switch (record->bd_entry->type) {
  case BGPDUMP_TYPE_MRTD_TABLE_DUMP:
    return table_line_mrtd_route(self, record->bd_entry);
    break;

  case BGPDUMP_TYPE_TABLE_DUMP_V2:
    return table_line_dump_v2_prefix(self, record->bd_entry);
    break;

  case BGPDUMP_TYPE_ZEBRA_BGP:
    switch (record->bd_entry->subtype) {
    case BGPDUMP_SUBTYPE_ZEBRA_BGP_MESSAGE:
    case BGPDUMP_SUBTYPE_ZEBRA_BGP_MESSAGE_AS4:
      switch (record->bd_entry->body.zebra_message.type) {
      case BGP_MSG_UPDATE:
        return table_line_update(self, record->bd_entry);
        break;
      }
      break;

    case BGPDUMP_SUBTYPE_ZEBRA_BGP_STATE_CHANGE: // state messages
    case BGPDUMP_SUBTYPE_ZEBRA_BGP_STATE_CHANGE_AS4:
      return bgp_state_change(self, record->bd_entry);
      break;
    }
  }
  return -1;
}

bgpstream_elem_t *
bgpstream_elem_generator_get_next_elem(bgpstream_elem_generator_t *self)
{
  bgpstream_elem_t *elem = NULL;

  if (self->iter < self->elems_cnt) {
    elem = self->elems[self->iter];
    self->iter++;
  }

  return elem;
}
