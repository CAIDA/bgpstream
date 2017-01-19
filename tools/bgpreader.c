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

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <unistd.h>

#include "bgpstream.h"

#define PROJECT_CMD_CNT 10
#define TYPE_CMD_CNT 10
#define COLLECTOR_CMD_CNT 100
#define PREFIX_CMD_CNT 1000
#define COMMUNITY_CMD_CNT 1000
#define PEERASN_CMD_CNT 1000
#define WINDOW_CMD_CNT 1024
#define OPTION_CMD_CNT 1024
#define BGPSTREAM_RECORD_OUTPUT_FORMAT                                         \
  "# Record format:\n"                                                         \
  "# <dump-type>|<dump-pos>|<project>|<collector>|<status>|<dump-time>\n"      \
  "#\n"                                                                        \
  "# <dump-type>: R RIB, U Update\n"                                           \
  "# <dump-pos>:  B begin, M middle, E end\n"                                  \
  "# <status>:    V valid, E empty, F filtered, R corrupted record, S "        \
  "corrupted source\n"                                                         \
  "#\n"
#define BGPSTREAM_ELEM_OUTPUT_FORMAT                                           \
  "# Elem format:\n"                                                           \
  "# "                                                                         \
  "<dump-type>|<elem-type>|<record-ts>|<project>|<collector>|<peer-ASN>|<"     \
  "peer-IP>|<prefix>|<next-hop-IP>|<AS-path>|<origin-AS>|<communities>|<old-"  \
  "state>|<new-state>\n"                                                       \
  "#\n"                                                                        \
  "# <dump-type>: R RIB, U Update\n"                                           \
  "# <elem-type>: R RIB, A announcement, W withdrawal, S state message\n"      \
  "#\n"                                                                        \
  "# RIB control messages (signal Begin and End of RIB):\n"                    \
  "# <dump-type>|<dump-pos>|<record-ts>|<project>|<collector>\n"               \
  "#\n"                                                                        \
  "# <dump-pos>:  B begin, E end\n"                                            \
  "#\n"

struct window {
  uint32_t start;
  uint32_t end;
};

static bgpstream_t *bs;
static bgpstream_data_interface_id_t datasource_id_default = 0;
static bgpstream_data_interface_id_t datasource_id = 0;
static bgpstream_data_interface_info_t *datasource_info = NULL;

static void data_if_usage()
{
  bgpstream_data_interface_id_t *ids = NULL;
  int id_cnt = 0;
  int i;

  bgpstream_data_interface_info_t *info = NULL;

  id_cnt = bgpstream_get_data_interfaces(bs, &ids);

  for (i = 0; i < id_cnt; i++) {
    info = bgpstream_get_data_interface_info(bs, ids[i]);

    if (info != NULL) {
      fprintf(stderr, "       %-15s%s%s\n", info->name, info->description,
              (ids[i] == datasource_id_default) ? " (default)" : "");
    }
  }
}

static void dump_if_options()
{
  assert(datasource_id != 0);

  bgpstream_data_interface_option_t *options;
  int opt_cnt = 0;
  int i;

  opt_cnt = bgpstream_get_data_interface_options(bs, datasource_id, &options);

  fprintf(stderr, "Data interface options for '%s':\n", datasource_info->name);
  if (opt_cnt == 0) {
    fprintf(stderr, "   [NONE]\n");
  } else {
    for (i = 0; i < opt_cnt; i++) {
      fprintf(stderr, "   %-15s%s\n", options[i].name, options[i].description);
    }
  }
  fprintf(stderr, "\n");
}

