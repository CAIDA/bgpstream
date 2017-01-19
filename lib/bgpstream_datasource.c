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

#include "bgpstream_datasource.h"
#include "bgpstream_debug.h"
#include "config.h"
#include "utils.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define DATASOURCE_BLOCKING_MIN_WAIT 30
#define DATASOURCE_BLOCKING_MAX_WAIT 150

#define GET_DEFAULT_STR_VALUE(var_store, default_value)                        \
  do {                                                                         \
    if (strcmp(default_value, "not-set") == 0) {                               \
      var_store = NULL;                                                        \
    } else {                                                                   \
      var_store = strdup(default_value);                                       \
    }                                                                          \
  } while (0)

#define GET_DEFAULT_INT_VALUE(var_store, default_value)                        \
  do {                                                                         \
    if (strcmp(default_value, "not-set") == 0) {                               \
      var_store = 0;                                                           \
    } else {                                                                   \
      var_store = atoi(default_value);                                         \
    }                                                                          \
  } while (0)

/* datasource mgr related functions */

bgpstream_datasource_mgr_t *bgpstream_datasource_mgr_create()
{
  bgpstream_debug("\tBSDS_MGR: create start");
  bgpstream_datasource_mgr_t *datasource_mgr =
    (bgpstream_datasource_mgr_t *)malloc(sizeof(bgpstream_datasource_mgr_t));
  if (datasource_mgr == NULL) {
    return NULL; // can't allocate memory
  }
  // default values
  datasource_mgr->datasource =
    BGPSTREAM_DATA_INTERFACE_BROKER; // default data source
  datasource_mgr->blocking = 0;
  datasource_mgr->backoff_time = DATASOURCE_BLOCKING_MIN_WAIT;
// datasources (none of them is active at the beginning)

#ifdef WITH_DATA_INTERFACE_SINGLEFILE
  datasource_mgr->singlefile_ds = NULL;
  GET_DEFAULT_STR_VALUE(datasource_mgr->singlefile_rib_mrtfile,
                        BGPSTREAM_DS_SINGLEFILE_RIB_FILE);
  GET_DEFAULT_STR_VALUE(datasource_mgr->singlefile_upd_mrtfile,
                        BGPSTREAM_DS_SINGLEFILE_UPDATE_FILE);
#endif

#ifdef WITH_DATA_INTERFACE_CSVFILE
  datasource_mgr->csvfile_ds = NULL;
  GET_DEFAULT_STR_VALUE(datasource_mgr->csvfile_file,
                        BGPSTREAM_DS_CSVFILE_CSV_FILE);
#endif

#ifdef WITH_DATA_INTERFACE_SQLITE
  datasource_mgr->sqlite_ds = NULL;
  GET_DEFAULT_STR_VALUE(datasource_mgr->sqlite_file,
                        BGPSTREAM_DS_SQLITE_DB_FILE);
#endif

#ifdef WITH_DATA_INTERFACE_BROKER
  datasource_mgr->broker_ds = NULL;
  GET_DEFAULT_STR_VALUE(datasource_mgr->broker_url, BGPSTREAM_DS_BROKER_URL);
  datasource_mgr->broker_params = NULL;
  datasource_mgr->broker_params_cnt = 0;
#endif

  datasource_mgr->status = BGPSTREAM_DATASOURCE_STATUS_OFF;

  bgpstream_debug("\tBSDS_MGR: create end");
  return datasource_mgr;
}

void bgpstream_datasource_mgr_set_data_interface(
  bgpstream_datasource_mgr_t *datasource_mgr,
  const bgpstream_data_interface_id_t datasource)
{
  bgpstream_debug("\tBSDS_MGR: set data interface start");
  if (datasource_mgr == NULL) {
    return; // no manager
  }
  datasource_mgr->datasource = datasource;
  bgpstream_debug("\tBSDS_MGR: set  data interface end");
}

