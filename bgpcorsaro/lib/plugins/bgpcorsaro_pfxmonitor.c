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
#include "bgpcorsaro_int.h"
#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "wandio_utils.h"

#include "bgpcorsaro_io.h"
#include "bgpcorsaro_log.h"
#include "bgpcorsaro_plugin.h"

#include "bgpcorsaro_pfxmonitor.h"

#include "bgpstream_utils.h"
#include "khash.h"

/** @file
 *
 * @brief BGPCorsaro PfxMonitor plugin implementation
 *
 * @author Chiara Orsini
 *
 */

/** The number of output file pointers to support non-blocking close at the end
    of an interval. If the wandio buffers are large enough that it takes more
    than 1 interval to drain the buffers, consider increasing this number */
#define OUTFILE_POINTERS 2

/** The name of this plugin */
#define PLUGIN_NAME "pfxmonitor"

/** The version of this plugin */
#define PLUGIN_VERSION "0.1"

/** Default metric prefix */
#define PFXMONITOR_DEFAULT_METRIC_PFX "bgp"

/** Default IP space name */
#define PFXMONITOR_DEFAULT_IPSPACE_NAME "ip-space"

/** Minimum number of peer ASns to declare prefix visible */
#define PFXMONITOR_DEFAULT_PEER_ASNS_THRESHOLD 10

/** Maximum string length for the metric prefix */
#define PFXMONITOR_METRIC_PFX_LEN 256

/** Maximum string length for the log entries */
#define MAX_LOG_BUFFER_LEN 1024

#define DUMP_METRIC(value, time, fmt, ...)                                     \
  do {                                                                         \
    fprintf(stdout, fmt " %" PRIu32 " %" PRIu32 "\n", __VA_ARGS__, value,      \
            time);                                                             \
  } while (0)

/** Common plugin information across all instances */
static bgpcorsaro_plugin_t bgpcorsaro_pfxmonitor_plugin = {
  PLUGIN_NAME,                                            /* name */
  PLUGIN_VERSION,                                         /* version */
  BGPCORSARO_PLUGIN_ID_PFXMONITOR,                        /* id */
  BGPCORSARO_PLUGIN_GENERATE_PTRS(bgpcorsaro_pfxmonitor), /* func ptrs */
  BGPCORSARO_PLUGIN_GENERATE_TAIL,
};

/* Transform a string to be safe for use with Graphite
   (http://graphite.readthedocs.org/en/latest/) */
static char *graphite_safe(char *p)
{
  if (p == NULL) {
    return p;
  }

  char *r = p;
  while (*p != '\0') {
    if (*p == '.') {
      *p = '-';
    }
    if (*p == '*') {
      *p = '-';
    }
    p++;
  }
  return r;
}

/** A map that contains, for each origin the number of peer ASns observing it */
KHASH_INIT(asn_count_map, uint32_t, uint32_t, 1, kh_int_hash_func,
           kh_int_hash_equal);

/** A map that contains the origin observed by a specific peer */
KHASH_INIT(peer_asn_map, uint32_t, uint32_t, 1, kh_int_hash_func,
           kh_int_hash_equal);

/** A map that contains information for each prefix */
KHASH_INIT(pfx_info_map, bgpstream_pfx_storage_t, khash_t(peer_asn_map) *, 1,
           bgpstream_pfx_storage_hash_val, bgpstream_pfx_storage_equal_val);

/** Holds the state for an instance of this plugin */
struct bgpcorsaro_pfxmonitor_state_t {

  /** The outfile for the plugin */
  iow_t *outfile;
  /** A set of pointers to outfiles to support non-blocking close */
  iow_t *outfile_p[OUTFILE_POINTERS];
  /** The current outfile */
  int outfile_n;

  /** Time at which the current interval started */
  uint32_t interval_start;

  /** Prefixes of interest */
  bgpstream_ip_counter_t *poi;

  /** Cache of prefixes (it speeds up the overlapping check) */
  bgpstream_pfx_storage_set_t *overlapping_pfx_cache;
  bgpstream_pfx_storage_set_t *non_overlapping_pfx_cache;

  /** Peers' ASNs */
  bgpstream_id_set_t *peer_asns;

  /** Prefix info map */
  khash_t(pfx_info_map) * pfx_info;

  /** utility set to compute unique origin
   *  ASns  */
  bgpstream_id_set_t *unique_origins;

