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

#include "bgpcorsaro_asmonitor.h"

#include "bgpstream_utils.h"
#include "khash.h"

/** @file
 *
 * @brief BGPCorsaro PfxMonitor plugin implementation
 *
 * @author Chiara Orsini
 *
 */

/* static char tmp_buffer[1024]; */

/** The number of output file pointers to support non-blocking close at the end
    of an interval. If the wandio buffers are large enough that it takes more
    than 1 interval to drain the buffers, consider increasing this number */
#define OUTFILE_POINTERS 2

/** The name of this plugin */
#define PLUGIN_NAME "asmonitor"

/** The version of this plugin */
#define PLUGIN_VERSION "0.1"

/** Default metric prefix */
#define ASMONITOR_DEFAULT_METRIC_PFX "bgp"

/** Default IP space name */
#define ASMONITOR_DEFAULT_IPSPACE_NAME "ip-space"

/** Minimum number of peer ASns to declare prefix visible */
#define ASMONITOR_DEFAULT_PEER_ASNS_THRESHOLD 10

/** How long it takes for a prefix to be discarded from monitoring (if not
 * announced) */
#define ASMONITOR_DEFAULT_PREFIX_WINDOW 3600 * 24

/** Maximum string length for the metric prefix */
#define ASMONITOR_METRIC_PFX_LEN 256

/** Maximum string length for the log entries */
#define MAX_LOG_BUFFER_LEN 1024

#define DUMP_METRIC(value, time, fmt, ...)                                     \
  do {                                                                         \
    fprintf(stdout, fmt " %" PRIu32 " %" PRIu32 "\n", __VA_ARGS__, value,      \
            time);                                                             \
  } while (0)

/** Common plugin information across all instances */
static bgpcorsaro_plugin_t bgpcorsaro_asmonitor_plugin = {
  PLUGIN_NAME,                                           /* name */
  PLUGIN_VERSION,                                        /* version */
  BGPCORSARO_PLUGIN_ID_ASMONITOR,                        /* id */
  BGPCORSARO_PLUGIN_GENERATE_PTRS(bgpcorsaro_asmonitor), /* func ptrs */
  BGPCORSARO_PLUGIN_GENERATE_TAIL,
};

/** Data structure associated with each prefix
 *  in the patricia tree (attached to the user
 *  pointer */
typedef struct perpfx_info {

  /** last ts the prefix was observed */
  uint32_t last_observed;

} perpfx_info_t;

/** A set that contains a unique set of path segments */
KHASH_INIT(path_segments, bgpstream_as_path_seg_t *, char, 0,
           bgpstream_as_path_seg_hash, bgpstream_as_path_seg_equal);
typedef khash_t(path_segments) path_segments_t;

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
struct bgpcorsaro_asmonitor_state_t {

  /** The outfile for the plugin */
  iow_t *outfile;
  /** A set of pointers to outfiles to support non-blocking close */
  iow_t *outfile_p[OUTFILE_POINTERS];
  /** The current outfile */
  int outfile_n;

  /** Time at which the current interval started */
  uint32_t interval_start;

  /** ASes of interest */
  bgpstream_id_set_t *monitored_ases;

  /** Prefixes of interest */
  bgpstream_patricia_tree_t *patricia;

  /* Prefix window (explain) */
  uint32_t pfx_window;

  /** Peers' ASNs */
  bgpstream_id_set_t *peer_asns;

  /** Prefix info map */
  khash_t(pfx_info_map) * pfx_info;

  /** origins' set to compute unique origin
   *  ASns  */
  bgpstream_id_set_t *unique_origins[BGPSTREAM_MAX_IP_VERSION_IDX];

  /* Peer threshold - minimum number of
   * peers' ASns to declare prefix visible */
  uint32_t peer_asns_th;

  /* If 1 we consider only more specific prefixes */
  uint8_t more_specific;

  /** Metric prefix */
  char metric_prefix[ASMONITOR_METRIC_PFX_LEN];

