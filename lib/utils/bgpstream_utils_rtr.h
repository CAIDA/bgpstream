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

#ifndef __BGPSTREAM_UTILS_RTR_H
#define __BGPSTREAM_UTILS_RTR_H

#include "config.h"

#if defined(FOUND_RTR)
#include "rtrlib/rtrlib.h"

/** @file
 *
 * @brief Header file that exposes the public interface of the RTR-Validation
 * for a BGP-Record Set.
 *
 * @author Samir Al-Sheikh, Fabrice J. Ryba
 *
 */

/** @} */

struct reasoned_result {
  struct pfx_record *reason;
  enum pfxv_state result;
  unsigned int reason_len;
};

/**
 * @name Public API Functions
 *
 * @{ */

/** Start a connection to a desired RTR-Server over SSH or TCP
 *
 * @param host         		string of the host-address
 * @param port         		if specific port should be used, string of the
 * port-number
 * @param polling_period 	if specific polling period should be used,
 * uint32_t number (seconds)
 * @param cache_timeout		if specific cache_timeout should be used,
 * uint32_t nuber (seconds)
 * @param ssh_user    		if ssh should be used, the username to be used
 * @param ssh_hostkey     if ssh should be used, the hostkey to be used
 * @param ssh_privkey     if ssh should be used, the private key to be used
 * @return a struct consisting of the configuration and the address of the
 * transport-socket
 */
struct rtr_mgr_config *bgpstream_rtr_start_connection(
    char *host, char *port, uint32_t *polling_period, uint32_t *cache_timeout,
    uint32_t *retry_inv, char *ssh_user, char *ssh_hostkey, char *ssh_privkey);

/**
 * @brief Validates the origin of a BGP-Route.
 * @param mgr_cfg Manager-Configuration
 * @param asn Autonomous system number of the Origin-AS of the prefix
 * @param prefix Announced network prefix
 * @param mask_len Length of the network mask of the announced prefix
 * @param[out] result Outcome of the validation
 * @return the validation of the ip: BGP_PFXV_STATE_VALID,
 * BGP_PFXV_STATE_NOT_FOUND or BGP_PFXV_STATE_INVALID.
 */
enum pfxv_state bgpstream_rtr_validate(struct rtr_mgr_config *mgr_cfg,
                                       uint32_t asn, char *prefix,
                                       uint32_t mask_len);

/**
 * @brief Validates the origin of a BGP-Route and returns the reason for the
 * state.
 * @param mgr_cfg Manager-Configuration
 * @param asn Autonomous system number of the Origin-AS of the prefix
 * @param prefix Announced network prefix
 * @param mask_len Length of the network mask of the announced prefix
 * @param[out] result Outcome of the validation and the reason for the state
 * @return the validation of the ip: BGP_PFXV_STATE_VALID,
 * BGP_PFXV_STATE_NOT_FOUND or BGP_PFXV_STATE_INVALID.
 */
struct reasoned_result
bgpstream_rtr_validate_reason(struct rtr_mgr_config *mgr_cfg, uint32_t asn,
                              char prefix[], uint32_t mask_len);

/** Stop a connection to a desired RTR-Server over SSH or TCP
 *
 * @param mgr_cfg Configuration and socket
 */
void bgpstream_rtr_close_connection(struct rtr_mgr_config *mgr_cfg);

/** @} */

#endif /*__RTR*/
#endif /*__BGPSTREAM_UTILS_RTR_H*/
