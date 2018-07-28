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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

#include "bgpcorsaro.h"
#include "bgpcorsaro_log.h"

/** @file
 *
 * @brief Code which uses libbgpcorsaro to process a trace file and generate
 * output
 *
 * @author Alistair King
 *
 */

#define PROJECT_CMD_CNT 10
#define TYPE_CMD_CNT 10
#define COLLECTOR_CMD_CNT 100
#define PREFIX_CMD_CNT 1000
#define COMMUNITY_CMD_CNT 1000
#define PEERASN_CMD_CNT 1000
#define WINDOW_CMD_CNT 1024
#define OPTION_CMD_CNT 1024

struct window {
  uint32_t start;
  uint32_t end;
};

/* default gap limit */
#define GAP_LIMIT_DEFAULT 0

/** Maximum allowed packet inter-arrival time */
static int gap_limit = GAP_LIMIT_DEFAULT;

/** Indicates that Bgpcorsaro is waiting to shutdown */
volatile sig_atomic_t bgpcorsaro_shutdown = 0;

/** The number of SIGINTs to catch before aborting */
#define HARD_SHUTDOWN 3

/* for when we are reading trace files */
/** A pointer to a bgpstream object */
static bgpstream_t *stream = NULL;

/** A pointer to a bgpstream record */
static bgpstream_record_t *record = NULL;

/** A pointer to the instance of bgpcorsaro that we will drive */
static bgpcorsaro_t *bgpcorsaro = NULL;

/** The id associated with the bgpstream data interface  */
static bgpstream_data_interface_id_t datasource_id_default = 0;
static bgpstream_data_interface_id_t datasource_id = 0;
static bgpstream_data_interface_info_t *datasource_info = NULL;

/** Handles SIGINT gracefully and shuts down */
static void catch_sigint(int sig)
{
  bgpcorsaro_shutdown++;
  if (bgpcorsaro_shutdown == HARD_SHUTDOWN) {
    fprintf(stderr, "caught %d SIGINT's. shutting down NOW\n", HARD_SHUTDOWN);
    exit(-1);
  }

  fprintf(stderr, "caught SIGINT, shutting down at the next opportunity\n");

  signal(sig, catch_sigint);
}

/** Clean up all state before exit */
static void clean()
{
  if (record != NULL) {
    bgpstream_record_destroy(record);
    record = NULL;
  }

  if (bgpcorsaro != NULL) {
    bgpcorsaro_finalize_output(bgpcorsaro);
  }
}

