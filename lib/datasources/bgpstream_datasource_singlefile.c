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

#include "bgpstream_datasource_singlefile.h"
#include "bgpstream_debug.h"
#include "config.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <wandio.h>

/* check for new ribs once every 30 mins */
#define RIB_FREQUENCY_CHECK 1800
/* check for new updates once every 2 minutes */
#define UPDATE_FREQUENCY_CHECK 120

#define MAX_HEADER_READ_BYTES 1024
static unsigned char buffer[MAX_HEADER_READ_BYTES];

struct struct_bgpstream_singlefile_datasource_t {
  bgpstream_filter_mgr_t *filter_mgr;
  char rib_filename[BGPSTREAM_DUMP_MAX_LEN];
  unsigned char rib_header[MAX_HEADER_READ_BYTES];
  uint32_t last_rib_filetime;
  char update_filename[BGPSTREAM_DUMP_MAX_LEN];
  unsigned char update_header[MAX_HEADER_READ_BYTES];
  uint32_t last_update_filetime;
};

bgpstream_singlefile_datasource_t *
bgpstream_singlefile_datasource_create(bgpstream_filter_mgr_t *filter_mgr,
                                       char *singlefile_rib_mrtfile,
                                       char *singlefile_upd_mrtfile)
{
  bgpstream_debug("\t\tBSDS_CLIST: create singlefile_ds start");
  bgpstream_singlefile_datasource_t *singlefile_ds =
    (bgpstream_singlefile_datasource_t *)malloc(
      sizeof(bgpstream_singlefile_datasource_t));
  if (singlefile_ds == NULL) {
    bgpstream_log_err(
      "\t\tBSDS_CLIST: create singlefile_ds can't allocate memory");
    return NULL; // can't allocate memory
  }
  singlefile_ds->filter_mgr = filter_mgr;
  singlefile_ds->rib_filename[0] = '\0';
  singlefile_ds->rib_header[0] = '\0';
  singlefile_ds->last_rib_filetime = 0;
  if (singlefile_rib_mrtfile != NULL) {
    strcpy(singlefile_ds->rib_filename, singlefile_rib_mrtfile);
  }
  singlefile_ds->update_filename[0] = '\0';
  singlefile_ds->update_header[0] = '\0';
  singlefile_ds->last_update_filetime = 0;
  if (singlefile_upd_mrtfile != NULL) {
    strcpy(singlefile_ds->update_filename, singlefile_upd_mrtfile);
  }
  bgpstream_debug("\t\tBSDS_CLIST: create customlist_ds end");
  return singlefile_ds;
}

static int same_header(char *mrt_filename, unsigned char *previous_header)
{
  off_t read_bytes;
  io_t *io_h = wandio_create(mrt_filename);
  if (io_h == NULL) {
    bgpstream_log_err("\t\tBSDS_SINGLEFILE: can't open file!");
    return -1;
  }

  read_bytes = wandio_read(io_h, (void *)&(buffer[0]), MAX_HEADER_READ_BYTES);
  if (read_bytes < 0) {
    bgpstream_log_err("\t\tBSDS_SINGLEFILE: can't read file!");
    wandio_destroy(io_h);
    return -1;
  }

  int ret = memcmp(buffer, previous_header, sizeof(unsigned char) * read_bytes);
  wandio_destroy(io_h);
  /* if there is no difference, then they have the same header */
  if (ret == 0) {
    /* fprintf(stderr, "same header\n"); */
    return 1;
  }
  memcpy(previous_header, buffer, sizeof(unsigned char) * read_bytes);
  return 0;
}

int bgpstream_singlefile_datasource_update_input_queue(
  bgpstream_singlefile_datasource_t *singlefile_ds,
  bgpstream_input_mgr_t *input_mgr)
{
  bgpstream_debug("\t\tBSDS_CLIST: singlefile_ds update input queue start");
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint32_t now = tv.tv_sec;
  int num_results = 0;

  /* check digest, if different (or first) then add files to input queue) */
  if (singlefile_ds->rib_filename[0] != '\0' &&
      now - singlefile_ds->last_rib_filetime > RIB_FREQUENCY_CHECK &&
      same_header(singlefile_ds->rib_filename, singlefile_ds->rib_header) ==
        0) {
    /* fprintf(stderr, "new RIB at: %"PRIu32"\n", now); */
    singlefile_ds->last_rib_filetime = now;
    num_results += bgpstream_input_mgr_push_sorted_input(
      input_mgr, strdup(singlefile_ds->rib_filename), strdup("singlefile_ds"),
      strdup("singlefile_ds"), strdup("ribs"), singlefile_ds->last_rib_filetime,
      RIB_FREQUENCY_CHECK);
  }

  if (singlefile_ds->update_filename[0] != '\0' &&
      now - singlefile_ds->last_update_filetime > UPDATE_FREQUENCY_CHECK &&
      same_header(singlefile_ds->update_filename,
                  singlefile_ds->update_header) == 0) {
    /* fprintf(stderr, "new updates at: %"PRIu32"\n", now); */
    singlefile_ds->last_update_filetime = now;
    num_results += bgpstream_input_mgr_push_sorted_input(
      input_mgr, strdup(singlefile_ds->update_filename),
      strdup("singlefile_ds"), strdup("singlefile_ds"), strdup("updates"),
      singlefile_ds->last_update_filetime, UPDATE_FREQUENCY_CHECK);
  }

  bgpstream_debug("\t\tBSDS_CLIST: singlefile_ds update input queue end");
  return num_results;
}

void bgpstream_singlefile_datasource_destroy(
  bgpstream_singlefile_datasource_t *singlefile_ds)
{
  bgpstream_debug("\t\tBSDS_CLIST: destroy singlefile_ds start");
  if (singlefile_ds == NULL) {
    return; // nothing to destroy
  }
  singlefile_ds->filter_mgr = NULL;
  free(singlefile_ds);
  bgpstream_debug("\t\tBSDS_CLIST: destroy singlefile_ds end");
}
