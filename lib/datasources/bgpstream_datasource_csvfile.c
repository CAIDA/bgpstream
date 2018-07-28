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

#include "bgpstream_datasource_csvfile.h"
#include "bgpstream_debug.h"
#include "config.h"
#include "utils.h"
#include "libcsv/csv.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <wandio.h>

#define BUFFER_LEN 1024

typedef enum {

  CSVFILE_PATH = 0,
  CSVFILE_PROJECT = 1,
  CSVFILE_BGPTYPE = 2,
  CSVFILE_COLLECTOR = 3,
  CSVFILE_FILETIME = 4,
  CSVFILE_TIMESPAN = 5,
  CSVFILE_TIMESTAMP = 6,

  CSVFILE_FIELDCNT = 7
} csvfile_field_t;

struct struct_bgpstream_csvfile_datasource_t {
  char *csvfile_file;
  struct csv_parser parser;
  int current_field;
  int num_results;
  bgpstream_filter_mgr_t *filter_mgr;
  bgpstream_input_mgr_t *input_mgr;

  /* bgp record metadata */
  char filename[BGPSTREAM_DUMP_MAX_LEN];
  char project[BGPSTREAM_PAR_MAX_LEN];
  char bgp_type[BGPSTREAM_PAR_MAX_LEN];
  char collector[BGPSTREAM_PAR_MAX_LEN];
  uint32_t filetime;
  uint32_t time_span;
  uint32_t timestamp;

  /* maximum timestamp processed in the current file */
  uint32_t max_ts_infile;
  /* maximum timestamp processed in the past file */
  uint32_t last_processed_ts;
  /* maximum timestamp accepted in the current round */
  uint32_t max_accepted_ts;
};

bgpstream_csvfile_datasource_t *
bgpstream_csvfile_datasource_create(bgpstream_filter_mgr_t *filter_mgr,
                                    char *csvfile_file)
{
  bgpstream_debug("\t\tBSDS_CSVFILE: create csvfile_ds start");
  bgpstream_csvfile_datasource_t *csvfile_ds =
    (bgpstream_csvfile_datasource_t *)malloc_zero(
      sizeof(bgpstream_csvfile_datasource_t));
  if (csvfile_ds == NULL) {
    bgpstream_log_err(
      "\t\tBSDS_CSVFILE: create csvfile_ds can't allocate memory");
    goto err;
  }
  if (csvfile_file == NULL) {
    bgpstream_log_err("\t\tBSDS_CSVFILE: create csvfile_ds no file provided");
    goto err;
  }
  if ((csvfile_ds->csvfile_file = strdup(csvfile_file)) == NULL) {
    bgpstream_log_err("\t\tBSDS_CSVFILE: can't allocate memory for filename");
    goto err;
  }

  /* cvs file parser options */
  unsigned char options = CSV_STRICT | CSV_REPALL_NL | CSV_STRICT_FINI |
                          CSV_APPEND_NULL | CSV_EMPTY_IS_NULL;

  if (csv_init(&(csvfile_ds->parser), options) != 0) {
    bgpstream_log_err("\t\tBSDS_CSVFILE: can't initialize csv parser");
    goto err;
  }

  csvfile_ds->current_field = CSVFILE_PATH;

  csvfile_ds->filter_mgr = filter_mgr;
  csvfile_ds->input_mgr = NULL;

  csvfile_ds->num_results = 0;

  csvfile_ds->max_ts_infile = 0;
  csvfile_ds->last_processed_ts = 0;
  csvfile_ds->max_accepted_ts = 0;

  bgpstream_debug("\t\tBSDS_CSVFILE: create csvfile_ds end");
  return csvfile_ds;

err:
  bgpstream_csvfile_datasource_destroy(csvfile_ds);
  return NULL;
}