static void data_if_usage()
{
  bgpstream_data_interface_id_t *ids = NULL;
  int id_cnt = 0;
  int i;

  bgpstream_data_interface_info_t *info = NULL;

  id_cnt = bgpstream_get_data_interfaces(stream, &ids);

  for (i = 0; i < id_cnt; i++) {
    info = bgpstream_get_data_interface_info(stream, ids[i]);

    if (info != NULL) {
      fprintf(stderr, "       %-13s%s%s\n", info->name, info->description,
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

  opt_cnt =
    bgpstream_get_data_interface_options(stream, datasource_id, &options);

  fprintf(stderr, "Data interface options for '%s':\n", datasource_info->name);
  if (opt_cnt == 0) {
    fprintf(stderr, "   [NONE]\n");
  } else {
    for (i = 0; i < opt_cnt; i++) {
      fprintf(stderr, "   %-13s%s\n", options[i].name, options[i].description);
    }
  }
  fprintf(stderr, "\n");
}

/** Print usage information to stderr */
static void usage()
{
  int i;
  char **plugin_names;
  int plugin_cnt;
  if ((plugin_cnt = bgpcorsaro_get_plugin_names(&plugin_names)) < 0) {
    /* not much we can do */
    fprintf(stderr, "bgpcorsaro_get_plugin_names failed\n");
    return;
  }

  fprintf(stderr,
          "usage: bgpcorsaro -w <start>[,<end>] -O outfile [<options>]\n"
          "Available options are:\n"
          "   -d <interface> use the given bgpstream data interface to find "
          "available data\n"
          "                   available data interfaces are:\n");
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
    "                  allows bgpcorsaro to be used to process data in "
    "real-time\n"
    "\n"
    "   -i <interval>  distribution interval in seconds (default: %d)\n"
    "   -a             align the end time of the first interval\n"
    "   -g <gap-limit> maximum allowed gap between packets (0 is no limit) "
    "(default: %d)\n"
    "   -L             disable logging to a file\n"
    "\n",
    BGPCORSARO_INTERVAL_DEFAULT, GAP_LIMIT_DEFAULT);
  fprintf(stderr, "   -x <plugin>    enable the given plugin (default: all)*\n"
                  "                   available plugins:\n");

  for (i = 0; i < plugin_cnt; i++) {
    fprintf(stderr, "                    - %s\n", plugin_names[i]);
  }
  fprintf(
    stderr,
    "                   use -p \"<plugin_name> -?\" to see plugin options\n"
    "   -n <name>      monitor name (default: " STR(
      BGPCORSARO_MONITOR_NAME) ")\n"
                               "   -O <outfile>   use <outfile> as a template "
                               "for file names.\n"
                               "                   - %%X => plugin name\n"
                               "                   - %%N => monitor name\n"
                               "                   - see man strftime(3) for "
                               "more options\n"
                               "   -r <intervals> rotate output files after n "
                               "intervals\n"
                               "   -R <intervals> rotate bgpcorsaro meta files "
                               "after n intervals\n"
                               "\n"
                               "   -h             print this help menu\n"
                               "* denotes an option that can be given multiple "
                               "times\n");

  bgpcorsaro_free_plugin_names(plugin_names, plugin_cnt);
}

/** Entry point for the Bgpcorsaro tool */
int main(int argc, char *argv[])
{
  /* we MUST not use any of the getopt global vars outside of arg parsing */
  /* this is because the plugins can use get opt to parse their config */
  int opt;
  int prevoptind;
  char *tmpl = NULL;
  char *name = NULL;
  int i = 0;
  int interval = -1000;
  double this_time = 0;
  double last_time = 0;
  char *plugins[BGPCORSARO_PLUGIN_ID_MAX];
  int plugin_cnt = 0;
  char *plugin_arg_ptr = NULL;
  int align = 0;
  int rotate = 0;
  int meta_rotate = -1;
  int logfile_disable = 0;

  bgpstream_data_interface_option_t *option;

  char *projects[PROJECT_CMD_CNT];
  int projects_cnt = 0;

  char *types[TYPE_CMD_CNT];
  int types_cnt = 0;

  char *collectors[COLLECTOR_CMD_CNT];
  int collectors_cnt = 0;

  char *endp;
  struct window windows[WINDOW_CMD_CNT];
  int windows_cnt = 0;

  char *peerasns[PEERASN_CMD_CNT];
  int peerasns_cnt = 0;

  char *prefixes[PREFIX_CMD_CNT];
  int prefixes_cnt = 0;

  char *communities[COMMUNITY_CMD_CNT];
  int communities_cnt = 0;

  char *interface_options[OPTION_CMD_CNT];
  int interface_options_cnt = 0;

  int rib_period = 0;
  int live = 0;

  int rc = 0;

  signal(SIGINT, catch_sigint);

  if ((stream = bgpstream_create()) == NULL) {
    fprintf(stderr, "ERROR: Could not create BGPStream instance\n");
    return -1;
  }
  datasource_id_default = datasource_id =
    bgpstream_get_data_interface_id(stream);
  datasource_info = bgpstream_get_data_interface_info(stream, datasource_id);
  assert(datasource_id != 0);

  while (prevoptind = optind,
         (opt = getopt(argc, argv,
                       ":d:o:p:c:t:w:j:k:y:P:i:ag:lLx:n:O:r:R:hv?")) >= 0) {
    if (optind == prevoptind + 2 && (optarg == NULL || *optarg == '-')) {
      opt = ':';
      --optind;
    }
    switch (opt) {
    case 'd':
      if ((datasource_id =
             bgpstream_get_data_interface_id_by_name(stream, optarg)) == 0) {
        fprintf(stderr, "ERROR: Invalid data interface name '%s'\n", optarg);
        usage();
        exit(-1);
      }
      datasource_info =
        bgpstream_get_data_interface_info(stream, datasource_id);
      break;

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

    case 'P':
      rib_period = atoi(optarg);
      break;

    case 'l':
      live = 1;
      break;

    case 'g':
      gap_limit = atoi(optarg);
      break;

    case 'a':
      align = 1;
      break;

    case 'i':
      interval = atoi(optarg);
      break;

    case 'L':
      logfile_disable = 1;
      break;

    case 'n':
      name = strdup(optarg);
      break;

    case 'O':
      tmpl = strdup(optarg);
      break;

    case 'x':
      plugins[plugin_cnt++] = strdup(optarg);
      break;

    case 'r':
      rotate = atoi(optarg);
      break;

    case 'R':
      meta_rotate = atoi(optarg);
      break;

    case ':':
      fprintf(stderr, "ERROR: Missing option argument for -%c\n", optopt);
      usage();
      exit(-1);
      break;

    case '?':
    case 'v':
      fprintf(stderr, "bgpcorsaro version %d.%d.%d\n", BGPSTREAM_MAJOR_VERSION,
              BGPSTREAM_MID_VERSION, BGPSTREAM_MINOR_VERSION);
      usage();
      exit(0);
      break;

    default:
      usage();
      exit(-1);
    }
  }

  /* store the value of the last index*/
  /*lastopt = optind;*/

  /* reset getopt for others */
  optind = 1;

  /* -- call NO library functions which may use getopt before here -- */
  /* this ESPECIALLY means bgpcorsaro_enable_plugin */

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
             stream, datasource_id, interface_options[i])) == NULL) {
        fprintf(stderr, "ERROR: Invalid option '%s' for data interface '%s'\n",
                interface_options[i], datasource_info->name);
        usage();
        exit(-1);
      }
      bgpstream_set_data_interface_option(stream, option, endp);
    }
    free(interface_options[i]);
    interface_options[i] = NULL;
  }
  interface_options_cnt = 0;

  if (windows_cnt == 0) {
    fprintf(stderr,
            "ERROR: At least one time window must be specified using -w\n");
    usage();
    goto err;
  }

  if (tmpl == NULL) {
    fprintf(stderr,
            "ERROR: An output file template must be specified using -O\n");
    usage();
    goto err;
  }

  /* alloc bgpcorsaro */
  if ((bgpcorsaro = bgpcorsaro_alloc_output(tmpl)) == NULL) {
    usage();
    goto err;
  }

  if (name != NULL && bgpcorsaro_set_monitorname(bgpcorsaro, name) != 0) {
    bgpcorsaro_log(__func__, bgpcorsaro, "failed to set monitor name");
    goto err;
  }

  if (interval > -1000) {
    bgpcorsaro_set_interval(bgpcorsaro, interval);
  }

  if (align == 1) {
    bgpcorsaro_set_interval_alignment(bgpcorsaro,
                                      BGPCORSARO_INTERVAL_ALIGN_YES);
  }

  if (rotate > 0) {
    bgpcorsaro_set_output_rotation(bgpcorsaro, rotate);
  }

  if (meta_rotate >= 0) {
    bgpcorsaro_set_meta_output_rotation(bgpcorsaro, meta_rotate);
  }

  for (i = 0; i < plugin_cnt; i++) {
    /* the string at plugins[i] will contain the name of the plugin,
       optionally followed by a space and then the arguments to pass
       to the plugin */
    if ((plugin_arg_ptr = strchr(plugins[i], ' ')) != NULL) {
      /* set the space to a nul, which allows plugins[i] to be used
         for the plugin name, and then increment plugin_arg_ptr to
         point to the next character, which will be the start of the
         arg string (or at worst case, the terminating \0 */
      *plugin_arg_ptr = '\0';
      plugin_arg_ptr++;
    }

    if (bgpcorsaro_enable_plugin(bgpcorsaro, plugins[i], plugin_arg_ptr) != 0) {
      fprintf(stderr, "ERROR: Could not enable plugin %s\n", plugins[i]);
      usage();
      goto err;
    }
  }

  if (logfile_disable != 0) {
    bgpcorsaro_disable_logfile(bgpcorsaro);
  }

  if (bgpcorsaro_start_output(bgpcorsaro) != 0) {
    usage();
    goto err;
  }

  /* create a record buffer */
  if (record == NULL && (record = bgpstream_record_create()) == NULL) {
    fprintf(stderr, "ERROR: Could not create BGPStream record\n");
    return -1;
  }

  /* pass along the user's filter requests to bgpstream */

  /* types */
  for (i = 0; i < types_cnt; i++) {
    bgpstream_add_filter(stream, BGPSTREAM_FILTER_TYPE_RECORD_TYPE, types[i]);
    free(types[i]);
  }

  /* projects */
  for (i = 0; i < projects_cnt; i++) {
    bgpstream_add_filter(stream, BGPSTREAM_FILTER_TYPE_PROJECT, projects[i]);
    free(projects[i]);
  }

  /* collectors */
  for (i = 0; i < collectors_cnt; i++) {
    bgpstream_add_filter(stream, BGPSTREAM_FILTER_TYPE_COLLECTOR,
                         collectors[i]);
    free(collectors[i]);
  }

  /* windows */
  int minimum_time = 0;
  int current_time = 0;
  for (i = 0; i < windows_cnt; i++) {
    bgpstream_add_interval_filter(stream, windows[i].start, windows[i].end);
    current_time = windows[i].start;
    if (minimum_time == 0 || current_time < minimum_time) {
      minimum_time = current_time;
    }
  }

  /* peer asns */
  for (i = 0; i < peerasns_cnt; i++) {
    bgpstream_add_filter(stream, BGPSTREAM_FILTER_TYPE_ELEM_PEER_ASN,
                         peerasns[i]);
    free(peerasns[i]);
  }

  /* prefixes */
  for (i = 0; i < prefixes_cnt; i++) {
    bgpstream_add_filter(stream, BGPSTREAM_FILTER_TYPE_ELEM_PREFIX,
                         prefixes[i]);
    free(prefixes[i]);
  }

  /* communities */
  for (i = 0; i < communities_cnt; i++) {
    bgpstream_add_filter(stream, BGPSTREAM_FILTER_TYPE_ELEM_COMMUNITY,
                         communities[i]);
    free(communities[i]);
  }

  /* frequencies */
  if (rib_period > 0) {
    bgpstream_add_rib_period_filter(stream, rib_period);
  }

  /* live mode */
  if (live != 0) {
    bgpstream_set_live_mode(stream);
  }

  bgpstream_set_data_interface(stream, datasource_id);

  if (bgpstream_start(stream) < 0) {
    fprintf(stderr, "ERROR: Could not init BGPStream\n");
    return -1;
  }

  /* let bgpcorsaro have the trace pointer */
  bgpcorsaro_set_stream(bgpcorsaro, stream);

  while (bgpcorsaro_shutdown == 0 &&
         (rc = bgpstream_get_next_record(stream, record)) > 0) {

    /* remove records that preceed the beginning of the stream */
    if (record->attributes.record_time < minimum_time) {
      continue;
    }

    /* check the gap limit is not exceeded */
    this_time = record->attributes.record_time;
    if (gap_limit > 0 &&                     /* gap limit is enabled */
        last_time > 0 &&                     /* this is not the first packet */
        ((this_time - last_time) > 0) &&     /* packet doesn't go backward */
        (this_time - last_time) > gap_limit) /* packet exceeds gap */
    {
      bgpcorsaro_log(__func__, bgpcorsaro,
                     "gap limit exceeded (prev: %f this: %f diff: %f)",
                     last_time, this_time, (this_time - last_time));
      return -1;
    }
    last_time = this_time;

    /*bgpcorsaro_log(__func__, bgpcorsaro, "got a record!");*/
    if (bgpcorsaro_per_record(bgpcorsaro, record) != 0) {
      bgpcorsaro_log(__func__, bgpcorsaro, "bgpcorsaro_per_record failed");
      return -1;
    }
  }

  if (rc < 0) {
    bgpcorsaro_log(__func__, bgpcorsaro,
                   "bgpstream encountered an error processing records");
    return 1;
  }

  /* free the plugin strings */
  for (i = 0; i < plugin_cnt; i++) {
    if (plugins[i] != NULL)
      free(plugins[i]);
  }

  /* free the template string */
  if (tmpl != NULL)
    free(tmpl);

  bgpcorsaro_finalize_output(bgpcorsaro);
  bgpcorsaro = NULL;
  if (stream != NULL) {
    bgpstream_destroy(stream);
    stream = NULL;
  }

  clean();
  return 0;

err:
  /* if we bail early, let us be responsible and up the memory we alloc'd */
  for (i = 0; i < plugin_cnt; i++) {
    if (plugins[i] != NULL)
      free(plugins[i]);
  }

  if (tmpl != NULL)
    free(tmpl);

  clean();

  return -1;
}
