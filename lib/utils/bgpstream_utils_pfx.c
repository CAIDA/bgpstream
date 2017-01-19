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

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "khash.h"

#include "bgpstream_utils_pfx.h"

char *bgpstream_pfx_snprintf(char *buf, size_t len, bgpstream_pfx_t *pfx)
{
  char *p = buf;

  /* print the address */
  if (bgpstream_addr_ntop(buf, len, &(pfx->address)) == NULL) {
    return NULL;
  }

  while (*p != '\0') {
    p++;
    len--;
  }

  /* print the mask */
  snprintf(p, len, "/%" PRIu8, pfx->mask_len);

  return buf;
}

#if UINT_MAX == 0xffffffffu
unsigned int
#elif ULONG_MAX == 0xffffffffu
unsigned long
#endif
bgpstream_ipv4_pfx_hash(bgpstream_ipv4_pfx_t *pfx)
{
  // embed the network mask length in the 32 bits
  return __ac_Wang_hash(pfx->address.ipv4.s_addr | (uint32_t)pfx->mask_len);
}

#if ULONG_MAX == ULLONG_MAX
unsigned long
#else
unsigned long long
#endif
bgpstream_ipv6_pfx_hash(bgpstream_ipv6_pfx_t *pfx)
{
  // ipv6 number - we take most significant 64 bits only
  unsigned char *s6 = &(pfx->address.ipv6.s6_addr[0]);
  uint64_t address = *((uint64_t *)s6);
  // embed the network mask length in the 64 bits
  return __ac_Wang_hash(address | (uint64_t)pfx->mask_len);
}

#if ULONG_MAX == ULLONG_MAX
unsigned long
#else
unsigned long long
#endif
bgpstream_pfx_storage_hash(bgpstream_pfx_storage_t *pfx)
{
  if (pfx->address.version == BGPSTREAM_ADDR_VERSION_IPV4) {
    return bgpstream_ipv4_pfx_hash((bgpstream_ipv4_pfx_t *)pfx);
  }
  if (pfx->address.version == BGPSTREAM_ADDR_VERSION_IPV6) {
    return bgpstream_ipv6_pfx_hash((bgpstream_ipv6_pfx_t *)pfx);
  }
  return 0;
}

int bgpstream_pfx_equal(bgpstream_pfx_t *pfx1, bgpstream_pfx_t *pfx2)
{
  if (pfx1->address.version != pfx2->address.version) {
    return 0;
  }
  if (pfx1->address.version == BGPSTREAM_ADDR_VERSION_IPV4) {
    return bgpstream_ipv4_pfx_equal((bgpstream_ipv4_pfx_t *)pfx1,
                                    (bgpstream_ipv4_pfx_t *)pfx2);
  }
  if (pfx1->address.version == BGPSTREAM_ADDR_VERSION_IPV6) {
    return bgpstream_ipv6_pfx_equal((bgpstream_ipv6_pfx_t *)pfx1,
                                    (bgpstream_ipv6_pfx_t *)pfx2);
  }
  return 0;
}

int bgpstream_ipv4_pfx_equal(bgpstream_ipv4_pfx_t *pfx1,
                             bgpstream_ipv4_pfx_t *pfx2)
{
  return ((pfx1->mask_len == pfx2->mask_len) &&
          bgpstream_ipv4_addr_equal(&pfx1->address, &pfx2->address));
}

int bgpstream_ipv6_pfx_equal(bgpstream_ipv6_pfx_t *pfx1,
                             bgpstream_ipv6_pfx_t *pfx2)
{
  // note: it could be faster to use a loop when inserting differing prefixes

  return ((pfx1->mask_len == pfx2->mask_len) &&
          bgpstream_ipv6_addr_equal(&pfx1->address, &pfx2->address));
}

int bgpstream_pfx_storage_equal(bgpstream_pfx_storage_t *pfx1,
                                bgpstream_pfx_storage_t *pfx2)
{
  if (pfx1->mask_len == pfx2->mask_len) {
    return bgpstream_addr_storage_equal(&pfx1->address, &pfx2->address);
  }
  return 0;
}

int bgpstream_pfx_contains(bgpstream_pfx_t *outer, bgpstream_pfx_t *inner)
{
  bgpstream_addr_storage_t tmp;

  if (outer->address.version != inner->address.version ||
      outer->mask_len > inner->mask_len) {
    return 0;
  }

  bgpstream_addr_copy((bgpstream_ip_addr_t *)&tmp, &inner->address);
  bgpstream_addr_mask((bgpstream_ip_addr_t *)&tmp, outer->mask_len);
  return bgpstream_addr_equal((bgpstream_ip_addr_t *)&tmp, &outer->address);
}

bgpstream_pfx_storage_t *bgpstream_str2pfx(const char *pfx_str,
                                           bgpstream_pfx_storage_t *pfx)
{
  if (pfx_str == NULL || pfx == NULL) {
    return NULL;
  }

  char pfx_copy[INET6_ADDRSTRLEN + 3];
  char *endptr = NULL;

  /* strncpy() functions copy at most len characters from src into
   * dst.  If src is less than len characters long, the remainder of
   * dst is filled with `\0' characters.  Otherwise, dst is not
   * terminated. */
  strncpy(pfx_copy, pfx_str, INET6_ADDRSTRLEN + 3);
  if (pfx_copy[INET6_ADDRSTRLEN + 3 - 1] != '\0') {
    return NULL;
  }

  /* get pointer to ip/mask divisor */
  char *found = strchr(pfx_copy, '/');
  if (found == NULL) {
    return NULL;
  }

  *found = '\0';
  /* get the ip address */
  if (bgpstream_str2addr(pfx_copy, &pfx->address) == NULL) {
    return NULL;
  }

  /* get the mask len */
  errno = 0;
  unsigned long int r = strtoul(found + 1, &endptr, 10);
  int ret = errno;
  if (!(endptr != NULL && *endptr == '\0') || ret != 0 ||
      (pfx->address.version == BGPSTREAM_ADDR_VERSION_IPV4 && r > 32) ||
      (pfx->address.version == BGPSTREAM_ADDR_VERSION_IPV6 && r > 128)) {
    return NULL;
  }
  pfx->mask_len = (uint8_t)r;
  pfx->allowed_matches = BGPSTREAM_PREFIX_MATCH_ANY;

  bgpstream_addr_mask((bgpstream_ip_addr_t *)&pfx->address, pfx->mask_len);

  return pfx;
}
