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

#include "config.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include "khash.h"
#include "utils.h"

#include "bgpstream_utils_as_path_int.h"

#include "bgpstream_utils_as_path_store.h"

/* wrapper around an AS path */
struct bgpstream_as_path_store_path {

  /** Is this a core path? */
  uint8_t is_core;

  /** Internal index of this path within the store */
  uint32_t idx;

  /** Underlying AS Path structure */
  bgpstream_as_path_t path;
};

/** A per-origin set of AS Paths */
typedef struct pathset {

  /** Array of AS Paths in the set */
  bgpstream_as_path_store_path_t *paths;

  /** Number of AS paths in the set */
  uint16_t paths_cnt;

} __attribute__((packed)) pathset_t;

KHASH_INIT(pathset, uint32_t, pathset_t, 1, kh_int_hash_func,
           kh_int_hash_equal);

struct bgpstream_as_path_store {

  khash_t(pathset) * path_set;

  /** The total number of paths in the store */
  uint32_t paths_cnt;

  /** The currently iterated pathset */
  khiter_t cur_pathset;

  /** The current path within the current pathset */
  int cur_path;
};

static void store_path_destroy(bgpstream_as_path_store_path_t *spath)
{
  if (spath == NULL) {
    return;
  }

  /* destroy the path's data */
  assert(spath->path.data_alloc_len != UINT16_MAX);
  free(spath->path.data);
  spath->path.data = NULL;
  spath->path.data_alloc_len = 0;
}

int store_path_dup(bgpstream_as_path_store_path_t *dst,
                   bgpstream_as_path_store_path_t *src)
{
  *dst = *src;

  /* copy the path data */
  dst->path.data_alloc_len = 0;
  if ((dst->path.data = malloc(src->path.data_len)) == NULL) {
    goto err;
  }
  dst->path.data_alloc_len = src->path.data_len;
  dst->path.data_len = src->path.data_len;
  memcpy(dst->path.data, src->path.data, src->path.data_len);

  return 0;

err:
  store_path_destroy(dst);
  return -1;
}

static inline int store_path_equal(bgpstream_as_path_store_path_t *sp1,
                                   bgpstream_as_path_store_path_t *sp2)
{
  return (sp1->is_core == sp2->is_core) &&
         bgpstream_as_path_equal(&sp1->path, &sp2->path);
}

static void pathset_destroy(pathset_t ps)
{
  int i;

  /* destroy each store path */
  for (i = 0; i < ps.paths_cnt; i++) {
    store_path_destroy(&ps.paths[i]);
  }

  /* destroy the array of paths */
  free(ps.paths);
  ps.paths = NULL;
  ps.paths_cnt = 0;
}

static uint16_t pathset_get_path_id(bgpstream_as_path_store_t *store,
                                    pathset_t *ps,
                                    bgpstream_as_path_store_path_t *findme)
{
  int i;
  uint32_t path_id;

  /* check if it is already in the set */
  for (i = 0; i < ps->paths_cnt; i++) {
    if (store_path_equal(&ps->paths[i], findme) != 0) {
      return i;
    }
  }

  findme->idx = store->paths_cnt++;

  /* need to append this path */
  if ((ps->paths = realloc(ps->paths, sizeof(bgpstream_as_path_store_path_t) *
                                        (ps->paths_cnt + 1))) == NULL) {
    fprintf(stderr, "ERROR: Could not realloc paths\n");
    return UINT16_MAX;
  }
  if (store_path_dup(&ps->paths[ps->paths_cnt], findme) != 0) {
    fprintf(stderr, "ERROR: Could not create store path\n");
    return UINT16_MAX;
  }
  path_id = ps->paths_cnt++;
  assert(ps->paths_cnt <= UINT16_MAX);

  return path_id;
}

/* ==================== PUBLIC FUNCTIONS ==================== */

bgpstream_as_path_store_t *bgpstream_as_path_store_create()
{
  bgpstream_as_path_store_t *store;

  if ((store = malloc_zero(sizeof(bgpstream_as_path_store_t))) == NULL) {
    return NULL;
  }

  if ((store->path_set = kh_init(pathset)) == NULL) {
    goto err;
  }
  /* pre-allocate to minimize resize events */
  kh_resize(pathset, store->path_set, 1 << 24); /* 2^24 = 16.8M buckets */

  return store;

err:
  bgpstream_as_path_store_destroy(store);
  return NULL;
}

void bgpstream_as_path_store_destroy(bgpstream_as_path_store_t *store)
{
  if (store == NULL) {
    return;
  }

  if (store->path_set != NULL) {
    kh_free_vals(pathset, store->path_set, pathset_destroy);
    kh_destroy(pathset, store->path_set);
    store->path_set = NULL;
  }

  free(store);
}

