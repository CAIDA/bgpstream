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

#define TEST_MIN_LEN 24
#define TEST_MAX_LEN 24
#define TEST_SOCKET NULL
#define TEST_ASN 12654
#define TEST_IP_ADDR "93.175.146.0\0"

#define IPV4_TEST_PFX_A "192.0.43.0/31"
#define TEST_PFX_MAX_LEN 31
#define TEST_PFX_ASN 12340

#define TEST_RPKI_VALIDATOR_URL "rpki-validator.realmv6.org"
#define TEST_RPKI_VALIDATOR_PORT "8282"

#ifdef WITH_RTR
int test_rtr_validation()
{
  cfg_tr = bgpstream_rtr_start_connection(TEST_RPKI_VALIDATOR_URL,
                                          TEST_RPKI_VALIDATOR_PORT, NULL, NULL,
                                          NULL, NULL, NULL, NULL);
  struct pfx_table *pfxt = cfg_tr->groups[0].sockets[0]->pfx_table;

  struct pfx_record pfx_v4;
  char ipv4_address[INET_ADDRSTRLEN];
  pfx_v4.min_len = TEST_MIN_LEN;
  pfx_v4.max_len = TEST_MAX_LEN;
  pfx_v4.socket = TEST_SOCKET;
  pfx_v4.asn = TEST_ASN;
  strcpy(ipv4_address, TEST_IP_ADDR);
  lrtr_ip_str_to_addr(ipv4_address, &pfx_v4.prefix);
  pfx_table_remove(pfxt, &pfx_v4);

  struct pfx_record pfx_v6;
  char ipv6_address[INET6_ADDRSTRLEN];
  pfx_v6.min_len = 48;
  pfx_v6.max_len = 48;
  pfx_v6.socket = NULL;
  pfx_v6.asn = 12654;
  strcpy(ipv6_address, "2001:7fb:fd02::\0");
  lrtr_ip_str_to_addr(ipv6_address, &pfx_v6.prefix);
  pfx_table_remove(pfxt, &pfx_v6);

  struct reasoned_result res;

  CHECK("RTR: Add a valid pfx_record to pfx_table",
        pfx_table_add(pfxt, &pfx_v4) == PFX_SUCCESS);
  res = bgpstream_rtr_validate_reason(cfg_tr, pfx_v4.asn, ipv4_address,
                                      pfx_v4.max_len);
  CHECK("RTR: Compare valid IPv4 ROA with validation result",
        res.result == BGP_PFXV_STATE_VALID);

  CHECK("RTR: Add a valid pfx_record to pfx_table",
        pfx_table_add(pfxt, &pfx_v6) == PFX_SUCCESS);
  res = bgpstream_rtr_validate_reason(cfg_tr, pfx_v6.asn, ipv6_address,
                                      pfx_v6.max_len);
  CHECK("RTR: Compare valid IPv6 ROA with validation result",
        res.result == BGP_PFXV_STATE_VALID);

  res = bgpstream_rtr_validate_reason(cfg_tr, 196615, ipv4_address,
                                      pfx_v4.max_len);
  CHECK("RTR: Compare invalid (asn) IPv4 ROA with validation result",
        res.result == BGP_PFXV_STATE_INVALID);

  res = bgpstream_rtr_validate_reason(cfg_tr, 196615, ipv6_address,
                                      pfx_v6.max_len);
  CHECK("RTR: Compare invalid (asn) IPv6 ROA with validation result",
        res.result == BGP_PFXV_STATE_INVALID);

  res = bgpstream_rtr_validate_reason(cfg_tr, pfx_v4.asn, ipv4_address, 30);
  CHECK("RTR: Compare invalid (max len) IPv4 ROA with validation result",
        res.result == BGP_PFXV_STATE_INVALID);

  res = bgpstream_rtr_validate_reason(cfg_tr, pfx_v6.asn, ipv6_address, 50);
  CHECK("RTR: Compare invalid (max len) IPv6 ROA with validation result",
        res.result == BGP_PFXV_STATE_INVALID);

  pfx_v4.asn = 12345;
  lrtr_ip_str_to_addr("84.205.83.0\0", &pfx_v4.prefix);
  pfx_table_remove(pfxt, &pfx_v4);
  res = bgpstream_rtr_validate_reason(cfg_tr, 12345, "84.205.83.0\0",
                                      pfx_v4.max_len);
  CHECK("RTR: Compare none existend IPv4 ROA with validation result",
        res.result == BGP_PFXV_STATE_NOT_FOUND);

  pfx_v6.asn = 12345;
  lrtr_ip_str_to_addr("2001:7fb:ff03::\0", &pfx_v6.prefix);
  pfx_table_remove(pfxt, &pfx_v6);
  res = bgpstream_rtr_validate_reason(cfg_tr, 12345, "2001:7fb:ff03::\0",
                                      pfx_v6.max_len);
  CHECK("RTR: Compare none existend IPv6 ROA with validation result",
        res.result == BGP_PFXV_STATE_NOT_FOUND);

  free(res.reason);
  bgpstream_rtr_close_connection(cfg_tr);

  return 0;
}

#endif

int main()
{
#ifdef WITH_RTR
  CHECK_SECTION("RTRLIB VALIDATION", test_rtr_validation() == 0);
#else
  SKIPPED("RPKI validation unit test");
#endif
  return 0;
}