static bool bgpstream_csvfile_datasource_filter_ok(
  bgpstream_csvfile_datasource_t *csvfile_ds)
{
  bgpstream_debug("\t\tBSDS_CSVFILE: csvfile_ds apply filter start");

  /* fprintf(stderr, "%s %s %s %s\n", */
  /*         csvfile_ds->filename, csvfile_ds->project, csvfile_ds->collector,
   * csvfile_ds->bgp_type); */

  bgpstream_interval_filter_t *tif;
  bool all_false;

  char *f;

  // projects
  all_false = true;
  if (csvfile_ds->filter_mgr->projects != NULL) {
    bgpstream_str_set_rewind(csvfile_ds->filter_mgr->projects);
    while ((f = bgpstream_str_set_next(csvfile_ds->filter_mgr->projects)) !=
           NULL) {
      if (strcmp(f, csvfile_ds->project) == 0) {
        all_false = false;
        break;
      }
    }
    if (all_false) {
      return false;
    }
  }
  // collectors
  all_false = true;
  if (csvfile_ds->filter_mgr->collectors != NULL) {
    bgpstream_str_set_rewind(csvfile_ds->filter_mgr->collectors);
    while ((f = bgpstream_str_set_next(csvfile_ds->filter_mgr->collectors)) !=
           NULL) {
      if (strcmp(f, csvfile_ds->collector) == 0) {
        all_false = false;
        break;
      }
    }
    if (all_false) {
      return false;
    }
  }

  // bgp_types
  all_false = true;
  if (csvfile_ds->filter_mgr->bgp_types != NULL) {
    bgpstream_str_set_rewind(csvfile_ds->filter_mgr->bgp_types);
    while ((f = bgpstream_str_set_next(csvfile_ds->filter_mgr->bgp_types)) !=
           NULL) {
      if (strcmp(f, csvfile_ds->bgp_type) == 0) {
        all_false = false;
        break;
      }
    }
    if (all_false) {
      return false;
    }
  }

  // time_intervals
  all_false = true;
  if (csvfile_ds->filter_mgr->time_intervals != NULL) {
    tif = csvfile_ds->filter_mgr->time_intervals;
    while (tif != NULL) {
      // filetime (we consider 15 mins before to consider routeviews updates
      // and 120 seconds to have some margins)
      if (csvfile_ds->filetime >= (tif->begin_time - 15 * 60 - 120) &&
          (tif->end_time == BGPSTREAM_FOREVER ||
           csvfile_ds->filetime <= tif->end_time)) {
        all_false = false;
        break;
      }
      tif = tif->next;
    }
    if (all_false) {
      return false;
    }
  }
  // if all the filters are passed
  return true;
}

static void parse_csvfile_field(void *field, size_t i, void *user_data)
{

  char *field_str = (char *)field;
  bgpstream_csvfile_datasource_t *csvfile_ds =
    (bgpstream_csvfile_datasource_t *)user_data;

  /* fprintf(stderr, "%s\n", field_str); */

  switch (csvfile_ds->current_field) {
  case CSVFILE_PATH:
    assert(i < BGPSTREAM_DUMP_MAX_LEN - 1);
    strncpy(csvfile_ds->filename, field_str, i);
    csvfile_ds->filename[i] = '\0';
    break;
  case CSVFILE_PROJECT:
    assert(i < BGPSTREAM_PAR_MAX_LEN - 1);
    strncpy(csvfile_ds->project, field_str, i);
    csvfile_ds->project[i] = '\0';
    break;
  case CSVFILE_BGPTYPE:
    assert(i < BGPSTREAM_PAR_MAX_LEN - 1);
    strncpy(csvfile_ds->bgp_type, field_str, i);
    csvfile_ds->bgp_type[i] = '\0';
    break;
  case CSVFILE_COLLECTOR:
    assert(i < BGPSTREAM_PAR_MAX_LEN - 1);
    strncpy(csvfile_ds->collector, field_str, i);
    csvfile_ds->collector[i] = '\0';
    break;
  case CSVFILE_FILETIME:
    csvfile_ds->filetime = atoi(field_str);
    break;
  case CSVFILE_TIMESPAN:
    csvfile_ds->time_span = atoi(field_str);
    break;
  case CSVFILE_TIMESTAMP:
    csvfile_ds->timestamp = atoi(field_str);
    break;
  }

  /* one more field read */
  csvfile_ds->current_field++;
}

