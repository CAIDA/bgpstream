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
#ifndef __BGPCORSARO_IO_H
#define __BGPCORSARO_IO_H

#include "config.h"

#include "bgpcorsaro_int.h"

#include "bgpcorsaro_plugin.h"

/** @file
 *
 * @brief Header file dealing with the bgpcorsaro file IO
 *
 * @author Alistair King
 *
 */

/** The suffix used to detect gzip output is desired */
#define BGPCORSARO_IO_ZLIB_SUFFIX ".gz"

/** The suffix used to detect bzip output is desired */
#define BGPCORSARO_IO_BZ2_SUFFIX ".bz2"

/** The character to replace with the name of the plugin */
#define BGPCORSARO_IO_PLUGIN_PATTERN 'X'
/** The pattern to replace in the output file name with the name of the plugin
 */
#define BGPCORSARO_IO_PLUGIN_PATTERN_STR "%X"

/** The character to replace with the monitor name */
#define BGPCORSARO_IO_MONITOR_PATTERN 'N'
/** The pattern to replace in the output file name with monitor name */
#define BGPCORSARO_IO_MONITOR_PATTERN_STR "%N"

/** The name to use for the log 'plugin' file */
#define BGPCORSARO_IO_LOG_NAME "log"

/** Uses the given settings to open an bgpcorsaro file for the given plugin
 *
 * @param bgpcorsaro          The bgpcorsaro object associated with the file
 * @param plugin_name    The name of the plugin (inserted into the template)
 * @param interval       The first interval start time represented in the file
 *                       (inserted into the template)
 * @param compress       The wandio file compression type to use
 * @param compress_level The wandio file compression level to use
 * @param flags          The flags to use when opening the file (e.g. O_CREAT)
 * @return A pointer to a new wandio output file, or NULL if an error occurs
 */
iow_t *bgpcorsaro_io_prepare_file_full(bgpcorsaro_t *bgpcorsaro,
                                       const char *plugin_name,
                                       bgpcorsaro_interval_t *interval,
                                       int compress, int compress_level,
                                       int flags);

/** Uses the current settings to open an bgpcorsaro file for the given plugin
 *
 * @param bgpcorsaro   The bgpcorsaro object associated with the file
 * @param plugin_name  The name of the plugin (inserted into the template)
 * @param interval     The first interval start time represented in the file
 *                     (inserted into the template)
 * @return A pointer to a new wandio output file, or NULL if an error occurs
 */
iow_t *bgpcorsaro_io_prepare_file(bgpcorsaro_t *bgpcorsaro,
                                  const char *plugin_name,
                                  bgpcorsaro_interval_t *interval);

/** Validates a output file template for needed features
 *
 * @param bgpcorsaro    The bgpcorsaro object associated with the template
 * @param template      The file template to be validated
 * @return 1 if the template is valid, 0 if it is invalid
 */
int bgpcorsaro_io_validate_template(bgpcorsaro_t *bgpcorsaro, char *template);

/** Determines whether there are any time-related patterns in the file template.
 *
 * @param bgpcorsaro       The bgpcorsaro object to check
 * @return 1 if there are time-related patterns, 0 if not
 */
int bgpcorsaro_io_template_has_timestamp(bgpcorsaro_t *bgpcorsaro);

/** Write the appropriate interval headers to the file
 *
 * @param bgpcorsaro     The bgpcorsaro object associated with the file
 * @param file           The wandio output file to write to
 * @param int_start      The start interval to write out
 * @return The amount of data written, or -1 if an error occurs
 */
off_t bgpcorsaro_io_write_interval_start(bgpcorsaro_t *bgpcorsaro, iow_t *file,
                                         bgpcorsaro_interval_t *int_start);

/** Write the interval headers to stdout
 *
 * @param int_start      The start interval to write out
 */
void bgpcorsaro_io_print_interval_start(bgpcorsaro_interval_t *int_start);

/** Write the appropriate interval trailers to the file
 *
 * @param bgpcorsaro     The bgpcorsaro object associated with the file
 * @param file           The wandio output file to write to
 * @param int_end        The end interval to write out
 * @return The amount of data written, or -1 if an error occurs
 */
off_t bgpcorsaro_io_write_interval_end(bgpcorsaro_t *bgpcorsaro, iow_t *file,
                                       bgpcorsaro_interval_t *int_end);

/** Write the interval trailers to stdout
 *
 * @param int_end      The end interval to write out
 */
void bgpcorsaro_io_print_interval_end(bgpcorsaro_interval_t *int_end);

/** Write the appropriate plugin header to the file
 *
 * @param bgpcorsaro     The bgpcorsaro object associated with the file
 * @param file           The wandio output file to write to
 * @param plugin         The plugin object to write a start record for
 * @return The amount of data written, or -1 if an error occurs
 */
off_t bgpcorsaro_io_write_plugin_start(bgpcorsaro_t *bgpcorsaro, iow_t *file,
                                       bgpcorsaro_plugin_t *plugin);

/** Write the appropriate plugin trailer to the file
 *
 * @param bgpcorsaro     The bgpcorsaro object associated with the file
 * @param file           The wandio output file to write to
 * @param plugin         The plugin object to write an end record for
 * @return The amount of data written, or -1 if an error occurs
 */
off_t bgpcorsaro_io_write_plugin_end(bgpcorsaro_t *bgpcorsaro, iow_t *file,
                                     bgpcorsaro_plugin_t *plugin);

#endif /* __BGPCORSARO_IO_H */
