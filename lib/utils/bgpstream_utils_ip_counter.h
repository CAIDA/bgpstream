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

#ifndef __BGPSTREAM_UTILS_IPCNT_H
#define __BGPSTREAM_UTILS_IPCNT_H

#include <stdint.h>

#include "bgpstream_utils_pfx.h"

/** @file
 *
 * @brief Header file that exposes the public interface of BGP Stream IP
 * Counter objects
 *
 * @author Chiara Orsini
 *
 */

/**
 * @name Opaque Data Structures
 *
 * @{ */

/** Opaque structure containing an IP Counter instance */
typedef struct bgpstream_ip_counter bgpstream_ip_counter_t;

/** @} */

/**
 * @name Public API Functions
 *
 * @{ */

/** Create a new IP Counter instance
 *
 * @return a pointer to the structure, or NULL if an error occurred
 */
bgpstream_ip_counter_t *bgpstream_ip_counter_create();

/** Add a prefix to the IP Counter
 *
 * @param counter      pointer to the IP Counter
 * @param pfx          prefix to insert in IP Counter
 * @return             0 if a prefix was added correctly, -1 otherwise
 */
int bgpstream_ip_counter_add(bgpstream_ip_counter_t *ipc, bgpstream_pfx_t *pfx);

/** Get the number of unique IPs in the IP Counter
 *
 * @param counter        pointer to the IP Counter
 * @param v              IP version
 * @return               number of unique IPs in the IP Counter
 *                       (unique /32 in IPv4, unique /64 in IPv6)
 */
uint64_t bgpstream_ip_counter_get_ipcount(bgpstream_ip_counter_t *ipc,
                                          bgpstream_addr_version_t v);

/** Return the number of unique IPs in the IP Counter instance that
 *  overlap with the provided prefix
 *
 * @param counter        pointer to the IP Counter
 * @param pfx            prefix to compare
 * @param more_specific  it is set to 1 if the prefix is a more specific
 * @return               number of unique IPs in the IP Counter that
 *                       overlap with pfx
 */
uint64_t bgpstream_ip_counter_is_overlapping(bgpstream_ip_counter_t *ipc,
                                             bgpstream_pfx_t *pfx,
                                             uint8_t *more_specific);

/** Empty the IP Counter
 *
 * @param counter        pointer to the IP Counter to clear
 */
void bgpstream_ip_counter_clear(bgpstream_ip_counter_t *ipc);

/** Destroy the given IP Counter
 *
 * @param counter        pointer to the IP Counter to destroy
 */
void bgpstream_ip_counter_destroy(bgpstream_ip_counter_t *ipc);

#endif /* __BGPSTREAM_UTILS_IPCNT_H */
