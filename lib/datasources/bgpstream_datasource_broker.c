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

#include "bgpstream_datasource_broker.h"
#include "bgpstream_debug.h"
#include "utils.h"
#include "libjsmn/jsmn.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <wandio.h>

#define URL_BUFLEN 4096

// the max time we will wait between retries to the broker
#define MAX_WAIT_TIME 900

// indicates a fatal error
#define ERR_FATAL -1
// indicates a non-fatal error
#define ERR_RETRY -2

#define APPEND_STR(str)                                                        \
  do {                                                                         \
    size_t len = strlen(str);                                                  \
    if (broker_ds->query_url_remaining < len + 1) {                            \
      goto err;                                                                \
    }                                                                          \
    strncat(broker_ds->query_url_buf, str,                                     \
            broker_ds->query_url_remaining - 1);                               \
    broker_ds->query_url_remaining -= len;                                     \
  } while (0)

struct struct_bgpstream_broker_datasource_t {
  bgpstream_filter_mgr_t *filter_mgr;

  // working space to build query urls
  char query_url_buf[URL_BUFLEN];

  size_t query_url_remaining;

  // pointer to the end of the common query url (for appending last ts info)
  char *query_url_end;

  // have any parameters been added to the url?
  int first_param;

  // time of the last response we got from the broker
  uint32_t last_response_time;

  // the max (file_time + duration) that we have seen
  uint32_t current_window_end;
};

#define AMPORQ                                                                 \
  do {                                                                         \
    if (broker_ds->first_param) {                                              \
      APPEND_STR("?");                                                         \
      broker_ds->first_param = 0;                                              \
    } else {                                                                   \
      APPEND_STR("&");                                                         \
    }                                                                          \
  } while (0)

static int json_isnull(const char *json, jsmntok_t *tok)
{
  if (tok->type == JSMN_PRIMITIVE &&
      strncmp("null", json + tok->start, tok->end - tok->start) == 0) {
    return 1;
  }
  return 0;
}

