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

#include "bgpcorsaro_pacifier.h"

/** @file
 *
 * @brief Bgpcorsaro Pacifier plugin implementation
 *
 * @author Chiara Orsini
 *
 */

/** The number of output file pointers to support non-blocking close at the end
    of an interval. If the wandio buffers are large enough that it takes more
    than 1 interval to drain the buffers, consider increasing this number */
#define OUTFILE_POINTERS 2

/** The name of this plugin */
#define PLUGIN_NAME "pacifier"

/** The version of this plugin */
#define PLUGIN_VERSION "0.1"

/** Common plugin information across all instances */
static bgpcorsaro_plugin_t bgpcorsaro_pacifier_plugin = {
  PLUGIN_NAME,                                          /* name */
  PLUGIN_VERSION,                                       /* version */
  BGPCORSARO_PLUGIN_ID_PACIFIER,                        /* id */
  BGPCORSARO_PLUGIN_GENERATE_PTRS(bgpcorsaro_pacifier), /* func ptrs */
  BGPCORSARO_PLUGIN_GENERATE_TAIL,
};

/** Holds the state for an instance of this plugin */
struct bgpcorsaro_pacifier_state_t {
  /** The outfile for the plugin */
  iow_t *outfile;
  /** A set of pointers to outfiles to support non-blocking close */
  iow_t *outfile_p[OUTFILE_POINTERS];
  /** The current outfile */
  int outfile_n;

  // first time
  int tv_first_time;
  // interval counter
  int intervals;

  // start time
  int tv_start;
  // interval length
  uint8_t wait;

  // adaptive behavior
  uint8_t adaptive;
};

/** Extends the generic plugin state convenience macro in bgpcorsaro_plugin.h */
#define STATE(bgpcorsaro)                                                      \
  (BGPCORSARO_PLUGIN_STATE(bgpcorsaro, pacifier, BGPCORSARO_PLUGIN_ID_PACIFIER))
/** Extends the generic plugin plugin convenience macro in bgpcorsaro_plugin.h
 */
#define PLUGIN(bgpcorsaro)                                                     \
  (BGPCORSARO_PLUGIN_PLUGIN(bgpcorsaro, BGPCORSARO_PLUGIN_ID_PACIFIER))

/** Print usage information to stderr */
static void usage(bgpcorsaro_plugin_t *plugin)
{
  fprintf(stderr, "plugin usage: %s [-w interval-lenght] -a\n"
                  "       -w interval-lenght  (default: 30s)\n"
                  "       -a                  adaptive (default:off) \n",
          plugin->argv[0]);
}

