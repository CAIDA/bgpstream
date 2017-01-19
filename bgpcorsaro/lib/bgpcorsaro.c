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
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "bgpcorsaro_io.h"
#include "bgpcorsaro_log.h"
#include "utils.h"
#include "wandio_utils.h"

/** @file
 *
 * @brief Code which implements the public functions of libbgpcorsaro
 *
 * @author Alistair King
 *
 */

/** Allocate a bgpcorsaro record wrapper structure */
static bgpcorsaro_record_t *bgpcorsaro_record_alloc(bgpcorsaro_t *bgpcorsaro)
{
  bgpcorsaro_record_t *rec;

  if ((rec = malloc_zero(sizeof(bgpcorsaro_record_t))) == NULL) {
    bgpcorsaro_log(__func__, bgpcorsaro, "could not malloc bgpcorsaro_record");
    return NULL;
  }

  /* this bgpcorsaro record still needs a bgpstream record to be loaded... */
  return rec;
}

/** Reset the state for a the given bgpcorsaro record wrapper */
static inline void bgpcorsaro_record_state_reset(bgpcorsaro_record_t *record)
{
  assert(record != NULL);

  /* now that we have added the bgpcorsaro_tag framework we can no longer do the
     brute-force reset of the record state to 0, each field MUST be individually
     cleared. if you add a field to bgpcorsaro_record_state_t, you MUST reset it
     here */

  /* reset the general flags */
  record->state.flags = 0;
}

/** Free the given bgpcorsaro record wrapper */
static void bgpcorsaro_record_free(bgpcorsaro_record_t *record)
{
  /* we will assume that somebody else is taking care of the bgpstream record */
  if (record != NULL) {
    free(record);
  }
}

/** Cleanup and free the given bgpcorsaro instance */
static void bgpcorsaro_free(bgpcorsaro_t *bgpcorsaro)
{
  bgpcorsaro_plugin_t *p = NULL;

  if (bgpcorsaro == NULL) {
    /* nothing to be done... */
    return;
  }

  /* free up the plugins first, they may try and use some of our info before
     closing */
  if (bgpcorsaro->plugin_manager != NULL) {
    while ((p = bgpcorsaro_plugin_next(bgpcorsaro->plugin_manager, p)) !=
           NULL) {
      p->close_output(bgpcorsaro);
    }

    bgpcorsaro_plugin_manager_free(bgpcorsaro->plugin_manager);
    bgpcorsaro->plugin_manager = NULL;
  }

  if (bgpcorsaro->monitorname != NULL) {
    free(bgpcorsaro->monitorname);
    bgpcorsaro->monitorname = NULL;
  }

  if (bgpcorsaro->template != NULL) {
    free(bgpcorsaro->template);
    bgpcorsaro->template = NULL;
  }

  if (bgpcorsaro->record != NULL) {
    bgpcorsaro_record_free(bgpcorsaro->record);
    bgpcorsaro->record = NULL;
  }

  /* close this as late as possible */
  bgpcorsaro_log_close(bgpcorsaro);

  free(bgpcorsaro);

  return;
}

/** Fill the given interval object with the default values */
static inline void populate_interval(bgpcorsaro_interval_t *interval,
                                     uint32_t number, uint32_t time)
{
  interval->number = number;
  interval->time = time;
}

/** Check if the meta output files should be rotated */
static int is_meta_rotate_interval(bgpcorsaro_t *bgpcorsaro)
{
  assert(bgpcorsaro != NULL);

  if (bgpcorsaro->meta_output_rotate < 0) {
    return bgpcorsaro_is_rotate_interval(bgpcorsaro);
  } else if (bgpcorsaro->meta_output_rotate > 0 &&
             (bgpcorsaro->interval_start.number + 1) %
                 bgpcorsaro->meta_output_rotate ==
               0) {
    return 1;
  } else {
    return 0;
  }
}

