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
#include "config.h"
#include "bgpstream.h"

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

#define BUFFER_MAX_LEN 1024
#define IPV4_TEST_ADDR "192.0.43.8"
#define IPV4_TEST_PFX "192.0.43.0/24"
#define IPV6_TEST_ADDR "2001:500:88:200::8"
#define IPV6_TEST_PFX "2001:500:88::/48"

int test_addresses()
{
  int ret = 0;
  char buffer[BUFFER_MAX_LEN];
  char ip[BUFFER_MAX_LEN];
  bgpstream_addr_storage_t a;
  bgpstream_addr_storage_t *aptr;

  /* IPv4 */
  strcpy(ip, IPV4_TEST_ADDR);
  aptr = &a;
  aptr = bgpstream_str2addr(IPV4_TEST_ADDR, aptr);
  if(aptr == NULL)
    {
      ret -=1;
    }
  else
    {
      bgpstream_addr_ntop(buffer, BUFFER_MAX_LEN, aptr);
      fprintf(stderr, "%s\n", buffer);
      if(strcmp(buffer,ip) != 0)
        {
          ret -= 1;
        }
    }

  /* IPv6 */
  strcpy(ip, IPV6_TEST_ADDR);
  aptr = &a;
  aptr = bgpstream_str2addr(IPV6_TEST_ADDR, aptr);
  if(aptr == NULL)
    {
      ret -=1;
    }
  else
    {
      bgpstream_addr_ntop(buffer, BUFFER_MAX_LEN, aptr);
      fprintf(stderr, "%s\n", buffer);
      if(strcmp(buffer,ip) != 0)
        {
          ret -= 1;
        }
    }
  
  return ret;
}


int test_prefixes()
{
  int ret = 0;
  char buffer[BUFFER_MAX_LEN];
  char pfx[BUFFER_MAX_LEN];
  bgpstream_pfx_storage_t p;
  bgpstream_pfx_storage_t *pptr;

  /* IPv4 */
  strcpy(pfx, IPV4_TEST_PFX);
  pptr = &p;
  pptr = bgpstream_str2pfx(IPV4_TEST_PFX, pptr);
  if(pptr == NULL)
    {
      ret -=1;
    }
  else
    {
      bgpstream_pfx_snprintf(buffer, BUFFER_MAX_LEN, (bgpstream_pfx_t *)pptr);
      fprintf(stderr, "%s\n", buffer);
      if(strcmp(buffer,pfx) != 0)
        {
          ret -= 1;
        }
    }

  /* IPv6 */
  strcpy(pfx, IPV6_TEST_PFX);
  pptr = &p;
  pptr = bgpstream_str2pfx(IPV6_TEST_PFX, pptr);
  if(pptr == NULL)
    {
      ret -=1;
    }
  else
    {
      bgpstream_pfx_snprintf(buffer, BUFFER_MAX_LEN, (bgpstream_pfx_t *)pptr);
      fprintf(stderr, "%s\n", buffer);
      if(strcmp(buffer,pfx) != 0)
        {
          ret -= 1;
        }
    }
  
  return ret;
}


int main()
{
  int res = 0;
  
  /* Address utilities */
  res += test_addresses();
  res += test_prefixes();
  
  /* res is going to be zero if everything worked fine, */
  /* negative if something failed */
  return res;
}