uint32_t bgpstream_as_path_store_get_size(bgpstream_as_path_store_t *store)
{
  return store->paths_cnt;
}

static int get_path_id(bgpstream_as_path_store_t *store,
                       bgpstream_as_path_store_path_t *findme,
                       bgpstream_as_path_store_path_id_t *id)
{
  khiter_t k;
  int khret;

  id->path_hash = bgpstream_as_path_hash(&findme->path);

  k = kh_put(pathset, store->path_set, id->path_hash, &khret);
  if (khret == 1) {
    /* just added this pathset */
    /* clear the pathset fields */
    kh_val(store->path_set, k).paths = NULL;
    kh_val(store->path_set, k).paths_cnt = 0;
  } else if (khret != 0) {
    fprintf(stderr, "ERROR: Could not add path set to the store\n");
    goto err;
  }

  /* now get the path id from the origin set */
  if ((id->path_id = pathset_get_path_id(store, &kh_val(store->path_set, k),
                                         findme)) == UINT16_MAX) {
    fprintf(stderr, "ERROR: Could not add path to origin set\n");
    goto err;
  }

  return 0;

err:
  return -1;
}

int bgpstream_as_path_store_get_path_id(bgpstream_as_path_store_t *store,
                                        bgpstream_as_path_t *path,
                                        uint32_t peer_asn,
                                        bgpstream_as_path_store_path_id_t *id)
{
  /* shallow copy of the provided path, possibly with the peer segment removed
   */
  bgpstream_as_path_store_path_t findme;
  bgpstream_as_path_seg_t *seg;

  /* special case for empty path */
  if (path == NULL) {
    id->path_hash = UINT32_MAX;
    id->path_id = UINT16_MAX;
    return 0;
  }

  seg = (bgpstream_as_path_seg_t *)path->data;

  /* perform a shallow copy of the path, only extracting the core path if
     needed */
  if (path->data_len > 0 &&                     /* empty path */
      path->seg_cnt > 1 &&                      /* peer seg != origin seg */
      seg->type == BGPSTREAM_AS_PATH_SEG_ASN && /* simple ASN */
      ((bgpstream_as_path_seg_asn_t *)seg)->asn ==
        peer_asn) /* peer prepended */
  {
    /* just point to the core path */
    findme.path.data = path->data + sizeof(bgpstream_as_path_seg_asn_t);
    findme.path.data_len = path->data_len - sizeof(bgpstream_as_path_seg_asn_t);
    findme.path.data_alloc_len = UINT16_MAX;
    findme.path.seg_cnt = path->seg_cnt - 1;
    findme.path.origin_offset =
      path->origin_offset - sizeof(bgpstream_as_path_seg_asn_t);
    findme.is_core = 1;
  } else {
    /* empty path or no peer ASN */
    findme.path = *path;
    findme.is_core = 0;
  }

  return get_path_id(store, &findme, id);
}

int bgpstream_as_path_store_insert_path(bgpstream_as_path_store_t *store,
                                        uint8_t *path_data, uint16_t path_len,
                                        int is_core,
                                        bgpstream_as_path_store_path_id_t *id)
{
  bgpstream_as_path_store_path_t findme;
  findme.is_core = is_core;

  bgpstream_as_path_populate_from_data_zc(&findme.path, path_data, path_len);

  return get_path_id(store, &findme, id);
}

void bgpstream_as_path_store_iter_first_path(bgpstream_as_path_store_t *store)
{
  store->cur_pathset = kh_begin(store->path_set);

  while (!kh_exist(store->path_set, store->cur_pathset) &&
         store->cur_pathset < kh_end(store->path_set)) {
    store->cur_pathset++;
  }

  store->cur_path = 0;
}

void bgpstream_as_path_store_iter_next_path(bgpstream_as_path_store_t *store)
{
  pathset_t *pathset;

  if (store->cur_pathset >= kh_end(store->path_set)) {
    return;
  }

  pathset = &kh_val(store->path_set, store->cur_pathset);
  if (store->cur_path >= pathset->paths_cnt) {
    /* move to the next pathset */
    do {
      store->cur_pathset++;
    } while (store->cur_pathset < kh_end(store->path_set) &&
             !kh_exist(store->path_set, store->cur_pathset));
    store->cur_path = 0;
  }
}

int bgpstream_as_path_store_iter_has_more_path(bgpstream_as_path_store_t *store)
{
  return (store->cur_pathset < kh_end(store->path_set)) &&
         (store->cur_path <
          kh_val(store->path_set, store->cur_pathset).paths_cnt);
}

