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
#include <inttypes.h>
#include "rtrlib/rtrlib.h"
#include "bgpstream_utils_rtr.h"

int main()
{
  char ip[] = "2001:678:3::";
  uint32_t asn = 42;
  uint32_t mask_len = 48;

  /* TCP Example - without Reason */
  struct rtr_mgr_config* cfg_tr = bgpstream_rtr_start_connection("rpki-validator.realmv6.org", NULL, NULL, NULL, NULL, NULL ,NULL);  
  enum pfxv_state result = bgpstream_rtr_validate(cfg_tr, asn, ip, mask_len);
  bgpstream_rtr_close_connection(cfg_tr);
  printf("\nTCP - State for IP-Address: %s is %s \n\n",ip, pfxv2str(result));

  /* TCP Example - Reason*/
  cfg_tr = bgpstream_rtr_start_connection("rpki-validator.realmv6.org", NULL, NULL, NULL, NULL, NULL ,NULL);  
  struct reasoned_result res_reasoned = bgpstream_rtr_validate_reason(cfg_tr, asn, ip, mask_len);
  bgpstream_rtr_close_connection(cfg_tr);
  printf("\nTCP - State for IP-Address: %s is %s \n",ip, pfxv2str(res_reasoned.result) );
  if(res_reasoned.reason){
		struct pfx_record *reason = res_reasoned.reason;
		char ip_prefix[INET6_ADDRSTRLEN+1];
		lrtr_ip_addr_to_str(&(reason->prefix), ip_prefix, sizeof(ip_prefix));
		int len_of_prefix = sizeof(ip_prefix);
  	printf(
			"TCP - Reason for IP-Address %s is: ASN(%"PRIu32") Prefix(%.*s) minimal length(%"PRIu8") maximal length(%"PRIu8") \n\n",
			ip, 
			reason->asn, 
			len_of_prefix,
			ip_prefix,
			reason->min_len,
			reason->max_len);
	}
	else{
		printf("Since %s is not found there is no reason %p\n\n", ip, res_reasoned.reason);
	} 

  /*SSH Example - without Reason*/
  /*cfg_tr = bgpstream_rtr_start_connection("rpki-validator.realmv6.org", "22", NULL, NULL, "rtr-ssh", "~/.ssh/id_rsa" ,"~/.ssh/known_hosts");  
  result = bgpstream_rtr_validate(cfg_tr, asn, ip, mask_len);
  bgpstream_rtr_close_connection(cfg_tr);
  printf("\nSSH - State for IP-Address: %s is %s \n\n",ip, pfxv2str(result));*/

	return 0;
}
