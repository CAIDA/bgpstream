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

#ifndef __BGPSTREAM_UTILS_ADDR_H
#define __BGPSTREAM_UTILS_ADDR_H

#include <arpa/inet.h>
#include <limits.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>

/** @file
 *
 * @brief Header file that exposes the public interface of BGP Stream Address
 * objects
 *
 * @author Chiara Orsini
 *
 */

/**
 * @name Public Enums
 *
 * @{ */

/** Version of a BGP Stream IP Address
 *
 * These share values with AF_INET and AF_INET6 so that the version field of a
 * BGP Stream address can be used directly with standard address manipulation
 * functions
 */
typedef enum {

  /** Address type unknown */
  BGPSTREAM_ADDR_VERSION_UNKNOWN = 0,

  /** Address type IPv4 */
  BGPSTREAM_ADDR_VERSION_IPV4 = AF_INET,

  /** Address type IPv6 */
  BGPSTREAM_ADDR_VERSION_IPV6 = AF_INET6

} bgpstream_addr_version_t;

/** Maximum number of IP versions */
#define BGPSTREAM_MAX_IP_VERSION_IDX 2

/** @} */

/**
 * @name Public Data Structures
 *
 * @{ */

/** A generic BGP Stream IP address.
 *
 * Must only be used for casting to the appropriate type based on the version
 */
typedef struct struct_bgpstream_ip_addr_t {

  /** Version of the IP address */
  bgpstream_addr_version_t version;

  /** Pointer to the address portion */
  char addr[0];

} bgpstream_ip_addr_t;

/** An IPv4 BGP Stream IP address */
typedef struct struct_bgpstream_ipv4_addr_t {

  /** Version of the IP address (must always be BGPSTREAM_ADDR_VERSION_IPV4) */
  bgpstream_addr_version_t version;

  /** IPv4 Address */
  struct in_addr ipv4;

} bgpstream_ipv4_addr_t;

/** An IPv6 BGP Stream IP address */
typedef struct struct_bgpstream_ipv6_addr_t {

  /** Version of the IP address (must always be BGPSTREAM_ADDR_VERSION_IPV6 */
  bgpstream_addr_version_t version;

  /** IPv6 Address */
  struct in6_addr ipv6;

} bgpstream_ipv6_addr_t;

/** Generic storage for a BGP Stream IP address. */
typedef struct struct_bgpstream_addr_storage_t {

  /** Version of the IP address */
  bgpstream_addr_version_t version;

  /** IP Address */
  union {

    /** IPv4 Address */
    struct in_addr ipv4;

    /** IPv6 Address */
    struct in6_addr ipv6;
  };

} bgpstream_addr_storage_t;

/** @} */

/**
 * @name Public API Functions
 *
 * @{ */

/** Write the string representation of the given IP address into the given
 * character buffer.
 *
 * @param buf           pointer to a character buffer at least len bytes long
 * @param len           length of the given character buffer
 * @param bsaddr        pointer to the bgpstream addr to convert to string
 *
 * You will likely want to use INET_ADDRSTRLEN or INET6_ADDRSTRLEN to dimension
 * the buffer.
 */
#define bgpstream_addr_ntop(buf, len, bsaddr)                                  \
  inet_ntop((bsaddr)->version, &(((bgpstream_ip_addr_t *)(bsaddr))->addr),     \
            buf, len)

/** Hash the given IPv4 address into a 32bit number
 *
 * @param addr          pointer to the IPv4 address to hash
 * @return 32bit hash of the address
 */
#if UINT_MAX == 0xffffffffu
unsigned int
#elif ULONG_MAX == 0xffffffffu
unsigned long
#endif
bgpstream_ipv4_addr_hash(bgpstream_ipv4_addr_t *addr);

/** Hash the given IPv6 address into a 64bit number
 *
 * @param addr          pointer to the IPv6 address to hash
 * @return 64bit hash of the address
 */
#if ULONG_MAX == ULLONG_MAX
unsigned long
#else
unsigned long long
#endif
bgpstream_ipv6_addr_hash(bgpstream_ipv6_addr_t *addr);

/** Hash the given address storage into a 64bit number
 *
 * @param addr          pointer to the address to hash
 * @return 64bit hash of the address
 */
#if ULONG_MAX == ULLONG_MAX
unsigned long
#else
unsigned long long
#endif
bgpstream_addr_storage_hash(bgpstream_addr_storage_t *addr);

