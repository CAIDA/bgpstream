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

#include "bgpstream_utils_rtr.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(FOUND_RTR)
#include "rtrlib/rtrlib.h"

/* PUBLIC FUNCTIONS */

struct rtr_mgr_config *bgpstream_rtr_start_connection(
    char *host, char *port, uint32_t *polling_period, uint32_t *cache_timeout,
    uint32_t *retry_inv, char *ssh_user, char *ssh_hostkey, char *ssh_privkey)
{

  uint32_t pp = 30;
  if (polling_period == NULL) {
    polling_period = &pp;
  }

  uint32_t ct = 600;
  if (cache_timeout == NULL) {
    cache_timeout = &ct;
  }

  uint32_t rt = 600;
  if (retry_inv == NULL) {
    retry_inv = &rt;
  }

  struct tr_socket *tr = malloc(sizeof(struct tr_socket));
  if (host != NULL && ssh_user != NULL && ssh_hostkey != NULL &&
      ssh_privkey != NULL) {
#if defined(FOUND_SSH)
    int port = port;
    struct tr_ssh_config config = {
        host, port, NULL, ssh_user, ssh_hostkey, ssh_privkey,
    };
    tr_ssh_init(&config, tr);
#endif
    if (tr_open(tr) == TR_ERROR) {
      fprintf(stderr, "ERROR: The SSH-values entered caused an error while "
                      "initialising the transport-socket\n");
      exit(-1);
    }
  }

  else if (host != NULL && port != NULL) {
    struct tr_tcp_config config = {host, port, NULL};
    tr_tcp_init(&config, tr);
  }

  struct rtr_socket *rtr = malloc(sizeof(struct rtr_socket));
  rtr->tr_socket = tr;

  struct rtr_mgr_group groups[1];
  groups[0].sockets = malloc(sizeof(struct rtr_socket *));
  groups[0].sockets_len = 1;
  groups[0].sockets[0] = rtr;
  groups[0].preference = 1;

  struct rtr_mgr_config *conf;
  int ret = rtr_mgr_init(&conf, groups, 1, *polling_period, *cache_timeout,
                         *retry_inv, NULL, NULL, NULL, NULL);

  rtr_mgr_start(conf);

  while (!rtr_mgr_conf_in_sync(conf))
    sleep(1);

  return conf;
}

enum pfxv_state bgpstream_rtr_validate(struct rtr_mgr_config *mgr_cfg,
                                       uint32_t asn, char *prefix,
                                       uint32_t mask_len)
{
  struct lrtr_ip_addr pref;
  lrtr_ip_str_to_addr(prefix, &pref);
  enum pfxv_state result;
  rtr_mgr_validate(mgr_cfg, asn, &pref, mask_len, &result);
  return result;
}

struct reasoned_result
bgpstream_rtr_validate_reason(struct rtr_mgr_config *mgr_cfg, uint32_t asn,
                              char prefix[], uint32_t mask_len)
{
  struct lrtr_ip_addr pref;
  lrtr_ip_str_to_addr(prefix, &pref);
  enum pfxv_state result;
  struct pfx_record *reason = NULL;
  unsigned int reason_len = 0;

  pfx_table_validate_r(mgr_cfg->groups[0].sockets[0]->pfx_table, &reason,
                       &reason_len, asn, &pref, mask_len, &result);

  struct reasoned_result reasoned_res;
  reasoned_res.reason = reason;
  reasoned_res.reason_len = reason_len;
  reasoned_res.result = result;

  return (reasoned_res);
}

void bgpstream_rtr_close_connection(struct rtr_mgr_config *mgr_cfg)
{
  struct tr_socket *tr = mgr_cfg->groups[0].sockets[0]->tr_socket;
  struct rtr_socket *rtr = mgr_cfg->groups[0].sockets[0];
  struct rtr_socket **socket = mgr_cfg->groups[0].sockets;
  rtr_mgr_stop(mgr_cfg);
  rtr_mgr_free(mgr_cfg);
  tr_free(tr);
  free(tr);
  free(rtr);
  free(socket);
}

#endif
