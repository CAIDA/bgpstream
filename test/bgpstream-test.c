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
#include "bgpstream.h"

#include <stdio.h>
#include <string.h>
#include <wandio.h>

#define SINGLEFILE_RECORDS 537347
#define CSVFILE_RECORDS    559424
#define SQLITE_RECORDS     538308
#define BROKER_RECORDS     2153

bgpstream_t *bs;
bgpstream_record_t *bs_record;
bgpstream_data_interface_id_t datasource_id = 0;
bgpstream_data_interface_option_t *option;



int run_bgpstream(char *interface)
{
  if(bgpstream_start(bs) < 0) {
    fprintf(stderr, "ERROR: Could not init BGPStream\n");
    return -1;
  }
  int get_next_ret = 0;
  int counter = 0;
  do
    {
      get_next_ret = bgpstream_get_next_record(bs, bs_record);
      if(get_next_ret && bs_record->status == BGPSTREAM_RECORD_STATUS_VALID_RECORD)
        {
          counter++;
        }
    }
  while(get_next_ret > 0);

  printf("\tread %d valid records\n", counter);
  
  bgpstream_stop(bs);

  if((strcmp(interface, "singlefile")== 0 && counter == SINGLEFILE_RECORDS) ||
     (strcmp(interface, "csvfile")== 0 && counter == CSVFILE_RECORDS) ||
     (strcmp(interface, "sqlite")== 0 && counter == SQLITE_RECORDS) ||
     (strcmp(interface, "broker")== 0 && counter == BROKER_RECORDS) )
    {
      printf("\tinterface is working correctly\n\n");
      return 0;
    }
  else
    {
      printf("\tinterface is NOT working correctly\n\n");
      return -1; 
    }

  return 0;
}


int main()
{

  /* Testing bgpstream */

  bs = bgpstream_create();
  if(!bs) {
    fprintf(stderr, "ERROR: Could not create BGPStream instance\n");
    return -1;
  }

  bs_record = bgpstream_record_create();
  if(bs_record == NULL)
    {
      fprintf(stderr, "ERROR: Could not create BGPStream record\n");
      bgpstream_destroy(bs);
      return -1;
    }
  bgpstream_destroy(bs);

  int res = 0;

  /* Testing singlefile interface */
  printf("Testing singlefile interface...\n");
  bs = bgpstream_create();
  datasource_id =
    bgpstream_get_data_interface_id_by_name(bs, "singlefile");
#ifdef WITH_DATA_INTERFACE_SINGLEFILE
  bgpstream_set_data_interface(bs, datasource_id);
  option =
    bgpstream_get_data_interface_option_by_name(bs, datasource_id,
                                                "rib-file");
  bgpstream_set_data_interface_option(bs, option, "./routeviews.route-views.jinx.ribs.1427846400.bz2");
  option =
    bgpstream_get_data_interface_option_by_name(bs, datasource_id,
                                                "upd-file");
  bgpstream_set_data_interface_option(bs, option, "./ris.rrc06.updates.1427846400.gz");
  res += run_bgpstream("singlefile");
  bgpstream_destroy(bs);
#else
  if(datasource_id != 0)
    {
      return -1;
    }
#endif

  /* Testing csvfile interface */
  printf("Testing csvfile interface...\n");
  bs = bgpstream_create();
  datasource_id =
    bgpstream_get_data_interface_id_by_name(bs, "csvfile");
#ifdef WITH_DATA_INTERFACE_CSVFILE
  bgpstream_set_data_interface(bs, datasource_id);

  option =
    bgpstream_get_data_interface_option_by_name(bs, datasource_id,
                                                "csv-file");
  bgpstream_set_data_interface_option(bs, option, "csv_test.csv");
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_COLLECTOR, "rrc06");
  res += run_bgpstream("csvfile");
  bgpstream_destroy(bs);
#else
  if(datasource_id != 0)
    {
      return -1;
    }
#endif

  /* Testing sqlite interface */
  printf("Testing sqlite interface...\n");
  bs = bgpstream_create();
  datasource_id =
    bgpstream_get_data_interface_id_by_name(bs, "sqlite");
#ifdef WITH_DATA_INTERFACE_SQLITE
  bgpstream_set_data_interface(bs, datasource_id);

  option =
    bgpstream_get_data_interface_option_by_name(bs, datasource_id,
                                                "db-file");
  bgpstream_set_data_interface_option(bs, option, "sqlite_test.db");
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_PROJECT, "routeviews");
  res += run_bgpstream("sqlite");
  bgpstream_destroy(bs);
#else
  if(datasource_id != 0)
    {
      return -1;
    }
#endif

  /* Testing broker interface */
  printf("Testing broker interface...\n");
  bs = bgpstream_create();
  datasource_id =
    bgpstream_get_data_interface_id_by_name(bs, "broker");
#ifdef WITH_DATA_INTERFACE_BROKER
  printf("Testing HTTP support/Internet connectivity...\n");
  io_t *file = wandio_create(BGPSTREAM_DS_BROKER_URL);
  if (file == NULL)
    {
      fprintf(stderr,
              "ERROR: Failed to connect to BGPStream Broker via HTTP.\n"
              "ERROR: Maybe wandio is built without HTTP support, "
              "or there is no Internet connectivity\n");
      return -1;
    }

  bgpstream_set_data_interface(bs, datasource_id);
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_COLLECTOR, "route-views6");
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_RECORD_TYPE, "updates");
  bgpstream_add_interval_filter(bs,1427846550,1427846700);
  res += run_bgpstream("broker");
  bgpstream_destroy(bs);
#else
  if(datasource_id != 0)
    {
      return -1;
    }
#endif

  bgpstream_record_destroy(bs_record);
  /* res is going to be zero if everything worked fine, */
  /* negative if something failed */
  return res;
}