/** Initialize a new bgpcorsaro object */
static bgpcorsaro_t *bgpcorsaro_init(char *template)
{
  bgpcorsaro_t *e;

  if ((e = malloc_zero(sizeof(bgpcorsaro_t))) == NULL) {
    bgpcorsaro_log(__func__, NULL, "could not malloc bgpcorsaro_t");
    return NULL;
  }

  /* what time is it? */
  gettimeofday_wrap(&e->init_time);

  /* set a default monitorname for when im bad and directly retrieve it
     from the structure */
  e->monitorname = strdup(STR(BGPCORSARO_MONITOR_NAME));

  /* template does, however */
  /* check that it is valid */
  if (bgpcorsaro_io_validate_template(e, template) == 0) {
    bgpcorsaro_log(__func__, e, "invalid template %s", template);
    goto err;
  }
  if ((e->template = strdup(template)) == NULL) {
    bgpcorsaro_log(__func__, e, "could not duplicate template string");
    goto err;
  }

  /* check if compression should be used based on the file name */
  e->compress = wandio_detect_compression_type(e->template);

  /* use the default compression level for now */
  e->compress_level = 6;

  /* lets get us a wrapper record ready */
  if ((e->record = bgpcorsaro_record_alloc(e)) == NULL) {
    bgpcorsaro_log(__func__, e, "could not create bgpcorsaro record");
    goto err;
  }

  /* ask the plugin manager to get us some plugins */
  /* this will init all compiled plugins but not start them, this gives
     us a chance to wait for the user to choose a subset to enable
     with bgpcorsaro_enable_plugin and then we'll re-init */
  if ((e->plugin_manager = bgpcorsaro_plugin_manager_init(e->logfile)) ==
      NULL) {
    bgpcorsaro_log(__func__, e, "could not initialize plugin manager");
    goto err;
  }

  /* set the default interval alignment value */
  e->interval_align = BGPCORSARO_INTERVAL_ALIGN_DEFAULT;

  /* interval doesn't need to be actively set, use the default for now */
  e->interval = BGPCORSARO_INTERVAL_DEFAULT;

  /* default for meta rotate should be to follow output_rotate */
  e->meta_output_rotate = -1;

  /* initialize the current interval */
  populate_interval(&e->interval_start, 0, 0);

  /* the rest are zero, as they should be. */

  /* ready to rock and roll! */

  return e;

err:
  /* 02/26/13 - ak comments because it is up to the user to call
     bgpcorsaro_finalize_output to free the memory */
  /*bgpcorsaro_free(e);*/
  return NULL;
}

/** Start a new interval */
static int start_interval(bgpcorsaro_t *bgpcorsaro, long int_start)
{
  bgpcorsaro_plugin_t *tmp = NULL;

  /* record this so we know when the interval started */
  /* the interval number is already incremented by end_interval */
  bgpcorsaro->interval_start.time = int_start;

  /* the following is to support file rotation */
  /* initialize the log file */
  if (bgpcorsaro->logfile_disabled == 0 && bgpcorsaro->logfile == NULL) {
    /* if this is the first interval, let them know we are switching to
       logging to a file */
    if (bgpcorsaro->interval_start.number == 0) {
      /* there is a replica of this message in the other place that
       * bgpcorsaro_log_init is called */
      bgpcorsaro_log(__func__, bgpcorsaro, "now logging to file"
#ifdef DEBUG
                                           " (and stderr)"
#endif
                     );
    }

    if (bgpcorsaro_log_init(bgpcorsaro) != 0) {
      bgpcorsaro_log(__func__, bgpcorsaro, "could not initialize log file");
      bgpcorsaro_free(bgpcorsaro);
      return -1;
    }
  }

  /* ask each plugin to start a new interval */
  /* plugins should rotate their files now too */
  while ((tmp = bgpcorsaro_plugin_next(bgpcorsaro->plugin_manager, tmp)) !=
         NULL) {
#ifdef WITH_PLUGIN_TIMING
    TIMER_START(start_interval);
#endif
    if (tmp->start_interval(bgpcorsaro, &bgpcorsaro->interval_start) != 0) {
      bgpcorsaro_log(__func__, bgpcorsaro, "%s failed to start interval at %ld",
                     tmp->name, int_start);
      return -1;
    }
#ifdef WITH_PLUGIN_TIMING
    TIMER_END(start_interval);
    tmp->start_interval_usec += TIMER_VAL(start_interval);
#endif
  }
  return 0;
}

