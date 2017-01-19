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

#include "bgpstream_datasource_sqlite.h"
#include "bgpstream_debug.h"
#include "utils.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_QUERY_LEN 2048
#define MAX_INTERVAL_LEN 16

#define APPEND_STR(str)                                                        \
  do {                                                                         \
    size_t len = strlen(str);                                                  \
    if (rem_buf_space < len + 1) {                                             \
      return NULL;                                                             \
    }                                                                          \
    strncat(sqlite_ds->sql_query, str, rem_buf_space);                         \
    rem_buf_space -= len;                                                      \
  } while (0)

struct struct_bgpstream_sqlite_datasource_t {
  bgpstream_filter_mgr_t *filter_mgr;
  /* sqlite connection handler */
  sqlite3 *db;
  sqlite3_stmt *stmt;
  char sql_query[MAX_QUERY_LEN];
  char *sqlite_file;
  uint32_t current_ts;
  uint32_t last_ts;
};

static int prepare_db(bgpstream_sqlite_datasource_t *sqlite_ds)
{
  assert(sqlite_ds);
  int rc = 0;
  if (sqlite3_open_v2(sqlite_ds->sqlite_file, &sqlite_ds->db,
                      SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
    bgpstream_log_err("\t\tBSDS_SQLITE: can't open database: %s",
                      sqlite3_errmsg(sqlite_ds->db));
    sqlite3_close(sqlite_ds->db);
    return -1;
  }

  rc = sqlite3_prepare_v2(sqlite_ds->db, sqlite_ds->sql_query, -1,
                          &sqlite_ds->stmt, NULL);
  if (rc != SQLITE_OK) {
    bgpstream_log_err("\t\tBSDS_SQLITE: failed to execute statement: %s",
                      sqlite3_errmsg(sqlite_ds->db));
    return -1;
  }
  return 0;
}

bgpstream_sqlite_datasource_t *
bgpstream_sqlite_datasource_create(bgpstream_filter_mgr_t *filter_mgr,
                                   char *sqlite_file)
{

  bgpstream_debug("\t\tBSDS_SQLITE: create sqlite_ds start");
  bgpstream_sqlite_datasource_t *sqlite_ds =
    (bgpstream_sqlite_datasource_t *)malloc_zero(
      sizeof(bgpstream_sqlite_datasource_t));
  if (sqlite_ds == NULL) {
    bgpstream_log_err(
      "\t\tBSDS_SQLITE: create sqlite_ds can't allocate memory");
    goto err;
  }
  if (sqlite_file == NULL) {
    bgpstream_log_err("\t\tBSDS_SQLITE: create sqlite_ds no file provided");
    goto err;
  }
  sqlite_ds->sqlite_file = strdup(sqlite_file);

  sqlite_ds->filter_mgr = filter_mgr;
  sqlite_ds->current_ts = 0;
  sqlite_ds->last_ts = 0;

  /* how many characters can be written in the query buffer */
  size_t rem_buf_space = MAX_QUERY_LEN;
  sqlite_ds->sql_query[0] = '\0';
  char interval_str[MAX_INTERVAL_LEN];

  APPEND_STR(
    "SELECT bgp_data.file_path, collectors.project, collectors.name, "
    "bgp_types.name, time_span.time_span, bgp_data.file_time, bgp_data.ts "
    "FROM  collectors JOIN bgp_data JOIN bgp_types JOIN time_span "
    "WHERE bgp_data.collector_id = collectors.id  AND "
    "bgp_data.collector_id = time_span.collector_id AND "
    "bgp_data.type_id = bgp_types.id AND "
    "bgp_data.type_id = time_span.bgp_type_id ");

  // projects, collectors, bgp_types, and time_intervals are used as filters
  // only if they are provided by the user
  bgpstream_interval_filter_t *tif;
  bool first;
  char *f;

  // projects
  first = true;
  if (filter_mgr->projects != NULL) {
    APPEND_STR(" AND collectors.project IN (");
    bgpstream_str_set_rewind(filter_mgr->projects);
    while ((f = bgpstream_str_set_next(filter_mgr->projects)) != NULL) {
      if (!first) {
        APPEND_STR(", ");
      }
      APPEND_STR("'");
      APPEND_STR(f);
      APPEND_STR("'");
      first = false;
    }
    APPEND_STR(" ) ");
  }

  // collectors
  first = true;
  if (filter_mgr->collectors != NULL) {
    APPEND_STR(" AND collectors.name IN (");
    bgpstream_str_set_rewind(filter_mgr->collectors);
    while ((f = bgpstream_str_set_next(filter_mgr->collectors)) != NULL) {
      if (!first) {
        APPEND_STR(", ");
      }
      APPEND_STR("'");
      APPEND_STR(f);
      APPEND_STR("'");
      first = false;
    }
    APPEND_STR(" ) ");
  }

  // bgp_types
  first = true;
  if (filter_mgr->bgp_types != NULL) {
    APPEND_STR(" AND bgp_types.name IN (");
    bgpstream_str_set_rewind(filter_mgr->bgp_types);
    while ((f = bgpstream_str_set_next(filter_mgr->bgp_types)) != NULL) {
      if (!first) {
        APPEND_STR(", ");
      }
      APPEND_STR("'");
      APPEND_STR(f);
      APPEND_STR("'");
      first = false;
    }
    APPEND_STR(" ) ");
  }

  // time_intervals
  int written = 0;
  if (filter_mgr->time_intervals != NULL) {
    tif = filter_mgr->time_intervals;
    APPEND_STR(" AND ( ");

    while (tif != NULL) {
      APPEND_STR(" ( ");

      // BEGIN TIME
      APPEND_STR(" (bgp_data.file_time >=  ");
      interval_str[0] = '\0';
      if ((written = snprintf(interval_str, MAX_INTERVAL_LEN, "%" PRIu32,
                              tif->begin_time)) < MAX_INTERVAL_LEN) {
        APPEND_STR(interval_str);
      }
      APPEND_STR("  - time_span.time_span - 120 )");
      APPEND_STR("  AND  ");

      // END TIME
      if (tif->end_time != BGPSTREAM_FOREVER) {
        APPEND_STR(" (bgp_data.file_time <=  ");
        interval_str[0] = '\0';
        if ((written = snprintf(interval_str, MAX_INTERVAL_LEN, "%" PRIu32,
                                tif->end_time)) < MAX_INTERVAL_LEN) {
          APPEND_STR(interval_str);
        }
        APPEND_STR(") ");
        APPEND_STR(" ) ");
      }

      tif = tif->next;
      if (tif != NULL) {
        APPEND_STR(" OR ");
      }
    }
    APPEND_STR(" )");
  }

  /*  comment on 120 seconds: */
  /*  sometimes it happens that ribs or updates carry a filetime which is not */
  /*  compliant with the expected filetime (e.g. : */
  /*   rib.23.59 instead of rib.00.00 */
  /*  in order to compensate for this kind of situations we  */
  /*  retrieve data that are 120 seconds older than the requested  */

  // minimum timestamp and current timestamp are the two placeholders
  APPEND_STR(" AND bgp_data.ts > ? AND bgp_data.ts <= ?");
  // order by filetime and bgptypes in reverse order: this way the
  // input insertions are always "head" insertions, i.e. queue insertion is
  // faster
  APPEND_STR(" ORDER BY file_time DESC, bgp_types.name DESC");

  if (prepare_db(sqlite_ds) != 0) {
    goto err;
  }

  // printf("%s\n", sqlite_ds->sql_query);

  bgpstream_debug("\t\tBSDS_SQLITE: create sqlite_ds end");

  return sqlite_ds;
err:
  bgpstream_sqlite_datasource_destroy(sqlite_ds);
  return NULL;
}

int bgpstream_sqlite_datasource_update_input_queue(
  bgpstream_sqlite_datasource_t *sqlite_ds, bgpstream_input_mgr_t *input_mgr)
{
  int rc;
  int num_results = 0;
  sqlite_ds->last_ts = sqlite_ds->current_ts;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  // update current_timestamp - we always ask for data 1 second old at least
  sqlite_ds->current_ts = tv.tv_sec - 1; // now() - 1 second

  sqlite3_bind_int(sqlite_ds->stmt, 1, sqlite_ds->last_ts);
  sqlite3_bind_int(sqlite_ds->stmt, 2, sqlite_ds->current_ts);

  /* printf("%d - %d \n", sqlite_ds->last_ts, sqlite_ds->current_ts); */
  while ((rc = sqlite3_step(sqlite_ds->stmt)) != SQLITE_DONE) {
    if (rc == SQLITE_ROW) {
      /* printf("%s: %d\n", sqlite3_column_text(sqlite_ds->stmt, 0),
       * sqlite3_column_int(sqlite_ds->stmt, 6)); */
      num_results += bgpstream_input_mgr_push_sorted_input(
        input_mgr, strdup((const char *)sqlite3_column_text(sqlite_ds->stmt,
                                                            0)) /* path */,
        strdup(
          (const char *)sqlite3_column_text(sqlite_ds->stmt, 1)) /* project */,
        strdup((const char *)sqlite3_column_text(sqlite_ds->stmt,
                                                 2)) /* collector */,
        strdup(
          (const char *)sqlite3_column_text(sqlite_ds->stmt, 3)) /* type */,
        sqlite3_column_int(sqlite_ds->stmt, 5) /* file time */,
        sqlite3_column_int(sqlite_ds->stmt, 4) /* time span */);

    } else {
      bgpstream_log_err(
        "\t\tBSDS_SQLITE: error while stepping through results");
      return -1;
    }
  }
  sqlite3_reset(sqlite_ds->stmt);
  return num_results;
}

void bgpstream_sqlite_datasource_destroy(
  bgpstream_sqlite_datasource_t *sqlite_ds)
{
  if (sqlite_ds != NULL) {
    if (sqlite_ds->sqlite_file != NULL) {
      free(sqlite_ds->sqlite_file);
      sqlite_ds->sqlite_file = NULL;
    }

    sqlite3_finalize(sqlite_ds->stmt);
    sqlite3_close(sqlite_ds->db);
    free(sqlite_ds);
  }
}