  /** IP space name */
  char ip_space_name[ASMONITOR_METRIC_PFX_LEN];
};

/** Extends the generic plugin state convenience macro in bgpcorsaro_plugin.h */
#define STATE(bgpcorsaro)                                                      \
  (BGPCORSARO_PLUGIN_STATE(bgpcorsaro, asmonitor,                              \
                           BGPCORSARO_PLUGIN_ID_ASMONITOR))

/** Extends the generic plugin plugin convenience macro in bgpcorsaro_plugin.h
 */
#define PLUGIN(bgpcorsaro)                                                     \
  (BGPCORSARO_PLUGIN_PLUGIN(bgpcorsaro, BGPCORSARO_PLUGIN_ID_ASMONITOR))

/* ================ per prefix info management ================ */

/** Create a per pfx info structure */
static perpfx_info_t *perpfx_info_create(uint32_t ts)
{
  perpfx_info_t *ppi;
  if ((ppi = (perpfx_info_t *)malloc_zero(sizeof(perpfx_info_t))) == NULL) {
    fprintf(stderr, "Warning could not alloc perpfx_info_t\n");
    return NULL;
  }
  ppi->last_observed = ts;
  return ppi;
}

/** Set the timestamp in the perpfx info structure */
static void perpfx_info_set_ts(perpfx_info_t *ppi, uint32_t ts)
{
  assert(ppi);
  ppi->last_observed = ts;
}

/** Destroy the perpfx info structure */
static void perpfx_info_destroy(void *ppi)
{
  if (ppi != NULL) {
    free((perpfx_info_t *)ppi);
  }
}

void remove_old_prefixes(bgpstream_patricia_tree_t *pt,
                         bgpstream_patricia_node_t *node, void *data)
{
  uint32_t *last_valid_ts = (uint32_t *)data;
  perpfx_info_t *info = (perpfx_info_t *)bgpstream_patricia_tree_get_user(node);
  if (info != NULL) {
    if (info->last_observed < *last_valid_ts) {
      bgpstream_patricia_tree_remove_node(pt, node);
    }
  }
}

/* ================ output stats  ================ */