static void usage()
{
  fprintf(
    stderr,
    "usage: bgpreader -w <start>[,<end>] [<options>]\n"
    "Available options are:\n"
    "   -d <interface> use the given data interface to find available data\n"
    "                  available data interfaces are:\n");
  data_if_usage();
  fprintf(
    stderr,
    "   -o <option-name,option-value>*\n"
    "                  set an option for the current data interface.\n"
    "                  use '-o ?' to get a list of available options for the "
    "current\n"
    "                  data interface. (data interface can be selected using "
    "-d)\n"
    "   -p <project>   process records from only the given project "
    "(routeviews, ris)*\n"
    "   -c <collector> process records from only the given collector*\n"
    "   -t <type>      process records with only the given type (ribs, "
    "updates)*\n"
    "   -f <filterstring>   filter records and elements using the rules \n"
    "                       described in the given filter string\n"
    "   -I <interval>       process records that were received recently, where "
    "the\n"
    "                       interval describes how far back in time to go. The "
    "\n"
    "                       interval should be expressed as '<num> <unit>', "
    "where\n"
    "                       where <unit> can be one of 's', 'm', 'h', 'd' "
    "(seconds,\n"
    "                       minutes, hours, days).\n"
    "   -w <start>[,<end>]\n"
    "                  process records within the given time window\n"
    "                    (omitting the end parameter enables live mode)*\n"
    "   -P <period>    process a rib files every <period> seconds (bgp time)\n"
    "   -j <peer ASN>  return valid elems originated by a specific peer ASN*\n"
    "   -k <prefix>    return valid elems associated with a specific prefix*\n"
    "   -y <community> return valid elems with the specified community* \n"
    "                  (format: asn:value, the '*' metacharacter is "
    "recognized)\n"
    "   -l             enable live mode (make blocking requests for BGP "
    "records)\n"
    "                  allows bgpstream to be used to process data in "
    "real-time\n"
    "\n"
    "   -e             print info for each element of a valid BGP record "
    "(default)\n"
    "   -m             print info for each BGP valid record in bgpdump -m "
    "format\n"
    "   -r             print info for each BGP record (used mostly for "
    "debugging BGPStream)\n"
    "   -i             print format information before output\n"
    "\n"
    "   -h             print this help menu\n"
    "* denotes an option that can be given multiple times\n");
}

// print / utility functions

static void print_bs_record(bgpstream_record_t *bs_record);
static int print_elem(bgpstream_record_t *bs_record, bgpstream_elem_t *elem);
static void print_rib_control_message(bgpstream_record_t *bs_record);

