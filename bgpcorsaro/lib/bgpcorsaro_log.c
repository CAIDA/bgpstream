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
#include "bgpcorsaro_int.h"
#include "config.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "utils.h"
#include "wandio_utils.h"

#include "bgpcorsaro_io.h"
#include "bgpcorsaro_log.h"

static char *timestamp_str(char *buf, const size_t len)
{
  struct timeval tv;
  struct tm *tm;
  int ms;
  time_t t;

  buf[0] = '\0';
  gettimeofday_wrap(&tv);
  t = tv.tv_sec;
  if ((tm = localtime(&t)) == NULL)
    return buf;

  ms = tv.tv_usec / 1000;
  snprintf(buf, len, "[%02d:%02d:%02d:%03d] ", tm->tm_hour, tm->tm_min,
           tm->tm_sec, ms);

  return buf;
}

void generic_log(const char *func, iow_t *logfile, const char *format,
                 va_list ap)
{
  char message[512];
  char ts[16];
  char fs[64];

  assert(format != NULL);

  vsnprintf(message, sizeof(message), format, ap);

  timestamp_str(ts, sizeof(ts));

  if (func != NULL)
    snprintf(fs, sizeof(fs), "%s: ", func);
  else
    fs[0] = '\0';

  if (logfile == NULL) {
    fprintf(stderr, "%s%s%s\n", ts, fs, message);
    fflush(stderr);
  } else {
    wandio_printf(logfile, "%s%s%s\n", ts, fs, message);
/*wandio_flush(logfile);*/

#ifdef DEBUG
    /* we've been asked to dump debugging information */
    fprintf(stderr, "%s%s%s\n", ts, fs, message);
    fflush(stderr);
#endif
  }
}

void bgpcorsaro_log_va(const char *func, bgpcorsaro_t *bgpcorsaro,
                       const char *format, va_list args)
{
  iow_t *lf = (bgpcorsaro == NULL) ? NULL : bgpcorsaro->logfile;
  generic_log(func, lf, format, args);
}

void bgpcorsaro_log(const char *func, bgpcorsaro_t *bgpcorsaro,
                    const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  bgpcorsaro_log_va(func, bgpcorsaro, format, ap);
  va_end(ap);
}

void bgpcorsaro_log_file(const char *func, iow_t *logfile, const char *format,
                         ...)
{
  va_list ap;
  va_start(ap, format);
  generic_log(func, logfile, format, ap);
  va_end(ap);
}

int bgpcorsaro_log_init(bgpcorsaro_t *bgpcorsaro)
{
  if ((bgpcorsaro->logfile = bgpcorsaro_io_prepare_file_full(
         bgpcorsaro, BGPCORSARO_IO_LOG_NAME, &bgpcorsaro->interval_start,
         WANDIO_COMPRESS_NONE, 0, O_CREAT)) == NULL) {
    fprintf(stderr, "could not open log for writing\n");
    return -1;
  }
  return 0;
}

void bgpcorsaro_log_close(bgpcorsaro_t *bgpcorsaro)
{
  if (bgpcorsaro->logfile != NULL) {
    wandio_wdestroy(bgpcorsaro->logfile);
    bgpcorsaro->logfile = NULL;
  }
}