static int output_stats_and_reset(bgpcorsaro_t *bgpcorsaro)
{
  struct bgpcorsaro_asmonitor_state_t *state = STATE(bgpcorsaro);

  khiter_t k;
  khiter_t p;
  khiter_t a;
  int khret;
  uint8_t pfx_visible;
  int v;
  uint32_t unique_pfxs[BGPSTREAM_MAX_IP_VERSION_IDX];
  uint32_t overlapping_pfxs[BGPSTREAM_MAX_IP_VERSION_IDX];
  khash_t(peer_asn_map) * pam;
  /* origin_asn -> num peer ASns*/
  khash_t(asn_count_map) *asn_np = NULL;

  if ((asn_np = kh_init(asn_count_map)) == NULL) {
    return -1;
  }

  /* init counters */
  for (v = 0; v < BGPSTREAM_MAX_IP_VERSION_IDX; v++) {
    unique_pfxs[v] =
      bgpstream_patricia_prefix_count(state->patricia, bgpstream_idx2ipv(v));
    overlapping_pfxs[v] = 0;
  }

  /* for each prefix go through all peers */
  for (k = kh_begin(state->pfx_info); k != kh_end(state->pfx_info); ++k) {
    if (kh_exist(state->pfx_info, k) == 0) {
      continue;
    }
    /* reset counters */
    kh_clear(asn_count_map, asn_np);

    /* get prefix version index */
    v = bgpstream_ipv2idx(kh_key(state->pfx_info, k).address.version);

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

        bgpstream_id_set_insert(state->unique_origins[v], kh_key(asn_np, a));
      }
    }

    /* updating counters */
    overlapping_pfxs[v] += pfx_visible;
  }

  for (v = 0; v < BGPSTREAM_MAX_IP_VERSION_IDX; v++) {
    DUMP_METRIC(unique_pfxs[v], state->interval_start, "%s.%s.%s.v%d.%s",
                state->metric_prefix, PLUGIN_NAME, state->ip_space_name,
                bgpstream_idx2number(v), "prefixes_cnt");
    DUMP_METRIC(overlapping_pfxs[v], state->interval_start, "%s.%s.%s.v%d.%s",
                state->metric_prefix, PLUGIN_NAME, state->ip_space_name,
                bgpstream_idx2number(v), "overlapping_prefixes_cnt");
    DUMP_METRIC(bgpstream_id_set_size(state->unique_origins[v]),
                state->interval_start, "%s.%s.%s.v%d.%s", state->metric_prefix,
                PLUGIN_NAME, state->ip_space_name, bgpstream_idx2number(v),
                "origin_ASN_cnt");
  }

  /* resetting data structures and removing from the patricia
   * tree prefixes that are older than a given window */
  uint32_t last_valid_ts = 0;
  if (state->interval_start > state->pfx_window) {
    last_valid_ts = state->interval_start - state->pfx_window;
  }
  bgpstream_patricia_tree_walk(state->patricia, remove_old_prefixes,
                               (void *)&last_valid_ts);

  /* now we check again all the stored prefixes and remove those
   * that do not overlap anymore */
  uint8_t overlap;

  for (k = kh_begin(state->pfx_info); k != kh_end(state->pfx_info); ++k) {
    if (kh_exist(state->pfx_info, k)) {
      overlap = bgpstream_patricia_tree_get_pfx_overlap_info(
        state->patricia, (bgpstream_pfx_t *)&kh_key(state->pfx_info, k));
      if (overlap == 0 || (state->more_specific &&
                           !(overlap & (BGPSTREAM_PATRICIA_LESS_SPECIFICS |
                                        BGPSTREAM_PATRICIA_EXACT_MATCH)))) {
        kh_destroy(peer_asn_map, kh_val(state->pfx_info, k));
        kh_del(pfx_info_map, state->pfx_info, k);
      }
    }
  }

  for (v = 0; v < BGPSTREAM_MAX_IP_VERSION_IDX; v++) {
    bgpstream_id_set_clear(state->unique_origins[v]);
  }
  kh_destroy(asn_count_map, asn_np);

  return 0;
}