int main(int argc, char *argv[])
{

  int opt;
  int prevoptind;

  opterr = 0;

  // variables associated with options
  char *projects[PROJECT_CMD_CNT];
  int projects_cnt = 0;

  char *types[TYPE_CMD_CNT];
  int types_cnt = 0;

  char *collectors[COLLECTOR_CMD_CNT];
  int collectors_cnt = 0;

  char *peerasns[PEERASN_CMD_CNT];
  int peerasns_cnt = 0;

  char *prefixes[PREFIX_CMD_CNT];
  int prefixes_cnt = 0;

  char *communities[COMMUNITY_CMD_CNT];
  int communities_cnt = 0;

  struct window windows[WINDOW_CMD_CNT];
  char *endp;
  int windows_cnt = 0;

  char *interface_options[OPTION_CMD_CNT];
  int interface_options_cnt = 0;

  char *filterstring = NULL;
  char *intervalstring = NULL;

  int rib_period = 0;
  int live = 0;
  int output_info = 0;
  int record_output_on = 0;
  int record_bgpdump_output_on = 0;
  int elem_output_on = 0;

  bgpstream_data_interface_option_t *option;

  int i;

  /* required to be created before usage is called */
  bs = bgpstream_create();
  if (!bs) {
    fprintf(stderr, "ERROR: Could not create BGPStream instance\n");
    return -1;
  }
  datasource_id_default = datasource_id = bgpstream_get_data_interface_id(bs);
  datasource_info = bgpstream_get_data_interface_info(bs, datasource_id);
  assert(datasource_id != 0);

  /* allocate memory for bs_record */
  bgpstream_record_t *bs_record = bgpstream_record_create();
  if (bs_record == NULL) {
    fprintf(stderr, "ERROR: Could not create BGPStream record\n");
    bgpstream_destroy(bs);
    return -1;
  }

  while (prevoptind = optind,
         (opt = getopt(argc, argv, "f:I:d:o:p:c:t:w:j:k:y:P:lrmeivh?")) >= 0) {
    if (optind == prevoptind + 2 && (optarg == NULL || *optarg == '-')) {
      opt = ':';
      --optind;
    }
    switch (opt) {
    case 'p':
      if (projects_cnt == PROJECT_CMD_CNT) {
        fprintf(stderr, "ERROR: A maximum of %d projects can be specified on "
                        "the command line\n",
                PROJECT_CMD_CNT);
        usage();
        exit(-1);
      }
      projects[projects_cnt++] = strdup(optarg);
      break;
    case 'c':
      if (collectors_cnt == COLLECTOR_CMD_CNT) {
        fprintf(stderr, "ERROR: A maximum of %d collectors can be specified on "
                        "the command line\n",
                COLLECTOR_CMD_CNT);
        usage();
        exit(-1);
      }
      collectors[collectors_cnt++] = strdup(optarg);
      break;
    case 't':
      if (types_cnt == TYPE_CMD_CNT) {
        fprintf(stderr, "ERROR: A maximum of %d types can be specified on "
                        "the command line\n",
                TYPE_CMD_CNT);
        usage();
        exit(-1);
      }
      types[types_cnt++] = strdup(optarg);
      break;
    case 'w':
      if (windows_cnt == WINDOW_CMD_CNT) {
        fprintf(stderr, "ERROR: A maximum of %d windows can be specified on "
                        "the command line\n",
                WINDOW_CMD_CNT);
        usage();
        exit(-1);
      }
      /* split the window into a start and end */
      if ((endp = strchr(optarg, ',')) == NULL) {
        windows[windows_cnt].end = BGPSTREAM_FOREVER;
      } else {
        *endp = '\0';
        endp++;
        windows[windows_cnt].end = atoi(endp);
      }
      windows[windows_cnt].start = atoi(optarg);
      windows_cnt++;
      break;
    case 'j':
      if (peerasns_cnt == PEERASN_CMD_CNT) {
        fprintf(stderr, "ERROR: A maximum of %d peer asns can be specified on "
                        "the command line\n",
                PEERASN_CMD_CNT);
        usage();
        exit(-1);
      }
      peerasns[peerasns_cnt++] = strdup(optarg);
      break;
    case 'k':
      if (prefixes_cnt == PREFIX_CMD_CNT) {
        fprintf(stderr, "ERROR: A maximum of %d peer asns can be specified on "
                        "the command line\n",
                PREFIX_CMD_CNT);
        usage();
        exit(-1);
      }
      prefixes[prefixes_cnt++] = strdup(optarg);
      break;
    case 'y':
      if (communities_cnt == COMMUNITY_CMD_CNT) {
        fprintf(stderr,
                "ERROR: A maximum of %d communities can be specified on "
                "the command line\n",
                PREFIX_CMD_CNT);
        usage();
        exit(-1);
      }
      communities[communities_cnt++] = strdup(optarg);
      break;
    case 'P':
      rib_period = atoi(optarg);
      break;
    case 'd':
      if ((datasource_id =
             bgpstream_get_data_interface_id_by_name(bs, optarg)) == 0) {
        fprintf(stderr, "ERROR: Invalid data interface name '%s'\n", optarg);
        usage();
        exit(-1);
      }
      datasource_info = bgpstream_get_data_interface_info(bs, datasource_id);
      break;
    case 'o':
      if (interface_options_cnt == OPTION_CMD_CNT) {
        fprintf(stderr,
                "ERROR: A maximum of %d interface options can be specified\n",
                OPTION_CMD_CNT);
        usage();
        exit(-1);
      }
      interface_options[interface_options_cnt++] = strdup(optarg);
      break;

    case 'l':
      live = 1;
      break;
    case 'r':
      record_output_on = 1;
      break;
    case 'm':
      record_bgpdump_output_on = 1;
      break;
    case 'e':
      elem_output_on = 1;
      break;
    case 'i':
      output_info = 1;
      break;
    case 'f':
      filterstring = optarg;
      break;
    case 'I':
      intervalstring = optarg;
      break;
    case ':':
      fprintf(stderr, "ERROR: Missing option argument for -%c\n", optopt);
      usage();
      exit(-1);
      break;
    case '?':
    case 'v':
      fprintf(stderr, "bgpreader version %d.%d.%d\n", BGPSTREAM_MAJOR_VERSION,
              BGPSTREAM_MID_VERSION, BGPSTREAM_MINOR_VERSION);
      usage();
      exit(0);
      break;
    default:
      usage();
      exit(-1);
    }
  }

  for (i = 0; i < interface_options_cnt; i++) {
    if (*interface_options[i] == '?') {
      dump_if_options();
      usage();
      exit(0);
    } else {
      /* actually set this option */
      if ((endp = strchr(interface_options[i], ',')) == NULL) {
        fprintf(stderr, "ERROR: Malformed data interface option (%s)\n",
                interface_options[i]);
        fprintf(stderr, "ERROR: Expecting <option-name>,<option-value>\n");
        usage();
        exit(-1);
      }
      *endp = '\0';
      endp++;
      if ((option = bgpstream_get_data_interface_option_by_name(
             bs, datasource_id, interface_options[i])) == NULL) {
        fprintf(stderr, "ERROR: Invalid option '%s' for data interface '%s'\n",
                interface_options[i], datasource_info->name);
        usage();
        exit(-1);
      }
      bgpstream_set_data_interface_option(bs, option, endp);
    }
    free(interface_options[i]);
    interface_options[i] = NULL;
  }
  interface_options_cnt = 0;

  if (windows_cnt == 0 && !intervalstring) {
    if (datasource_id == BGPSTREAM_DATA_INTERFACE_BROKER) {
      fprintf(stderr,
              "ERROR: At least one time window must be set when using the "
              "broker data interface\n");
      usage();
      exit(-1);
    } else {
      fprintf(stderr, "WARN: No time windows specified, defaulting to all "
                      "available data\n");
    }
  }

  /* if the user did not specify any output format
   * then the default one is per elem */
  if (record_output_on == 0 && elem_output_on == 0 &&
      record_bgpdump_output_on == 0) {
    elem_output_on = 1;
  }

  /* the program can now start */

  /* allocate memory for interface */

  /* Parse the filter string */
  if (filterstring) {
    bgpstream_parse_filter_string(bs, filterstring);
  }

  if (intervalstring) {
    bgpstream_add_recent_interval_filter(bs, intervalstring, live);
  }

  /* projects */
  for (i = 0; i < projects_cnt; i++) {
    bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_PROJECT, projects[i]);
    free(projects[i]);
  }

  /* collectors */
  for (i = 0; i < collectors_cnt; i++) {
    bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_COLLECTOR, collectors[i]);
    free(collectors[i]);
  }

  /* types */
  for (i = 0; i < types_cnt; i++) {
    bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_RECORD_TYPE, types[i]);
    free(types[i]);
  }

  /* windows */
  for (i = 0; i < windows_cnt; i++) {
    bgpstream_add_interval_filter(bs, windows[i].start, windows[i].end);
  }

  /* peer asns */
  for (i = 0; i < peerasns_cnt; i++) {
    bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PEER_ASN, peerasns[i]);
    free(peerasns[i]);
  }

  /* prefixes */
  for (i = 0; i < prefixes_cnt; i++) {
    bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PREFIX, prefixes[i]);
    free(prefixes[i]);
  }

  /* communities */
  for (i = 0; i < communities_cnt; i++) {
    bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_COMMUNITY,
                         communities[i]);
    free(communities[i]);
  }

  /* frequencies */
  if (rib_period > 0) {
    bgpstream_add_rib_period_filter(bs, rib_period);
  }

  /* datasource */
  bgpstream_set_data_interface(bs, datasource_id);

  /* live */
  if (live != 0) {
    bgpstream_set_live_mode(bs);
  }

  /* turn on interface */
  if (bgpstream_start(bs) < 0) {
    fprintf(stderr, "ERROR: Could not init BGPStream\n");
    return -1;
  }

  if (output_info) {
    if (record_output_on) {
      printf(BGPSTREAM_RECORD_OUTPUT_FORMAT);
    }
    if (elem_output_on) {
      printf(BGPSTREAM_ELEM_OUTPUT_FORMAT);
    }
  }

  /* use the interface */
  int get_next_ret = 0;
  bgpstream_elem_t *bs_elem;
  do {
    get_next_ret = bgpstream_get_next_record(bs, bs_record);
    if (get_next_ret && record_output_on) {
      print_bs_record(bs_record);
    }
    if (get_next_ret &&
        bs_record->status == BGPSTREAM_RECORD_STATUS_VALID_RECORD) {
      if (record_bgpdump_output_on) {
        bgpstream_record_print_mrt_data(bs_record);
      }
      if (elem_output_on) {
        /* check if the record is of type RIB, in case extract the ID */
        if (bs_record->attributes.dump_type == BGPSTREAM_RIB) {

          /* print the RIB start line */
          if (bs_record->dump_pos == BGPSTREAM_DUMP_START) {
            print_rib_control_message(bs_record);
          }
        }

        while ((bs_elem = bgpstream_record_get_next_elem(bs_record)) != NULL) {
          if (print_elem(bs_record, bs_elem) != 0) {
            goto err;
          }
        }
        /* check if end of RIB has been reached */
        if (bs_record->attributes.dump_type == BGPSTREAM_RIB &&
            bs_record->dump_pos == BGPSTREAM_DUMP_END) {
          print_rib_control_message(bs_record);
        }
      }
    }
  } while (get_next_ret > 0);

  /* de-allocate memory for bs_record */
  bgpstream_record_destroy(bs_record);

  /* turn off interface */
  bgpstream_stop(bs);

  /* deallocate memory for interface */
  bgpstream_destroy(bs);

  return 0;