  /* Peer threshold - minimum number of
   * peers' ASns to declare prefix visible */
  uint32_t peer_asns_th;

  /* If 1 we consider only more specific prefixes */
  uint8_t more_specific;

  /** Metric prefix */
  char metric_prefix[PFXMONITOR_METRIC_PFX_LEN];

  /** IP space name */
  char ip_space_name[PFXMONITOR_METRIC_PFX_LEN];
};

/** Extends the generic plugin state convenience macro in bgpcorsaro_plugin.h */
#define STATE(bgpcorsaro)                                                      \
  (BGPCORSARO_PLUGIN_STATE(bgpcorsaro, pfxmonitor,                             \
                           BGPCORSARO_PLUGIN_ID_PFXMONITOR))

/** Extends the generic plugin plugin convenience macro in bgpcorsaro_plugin.h
 */
#define PLUGIN(bgpcorsaro)                                                     \
  (BGPCORSARO_PLUGIN_PLUGIN(bgpcorsaro, BGPCORSARO_PLUGIN_ID_PFXMONITOR))

static int output_stats_and_reset(struct bgpcorsaro_pfxmonitor_state_t *state,
                                  uint32_t interval_start)
{
  khiter_t k;
  khiter_t p;
  khiter_t a;
  int khret;
  uint8_t pfx_visible;
  uint32_t unique_pfxs = 0;
  khash_t(peer_asn_map) * pam;
  /* origin_asn -> num peer ASns*/
  khash_t(asn_count_map) *asn_np = NULL;

  if ((asn_np = kh_init(asn_count_map)) == NULL) {
    return -1;
  }

  /* for each prefix go through all peers */
  for (k = kh_begin(state->pfx_info); k != kh_end(state->pfx_info); ++k) {
    if (kh_exist(state->pfx_info, k) == 0) {
      continue;
    }
    /* reset counters */
    kh_clear(asn_count_map, asn_np);

    /* get peer-asn map for this prefix */
    pam = kh_value(state->pfx_info, k);

    /* save the origin asn visibility (i.e. how many peers' ASns
     * observe such information */

    /* for each peer, go through all origins */
    for (p = kh_begin(pam); p != kh_end(pam); ++p) {
      if (kh_exist(pam, p) == 0) {
        continue;
      }
      /* increment the counter for this ASN */
      if ((a = kh_get(asn_count_map, asn_np, kh_value(pam, p))) ==
          kh_end(asn_np)) {
        a = kh_put(asn_count_map, asn_np, kh_value(pam, p), &khret);
        kh_value(asn_np, a) = 1;
      } else {
        kh_value(asn_np, a)++;
      }
    }

    /* now asn_np has a complete count of the number of peers' ASns that
       observed
       each origin ASN */

    /* count the prefix and origins if their visibility
     * is above the threshold */
    pfx_visible = 0;
    for (a = kh_begin(asn_np); a != kh_end(asn_np); ++a) {
      if (kh_exist(asn_np, a) == 0) {
        continue;
      }
      /* the information is accounted only if it is
       * consistent on at least threshold peers' ASns */
      if (kh_value(asn_np, a) >= state->peer_asns_th) {
        pfx_visible = 1;
        bgpstream_id_set_insert(state->unique_origins, kh_key(asn_np, a));
      }
    }

    /* updating counters */
    unique_pfxs += pfx_visible;
  }

  DUMP_METRIC(unique_pfxs, state->interval_start, "%s.%s.%s.%s",
              state->metric_prefix, PLUGIN_NAME, state->ip_space_name,
              "prefixes_cnt");

  DUMP_METRIC(bgpstream_id_set_size(state->unique_origins),
              state->interval_start, "%s.%s.%s.%s", state->metric_prefix,
              PLUGIN_NAME, state->ip_space_name, "origin_ASns_cnt");

  bgpstream_id_set_clear(state->unique_origins);
  kh_destroy(asn_count_map, asn_np);

  return 0;
}

