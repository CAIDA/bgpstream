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

#ifndef __BGPSTREAM_UTILS_COMMUNITY_INT_H
#define __BGPSTREAM_UTILS_COMMUNITY_INT_H

#include "bgpstream_utils_community.h"

#include "bgpdump_lib.h"

/** @file
 *
 * @brief Header file that exposes the private interface of BGP Stream AS
 * objects
 *
 * @author Chiara Orsini, Alistair King
 *
 */

/**
 * @name Private Constants
 *
 * @{ */

/* @} */

/**
 * @name Private Enums
 *
 * @{ */

/** @} */

/**
 * @name Private Opaque Data Structures
 *
 * @{ */

/** @} */

/**
 * @name Private Data Structures
 *
 * @{ */

/** @} */

/**
 * @name Private API Functions
 *
 * @{ */

/** Populate a community set structure based on a BGP Dump Community Attribute
 *
 * @param set           pointer to the community set to populate
 * @param bd_comms      pointer to a BGP Dump community attribute structure
 * @return 0 if the set was populated successfully, -1 otherwise
 */
int bgpstream_community_set_populate(bgpstream_community_set_t *set,
                                     struct community *bd_comms);

/** @} */

#endif /* __BGPSTREAM_UTILS_COMMUNITY_INT_H */
