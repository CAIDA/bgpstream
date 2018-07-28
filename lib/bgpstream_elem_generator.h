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

#ifndef __BGPSTREAM_ELEM_GENERATOR_H
#define __BGPSTREAM_ELEM_GENERATOR_H

#include "bgpstream_record.h"

/** @file
 *
 * @brief Header file that exposes the protected interface of the bgpstream elem
 * generator.
 *
 * @author Alistair King
 *
 */

/**
 * @name Public Opaque Data Structures
 *
 * @{ */

typedef struct bgpstream_elem_generator bgpstream_elem_generator_t;

/** @} */

/**
 * @name Protected API Functions
 *
 * @{ */

/** Create a new Elem generator
 *
 * @return pointer to a new Elem generator instance
 *
 * Currently the elem generator is simply a reusable container for elem records
 * (populated by a call to bgpstream_elem_populate_generator), but it is
 * possible that at some point in the future it may actually generate elem
 * records on the fly.
 */
bgpstream_elem_generator_t *bgpstream_elem_generator_create();

/** Destroy the given generator
 *
 * @param generator     pointer to the generator to destroy
 */
void bgpstream_elem_generator_destroy(bgpstream_elem_generator_t *generator);

/** Clear the generator ready for re-use
 *
 * @param generator     pointer to the generator to clear
 */
void bgpstream_elem_generator_clear(bgpstream_elem_generator_t *generator);

/** Check if the generator has been populated
 *
 * @param generator     pointer to the generator
 * @return 1 if the generator is ready for use, 0 otherwise
 */
int bgpstream_elem_generator_is_populated(
  bgpstream_elem_generator_t *generator);

/** Populate a generator with elems from the given record
 *
 * @param generator     pointer to the generator to populate
 * @param record        pointer to a BGP Stream Record instance
 * @return 0 if the generator was populated successfully, -1 otherwise
 *
 * @note This function may defer processing of the record until each call to
 * bgpstream_elem_generator_next_elem
 */
int bgpstream_elem_generator_populate(bgpstream_elem_generator_t *generator,
                                      struct struct_bgpstream_record_t *record);

/** Get the next elem from the generator
 *
 * @param generator     pointer to the generator to retrieve an elem from
 * @return borrowed pointer to the next elem
 *
 * @note the memory for the returned elem belongs to the generator.
 */
bgpstream_elem_t *
bgpstream_elem_generator_get_next_elem(bgpstream_elem_generator_t *generator);

/** @} */

#endif /* __BGPSTREAM_ELEM_GENERATOR_H */
