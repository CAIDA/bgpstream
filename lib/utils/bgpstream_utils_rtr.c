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
#include "rtrlib/rtr_mgr.h"
#include "rtrlib/transport/ssh/ssh_transport.h"
#include "rtrlib/rtr_mgr.h"
#include "bgpstream.h"

#include "bgpstream_utils_rtr.h"

void main()
{
  char ip[] = "10.10.0.0";
  struct conf_tr cfg_tr = bgpstream_rtr_start_connection("rpki-validator.realmv6.org", "8282", NULL, NULL ,NULL);  
  char * result = bgpstream_rtr_validate(cfg_tr, 12345, ip, 24);
  bgpstream_rtr_close_connection(cfg_tr);

  printf("\nState for IP-Address: %s is %s \n\n",ip, result);
}

/* PUBLIC FUNCTIONS */

struct conf_tr bgpstream_rtr_start_connection(char * host, char * port, char * ssh_user, char * ssh_hostkey, char * ssh_privkey)
{
  struct tr_socket tr;
  if (host != NULL && port != NULL && ssh_user != NULL && ssh_hostkey != NULL && ssh_privkey != NULL)
  {
    int port = atoi(port);
    struct tr_ssh_config config = {
        host,
        port,
        NULL,
        ssh_user,
        ssh_hostkey,
        ssh_privkey,
    };
    tr_ssh_init(&config, &tr);
  }

  else if (host != NULL && port != NULL)
  {
    struct tr_tcp_config config = {
      host,
      port,
      NULL
    };
    tr_tcp_init(&config, &tr);
  }

  struct rtr_socket rtr;
  rtr.tr_socket = &tr;

  struct rtr_mgr_group groups[1];
  groups[0].sockets = malloc(sizeof(struct rtr_socket*));
  groups[0].sockets_len = 1;
  groups[0].sockets[0] = &rtr;
  groups[0].preference = 1;

  struct conf_tr cfmgr_tr;
  struct rtr_mgr_config *conf = rtr_mgr_init(groups, 1, 240, 520, NULL, NULL, NULL, NULL);
  rtr_mgr_start(conf);

  while(!rtr_mgr_conf_in_sync(conf))
      sleep(1);

  cfmgr_tr.conf = conf;
  cfmgr_tr.tr = tr;

  return cfmgr_tr;
}

char *
bgpstream_rtr_validate (struct conf_tr cfg_tr, uint32_t asn,
                        char * prefix, uint32_t mask_len)
{
  struct ip_addr *pref;
  lrtr_ip_str_to_addr(prefix, &pref);
  enum pfxv_state result;
  rtr_mgr_validate(cfg_tr.conf, asn, &pref, mask_len, &result);

  char * validity_code;
  if (result == BGP_PFXV_STATE_VALID) 
  {
      validity_code = "valid";
  } else if (result == BGP_PFXV_STATE_NOT_FOUND) 
  {
      validity_code = "State not found";
  } else if (result == BGP_PFXV_STATE_INVALID) 
  {
      validity_code = "State invalid";
  }

  return validity_code;
}

void bgpstream_rtr_close_connection(struct conf_tr cfg_tr)
{
  tr_close(&cfg_tr.tr);
  tr_free(&cfg_tr.tr);
}

