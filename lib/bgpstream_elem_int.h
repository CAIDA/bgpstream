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

#ifndef __BGPSTREAM_ELEM_INT_H
#define __BGPSTREAM_ELEM_INT_H

#include "bgpstream_elem.h"
#include "bgpstream_utils.h"

/** @file
 *
 * @brief Header file that exposes the protected interface of a bgpstream elem.
 *
 * @author Chiara Orsini
 *
 */

/**
 * @name Protected API Functions
 *
 * @{ */

/** Write the string representation of the elem into the provided buffer
 *
 * @param buf           pointer to a char array
 * @param len           length of the char array
 * @param elem          pointer to a BGP Stream Elem to convert to string
 * @param print_type    1 to print the elem type, 0 to ignore the elem type
 * @return pointer to the start of the buffer if successful, NULL otherwise
 */
char *bgpstream_elem_custom_snprintf(char *buf, size_t len,
                                     const bgpstream_elem_t *elem,
                                     int print_type);

/** @} */

#endif /* __BGPSTREAM_ELEM_INT_H */
