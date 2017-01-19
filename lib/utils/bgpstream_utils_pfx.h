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

#ifndef __BGPSTREAM_UTILS_PFX_H
#define __BGPSTREAM_UTILS_PFX_H

#include "bgpstream_utils_addr.h"

#define BGPSTREAM_PREFIX_MATCH_ANY 0
#define BGPSTREAM_PREFIX_MATCH_EXACT 1
#define BGPSTREAM_PREFIX_MATCH_MORE 2
#define BGPSTREAM_PREFIX_MATCH_LESS 3

/** @file
 *
 * @brief Header file that exposes the public interface of BGP Stream Prefix
 * objects
 *
 * @author Chiara Orsini
 *
 */

/**
 * @name Public Data Structures
 *
 * @{ */

/** A generic BGP Stream Prefix
 *
 * Must only be used for casting to the appropriate type based on the address
 * version
 */
typedef struct struct_bgpstream_pfx_t {

  /** Length of the prefix mask */
  uint8_t mask_len;

  /** Indicates what type of matches are allowed with this prefix.
   *  For filtering purposes only.
   */
  uint8_t allowed_matches;

  /** Pointer to the address portion */
  bgpstream_ip_addr_t address;

} bgpstream_pfx_t;

/** An IPv4 BGP Stream Prefix */
typedef struct struct_bgpstream_ipv4_pfx_t {

  /** Length of the prefix mask */
  uint8_t mask_len;

  /** Indicates what type of matches are allowed with this prefix.
   *  For filtering purposes only.
   */
  uint8_t allowed_matches;

  /** The address */
  bgpstream_ipv4_addr_t address;

} bgpstream_ipv4_pfx_t;

/** An IPv6 BGP Stream Prefix */
typedef struct struct_bgpstream_ipv6_pfx_t {

  /** Length of the prefix mask */
  uint8_t mask_len;

  /** Indicates what type of matches are allowed with this prefix.
   *  For filtering purposes only.
   */
  uint8_t allowed_matches;

  /** The address */
  bgpstream_ipv6_addr_t address;

} bgpstream_ipv6_pfx_t;

/** Generic storage for a BGP Stream Prefix */
typedef struct struct_bgpstream_pfx_storage_t {

  /** Length of the prefix mask */
  uint8_t mask_len;

  /** Indicates what type of matches are allowed with this prefix.
   *  For filtering purposes only.
   */
  uint8_t allowed_matches;

  /** The address */
  bgpstream_addr_storage_t address;

} bgpstream_pfx_storage_t;

/** @} */

/**
 * @name Public API Functions
 *
 * @{ */

/** Write the string representation of the given prefix into the given
 * character buffer.
 *
 * @param buf           pointer to a character buffer at least len bytes long
 * @param len           length of the given character buffer
 * @param pfx           pointer to the bgpstream pfx to convert to string
 * @param returns a pointer to buf if successful, NULL otherwise
 *
 * You will likely want to use INET_ADDRSTRLEN+3 or INET6_ADDRSTRLEN+3 to
 * dimension the buffer.
 */
char *bgpstream_pfx_snprintf(char *buf, size_t len, bgpstream_pfx_t *pfx);

/** Hash the given IPv4 Prefix into a 32bit number
 *
 * @param pfx          pointer to the IPv4 prefix to hash
 * @return 32bit hash of the prefix
 */
#if UINT_MAX == 0xffffffffu
unsigned int
#elif ULONG_MAX == 0xffffffffu
unsigned long
#endif
bgpstream_ipv4_pfx_hash(bgpstream_ipv4_pfx_t *pfx);

/** Hash the given IPv6 prefix into a 64bit number
 *
 * @param pfx          pointer to the IPv6 prefix to hash
 * @return 64bit hash of the prefix
 */
#if ULONG_MAX == ULLONG_MAX
unsigned long
#else
unsigned long long
#endif
bgpstream_ipv6_pfx_hash(bgpstream_ipv6_pfx_t *pfx);

/** Hash the given prefix storage into a 64bit number
 *
 * @param pfx          pointer to the prefix to hash
 * @return 64bit hash of the prefix
 */
#if ULONG_MAX == ULLONG_MAX
unsigned long
#else
unsigned long long
#endif
bgpstream_pfx_storage_hash(bgpstream_pfx_storage_t *pfx);

/** Compare two prefixes for equality
 *
 * @param pfx1          pointer to the first prefix to compare
 * @param pfx2          pointer to the first prefix to compare
 * @return 0 if the prefixes are not equal, non-zero if they are equal
 */
int bgpstream_pfx_equal(bgpstream_pfx_t *pfx1, bgpstream_pfx_t *pfx2);

/** Compare two IPv4 prefixes for equality
 *
 * @param pfx1         pointer to the first prefix to compare
 * @param pfx2         pointer to the second prefix to compare
 * @return 0 if the prefixes are not equal, non-zero if they are equal
 */
int bgpstream_ipv4_pfx_equal(bgpstream_ipv4_pfx_t *pfx1,
                             bgpstream_ipv4_pfx_t *pfx2);

/** Compare two IPv6 prefixes for equality
 *
 * @param pfx1         pointer to the first prefix to compare
 * @param pfx2         pointer to the second prefix to compare
 * @return 0 if the prefixes are not equal, non-zero if they are equal
 */
int bgpstream_ipv6_pfx_equal(bgpstream_ipv6_pfx_t *pfx1,
                             bgpstream_ipv6_pfx_t *pfx2);

/** Compare two generic prefixes for equality
 *
 * @param pfx1         pointer to the first prefix to compare
 * @param pfx2         pointer to the second prefix to compare
 * @return 0 if the prefixes are not equal, non-zero if they are equal
 */
int bgpstream_pfx_storage_equal(bgpstream_pfx_storage_t *pfx1,
                                bgpstream_pfx_storage_t *pfx2);

/** Check if one prefix contains another
 *
 * @param outer          pointer to the outer prefix
 * @param inner          pointer to the inner prefix to check
 * @return non-zero if inner is a more-specific prefix of outer, 0 if not
 */
int bgpstream_pfx_contains(bgpstream_pfx_t *outer, bgpstream_pfx_t *inner);

/** Utility macros used to pass khashes objects by reference
 *  instead of copying them */

#define bgpstream_pfx_storage_hash_val(arg) bgpstream_pfx_storage_hash(&(arg))
#define bgpstream_pfx_storage_equal_val(arg1, arg2)                            \
  bgpstream_pfx_storage_equal(&(arg1), &(arg2))

#define bgpstream_ipv4_pfx_storage_hash_val(arg) bgpstream_ipv4_pfx_hash(&(arg))
#define bgpstream_ipv4_pfx_storage_equal_val(arg1, arg2)                       \
  bgpstream_ipv4_pfx_equal(&(arg1), &(arg2))

#define bgpstream_ipv6_pfx_storage_hash_val(arg) bgpstream_ipv6_pfx_hash(&(arg))
#define bgpstream_ipv6_pfx_storage_equal_val(arg1, arg2)                       \
  bgpstream_ipv6_pfx_equal(&(arg1), &(arg2))

/** Convert a string into a prefix storage
 *
 * @param pfx_str     pointer a string representing an IP prefix
 * @param pfx         pointer to a prefix storage
 * @return the pointer to an initialized pfx storage, NULL if the
 *         prefix is not valid
 */
bgpstream_pfx_storage_t *bgpstream_str2pfx(const char *pfx_str,
                                           bgpstream_pfx_storage_t *pfx);

/** @} */

#endif /* __BGPSTREAM_UTILS_PFX_H */
