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

#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include "khash.h"

#include "bgpstream_utils_addr.h"

#if UINT_MAX == 0xffffffffu
unsigned int
#elif ULONG_MAX == 0xffffffffu
unsigned long
#endif
bgpstream_ipv4_addr_hash(bgpstream_ipv4_addr_t *addr)
{
  return __ac_Wang_hash(addr->ipv4.s_addr);
}


#if ULONG_MAX == ULLONG_MAX
unsigned long
#else
unsigned long long
#endif
bgpstream_ipv6_addr_hash(bgpstream_ipv6_addr_t *addr)
{
  unsigned char *s6 =  &(addr->ipv6.s6_addr[0]);
  return __ac_Wang_hash(*((khint64_t *) s6));
}


#if ULONG_MAX == ULLONG_MAX
unsigned long
#else
unsigned long long
#endif
bgpstream_addr_storage_hash(bgpstream_addr_storage_t *addr)
{
  switch(addr->version)
    {
    case BGPSTREAM_ADDR_VERSION_IPV4:
      return bgpstream_ipv4_addr_hash((bgpstream_ipv4_addr_t*)addr);
      break;

    case BGPSTREAM_ADDR_VERSION_IPV6:
      return bgpstream_ipv6_addr_hash((bgpstream_ipv6_addr_t*)addr);
      break;

    default:
      return 0;
      break;
    }
}


int bgpstream_addr_storage_equal(bgpstream_addr_storage_t *addr1,
                                 bgpstream_addr_storage_t *addr2)
{
  if(addr1->version == BGPSTREAM_ADDR_VERSION_IPV4 &&
     addr2->version == BGPSTREAM_ADDR_VERSION_IPV4)
    {
      return bgpstream_ipv4_addr_equal((bgpstream_ipv4_addr_t *)addr1,
                                       (bgpstream_ipv4_addr_t *)addr2);
    }
  if(addr1->version == BGPSTREAM_ADDR_VERSION_IPV6 &&
     addr2->version == BGPSTREAM_ADDR_VERSION_IPV6)
    {
      return bgpstream_ipv6_addr_equal((bgpstream_ipv6_addr_t *)addr1,
                                       (bgpstream_ipv6_addr_t *)addr2);
    }
  return 0;
}


int bgpstream_ipv4_addr_equal(bgpstream_ipv4_addr_t *addr1,
                              bgpstream_ipv4_addr_t *addr2)
{
  return addr1->ipv4.s_addr == addr2->ipv4.s_addr;
}


int bgpstream_ipv6_addr_equal(bgpstream_ipv6_addr_t *addr1,
                              bgpstream_ipv6_addr_t *addr2)
{
  return ((memcmp(&(addr1->ipv6.s6_addr[0]),
                  &(addr2->ipv6.s6_addr[0]),
                  sizeof(uint64_t)) == 0) &&
          (memcmp(&(addr1->ipv6.s6_addr[8]),
                  &(addr2->ipv6.s6_addr[8]),
                  sizeof(uint64_t)) == 0));
}

uint8_t
bgpstream_ipv2idx(bgpstream_addr_version_t v)
{
  switch(v)
    {
    case BGPSTREAM_ADDR_VERSION_IPV4:
      return 0;
    case BGPSTREAM_ADDR_VERSION_IPV6:
      return 1;
    default:
      assert(0);
    }
  return 255;
}

bgpstream_addr_version_t
bgpstream_idx2ipv(uint8_t i)
{
  switch(i)
    {
    case 0:
      return BGPSTREAM_ADDR_VERSION_IPV4;
    case 1:
      return BGPSTREAM_ADDR_VERSION_IPV6;
    default:
      assert(0);
    }
  return BGPSTREAM_ADDR_VERSION_UNKNOWN;

}


uint8_t
bgpstream_ipv2number(bgpstream_addr_version_t v)
{
  switch(v)
    {
    case BGPSTREAM_ADDR_VERSION_IPV4:
      return 4;
    case BGPSTREAM_ADDR_VERSION_IPV6:
      return 6;
    default:
      assert(0);
    }
  return 255;
}


uint8_t
bgpstream_idx2number(uint8_t i)
{
  switch(i)
    {
    case 0:
      return 4;
    case 1:
      return 6;
    default:
      assert(0);
    }
  return 255;
}
