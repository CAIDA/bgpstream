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
#include "bgpcorsaro_int.h"

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

#include "khash.h"
#include "bgpstream_utils_pfx.h"
#include "bgpstream_utils_pfx_set.h"
#include "bgpstream_utils_peer_sig_map.h"
#include "bgpstream_utils_ip_counter.h"

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

/** Minimum number of monitors to declare prefix visible */
#define PFXMONITOR_DEFAULT_MONITOR_THRESHOLD 10

/** Maximum string length for the metric prefix */
#define PFXMONITOR_METRIC_PFX_LEN 256 

/** Maximum string length for the log entries */
#define MAX_LOG_BUFFER_LEN 1024

#define DUMP_METRIC(value, time, fmt, ...)                              \
  do {                                                                  \
    fprintf(stdout, fmt" %"PRIu32" %"PRIu32"\n",                        \
            __VA_ARGS__, value, time);                                  \
  } while(0)

/** Common plugin information across all instances */
static bgpcorsaro_plugin_t bgpcorsaro_pfxmonitor_plugin = {
  PLUGIN_NAME,                                            /* name */
  PLUGIN_VERSION,                                         /* version */
  BGPCORSARO_PLUGIN_ID_PFXMONITOR,                        /* id */
  BGPCORSARO_PLUGIN_GENERATE_PTRS(bgpcorsaro_pfxmonitor), /* func ptrs */
  BGPCORSARO_PLUGIN_GENERATE_TAIL,
};

static char *
graphite_safe(char *p)
{
  if(p == NULL)
    {
      return p;
    }

  char *r = p;
  while(*p != '\0')
    {
      if(*p == '.')
	{
	  *p = '-';
	}
      if(*p == '*')
	{
	  *p = '-';
	}
      p++;
    }
  return r;
}

/** A map that contains, for each origin the number of monitors observing it */
KHASH_INIT(asn_count_map, uint32_t, uint32_t, 1, \
           kh_int_hash_func, kh_int_hash_equal);
typedef khash_t(asn_count_map) asn_count_map_t;


/** A map that contains the origin observed by a specific peer */
KHASH_INIT(peer_asn_map, uint32_t, uint32_t, 1, \
           kh_int_hash_func, kh_int_hash_equal);
typedef khash_t(peer_asn_map) peer_asn_map_t;

/** A map that contains information for each prefix */
KHASH_INIT(pfx_info_map, bgpstream_pfx_storage_t, peer_asn_map_t*, 1, \
           bgpstream_pfx_storage_hash_val, bgpstream_pfx_storage_equal_val);
typedef khash_t(pfx_info_map) pfx_info_map_t;


/** Holds the state for an instance of this plugin */
struct bgpcorsaro_pfxmonitor_state_t {
  
  /** The outfile for the plugin */
  iow_t *outfile;
  /** A set of pointers to outfiles to support non-blocking close */
  iow_t *outfile_p[OUTFILE_POINTERS];
  /** The current outfile */
  int outfile_n;

  uint32_t interval_start;
  
  /** Prefixes of interest */
  bgpstream_ip_counter_t *poi;

  /** Cache of prefixes
   *  (it speeds up the overlapping check) */
  bgpstream_pfx_storage_set_t *overlapping_pfx_cache;
  bgpstream_pfx_storage_set_t *non_overlapping_pfx_cache;
  
  /** Monitors' ASns */
  bgpstream_id_set_t *monitor_asns;

  /** Prefix info map */
  pfx_info_map_t *pfx_info;

  /** utility set to compute unique origin
   *  ASns  */
  bgpstream_id_set_t *unique_origins;

  /* Monitor threshold - minimum number of
   * monitors' ASns to declare prefix visible */
  uint32_t monitor_th;

  /* If 1 we consider only more specific prefixes */
  uint8_t more_specific;
  
  /** Metric prefix */
  char metric_prefix[PFXMONITOR_METRIC_PFX_LEN];

  /** IP space name */
  char ip_space_name[PFXMONITOR_METRIC_PFX_LEN];

};

/** Extends the generic plugin state convenience macro in bgpcorsaro_plugin.h */
#define STATE(bgpcorsaro)						\
  (BGPCORSARO_PLUGIN_STATE(bgpcorsaro, pfxmonitor, BGPCORSARO_PLUGIN_ID_PFXMONITOR))
/** Extends the generic plugin plugin convenience macro in bgpcorsaro_plugin.h */
#define PLUGIN(bgpcorsaro)						\
  (BGPCORSARO_PLUGIN_PLUGIN(bgpcorsaro, BGPCORSARO_PLUGIN_ID_PFXMONITOR))


