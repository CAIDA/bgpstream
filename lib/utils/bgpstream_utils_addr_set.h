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

#ifndef __BGPSTREAM_UTILS_ADDR_SET_H
#define __BGPSTREAM_UTILS_ADDR_SET_H

#include "bgpstream_utils_addr.h"

/** @file
 *
 * @brief Header file that exposes the public interface of the BGP Stream
 * Address Sets. There is one set for each bgpstream_addr type (Storage, IPv4
 * and IPv6).
 *
 * @author Chiara Orsini
 *
 */

/**
 * @name Opaque Data Structures
 *
 * @{ */

/** Opaque structure containing an Address Storage set instance */
typedef struct bgpstream_addr_storage_set bgpstream_addr_storage_set_t;

/** Opaque structure containing an IPv4 Address set instance */
typedef struct bgpstream_ipv4_addr_set bgpstream_ipv4_addr_set_t;

/** Opaque structure containing an IPv6 Address set instance */
typedef struct bgpstream_ipv6_addr_set bgpstream_ipv6_addr_set_t;

/** @} */

/**
 * @name Public API Functions
 *
 * @{ */

/* STORAGE */

/** Create a new Address Storage set instance
 *
 * @return a pointer to the structure, or NULL if an error occurred
 */
bgpstream_addr_storage_set_t *bgpstream_addr_storage_set_create();

/** Insert a new address into the given set.
 *
 * @param set           pointer to the address set
 * @param addr          address to insert in the set
 * @return 1 if the address was inserted, 0 if it already existed, -1 if an
 * error occurred
 */
int bgpstream_addr_storage_set_insert(bgpstream_addr_storage_set_t *set,
                                      bgpstream_addr_storage_t *addr);

/** Get the number of addresses in the given set
 *
 * @param set           pointer to the address set
 * @return the size of the address set
 */
int bgpstream_addr_storage_set_size(bgpstream_addr_storage_set_t *set);

/** Merge two address sets
 *
 * @param dst_set      pointer to the set to merge src into
 * @param src_set      pointer to the set to merge into dst
 * @return 0 if the sets were merged succsessfully, -1 otherwise
 */
int bgpstream_addr_storage_set_merge(bgpstream_addr_storage_set_t *dst_set,
                                     bgpstream_addr_storage_set_t *src_set);

/** Destroy the given address set
 *
 * @param set           pointer to the address set to destroy
 */
void bgpstream_addr_storage_set_destroy(bgpstream_addr_storage_set_t *set);

/** Empty the address set.
 *
 * @param set           pointer to the address set to clear
 */
void bgpstream_addr_storage_set_clear(bgpstream_addr_storage_set_t *set);

/* IPv4 */

/** Create a new IPv4 Address set instance
 *
 * @return a pointer to the structure, or NULL if an error occurred
 */
bgpstream_ipv4_addr_set_t *bgpstream_ipv4_addr_set_create();

/** Insert a new address into the given set.
 *
 * @param set           pointer to the address set
 * @param addr          address to insert in the set
 * @return 1 if the address was inserted, 0 if it already existed, -1 if an
 * error occurred
 *
 * This function takes a copy of the address before it is inserted in the set.
 */
int bgpstream_ipv4_addr_set_insert(bgpstream_ipv4_addr_set_t *set,
                                   bgpstream_ipv4_addr_t *addr);

/** Get the number of addresses in the given set
 *
 * @param set           pointer to the address set
 * @return the size of the address set
 */
int bgpstream_ipv4_addr_set_size(bgpstream_ipv4_addr_set_t *set);

/** Merge two address sets
 *
 * @param dst_set      pointer to the set to merge src into
 * @param src_set      pointer to the set to merge into dst
 * @return 0 if the sets were merged succsessfully, -1 otherwise
 */
int bgpstream_ipv4_addr_set_merge(bgpstream_ipv4_addr_set_t *dst_set,
                                  bgpstream_ipv4_addr_set_t *src_set);

/** Destroy the given address set
 *
 * @param set           pointer to the address set to destroy
 */
void bgpstream_ipv4_addr_set_destroy(bgpstream_ipv4_addr_set_t *set);

/** Empty the address set.
 *
 * @param set           pointer to the address set to clear
 */
void bgpstream_ipv4_addr_set_clear(bgpstream_ipv4_addr_set_t *set);

/** Create a new IPv6 Address set instance
 *
 * @return a pointer to the structure, or NULL if an error occurred
 */
bgpstream_ipv6_addr_set_t *bgpstream_ipv6_addr_set_create();

/** Insert a new address into the given set.
 *
 * @param set           pointer to the address set
 * @param addr          address to insert in the set
 * @return 1 if the address was inserted, 0 if it already existed, -1 if an
 * error occurred
 */
int bgpstream_ipv6_addr_set_insert(bgpstream_ipv6_addr_set_t *set,
                                   bgpstream_ipv6_addr_t *addr);

/** Get the number of addresses in the given set
 *
 * @param set           pointer to the address set
 * @return the size of the address set
 */
int bgpstream_ipv6_addr_set_size(bgpstream_ipv6_addr_set_t *set);

/** Merge two address sets
 *
 * @param dst_set      pointer to the set to merge src into
 * @param src_set      pointer to the set to merge into dst
 * @return 0 if the sets were merged succsessfully, -1 otherwise
 */
int bgpstream_ipv6_addr_set_merge(bgpstream_ipv6_addr_set_t *dst_set,
                                  bgpstream_ipv6_addr_set_t *src_set);

/** Destroy the given address set
 *
 * @param set           pointer to the address set to destroy
 */
void bgpstream_ipv6_addr_set_destroy(bgpstream_ipv6_addr_set_t *set);

/** Empty the address set.
 *
 * @param set           pointer to the address set to clear
 */
void bgpstream_ipv6_addr_set_clear(bgpstream_ipv6_addr_set_t *set);

#endif /* __BGPSTREAM_UTILS_ADDR_SET_H */