err:
  bgpstream_record_destroy(bs_record);
  bgpstream_stop(bs);
  bgpstream_destroy(bs);
  return -1;
}

/* print utility functions */

static char record_buf[65536];

static void print_bs_record(bgpstream_record_t *bs_record)
{
  assert(bs_record);

  size_t written = 0; /* < how many bytes we wanted to write */
  ssize_t c = 0;      /* < how many chars were written */
  char *buf_p = record_buf;
  int len = 65536;

  /* record type */
  if ((c = bgpstream_record_dump_type_snprintf(
         buf_p, len - written, bs_record->attributes.dump_type)) < 0) {
    return;
  }
  written += c;
  buf_p += c;

  c = snprintf(buf_p, len - written, "|");
  written += c;
  buf_p += c;

  /* record position */
  if ((c = bgpstream_record_dump_pos_snprintf(buf_p, len - written,
                                              bs_record->dump_pos)) < 0) {
    return;
  }
  written += c;
  buf_p += c;

  /* Record timestamp, project, collector */
  c = snprintf(
    buf_p, len - written, "|%ld|%s|%s|", bs_record->attributes.record_time,
    bs_record->attributes.dump_project, bs_record->attributes.dump_collector);
  written += c;
  buf_p += c;

  /* record status */
  if ((c = bgpstream_record_status_snprintf(buf_p, len - written,
                                            bs_record->status)) < 0) {
    return;
  }
  written += c;
  buf_p += c;

  /* dump time */
  c = snprintf(buf_p, len - written, "|%ld", bs_record->attributes.dump_time);
  written += c;
  buf_p += c;

  if (written >= len) {
    return;
  }

  printf("%s\n", record_buf);
}