static int
output_stats_and_reset(struct bgpcorsaro_pfxmonitor_state_t *state, uint32_t interval_start)
{
  khiter_t k;
  khiter_t p;
  khiter_t a;
  int khret;
  uint8_t pfx_visible;
  uint32_t unique_pfxs = 0;
  peer_asn_map_t *pam;
  /* origin_asn -> num monitors*/
  asn_count_map_t *asn_nm = NULL;
  /* char buffer[MAX_LOG_BUFFER_LEN]; */
  
  if((asn_nm = kh_init(asn_count_map)) == NULL)
    {
      return -1;
    }

  /* for each prefix go through all origins */
  for (k = kh_begin(state->pfx_info); 
       k != kh_end(state->pfx_info); ++k)
    {
      if (kh_exist(state->pfx_info, k))
        {
          kh_clear(asn_count_map, asn_nm);
          pfx_visible = 0;
          pam = kh_value(state->pfx_info, k);
          /* save the origin asn visibility (i.e. how many monitors
           * observe such information */
          for (p = kh_begin(pam); 
               p != kh_end(pam); ++p)
            {
              if (kh_exist(pam, p))
                {
                  if((a = kh_get(asn_count_map, asn_nm, kh_value(pam,p))) ==
                     kh_end(asn_nm))
                    {
                      a = kh_put(asn_count_map, asn_nm, kh_value(pam,p), &khret);
                      kh_value(asn_nm,a) = 1;
                    }
                  else
                    {
                      kh_value(asn_nm,a) += 1;
                    }
                }
            }

          /* count the prefix and origins if their visibility
           * is above the threshold */
          pfx_visible = 0;
          for (a = kh_begin(asn_nm); 
               a != kh_end(asn_nm); ++a)
            {
              if (kh_exist(asn_nm, a))
                {
                  /* the information is accounted only if it is
                   * consistent on at least threshold monitors */
                  if(kh_value(asn_nm, a) >= state->monitor_th)
                    {
                      pfx_visible = 1;
                      bgpstream_id_set_insert(state->unique_origins, kh_key(asn_nm, a));
                      /* bgpstream_pfx_snprintf(buffer,MAX_LOG_BUFFER_LEN, */
                      /*                        (bgpstream_pfx_t *)&kh_key(state->pfx_info, k)); */
                      /* fprintf(stderr, "%s %"PRIu32" %"PRIu32" \n", */
                      /*         buffer, kh_key(asn_nm, a), kh_value(asn_nm, a)); */
                    }
                }

            }

          /* updating counters */
          unique_pfxs += pfx_visible;                    
        }
    }

  kh_destroy(asn_count_map, asn_nm);

  uint32_t unique_origins_cnt = bgpstream_id_set_size(state->unique_origins);
  bgpstream_id_set_clear(state->unique_origins);

  DUMP_METRIC(unique_pfxs, state->interval_start,
              "%s.%s.%s.%s", state->metric_prefix, PLUGIN_NAME,
              state->ip_space_name, "prefixes_cnt");

  DUMP_METRIC(unique_origins_cnt, state->interval_start,
              "%s.%s.%s.%s", state->metric_prefix, PLUGIN_NAME,
              state->ip_space_name, "origin_ASns_cnt");

  return 0;
}