/** End the current interval */
static int end_interval(bgpcorsaro_t *bgpcorsaro, long int_end)
{
  bgpcorsaro_plugin_t *tmp = NULL;

  bgpcorsaro_interval_t interval_end;

  populate_interval(&interval_end, bgpcorsaro->interval_start.number, int_end);

  /* ask each plugin to end the current interval */
  while ((tmp = bgpcorsaro_plugin_next(bgpcorsaro->plugin_manager, tmp)) !=
         NULL) {
#ifdef WITH_PLUGIN_TIMING
    TIMER_START(end_interval);
#endif
    if (tmp->end_interval(bgpcorsaro, &interval_end) != 0) {
      bgpcorsaro_log(__func__, bgpcorsaro, "%s failed to end interval at %ld",
                     tmp->name, int_end);
      return -1;
    }
#ifdef WITH_PLUGIN_TIMING
    TIMER_END(end_interval);
    tmp->end_interval_usec += TIMER_VAL(end_interval);
#endif
  }

  /* if we are rotating, now is the time to close our output files */
  if (is_meta_rotate_interval(bgpcorsaro)) {
    /* this MUST be the last thing closed in case any of the other things want
       to log their end-of-interval activities (e.g. the pkt cnt from writing
       the trailers */
    if (bgpcorsaro->logfile != NULL) {
      bgpcorsaro_log_close(bgpcorsaro);
    }
  }

  bgpcorsaro->interval_end_needed = 0;
  return 0;
}

/** Process the given bgpcorsaro record */
static inline int process_record(bgpcorsaro_t *bgpcorsaro,
                                 bgpcorsaro_record_t *record)
{
  bgpcorsaro_plugin_t *tmp = NULL;
  while ((tmp = bgpcorsaro_plugin_next(bgpcorsaro->plugin_manager, tmp)) !=
         NULL) {
#ifdef WITH_PLUGIN_TIMING
    TIMER_START(process_record);
#endif
    if (tmp->process_record(bgpcorsaro, record) < 0) {
      bgpcorsaro_log(__func__, bgpcorsaro, "%s failed to process record",
                     tmp->name);
      return -1;
    }
#ifdef WITH_PLUGIN_TIMING
    TIMER_END(process_record);
    tmp->process_record_usec += TIMER_VAL(process_record);
#endif
  }
  return 0;
}

/* == PUBLIC BGPCORSARO API FUNCS BELOW HERE == */

bgpcorsaro_t *bgpcorsaro_alloc_output(char *template)
{
  bgpcorsaro_t *bgpcorsaro;

  /* quick sanity check that folks aren't trying to write to stdout */
  if (template == NULL || strcmp(template, "-") == 0) {
    bgpcorsaro_log(__func__, NULL, "writing to stdout not supported");
    return NULL;
  }

  /* initialize the bgpcorsaro object */
  if ((bgpcorsaro = bgpcorsaro_init(template)) == NULL) {
    bgpcorsaro_log(__func__, NULL, "could not initialize bgpcorsaro object");
    return NULL;
  }

  /* 10/17/13 AK moves logging init to bgpcorsaro_start_output so that the user
     may call bgpcorsaro_disable_logfile after alloc */

  return bgpcorsaro;
}

int bgpcorsaro_start_output(bgpcorsaro_t *bgpcorsaro)
{
  bgpcorsaro_plugin_t *p = NULL;

  assert(bgpcorsaro != NULL);

  /* only initialize the log file if there are no time format fields in the file
     name (otherwise they will get a log file with a zero timestamp. */
  /* Note that if indeed it does have a timestamp, the initialization error
     messages will not be logged to a file. In fact nothing will be logged until
     the first record is received. */
  assert(bgpcorsaro->logfile == NULL);
  if (bgpcorsaro->logfile_disabled == 0 &&
      bgpcorsaro_io_template_has_timestamp(bgpcorsaro) == 0) {
    bgpcorsaro_log(__func__, bgpcorsaro, "now logging to file"
#ifdef DEBUG
                                         " (and stderr)"
#endif
                   );

    if (bgpcorsaro_log_init(bgpcorsaro) != 0) {
      return -1;
    }
  }

  /* ask the plugin manager to start up */
  /* this allows it to remove disabled plugins */
  if (bgpcorsaro_plugin_manager_start(bgpcorsaro->plugin_manager) != 0) {
    bgpcorsaro_log(__func__, bgpcorsaro, "could not start plugin manager");
    return -1;
  }

  /* now, ask each plugin to open its output file */
  /* we need to do this here so that the bgpcorsaro object is fully set up
     that is, the traceuri etc is set */
  while ((p = bgpcorsaro_plugin_next(bgpcorsaro->plugin_manager, p)) != NULL) {
#ifdef WITH_PLUGIN_TIMING
    TIMER_START(init_output);
#endif
    if (p->init_output(bgpcorsaro) != 0) {
      return -1;
    }
#ifdef WITH_PLUGIN_TIMING
    TIMER_END(init_output);
    p->init_output_usec += TIMER_VAL(init_output);
#endif
  }

  bgpcorsaro->started = 1;

  return 0;
}

