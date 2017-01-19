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
#include "bgpstream.h"
#include "config.h"

#define CHECK_MSG(name, err_msg, check)                                        \
  do {                                                                         \
    if (!(check)) {                                                            \
      fprintf(stderr, " * " name ": FAILED\n");                                \
      fprintf(stderr, " ! Failed check was: '" #check "'\n");                  \
      fprintf(stderr, err_msg "\n\n");                                         \
      return -1;                                                               \
    } else {                                                                   \
      fprintf(stderr, " * " name ": OK\n");                                    \
    }                                                                          \
  } while (0)

#define CHECK(name, check)                                                     \
  do {                                                                         \
    if (!(check)) {                                                            \
      fprintf(stderr, " * " name ": FAILED\n");                                \
      fprintf(stderr, " ! Failed check was: '" #check "'\n");                  \
      return -1;                                                               \
    } else {                                                                   \
      fprintf(stderr, " * " name ": OK\n");                                    \
    }                                                                          \
  } while (0)

#define CHECK_SECTION(name, check)                                             \
  do {                                                                         \
    fprintf(stderr, "Checking " name "...\n");                                 \
    if (!(check)) {                                                            \
      fprintf(stderr, name ": FAILED\n\n");                                    \
      return -1;                                                               \
    } else {                                                                   \
      fprintf(stderr, name ": OK\n\n");                                        \
    }                                                                          \
  } while (0)

#define SKIPPED(name)                                                          \
  do {                                                                         \
    fprintf(stderr, " * " name ": SKIPPED\n");                                 \
  } while (0)

#define SKIPPED_SECTION(name)                                                  \
  do {                                                                         \
    fprintf(stderr, name ": SKIPPED\n");                                       \
  } while (0)