static int
process_pfx_info(struct bgpcorsaro_pfxmonitor_state_t *state,
                 bgpstream_elem_t *elem)
{
  char log_buffer[MAX_LOG_BUFFER_LEN];
  log_buffer[0] = '\0';
  int khret;
  khiter_t k;
  khiter_t p;
  peer_asn_map_t *pam;  
  uint32_t origin_asn = 0;
  bgpstream_as_path_seg_t *origin_seg = NULL;

  if(elem->type == BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT ||
     elem->type == BGPSTREAM_ELEM_TYPE_RIB)
    {
      /* get the origin asn (sets and confederations are ignored) */
      origin_seg = bgpstream_as_path_get_origin_seg(elem->aspath);
      if(origin_seg != NULL && origin_seg->type == BGPSTREAM_AS_PATH_SEG_ASN)
        {
          origin_asn = ((bgpstream_as_path_seg_asn_t*)origin_seg)->asn;
        }
      if(origin_asn == 0)
        {
          fprintf(stderr, "Warning: ignoring AS sets and confederations in statistics\n");
        }
      else
        {
          /* inserting announcement/rib in state structure */
          if((k = kh_get(pfx_info_map, state->pfx_info, elem->prefix)) ==
             kh_end(state->pfx_info))
            {
              k = kh_put(pfx_info_map, state->pfx_info, elem->prefix, &khret);
              
              if((pam = kh_init(peer_asn_map)) == NULL)
                {
                  return -1;
                }
              kh_value(state->pfx_info, k) = pam;
            }
          pam = kh_value(state->pfx_info, k);
          /* accessing {pfx}{peer} */
          if((p = kh_get(peer_asn_map, pam, elem->peer_asnumber)) == kh_end(pam))
            {
              p =  kh_put(peer_asn_map, pam, elem->peer_asnumber, &khret);
            }
          kh_value(pam, p) = origin_asn;          
        }      
    }
  else
    {
      /* removing pfx/peer from state structure */
      if((k = kh_get(pfx_info_map, state->pfx_info, elem->prefix)) !=
         kh_end(state->pfx_info))
        {
          pam = kh_value(state->pfx_info, k);
          if((p = kh_get(peer_asn_map, pam, elem->peer_asnumber)) != kh_end(pam))
            {
              kh_del(peer_asn_map, pam, p);
            }
        }
    }
  
  /* we always print all the info to the log */
  bgpstream_elem_snprintf(log_buffer,MAX_LOG_BUFFER_LEN, elem);
  wandio_printf(state->outfile,"%s\n",log_buffer);
  return 0;
}

static void
add_new_prefix(bgpstream_ip_counter_t *poi, char* pfx_string)
{
  if(pfx_string == NULL)
    {
      return;
    }
  int j;
  bgpstream_pfx_storage_t pfx_st;
  pfx_st.address.version = BGPSTREAM_ADDR_VERSION_UNKNOWN;
  for(j=0; j< strlen(pfx_string); j++)
    {
      if(pfx_string[j] == '.')
        {
          pfx_st.address.version = BGPSTREAM_ADDR_VERSION_IPV4;
        }
      if(pfx_string[j] == ':')
        {
          pfx_st.address.version = BGPSTREAM_ADDR_VERSION_IPV6;
        }
      if(pfx_string[j] == '/')
        {
          pfx_st.mask_len = atoi(&pfx_string[j+1]);
          pfx_string[j] = '\0';
          break;
        }
    }
  if(pfx_st.address.version == BGPSTREAM_ADDR_VERSION_IPV4)
    {
      inet_pton(BGPSTREAM_ADDR_VERSION_IPV4, pfx_string, &pfx_st.address.ipv4);
      bgpstream_ip_counter_add(poi, (bgpstream_pfx_t *) &pfx_st);
    }
  else
    {
      if(pfx_st.address.version == BGPSTREAM_ADDR_VERSION_IPV6)
        {
          inet_pton(BGPSTREAM_ADDR_VERSION_IPV6,pfx_string, &pfx_st.address.ipv6);
          bgpstream_ip_counter_add(poi, (bgpstream_pfx_t *) &pfx_st);
        }
    }
}

static int
add_prefixes_from_file(bgpstream_ip_counter_t *poi, char* pfx_file_string)
{

  char pfx_line[MAX_LOG_BUFFER_LEN];
  io_t *file = NULL;

  if(pfx_file_string != NULL)
    {
       if((file = wandio_create(pfx_file_string)) == NULL)
	{
	  fprintf(stderr, "ERROR: Could not open prefix file (%s)\n", pfx_file_string);
	  return -1;
	}
       
       while(wandio_fgets(file, &pfx_line, MAX_LOG_BUFFER_LEN, 1) > 0)
         {
           /* treat # as comment line, and ignore empty lines */
           if(pfx_line[0] == '#' || pfx_line[0] == '\0')
             {
               continue;
             }
           
           add_new_prefix(poi, pfx_line);           
	}
       wandio_destroy(file);
       return 0;       
    }
  return -1;
}



/** Print usage information to stderr */
static void usage(bgpcorsaro_plugin_t *plugin)
{
  fprintf(stderr,
	  "plugin usage: %s -l <pfx> \n"
	  "       -m <prefix>        metric prefix (default: %s)\n"
	  "       -l <prefix>        prefix to monitor*\n"
	  "       -L <prefix-file>   read the prefixes to monitor from file*\n"
          "       -M                 consider only more specifics (default: false)\n"
          "       -n <monitor_cnt>   minimum number of unique monitors' ASns to declare prefix visible (default: %d)\n"
          "       -i <name>          IP space name (default: %s)\n"
	  "* denotes an option that can be given multiple times\n",
	  plugin->argv[0], PFXMONITOR_DEFAULT_METRIC_PFX,
          PFXMONITOR_DEFAULT_MONITOR_THRESHOLD, PFXMONITOR_DEFAULT_IPSPACE_NAME);
}

