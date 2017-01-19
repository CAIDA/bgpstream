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


#ifndef __BGPSTREAM_UTILS_AS_PATH_H
#define __BGPSTREAM_UTILS_AS_PATH_H

#include <limits.h>
#include "bgpstream_utils_pfx.h"

/** @file
 *
 * @brief Header file that exposes the public interface of BGP Stream AS
 * objects
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

/** The type of an segment
 *
 */
typedef enum {

  /** Invalid Segment Type */
  BGPSTREAM_AS_PATH_SEG_INVALID      = 0,

  /** Simple ASN AS Path Segment */
  BGPSTREAM_AS_PATH_SEG_ASN          = 1,

  /** AS Path Segment Set */
  BGPSTREAM_AS_PATH_SEG_SET          = 2,

  /** AS Path Segment Confederation Set */
  BGPSTREAM_AS_PATH_SEG_CONFED_SET   = 3,

  /** AS Path Segment Confederation Sequence */
  BGPSTREAM_AS_PATH_SEG_CONFED_SEQ   = 4,

  /** @todo etc */

} bgpstream_as_path_seg_type_t;

/** @} */

/**
 * @name Public Opaque Data Structures
 *
 * @{ */

/** Opaque pointer to an AS Path object */
typedef struct bgpstream_as_path bgpstream_as_path_t;

/** @} */

/**
 * @name Public Data Structures
 *
 * @{ */

/** A BGP Stream object for valid prefixes in RPKI validation result */
typedef struct struct_bgpstream_rpki_validation_pfx_t {

  /** prefixes */
  bgpstream_pfx_storage_t pfx;

  /** maximum prefix length */
  uint8_t max_pfx_len;

} bgpstream_rpki_validation_pfx_t;

/** A BGP Stream object for valid ASNs and prefixes in RPKI validation result */
typedef struct struct_bgpstream_rpki_validation_asn_pfx_t {

  /** ASN */
  uint32_t asn;

  /** prefixes and lengths */
  bgpstream_rpki_validation_pfx_t *pfxs;

  /** number of prefixes */
  size_t pfx_used;

  /** maximal number of prefixes */
  size_t pfx_size;

} bgpstream_rpki_validation_asn_pfx_t;

/** A BGP Stream object for the RPKI validation result */
typedef struct struct_bgpstream_rpki_validation_result_t {

  /** ASNs & Prefixes */
  bgpstream_rpki_validation_asn_pfx_t *asn_pfx;

  /** number of ASNs */
  size_t asn_used;

  /** maximal number of ASNs */
  size_t asn_size;

} bgpstream_rpki_validation_result_t;

/** Generic AS Path Segment.
 *
 * This must be cast to the appropriate structure based on the type
 */
typedef struct bgpstream_as_path_seg {

  /** Type of the AS Path Segment (bgpstream_as_path_seg_type_t) */
  uint8_t type;

} bgpstream_as_path_seg_t;

/** Simple ASN AS Path Segment */
typedef struct bgpstream_as_path_seg_asn {

  /** Type of the AS Path Segment: BGPSTREAM_AS_PATH_SEG_ASN */
  uint8_t type;

  /** ASN value for this segment */
  uint32_t asn;

} __attribute__((packed)) bgpstream_as_path_seg_asn_t;

/** AS Path Segment Set */
typedef struct bgpstream_as_path_seg_set {

  /** Type of the AS Path Segment:
   *    BGPSTREAM_AS_PATH_SEG_SET,
   *    BGPSTREAM_AS_PATH_SEG_CONFED_SET,
   *    BGPSTREAM_AS_PATH_SEG_CONFED_SEQ
   */
  uint8_t type;

  /** Number of ASNs in the set */
  uint8_t asn_cnt;

  /** Array of ASNs in the set */
  uint32_t asn[0];

} __attribute__((packed)) bgpstream_as_path_seg_set_t;

/** Path iterator structure */
typedef struct bgpstream_as_path_iter {

  /* current offset into the data buffer */
  uint16_t cur_offset;

} bgpstream_as_path_iter_t;

/** @} */

/**
 * @name Public API Functions
 *
 * @{ */

/* AS PATH SEGMENT FUNCTIONS */

/** Write the string representation of the given AS Path Segment into the given
 *  character buffer.
 *
 * @param buf           pointer to a character buffer at least len bytes long
 * @param len           length of the given character buffer
 * @param seg           pointer to the segment to convert to string
 * @return the number of characters written given an infinite len (not including
 * the trailing nul). If this value is greater than or equal to len, then the
 * output was truncated.
 */