static void print_rib_control_message(bgpstream_record_t *bs_record)
{
  assert(bs_record);

  size_t written = 0; /* < how many bytes we wanted to write */
  ssize_t c = 0;      /* < how many chars were written */
  char *buf_p = record_buf;
  int len = 65536;

  /* record type */
  if ((c = bgpstream_record_dump_type_snprintf(
         buf_p, len - written, bs_record->attributes.dump_type)) < 0) {
    return;
  }
  written += c;
  buf_p += c;

  c = snprintf(buf_p, len - written, "|");
  written += c;
  buf_p += c;

  /* record position */
  if ((c = bgpstream_record_dump_pos_snprintf(buf_p, len - written,
                                              bs_record->dump_pos)) < 0) {
    return;
  }
  written += c;
  buf_p += c;

  /* Record timestamp, project, collector */
  c = snprintf(
    buf_p, len - written, "|%ld|%s|%s", bs_record->attributes.record_time,
    bs_record->attributes.dump_project, bs_record->attributes.dump_collector);
  written += c;
  buf_p += c;

  if (written >= len) {
    return;
  }

  printf("%s\n", record_buf);
}

static char elem_buf[65536];
static int print_elem(bgpstream_record_t *bs_record, bgpstream_elem_t *elem)
{
  assert(bs_record);
  assert(elem);
  int len = 65536;

  if (bgpstream_record_elem_snprintf(elem_buf, len, bs_record, elem) != NULL) {
    printf("%s\n", elem_buf);
    return 0;
  }

  return -1;
}
