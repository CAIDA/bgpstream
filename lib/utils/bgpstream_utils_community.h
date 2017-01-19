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

#ifndef __BGPSTREAM_UTILS_COMMUNITY_H
#define __BGPSTREAM_UTILS_COMMUNITY_H

#include <limits.h>

/** @file
 *
 * @brief Header file that exposes the public interface of BGP Stream Community
 * class
 *
 * @author Chiara Orsini, Alistair King
 *
 */

/**
 * @name Public Constants
 *
 * @{ */

/* @} */

/**
 * @name Public Enums
 *
 * @{ */

/** Well-known Community Types (see RFC 1997) */

#define BGPSTREAM_COMMUNITY_NO_EXPORT 0xFFFFFF01

#define BGPSTREAM_COMMUNITY_NO_ADVERTISE 0xFFFFFF02

#define BGPSTREAM_COMMUNITY_NO_EXPORT_SUBCONFED 0xFFFFFF03

/** Community mask used for filtering */

#define BGPSTREAM_COMMUNITY_FILTER_EXACT 0b0011
#define BGPSTREAM_COMMUNITY_FILTER_ASN 0b0010
#define BGPSTREAM_COMMUNITY_FILTER_VALUE 0b0001

/** @} */

/**
 * @name Public Opaque Data Structures
 *
 * @{ */

/** Opaque pointer to a set of community values */
typedef struct bgpstream_community_set bgpstream_community_set_t;

/** @} */

/**
 * @name Public Data Structures
 *
 * @{ */

/** Community attribute value */
typedef struct bgpstream_community {

  uint16_t asn;
  uint16_t value;

} __attribute__((packed)) bgpstream_community_t;

/** @} */

/**
 * @name Public API Functions
 *
 * @{ */

/** Write the string representation of the given community into the given
 *  character buffer.
 *
 * @param buf           pointer to a character buffer at least len bytes long
 * @param len           length of the given character buffer
 * @param comm          pointer to the community value to convert to string
 * @return the number of characters written given an infinite len (not including
 * the trailing nul). If this value is greater than or equal to len, then the
 * output was truncated.
 */
int bgpstream_community_snprintf(char *buf, size_t len,
                                 bgpstream_community_t *comm);

/** Read the string representation of a community from the buffer and
 *  populate the community structure .
 *
 * @param buf           pointer to a character buffer at least len bytes long
 * @param comm          pointer to the community structure populate
 * @return 0 if the operation is successful, -1 otherwise.
 */
int bgpstream_str2community(const char *buf, bgpstream_community_t *comm);

/** Duplicate the given community
 *
 * @param src           pointer to the community to duplicate
 * @return a pointer to the created community if successful, NULL otherwise
 *
 * @note the returned community must be destroyed using
 * bgpstream_community_destroy
 */
bgpstream_community_t *bgpstream_community_dup(bgpstream_community_t *src);

/** Destroy the given community
 *
 * @param comm          pointer to the community to destroy
 */
void bgpstream_community_destroy(bgpstream_community_t *comm);

/** Hash the given community into a 32bit number
 *
 * @param comm          pointer to the community to hash
 * @return 32bit hash of the community
 */
#if UINT_MAX == 0xffffffffu
unsigned int
#elif ULONG_MAX == 0xffffffffu
unsigned long
#endif
bgpstream_community_hash(bgpstream_community_t *comm);

/** Hash the given community into a 32bit number
 *
 * @param comm          community to hash
 * @return 32bit hash of the community
 */
#if UINT_MAX == 0xffffffffu
unsigned int
#elif ULONG_MAX == 0xffffffffu
unsigned long
#endif
bgpstream_community_hash_value(bgpstream_community_t comm);

/** Compare two communities for equality
 *
 * @param comm1         pointer to the first community to compare
 * @param comm2         pointer to the second community to compare
 * @return 0 if the communities are not equal, non-zero if they are equal
 */
int bgpstream_community_equal(bgpstream_community_t *comm1,
                              bgpstream_community_t *comm2);

/** Compare two communities for equality
 *
 * @param comm1         first community to compare
 * @param comm2         second community to compare
 * @return 0 if the communities are not equal, non-zero if they are equal
 */
int bgpstream_community_equal_value(bgpstream_community_t comm1,
                                    bgpstream_community_t comm2);

/** Write the string representation of the given community set into the given
 *  character buffer.
 *
 * @param buf           pointer to a character buffer at least len bytes long
 * @param len           length of the given character buffer
 * @param set           pointer to the community set to convert to string
 * @return the number of characters written given an infinite len (not including
 * the trailing nul). If this value is greater than or equal to len, then the
 * output was truncated.
 */
