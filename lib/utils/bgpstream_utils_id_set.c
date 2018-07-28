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

#include "bgpstream_utils_id_set.h"

/* PRIVATE */

/** set of unique ids
 *  this structure maintains a set of unique
 *  ids (using a uint32 type)
 */
KHASH_INIT(bgpstream_id_set /* name */, uint32_t /* khkey_t */,
           char /* khval_t */, 0 /* kh_is_set */,
           kh_int_hash_func /*__hash_func */,
           kh_int_hash_equal /* __hash_equal */);

struct bgpstream_id_set {
  khiter_t k;
  khash_t(bgpstream_id_set) * hash;
};

/* PUBLIC FUNCTIONS */

bgpstream_id_set_t *bgpstream_id_set_create()
{
  bgpstream_id_set_t *set;

  if ((set = (bgpstream_id_set_t *)malloc(sizeof(bgpstream_id_set_t))) ==
      NULL) {
    return NULL;
  }

  if ((set->hash = kh_init(bgpstream_id_set)) == NULL) {
    bgpstream_id_set_destroy(set);
    return NULL;
  }
  bgpstream_id_set_rewind(set);
  return set;
}

int bgpstream_id_set_insert(bgpstream_id_set_t *set, uint32_t id)
{
  int khret;
  khiter_t k;
  if ((k = kh_get(bgpstream_id_set, set->hash, id)) == kh_end(set->hash)) {
    /** @todo we should always check the return value from khash funcs */
    k = kh_put(bgpstream_id_set, set->hash, id, &khret);
    return 1;
  }
  return 0;
}

int bgpstream_id_set_exists(bgpstream_id_set_t *set, uint32_t id)
{
  khiter_t k;
  if ((k = kh_get(bgpstream_id_set, set->hash, id)) == kh_end(set->hash)) {
    return 0;
  }
  return 1;
}

int bgpstream_id_set_merge(bgpstream_id_set_t *dst_set,
                           bgpstream_id_set_t *src_set)
{
  khiter_t k;
  for (k = kh_begin(src_set->hash); k != kh_end(src_set->hash); ++k) {
    if (kh_exist(src_set->hash, k)) {
      if (bgpstream_id_set_insert(dst_set, kh_key(src_set->hash, k)) < 0) {
        return -1;
      }
    }
  }
  bgpstream_id_set_rewind(dst_set);
  bgpstream_id_set_rewind(src_set);
  return 0;
}

void bgpstream_id_set_rewind(bgpstream_id_set_t *set)
{
  set->k = kh_begin(set->hash);
}

uint32_t *bgpstream_id_set_next(bgpstream_id_set_t *set)
{
  uint32_t *v = NULL;
  for (; set->k != kh_end(set->hash); ++set->k) {
    if (kh_exist(set->hash, set->k)) {
      v = &kh_key(set->hash, set->k);
      set->k++;
      return v;
    }
  }
  return NULL;
}

int bgpstream_id_set_size(bgpstream_id_set_t *set)
{
  return kh_size(set->hash);
}

void bgpstream_id_set_destroy(bgpstream_id_set_t *set)
{
  kh_destroy(bgpstream_id_set, set->hash);
  free(set);
}

void bgpstream_id_set_clear(bgpstream_id_set_t *set)
{
  bgpstream_id_set_rewind(set);
  kh_clear(bgpstream_id_set, set->hash);
}