/* set the origin ASN for a given prefix as observed by a given peer */
static int set_pfx_peer_origin(struct bgpcorsaro_pfxmonitor_state_t *state,
                               const bgpstream_pfx_storage_t *pfx,
                               uint32_t peer_asn, uint32_t origin_asn)
{
  khiter_t k;
  int khret;
  khash_t(peer_asn_map) * pam;

  /* does the prefix already exist, if not, create it */
  if ((k = kh_get(pfx_info_map, state->pfx_info, *pfx)) ==
      kh_end(state->pfx_info)) {
    /* no, insert it */
    k = kh_put(pfx_info_map, state->pfx_info, *pfx, &khret);

    /* create a peer-asn map */
    if ((pam = kh_init(peer_asn_map)) == NULL) {
      return -1;
    }
    kh_value(state->pfx_info, k) = pam;
  } else {
    pam = kh_value(state->pfx_info, k);
  }

  /* does the peer exist? ({pfx}{peer}), if not, create it */
  if ((k = kh_get(peer_asn_map, pam, peer_asn)) == kh_end(pam)) {
    k = kh_put(peer_asn_map, pam, peer_asn, &khret);
  }

  /* set the origin ASN for this pfx/peer combo */
  kh_value(pam, k) = origin_asn;
  return 0;
}

/* remove pfx/peer from state */
static void rm_pfx_peer(struct bgpcorsaro_pfxmonitor_state_t *state,
                        const bgpstream_pfx_storage_t *pfx, uint32_t peer_asn)
{
  khiter_t k;
  khash_t(peer_asn_map) * pam;

  /* does the pfx exist? */
  if ((k = kh_get(pfx_info_map, state->pfx_info, *pfx)) ==
      kh_end(state->pfx_info)) {
    return;
  }
  pam = kh_value(state->pfx_info, k);

  /* does this peer exist for this pfx */
  if ((k = kh_get(peer_asn_map, pam, peer_asn)) == kh_end(pam)) {
    return;
  }

  kh_del(peer_asn_map, pam, k);

  return;
}

static int process_overlapping_pfx(struct bgpcorsaro_pfxmonitor_state_t *state,
                                   const bgpstream_record_t *bs_record,
                                   const bgpstream_elem_t *elem)
{
  char log_buffer[MAX_LOG_BUFFER_LEN] = "";
  bgpstream_as_path_seg_t *origin_seg = NULL;
  uint32_t origin_asn = 0;

  if (elem->type == BGPSTREAM_ELEM_TYPE_WITHDRAWAL) {
    /* remove pfx/peer from state structure */
    rm_pfx_peer(state, &elem->prefix, elem->peer_asnumber);
  } else /* (announcement or rib) */
  {
    /* get the origin asn (sets and confederations are ignored) */
    origin_seg = bgpstream_as_path_get_origin_seg(elem->aspath);
    if (origin_seg == NULL || origin_seg->type != BGPSTREAM_AS_PATH_SEG_ASN) {
      fprintf(stderr, "WARN: ignoring AS sets and confederations\n");
    } else {
      /* valid origin ASN */
      origin_asn = ((bgpstream_as_path_seg_asn_t *)origin_seg)->asn;
      if (set_pfx_peer_origin(state, &elem->prefix, elem->peer_asnumber,
                              origin_asn) != 0) {
        return -1;
      }
    }
  }

  /* we always print all the info to the log */
  bgpstream_record_elem_snprintf(log_buffer, MAX_LOG_BUFFER_LEN, bs_record,
                                 elem);
  wandio_printf(state->outfile, "%s\n", log_buffer);
  return 0;
}

static int add_prefixes_from_file(bgpstream_ip_counter_t *poi,
                                  char *pfx_file_string)
{

  char pfx_line[MAX_LOG_BUFFER_LEN];
  io_t *file = NULL;

  bgpstream_pfx_storage_t pfx_st;

  if (pfx_file_string == NULL) {
    return -1;
  }

  if ((file = wandio_create(pfx_file_string)) == NULL) {
    fprintf(stderr, "ERROR: Could not open prefix file (%s)\n",
            pfx_file_string);
    return -1;
  }

  while (wandio_fgets(file, &pfx_line, MAX_LOG_BUFFER_LEN, 1) > 0) {
    /* treat # as comment line, and ignore empty lines */
    if (pfx_line[0] == '#' || pfx_line[0] == '\0') {
      continue;
    }

    if (bgpstream_str2pfx(pfx_line, &pfx_st) == NULL ||
        bgpstream_ip_counter_add(poi, (bgpstream_pfx_t *)&pfx_st) != 0) {
      /* failed to parse/insert the prefix */
      return -1;
    }
  }
  wandio_destroy(file);
  return 0;
}

