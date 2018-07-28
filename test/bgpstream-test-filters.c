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
#include "bgpstream_test.h"

#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <wandio.h>

bgpstream_t *bs;
bgpstream_record_t *rec;
bgpstream_elem_t *elem;

bgpstream_data_interface_id_t datasource_id = 0;
bgpstream_data_interface_option_t *option;

static char elem_buf[65536];
static char *expected_results[7] = {
  "U|A|1427846850|ris|rrc06|25152|202.249.2.185|202.70.88.0/"
  "21|202.249.2.185|25152 2914 15412 9304 23752|23752|2914:410 2914:1408 "
  "2914:2401 2914:3400||",
  "U|A|1427846860|ris|rrc06|25152|202.249.2.185|202.70.88.0/"
  "21|202.249.2.185|25152 2914 15412 9304 23752|23752|2914:410 2914:1408 "
  "2914:2401 2914:3400||",
  "U|A|1427846871|ris|rrc06|25152|2001:200:0:fe00::6249:0|2620:110:9004::/"
  "48|2001:200:0:fe00::6249:0|25152 2914 3356 13620|13620|2914:420 2914:1001 "
  "2914:2000 2914:3000||",
  "U|A|1427846874|routeviews|route-views.jinx|37105|196.223.14.46|154.73.136.0/"
  "24|196.223.14.84|37105 37549|37549|37105:300||",
  "U|A|1427846874|routeviews|route-views.jinx|37105|196.223.14.46|154.73.137.0/"
  "24|196.223.14.84|37105 37549|37549|37105:300||",
  "U|A|1427846874|routeviews|route-views.jinx|37105|196.223.14.46|154.73.138.0/"
  "24|196.223.14.84|37105 37549|37549|37105:300||",
  "U|A|1427846874|routeviews|route-views.jinx|37105|196.223.14.46|154.73.139.0/"
  "24|196.223.14.84|37105 37549|37549|37105:300||"};

#define SETUP                                                                  \
  do {                                                                         \
    bs = bgpstream_create();                                                   \
    rec = bgpstream_record_create();                                           \
  } while (0)

#define TEARDOWN                                                               \
  do {                                                                         \
    bgpstream_record_destroy(rec);                                             \
    rec = NULL;                                                                \
    bgpstream_destroy(bs);                                                     \
    bs = NULL;                                                                 \
  } while (0)

#define CHECK_SET_INTERFACE(interface)                                         \
  do {                                                                         \
    CHECK("get data interface ID (" STR(interface) ")",                        \
          (datasource_id = bgpstream_get_data_interface_id_by_name(            \
             bs, STR(interface))) != 0);                                       \
    bgpstream_set_data_interface(bs, datasource_id);                           \
  } while (0)

int test_bgpstream_filters()
{
  SETUP;

  CHECK_SET_INTERFACE(broker);

  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_COLLECTOR, "rrc06");
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_COLLECTOR, "route-views.jinx");

  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_RECORD_TYPE, "updates");

  bgpstream_add_interval_filter(bs, 1427846847, 1427846874);

  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PEER_ASN, "25152");
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PEER_ASN, "37105");

  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PREFIX,
                       "2620:110:9004::/40");
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PREFIX,
                       "154.73.128.0/17");
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PREFIX, "202.70.88.0/21");

  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_COMMUNITY, "2914:*");
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_COMMUNITY, "*:300");

  int ret;
  int counter = 0;
  int check_res = 0;
  CHECK("stream start (" STR(interface) ")", bgpstream_start(bs) == 0);

  while ((ret = bgpstream_get_next_record(bs, rec)) > 0) {
    if (rec->status == BGPSTREAM_RECORD_STATUS_VALID_RECORD) {
      while ((elem = bgpstream_record_get_next_elem(rec)) != NULL) {
        if (bgpstream_record_elem_snprintf(elem_buf, 65536, rec, elem) !=
            NULL) {
          /* more results than the expected ones*/
          CHECK("elem partial count", counter < 7);

          /* check if the results are exactly the expected ones */
          CHECK("elem equality",
                (check_res =
                   strncmp(elem_buf, expected_results[counter], 65536)) == 0);

          counter++;
        }
      }
    }
  }

  CHECK("elem total count", counter == 7);

  bgpstream_stop(bs);

  TEARDOWN;
  return 0;
}

int main()
{

#ifdef WITH_DATA_INTERFACE_BROKER
  SETUP;
  CHECK_SET_INTERFACE(broker);

  test_bgpstream_filters();

  TEARDOWN;
#else
  SKIPPED_SECTION("broker data interface filters");
#endif

  return 0;
}