int bgpstream_datasource_mgr_set_data_interface_option(
  bgpstream_datasource_mgr_t *datasource_mgr,
  const bgpstream_data_interface_option_t *option_type,
  const char *option_value)
{
  // this option has no effect if the datasource selected is not
  // using this option
  switch (option_type->if_id) {
#ifdef WITH_DATA_INTERFACE_SINGLEFILE
  case BGPSTREAM_DATA_INTERFACE_SINGLEFILE:
    switch (option_type->id) {
    case 0:
      if (datasource_mgr->singlefile_rib_mrtfile != NULL) {
        free(datasource_mgr->singlefile_rib_mrtfile);
      }
      datasource_mgr->singlefile_rib_mrtfile = strdup(option_value);
      break;
    case 1:
      if (datasource_mgr->singlefile_upd_mrtfile != NULL) {
        free(datasource_mgr->singlefile_upd_mrtfile);
      }
      datasource_mgr->singlefile_upd_mrtfile = strdup(option_value);
      break;
    }
    break;
#endif

#ifdef WITH_DATA_INTERFACE_CSVFILE
  case BGPSTREAM_DATA_INTERFACE_CSVFILE:
    switch (option_type->id) {
    case 0:
      if (datasource_mgr->csvfile_file != NULL) {
        free(datasource_mgr->csvfile_file);
      }
      datasource_mgr->csvfile_file = strdup(option_value);
      break;
    }
    break;
#endif

#ifdef WITH_DATA_INTERFACE_SQLITE
  case BGPSTREAM_DATA_INTERFACE_SQLITE:
    switch (option_type->id) {
    case 0:
      if (datasource_mgr->sqlite_file != NULL) {
        free(datasource_mgr->sqlite_file);
      }
      datasource_mgr->sqlite_file = strdup(option_value);
      break;
    }
#endif

#ifdef WITH_DATA_INTERFACE_BROKER
  case BGPSTREAM_DATA_INTERFACE_BROKER:
    switch (option_type->id) {
    case 0:
      if (datasource_mgr->broker_url != NULL) {
        free(datasource_mgr->broker_url);
      }
      datasource_mgr->broker_url = strdup(option_value);
      break;

    case 1:
      if ((datasource_mgr->broker_params = realloc(
             datasource_mgr->broker_params,
             sizeof(char *) * (datasource_mgr->broker_params_cnt + 1))) ==
          NULL) {
        return -1;
      }
      datasource_mgr->broker_params[datasource_mgr->broker_params_cnt++] =
        strdup(option_value);
      break;
    }
    break;
#endif

  default:
    fprintf(stderr, "Invalid data interface (are all interfaces built?\n");
    return -1;
  }
  return 0;
}

void bgpstream_datasource_mgr_init(bgpstream_datasource_mgr_t *datasource_mgr,
                                   bgpstream_filter_mgr_t *filter_mgr)
{
  bgpstream_debug("\tBSDS_MGR: init start");
  if (datasource_mgr == NULL) {
    return; // no manager
  }

  void *ds = NULL;

  switch (datasource_mgr->datasource) {
#ifdef WITH_DATA_INTERFACE_SINGLEFILE
  case BGPSTREAM_DATA_INTERFACE_SINGLEFILE:
    datasource_mgr->singlefile_ds = bgpstream_singlefile_datasource_create(
      filter_mgr, datasource_mgr->singlefile_rib_mrtfile,
      datasource_mgr->singlefile_upd_mrtfile);
    ds = (void *)datasource_mgr->singlefile_ds;
    break;
#endif

#ifdef WITH_DATA_INTERFACE_CSVFILE
  case BGPSTREAM_DATA_INTERFACE_CSVFILE:
    datasource_mgr->csvfile_ds = bgpstream_csvfile_datasource_create(
      filter_mgr, datasource_mgr->csvfile_file);
    ds = (void *)datasource_mgr->csvfile_ds;
    break;
#endif

#ifdef WITH_DATA_INTERFACE_SQLITE
  case BGPSTREAM_DATA_INTERFACE_SQLITE:
    datasource_mgr->sqlite_ds = bgpstream_sqlite_datasource_create(
      filter_mgr, datasource_mgr->sqlite_file);
    ds = (void *)datasource_mgr->sqlite_ds;
    break;
#endif

#ifdef WITH_DATA_INTERFACE_BROKER
  case BGPSTREAM_DATA_INTERFACE_BROKER:
    datasource_mgr->broker_ds = bgpstream_broker_datasource_create(
      filter_mgr, datasource_mgr->broker_url, datasource_mgr->broker_params,
      datasource_mgr->broker_params_cnt);
    ds = (void *)datasource_mgr->broker_ds;
    break;
#endif

  default:
    ds = NULL;
  }

  if (ds == NULL) {
    datasource_mgr->status = BGPSTREAM_DATASOURCE_STATUS_ERROR;
  } else {
    datasource_mgr->status = BGPSTREAM_DATASOURCE_STATUS_ON;
  }
  bgpstream_debug("\tBSDS_MGR: init end");
}

void bgpstream_datasource_mgr_set_blocking(
  bgpstream_datasource_mgr_t *datasource_mgr)
{
  bgpstream_debug("\tBSDS_MGR: set blocking start");
  if (datasource_mgr == NULL) {
    return; // no manager
  }
  datasource_mgr->blocking = 1;
  bgpstream_debug("\tBSDS_MGR: set blocking end");
}

