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

#include "bgpcorsaro.h"
#include "bgpcorsaro_io.h"
#include "bgpcorsaro_log.h"
#include "bgpcorsaro_plugin.h"

#include "utils.h"
#include "wandio_utils.h"

static char *stradd(const char *str, char *bufp, char *buflim)
{
  while (bufp < buflim && (*bufp = *str++) != '\0') {
    ++bufp;
  }
  return bufp;
}

static char *generate_file_name(bgpcorsaro_t *bgpcorsaro, const char *plugin,
                                bgpcorsaro_interval_t *interval,
                                int compress_type)
{
  /* some of the structure of this code is borrowed from the
     FreeBSD implementation of strftime */

  /* the output buffer */
  /* @todo change the code to dynamically realloc this if we need more
     space */
  char buf[1024];
  char tbuf[1024];
  char *bufp = buf;
  char *buflim = buf + sizeof(buf);

  char *tmpl = bgpcorsaro->template;
  char secs[11]; /* length of UINT32_MAX +1 */
  struct timeval tv;

  for (; *tmpl; ++tmpl) {
    if (*tmpl == '.' && compress_type == WANDIO_COMPRESS_NONE) {
      if (strncmp(tmpl, BGPCORSARO_IO_ZLIB_SUFFIX,
                  strlen(BGPCORSARO_IO_ZLIB_SUFFIX)) == 0 ||
          strncmp(tmpl, BGPCORSARO_IO_BZ2_SUFFIX,
                  strlen(BGPCORSARO_IO_BZ2_SUFFIX)) == 0) {
        break;
      }
    } else if (*tmpl == '%') {
      switch (*++tmpl) {
      case '\0':
        --tmpl;
        break;

      /* BEWARE: if you add a new pattern here, you must also add it to
       * bgpcorsaro_io_template_has_timestamp */

      case BGPCORSARO_IO_MONITOR_PATTERN:
        bufp = stradd(bgpcorsaro->monitorname, bufp, buflim);
        continue;

      case BGPCORSARO_IO_PLUGIN_PATTERN:
        bufp = stradd(plugin, bufp, buflim);
        continue;

      case 's':
        if (interval != NULL) {
          snprintf(secs, sizeof(secs), "%" PRIu32, interval->time);
          bufp = stradd(secs, bufp, buflim);
          continue;
        }
      /* fall through */
      default:
        /* we want to be generous and leave non-recognized formats
           intact - especially for strftime to use */
        --tmpl;
      }
    }
    if (bufp == buflim)
      break;
    *bufp++ = *tmpl;
  }

  *bufp = '\0';

  /* now let strftime have a go */
  if (interval != NULL) {
    tv.tv_sec = interval->time;
    strftime(tbuf, sizeof(tbuf), buf, gmtime(&tv.tv_sec));
    return strdup(tbuf);
  }

  return strdup(buf);
}

/* == EXPORTED FUNCTIONS BELOW THIS POINT == */

iow_t *bgpcorsaro_io_prepare_file(bgpcorsaro_t *bgpcorsaro,
                                  const char *plugin_name,
                                  bgpcorsaro_interval_t *interval)
{
  return bgpcorsaro_io_prepare_file_full(bgpcorsaro, plugin_name, interval,
                                         bgpcorsaro->compress,
                                         bgpcorsaro->compress_level, O_CREAT);
}

iow_t *bgpcorsaro_io_prepare_file_full(bgpcorsaro_t *bgpcorsaro,
                                       const char *plugin_name,
                                       bgpcorsaro_interval_t *interval,
                                       int compress_type, int compress_level,
                                       int flags)
{
  iow_t *f = NULL;
  char *outfileuri;

  /* generate a file name based on the plugin name */
  if ((outfileuri = generate_file_name(bgpcorsaro, plugin_name, interval,
                                       compress_type)) == NULL) {
    bgpcorsaro_log(__func__, bgpcorsaro, "could not generate file name for %s",
                   plugin_name);
    return NULL;
  }

  if ((f = wandio_wcreate(outfileuri, compress_type, compress_level, flags)) ==
      NULL) {
    bgpcorsaro_log(__func__, bgpcorsaro, "could not open %s for writing",
                   outfileuri);
    return NULL;
  }

  free(outfileuri);
  return f;
}

