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

/* This file written by Shane Alcock and freely contributed back to the
 * BGPStream project  <salcock@waikato.ac.nz>
 */

#include "config.h"

#include "bgpstream_utils_time.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int bgpstream_time_calc_recent_interval(uint32_t *start, uint32_t *end,
                                        const char *optval)
{

  char *timetok;
  char *unittok;
  uint32_t unitcount = 0;

  struct timeval tv;

  timetok = strtok((char *)optval, " ");

  if (timetok == NULL) {
    return 0;
  }

  unitcount = strtoul(timetok, NULL, 10);
  unittok = strtok(NULL, " ");

  if (!unittok || *unittok == '\0') {
    return 0;
  }

  switch (*unittok) {
  case 's':
    break;
  case 'm':
    unitcount = unitcount * 60;
    break;
  case 'h':
    unitcount = unitcount * 60 * 60;
    break;
  case 'd':
    unitcount = unitcount * 60 * 60 * 24;
    break;
  default:
    return 0;
  }

  if (gettimeofday(&tv, NULL)) {
    return 0;
  }

  *start = tv.tv_sec - unitcount;
  *end = tv.tv_sec;
  return 1;
}
