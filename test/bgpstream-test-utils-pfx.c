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

#define IPV4_TEST_PFX_A "192.0.43.0/24"
#define IPV4_TEST_PFX_B "130.217.0.0/16"
#define IPV4_TEST_PFX_B_CHILD "130.217.250.0/24"

int test_prefixes_ipv4()
{
  bgpstream_pfx_storage_t a;
  bgpstream_pfx_storage_t b;

  bgpstream_ipv4_pfx_t a4;
  bgpstream_ipv4_pfx_t b4;

  /* build a prefix from a string */
  CHECK("IPv4 prefix from string",
        bgpstream_str2pfx(IPV4_TEST_PFX_A, &a) != NULL);

  /* convert prefix to string */
  bgpstream_pfx_snprintf(buffer, BUFFER_LEN, (bgpstream_pfx_t *)&a);
  CHECK("IPv4 prefix to string",
        strncmp(IPV4_TEST_PFX_A, buffer, BUFFER_LEN) == 0);

  /* STORAGE CHECKS */

  /* populate pfx b (storage) */
  bgpstream_str2pfx(IPV4_TEST_PFX_B, &b);

  /* check generic equal */
  CHECK(
    "IPv4 prefix generic-equals (cast from storage)",
    bgpstream_pfx_equal((bgpstream_pfx_t *)&a, (bgpstream_pfx_t *)&b) == 0 &&
      bgpstream_pfx_equal((bgpstream_pfx_t *)&a, (bgpstream_pfx_t *)&a) != 0);

  /* check storage equal */
  CHECK("IPv4 prefix storage-equals (storage)",
        bgpstream_pfx_storage_equal(&a, &b) == 0 &&
          bgpstream_pfx_storage_equal(&a, &a) != 0);

  /* IPV4-SPECIFIC CHECKS */

  /* populate ipv4 a */
  bgpstream_str2pfx(IPV4_TEST_PFX_A, (bgpstream_pfx_storage_t *)&a4);
  /* populate ipv4 b */
  bgpstream_str2pfx(IPV4_TEST_PFX_B, (bgpstream_pfx_storage_t *)&b4);

  /* check generic equal */
  CHECK(
    "IPv4 prefix generic-equals (cast from ipv4)",
    bgpstream_pfx_equal((bgpstream_pfx_t *)&a4, (bgpstream_pfx_t *)&b4) == 0 &&
      bgpstream_pfx_equal((bgpstream_pfx_t *)&a4, (bgpstream_pfx_t *)&a4) != 0);

  /* check ipv4 equal */
  CHECK("IPv4 prefix ipv4-equals (ipv4)",
        bgpstream_ipv4_pfx_equal(&a4, &b4) == 0 &&
          bgpstream_ipv4_pfx_equal(&a4, &a4) != 0);

  /* prefix contains (i.e. more specifics) */
  bgpstream_str2pfx(IPV4_TEST_PFX_B, &a);
  bgpstream_str2pfx(IPV4_TEST_PFX_B_CHILD, &b);
  /* b is a child of a BUT a is NOT a child of b */
  CHECK(
    "IPv4 prefix contains",
    bgpstream_pfx_contains((bgpstream_pfx_t *)&a, (bgpstream_pfx_t *)&b) != 0 &&
      bgpstream_pfx_contains((bgpstream_pfx_t *)&b, (bgpstream_pfx_t *)&a) ==
        0);

  return 0;
}

#define IPV6_TEST_PFX_A "2001:500:88::/48"
#define IPV6_TEST_PFX_A_CHILD "2001:500:88:beef::/64"
#define IPV6_TEST_PFX_B "2001:48d0:101:501::/64"
#define IPV6_TEST_PFX_B_CHILD "2001:48d0:101:501:beef::/96"

int test_prefixes_ipv6()
{
  bgpstream_pfx_storage_t a;
  bgpstream_pfx_storage_t a_child;
  bgpstream_pfx_storage_t b;
  bgpstream_pfx_storage_t b_child;

  bgpstream_ipv6_pfx_t a6;
  bgpstream_ipv6_pfx_t b6;

  /* build a prefix from a string */
  CHECK("IPv6 prefix from string",
        bgpstream_str2pfx(IPV6_TEST_PFX_A, &a) != NULL);

  /* convert prefix to string */
  bgpstream_pfx_snprintf(buffer, BUFFER_LEN, (bgpstream_pfx_t *)&a);
  CHECK("IPv6 prefix to string",
        strncmp(IPV6_TEST_PFX_A, buffer, BUFFER_LEN) == 0);

  /* STORAGE CHECKS */

  /* populate pfx b (storage) */
  bgpstream_str2pfx(IPV6_TEST_PFX_B, &b);

  /* check generic equal */
  CHECK(
    "IPv6 prefix generic-equals (cast from storage)",
    bgpstream_pfx_equal((bgpstream_pfx_t *)&a, (bgpstream_pfx_t *)&b) == 0 &&
      bgpstream_pfx_equal((bgpstream_pfx_t *)&a, (bgpstream_pfx_t *)&a) != 0);

  /* check storage equal */
  CHECK("IPv6 prefix storage-equals (storage)",
        bgpstream_pfx_storage_equal(&a, &b) == 0 &&
          bgpstream_pfx_storage_equal(&a, &a) != 0);

  /* IPV6-SPECIFIC CHECKS */

  /* populate ipv6 a */
  bgpstream_str2pfx(IPV6_TEST_PFX_A, (bgpstream_pfx_storage_t *)&a6);
  /* populate ipv6 b */
  bgpstream_str2pfx(IPV6_TEST_PFX_B, (bgpstream_pfx_storage_t *)&b6);

  /* check generic equal */
  CHECK(
    "IPv6 prefix generic-equals (cast from ipv6)",
    bgpstream_pfx_equal((bgpstream_pfx_t *)&a6, (bgpstream_pfx_t *)&b6) == 0 &&
      bgpstream_pfx_equal((bgpstream_pfx_t *)&a6, (bgpstream_pfx_t *)&a6) != 0);

  /* check ipv6 equal */
  CHECK("IPv6 prefix ipv6-equals (ipv6)",
        bgpstream_ipv6_pfx_equal(&a6, &b6) == 0 &&
          bgpstream_ipv6_pfx_equal(&a6, &a6) != 0);

  /* prefix contains (i.e. more specifics) */
  bgpstream_str2pfx(IPV6_TEST_PFX_A, &a);
  bgpstream_str2pfx(IPV6_TEST_PFX_A_CHILD, &a_child);
  bgpstream_str2pfx(IPV6_TEST_PFX_B, &b);
  bgpstream_str2pfx(IPV6_TEST_PFX_B_CHILD, &b_child);
  /* b is a child of a BUT a is NOT a child of b */
  CHECK("IPv6 prefix contains",
        bgpstream_pfx_contains((bgpstream_pfx_t *)&a,
                               (bgpstream_pfx_t *)&a_child) != 0 &&
          bgpstream_pfx_contains((bgpstream_pfx_t *)&a_child,
                                 (bgpstream_pfx_t *)&a) == 0 &&
          bgpstream_pfx_contains((bgpstream_pfx_t *)&b,
                                 (bgpstream_pfx_t *)&b_child) != 0 &&
          bgpstream_pfx_contains((bgpstream_pfx_t *)&b_child,
                                 (bgpstream_pfx_t *)&b) == 0);

  return 0;
}

int main()
{
  CHECK_SECTION("IPv4 prefixes", test_prefixes_ipv4() == 0);
  CHECK_SECTION("IPv6 prefixes", test_prefixes_ipv6() == 0);

  return 0;
}