int bgpstream_as_path_seg_snprintf(char *buf, size_t len,
                                   bgpstream_as_path_seg_t *seg);

/** Duplicate the given AS Path Segment
 *
 * @param src           pointer to the AS path segment to duplicate
 * @return a pointer to the created segment if successful, NULL otherwise
 *
 * @note the returned segment must be destroyed using
 * bgpstream_as_path_seg_destroy
 */
bgpstream_as_path_seg_t *bgpstream_as_path_seg_dup(bgpstream_as_path_seg_t *src);

/** Destroy the given AS Path Segment
 *
 * @param seg           pointer to the AS path segment to destroy
 */
void bgpstream_as_path_seg_destroy(bgpstream_as_path_seg_t *seg);

/** Hash the given AS path segment into a 32bit number
 *
 * @param seg        pointer to the AS path segment to hash
 * @return 32bit hash of the AS path segment
 */
#if UINT_MAX == 0xffffffffu
unsigned int
#elif ULONG_MAX == 0xffffffffu
unsigned long
#endif
bgpstream_as_path_seg_hash(bgpstream_as_path_seg_t *seg);


/** Compare two AS path segments for equality
 *
 * @param seg1          pointer to the first AS path segment to compare
 * @param seg2          pointer to the second AS path segment to compare
 * @return 0 if the segments are not equal, non-zero if they are equal
 */
int bgpstream_as_path_seg_equal(bgpstream_as_path_seg_t *seg1,
                                bgpstream_as_path_seg_t *seg2);


/* AS PATH FUNCTIONS */

/** Write the string representation of the given AS path into the given character
 *  buffer.
 *
 * @param buf           pointer to a character buffer at least len bytes long
 * @param len           length of the given character buffer
 * @param as_path       pointer to the bgpstream AS path to convert to string
 * @return the number of characters written given an infinite len (not including
 * the trailing nul). If this value is greater than or equal to len, then the
 * output was truncated.
 */
int bgpstream_as_path_snprintf(char *buf, size_t len,
                               bgpstream_as_path_t *as_path);

/** Create an empty AS path structure.
 *
 * @return pointer to the created AS path object if successful, NULL otherwise
 */
bgpstream_as_path_t *bgpstream_as_path_create();

/** Clear the given AS path structure
 *
 * @param               pointer to the AS path to clear
 */
void bgpstream_as_path_clear(bgpstream_as_path_t *path);

/** Destroy the given AS path structure
 *
 * @param path        pointer to the AS path structure to destroy
 */
void bgpstream_as_path_destroy(bgpstream_as_path_t *path);

/** Copy one AS Path structure into another
 *
 * @param dst           pointer to the AS path structure to copy into
 * @param src           pointer to the AS path structure to copy from
 * @return 0 if the copy was successful, -1 otherwise
 *
 * @note this function will overwrite any data currently in dst. If there are
 * existing borrowed segment pointers into the path they will become garbage.
 */
int bgpstream_as_path_copy(bgpstream_as_path_t *dst, bgpstream_as_path_t *src);

/** Get the origin AS segment from the given path
 *
 * @param path          pointer to the AS path to extract the origin AS for
 * @return **borrowed** pointer to the origin AS segment
 *
 * @note the returned pointer is owned **by the path**. It MUST NOT be destroyed
 * using bgpstream_as_path_seg_destroy. Also, it is only valid as long as the
 * path is valid.
 */
bgpstream_as_path_seg_t *
bgpstream_as_path_get_origin_seg(bgpstream_as_path_t *path);

/** Get the origin ASN value from the given path if the origin segment is a
 * simple ASN value (i.e. not a set or confederation).
 *
 * @param path          pointer to the AS path to extract the origin AS for
 * @param asn           pointer to a 32-bit field in which the ASN value will be
 *                      stored
 * @return 0 if the asn value is valid, -1 otherwise
 *
 * The return value of this function **must** be checked. If the return value is
 * not 0, then the value of the `asn` parameter is undefined.
 */
int bgpstream_as_path_get_origin_val(bgpstream_as_path_t *path, uint32_t *asn);

/** Reset the segment iterator for the given path
 *
 * @param iter          pointer to the AS path iterator to reset
 */
void bgpstream_as_path_iter_reset(bgpstream_as_path_iter_t *iter);

