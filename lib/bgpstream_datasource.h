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

#ifndef _BGPSTREAM_DATASOURCE_H
#define _BGPSTREAM_DATASOURCE_H

#include "bgpstream_constants.h"
#include "bgpstream_filter.h"
#include "bgpstream_input.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef WITH_DATA_INTERFACE_SINGLEFILE
#include "bgpstream_datasource_singlefile.h"
#endif

#ifdef WITH_DATA_INTERFACE_CSVFILE
#include "bgpstream_datasource_csvfile.h"
#endif

#ifdef WITH_DATA_INTERFACE_SQLITE
#include "bgpstream_datasource_sqlite.h"
#endif

#ifdef WITH_DATA_INTERFACE_BROKER
#include "bgpstream_datasource_broker.h"
#endif

typedef enum {
  BGPSTREAM_DATASOURCE_STATUS_ON,   /* current data source is on */
  BGPSTREAM_DATASOURCE_STATUS_OFF,  /* current data source is off */
  BGPSTREAM_DATASOURCE_STATUS_ERROR /* current data source generated an error */
} bgpstream_datasource_status_t;

typedef struct struct_bgpstream_datasource_mgr_t {
  bgpstream_data_interface_id_t datasource;
// datasources available
#ifdef WITH_DATA_INTERFACE_SINGLEFILE
  bgpstream_singlefile_datasource_t *singlefile_ds;
  char *singlefile_rib_mrtfile;
  char *singlefile_upd_mrtfile;
#endif

#ifdef WITH_DATA_INTERFACE_CSVFILE
  bgpstream_csvfile_datasource_t *csvfile_ds;
  char *csvfile_file;
#endif

#ifdef WITH_DATA_INTERFACE_SQLITE
  bgpstream_sqlite_datasource_t *sqlite_ds;
  char *sqlite_file;
#endif

#ifdef WITH_DATA_INTERFACE_BROKER
  bgpstream_broker_datasource_t *broker_ds;
  char *broker_url;
  char **broker_params;
  int broker_params_cnt;
#endif

  // blocking options
  int blocking;
  int backoff_time;
  bgpstream_datasource_status_t status;
} bgpstream_datasource_mgr_t;

/* allocates memory for datasource_mgr */
bgpstream_datasource_mgr_t *bgpstream_datasource_mgr_create();

void bgpstream_datasource_mgr_set_data_interface(
  bgpstream_datasource_mgr_t *datasource_mgr,
  const bgpstream_data_interface_id_t datasource);

int bgpstream_datasource_mgr_set_data_interface_option(
  bgpstream_datasource_mgr_t *datasource_mgr,
  const bgpstream_data_interface_option_t *option_type,
  const char *option_value);

/* init the datasource_mgr and start/init the selected datasource */
void bgpstream_datasource_mgr_init(bgpstream_datasource_mgr_t *datasource_mgr,
                                   bgpstream_filter_mgr_t *filter_mgr);

void bgpstream_datasource_mgr_set_blocking(
  bgpstream_datasource_mgr_t *datasource_mgr);

int bgpstream_datasource_mgr_update_input_queue(
  bgpstream_datasource_mgr_t *datasource_mgr, bgpstream_input_mgr_t *input_mgr);

/* stop the active data source */
void bgpstream_datasource_mgr_close(bgpstream_datasource_mgr_t *datasource_mgr);

/* destroy the memory allocated for the datasource_mgr */
void bgpstream_datasource_mgr_destroy(
  bgpstream_datasource_mgr_t *datasource_mgr);

#endif /* _BGPSTREAM_DATASOURCE */