int bgpstream_community_set_snprintf(char *buf, size_t len,
                                     bgpstream_community_set_t *set);

/** Create an empty community set structure.
 *
 * @return pointer to the created community set object if successful, NULL
 * otherwise
 */
bgpstream_community_set_t *bgpstream_community_set_create();

/** Clear the given community set structure
 *
 * @param set           pointer to the community set to clear
 */
void bgpstream_community_set_clear(bgpstream_community_set_t *set);

/** Destroy the given community set structure
 *
 * @param path        pointer to the AS path structure to destroy
 */
void bgpstream_community_set_destroy(bgpstream_community_set_t *set);

/** Copy one community set structure into another
 *
 * @param dst           pointer to the community set structure to copy into
 * @param src           pointer to the community set structure to copy from
 * @return 0 if the copy was successful, -1 otherwise
 *
 * @note this function will overwrite any data currently in dst. If there are
 * existing borrowed community pointers into the path they will become garbage.
 */
int bgpstream_community_set_copy(bgpstream_community_set_t *dst,
                                 bgpstream_community_set_t *src);

/** Get the community value at the given index in the set
 *
 * @param set           pointer to the set to get the community from
 * @param i             index of the community value to get
 * @return **borrowed** pointer to the community, NULL if index is out of bounds
 *
 * @note the returned pointer is owned **by the set**. It MUST NOT be destroyed
 * using bgpstream_community_destroy. Also, it is only valid as long as the set
 * is valid.
 */
bgpstream_community_t *
bgpstream_community_set_get(bgpstream_community_set_t *set, int i);

/** Get the number of communities in the set
 *
 * @param set           pointer to the set to get the size of
 * @return the number of communities in the given set
 */
int bgpstream_community_set_size(bgpstream_community_set_t *set);

/** Insert the given community into the community set
 *
 * @param set           pointer to the set to populate
 * @param comm          pointer to the community
 * @return 0 if the set was populated successfully, -1 otherwise
 */
int bgpstream_community_set_insert(bgpstream_community_set_t *set,
                                   bgpstream_community_t *comm);

/** Populate the given community set from the given community array
 *
 * @param set           pointer to the set to populate
 * @param comms         pointer to the community array
 * @param comms_cnt     number of communities in the array
 * @return 0 if the set was populated successfully, -1 otherwise
 */
int bgpstream_community_set_populate_from_array(bgpstream_community_set_t *set,
                                                bgpstream_community_t *comms,
                                                int comms_cnt);

/** Populate the given community set from the given community array (Zero Copy)
 *
 * @param set           pointer to the set to populate
 * @param comms         pointer to the community array
 * @param comms_cnt     number of communities in the array
 * @return 0 if the set was populated successfully, -1 otherwise
 *
 * @note this function **does not** copy the data into the set. The set is
 * only valid as long as the comms array passed to this function is valid.
 */
int bgpstream_community_set_populate_from_array_zc(
  bgpstream_community_set_t *set, bgpstream_community_t *comms, int comms_cnt);

/** Hash the given community set into a 32bit number
 *
 * @param path          pointer to the community set to hash
 * @return 32bit hash of the community set
 */
#if UINT_MAX == 0xffffffffu
unsigned int
#elif ULONG_MAX == 0xffffffffu
unsigned long
#endif
bgpstream_community_set_hash(bgpstream_community_set_t *set);

/** Compare two community sets for equality
 *
 * @param set1          pointer to the first community set to compare
 * @param set2          pointer to the second community set to compare
 * @return 0 if the sets are not equal, non-zero if they are equal
 *
 * @note this is not a true check for set equality. For this function to return
 * true, the ordering within the sets must also be identical.
 */
int bgpstream_community_set_equal(bgpstream_community_set_t *set1,
                                  bgpstream_community_set_t *set2);

/** Check if a community is part of a community set
 *
 * @param set          pointer to the community set to check
 * @param com          pointer to the community set to search
 * @return 1 if the set contains the community, 0 if not
 *
 */
int bgpstream_community_set_exists(bgpstream_community_set_t *set,
                                   bgpstream_community_t *com);

/** Check if a community matches of a community set
 *
 * @param set          pointer to the community set to check
 * @param com          pointer to the community set to search
 * @param mask         it tells whether we check the entire community,
 *                     the asn field or the value field
 * @return 1 if the set matches the community, 0 if not
 *
 */
int bgpstream_community_set_match(bgpstream_community_set_t *set,
                                  bgpstream_community_t *com, uint8_t mask);

/** @} */

#endif /* __BGPSTREAM_UTILS_COMMUNITY_H */
