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
#include <stdio.h>

#include "khash.h"
#include "utils.h"

#include "bgpstream_utils_peer_sig_map.h"

#define IPV4_ID_OFFSET 1
#define IPV6_ID_OFFSET 1

/** Hash a peer signature into a 64bit number
 *
 * @param               the peer signature to hash
 * @return 64bit hash of the given peer signature
 */
khint64_t bgpstream_peer_sig_hash(bgpstream_peer_sig_t *ps);

/** Check if two peer signatures are equal
 *
 * @param ps1           pointer to the first peer signature
 * @param ps2           pointer to the second peer signature
 * @return 0 if the signatures are not equal, non-zero if they are equal
 */
int bgpstream_peer_sig_equal(bgpstream_peer_sig_t *ps1,
                             bgpstream_peer_sig_t *ps2);

/** Set the peer ID for the given collector/peer
 *
 * @param map           peer sig map to set peer ID for
 * @param peer_id       peer id to set
 * @param collector_str name of the collector the peer is associated with
 * @param peer_ip_addr  pointer to the peer IP address
 * @param peer_asnumber  AS number of the peer
 * @return 0 if the ID was associated successfully, -1 otherwise
 *
 * @note the peer sig map expects to be able to allocate IDs internally. This
 * function must be used with care.
 */
int bgpstream_peer_sig_map_set(bgpstream_peer_sig_map_t *map,
                               bgpstream_peer_id_t peer_id, char *collector_str,
                               bgpstream_addr_storage_t *peer_ip_addr,
                               uint32_t peer_asnumber);

/** Map from peer signature to peer ID */
KHASH_INIT(bgpstream_peer_sig_id_map, bgpstream_peer_sig_t *,
           bgpstream_peer_id_t, 1, bgpstream_peer_sig_hash,
           bgpstream_peer_sig_equal);

/** Map from peer ID to signature */
KHASH_INIT(bgpstream_peer_id_sig_map, bgpstream_peer_id_t,
           bgpstream_peer_sig_t *, 1, kh_int_hash_func, kh_int_hash_equal);

/** Structure representing an instance of a Peer Signature Map */
struct bgpstream_peer_sig_map {
  khash_t(bgpstream_peer_sig_id_map) * ps_id;
  khash_t(bgpstream_peer_id_sig_map) * id_ps;
  bgpstream_peer_id_t v4_next_id;
  bgpstream_peer_id_t v6_next_id;
};

/* PRIVATE FUNCTIONS (static) */

static void sig_free(bgpstream_peer_sig_t *sig)
{
  free(sig);
}

static bgpstream_peer_id_t
bgpstream_peer_sig_map_set_and_get_ps(bgpstream_peer_sig_map_t *map,
                                      bgpstream_peer_sig_t *ps)
{
  khiter_t k;
  int khret;
  bgpstream_peer_id_t new_id;

  if ((k = kh_get(bgpstream_peer_sig_id_map, map->ps_id, ps)) ==
      kh_end(map->ps_id)) {
    /* was not already in the map */
    /* what ID should we use? */
    if (map->v4_next_id >= IPV6_ID_OFFSET) {
      /* v4 peers are in v6 range */
      /* regardless of the version, use the v6 id */
      new_id = map->v6_next_id++;
    } else if (ps->peer_ip_addr.version == BGPSTREAM_ADDR_VERSION_IPV6) {
      assert(map->v4_next_id < IPV6_ID_OFFSET);
      new_id = map->v6_next_id++;
    } else {
      new_id = map->v4_next_id++;
    }

    /* insert into both maps */
    k = kh_put(bgpstream_peer_sig_id_map, map->ps_id, ps, &khret);
    kh_value(map->ps_id, k) = new_id;
    k = kh_put(bgpstream_peer_id_sig_map, map->id_ps, new_id, &khret);
    kh_value(map->id_ps, k) = ps;

    return new_id;
  } else {
    /* already exists... */
    free(ps); /* it was mallocd for us...*/
    return kh_value(map->ps_id, k);
  }
  return 0;
}

/* PROTECTED FUNCTIONS (_int.h) */

khint64_t bgpstream_peer_sig_hash(bgpstream_peer_sig_t *ps)
{
  /* assuming that the number of peers that have the same ip and belong to two
   * different collectors is low (in this specific case there will be a
   * collision in terms of hash). */
  return bgpstream_addr_storage_hash(&ps->peer_ip_addr);
}