/** Print usage information to stderr */
static void usage(bgpcorsaro_plugin_t *plugin)
{
  fprintf(
    stderr,
    "plugin usage: %s -l <pfx> \n"
    "       -m <prefix>        metric prefix (default: %s)\n"
    "       -l <prefix>        prefix to monitor*\n"
    "       -L <prefix-file>   read the prefixes to monitor from file*\n"
    "       -M                 consider only more specifics (default: false)\n"
    "       -n <peer_cnt>   minimum number of unique peers' ASNs to declare "
    "prefix visible (default: %d)\n"
    "       -i <name>          IP space name (default: %s)\n"
    "* denotes an option that can be given multiple times\n",
    plugin->argv[0], PFXMONITOR_DEFAULT_METRIC_PFX,
    PFXMONITOR_DEFAULT_PEER_ASNS_THRESHOLD, PFXMONITOR_DEFAULT_IPSPACE_NAME);
}

/** Parse the arguments given to the plugin */
static int parse_args(bgpcorsaro_t *bgpcorsaro)
{
  bgpcorsaro_plugin_t *plugin = PLUGIN(bgpcorsaro);
  struct bgpcorsaro_pfxmonitor_state_t *state = STATE(bgpcorsaro);
  int opt;
  bgpstream_pfx_storage_t pfx_st;

  if (plugin->argc <= 0) {
    return 0;
  }

  /* NB: remember to reset optind to 1 before using getopt! */
  optind = 1;

  while ((opt = getopt(plugin->argc, plugin->argv, ":l:L:m:n:i:M?")) >= 0) {
    switch (opt) {
    case 'm':
      if (optarg != NULL && strlen(optarg) - 1 <= PFXMONITOR_METRIC_PFX_LEN) {
        strncpy(state->metric_prefix, optarg, PFXMONITOR_METRIC_PFX_LEN);
      } else {
        fprintf(stderr, "Error: could not set metric prefix\n");
        usage(plugin);
        return -1;
      }
      break;

    case 'i':
      if (optarg != NULL && strlen(optarg) - 1 <= PFXMONITOR_METRIC_PFX_LEN) {
        strncpy(state->ip_space_name, optarg, PFXMONITOR_METRIC_PFX_LEN);
      } else {
        fprintf(stderr, "Error: could not set IP space name\n");
        usage(plugin);
        return -1;
      }
      break;

    case 'l':
      if (bgpstream_str2pfx(optarg, &pfx_st) == NULL) {
        fprintf(stderr, "Error: Could not parse prefix (%s)\n", optarg);
        usage(plugin);
        return -1;
      }
      bgpstream_ip_counter_add(state->poi, (bgpstream_pfx_t *)&pfx_st);
      break;

    case 'L':
      if (add_prefixes_from_file(state->poi, optarg) != 0) {
        return -1;
      }
      break;

    case 'M':
      state->more_specific = 1;
      break;

    case 'n':
      state->peer_asns_th = atoi(optarg);
      break;

    case '?':
    case ':':
    default:
      usage(plugin);
      return -1;
    }
  }

  /* if no prefixes were provided,  */
  if (bgpstream_ip_counter_get_ipcount(state->poi,
                                       BGPSTREAM_ADDR_VERSION_IPV4) == 0 &&
      bgpstream_ip_counter_get_ipcount(state->poi,
                                       BGPSTREAM_ADDR_VERSION_IPV6) == 0) {
    fprintf(stderr, "Error: no valid prefixes provided\n");
    usage(plugin);
    return -1;
  }

  return 0;
}

/* == PUBLIC PLUGIN FUNCS BELOW HERE == */

/** Implements the alloc function of the plugin API */
bgpcorsaro_plugin_t *bgpcorsaro_pfxmonitor_alloc(bgpcorsaro_t *bgpcorsaro)
{
  return &bgpcorsaro_pfxmonitor_plugin;
}

