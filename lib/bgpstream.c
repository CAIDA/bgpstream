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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "bgpstream_int.h"
#include "bgpdump_lib.h"
#include "bgpstream_debug.h"
#include "utils.h"

/* TEMPORARY STRUCTURES TO FAKE DATA INTERFACE PLUGIN API */

/* this should be the complete list of interface types */
static bgpstream_data_interface_id_t bgpstream_data_interfaces[] = {
  BGPSTREAM_DATA_INTERFACE_BROKER, BGPSTREAM_DATA_INTERFACE_SINGLEFILE,
  BGPSTREAM_DATA_INTERFACE_CSVFILE, BGPSTREAM_DATA_INTERFACE_SQLITE,
};

#ifdef WITH_DATA_INTERFACE_SINGLEFILE
static bgpstream_data_interface_info_t bgpstream_singlefile_info = {
  BGPSTREAM_DATA_INTERFACE_SINGLEFILE, "singlefile",
  "Read a single mrt data file (a RIB and/or an update)",
};
#endif

#ifdef WITH_DATA_INTERFACE_CSVFILE
static bgpstream_data_interface_info_t bgpstream_csvfile_info = {
  BGPSTREAM_DATA_INTERFACE_CSVFILE, "csvfile",
  "Retrieve metadata information from a csv file",
};
#endif

#ifdef WITH_DATA_INTERFACE_SQLITE
static bgpstream_data_interface_info_t bgpstream_sqlite_info = {
  BGPSTREAM_DATA_INTERFACE_SQLITE, "sqlite",
  "Retrieve metadata information from a sqlite database",
};
#endif

#ifdef WITH_DATA_INTERFACE_BROKER
static bgpstream_data_interface_info_t bgpstream_broker_info = {
  BGPSTREAM_DATA_INTERFACE_BROKER, "broker",
  "Retrieve metadata information from the BGPStream Broker service",
};
#endif

static bgpstream_data_interface_info_t *bgpstream_data_interface_infos[] = {
  NULL, /* NO VALID IF WITH ID 0 */

#ifdef WITH_DATA_INTERFACE_BROKER
  &bgpstream_broker_info,
#else
  NULL,
#endif

#ifdef WITH_DATA_INTERFACE_SINGLEFILE
  &bgpstream_singlefile_info,
#else
  NULL,
#endif

#ifdef WITH_DATA_INTERFACE_CSVFILE
  &bgpstream_csvfile_info,
#else
  NULL,
#endif

#ifdef WITH_DATA_INTERFACE_SQLITE
  &bgpstream_sqlite_info,
#else
  NULL,
#endif

};

/* this should be a complete list of per-interface options */

#ifdef WITH_DATA_INTERFACE_SINGLEFILE
static bgpstream_data_interface_option_t bgpstream_singlefile_options[] = {
  /* RIB MRT file path */
  {
    BGPSTREAM_DATA_INTERFACE_SINGLEFILE, 0, "rib-file",
    "rib mrt file to read (default: " STR(BGPSTREAM_DS_SINGLEFILE_RIB_FILE) ")",
  },
  {
    BGPSTREAM_DATA_INTERFACE_SINGLEFILE, 1, "upd-file",
    "updates mrt file to read (default: " STR(
      BGPSTREAM_DS_SINGLEFILE_UPDATE_FILE) ")",
  },
};
#endif

#ifdef WITH_DATA_INTERFACE_CSVFILE
static bgpstream_data_interface_option_t bgpstream_csvfile_options[] = {
  /* CSV file name */
  {
    BGPSTREAM_DATA_INTERFACE_CSVFILE, 0, "csv-file",
    "csv file listing the mrt data to read (default: " STR(
      BGPSTREAM_DS_CSVFILE_CSV_FILE) ")",
  },
};
#endif

#ifdef WITH_DATA_INTERFACE_SQLITE
static bgpstream_data_interface_option_t bgpstream_sqlite_options[] = {
  /* SQLITE database file name */
  {
    BGPSTREAM_DATA_INTERFACE_SQLITE, 0, "db-file",
    "sqlite database (default: " STR(BGPSTREAM_DS_SQLITE_DB_FILE) ")",
  },
};
#endif

