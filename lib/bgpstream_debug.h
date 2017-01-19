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
 *
 * Some code adapted from http://c.learncodethehardway.org/book/ex20.html
 */

#ifndef _BGPSTREAM_DEBUG_H
#define _BGPSTREAM_DEBUG_H

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define NDEBUG

// compile with NDEBUG defined -> then "no debug" messages will remain.
#ifdef NDEBUG
#define bgpstream_debug(M, ...)
#else
#define bgpstream_debug(M, ...)                                                \
  fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

// get a safe readable version of errno.
#define bgpstream_clean_errno() (errno == 0 ? "None" : strerror(errno))

// macros for logging messages meant for the end use
#define bgpstream_log_err(M, ...)                                              \
  fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__,    \
          bgpstream_clean_errno(), ##__VA_ARGS__)

#define bgpstream_log_warn(M, ...)                                             \
  fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__,     \
          bgpstream_clean_errno(), ##__VA_ARGS__)

#define bgpstream_log_info(M, ...)                                             \
  fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

// check will make sure the condition A is true, and if not logs the error M
// (with variable arguments for log_err), then jumps to the function's error:
// for cleanup.
#define bgpstream_check(A, M, ...)                                             \
  if (!(A)) {                                                                  \
    bgpstream_log_err(M, ##__VA_ARGS__);                                       \
    errno = 0;                                                                 \
    goto error;                                                                \
  }

// sentinel is placed in any part of a function that shouldn't run, and if it
// does prints an error message then jumps to the error: label. You put this in
// if-statements and switch-statements to catch conditions that shouldn't
// happen, like the default:.
#define bgpstream_sentinel(M, ...)                                             \
  {                                                                            \
    bgpstream_log_err(M, ##__VA_ARGS__);                                       \
    errno = 0;                                                                 \
    goto error;                                                                \
  }

// check_mem that makes sure a pointer is valid, and if it isn't reports it as
// an error with "Out of memory."
#define bgpstream_check_mem(A) bgpstream_check((A), "Out of memory.")

// An alternative macro check_debug that still checks and handles an error, but
// if the error is common then you don't want to bother reporting it. In this
// one it will use debug instead of log_err to report the message, so when you
// define NDEBUG the check still happens, the error jump goes off, but the
// message isn't printed.
#define bgpstream_check_debug(A, M, ...)                                       \
  if (!(A)) {                                                                  \
    bgpstream_debug(M, ##__VA_ARGS__);                                         \
    errno = 0;                                                                 \
    goto error;                                                                \
  }

#endif /* _BGPSTREAM_DEBUG_H */