/** Implements the init_output function of the plugin API */
int bgpcorsaro_pfxmonitor_init_output(bgpcorsaro_t *bgpcorsaro)
{
  struct bgpcorsaro_pfxmonitor_state_t *state;
  bgpcorsaro_plugin_t *plugin = PLUGIN(bgpcorsaro);
  assert(plugin != NULL);

  if ((state = malloc_zero(sizeof(struct bgpcorsaro_pfxmonitor_state_t))) ==
      NULL) {
    bgpcorsaro_log(__func__, bgpcorsaro,
                   "could not malloc bgpcorsaro_pfxmonitor_state_t");
    goto err;
  }
  bgpcorsaro_plugin_register_state(bgpcorsaro->plugin_manager, plugin, state);

  /* initialize state with default values */
  state = STATE(bgpcorsaro);
  strncpy(state->metric_prefix, PFXMONITOR_DEFAULT_METRIC_PFX,
          PFXMONITOR_METRIC_PFX_LEN);
  strncpy(state->ip_space_name, PFXMONITOR_DEFAULT_IPSPACE_NAME,
          PFXMONITOR_METRIC_PFX_LEN);

  if ((state->poi = bgpstream_ip_counter_create()) == NULL) {
    goto err;
  }
  state->peer_asns_th = PFXMONITOR_DEFAULT_PEER_ASNS_THRESHOLD;
  state->more_specific = 0;

  /* parse the arguments */
  if (parse_args(bgpcorsaro) != 0) {
    goto err;
  }

  graphite_safe(state->metric_prefix);
  graphite_safe(state->ip_space_name);

  /* create all the sets and maps we need */
  if ((state->overlapping_pfx_cache = bgpstream_pfx_storage_set_create()) ==
        NULL ||
      (state->non_overlapping_pfx_cache = bgpstream_pfx_storage_set_create()) ==
        NULL ||
      (state->pfx_info = kh_init(pfx_info_map)) == NULL ||
      (state->unique_origins = bgpstream_id_set_create()) == NULL ||
      (state->peer_asns = bgpstream_id_set_create()) == NULL) {
    goto err;
  }

  /* defer opening the output file until we start the first interval */

  return 0;

err:
  bgpcorsaro_pfxmonitor_close_output(bgpcorsaro);
  return -1;
}

/** Implements the close_output function of the plugin API */
int bgpcorsaro_pfxmonitor_close_output(bgpcorsaro_t *bgpcorsaro)
{
  int i;
  struct bgpcorsaro_pfxmonitor_state_t *state = STATE(bgpcorsaro);
  khiter_t k;
  khash_t(peer_asn_map) * v;

  if (state == NULL) {
    return 0;
  }

  /* close all the outfile pointers */
  for (i = 0; i < OUTFILE_POINTERS; i++) {
    if (state->outfile_p[i] != NULL) {
      wandio_wdestroy(state->outfile_p[i]);
      state->outfile_p[i] = NULL;
    }
  }
  state->outfile = NULL;
  if (state->poi != NULL) {
    bgpstream_ip_counter_destroy(state->poi);
    state->poi = NULL;
  }

  /* deallocate the dynamic memory in use */
  if (state->overlapping_pfx_cache != NULL) {
    bgpstream_pfx_storage_set_destroy(state->overlapping_pfx_cache);
    state->overlapping_pfx_cache = NULL;
  }

  if (state->non_overlapping_pfx_cache != NULL) {
    bgpstream_pfx_storage_set_destroy(state->non_overlapping_pfx_cache);
    state->non_overlapping_pfx_cache = NULL;
  }

  if (state->peer_asns != NULL) {
    bgpstream_id_set_destroy(state->peer_asns);
    state->peer_asns = NULL;
  }

  if (state->pfx_info != NULL) {
    for (k = kh_begin(state->pfx_info); k != kh_end(state->pfx_info); ++k) {
      if (kh_exist(state->pfx_info, k)) {
        v = kh_val(state->pfx_info, k);
        kh_destroy(peer_asn_map, v);
      }
    }
    kh_destroy(pfx_info_map, state->pfx_info);
    state->pfx_info = NULL;
  }

  if (state->unique_origins != NULL) {
    bgpstream_id_set_destroy(state->unique_origins);
    state->unique_origins = NULL;
  }

  bgpcorsaro_plugin_free_state(bgpcorsaro->plugin_manager, PLUGIN(bgpcorsaro));
  return 0;
}