/** Get the next segment from the given path
 *
 * @param path          pointer to the AS path to get the segment from
 * @param iter          pointer to an AS path iterator
 * @return **borrowed** pointer to the next segment, NULL if the path has no
 *         more segments
 *
 * @note the returned pointer is owned **by the path**. It MUST NOT be destroyed
 * using bgpstream_as_path_seg_destroy. Also, it is only valid as long as the
 * path is valid.
 */
bgpstream_as_path_seg_t *
bgpstream_as_path_get_next_seg(bgpstream_as_path_t *path,
                               bgpstream_as_path_iter_t *iter);

/** Get the number of segments in the AS Path
 *
 * @param path          pointer to the path to get the length of
 * @return the number of segments in the given path
 *
 * @note that this returns the number of BGPStream segments. This may not be the
 * same as the number of segments in the original MRT message as BGPStream
 * expands AS_SEQUENCE segments into a series of individual ASN segments.
 */
int bgpstream_as_path_get_len(bgpstream_as_path_t *path);

/** Provides access to the internal byte array that stores the path segments
 *
 * @param path          pointer to the path
 * @param[out] data     set to point to the path's internal byte array
 * @return the number of bytes in the data array
 *
 * This function is to be used when serializing a path. The returned data array
 * belongs to the path and must not be modified or freed.
 */
uint16_t bgpstream_as_path_get_data(bgpstream_as_path_t *path, uint8_t **data);

/** Populate the given AS Path from the given byte array
 *
 * @param path          pointer to the path to populate
 * @param data          pointer to the data arrayg
 * @param data_len      number of bytes in the data array
 * @return 0 if the path was populated successfully, -1 otherwise
 */
int bgpstream_as_path_populate_from_data(bgpstream_as_path_t *path,
                                         uint8_t *data, uint16_t data_len);

/** Populate the given AS Path from the given byte array (Zero Copy)
 *
 * @param path          pointer to the path to populate
 * @param data          pointer to the data array
 * @param data_len      number of bytes in the data array
 * @return 0 if the path was populated successfully, -1 otherwise
 *
 * @note this function **does not** copy the data into the path. The path is
 * only valid as long as the data array passed to this function is valid.
 */
int bgpstream_as_path_populate_from_data_zc(bgpstream_as_path_t *path,
                                            uint8_t *data, uint16_t data_len);

/** Hash the given AS path into a 32bit number
 *
 * @param path          pointer to the AS path to hash
 * @return 32bit hash of the AS path
 */
#if UINT_MAX == 0xffffffffu
unsigned int
#elif ULONG_MAX == 0xffffffffu
unsigned long
#endif
bgpstream_as_path_hash(bgpstream_as_path_t *path);

/** Compare two AS path for equality
 *
 * @param path1          pointer to the first AS path to compare
 * @param path2          pointer to the second AS path to compare
 * @return 0 if the paths are not equal, non-zero if they are equal
 *
 * @note for this function to return true, the paths must be identical. If ASN
 * ordering within sets is not consistent, this will return false.
 */
int bgpstream_as_path_equal(bgpstream_as_path_t *path1,
                            bgpstream_as_path_t *path2);

/** Initialize the RPKI validation result
 *
 * @param asn_arr       pointer to bgpstream_rpki_validation_result
 * @param size          beginning size of the dynamic array
 */
void bgpstream_rpki_validation_result_init(
    bgpstream_rpki_validation_result_t *asn_arr, size_t size);

/** Insert a new ASN to the RPKI validation result
 *
 * @param asn_arr       pointer to bgpstream_rpki_validation_result
 * @param asn_seg       the ASN which will be added
 */
void bgpstream_rpki_validation_result_insert_asn(
    bgpstream_rpki_validation_result_t *asn_arr, uint32_t asn_seg);

/** Insert a new prefix to an ASN in the RPKI validation result
 *
 * @param asn_arr       pointer to bgpstream_rpki_validation_result
 * @param asn_seg       the ASN to which the prefix will be added
 * @param pfx           pointer to the prefix
 * @param max_pfx_len   maximum prefix length
 */
void bgpstream_rpki_validation_result_insert_pfx(
    bgpstream_rpki_validation_result_t *asn_arr, uint32_t asn_seg,
    bgpstream_pfx_t *pfx, uint8_t max_pfx_len);

/** Free the memory used by the RPKI validation result
 *
 * @param asn_arr       pointer to a bgpstream_rpki_validation_result
 */
void bgpstream_rpki_validation_result_free(
    bgpstream_rpki_validation_result_t *asn_arr);


/** @} */


#endif /* __BGPSTREAM_UTILS_AS_PATH_H */