void bgpcorsaro_set_interval_alignment(bgpcorsaro_t *bgpcorsaro,
                                       bgpcorsaro_interval_align_t align)
{
  assert(bgpcorsaro != NULL);
  /* you cant set interval alignment once bgpcorsaro has started */
  assert(bgpcorsaro->started == 0);

  bgpcorsaro_log(__func__, bgpcorsaro, "setting interval alignment to %d",
                 align);

  bgpcorsaro->interval_align = align;
}

void bgpcorsaro_set_interval(bgpcorsaro_t *bgpcorsaro, unsigned int i)
{
  assert(bgpcorsaro != NULL);
  /* you can't set the interval once bgpcorsaro has been started */
  assert(bgpcorsaro->started == 0);

  bgpcorsaro_log(__func__, bgpcorsaro, "setting interval length to %d", i);

  bgpcorsaro->interval = i;
}

void bgpcorsaro_set_output_rotation(bgpcorsaro_t *bgpcorsaro, int intervals)
{
  assert(bgpcorsaro != NULL);
  /* you can't enable rotation once bgpcorsaro has been started */
  assert(bgpcorsaro->started == 0);

  bgpcorsaro_log(__func__, bgpcorsaro,
                 "setting output rotation after %d interval(s)", intervals);

  /* if they have asked to rotate, but did not put a timestamp in the template,
   * we will end up clobbering files. warn them. */
  if (bgpcorsaro_io_template_has_timestamp(bgpcorsaro) == 0) {
    /* we skip the log and write directly out so that it is clear even if they
     * have debugging turned off */
    fprintf(stderr, "WARNING: using output rotation without any timestamp "
                    "specifiers in the template.\n");
    fprintf(stderr,
            "WARNING: output files will be overwritten upon rotation\n");
    /* @todo consider making this a fatal error */
  }

  bgpcorsaro->output_rotate = intervals;
}

void bgpcorsaro_set_meta_output_rotation(bgpcorsaro_t *bgpcorsaro,
                                         int intervals)
{
  assert(bgpcorsaro != NULL);
  /* you can't enable rotation once bgpcorsaro has been started */
  assert(bgpcorsaro->started == 0);

  bgpcorsaro_log(__func__, bgpcorsaro,
                 "setting meta output rotation after %d intervals(s)",
                 intervals);

  bgpcorsaro->meta_output_rotate = intervals;
}

int bgpcorsaro_is_rotate_interval(bgpcorsaro_t *bgpcorsaro)
{
  assert(bgpcorsaro != NULL);

  if (bgpcorsaro->output_rotate == 0) {
    return 0;
  } else if ((bgpcorsaro->interval_start.number + 1) %
               bgpcorsaro->output_rotate ==
             0) {
    return 1;
  } else {
    return 0;
  }
}

int bgpcorsaro_set_stream(bgpcorsaro_t *bgpcorsaro, bgpstream_t *stream)
{
  assert(bgpcorsaro != NULL);

  /* this function can actually be called once bgpcorsaro is started */
  if (bgpcorsaro->stream != NULL) {
    bgpcorsaro_log(__func__, bgpcorsaro, "updating stream pointer");
  } else {
    bgpcorsaro_log(__func__, bgpcorsaro, "setting stream pointer");
  }

  bgpcorsaro->stream = stream;
  return 0;
}

void bgpcorsaro_disable_logfile(bgpcorsaro_t *bgpcorsaro)
{
  assert(bgpcorsaro != NULL);
  bgpcorsaro->logfile_disabled = 1;
}