/** Parse the arguments given to the plugin */
static int parse_args(bgpcorsaro_t *bgpcorsaro)
{
  bgpcorsaro_plugin_t *plugin = PLUGIN(bgpcorsaro);
  struct bgpcorsaro_pfxmonitor_state_t *state = STATE(bgpcorsaro);
  int opt;
  char *tmp_string = NULL;
  int ret = 0;
  if(plugin->argc <= 0)
    {
      return 0;
    }

  /* NB: remember to reset optind to 1 before using getopt! */
  optind = 1;

  while((opt = getopt(plugin->argc, plugin->argv, ":l:L:m:n:i:M?")) >= 0)
    {
      switch(opt)
	{
	case 'm':
          tmp_string = strdup(optarg);
          if(tmp_string != NULL && strlen(tmp_string)-1 <= PFXMONITOR_METRIC_PFX_LEN)
            {
              strcpy(state->metric_prefix, tmp_string);
              free(tmp_string);
            }
          else
            {
              fprintf(stderr,
                      "Warning: could not set metric prefix, using default %s \n",
                      PFXMONITOR_DEFAULT_METRIC_PFX);
            }
	  break;
	case 'i':
          tmp_string = strdup(optarg);
          if(tmp_string != NULL && strlen(tmp_string)-1 <= PFXMONITOR_METRIC_PFX_LEN)
            {
              strcpy(state->ip_space_name, tmp_string);
              free(tmp_string);
            }
          else
            {
              fprintf(stderr,
                      "Warning: could not set metric prefix, using default %s \n",
                      PFXMONITOR_DEFAULT_IPSPACE_NAME);
            }
	  break;
        case 'l':
          tmp_string = strdup(optarg);
	  add_new_prefix(state->poi, tmp_string);
          free(tmp_string);
	  break;
        case 'L':
          tmp_string = strdup(optarg);
	  ret = add_prefixes_from_file(state->poi, tmp_string);
          free(tmp_string);
          if(ret < 0)
            {
              return -1;
            }
	  break;
        case 'M':
          state->more_specific = 1;
          break;
	case 'n':
          state->monitor_th = atoi(optarg);
	  break;

	case '?':
	case ':':
	default:
	  usage(plugin);
	  return -1;
	}
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

  if((state = malloc_zero(sizeof(struct bgpcorsaro_pfxmonitor_state_t))) == NULL)
    {
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

  if((state->poi = bgpstream_ip_counter_create()) == NULL)
    {
      goto err;
    }
  state->monitor_th = PFXMONITOR_DEFAULT_MONITOR_THRESHOLD;
  state->more_specific = 0;

  /* parse the arguments */
  if(parse_args(bgpcorsaro) != 0)
    {
      goto err;
    }

  graphite_safe(&(state->metric_prefix[0]));
  graphite_safe(&(state->ip_space_name[0]));
  /* build the remaining state variables */
  if((state->overlapping_pfx_cache = bgpstream_pfx_storage_set_create()) == NULL)
    {
      goto err;
    }
  if((state->non_overlapping_pfx_cache = bgpstream_pfx_storage_set_create()) == NULL)
    {
      goto err;
    }

  if((state->pfx_info = kh_init(pfx_info_map)) == NULL)
    {
      goto err;
    }

  if((state->unique_origins = bgpstream_id_set_create()) == NULL)
    {
      goto err;
    }

  if((state->monitor_asns = bgpstream_id_set_create()) == NULL)
    {
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

  if(state != NULL)
    {
      /* close all the outfile pointers */
      for(i = 0; i < OUTFILE_POINTERS; i++)
	{
	  if(state->outfile_p[i] != NULL)
	    {
	      wandio_wdestroy(state->outfile_p[i]);
	      state->outfile_p[i] = NULL;
	    }
	}
      state->outfile = NULL;
      if(state->poi != NULL)
        {
          bgpstream_ip_counter_destroy(state->poi);
        }

      /* deallocate the dynamic memory in use */
      if(state->overlapping_pfx_cache != NULL)
        {
          bgpstream_pfx_storage_set_destroy(state->overlapping_pfx_cache);
        }

      if(state->non_overlapping_pfx_cache != NULL)
        {
          bgpstream_pfx_storage_set_destroy(state->non_overlapping_pfx_cache);
        }

      if(state->monitor_asns != NULL)
        {
          bgpstream_id_set_destroy(state->monitor_asns);
          state->monitor_asns = NULL;
        }

      khiter_t k;
      peer_asn_map_t *v;
      if(state->pfx_info != NULL)
        {
          for (k = kh_begin(state->pfx_info);
               k != kh_end(state->pfx_info); ++k)
            {          
              if (kh_exist(state->pfx_info, k))
                {
                  v = kh_val(state->pfx_info, k);
                  kh_destroy(peer_asn_map, v);
                }
            }
          kh_destroy(pfx_info_map, state->pfx_info);
          state->pfx_info = NULL;
        }

      if(state->unique_origins != NULL)
        {
          bgpstream_id_set_destroy(state->unique_origins);
          state->unique_origins = NULL;
        }
      bgpcorsaro_plugin_free_state(bgpcorsaro->plugin_manager, PLUGIN(bgpcorsaro));
    }
  return 0;
}

/** Implements the start_interval function of the plugin API */
int bgpcorsaro_pfxmonitor_start_interval(bgpcorsaro_t *bgpcorsaro,
				       bgpcorsaro_interval_t *int_start)
{
  struct bgpcorsaro_pfxmonitor_state_t *state = STATE(bgpcorsaro);

  if(state->outfile == NULL)
    {
      if((
	  state->outfile_p[state->outfile_n] =
	  bgpcorsaro_io_prepare_file(bgpcorsaro,
				     PLUGIN(bgpcorsaro)->name,
				     int_start)) == NULL)
	{
	  bgpcorsaro_log(__func__, bgpcorsaro, "could not open %s output file",
			 PLUGIN(bgpcorsaro)->name);
	  return -1;
	}
      state->outfile = state->
	outfile_p[state->outfile_n];
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
  if(bgpcorsaro_is_rotate_interval(bgpcorsaro))
    {
      /* leave the current file to finish draining buffers */
      assert(state->outfile != NULL);

      /* move on to the next output pointer */
      state->outfile_n = (state->outfile_n+1) %
	OUTFILE_POINTERS;

      if(state->outfile_p[state->outfile_n] != NULL)
	{
	  /* we're gonna have to wait for this to close */
	  wandio_wdestroy(state->outfile_p[state->outfile_n]);
	  state->outfile_p[state->outfile_n] =  NULL;
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
  bgpstream_record_t * bs_record = BS_REC(record);
  bgpstream_elem_t *elem;
  uint64_t overlap;
  uint8_t more_specific;

  /* no point carrying on if a previous plugin has already decided we should
     ignore this record */
  if((record->state.flags & BGPCORSARO_RECORD_STATE_FLAG_IGNORE) != 0)
    {
      return 0;
    }

  /* consider only valid records */
  if(bs_record->status == BGPSTREAM_RECORD_STATUS_VALID_RECORD)
    {
      while((elem = bgpstream_record_get_next_elem(bs_record)) != NULL)
        {
          if(elem->type == BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT ||
             elem->type == BGPSTREAM_ELEM_TYPE_WITHDRAWAL ||
             elem->type == BGPSTREAM_ELEM_TYPE_RIB)
            {              
              /* consider only prefixes announced by the monitors (not the collector) */
              if(elem->type != BGPSTREAM_ELEM_TYPE_WITHDRAWAL &&
                 bgpstream_as_path_get_len(elem->aspath) == 0)
                {
                  continue;
                }

              /* consider only prefixes that overlap with the set provided */

              /* check the overlapping pfxs cache */
              if(bgpstream_pfx_storage_set_exists(state->overlapping_pfx_cache, &elem->prefix))
                {
                  process_pfx_info(state, elem);
                }
              else
                {
                  /* check the non overlapping pfxs cache */
                  if(bgpstream_pfx_storage_set_exists(state->non_overlapping_pfx_cache, &elem->prefix) == 0)
                    {
                      /* otherwise compute the overlapping and populate the appropriate cache */
                      more_specific = 0;
                      overlap = bgpstream_ip_counter_is_overlapping(state->poi,(bgpstream_pfx_t *) &elem->prefix,
                                                                    &more_specific);
                      if(overlap > 0 && (!state->more_specific || more_specific))
                        {
                          bgpstream_pfx_storage_set_insert(state->overlapping_pfx_cache, &elem->prefix);
                          process_pfx_info(state, elem);
                        }
                      else
                        {
                          bgpstream_pfx_storage_set_insert(state->non_overlapping_pfx_cache, &elem->prefix);
                        }
                    }
                }
            }

        }

    }

  return 0;
}
