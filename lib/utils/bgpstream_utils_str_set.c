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

#include "bgpstream_utils_str_set.h"

/* PRIVATE */

KHASH_INIT(bgpstream_str_set, char *, char, 0, kh_str_hash_func,
           kh_str_hash_equal);

struct bgpstream_str_set_t {
  khiter_t k;
  khash_t(bgpstream_str_set) * hash;
};

/* PUBLIC FUNCTIONS */

bgpstream_str_set_t *bgpstream_str_set_create()
{
  bgpstream_str_set_t *set;

  if ((set = (bgpstream_str_set_t *)malloc(sizeof(bgpstream_str_set_t))) ==
      NULL) {
    return NULL;
  }

  if ((set->hash = kh_init(bgpstream_str_set)) == NULL) {
    bgpstream_str_set_destroy(set);
    return NULL;
  }

  bgpstream_str_set_rewind(set);
  return set;
}

int bgpstream_str_set_insert(bgpstream_str_set_t *set, const char *val)
{
  int khret;
  khiter_t k;
  char *cpy;
  if ((cpy = strdup(val)) == NULL) {
    return -1;
  }

  if ((k = kh_get(bgpstream_str_set, set->hash, cpy)) == kh_end(set->hash)) {
    k = kh_put(bgpstream_str_set, set->hash, cpy, &khret);
    return 1;
  } else {
    free(cpy);
  }
  return 0;
}

int bgpstream_str_set_remove(bgpstream_str_set_t *set, char *val)
{
  khiter_t k;
  bgpstream_str_set_rewind(set);
  if ((k = kh_get(bgpstream_str_set, set->hash, val)) != kh_end(set->hash)) {
    // free memory allocated for the key (string)
    free(kh_key(set->hash, k));
    // delete entry
    kh_del(bgpstream_str_set, set->hash, k);
    return 1;
  }
  return 0;
}

int bgpstream_str_set_exists(bgpstream_str_set_t *set, char *val)
{
  khiter_t k;
  if ((k = kh_get(bgpstream_str_set, set->hash, val)) == kh_end(set->hash)) {
    return 0;
  }
  return 1;
}

int bgpstream_str_set_size(bgpstream_str_set_t *set)
{
  return kh_size(set->hash);
}

int bgpstream_str_set_merge(bgpstream_str_set_t *dst_set,
                            bgpstream_str_set_t *src_set)
{
  khiter_t k;

  for (k = kh_begin(src_set->hash); k != kh_end(src_set->hash); ++k) {
    if (kh_exist(src_set->hash, k)) {
      if (bgpstream_str_set_insert(dst_set, kh_key(src_set->hash, k)) < 0) {
        return -1;
      }
    }
  }
  bgpstream_str_set_rewind(dst_set);
  bgpstream_str_set_rewind(src_set);
  return 0;
}

void bgpstream_str_set_rewind(bgpstream_str_set_t *set)
{
  set->k = kh_begin(set->hash);
}

char *bgpstream_str_set_next(bgpstream_str_set_t *set)
{
  char *v = NULL;
  for (; set->k != kh_end(set->hash); ++set->k) {
    if (kh_exist(set->hash, set->k)) {
      v = kh_key(set->hash, set->k);
      set->k++;
      return v;
    }
  }
  return v;
}

void bgpstream_str_set_clear(bgpstream_str_set_t *set)
{
  khiter_t k;
  bgpstream_str_set_rewind(set);
  for (k = kh_begin(set->hash); k != kh_end(set->hash); ++k) {
    if (kh_exist(set->hash, k)) {
      free(kh_key(set->hash, k));
    }
  }
  kh_clear(bgpstream_str_set, set->hash);
}

void bgpstream_str_set_destroy(bgpstream_str_set_t *set)
{
  khiter_t k;
  if (set->hash != NULL) {
    for (k = kh_begin(set->hash); k != kh_end(set->hash); ++k) {
      if (kh_exist(set->hash, k)) {
        free(kh_key(set->hash, k));
      }
    }
    kh_destroy(bgpstream_str_set, set->hash);
    set->hash = NULL;
  }
  free(set);
}