bgpstream_as_path_store_path_t *
bgpstream_as_path_store_iter_get_path(bgpstream_as_path_store_t *store)
{
  return &kh_val(store->path_set, store->cur_pathset).paths[store->cur_path++];
}

bgpstream_as_path_store_path_id_t
bgpstream_as_path_store_iter_get_path_id(bgpstream_as_path_store_t *store)
{
  bgpstream_as_path_store_path_id_t id;

  id.path_hash = kh_key(store->path_set, store->cur_pathset);
  id.path_id = store->cur_path;

  return id;
}

bgpstream_as_path_store_path_t *
bgpstream_as_path_store_get_store_path(bgpstream_as_path_store_t *store,
                                       bgpstream_as_path_store_path_id_t id)
{
  khiter_t k;

  /* special case for NULL path */
  if (id.path_hash == UINT32_MAX && id.path_id == UINT16_MAX) {
    return NULL;
  }

  if ((k = kh_get(pathset, store->path_set, id.path_hash)) ==
      kh_end(store->path_set)) {
    return NULL;
  }

  if (id.path_id > kh_val(store->path_set, k).paths_cnt - 1) {
    return NULL;
  }

  return &kh_val(store->path_set, k).paths[id.path_id];
}

bgpstream_as_path_t *bgpstream_as_path_store_path_get_path(
  bgpstream_as_path_store_path_t *store_path, uint32_t peer_asn)
{
  bgpstream_as_path_t *pc = NULL;

  if ((pc = bgpstream_as_path_create()) == NULL) {
    goto err;
  }

  /* if it is not a core path, just use the native copy */
  if (store_path->is_core == 0) {
    if (bgpstream_as_path_copy(pc, &store_path->path) != 0) {
      goto err;
    }
    return pc;
  }

  /* otherwise, do some manual copying */
  bgpstream_as_path_clear(pc);

  pc->data_alloc_len = pc->data_len =
    store_path->path.data_len + sizeof(bgpstream_as_path_seg_asn_t);
  if ((pc->data = malloc(pc->data_len)) == NULL) {
    goto err;
  }

  ((bgpstream_as_path_seg_asn_t *)pc->data)->type = BGPSTREAM_AS_PATH_SEG_ASN;
  ((bgpstream_as_path_seg_asn_t *)pc->data)->asn = peer_asn;

  memcpy(pc->data + sizeof(bgpstream_as_path_seg_asn_t), store_path->path.data,
         store_path->path.data_len);

  bgpstream_as_path_update_fields(pc);

  return pc;

err:
  bgpstream_as_path_destroy(pc);
  return NULL;
}

bgpstream_as_path_seg_t *bgpstream_as_path_store_path_get_origin_seg(
  bgpstream_as_path_store_path_t *store_path)
{
  return ((store_path == NULL) || store_path->path.data_len == 0)
           ? NULL
           : (bgpstream_as_path_seg_t *)(store_path->path.data +
                                         store_path->path.origin_offset);
}

void bgpstream_as_path_store_path_iter_reset(
  bgpstream_as_path_store_path_t *store_path,
  bgpstream_as_path_store_path_iter_t *iter, uint32_t peer_asn)
{
  iter->spath = store_path;

  if (store_path->is_core == 0) {
    bgpstream_as_path_iter_reset(&iter->pi);
  } else {
    /* return fake peer seg next */
    iter->pi.cur_offset = UINT16_MAX;
    iter->peerseg.type = BGPSTREAM_AS_PATH_SEG_ASN;
    iter->peerseg.asn = peer_asn;
  }
}

bgpstream_as_path_seg_t *bgpstream_as_path_store_path_get_next_seg(
  bgpstream_as_path_store_path_iter_t *iter)
{
  if (iter->pi.cur_offset == UINT16_MAX) {
    bgpstream_as_path_iter_reset(&iter->pi);
    return (bgpstream_as_path_seg_t *)&iter->peerseg;
  }
  return bgpstream_as_path_get_next_seg(&iter->spath->path, &iter->pi);
}

uint32_t
bgpstream_as_path_store_path_get_idx(bgpstream_as_path_store_path_t *store_path)
{
  return store_path->idx;
}

int bgpstream_as_path_store_path_is_core(
  bgpstream_as_path_store_path_t *store_path)
{
  return store_path->is_core;
}

bgpstream_as_path_t *bgpstream_as_path_store_path_get_int_path(
  bgpstream_as_path_store_path_t *store_path)
{
  return &store_path->path;
}
