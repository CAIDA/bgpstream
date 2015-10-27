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

#include "bgpcorsaro_elemcounter.h"

/** @file
 *
 * @brief Bgpcorsaro Elem Counter plugin implementation
 *
 * @author Chiara Orsini
 *
 */

/** The name of this plugin */
#define PLUGIN_NAME "elemcounter"

/** The version of this plugin */
#define PLUGIN_VERSION "0.1"

/** Common plugin information across all instances */
static bgpcorsaro_plugin_t bgpcorsaro_elemcounter_plugin = {
  PLUGIN_NAME,                                  /* name */
  PLUGIN_VERSION,                               /* version */
  BGPCORSARO_PLUGIN_ID_ELEMCOUNTER,                 /* id */
  BGPCORSARO_PLUGIN_GENERATE_PTRS(bgpcorsaro_elemcounter), /* func ptrs */
  BGPCORSARO_PLUGIN_GENERATE_TAIL,
};


/** Holds the state for an instance of this plugin */
struct bgpcorsaro_elemcounter_state_t {

  /* elem counter array, one counter per 
   * BGPStream elem type*/
  int elem_cnt[5];

  /* 0 if the preferred output is singleline (default)
  *  1 if the preferred output is multiline */
  int multiline_flag;
  
};


/** Extends the generic plugin state convenience macro in bgpcorsaro_plugin.h */
#define STATE(bgpcorsaro)						\
  (BGPCORSARO_PLUGIN_STATE(bgpcorsaro, elemcounter, BGPCORSARO_PLUGIN_ID_ELEMCOUNTER))

/** Extends the generic plugin plugin convenience macro in bgpcorsaro_plugin.h */
#define PLUGIN(bgpcorsaro)						\
  (BGPCORSARO_PLUGIN_PLUGIN(bgpcorsaro, BGPCORSARO_PLUGIN_ID_ELEMCOUNTER))

/** Print usage information to stderr */
static void usage(bgpcorsaro_plugin_t *plugin)
{
  fprintf(stderr,
	  "plugin usage: %s [-m]\n"
          "       -m multiline output  (default: singleline)\n",
	  plugin->argv[0]);
}

/** Parse the arguments given to the plugin */
static int parse_args(bgpcorsaro_t *bgpcorsaro)
{
  bgpcorsaro_plugin_t *plugin = PLUGIN(bgpcorsaro);
  struct bgpcorsaro_elemcounter_state_t *state = STATE(bgpcorsaro);

  int opt;

  if(plugin->argc <= 0)
    {
      return 0;
    }

  /* NB: remember to reset optind to 1 before using getopt! */
  optind = 1;

  while((opt = getopt(plugin->argc, plugin->argv, ":m?")) >= 0)
    {
      switch(opt)
	{
        case 'm':
	  state->multiline_flag = 1;
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
bgpcorsaro_plugin_t *bgpcorsaro_elemcounter_alloc(bgpcorsaro_t *bgpcorsaro)
{
  return &bgpcorsaro_elemcounter_plugin;
}

/** Implements the init_output function of the plugin API */
int bgpcorsaro_elemcounter_init_output(bgpcorsaro_t *bgpcorsaro)
{
  struct bgpcorsaro_elemcounter_state_t *state;
  bgpcorsaro_plugin_t *plugin = PLUGIN(bgpcorsaro);
  assert(plugin != NULL);

  if((state = malloc_zero(sizeof(struct bgpcorsaro_elemcounter_state_t))) == NULL)
    {
      bgpcorsaro_log(__func__, bgpcorsaro,
		     "could not malloc bgpcorsaro_elemcounter_state_t");
      goto err;
    }
  bgpcorsaro_plugin_register_state(bgpcorsaro->plugin_manager, plugin, state);

  /* allocate memory for state variables:
   * elemcounter does not have any dynamic memory variable
   * so no malloc is required */
  
  /* initialize state variables */
  int i;
  for(i=0; i< 5; i++)
    {
      state->elem_cnt[i] = 0;
    }

  /* single line is the default output format */
  state->multiline_flag = 0;
  
  /* parse the arguments */
  if(parse_args(bgpcorsaro) != 0)
    {
      return -1;
    }

  return 0;

 err:
  bgpcorsaro_elemcounter_close_output(bgpcorsaro);
  return -1;
}

/** Implements the close_output function of the plugin API */
int bgpcorsaro_elemcounter_close_output(bgpcorsaro_t *bgpcorsaro)
{

  struct bgpcorsaro_elemcounter_state_t *state = STATE(bgpcorsaro);

  if(state != NULL)
    {

      /* deallocate dynamic memory in state:
       * elemcounter does not have any dynamic memory variable
       * so no free is required  */
      
      bgpcorsaro_plugin_free_state(bgpcorsaro->plugin_manager, PLUGIN(bgpcorsaro));
    }
  return 0;
}

/** Implements the start_interval function of the plugin API */
int bgpcorsaro_elemcounter_start_interval(bgpcorsaro_t *bgpcorsaro,
                                          bgpcorsaro_interval_t *int_start)
{
  struct bgpcorsaro_elemcounter_state_t *state = STATE(bgpcorsaro);
  int i;
  /* reset counters */
  for(i=0; i< 5; i++)
    {
      state->elem_cnt[i] = 0;
    }
  return 0;
}

/** Implements the end_interval function of the plugin API */
int bgpcorsaro_elemcounter_end_interval(bgpcorsaro_t *bgpcorsaro,
                                        bgpcorsaro_interval_t *int_end)
{
  struct bgpcorsaro_elemcounter_state_t *state = STATE(bgpcorsaro);

  /* int_end->time is a uint32_t time in epoch */
  
  /* single line output */
  if(state->multiline_flag == 0)
    {
      printf("%"PRIu32" R: %d A: %d W: %d S: %d\n", int_end->time,
             state->elem_cnt[BGPSTREAM_ELEM_TYPE_RIB],
             state->elem_cnt[BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT],
             state->elem_cnt[BGPSTREAM_ELEM_TYPE_WITHDRAWAL],
             state->elem_cnt[BGPSTREAM_ELEM_TYPE_PEERSTATE]);
    }
  else
    { /* multi line output */
      printf("%"PRIu32" R: %d\n", int_end->time,
             state->elem_cnt[BGPSTREAM_ELEM_TYPE_RIB]);
      printf("%"PRIu32" A: %d\n", int_end->time,
             state->elem_cnt[BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT]);
      printf("%"PRIu32" W: %d\n", int_end->time,
             state->elem_cnt[BGPSTREAM_ELEM_TYPE_WITHDRAWAL]);
      printf("%"PRIu32" S: %d\n", int_end->time,
             state->elem_cnt[BGPSTREAM_ELEM_TYPE_PEERSTATE]);
    }
  return 0;
}

/** Implements the process_record function of the plugin API */
int bgpcorsaro_elemcounter_process_record(bgpcorsaro_t *bgpcorsaro,
                                          bgpcorsaro_record_t *record)
{
  struct bgpcorsaro_elemcounter_state_t *state = STATE(bgpcorsaro);
  bgpstream_record_t *bs_record = BS_REC(record);
  bgpstream_elem_t *elem;

  /* consider only valid records */
  if(bs_record->status != BGPSTREAM_RECORD_STATUS_VALID_RECORD)
    {
      return 0;
    }
  while((elem = bgpstream_record_get_next_elem(bs_record)) != NULL)
    {
      /* increment the specific type counter */
      state->elem_cnt[elem->type]++;
    }
return 0;
}