/** @note we do not need to take into account the peer AS number
 *  to check whether a peer differs or not */
int bgpstream_peer_sig_equal(bgpstream_peer_sig_t *ps1,
                             bgpstream_peer_sig_t *ps2)
{
  return (
    bgpstream_addr_storage_equal(&ps1->peer_ip_addr, &ps2->peer_ip_addr) &&
    (strcmp(ps1->collector_str, ps2->collector_str) == 0));
}

/* PUBLIC FUNCTIONS */

bgpstream_peer_sig_map_t *bgpstream_peer_sig_map_create()
{
  bgpstream_peer_sig_map_t *map = NULL;
  if ((map = (bgpstream_peer_sig_map_t *)malloc_zero(
         sizeof(bgpstream_peer_sig_map_t))) == NULL) {
    return NULL;
  }

  if ((map->ps_id = kh_init(bgpstream_peer_sig_id_map)) == NULL) {
    goto err;
  }

  if ((map->id_ps = kh_init(bgpstream_peer_id_sig_map)) == NULL) {
    goto err;
  }

  map->v4_next_id = IPV4_ID_OFFSET;
  map->v6_next_id = IPV6_ID_OFFSET;

  return map;

err:
  bgpstream_peer_sig_map_destroy(map);
  return NULL;
}

bgpstream_peer_id_t bgpstream_peer_sig_map_get_id(
  bgpstream_peer_sig_map_t *map, char *collector_str,
  bgpstream_ip_addr_t *peer_ip_addr, uint32_t peer_asnumber)
{
  bgpstream_peer_sig_t *new_ps;
  if ((new_ps = malloc(sizeof(bgpstream_peer_sig_t))) == NULL) {
    return -1;
  }

  new_ps->peer_ip_addr.version = peer_ip_addr->version;
  switch (peer_ip_addr->version) {
  case BGPSTREAM_ADDR_VERSION_IPV4:
    memcpy(&new_ps->peer_ip_addr.ipv4.s_addr,
           &((bgpstream_ipv4_addr_t *)peer_ip_addr)->ipv4.s_addr,
           sizeof(uint32_t));
    break;
  case BGPSTREAM_ADDR_VERSION_IPV6:
    memcpy(&new_ps->peer_ip_addr.ipv6.s6_addr,
           &((bgpstream_ipv6_addr_t *)peer_ip_addr)->ipv6.s6_addr,
           sizeof(uint8_t) * 16);
    break;
  default:
    /* programming error */
    assert(0);
  }

  strcpy(new_ps->collector_str, collector_str);
  new_ps->peer_asnumber = peer_asnumber;

  return bgpstream_peer_sig_map_set_and_get_ps(map, new_ps);
}

bgpstream_peer_sig_t *
bgpstream_peer_sig_map_get_sig(bgpstream_peer_sig_map_t *map,
                               bgpstream_peer_id_t id)
{
  bgpstream_peer_sig_t *ps = NULL;
  khiter_t k;
  if ((k = kh_get(bgpstream_peer_id_sig_map, map->id_ps, id)) !=
      kh_end(map->id_ps)) {
    ps = kh_value(map->id_ps, k);
  }
  return ps;
}

int bgpstream_peer_sig_map_get_size(bgpstream_peer_sig_map_t *map)
{
  assert(kh_size(map->id_ps) == kh_size(map->ps_id));
  return kh_size(map->id_ps);
}

void bgpstream_peer_sig_map_destroy(bgpstream_peer_sig_map_t *map)
{
  if (map != NULL) {
    if (map->ps_id != NULL) {
      kh_destroy(bgpstream_peer_sig_id_map, map->ps_id);
      map->ps_id = NULL;
    }
    if (map->id_ps != NULL) {
      /* only call free vals on ONE map, they are shared */
      kh_free_vals(bgpstream_peer_id_sig_map, map->id_ps, sig_free);
      kh_destroy(bgpstream_peer_id_sig_map, map->id_ps);
      map->id_ps = NULL;
    }
    free(map);
  }
}

void bgpstream_peer_sig_map_clear(bgpstream_peer_sig_map_t *map)
{
  /* only call free vals on ONE map, they are shared */
  kh_free_vals(bgpstream_peer_id_sig_map, map->id_ps, sig_free);
  kh_clear(bgpstream_peer_id_sig_map, map->id_ps);
  kh_clear(bgpstream_peer_sig_id_map, map->ps_id);
}