/** Implements the start_interval function of the plugin API */
int bgpcorsaro_pfxmonitor_start_interval(bgpcorsaro_t *bgpcorsaro,
                                         bgpcorsaro_interval_t *int_start)
{
  struct bgpcorsaro_pfxmonitor_state_t *state = STATE(bgpcorsaro);

  /* open an output file */
  if (state->outfile == NULL) {
    if ((state->outfile_p[state->outfile_n] = bgpcorsaro_io_prepare_file(
           bgpcorsaro, PLUGIN(bgpcorsaro)->name, int_start)) == NULL) {
      bgpcorsaro_log(__func__, bgpcorsaro, "could not open %s output file",
                     PLUGIN(bgpcorsaro)->name);
      return -1;
    }
    state->outfile = state->outfile_p[state->outfile_n];
  }

  bgpcorsaro_io_write_interval_start(bgpcorsaro, state->outfile, int_start);

  /* save the interval start to correctly output the time series values */
  state->interval_start = int_start->time;

  return 0;
}

/** Implements the end_interval function of the plugin API */
int bgpcorsaro_pfxmonitor_end_interval(bgpcorsaro_t *bgpcorsaro,
                                       bgpcorsaro_interval_t *int_end)
{
  struct bgpcorsaro_pfxmonitor_state_t *state = STATE(bgpcorsaro);

  bgpcorsaro_io_write_interval_end(bgpcorsaro, state->outfile, int_end);

  /* if we are rotating, now is when we should do it */
  if (bgpcorsaro_is_rotate_interval(bgpcorsaro)) {
    /* leave the current file to finish draining buffers */
    assert(state->outfile != NULL);

    /* move on to the next output pointer */
    state->outfile_n = (state->outfile_n + 1) % OUTFILE_POINTERS;

    if (state->outfile_p[state->outfile_n] != NULL) {
      /* we're gonna have to wait for this to close */
      wandio_wdestroy(state->outfile_p[state->outfile_n]);
      state->outfile_p[state->outfile_n] = NULL;
    }

    state->outfile = NULL;
  }

  output_stats_and_reset(state, state->interval_start);

  return 0;
}

/** Implements the process_record function of the plugin API */
int bgpcorsaro_pfxmonitor_process_record(bgpcorsaro_t *bgpcorsaro,
                                         bgpcorsaro_record_t *record)
{
  struct bgpcorsaro_pfxmonitor_state_t *state = STATE(bgpcorsaro);
  bgpstream_record_t *bs_record = BS_REC(record);
  bgpstream_elem_t *elem;
  uint64_t overlap;
  uint8_t more_specific;

  /* no point carrying on if a previous plugin has already decided we should
     ignore this record */
  if ((record->state.flags & BGPCORSARO_RECORD_STATE_FLAG_IGNORE) != 0) {
    return 0;
  }

  /* consider only valid records */
  if (bs_record->status != BGPSTREAM_RECORD_STATUS_VALID_RECORD) {
    return 0;
  }

  /* process all elems in the record */
  while ((elem = bgpstream_record_get_next_elem(bs_record)) != NULL) {
    if (elem->type != BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT &&
        elem->type != BGPSTREAM_ELEM_TYPE_WITHDRAWAL &&
        elem->type != BGPSTREAM_ELEM_TYPE_RIB) {
      continue;
    }

    /* consider only prefixes announced by the peers (not the collector) */
    if (elem->type != BGPSTREAM_ELEM_TYPE_WITHDRAWAL &&
        bgpstream_as_path_get_len(elem->aspath) == 0) {
      continue;
    }

    /* consider only prefixes that overlap with the set provided */

    /* first, check the non overlapping pfxs cache */
    if (bgpstream_pfx_storage_set_exists(state->non_overlapping_pfx_cache,
                                         &elem->prefix) != 0) {
      /* confirmed as non-overlapping */
      continue;
    }

    /* check the overlapping pfxs cache */
    if (bgpstream_pfx_storage_set_exists(state->overlapping_pfx_cache,
                                         &elem->prefix) == 0) {
      /* not cached, but could still be overlapping, need to check */
      more_specific = 0;
      overlap = bgpstream_ip_counter_is_overlapping(
        state->poi, (bgpstream_pfx_t *)&elem->prefix, &more_specific);

      if (overlap > 0 && (!state->more_specific || more_specific)) {
        bgpstream_pfx_storage_set_insert(state->overlapping_pfx_cache,
                                         &elem->prefix);
        /* overlapping */
      } else {
        bgpstream_pfx_storage_set_insert(state->non_overlapping_pfx_cache,
                                         &elem->prefix);
        /* non-overlapping */
        continue;
      }
    }

    /* by here, confirmed as overlapping, process the elem */
    process_overlapping_pfx(state, bs_record, elem);
  }

  return 0;
}