/* set the origin ASN for a given prefix as observed by a given peer */
static int set_pfx_peer_origin(struct bgpcorsaro_asmonitor_state_t *state,
                               const bgpstream_pfx_storage_t *pfx,
                               uint32_t peer_asn,
                               bgpstream_as_path_seg_t *origin_seg)
{
  khiter_t k;
  int khret;
  khash_t(peer_asn_map) * pam;

  if (origin_seg == NULL || origin_seg->type != BGPSTREAM_AS_PATH_SEG_ASN) {
    fprintf(stderr, "WARN: ignoring AS sets and confederations\n");
    return 0;
  }
  /* simple origin ASN */
  uint32_t origin_asn = ((bgpstream_as_path_seg_asn_t *)origin_seg)->asn;

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
static void rm_pfx_peer(struct bgpcorsaro_asmonitor_state_t *state,
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

  /* deallocate memory for segment */
  kh_del(peer_asn_map, pam, k);

  return;
}

static int process_overlapping_pfx(struct bgpcorsaro_asmonitor_state_t *state,
                                   const bgpstream_record_t *bs_record,
                                   const bgpstream_elem_t *elem)
{
  char log_buffer[MAX_LOG_BUFFER_LEN] = "";

  if (elem->type == BGPSTREAM_ELEM_TYPE_WITHDRAWAL) {
    /* remove pfx/peer from state structure */
    rm_pfx_peer(state, &elem->prefix, elem->peer_asnumber);
  } else /* (announcement or rib) */
  {
    /* get the origin asn segment and update the data structure */
    if (set_pfx_peer_origin(state, &elem->prefix, elem->peer_asnumber,
                            bgpstream_as_path_get_origin_seg(elem->aspath)) !=
        0) {
      return -1;
    }
  }

  /* we always print all the info to the log */
  bgpstream_record_elem_snprintf(log_buffer, MAX_LOG_BUFFER_LEN, bs_record,
                                 elem);
  wandio_printf(state->outfile, "%s\n", log_buffer);
  return 0;
}

static int add_asns_from_file(bgpstream_id_set_t *monitored_ases,
                              char *as_file_string)
{

  char as_line[MAX_LOG_BUFFER_LEN];
  io_t *file = NULL;
  uint32_t monitor_asn;

  if (as_file_string == NULL) {
    return -1;
  }

  if ((file = wandio_create(as_file_string)) == NULL) {
    fprintf(stderr, "ERROR: Could not open ASns file (%s)\n", as_file_string);
    return -1;
  }

  while (wandio_fgets(file, &as_line, MAX_LOG_BUFFER_LEN, 1) > 0) {
    /* treat # as comment line, and ignore empty lines */
    if (as_line[0] == '#' || as_line[0] == '\0') {
      continue;
    }
    monitor_asn = strtoul(as_line, NULL, 10);
    bgpstream_id_set_insert(monitored_ases, monitor_asn);
  }
  wandio_destroy(file);
  return 0;
}

/* ================ command line args management ================ */

/** Print usage information to stderr */
static void usage(bgpcorsaro_plugin_t *plugin)
{
  fprintf(
    stderr,
    "plugin usage: %s -a <asn> [options] \n"
    "       -m <prefix>        metric prefix (default: %s)\n"
    "       -a <asn>           ASn to monitor*\n"
    "       -A <asns-file>     read the ASn to monitor from file*\n"
    "       -M                 consider only more specifics (default: false)\n"
    "       -w <pfx-window>    how long a prefix is to be considered valid for "
    "monitoring purposes (default: %d s)\n"
    "       -n <peer_cnt>      minimum number of unique peers' ASNs to declare "
    "prefix visible (default: %d)\n"
    "       -i <name>          IP space name (default: %s)\n"
    "* denotes an option that can be given multiple times\n",
    plugin->argv[0], ASMONITOR_DEFAULT_METRIC_PFX,
    ASMONITOR_DEFAULT_PREFIX_WINDOW, ASMONITOR_DEFAULT_PEER_ASNS_THRESHOLD,
    ASMONITOR_DEFAULT_IPSPACE_NAME);
}

/** Parse the arguments given to the plugin */
static int parse_args(bgpcorsaro_t *bgpcorsaro)
{
  bgpcorsaro_plugin_t *plugin = PLUGIN(bgpcorsaro);
  struct bgpcorsaro_asmonitor_state_t *state = STATE(bgpcorsaro);
  int opt;
  uint32_t monitor_asn;

  if (plugin->argc <= 0) {
    return 0;
  }

  /* NB: remember to reset optind to 1 before using getopt! */
  optind = 1;

  while ((opt = getopt(plugin->argc, plugin->argv, ":a:A:w:m:n:i:M?")) >= 0) {
    switch (opt) {
    case 'm':
      if (optarg != NULL && strlen(optarg) - 1 <= ASMONITOR_METRIC_PFX_LEN) {
        strncpy(state->metric_prefix, optarg, ASMONITOR_METRIC_PFX_LEN);
      } else {
        fprintf(stderr, "Error: could not set metric prefix\n");
        usage(plugin);
        return -1;
      }
      break;

    case 'i':
      if (optarg != NULL && strlen(optarg) - 1 <= ASMONITOR_METRIC_PFX_LEN) {
        strncpy(state->ip_space_name, optarg, ASMONITOR_METRIC_PFX_LEN);
      } else {
        fprintf(stderr, "Error: could not set IP space name\n");
        usage(plugin);
        return -1;
      }
      break;

    case 'a':
      monitor_asn = strtoul(optarg, NULL, 10);
      bgpstream_id_set_insert(state->monitored_ases, monitor_asn);
      break;
    case 'A':
      if (add_asns_from_file(state->monitored_ases, optarg) != 0) {
        return -1;
      }
      break;

    case 'w':
      state->pfx_window = strtoul(optarg, NULL, 10);
      ;
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
  if (bgpstream_id_set_size(state->monitored_ases) == 0) {
    fprintf(stderr, "Error: no valid ASns provided\n");
    usage(plugin);
    return -1;
  }

  return 0;
}

/* == PUBLIC PLUGIN FUNCS BELOW HERE == */

/** Implements the alloc function of the plugin API */
bgpcorsaro_plugin_t *bgpcorsaro_asmonitor_alloc(bgpcorsaro_t *bgpcorsaro)
{
  return &bgpcorsaro_asmonitor_plugin;
}

/** Implements the init_output function of the plugin API */
int bgpcorsaro_asmonitor_init_output(bgpcorsaro_t *bgpcorsaro)
{
  struct bgpcorsaro_asmonitor_state_t *state;
  int v;
  bgpcorsaro_plugin_t *plugin = PLUGIN(bgpcorsaro);
  assert(plugin != NULL);

  if ((state = malloc_zero(sizeof(struct bgpcorsaro_asmonitor_state_t))) ==
      NULL) {
    bgpcorsaro_log(__func__, bgpcorsaro,
                   "could not malloc bgpcorsaro_asmonitor_state_t");
    goto err;
  }
  bgpcorsaro_plugin_register_state(bgpcorsaro->plugin_manager, plugin, state);

  /* initialize state with default values */
  state = STATE(bgpcorsaro);
  strcpy(state->metric_prefix, ASMONITOR_DEFAULT_METRIC_PFX);
  strcpy(state->ip_space_name, ASMONITOR_DEFAULT_IPSPACE_NAME);

  if ((state->monitored_ases = bgpstream_id_set_create()) == NULL) {
    goto err;
  }
  state->pfx_window = ASMONITOR_DEFAULT_PREFIX_WINDOW;
  state->peer_asns_th = ASMONITOR_DEFAULT_PEER_ASNS_THRESHOLD;
  state->more_specific = 0;

  /* parse the arguments */
  if (parse_args(bgpcorsaro) != 0) {
    goto err;
  }

  /* we do not graphite_safe the prefix, it could be
   * hierarchical on purpose:
   * state->metric_prefix;
   * same for the ip space name:
   * state->ip_space_name; */

  /* create all the sets and maps we need */
  if ((state->patricia = bgpstream_patricia_tree_create(perpfx_info_destroy)) ==
        NULL ||
      (state->pfx_info = kh_init(pfx_info_map)) == NULL ||
      (state->peer_asns = bgpstream_id_set_create()) == NULL) {
    goto err;
  }

  for (v = 0; v < BGPSTREAM_MAX_IP_VERSION_IDX; v++) {
    if ((state->unique_origins[v] = bgpstream_id_set_create()) == NULL) {
      goto err;
    }
  }

  /* defer opening the output file until we start the first interval */

  return 0;

err:
  bgpcorsaro_asmonitor_close_output(bgpcorsaro);
  return -1;
}

/** Implements the close_output function of the plugin API */
int bgpcorsaro_asmonitor_close_output(bgpcorsaro_t *bgpcorsaro)
{
  int i;
  struct bgpcorsaro_asmonitor_state_t *state = STATE(bgpcorsaro);
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
  if (state->patricia != NULL) {
    bgpstream_patricia_tree_destroy(state->patricia);
    state->patricia = NULL;
  }

  /* deallocate the dynamic memory in use */

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

  int vers_id;
  for (vers_id = 0; vers_id < BGPSTREAM_MAX_IP_VERSION_IDX; vers_id++) {
    if (state->unique_origins[vers_id] != NULL) {
      bgpstream_id_set_destroy(state->unique_origins[vers_id]);
      state->unique_origins[vers_id] = NULL;
    }
  }

  bgpcorsaro_plugin_free_state(bgpcorsaro->plugin_manager, PLUGIN(bgpcorsaro));
  return 0;
}

/** Implements the start_interval function of the plugin API */
int bgpcorsaro_asmonitor_start_interval(bgpcorsaro_t *bgpcorsaro,
                                        bgpcorsaro_interval_t *int_start)
{
  struct bgpcorsaro_asmonitor_state_t *state = STATE(bgpcorsaro);

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

  /* save the interval start to correctly output the timeseries */
  state->interval_start = int_start->time;

  return 0;
}

/** Implements the end_interval function of the plugin API */
int bgpcorsaro_asmonitor_end_interval(bgpcorsaro_t *bgpcorsaro,
                                      bgpcorsaro_interval_t *int_end)
{
  struct bgpcorsaro_asmonitor_state_t *state = STATE(bgpcorsaro);

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

  output_stats_and_reset(bgpcorsaro);

  return 0;
}

/** Implements the process_record function of the plugin API */
int bgpcorsaro_asmonitor_process_record(bgpcorsaro_t *bgpcorsaro,
                                        bgpcorsaro_record_t *record)
{
  struct bgpcorsaro_asmonitor_state_t *state = STATE(bgpcorsaro);
  bgpstream_record_t *bs_record = BS_REC(record);
  bgpstream_elem_t *elem;
  uint32_t origin_asn;
  bgpstream_as_path_seg_t *origin_seg;
  uint8_t overlap;
  bgpstream_patricia_node_t *n;
  perpfx_info_t *info;

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
    if (elem->type == BGPSTREAM_ELEM_TYPE_PEERSTATE) {
      continue;
    }

    /* consider only prefixes announced by the peers (not the collector) */
    if (elem->type != BGPSTREAM_ELEM_TYPE_WITHDRAWAL &&
        bgpstream_as_path_get_len(elem->aspath) == 0) {
      continue;
    }

    /* Check if the origin is one of the ASns to monitor */
    if (elem->type == BGPSTREAM_ELEM_TYPE_RIB ||
        elem->type == BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT) {
      origin_seg = bgpstream_as_path_get_origin_seg(elem->aspath);
      if (origin_seg == NULL || origin_seg->type != BGPSTREAM_AS_PATH_SEG_ASN) {
        return 0;
      }
      origin_asn = ((bgpstream_as_path_seg_asn_t *)origin_seg)->asn;

      if (bgpstream_id_set_exists(state->monitored_ases, origin_asn)) {
        /* if the origin match, then add the prefix to the tree and
         * update the timestamp if it already exist */
        n = bgpstream_patricia_tree_insert(state->patricia,
                                           (bgpstream_pfx_t *)&elem->prefix);
        info = (perpfx_info_t *)bgpstream_patricia_tree_get_user(n);
        if (info != NULL) {
          perpfx_info_set_ts(info, state->interval_start);
        } else {
          /* tmp_buffer[0] = '\0'; */
          /* fprintf(stderr, "Adding a new prefix to the PT: %s\n", */
          /*         bgpstream_pfx_snprintf(tmp_buffer, 1024, (bgpstream_pfx_t
           * *) &elem->prefix)); */
          bgpstream_patricia_tree_set_user(
            state->patricia, n, perpfx_info_create(state->interval_start));
        }
      }
    }

    /* consider only prefixes that overlap with the set provided */
    overlap = bgpstream_patricia_tree_get_pfx_overlap_info(
      state->patricia, (bgpstream_pfx_t *)&elem->prefix);

    /* if the more specific flag is set, then we accept only more specifics or
     * exact matches */
    if (overlap > 0 && (!state->more_specific ||
                        overlap & (BGPSTREAM_PATRICIA_LESS_SPECIFICS |
                                   BGPSTREAM_PATRICIA_EXACT_MATCH))) {
      /* fprintf(stderr, "Overlapping prefix message: %s\n", */
      /*         bgpstream_record_elem_snprintf(tmp_buffer, 1024, bs_record,
       * elem)); */
      process_overlapping_pfx(state, bs_record, elem);
    }
  }

  return 0;
}
