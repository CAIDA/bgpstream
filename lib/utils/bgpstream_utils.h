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

#ifndef __BGPSTREAM_UTILS_H
#define __BGPSTREAM_UTILS_H

/** @file
 *
 * @brief Header file that exposes all BGPStream utility types and functions.
 *
 * @author Chiara Orsini
 *
 */

/**
 * @name Public Constants
 *
 * @{ */

/** The maximum number of characters allowed in a collector name */
#define BGPSTREAM_UTILS_STR_NAME_LEN 128

/** @} */

/* Include all utility headers */
#include "bgpstream_utils_addr.h"          /*< IP Address utilities */
#include "bgpstream_utils_addr_set.h"      /*< IP Address Set utilities */
#include "bgpstream_utils_as_path.h"       /*< AS Path utilities */
#include "bgpstream_utils_as_path_store.h" /*< AS Path Store utilities */
#include "bgpstream_utils_community.h"     /*< Community utilities */
#include "bgpstream_utils_id_set.h"        /*< ID Set utilities */
#include "bgpstream_utils_ip_counter.h"    /*< IP Overlap Counter */
#include "bgpstream_utils_patricia.h"      /*< Patricia Tree utilities */
#include "bgpstream_utils_peer_sig_map.h"  /*< Peer Signature utilities */
#include "bgpstream_utils_pfx.h"           /*< Prefix utilities */
#include "bgpstream_utils_pfx_set.h"       /*< Prefix Set utilities */
#include "bgpstream_utils_str_set.h"       /*< String Set utilities */
#include "bgpstream_utils_time.h"          /*< Time management utilities */

#endif /* __BGPSTREAM_UTILS_H */
