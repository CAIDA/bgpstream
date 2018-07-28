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

#include "bgpstream_utils_addr_set.h"

/* PRIVATE */

#define STORAGE_HASH_VAL(arg) bgpstream_addr_storage_hash(&(arg))
#define STORAGE_EQUAL_VAL(arg1, arg2)                                          \
  bgpstream_addr_storage_equal(&(arg1), &(arg2))

#define V4_HASH_VAL(arg) bgpstream_ipv4_addr_hash(&(arg))
#define V4_EQUAL_VAL(arg1, arg2) bgpstream_ipv4_addr_equal(&(arg1), &(arg2))

#define V6_HASH_VAL(arg) bgpstream_ipv6_addr_hash(&(arg))
#define V6_EQUAL_VAL(arg1, arg2) bgpstream_ipv6_addr_equal(&(arg1), &(arg2))

/* STORAGE */
KHASH_INIT(bgpstream_addr_storage_set /* name */,
           bgpstream_addr_storage_t /* khkey_t */, char /* khval_t */,
           0 /* kh_is_set */, STORAGE_HASH_VAL /*__hash_func */,
           STORAGE_EQUAL_VAL /* __hash_equal */);

struct bgpstream_addr_storage_set {
  khash_t(bgpstream_addr_storage_set) * hash;
};

/* IPv4 */
KHASH_INIT(bgpstream_ipv4_addr_set /* name */,
           bgpstream_ipv4_addr_t /* khkey_t */, char /* khval_t */,
           0 /* kh_is_set */, V4_HASH_VAL /*__hash_func */,
           V4_EQUAL_VAL /* __hash_equal */);

struct bgpstream_ipv4_addr_set {
  khash_t(bgpstream_ipv4_addr_set) * hash;
};

/* IPv6 */
KHASH_INIT(bgpstream_ipv6_addr_set /* name */,
           bgpstream_ipv6_addr_t /* khkey_t */, char /* khval_t */,
           0 /* kh_is_set */, V6_HASH_VAL /*__hash_func */,
           V6_EQUAL_VAL /* __hash_equal */);

struct bgpstream_ipv6_addr_set {
  khash_t(bgpstream_ipv6_addr_set) * hash;
};

/* PUBLIC FUNCTIONS */

/* STORAGE */

bgpstream_addr_storage_set_t *bgpstream_addr_storage_set_create()
{
  bgpstream_addr_storage_set_t *set;

  if ((set = (bgpstream_addr_storage_set_t *)malloc(
         sizeof(bgpstream_addr_storage_set_t))) == NULL) {
    return NULL;
  }

  if ((set->hash = kh_init(bgpstream_addr_storage_set)) == NULL) {
    bgpstream_addr_storage_set_destroy(set);
    return NULL;
  }

  return set;
}

int bgpstream_addr_storage_set_insert(bgpstream_addr_storage_set_t *set,
                                      bgpstream_addr_storage_t *addr)
{
  int khret;
  khiter_t k;
  if ((k = kh_get(bgpstream_addr_storage_set, set->hash, *addr)) ==
      kh_end(set->hash)) {
    k = kh_put(bgpstream_addr_storage_set, set->hash, *addr, &khret);
    return 1;
  }
  return 0;
}

int bgpstream_addr_storage_set_size(bgpstream_addr_storage_set_t *set)
{
  return kh_size(set->hash);
}

int bgpstream_addr_storage_set_merge(bgpstream_addr_storage_set_t *dst_set,
                                     bgpstream_addr_storage_set_t *src_set)
{
  khiter_t k;
  for (k = kh_begin(src_set->hash); k != kh_end(src_set->hash); ++k) {
    if (kh_exist(src_set->hash, k)) {
      if (bgpstream_addr_storage_set_insert(dst_set,
                                            &(kh_key(src_set->hash, k))) < 0) {
        return -1;
      }
    }
  }
  return 0;
}

void bgpstream_addr_storage_set_destroy(bgpstream_addr_storage_set_t *set)
{
  kh_destroy(bgpstream_addr_storage_set, set->hash);
  free(set);
}

void bgpstream_addr_storage_set_clear(bgpstream_addr_storage_set_t *set)
{
  kh_clear(bgpstream_addr_storage_set, set->hash);
}