int bgpstream_datasource_mgr_update_input_queue(
  bgpstream_datasource_mgr_t *datasource_mgr, bgpstream_input_mgr_t *input_mgr)
{
  bgpstream_debug("\tBSDS_MGR: get data start");
  if (datasource_mgr == NULL) {
    return -1; // no datasource manager
  }
  int results = -1;

  do {
    switch (datasource_mgr->datasource) {
#ifdef WITH_DATA_INTERFACE_SINGLEFILE
    case BGPSTREAM_DATA_INTERFACE_SINGLEFILE:
      results = bgpstream_singlefile_datasource_update_input_queue(
        datasource_mgr->singlefile_ds, input_mgr);
      break;
#endif

#ifdef WITH_DATA_INTERFACE_CSVFILE
    case BGPSTREAM_DATA_INTERFACE_CSVFILE:
      results = bgpstream_csvfile_datasource_update_input_queue(
        datasource_mgr->csvfile_ds, input_mgr);
      break;
#endif

#ifdef WITH_DATA_INTERFACE_SQLITE
    case BGPSTREAM_DATA_INTERFACE_SQLITE:
      results = bgpstream_sqlite_datasource_update_input_queue(
        datasource_mgr->sqlite_ds, input_mgr);
      break;
#endif

#ifdef WITH_DATA_INTERFACE_BROKER
    case BGPSTREAM_DATA_INTERFACE_BROKER:
      results = bgpstream_broker_datasource_update_input_queue(
        datasource_mgr->broker_ds, input_mgr);
      break;
#endif

    default:
      fprintf(stderr, "Invalid data interface\n");
      break;
    }
    if (results == 0 && datasource_mgr->blocking) {
      // results = 0 => 2+ time and database did not give any error
      sleep(datasource_mgr->backoff_time);
      datasource_mgr->backoff_time = datasource_mgr->backoff_time * 2;
      if (datasource_mgr->backoff_time > DATASOURCE_BLOCKING_MAX_WAIT) {
        datasource_mgr->backoff_time = DATASOURCE_BLOCKING_MAX_WAIT;
      }
    }
    bgpstream_debug("\tBSDS_MGR: got %d (blocking: %d)", results,
                    datasource_mgr->blocking);
  } while (datasource_mgr->blocking && results == 0);

  datasource_mgr->backoff_time = DATASOURCE_BLOCKING_MIN_WAIT;

  bgpstream_debug("\tBSDS_MGR: get data end");
  return results;
}

void bgpstream_datasource_mgr_close(bgpstream_datasource_mgr_t *datasource_mgr)
{
  bgpstream_debug("\tBSDS_MGR: close start");
  if (datasource_mgr == NULL) {
    return; // no manager to destroy
  }
  switch (datasource_mgr->datasource) {
#ifdef WITH_DATA_INTERFACE_SINGLEFILE
  case BGPSTREAM_DATA_INTERFACE_SINGLEFILE:
    bgpstream_singlefile_datasource_destroy(datasource_mgr->singlefile_ds);
    datasource_mgr->singlefile_ds = NULL;
    break;
#endif

#ifdef WITH_DATA_INTERFACE_CSVFILE
  case BGPSTREAM_DATA_INTERFACE_CSVFILE:
    bgpstream_csvfile_datasource_destroy(datasource_mgr->csvfile_ds);
    datasource_mgr->csvfile_ds = NULL;
    break;
#endif

#ifdef WITH_DATA_INTERFACE_SQLITE
  case BGPSTREAM_DATA_INTERFACE_SQLITE:
    bgpstream_sqlite_datasource_destroy(datasource_mgr->sqlite_ds);
    datasource_mgr->sqlite_ds = NULL;
    break;
#endif

#ifdef WITH_DATA_INTERFACE_BROKER
  case BGPSTREAM_DATA_INTERFACE_BROKER:
    bgpstream_broker_datasource_destroy(datasource_mgr->broker_ds);
    datasource_mgr->broker_ds = NULL;
    break;
#endif

  default:
    assert(0);
    break;
  }
  datasource_mgr->status = BGPSTREAM_DATASOURCE_STATUS_OFF;
  bgpstream_debug("\tBSDS_MGR: close end");
}

void bgpstream_datasource_mgr_destroy(
  bgpstream_datasource_mgr_t *datasource_mgr)
{
  bgpstream_debug("\tBSDS_MGR: destroy start");
  if (datasource_mgr == NULL) {
    return; // no manager to destroy
  }
// destroy any active datasource (if they have not been destroyed before)
#ifdef WITH_DATA_INTERFACE_SINGLEFILE
  bgpstream_singlefile_datasource_destroy(datasource_mgr->singlefile_ds);
  datasource_mgr->singlefile_ds = NULL;
  free(datasource_mgr->singlefile_rib_mrtfile);
  free(datasource_mgr->singlefile_upd_mrtfile);
#endif

#ifdef WITH_DATA_INTERFACE_CSVFILE
  bgpstream_csvfile_datasource_destroy(datasource_mgr->csvfile_ds);
  datasource_mgr->csvfile_ds = NULL;
  free(datasource_mgr->csvfile_file);
#endif

#ifdef WITH_DATA_INTERFACE_SQLITE
  bgpstream_sqlite_datasource_destroy(datasource_mgr->sqlite_ds);
  datasource_mgr->sqlite_ds = NULL;
  free(datasource_mgr->sqlite_file);
#endif

#ifdef WITH_DATA_INTERFACE_BROKER
  bgpstream_broker_datasource_destroy(datasource_mgr->broker_ds);
  datasource_mgr->broker_ds = NULL;
  free(datasource_mgr->broker_url);
  int i;
  for (i = 0; i < datasource_mgr->broker_params_cnt; i++) {
    free(datasource_mgr->broker_params[i]);
    datasource_mgr->broker_params[i] = NULL;
  }
  free(datasource_mgr->broker_params);
  datasource_mgr->broker_params = NULL;
  datasource_mgr->broker_params_cnt = 0;
#endif

  free(datasource_mgr);
  bgpstream_debug("\tBSDS_MGR: destroy end");
}