#ifdef WITH_DATA_INTERFACE_BROKER
static bgpstream_data_interface_option_t bgpstream_broker_options[] = {
  /* Broker URL */
  {
    BGPSTREAM_DATA_INTERFACE_BROKER, 0, "url",
    "Broker URL (default: " STR(BGPSTREAM_DS_BROKER_URL) ")",
  },
  /* Broker Param */
  {
    BGPSTREAM_DATA_INTERFACE_BROKER, 1, "param",
    "Additional Broker GET parameter*",
  },
};
#endif

/* allocate memory for a new bgpstream interface
 */
bgpstream_t *bgpstream_create()
{
  bgpstream_debug("BS: create start");
  bgpstream_t *bs = (bgpstream_t *)malloc(sizeof(bgpstream_t));
  if (bs == NULL) {
    return NULL; // can't allocate memory
  }
  bs->filter_mgr = bgpstream_filter_mgr_create();
  if (bs->filter_mgr == NULL) {
    bgpstream_destroy(bs);
    bs = NULL;
    return NULL;
  }
  bs->datasource_mgr = bgpstream_datasource_mgr_create();
  if (bs->datasource_mgr == NULL) {
    bgpstream_destroy(bs);
    return NULL;
  }
  /* create an empty input mgr
   * the input queue will be populated when a
   * bgpstream record is requested */
  bs->input_mgr = bgpstream_input_mgr_create();
  if (bs->input_mgr == NULL) {
    bgpstream_destroy(bs);
    bs = NULL;
    return NULL;
  }
  bs->reader_mgr = bgpstream_reader_mgr_create(bs->filter_mgr);
  if (bs->reader_mgr == NULL) {
    bgpstream_destroy(bs);
    bs = NULL;
    return NULL;
  }
  /* memory for the bgpstream interface has been
   * allocated correctly */
  bs->status = BGPSTREAM_STATUS_ALLOCATED;
  bgpstream_debug("BS: create end");
  return bs;
}
/* side note: filters are part of the bgpstream so they
 * can be accessed both from the input_mgr and the
 * reader_mgr (input_mgr use them to apply a coarse-grained
 * filtering, the reader_mgr applies a fine-grained filtering
 * of the data provided by the input_mgr)
 */

/* configure filters in order to select a subset of the bgp data available */
void bgpstream_add_filter(bgpstream_t *bs, bgpstream_filter_type_t filter_type,
                          const char *filter_value)
{
  bgpstream_debug("BS: set_filter start");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ALLOCATED)) {
    return; // nothing to customize
  }
  bgpstream_filter_mgr_filter_add(bs->filter_mgr, filter_type, filter_value);
  bgpstream_debug("BS: set_filter end");
}

void bgpstream_add_rib_period_filter(bgpstream_t *bs, uint32_t period)
{
  bgpstream_debug("BS: set_filter start");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ALLOCATED)) {
    return; // nothing to customize
  }
  bgpstream_filter_mgr_rib_period_filter_add(bs->filter_mgr, period);
  bgpstream_debug("BS: set_filter end");
}

void bgpstream_add_recent_interval_filter(bgpstream_t *bs, const char *interval,
                                          uint8_t islive)
{

  uint32_t starttime, endtime;
  bgpstream_debug("BS: set_filter start");

  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ALLOCATED)) {
    return; // nothing to customize
  }

  if (bgpstream_time_calc_recent_interval(&starttime, &endtime, interval) ==
      0) {
    bgpstream_log_err("Failed to determine suitable time interval");
    return;
  }

  if (islive) {
    bgpstream_set_live_mode(bs);
    endtime = BGPSTREAM_FOREVER;
  }

  bgpstream_filter_mgr_interval_filter_add(bs->filter_mgr, starttime, endtime);
  bgpstream_debug("BS: set_filter end");
}

void bgpstream_add_interval_filter(bgpstream_t *bs, uint32_t begin_time,
                                   uint32_t end_time)
{
  bgpstream_debug("BS: set_filter start");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ALLOCATED)) {
    return; // nothing to customize
  }
  if (end_time == BGPSTREAM_FOREVER) {
    bgpstream_set_live_mode(bs);
  }
  bgpstream_filter_mgr_interval_filter_add(bs->filter_mgr, begin_time,
                                           end_time);
  bgpstream_debug("BS: set_filter end");
}

int bgpstream_get_data_interfaces(bgpstream_t *bs,
                                  bgpstream_data_interface_id_t **if_ids)
{
  assert(if_ids != NULL);
  *if_ids = bgpstream_data_interfaces;
  return ARR_CNT(bgpstream_data_interfaces);
}