/* IPv4 */

bgpstream_ipv4_addr_set_t *bgpstream_ipv4_addr_set_create()
{
  bgpstream_ipv4_addr_set_t *set;

  if ((set = (bgpstream_ipv4_addr_set_t *)malloc(
         sizeof(bgpstream_ipv4_addr_set_t))) == NULL) {
    return NULL;
  }

  if ((set->hash = kh_init(bgpstream_ipv4_addr_set)) == NULL) {
    bgpstream_ipv4_addr_set_destroy(set);
    return NULL;
  }

  return set;
}

int bgpstream_ipv4_addr_set_insert(bgpstream_ipv4_addr_set_t *set,
                                   bgpstream_ipv4_addr_t *addr)
{
  int khret;
  khiter_t k;
  if ((k = kh_get(bgpstream_ipv4_addr_set, set->hash, *addr)) ==
      kh_end(set->hash)) {
    k = kh_put(bgpstream_ipv4_addr_set, set->hash, *addr, &khret);
    return 1;
  }
  return 0;
}

int bgpstream_ipv4_addr_set_size(bgpstream_ipv4_addr_set_t *set)
{
  return kh_size(set->hash);
}

int bgpstream_ipv4_addr_set_merge(bgpstream_ipv4_addr_set_t *dst_set,
                                  bgpstream_ipv4_addr_set_t *src_set)
{
  khiter_t k;
  for (k = kh_begin(src_set->hash); k != kh_end(src_set->hash); ++k) {
    if (kh_exist(src_set->hash, k)) {
      if (bgpstream_ipv4_addr_set_insert(dst_set, &(kh_key(src_set->hash, k))) <
          0) {
        return -1;
      }
    }
  }
  return 0;
}

void bgpstream_ipv4_addr_set_destroy(bgpstream_ipv4_addr_set_t *set)
{
  kh_destroy(bgpstream_ipv4_addr_set, set->hash);
  free(set);
}

void bgpstream_ipv4_addr_set_clear(bgpstream_ipv4_addr_set_t *set)
{
  kh_clear(bgpstream_ipv4_addr_set, set->hash);
}

/* IPv6 */

bgpstream_ipv6_addr_set_t *bgpstream_ipv6_addr_set_create()
{
  bgpstream_ipv6_addr_set_t *set;

  if ((set = (bgpstream_ipv6_addr_set_t *)malloc(
         sizeof(bgpstream_ipv6_addr_set_t))) == NULL) {
    return NULL;
  }

  if ((set->hash = kh_init(bgpstream_ipv6_addr_set)) == NULL) {
    bgpstream_ipv6_addr_set_destroy(set);
    return NULL;
  }

  return set;
}

int bgpstream_ipv6_addr_set_insert(bgpstream_ipv6_addr_set_t *set,
                                   bgpstream_ipv6_addr_t *addr)
{
  int khret;
  khiter_t k;
  if ((k = kh_get(bgpstream_ipv6_addr_set, set->hash, *addr)) ==
      kh_end(set->hash)) {
    k = kh_put(bgpstream_ipv6_addr_set, set->hash, *addr, &khret);
    return 1;
  }
  return 0;
}

int bgpstream_ipv6_addr_set_size(bgpstream_ipv6_addr_set_t *set)
{
  return kh_size(set->hash);
}

int bgpstream_ipv6_addr_set_merge(bgpstream_ipv6_addr_set_t *dst_set,
                                  bgpstream_ipv6_addr_set_t *src_set)
{
  khiter_t k;
  for (k = kh_begin(src_set->hash); k != kh_end(src_set->hash); ++k) {
    if (kh_exist(src_set->hash, k)) {
      if (bgpstream_ipv6_addr_set_insert(dst_set, &(kh_key(src_set->hash, k))) <
          0) {
        return -1;
      }
    }
  }
  return 0;
}

void bgpstream_ipv6_addr_set_destroy(bgpstream_ipv6_addr_set_t *set)
{
  kh_destroy(bgpstream_ipv6_addr_set, set->hash);
  free(set);
}

void bgpstream_ipv6_addr_set_clear(bgpstream_ipv6_addr_set_t *set)
{
  kh_clear(bgpstream_ipv6_addr_set, set->hash);
}
