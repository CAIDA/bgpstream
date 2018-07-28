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
#ifndef __BGPCORSARO_LOG_H
#define __BGPCORSARO_LOG_H

#include "config.h"

#include <stdarg.h>

#include "bgpcorsaro_int.h"

/** @file
 *
 * @brief Header file dealing with the bgpcorsaro logging sub-system
 *
 * @author Alistair King
 *
 */

/** Write a formatted string to the logfile associated with an bgpcorsaro object
 *
 * @param func         The name of the calling function (__func__)
 * @param bgpcorsaro   The bgpcorsaro output object to log for
 * @param format       The printf style formatting string
 * @param args         Variable list of arguments to the format string
 *
 * This function takes the same style of arguments that printf(3) does.
 */
void bgpcorsaro_log_va(const char *func, bgpcorsaro_t *bgpcorsaro,
                       const char *format, va_list args);

/** Write a formatted string to the logfile associated with an bgpcorsaro object
 *
 * @param func         The name of the calling function (__func__)
 * @param bgpcorsaro        The bgpcorsaro output object to log for
 * @param format       The printf style formatting string
 * @param ...          Variable list of arguments to the format string
 *
 * This function takes the same style of arguments that printf(3) does.
 */
void bgpcorsaro_log(const char *func, bgpcorsaro_t *bgpcorsaro,
                    const char *format, ...);

/** Write a formatted string to a generic log file
 *
 * @param func         The name of the calling function (__func__)
 * @param logfile      The file to log to (STDERR if NULL is passed)
 * @param format       The printf style formatting string
 * @param ...          Variable list of arguments to the format string
 *
 * This function takes the same style of arguments that printf(3) does.
 */
void bgpcorsaro_log_file(const char *func, iow_t *logfile, const char *format,
                         ...);

/** Initialize the logging sub-system for an bgpcorsaro output object
 *
 * @param bgpcorsaro        The bgpcorsaro object to associate the logger with
 * @return 0 if successful, -1 if an error occurs
 */
int bgpcorsaro_log_init(bgpcorsaro_t *bgpcorsaro);

/** Close the log file for an bgpcorsaro output object
 *
 * @param bgpcorsaro         The bgpcorsaro output object to close logging for
 */
void bgpcorsaro_log_close(bgpcorsaro_t *bgpcorsaro);

#endif /* __BGPCORSARO_LOG_H */