bgpstream_data_interface_id_t
bgpstream_get_data_interface_id_by_name(bgpstream_t *bs, const char *name)
{
  int i;

  for (i = 1; i < ARR_CNT(bgpstream_data_interface_infos); i++) {
    if (bgpstream_data_interface_infos[i] != NULL &&
        strcmp(bgpstream_data_interface_infos[i]->name, name) == 0) {
      return bgpstream_data_interface_infos[i]->id;
    }
  }

  return 0;
}

bgpstream_data_interface_info_t *
bgpstream_get_data_interface_info(bgpstream_t *bs,
                                  bgpstream_data_interface_id_t if_id)
{
  return bgpstream_data_interface_infos[if_id];
}

int bgpstream_get_data_interface_options(
  bgpstream_t *bs, bgpstream_data_interface_id_t if_id,
  bgpstream_data_interface_option_t **opts)
{
  assert(opts != NULL);

  switch (if_id) {

#ifdef WITH_DATA_INTERFACE_SINGLEFILE
  case BGPSTREAM_DATA_INTERFACE_SINGLEFILE:
    *opts = bgpstream_singlefile_options;
    return ARR_CNT(bgpstream_singlefile_options);
    break;
#endif

#ifdef WITH_DATA_INTERFACE_CSVFILE
  case BGPSTREAM_DATA_INTERFACE_CSVFILE:
    *opts = bgpstream_csvfile_options;
    return ARR_CNT(bgpstream_csvfile_options);
    break;
#endif

#ifdef WITH_DATA_INTERFACE_SQLITE
  case BGPSTREAM_DATA_INTERFACE_SQLITE:
    *opts = bgpstream_sqlite_options;
    return ARR_CNT(bgpstream_sqlite_options);
    break;
#endif

#ifdef WITH_DATA_INTERFACE_BROKER
  case BGPSTREAM_DATA_INTERFACE_BROKER:
    *opts = bgpstream_broker_options;
    return ARR_CNT(bgpstream_broker_options);
    break;
#endif

  default:
    *opts = NULL;
    return 0;
    break;
  }
}

bgpstream_data_interface_option_t *bgpstream_get_data_interface_option_by_name(
  bgpstream_t *bs, bgpstream_data_interface_id_t if_id, const char *name)
{
  bgpstream_data_interface_option_t *options;
  int opt_cnt = 0;
  int i;

  opt_cnt = bgpstream_get_data_interface_options(bs, if_id, &options);

  if (options == NULL || opt_cnt == 0) {
    return NULL;
  }

  for (i = 0; i < opt_cnt; i++) {
    if (strcmp(options[i].name, name) == 0) {
      return &options[i];
    }
  }

  return NULL;
}

/* configure the datasource interface options */

void bgpstream_set_data_interface_option(
  bgpstream_t *bs, bgpstream_data_interface_option_t *option_type,
  const char *option_value)
{

  bgpstream_debug("BS: set_data_interface_options start");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ALLOCATED)) {
    return; // nothing to customize
  }

  bgpstream_datasource_mgr_set_data_interface_option(bs->datasource_mgr,
                                                     option_type, option_value);

  bgpstream_debug("BS: set_data_interface_options stop");
}

/* configure the interface so that it connects
 * to a specific datasource interface
 */
void bgpstream_set_data_interface(bgpstream_t *bs,
                                  bgpstream_data_interface_id_t datasource)
{
  bgpstream_debug("BS: set_data_interface start");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ALLOCATED)) {
    return; // nothing to customize
  }
  bgpstream_datasource_mgr_set_data_interface(bs->datasource_mgr, datasource);
  bgpstream_debug("BS: set_data_interface stop");
}

bgpstream_data_interface_id_t bgpstream_get_data_interface_id(bgpstream_t *bs)
{
  return bs->datasource_mgr->datasource;
}

/* configure the interface so that it blocks
 * waiting for new data
 */
void bgpstream_set_live_mode(bgpstream_t *bs)
{
  bgpstream_debug("BS: set_live_mode start");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ALLOCATED)) {
    return; // nothing to customize
  }
  bgpstream_datasource_mgr_set_blocking(bs->datasource_mgr);
  bgpstream_debug("BS: set_blocking stop");
}