static int json_strcmp(const char *json, jsmntok_t *tok, const char *s)
{
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

// NB: this ONLY replaces \/ with /
static void unescape_url(char *url)
{
  char *p = url;

  while (*p != '\0') {
    if (*p == '\\' && *(p + 1) == '/') {
      // copy the remainder of the string backward (ugh)
      memmove(p, p + 1, strlen(p + 1) + 1);
    }
    p++;
  }
}

static jsmntok_t *json_skip(jsmntok_t *t)
{
  int i;
  jsmntok_t *s;
  switch (t->type) {
  case JSMN_PRIMITIVE:
  case JSMN_STRING:
    t++;
    break;
  case JSMN_OBJECT:
  case JSMN_ARRAY:
    s = t;
    t++; // move onto first key
    for (i = 0; i < s->size; i++) {
      t = json_skip(t);
      if (s->type == JSMN_OBJECT) {
        t = json_skip(t);
      }
    }
  }

  return t;
}

#define json_str_assert(js, t, str)                                            \
  do {                                                                         \
    if (json_strcmp(js, t, str) != 0) {                                        \
      goto err;                                                                \
    }                                                                          \
  } while (0)

#define json_type_assert(t, asstype)                                           \
  do {                                                                         \
    if (t->type != asstype) {                                                  \
      goto err;                                                                \
    }                                                                          \
  } while (0)

#define NEXT_TOK t++

#define json_strcpy(dest, t, js)                                               \
  do {                                                                         \
    memcpy(dest, js + t->start, t->end - t->start);                            \
    dest[t->end - t->start] = '\0';                                            \
  } while (0)

#define json_strtoul(dest, t)                                                  \
  do {                                                                         \
    char intbuf[20];                                                           \
    char *endptr = NULL;                                                       \
    assert(t->end - t->start < 20);                                            \
    strncpy(intbuf, js + t->start, t->end - t->start);                         \
    intbuf[t->end - t->start] = '\0';                                          \
    dest = strtoul(intbuf, &endptr, 10);                                       \
    if (*endptr != '\0') {                                                     \
      goto err;                                                                \
    }                                                                          \
  } while (0)

static int process_json(bgpstream_broker_datasource_t *broker_ds,
                        bgpstream_input_mgr_t *input_mgr, const char *js,
                        jsmntok_t *root_tok, size_t count)
{
  int i, j, k;
  jsmntok_t *t = root_tok + 1;

  int arr_len, obj_len;

  int time_set = 0;

  int num_results = 0;

  // per-file info
  char *url = NULL;
  size_t url_len = 0;
  int url_set = 0;
  char collector[BGPSTREAM_UTILS_STR_NAME_LEN] = "";
  int collector_set = 0;
  char project[BGPSTREAM_UTILS_STR_NAME_LEN] = "";
  int project_set = 0;
  char type[BGPSTREAM_UTILS_STR_NAME_LEN] = "";
  int type_set = 0;
  uint32_t initial_time = 0;
  int initial_time_set = 0;
  uint32_t duration = 0;
  int duration_set = 0;

  if (count == 0) {
    fprintf(stderr, "ERROR: Empty JSON response from broker\n");
    goto retry;
  }

  if (root_tok->type != JSMN_OBJECT) {
    fprintf(stderr, "ERROR: Root object is not JSON\n");
    fprintf(stderr, "INFO: JSON: %s\n", js);
    goto err;
  }

  // iterate over the children of the root object
  for (i = 0; i < root_tok->size; i++) {
    // all keys must be strings
    if (t->type != JSMN_STRING) {
      fprintf(stderr, "ERROR: Encountered non-string key: '%.*s'\n",
              t->end - t->start, js + t->start);
      goto err;
    }
    if (json_strcmp(js, t, "time") == 0) {
      NEXT_TOK;
      json_type_assert(t, JSMN_PRIMITIVE);
      json_strtoul(broker_ds->last_response_time, t);
      time_set = 1;
      NEXT_TOK;
    } else if (json_strcmp(js, t, "type") == 0) {
      NEXT_TOK;
      json_str_assert(js, t, "data");
      NEXT_TOK;
    } else if (json_strcmp(js, t, "error") == 0) {
      NEXT_TOK;
      if (json_isnull(js, t) == 0) { // i.e. there is an error set
        fprintf(stderr, "ERROR: Broker reported an error: %.*s\n",
                t->end - t->start, js + t->start);
        goto err;
      }
      NEXT_TOK;
    } else if (json_strcmp(js, t, "queryParameters") == 0) {
      NEXT_TOK;
      json_type_assert(t, JSMN_OBJECT);
      // skip over this object
      t = json_skip(t);
    } else if (json_strcmp(js, t, "data") == 0) {
      NEXT_TOK;
      json_type_assert(t, JSMN_OBJECT);
      NEXT_TOK;
      json_str_assert(js, t, "dumpFiles");
      NEXT_TOK;
      json_type_assert(t, JSMN_ARRAY);
      arr_len = t->size; // number of dump files
      NEXT_TOK;          // first elem in array
      for (j = 0; j < arr_len; j++) {
        json_type_assert(t, JSMN_OBJECT);
        obj_len = t->size;
        NEXT_TOK;

        url_set = 0;
        project_set = 0;
        collector_set = 0;
        type_set = 0;
        initial_time_set = 0;
        duration_set = 0;

        for (k = 0; k < obj_len; k++) {
          if (json_strcmp(js, t, "urlType") == 0) {
            NEXT_TOK;
            if (json_strcmp(js, t, "simple") != 0) {
              // not yet supported?
              fprintf(stderr, "ERROR: Unsupported URL type '%.*s'\n",
                      t->end - t->start, js + t->start);
              goto err;
            }
            NEXT_TOK;
          } else if (json_strcmp(js, t, "url") == 0) {
            NEXT_TOK;
            json_type_assert(t, JSMN_STRING);
            if (url_len < (t->end - t->start + 1)) {
              url_len = t->end - t->start + 1;
              if ((url = realloc(url, url_len)) == NULL) {
                fprintf(stderr, "ERROR: Could not realloc URL string\n");
                goto err;
              }
            }
            json_strcpy(url, t, js);
            unescape_url(url);
            url_set = 1;
            NEXT_TOK;
          } else if (json_strcmp(js, t, "project") == 0) {
            NEXT_TOK;
            json_type_assert(t, JSMN_STRING);
            json_strcpy(project, t, js);
            project_set = 1;
            NEXT_TOK;
          } else if (json_strcmp(js, t, "collector") == 0) {
            NEXT_TOK;
            json_type_assert(t, JSMN_STRING);
            json_strcpy(collector, t, js);
            collector_set = 1;
            NEXT_TOK;
          } else if (json_strcmp(js, t, "type") == 0) {
            NEXT_TOK;
            json_type_assert(t, JSMN_STRING);
            json_strcpy(type, t, js);
            type_set = 1;
            NEXT_TOK;
          } else if (json_strcmp(js, t, "initialTime") == 0) {
            NEXT_TOK;
            json_type_assert(t, JSMN_PRIMITIVE);
            json_strtoul(initial_time, t);
            initial_time_set = 1;
            NEXT_TOK;
          } else if (json_strcmp(js, t, "duration") == 0) {
            NEXT_TOK;
            json_type_assert(t, JSMN_PRIMITIVE);
            json_strtoul(duration, t);
            duration_set = 1;
            NEXT_TOK;
          } else {
            fprintf(stderr, "ERROR: Unknown field '%.*s'\n", t->end - t->start,
                    js + t->start);
            goto err;
          }
        }
        // file obj has been completely read
        if (url_set == 0 || project_set == 0 || collector_set == 0 ||
            type_set == 0 || initial_time_set == 0 || duration_set == 0) {
          fprintf(stderr, "ERROR: Invalid dumpFile record\n");
          goto retry;
        }
#ifdef WITH_BROKER_DEBUG
        fprintf(stderr, "----------\n");
        fprintf(stderr, "URL: %s\n", url);
        fprintf(stderr, "Project: %s\n", project);
        fprintf(stderr, "Collector: %s\n", collector);
        fprintf(stderr, "Type: %s\n", type);
        fprintf(stderr, "InitialTime: %" PRIu32 "\n", initial_time);
        fprintf(stderr, "Duration: %" PRIu32 "\n", duration);
#endif

        // do we need to update our current_window_end?
        if (initial_time + duration > broker_ds->current_window_end) {
          broker_ds->current_window_end = (initial_time + duration);
        }

        if (bgpstream_input_mgr_push_sorted_input(
              input_mgr, strdup(url), strdup(project), strdup(collector),
              strdup(type), initial_time, duration) <= 0) {
          goto err;
        }

        num_results++;
      }
    }
    // TODO: handle unknown tokens
  }

  if (time_set == 0) {
    goto err;
  }

  free(url);
  return num_results;

retry:
  free(url);
  return ERR_RETRY;

err:
  fprintf(stderr, "ERROR: Invalid JSON response received from broker\n");
  free(url);
  return ERR_RETRY;
}

static int read_json(bgpstream_broker_datasource_t *broker_ds,
                     bgpstream_input_mgr_t *input_mgr, io_t *jsonfile)
{
  jsmn_parser p;
  jsmntok_t *tok = NULL;
  size_t tokcount = 128;

  int ret;
  char *js = NULL;
  size_t jslen = 0;
#define BUFSIZE 1024
  char buf[BUFSIZE];

  // prepare parser
  jsmn_init(&p);

  // allocate some tokens to start
  if ((tok = malloc(sizeof(jsmntok_t) * tokcount)) == NULL) {
    fprintf(stderr, "ERROR: Could not malloc initial tokens\n");
    goto err;
  }

  // slurp the whole file into a buffer
  while (1) {
    /* do a read */
    ret = wandio_read(jsonfile, buf, BUFSIZE);
    if (ret < 0) {
      fprintf(stderr, "ERROR: Reading from broker failed\n");
      goto err;
    }
    if (ret == 0) {
      // we're done
      break;
    }
    if ((js = realloc(js, jslen + ret + 1)) == NULL) {
      fprintf(stderr, "ERROR: Could not realloc json string\n");
      goto err;
    }
    strncpy(js + jslen, buf, ret);
    jslen += ret;
  }

again:
  if ((ret = jsmn_parse(&p, js, jslen, tok, tokcount)) < 0) {
    if (ret == JSMN_ERROR_NOMEM) {
      tokcount *= 2;
      if ((tok = realloc(tok, sizeof(jsmntok_t) * tokcount)) == NULL) {
        fprintf(stderr, "ERROR: Could not realloc tokens\n");
        goto err;
      }
      goto again;
    }
    if (ret == JSMN_ERROR_INVAL) {
      fprintf(stderr, "ERROR: Invalid character in JSON string\n");
      goto err;
    }
    fprintf(stderr, "ERROR: JSON parser returned %d\n", ret);
    goto err;
  }
  ret = process_json(broker_ds, input_mgr, js, tok, p.toknext);

  free(js);
  free(tok);
  if (ret == ERR_FATAL) {
    fprintf(stderr, "ERROR: Received fatal error from process_json\n");
  }
  return ret;

err:
  free(js);
  free(tok);
  fprintf(stderr, "%s: Returning fatal error code\n", __func__);
  return ERR_FATAL;
}

bgpstream_broker_datasource_t *
bgpstream_broker_datasource_create(bgpstream_filter_mgr_t *filter_mgr,
                                   char *broker_url, char **params,
                                   int params_cnt)
{
  int i;
  bgpstream_debug("\t\tBSDS_BROKER: create broker_ds start");
  bgpstream_broker_datasource_t *broker_ds;

  if ((broker_ds = malloc_zero(sizeof(bgpstream_broker_datasource_t))) ==
      NULL) {
    bgpstream_log_err(
      "\t\tBSDS_BROKER: create broker_ds can't allocate memory");
    goto err;
  }
  if (broker_url == NULL) {
    bgpstream_log_err("\t\tBSDS_BROKER: create broker_ds no file provided");
    goto err;
  }
  broker_ds->filter_mgr = filter_mgr;
  broker_ds->first_param = 1;
  broker_ds->query_url_remaining = URL_BUFLEN;
  broker_ds->query_url_buf[0] = '\0';

  // http://bgpstream.caida.org/broker (e.g.)
  APPEND_STR(broker_url);

  // http://bgpstream.caida.org/broker/data?
  APPEND_STR("/data");

  // projects, collectors, bgp_types, and time_intervals are used as filters
  // only if they are provided by the user
  bgpstream_interval_filter_t *tif;

  // projects
  char *f;
  if (filter_mgr->projects != NULL) {
    bgpstream_str_set_rewind(filter_mgr->projects);
    while ((f = bgpstream_str_set_next(filter_mgr->projects)) != NULL) {
      AMPORQ;
      APPEND_STR("projects[]=");
      APPEND_STR(f);
    }
  }
  // collectors
  if (filter_mgr->collectors != NULL) {
    bgpstream_str_set_rewind(filter_mgr->collectors);
    while ((f = bgpstream_str_set_next(filter_mgr->collectors)) != NULL) {
      AMPORQ;
      APPEND_STR("collectors[]=");
      APPEND_STR(f);
    }
  }
  // bgp_types
  if (filter_mgr->bgp_types != NULL) {
    bgpstream_str_set_rewind(filter_mgr->bgp_types);
    while ((f = bgpstream_str_set_next(filter_mgr->bgp_types)) != NULL) {
      AMPORQ;
      APPEND_STR("types[]=");
      APPEND_STR(f);
    }
  }

  // user-provided params
  for (i = 0; i < params_cnt; i++) {
    assert(params[i] != NULL);
    AMPORQ;
    APPEND_STR(params[i]);
  }

// time_intervals
#define BUFLEN 20
  char int_buf[BUFLEN];
  if (filter_mgr->time_intervals != NULL) {
    tif = filter_mgr->time_intervals;

    while (tif != NULL) {
      AMPORQ;
      APPEND_STR("intervals[]=");

      // BEGIN TIME
      if (snprintf(int_buf, BUFLEN, "%" PRIu32, tif->begin_time) >= BUFLEN) {
        goto err;
      }
      APPEND_STR(int_buf);
      APPEND_STR(",");

      // END TIME
      if (snprintf(int_buf, BUFLEN, "%" PRIu32, tif->end_time) >= BUFLEN) {
        goto err;
      }
      APPEND_STR(int_buf);

      tif = tif->next;
    }
  }

  // grab pointer to the end of the current string to simplify modifying the
  // query later
  broker_ds->query_url_end =
    broker_ds->query_url_buf + strlen(broker_ds->query_url_buf);
  assert((*broker_ds->query_url_end) == '\0');

  bgpstream_debug("\t\tBSDS_BROKER: create broker_ds end");

  return broker_ds;
err:
  bgpstream_broker_datasource_destroy(broker_ds);
  return NULL;
}

int bgpstream_broker_datasource_update_input_queue(
  bgpstream_broker_datasource_t *broker_ds, bgpstream_input_mgr_t *input_mgr)
{

// we need to set two parameters:
//  - dataAddedSince ("time" from last response we got)
//  - minInitialTime (max("initialTime"+"duration") of any file we've ever seen)

#define BUFLEN 20
  char buf[BUFLEN];

  io_t *jsonfile = NULL;
  ;

  int num_results;

  int attempts = 0;
  int wait_time = 1;

  int success = 0;

  if (broker_ds->last_response_time > 0) {
    // need to add dataAddedSince
    if (snprintf(buf, BUFLEN, "%" PRIu32, broker_ds->last_response_time) >=
        BUFLEN) {
      fprintf(stderr, "ERROR: Could not build dataAddedSince param string\n");
      goto err;
    }
    AMPORQ;
    APPEND_STR("dataAddedSince=");
    APPEND_STR(buf);
  }

  if (broker_ds->current_window_end > 0) {
    // need to add minInitialTime
    if (snprintf(buf, BUFLEN, "%" PRIu32, broker_ds->current_window_end) >=
        BUFLEN) {
      fprintf(stderr, "ERROR: Could not build minInitialTime param string\n");
      goto err;
    }
    AMPORQ;
    APPEND_STR("minInitialTime=");
    APPEND_STR(buf);
  }

  do {
    if (attempts > 0) {
      fprintf(stderr, "WARN: Broker request failed, waiting %ds before retry\n",
              wait_time);
      sleep(wait_time);
      if (wait_time < MAX_WAIT_TIME) {
        wait_time *= 2;
      }
    }
    attempts++;

#ifdef WITH_BROKER_DEBUG
    fprintf(stderr, "\nQuery URL: \"%s\"\n", broker_ds->query_url_buf);
#endif

    if ((jsonfile = wandio_create(broker_ds->query_url_buf)) == NULL) {
      fprintf(stderr, "ERROR: Could not open %s for reading\n",
              broker_ds->query_url_buf);
      goto retry;
    }

    if ((num_results = read_json(broker_ds, input_mgr, jsonfile)) ==
        ERR_FATAL) {
      fprintf(stderr, "ERROR: Received fatal error code from read_json\n");
      goto err;
    } else if (num_results == ERR_RETRY) {
      goto retry;
    } else {
      // success!
      success = 1;
    }

  retry:
    if (jsonfile != NULL) {
      wandio_destroy(jsonfile);
      jsonfile = NULL;
    }
  } while (success == 0);

  // reset the variable params
  *broker_ds->query_url_end = '\0';
  broker_ds->query_url_remaining =
    URL_BUFLEN - strlen(broker_ds->query_url_end);
  return num_results;

err:
  fprintf(stderr, "ERROR: Fatal error in broker data source\n");
  if (jsonfile != NULL) {
    wandio_destroy(jsonfile);
  }
  return -1;
}

void bgpstream_broker_datasource_destroy(
  bgpstream_broker_datasource_t *broker_ds)
{
  if (broker_ds == NULL) {
    return;
  }

  free(broker_ds);
}