int bgpcorsaro_enable_plugin(bgpcorsaro_t *bgpcorsaro, const char *plugin_name,
                             const char *plugin_args)
{
  assert(bgpcorsaro != NULL);
  assert(bgpcorsaro->plugin_manager != NULL);

  return bgpcorsaro_plugin_enable_plugin(bgpcorsaro->plugin_manager,
                                         plugin_name, plugin_args);
}

int bgpcorsaro_get_plugin_names(char ***plugin_names)
{
  /* we hax this by creating a plugin manager, walking the list of plugins, and
   * dumping their names.
   */
  /** @todo add a 'usage' method to the plugin API so we can explicitly dump the
     usage for each plugin */

  int i = 0;
  char **names = NULL;
  bgpcorsaro_plugin_t *tmp = NULL;
  bgpcorsaro_plugin_manager_t *tmp_manager = NULL;
  if ((tmp_manager = bgpcorsaro_plugin_manager_init(NULL)) == NULL) {
    return -1;
  }

  /* initialize the array of char pointers */
  if ((names = malloc(sizeof(char *) * tmp_manager->plugins_cnt)) == NULL) {
    return -1;
  }

  while ((tmp = bgpcorsaro_plugin_next(tmp_manager, tmp)) != NULL) {
    names[i] = strdup(tmp->name);
    i++;
  }

  bgpcorsaro_plugin_manager_free(tmp_manager);

  *plugin_names = names;
  return i;
}

void bgpcorsaro_free_plugin_names(char **plugin_names, int plugin_cnt)
{
  int i;
  if (plugin_names == NULL) {
    return;
  }
  for (i = 0; i < plugin_cnt; i++) {
    if (plugin_names[i] != NULL) {
      free(plugin_names[i]);
    }
    plugin_names[i] = NULL;
  }
  free(plugin_names);
}

int bgpcorsaro_set_monitorname(bgpcorsaro_t *bgpcorsaro, char *name)
{
  assert(bgpcorsaro != NULL);

  if (bgpcorsaro->started != 0) {
    bgpcorsaro_log(__func__, bgpcorsaro, "monitor name can only be set before "
                                         "bgpcorsaro_start_output is called");
    return -1;
  }

  if (bgpcorsaro->monitorname != NULL) {
    bgpcorsaro_log(__func__, bgpcorsaro, "updating monitor name from %s to %s",
                   bgpcorsaro->monitorname, name);
  } else {
    bgpcorsaro_log(__func__, bgpcorsaro, "setting monitor name to %s", name);
  }

  if ((bgpcorsaro->monitorname = strdup(name)) == NULL) {
    bgpcorsaro_log(__func__, bgpcorsaro,
                   "could not duplicate monitor name string");
    return -1;
  }
  bgpcorsaro_log(__func__, bgpcorsaro, "%s", bgpcorsaro->monitorname);
  return 0;
}

const char *bgpcorsaro_get_monitorname(bgpcorsaro_t *bgpcorsaro)
{
  return bgpcorsaro->monitorname;
}