/* turn on the bgpstream interface, i.e.:
 * it makes the interface ready
 * for a new get next call
*/
int bgpstream_start(bgpstream_t *bs)
{
  bgpstream_debug("BS: init start");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ALLOCATED)) {
    return 0; // nothing to init
  }

  // validate the filters that have been set
  int rc;
  if ((rc = bgpstream_filter_mgr_validate(bs->filter_mgr)) != 0) {
    return rc;
  }

  // turn on datasource interface
  bgpstream_datasource_mgr_init(bs->datasource_mgr, bs->filter_mgr);
  if (bs->datasource_mgr->status == BGPSTREAM_DATASOURCE_STATUS_ON) {
    bs->status = BGPSTREAM_STATUS_ON; // interface is on
    bgpstream_debug("BS: init end: ok");
    return 0;
  } else {
    // interface is not on (something wrong with datasource)
    bs->status = BGPSTREAM_STATUS_ALLOCATED;
    bgpstream_debug("BS: init warning: check if the datasource provided is ok");
    bgpstream_debug("BS: init end: not ok");
    return -1;
  }
}

/* this function returns the next available record read
 * if the input_queue (i.e. list of files connected from
 * an external source) or the reader_cqueue (i.e. list
 * of bgpdump currently open) are empty then it
 * triggers a mechanism to populate the queues or
 * return 0 if nothing is available
 */
int bgpstream_get_next_record(bgpstream_t *bs, bgpstream_record_t *record)
{
  bgpstream_debug("BS: get next");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ON)) {
    return -1; // wrong status
  }

  int num_query_results = 0;
  bgpstream_input_t *bs_in = NULL;

  // if bs_record contains an initialized bgpdump entry we destroy it
  bgpstream_record_clear(record);

  while (bgpstream_reader_mgr_is_empty(bs->reader_mgr)) {
    bgpstream_debug("BS: reader mgr is empty");
    // get new data to process and set the reader_mgr
    while (bgpstream_input_mgr_is_empty(bs->input_mgr)) {
      bgpstream_debug("BS: input mgr is empty");
      /* query the external source and append new
       * input objects to the input_mgr queue */
      num_query_results = bgpstream_datasource_mgr_update_input_queue(
        bs->datasource_mgr, bs->input_mgr);
      if (num_query_results == 0) {
        bgpstream_debug("BS: no (more) data are available");
        return 0; // no (more) data are available
      }
      if (num_query_results < 0) {
        bgpstream_debug("BS: error during datasource_mgr_update_input_queue");
        return -1; // error during execution
      }
      bgpstream_debug("BS: got results from datasource");
    }
    bgpstream_debug("BS: input mgr not empty");
    bs_in = bgpstream_input_mgr_get_queue_to_process(bs->input_mgr);
    bgpstream_reader_mgr_add(bs->reader_mgr, bs_in, bs->filter_mgr);
    bgpstream_input_mgr_destroy_queue(bs_in);
    bs_in = NULL;
  }
  bgpstream_debug("BS: reader mgr not empty");
  /* init the record with a pointer to bgpstream */
  record->bs = bs;
  return bgpstream_reader_mgr_get_next_record(bs->reader_mgr, record,
                                              bs->filter_mgr);
}

/* turn off the bgpstream interface */
void bgpstream_stop(bgpstream_t *bs)
{
  bgpstream_debug("BS: close start");
  if (bs == NULL || (bs != NULL && bs->status != BGPSTREAM_STATUS_ON)) {
    return; // nothing to close
  }
  bgpstream_datasource_mgr_close(bs->datasource_mgr);
  bs->status = BGPSTREAM_STATUS_OFF; // interface is off
  bgpstream_debug("BS: close end");
}

/* destroy a bgpstream interface istance
 */
void bgpstream_destroy(bgpstream_t *bs)
{
  bgpstream_debug("BS: destroy start");
  if (bs == NULL) {
    return; // nothing to destroy
  }
  bgpstream_input_mgr_destroy(bs->input_mgr);
  bs->input_mgr = NULL;
  bgpstream_reader_mgr_destroy(bs->reader_mgr);
  bs->reader_mgr = NULL;
  bgpstream_filter_mgr_destroy(bs->filter_mgr);
  bs->filter_mgr = NULL;
  bgpstream_datasource_mgr_destroy(bs->datasource_mgr);
  bs->datasource_mgr = NULL;
  free(bs);
  bgpstream_debug("BS: destroy end");
}
