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
#include "bgpstream_test.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_LEN 1024
char buffer[BUFFER_LEN];

/* IPv4 Addresses */
#define IPV4_TEST_ADDR_A "192.0.43.8"
#define IPV4_TEST_ADDR_A_MASKED "192.0.43.0"
#define IPV4_TEST_ADDR_A_MASKLEN 24
#define IPV4_TEST_ADDR_B "192.172.226.3"

int test_addresses_ipv4()
{
  bgpstream_addr_storage_t a;
  bgpstream_addr_storage_t a_masked;
  bgpstream_addr_storage_t b;

  bgpstream_ipv4_addr_t a4;
  bgpstream_ipv4_addr_t a4_masked;
  bgpstream_ipv4_addr_t b4;

  /* IPv4 */
  CHECK("IPv4 address from string",
        bgpstream_str2addr(IPV4_TEST_ADDR_A, &a) != NULL);

  /* check conversion from and to string */
  bgpstream_addr_ntop(buffer, BUFFER_LEN, &a);
  CHECK("IPv4 address to string", strcmp(buffer, IPV4_TEST_ADDR_A) == 0);

  /* STORAGE CHECKS */

  /* populate address b (storage) */
  bgpstream_str2addr(IPV4_TEST_ADDR_B, &b);

  /* check generic equal */
  CHECK("IPv4 address generic-equals (cast from storage)",
        bgpstream_addr_equal((bgpstream_ip_addr_t *)&a,
                             (bgpstream_ip_addr_t *)&b) == 0 &&
          bgpstream_addr_equal((bgpstream_ip_addr_t *)&a,
                               (bgpstream_ip_addr_t *)&a) != 0);

  /* check storage equal */
  CHECK("IPv4 address storage-equals (storage)",
        bgpstream_addr_storage_equal(&a, &b) == 0 &&
          bgpstream_addr_storage_equal(&a, &a) != 0);

  /* IPV4-SPECIFIC CHECKS */

  /* populate ipv4 a */
  bgpstream_str2addr(IPV4_TEST_ADDR_A, (bgpstream_addr_storage_t *)&a4);
  /* populate ipv4 b */
  bgpstream_str2addr(IPV4_TEST_ADDR_B, (bgpstream_addr_storage_t *)&b4);

  /* check generic equal */
  CHECK("IPv4 address generic-equals (cast from ipv4)",
        bgpstream_addr_equal((bgpstream_ip_addr_t *)&a4,
                             (bgpstream_ip_addr_t *)&b4) == 0 &&
          bgpstream_addr_equal((bgpstream_ip_addr_t *)&a4,
                               (bgpstream_ip_addr_t *)&a4) != 0);

  /* check ipv4 equal */
  CHECK("IPv4 address ipv4-equals (ipv4)",
        bgpstream_ipv4_addr_equal(&a4, &b4) == 0 &&
          bgpstream_ipv4_addr_equal(&a4, &a4) != 0);

  /* MASK CHECKS */
  /* generic mask */
  bgpstream_str2addr(IPV4_TEST_ADDR_A, &a);
  bgpstream_str2addr(IPV4_TEST_ADDR_A_MASKED, &a_masked);

  bgpstream_addr_mask((bgpstream_ip_addr_t *)&a, IPV4_TEST_ADDR_A_MASKLEN);

  CHECK("IPv4 address generic-mask (cast from storage)",
        bgpstream_addr_equal((bgpstream_ip_addr_t *)&a,
                             (bgpstream_ip_addr_t *)&a_masked) != 0);

  /* ipv4-specific */
  bgpstream_str2addr(IPV4_TEST_ADDR_A, (bgpstream_addr_storage_t *)&a4);
  bgpstream_str2addr(IPV4_TEST_ADDR_A_MASKED,
                     (bgpstream_addr_storage_t *)&a4_masked);

  bgpstream_ipv4_addr_mask(&a4, IPV4_TEST_ADDR_A_MASKLEN);

  CHECK("IPv4 address ipv4-mask (ipv4)",
        bgpstream_ipv4_addr_equal(&a4, &a4_masked) != 0);

  /* copy checks */
  bgpstream_addr_copy((bgpstream_ip_addr_t *)&b, (bgpstream_ip_addr_t *)&a);

  CHECK("IPv4 address copy",
        bgpstream_addr_equal((bgpstream_ip_addr_t *)&a,
                             (bgpstream_ip_addr_t *)&b) != 0);

  return 0;
}

#define IPV6_TEST_ADDR_A "2001:500:88:200::8"
#define IPV6_TEST_ADDR_A_MASKED "2001:500:88::"
#define IPV6_TEST_ADDR_A_MASKLEN 48
#define IPV6_TEST_ADDR_B "2001:48d0:101:501::123"
#define IPV6_TEST_ADDR_B_MASKED "2001:48d0:101:501::"
#define IPV6_TEST_ADDR_B_MASKLEN 96