/** Parse the arguments given to the plugin */
static int parse_args(bgpcorsaro_t *bgpcorsaro)
{
  bgpcorsaro_plugin_t *plugin = PLUGIN(bgpcorsaro);
  struct bgpcorsaro_pacifier_state_t *state = STATE(bgpcorsaro);
  int opt;

  if (plugin->argc <= 0) {
    return 0;
  }

  /* NB: remember to reset optind to 1 before using getopt! */
  optind = 1;

  while ((opt = getopt(plugin->argc, plugin->argv, ":w:a?")) >= 0) {
    switch (opt) {
    case 'w':
      state->wait = atoi(optarg);
      break;
    case 'a':
      state->adaptive = 1;
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
bgpcorsaro_plugin_t *bgpcorsaro_pacifier_alloc(bgpcorsaro_t *bgpcorsaro)
{
  return &bgpcorsaro_pacifier_plugin;
}

/** Implements the init_output function of the plugin API */
int bgpcorsaro_pacifier_init_output(bgpcorsaro_t *bgpcorsaro)
{
  struct bgpcorsaro_pacifier_state_t *state;
  bgpcorsaro_plugin_t *plugin = PLUGIN(bgpcorsaro);
  assert(plugin != NULL);

  if ((state = malloc_zero(sizeof(struct bgpcorsaro_pacifier_state_t))) ==
      NULL) {
    bgpcorsaro_log(__func__, bgpcorsaro,
                   "could not malloc bgpcorsaro_pacifier_state_t");
    goto err;
  }
  bgpcorsaro_plugin_register_state(bgpcorsaro->plugin_manager, plugin, state);

  // initializing state
  state = STATE(bgpcorsaro);
  state->tv_start = 0; // 0 means it is the first interval, so the time has to
                       // be initialized at interval start
  state->wait =
    30; // 30 seconds is the default wait time, between start and end
  state->tv_first_time = 0; // 0 means it is the first interval, so the time has
                            // to be initialized at interval start
  state->intervals = 0;     // number of intervals processed
  state->adaptive = 0;      // default behavior is not adaptive

  /* parse the arguments */
  if (parse_args(bgpcorsaro) != 0) {
    return -1;
  }

  /* defer opening the output file until we start the first interval */

  return 0;

err:
  bgpcorsaro_pacifier_close_output(bgpcorsaro);
  return -1;
}

/** Implements the close_output function of the plugin API */
int bgpcorsaro_pacifier_close_output(bgpcorsaro_t *bgpcorsaro)
{
  int i;
  struct bgpcorsaro_pacifier_state_t *state = STATE(bgpcorsaro);

  if (state != NULL) {
    /* close all the outfile pointers */
    for (i = 0; i < OUTFILE_POINTERS; i++) {
      if (state->outfile_p[i] != NULL) {
        wandio_wdestroy(state->outfile_p[i]);
        state->outfile_p[i] = NULL;
      }
    }
    state->outfile = NULL;
    bgpcorsaro_plugin_free_state(bgpcorsaro->plugin_manager,
                                 PLUGIN(bgpcorsaro));
  }
  return 0;
}

/** Implements the start_interval function of the plugin API */
int bgpcorsaro_pacifier_start_interval(bgpcorsaro_t *bgpcorsaro,
                                       bgpcorsaro_interval_t *int_start)
{
  struct bgpcorsaro_pacifier_state_t *state = STATE(bgpcorsaro);

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

  struct timeval tv;

  if (state->tv_start == 0) {
    gettimeofday_wrap(&tv);
    state->tv_start = tv.tv_sec;
    state->tv_first_time = state->tv_start;
  }

  // a new interval is starting
  state->intervals++;

  // fprintf(stderr, "START INTERVAL TIME: %d \n", state->tv_start);

  return 0;
}

/** Implements the end_interval function of the plugin API */
int bgpcorsaro_pacifier_end_interval(bgpcorsaro_t *bgpcorsaro,
                                     bgpcorsaro_interval_t *int_end)
{
  struct bgpcorsaro_pacifier_state_t *state = STATE(bgpcorsaro);

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

  struct timeval tv;
  gettimeofday_wrap(&tv);

  int expected_time = 0;
  int diff = 0;

  if (state->adaptive == 0) {
    diff = state->wait - (tv.tv_sec - state->tv_start);
  } else {
    expected_time = state->tv_first_time + state->intervals * state->wait;
    diff = expected_time - tv.tv_sec;
  }
  // if the end interval is faster than "the wait" time
  // then we wait for the remaining seconds
  if (diff > 0) {
    // fprintf(stderr, "\tWaiting: %d s\n", diff);
    sleep(diff);
    gettimeofday_wrap(&tv);
  }
  state->tv_start = tv.tv_sec;

  // fprintf(stderr, "END INTERVAL TIME: %d \n", state->tv_start);
  return 0;
}

/** Implements the process_record function of the plugin API */
int bgpcorsaro_pacifier_process_record(bgpcorsaro_t *bgpcorsaro,
                                       bgpcorsaro_record_t *record)
{

  /* no point carrying on if a previous plugin has already decided we should
     ignore this record */
  if ((record->state.flags & BGPCORSARO_RECORD_STATE_FLAG_IGNORE) != 0) {
    return 0;
  }

  return 0;
}
