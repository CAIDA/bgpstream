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

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>
#include "rtrlib/rtrlib.h"
#include "rtrlib/transport/ssh/ssh_transport.h"
#include "rtrlib/rtr_mgr.h"
#include "bgpstream.h"

#ifndef __BGPSTREAM_UTILS_RTR_H
#define __BGPSTREAM_UTILS_RTR_H

/** @file
 *
 * @brief Header file that exposes the public interface of the RTR-Validation for a BGP-Records
 * Set.
 *
 * @author Chiara Orsini
 *
 */

/** @} */

struct conf_tr
{
  struct rtr_mgr_config *conf;
  struct tr_socket tr;

};

/**
 * @name Public API Functions
 *
 * @{ */

/** Start a connection to a desired RTR-Server over SSH or TCP
 *
 * @param host         string of the host-address
 * @param port         string of the port-number
 * @param ssh_user     if ssh should be used, the username to be used
 * @param ssh_hostkey     if ssh should be used, the hostkey to be used
 * @param ssh_privkey     if ssh should be used, the private key to be used
 * @return a struct consisting of the configuration and the address of the transport-socket
 */
struct conf_tr bgpstream_rtr_start_connection(char * host, char * port, char * ssh_user, char * ssh_hostkey, char * ssh_privkey);

/**
 * @brief Validates the origin of a BGP-Route.
 * @param conf_tr Configuration and socket
 * @param asn Autonomous system number of the Origin-AS of the prefix.
 * @param prefix Announced network prefix
 * @param mask_len Length of the network mask of the announced prefix
 * @param[out] result Outcome of the validation.
 * @return the validation of the ip: BGP_PFXV_STATE_VALID, BGP_PFXV_STATE_NOT_FOUND or BGP_PFXV_STATE_INVALID.
 */
char *bgpstream_rtr_validate(struct conf_tr, uint32_t asn, char * prefix, uint32_t mask_len);

/** Stop a connection to a desired RTR-Server over SSH or TCP
 *
 * @param conf_tr Configuration and socket
 */
void bgpstream_rtr_close_connection(struct conf_tr cfg_tr);


/** @} */

#endif /*__BGPSTREAM_UTILS_RTR_H*/