int bgpcorsaro_per_record(bgpcorsaro_t *bgpcorsaro,
                          bgpstream_record_t *bsrecord)
{
  long ts;
  long report;

  assert(bgpcorsaro != NULL);
  assert(bgpcorsaro->started == 1 &&
         "bgpcorsaro_start_output must be called before records can be "
         "processed");

  /* poke this bsrecord into our bgpcorsaro record */
  bgpcorsaro->record->bsrecord = bsrecord;

  /* ensure that the state is clear */
  bgpcorsaro_record_state_reset(bgpcorsaro->record);

  /* this is now the latest record we have seen */
  /** @chiara is this correct? */
  /** @chiara might be nice to provide an accessor func:
      bgpstream_get_time(record) */
  bgpcorsaro->last_ts = ts = bsrecord->attributes.record_time;

  /* it also means we need to dump an interval end record */
  bgpcorsaro->interval_end_needed = 1;

  /* if this is the first record we record, keep the timestamp */
  if (bgpcorsaro->record_cnt == 0) {
    bgpcorsaro->first_ts = ts;
    if (start_interval(bgpcorsaro, ts) != 0) {
      bgpcorsaro_log(__func__, bgpcorsaro, "could not start interval at %ld",
                     ts);
      return -1;
    }

    bgpcorsaro->next_report = ts + bgpcorsaro->interval;

    /* if we are aligning our intervals, truncate the end down */
    if (bgpcorsaro->interval_align == BGPCORSARO_INTERVAL_ALIGN_YES) {
      bgpcorsaro->next_report =
        (bgpcorsaro->next_report / bgpcorsaro->interval) * bgpcorsaro->interval;
    }
  }

  /* using an interval value of less than zero disables intervals
     such that only one distribution will be generated upon completion */
  while (bgpcorsaro->interval >= 0 && ts >= bgpcorsaro->next_report) {
    /* we want to mark the end of the interval such that all pkt times are <=
       the time of the end of the interval.
       because we deal in second granularity, we simply subtract one from the
       time */
    report = bgpcorsaro->next_report - 1;

    if (end_interval(bgpcorsaro, report) != 0) {
      bgpcorsaro_log(__func__, bgpcorsaro, "could not end interval at %ld", ts);
      /* we don't free in case the client wants to try to carry on */
      return -1;
    }

    bgpcorsaro->interval_start.number++;

    /* we now add the second back on to the time to get the start time */
    report = bgpcorsaro->next_report;
    if (start_interval(bgpcorsaro, report) != 0) {
      bgpcorsaro_log(__func__, bgpcorsaro, "could not start interval at %ld",
                     ts);
      /* we don't free in case the client wants to try to carry on */
      return -1;
    }
    bgpcorsaro->next_report += bgpcorsaro->interval;
  }

  /* count this record for our overall record count */
  bgpcorsaro->record_cnt++;

  /* ask each plugin to process this record */
  return process_record(bgpcorsaro, bgpcorsaro->record);
}

int bgpcorsaro_finalize_output(bgpcorsaro_t *bgpcorsaro)
{
#ifdef WITH_PLUGIN_TIMING
  bgpcorsaro_plugin_t *p = NULL;
  struct timeval total_end, total_diff;
  gettimeofday_wrap(&total_end);
  timeval_subtract(&total_diff, &total_end, &bgpcorsaro->init_time);
  uint64_t total_time_usec =
    ((total_diff.tv_sec * 1000000) + total_diff.tv_usec);
#endif

  if (bgpcorsaro == NULL) {
    return 0;
  }
  if (bgpcorsaro->started != 0) {
    if (bgpcorsaro->interval_end_needed != 0 &&
        end_interval(bgpcorsaro, bgpcorsaro->last_ts) != 0) {
      bgpcorsaro_log(__func__, bgpcorsaro, "could not end interval at %ld",
                     bgpcorsaro->last_ts);
      bgpcorsaro_free(bgpcorsaro);
      return -1;
    }
  }

#ifdef WITH_PLUGIN_TIMING
  fprintf(stderr, "========================================\n");
  fprintf(stderr, "Plugin Timing\n");
  while ((p = bgpcorsaro_plugin_next(bgpcorsaro->plugin_manager, p)) != NULL) {
    fprintf(stderr, "----------------------------------------\n");
    fprintf(stderr, "%s\n", p->name);
    fprintf(stderr, "\tinit_output    %" PRIu64 " (%0.2f%%)\n",
            p->init_output_usec, p->init_output_usec * 100.0 / total_time_usec);
    fprintf(stderr, "\tprocess_record %" PRIu64 " (%0.2f%%)\n",
            p->process_record_usec,
            p->process_record_usec * 100.0 / total_time_usec);
    fprintf(stderr, "\tstart_interval %" PRIu64 " (%0.2f%%)\n",
            p->start_interval_usec,
            p->start_interval_usec * 100.0 / total_time_usec);
    fprintf(stderr, "\tend_interval   %" PRIu64 " (%0.2f%%)\n",
            p->end_interval_usec,
            p->end_interval_usec * 100.0 / total_time_usec);
    fprintf(stderr, "\ttotal   %" PRIu64 " (%0.2f%%)\n",
            p->init_output_usec + p->process_record_usec +
              p->start_interval_usec + p->end_interval_usec,
            (p->init_output_usec + p->process_record_usec +
             p->start_interval_usec + p->end_interval_usec) *
              100.0 / total_time_usec);
  }
  fprintf(stderr, "========================================\n");
  fprintf(stderr, "Total Time (usec): %" PRIu64 "\n", total_time_usec);
#endif

  bgpcorsaro_free(bgpcorsaro);
  return 0;
}