static void parse_csvfile_rowend(int c, void *user_data)
{
  bgpstream_csvfile_datasource_t *csvfile_ds =
    (bgpstream_csvfile_datasource_t *)user_data;

  /* if the number of fields read is compliant with the expected file format */
  if (csvfile_ds->current_field == CSVFILE_FIELDCNT) {
    /* check if the timestamp is acceptable */
    if (csvfile_ds->timestamp > csvfile_ds->last_processed_ts &&
        csvfile_ds->timestamp <= csvfile_ds->max_accepted_ts) {
      /* update max in file timestamp */
      if (csvfile_ds->timestamp > csvfile_ds->max_ts_infile) {
        csvfile_ds->max_ts_infile = csvfile_ds->timestamp;
      }
      if (bgpstream_csvfile_datasource_filter_ok(csvfile_ds)) {
        csvfile_ds->num_results += bgpstream_input_mgr_push_sorted_input(
          csvfile_ds->input_mgr, strdup(csvfile_ds->filename),
          strdup(csvfile_ds->project), strdup(csvfile_ds->collector),
          strdup(csvfile_ds->bgp_type), csvfile_ds->filetime,
          csvfile_ds->time_span);
      }
    }
  }
  csvfile_ds->current_field = 0;
}

int bgpstream_csvfile_datasource_update_input_queue(
  bgpstream_csvfile_datasource_t *csvfile_ds, bgpstream_input_mgr_t *input_mgr)
{
  bgpstream_debug("\t\tBSDS_CSVFILE: csvfile_ds update input queue start");

  io_t *file_io = NULL;
  char buffer[BUFFER_LEN];
  int read = 0;

  struct timeval tv;
  gettimeofday(&tv, NULL);

  /* we accept all timestamp earlier than now() - 1 second */
  csvfile_ds->max_accepted_ts = tv.tv_sec - 1;

  csvfile_ds->num_results = 0;
  csvfile_ds->max_ts_infile = 0;
  csvfile_ds->input_mgr = input_mgr;

  if ((file_io = wandio_create(csvfile_ds->csvfile_file)) == NULL) {
    bgpstream_log_err("\t\tBSDS_CSVFILE: create csvfile_ds can't open file %s",
                      csvfile_ds->csvfile_file);
    return -1;
  }

  while ((read = wandio_read(file_io, &buffer, BUFFER_LEN)) > 0) {
    if (csv_parse(&(csvfile_ds->parser), buffer, read, parse_csvfile_field,
                  parse_csvfile_rowend, csvfile_ds) != read) {
      bgpstream_log_err("\t\tBSDS_CSVFILE: CSV error %s",
                        csv_strerror(csv_error(&(csvfile_ds->parser))));
      return -1;
    }
  }

  if (csv_fini(&(csvfile_ds->parser), parse_csvfile_field, parse_csvfile_rowend,
               csvfile_ds) != 0) {
    bgpstream_log_err("\t\tBSDS_CSVFILE: CSV error %s",
                      csv_strerror(csv_error(&(csvfile_ds->parser))));
    return -1;
  }

  wandio_destroy(file_io);
  csvfile_ds->input_mgr = NULL;
  csvfile_ds->last_processed_ts = csvfile_ds->max_ts_infile;

  bgpstream_debug("\t\tBSDS_CSVFILE: csvfile_ds update input queue end");
  return csvfile_ds->num_results;
}

void bgpstream_csvfile_datasource_destroy(
  bgpstream_csvfile_datasource_t *csvfile_ds)
{
  bgpstream_debug("\t\tBSDS_CSVFILE: destroy csvfile_ds start");
  if (csvfile_ds == NULL) {
    return; // nothing to destroy
  }
  csvfile_ds->filter_mgr = NULL;
  if (csvfile_ds->csvfile_file != NULL) {
    free(csvfile_ds->csvfile_file);
  }
  if (&(csvfile_ds->parser) != NULL) {
    csv_free(&(csvfile_ds->parser));
  }
  free(csvfile_ds);
  bgpstream_debug("\t\tBSDS_CSVFILE: destroy csvfile_ds end");
}