int test_addresses_ipv6()
{
  bgpstream_addr_storage_t a;
  bgpstream_addr_storage_t a_masked;
  bgpstream_addr_storage_t b;
  bgpstream_addr_storage_t b_masked;

  bgpstream_ipv6_addr_t a6;
  bgpstream_ipv6_addr_t a6_masked;
  bgpstream_ipv6_addr_t b6;
  bgpstream_ipv6_addr_t b6_masked;

  /* IPv6 */
  CHECK("IPv6 address from string",
        bgpstream_str2addr(IPV6_TEST_ADDR_A, &a) != NULL);

  /* check conversion from and to string */
  bgpstream_addr_ntop(buffer, BUFFER_LEN, &a);
  CHECK("IPv6 address to string", strcmp(buffer, IPV6_TEST_ADDR_A) == 0);

  /* STORAGE CHECKS */

  /* populate address b (storage) */
  bgpstream_str2addr(IPV6_TEST_ADDR_B, &b);

  /* check generic equal */
  CHECK("IPv6 address generic-equals (cast from storage)",
        bgpstream_addr_equal((bgpstream_ip_addr_t *)&a,
                             (bgpstream_ip_addr_t *)&b) == 0 &&
          bgpstream_addr_equal((bgpstream_ip_addr_t *)&a,
                               (bgpstream_ip_addr_t *)&a) != 0);

  /* check storage equal */
  CHECK("IPv6 address storage-equals (storage)",
        bgpstream_addr_storage_equal(&a, &b) == 0 &&
          bgpstream_addr_storage_equal(&a, &a) != 0);

  /* IPV6-SPECIFIC CHECKS */

  /* populate ipv6 a */
  bgpstream_str2addr(IPV6_TEST_ADDR_A, (bgpstream_addr_storage_t *)&a6);
  /* populate ipv6 b */
  bgpstream_str2addr(IPV6_TEST_ADDR_B, (bgpstream_addr_storage_t *)&b6);

  /* check generic equal */
  CHECK("IPv6 address generic-equals (cast from ipv6)",
        bgpstream_addr_equal((bgpstream_ip_addr_t *)&a6,
                             (bgpstream_ip_addr_t *)&b6) == 0 &&
          bgpstream_addr_equal((bgpstream_ip_addr_t *)&a6,
                               (bgpstream_ip_addr_t *)&a6) != 0);

  /* check ipv6 equal */
  CHECK("IPv6 address ipv6-equals (ipv6)",
        bgpstream_ipv6_addr_equal(&a6, &b6) == 0 &&
          bgpstream_ipv6_addr_equal(&a6, &a6) != 0);

  /* MASK CHECKS (addr a checks len < 64, addr b checks len > 64) */
  /* generic mask */
  bgpstream_str2addr(IPV6_TEST_ADDR_A, &a);
  bgpstream_str2addr(IPV6_TEST_ADDR_A_MASKED, &a_masked);
  bgpstream_addr_mask((bgpstream_ip_addr_t *)&a, IPV6_TEST_ADDR_A_MASKLEN);

  bgpstream_str2addr(IPV6_TEST_ADDR_B, &b);
  bgpstream_str2addr(IPV6_TEST_ADDR_B_MASKED, &b_masked);
  bgpstream_addr_mask((bgpstream_ip_addr_t *)&b, IPV6_TEST_ADDR_B_MASKLEN);

  CHECK("IPv6 address generic-mask (cast from storage)",
        bgpstream_addr_equal((bgpstream_ip_addr_t *)&a,
                             (bgpstream_ip_addr_t *)&a_masked) != 0 &&
          bgpstream_addr_equal((bgpstream_ip_addr_t *)&b,
                               (bgpstream_ip_addr_t *)&b_masked) != 0);

  /* ipv6-specific */
  bgpstream_str2addr(IPV6_TEST_ADDR_A, (bgpstream_addr_storage_t *)&a6);
  bgpstream_str2addr(IPV6_TEST_ADDR_A_MASKED,
                     (bgpstream_addr_storage_t *)&a6_masked);
  bgpstream_ipv6_addr_mask(&a6, IPV6_TEST_ADDR_A_MASKLEN);

  bgpstream_str2addr(IPV6_TEST_ADDR_B, (bgpstream_addr_storage_t *)&b6);
  bgpstream_str2addr(IPV6_TEST_ADDR_B_MASKED,
                     (bgpstream_addr_storage_t *)&b6_masked);
  bgpstream_ipv6_addr_mask(&b6, IPV6_TEST_ADDR_B_MASKLEN);

  CHECK("IPv6 address ipv6-mask (ipv6)",
        bgpstream_ipv6_addr_equal(&a6, &a6_masked) != 0 &&
          bgpstream_ipv6_addr_equal(&b6, &b6_masked) != 0);

  /* copy checks */
  bgpstream_addr_copy((bgpstream_ip_addr_t *)&b, (bgpstream_ip_addr_t *)&a);

  CHECK("IPv6 address copy",
        bgpstream_addr_equal((bgpstream_ip_addr_t *)&a,
                             (bgpstream_ip_addr_t *)&b) != 0);

  return 0;
}

int main()
{
  CHECK_SECTION("IPv4 addresses", test_addresses_ipv4() == 0);
  CHECK_SECTION("IPv6 addresses", test_addresses_ipv6() == 0);
  return 0;
}
