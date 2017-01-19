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

#ifndef __BGPSTREAM_UTILS_ID_SET_H
#define __BGPSTREAM_UTILS_ID_SET_H

/** @file
 *
 * @brief Header file that exposes the public interface of the BGP Stream ID
 * Set.
 *
 * @author Chiara Orsini
 *
 */

/**
 * @name Opaque Data Structures
 *
 * @{ */

/** Opaque structure containing a ID set instance */
typedef struct bgpstream_id_set bgpstream_id_set_t;

/** @} */

/**
 * @name Public API Functions
 *
 * @{ */

/** Create a new ID set instance
 *
 * @return a pointer to the structure, or NULL if an error occurred
 */
bgpstream_id_set_t *bgpstream_id_set_create();

/** Insert a new ID into the given set.
 *
 * @param set           pointer to the id set
 * @param id            id to insert in the set
 * @return 1 if the id was inserted, 0 if it already existed, -1 if an error
 * occurred
 */
int bgpstream_id_set_insert(bgpstream_id_set_t *set, uint32_t id);

/** Check whether an ID exists in the set
 *
 * @param set           pointer to the ID set
 * @param id            the ID to check
 * @return 0 if the ID is not in the set, 1 if it is in the set
 */
int bgpstream_id_set_exists(bgpstream_id_set_t *set, uint32_t id);

/** Get the number of IDs in the given set
 *
 * @param set           pointer to the id set
 * @return the size of the id set
 */
int bgpstream_id_set_size(bgpstream_id_set_t *set);

/** Merge two ID sets
 *
 * @param dst_set      pointer to the set to merge src into
 * @param src_set      pointer to the set to merge into dst
 * @return 0 if the sets were merged succsessfully, -1 otherwise
 */
int bgpstream_id_set_merge(bgpstream_id_set_t *dst_set,
                           bgpstream_id_set_t *src_set);

/** Reset the internal iterator
 *
 * @param set           pointer to the id set
 */
void bgpstream_id_set_rewind(bgpstream_id_set_t *set);

/** Returns a pointer to the next id
 *
 * @param set           pointer to the string set
 * @return a pointer to the next id in the set
 *         (borrowed pointer), NULL if the end of the set
 *         has been reached
 */
uint32_t *bgpstream_id_set_next(bgpstream_id_set_t *set);

/** Destroy the given ID set
 *
 * @param set           pointer to the ID set to destroy
 */
void bgpstream_id_set_destroy(bgpstream_id_set_t *set);

/** Empty the id set.
 *
 * @param set           a pointer to the id set to clear
 */
void bgpstream_id_set_clear(bgpstream_id_set_t *set);

#endif /* __BGPSTREAM_UTILS_ID_SET_H */
