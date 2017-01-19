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
#define IPV4_TEST_PFX_CNT 3
#define IPV4_TEST_24_CNT 257
#define IPV4_TEST_PFX_OVERLAP "130.217.0.0/20"

#define IPV6_TEST_PFX_A "2001:500:88::/48"
#define IPV6_TEST_PFX_A_CHILD "2001:500:88:beef::/64"
#define IPV6_TEST_PFX_B "2001:48d0:101:501::/64"
#define IPV6_TEST_PFX_B_CHILD "2001:48d0:101:501:beef::/96"
#define IPV6_TEST_64_CNT 65537
#define IPV6_TEST_PFX_CNT 4

int test_patricia()
{
  bgpstream_patricia_tree_t *pt;
  bgpstream_patricia_tree_result_set_t *res;
  bgpstream_pfx_storage_t pfx;

  /* Create a Patricia Tree */
  CHECK("Create Patricia Tree",
        (pt = bgpstream_patricia_tree_create(NULL)) != NULL);

  /* Create a Patricia Tree */
  CHECK("Create Patricia Tree Result",
        (res = bgpstream_patricia_tree_result_set_create()) != NULL);

  /* Insert into Patricia Tree */

  CHECK("Insert into Patricia Tree v4",
        bgpstream_patricia_tree_insert(pt, (bgpstream_pfx_t *)bgpstream_str2pfx(
                                             IPV4_TEST_PFX_A, &pfx)) != NULL);
  CHECK("Insert into Patricia Tree v4",
        bgpstream_patricia_tree_insert(pt, (bgpstream_pfx_t *)bgpstream_str2pfx(
                                             IPV4_TEST_PFX_B, &pfx)) != NULL);
  CHECK("Insert into Patricia Tree v4",
        bgpstream_patricia_tree_insert(pt, (bgpstream_pfx_t *)bgpstream_str2pfx(
                                             IPV4_TEST_PFX_B_CHILD, &pfx)) !=
          NULL);

  CHECK("Insert into Patricia Tree v6",
        bgpstream_patricia_tree_insert(pt, (bgpstream_pfx_t *)bgpstream_str2pfx(
                                             IPV6_TEST_PFX_A, &pfx)) != NULL);
  CHECK("Insert into Patricia Tree v6",
        bgpstream_patricia_tree_insert(pt, (bgpstream_pfx_t *)bgpstream_str2pfx(
                                             IPV6_TEST_PFX_A_CHILD, &pfx)) !=
          NULL);
  CHECK("Insert into Patricia Tree v6",
        bgpstream_patricia_tree_insert(pt, (bgpstream_pfx_t *)bgpstream_str2pfx(
                                             IPV6_TEST_PFX_B, &pfx)) != NULL);
  CHECK("Insert into Patricia Tree v6",
        bgpstream_patricia_tree_insert(pt, (bgpstream_pfx_t *)bgpstream_str2pfx(
                                             IPV6_TEST_PFX_B_CHILD, &pfx)) !=
          NULL);

  /* Count prefixes */
  CHECK("Count Patricia Tree v4 prefixes",
        bgpstream_patricia_prefix_count(pt, BGPSTREAM_ADDR_VERSION_IPV4) ==
          IPV4_TEST_PFX_CNT);
  CHECK("Count Patricia Tree v6 prefixes",
        bgpstream_patricia_prefix_count(pt, BGPSTREAM_ADDR_VERSION_IPV6) ==
          IPV6_TEST_PFX_CNT);

  /* Search prefixes */
  CHECK("Patricia Tree v4 search exact",
        bgpstream_patricia_tree_search_exact(
          pt, (bgpstream_pfx_t *)bgpstream_str2pfx(IPV4_TEST_PFX_A, &pfx)) !=
          NULL);
  CHECK("Patricia Tree v6 search exact",
        bgpstream_patricia_tree_search_exact(
          pt, (bgpstream_pfx_t *)bgpstream_str2pfx(IPV6_TEST_PFX_A, &pfx)) !=
          NULL);

  /* Overlap info */
  CHECK(
    "Patricia Tree v4 overlap info",
    bgpstream_patricia_tree_get_pfx_overlap_info(
      pt, (bgpstream_pfx_t *)bgpstream_str2pfx(IPV4_TEST_PFX_OVERLAP, &pfx)) ==
        BGPSTREAM_PATRICIA_LESS_SPECIFICS ||
      BGPSTREAM_PATRICIA_MORE_SPECIFICS);
  CHECK("Patricia Tree v6 overlap info",
        bgpstream_patricia_tree_get_pfx_overlap_info(
          pt, (bgpstream_pfx_t *)bgpstream_str2pfx(IPV6_TEST_PFX_B, &pfx)) ==
            BGPSTREAM_PATRICIA_EXACT_MATCH ||
          BGPSTREAM_PATRICIA_MORE_SPECIFICS);

  /* Count minimum coverage prefixes */
  CHECK("Patricia Tree v4 minimum coverage",
        bgpstream_patricia_tree_get_minimum_coverage(
          pt, BGPSTREAM_ADDR_VERSION_IPV4, res) == 0 &&
          bgpstream_patricia_tree_result_set_count(res) == 2);
  CHECK("Patricia Tree v6 minimum coverage",
        bgpstream_patricia_tree_get_minimum_coverage(
          pt, BGPSTREAM_ADDR_VERSION_IPV6, res) == 0 &&
          bgpstream_patricia_tree_result_set_count(res) == 2);

  /* Count prefixes subnets */
  CHECK("Patricia Tree v4 /24 subnets",
        bgpstream_patricia_tree_count_24subnets(pt) == IPV4_TEST_24_CNT);
  CHECK("Patricia Tree v6 /64 subnets",
        bgpstream_patricia_tree_count_64subnets(pt) == IPV6_TEST_64_CNT);

  bgpstream_patricia_tree_destroy(pt);
  bgpstream_patricia_tree_result_set_destroy(&res);

  return 0;
}

int main()
{
  CHECK_SECTION("Patricia Tree", test_patricia() == 0);
  return 0;
}