/** Compare two addresses for equality
 *
 * @param addr1         pointer to the first address to compare
 * @param addr2         pointer to the second address to compare
 * @return 0 if the addresses are not equal, non-zero if they are equal
 */
int bgpstream_addr_equal(bgpstream_ip_addr_t *addr1,
                         bgpstream_ip_addr_t *addr2);

/** Compare two IPv4 addresses for equality
 *
 * @param addr1         pointer to the first address to compare
 * @param addr2         pointer to the second address to compare
 * @return 0 if the addresses are not equal, non-zero if they are equal
 */
int bgpstream_ipv4_addr_equal(bgpstream_ipv4_addr_t *addr1,
                              bgpstream_ipv4_addr_t *addr2);

/** Compare two IPv6 addresses for equality
 *
 * @param addr1         pointer to the first address to compare
 * @param addr2         pointer to the second address to compare
 * @return 0 if the addresses are not equal, non-zero if they are equal
 */
int bgpstream_ipv6_addr_equal(bgpstream_ipv6_addr_t *addr1,
                              bgpstream_ipv6_addr_t *addr2);

/** Compare two generic addresses for equality
 *
 * @param addr1         pointer to the first address to compare
 * @param addr2         pointer to the second address to compare
 * @return 0 if the addresses are not equal, non-zero if they are equal
 */
int bgpstream_addr_storage_equal(bgpstream_addr_storage_t *addr1,
                                 bgpstream_addr_storage_t *ip2);

/** Apply a mask to the given IP address
 *
 * @param addr          pointer to the address to mask
 * @param mask          number of bits to mask
 * @return pointer to the address masked
 *
 * If the mask length is longer than the address length (32 for IPv4, 128 for
 * IPv6), then the address will be left unaffected.
 */
bgpstream_ip_addr_t *bgpstream_addr_mask(bgpstream_ip_addr_t *addr,
                                         uint8_t mask);

/** Apply a mask to the given IPv4 address
 *
 * @param addr          pointer to the address to mask
 * @param mask          number of bits to mask
 * @return pointer to the address masked
 *
 * If the mask length is longer than 31 then the address will be left
 * unaffected.
 */
bgpstream_ipv4_addr_t *bgpstream_ipv4_addr_mask(bgpstream_ipv4_addr_t *addr,
                                                uint8_t mask);

/** Apply a mask to the given IPv6 address
 *
 * @param addr          pointer to the address to mask
 * @param mask          number of bits to mask
 * @return pointer to the address masked
 *
 * If the mask length is longer than 127 then the address will be left
 * unaffected.
 */
bgpstream_ipv6_addr_t *bgpstream_ipv6_addr_mask(bgpstream_ipv6_addr_t *addr,
                                                uint8_t mask);

/** Copy one prefix into another
 *
 * @param dst          pointer to the destination prefix
 * @param src          pointer to the source prefix
 *
 * The destination address structure **must** be large enough to hold the source
 * address type (e.g., if src points to an address storage structure, it may be
 * copied into a destination v4 structure **iff** the src version is v4)
 */
void bgpstream_addr_copy(bgpstream_ip_addr_t *dst, bgpstream_ip_addr_t *src);

/** Convert a string into an address storage
 *
 * @param addr_str     pointer a string representing an IP address
 * @param addr         pointer to an address storage
 * @return the pointer to an initialized address storage, NULL if the
 *         address is not valid
 */
bgpstream_addr_storage_t *bgpstream_str2addr(char *addr_str,
                                             bgpstream_addr_storage_t *addr);

/** Returns the index associated to an IP version
 * @param v             enum rapresenting the IP address version
 * @return the index associated with the IP version, 255 if
 * there is an error in the translation
 */
uint8_t bgpstream_ipv2idx(bgpstream_addr_version_t v);

/** Returns the IP version associated with an index
 * @param i             index associated to the IP address version
 * @return the IP version associated with an index
 */
bgpstream_addr_version_t bgpstream_idx2ipv(uint8_t i);

/** Returns the number associated to an IP version
 * @param v             enum rapresenting the IP address version
 * @return the IP version number, 255 if there is an error in the
 * translation
 */
uint8_t bgpstream_ipv2number(bgpstream_addr_version_t v);

/** Returns the number associated to the index (associated to an IP version)
 * @param i             index associated to the IP address version
 * @return the index number, 255 if there is an error in the
 * translation
 */
uint8_t bgpstream_idx2number(uint8_t i);

/** @} */

#endif /* __BGPSTREAM_UTILS_ADDR_H */