int bgpcorsaro_io_validate_template(bgpcorsaro_t *bgpcorsaro, char *template)
{
  /* be careful using bgpcorsaro here, it is likely not initialized fully */

  /* check for length first */
  if (template == NULL) {
    bgpcorsaro_log(__func__, bgpcorsaro, "output template must be set");
    return 0;
  }

  /* check that the plugin pattern is in the template */
  if (strstr(template, BGPCORSARO_IO_PLUGIN_PATTERN_STR) == NULL) {
    bgpcorsaro_log(__func__, bgpcorsaro, "template string must contain %s",
                   BGPCORSARO_IO_PLUGIN_PATTERN_STR);
    return 0;
  }

  /* we're good! */
  return 1;
}

int bgpcorsaro_io_template_has_timestamp(bgpcorsaro_t *bgpcorsaro)
{
  char *p = bgpcorsaro->template;
  assert(bgpcorsaro->template);
  /* be careful using bgpcorsaro here, this is called pre-start */

  /* the easiest (but not easiest to maintain) way to do this is to step through
   * each '%' character in the string and check what is after it. if it is
   * anything other than P (for plugin) or N (for monitor name), then it is a
   * timestamp. HOWEVER. If new bgpcorsaro-specific patterns are added, they
   * must
   * also be added here. gross */

  for (; *p; ++p) {
    if (*p == '%') {
      /* BEWARE: if you add a new pattern here, you must also add it to
       * generate_file_name */
      if (*(p + 1) != BGPCORSARO_IO_MONITOR_PATTERN &&
          *(p + 1) != BGPCORSARO_IO_PLUGIN_PATTERN) {
        return 1;
      }
    }
  }
  return 0;
}

off_t bgpcorsaro_io_write_interval_start(bgpcorsaro_t *bgpcorsaro, iow_t *file,
                                         bgpcorsaro_interval_t *int_start)
{
  return wandio_printf(file, "# BGPCORSARO_INTERVAL_START %d %ld\n",
                       int_start->number, int_start->time);
}

void bgpcorsaro_io_print_interval_start(bgpcorsaro_interval_t *int_start)
{
  fprintf(stdout, "# BGPCORSARO_INTERVAL_START %d %" PRIu32 "\n",
          int_start->number, int_start->time);
}

off_t bgpcorsaro_io_write_interval_end(bgpcorsaro_t *bgpcorsaro, iow_t *file,
                                       bgpcorsaro_interval_t *int_end)
{
  return wandio_printf(file, "# BGPCORSARO_INTERVAL_END %d %ld\n",
                       int_end->number, int_end->time);
}

void bgpcorsaro_io_print_interval_end(bgpcorsaro_interval_t *int_end)
{
  fprintf(stdout, "# BGPCORSARO_INTERVAL_END %d %" PRIu32 "\n", int_end->number,
          int_end->time);
}

off_t bgpcorsaro_io_write_plugin_start(bgpcorsaro_t *bgpcorsaro, iow_t *file,
                                       bgpcorsaro_plugin_t *plugin)
{
  assert(plugin != NULL);

  return wandio_printf(file, "# BGPCORSARO_PLUGIN_DATA_START %s\n",
                       plugin->name);
}

void bgpcorsaro_io_print_plugin_start(bgpcorsaro_plugin_t *plugin)
{
  fprintf(stdout, "# BGPCORSARO_PLUGIN_DATA_START %s\n", plugin->name);
}

off_t bgpcorsaro_io_write_plugin_end(bgpcorsaro_t *bgpcorsaro, iow_t *file,
                                     bgpcorsaro_plugin_t *plugin)
{
  assert(plugin != NULL);

  return wandio_printf(file, "# BGPCORSARO_PLUGIN_DATA_END %s\n", plugin->name);
}

void bgpcorsaro_io_print_plugin_end(bgpcorsaro_plugin_t *plugin)
{
  fprintf(stdout, "# BGPCORSARO_PLUGIN_DATA_END %s\n", plugin->name);
}
