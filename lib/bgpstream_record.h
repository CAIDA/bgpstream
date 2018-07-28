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

#ifndef __BGPSTREAM_RECORD_H
#define __BGPSTREAM_RECORD_H

#include "bgpstream_elem.h"
#include "bgpstream_utils.h"

/** @file
 *
 * @brief Header file that exposes the public interface of a bgpstream record.
 *
 * @author Chiara Orsini
 *
 */

/**
 * @name Public Opaque Data Structures
 *
 * @{ */

/** An opaque pointer to a BGPDUMP_ENTRY field.
 *
 * @note this is an *internal* data structure, it is *not* guaranteed that it
 * will continue to conform to the public BGPDUMP_ENTRY structure. This is NOT
 * for use in user code
 */
typedef struct struct_BGPDUMP_ENTRY bgpstream_record_mrt_data_t;

/** @} */

/**
 * @name Public Constants
 *
 * @{ */

/** @} */

/**
 * @name Public Enums
 *
 * @{ */

/** The type of the record */
typedef enum {

  /** The record contains data for a BGP Update message */
  BGPSTREAM_UPDATE = 0,

  /** The record contains data for a BGP RIB message */
  BGPSTREAM_RIB = 1,

} bgpstream_record_dump_type_t;

/** The position of this record in the dump */
typedef enum {

  /** This is the first record of the dump */
  BGPSTREAM_DUMP_START = 0,

  /** This is a record in the middle of the dump. i.e. not the first or the last
      record of the dump */
  BGPSTREAM_DUMP_MIDDLE = 1,

  /** This is the last record of the dump */
  BGPSTREAM_DUMP_END = 2,

} bgpstream_dump_position_t;

/** Status of the record */
typedef enum {

  /** The record is valid and may be used */
  BGPSTREAM_RECORD_STATUS_VALID_RECORD = 0,

  /** Source is not empty, but no valid record was found */
  BGPSTREAM_RECORD_STATUS_FILTERED_SOURCE = 1,

  /** Source has no entries */
  BGPSTREAM_RECORD_STATUS_EMPTY_SOURCE = 2,

  /* Error in opening dump */
  BGPSTREAM_RECORD_STATUS_CORRUPTED_SOURCE = 3,

  /* Dump corrupted at some point */
  BGPSTREAM_RECORD_STATUS_CORRUPTED_RECORD = 4,

} bgpstream_record_status_t;

/** @} */

/**
 * @name Public Data Structures
 *
 * @{ */

/** Record attributes */
typedef struct struct_bgpstream_record_attributes_t {

  /** Project name */
  char dump_project[BGPSTREAM_UTILS_STR_NAME_LEN];

  /** Collector name */
  char dump_collector[BGPSTREAM_UTILS_STR_NAME_LEN];

  /** Dump type */
  bgpstream_record_dump_type_t dump_type;

  /** Time that the BGP data was "aggregated". E.g. the start time of an MRT
      dump file */
  long dump_time;

  /** Time from the MRT record. I.e. the time *this* record was dumped */
  long record_time;

} bgpstream_record_attributes_t;

/** Record structure */
typedef struct struct_bgpstream_record_t {

  /** INTERNAL: pointer to the originating bgpstream instance */
  struct struct_bgpstream_t *bs;

  /** INTERNAL: buffer containing the underlying MRT data for the record */
  bgpstream_record_mrt_data_t *bd_entry;

  /** INTERNAL: state used to generate elems for get_next_elem */
  struct bgpstream_elem_generator *elem_generator;

  /** Collection of attributes pertaining to this record */
  bgpstream_record_attributes_t attributes;

  /** Status of this record */
  bgpstream_record_status_t status;

  /** Position of this record in the dump */
  bgpstream_dump_position_t dump_pos;

} bgpstream_record_t;

/** @} */

/**
 * @name Public API Functions
 *
 * @{ */

/** Create a new BGP Stream Record instance for passing to
 * bgpstream_get_next_record.
 *
 * @return a pointer to a Record instance if successful, NULL otherwise
 *
 * A Record may be reused for successive calls to bgpstream_get_next_record if
 * records are processed independently of each other
 */
bgpstream_record_t *bgpstream_record_create();

/** Destroy the given BGP Stream Record instance
 *
 * @param record        pointer to a BGP Stream Record instance to destroy
 */
void bgpstream_record_destroy(bgpstream_record_t *record);

/** Clear the given BGP Stream Record instance
 *
 * @param record        pointer to a BGP Stream Record instance to clear
 *
 * @note the record passed to bgpstream_get_next_record is automatically
 * cleaned.
 */
void bgpstream_record_clear(bgpstream_record_t *record);

/** Retrieve the next elem from the record
 *
 * @param record        pointer to the BGP Stream Record to retrieve the elem
 *                      from
 * @return borrowed pointer to the next Elem in the record, NULL if there are no
 * more Elems
 *
 * The returned pointer is guaranteed to be valid until the record is re-used in
 * a subsequent call to bgpstream_get_next_record, or is destroyed with
 * bgpstream_record_destroy
 */
bgpstream_elem_t *bgpstream_record_get_next_elem(bgpstream_record_t *record);

/** Dump the given record to stdout in bgpdump format
 *
 * @param record        pointer to a BGP Stream Record instance to dump
 *
 * See https://bitbucket.org/ripencc/bgpdump for more information about bgpdump
 */
void bgpstream_record_print_mrt_data(bgpstream_record_t *const bs_record);

/** Write the string representation of the record dump type into the provided
 *  buffer
 *
 * @param buf           pointer to a char array
 * @param len           length of the char array
 * @param dump_type     BGP Stream Record dump type to convert to string
 * @return the number of characters that would have been written if len was
 * unlimited
 */
int bgpstream_record_dump_type_snprintf(char *buf, size_t len,
                                        bgpstream_record_dump_type_t dump_type);

/** Write the string representation of the record dump position into the
 * provided
 *  buffer
 *
 * @param buf           pointer to a char array
 * @param len           length of the char array
 * @param dump_pos      BGP Stream Record dump position to convert to string
 * @return the number of characters that would have been written if len was
 * unlimited
 */
int bgpstream_record_dump_pos_snprintf(char *buf, size_t len,
                                       bgpstream_dump_position_t dump_pos);

/** Write the string representation of the record status into the provided
 *  buffer
 *
 * @param buf           pointer to a char array
 * @param len           length of the char array
 * @param status        BGP Stream Record status to convert to string
 * @return the number of characters that would have been written if len was
 * unlimited
 */
int bgpstream_record_status_snprintf(char *buf, size_t len,
                                     bgpstream_record_status_t status);

/** Write the string representation of the record/elem into the provided buffer
 *
 * @param buf           pointer to a char array
 * @param len           length of the char array
 * @param elem          pointer to a BGP Stream Record to convert to string
 * @param elem          pointer to a BGP Stream Elem to convert to string
 * @return pointer to the start of the buffer if successful, NULL otherwise
 */
char *bgpstream_record_elem_snprintf(char *buf, size_t len,
                                     const bgpstream_record_t *bs_record,
                                     const bgpstream_elem_t *elem);

/** @} */

#endif /* __BGPSTREAM_RECORD_H */
