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

#if defined(FOUND_RTR)
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

int test_val_res_struct()
{
  uint32_t asn = TEST_PFX_ASN;
  uint8_t max_pfx_len = TEST_PFX_MAX_LEN;
  bgpstream_ipv4_addr_t pfxv4;
  bgpstream_str2pfx(IPV4_TEST_PFX_A, (bgpstream_pfx_storage_t *)&pfxv4);

  bgpstream_rpki_validation_result_t val_table;
  bgpstream_rpki_validation_result_init(&val_table, 2);
  CHECK("RTR: Compare max size after init", val_table.asn_size == 2);
  CHECK("RTR: Compare used asn size after init", val_table.asn_used == 0);
  CHECK("RTR: Compare asn_pfx pointer address after init",
        val_table.asn_pfx != NULL);

  /*
      the sequence to calculate the memory size is:
      max(2, 2**(ceil(log(X + 1) / log(2))))
      where X is the number of added elements
  */

  size_t memory_test_sizes[10] = {2, 2, 4, 4, 8, 8, 8, 8, 16, 16};

  int x;
  for (x = 0; x < 10; x++) {
    bgpstream_rpki_validation_result_insert_asn(&val_table, asn + x);
    CHECK("RTR: Compare used asn size after adding asn",
          val_table.asn_used == (x + 1));
    CHECK("RTR: Compare max asn size after adding asn",
          val_table.asn_size == memory_test_sizes[x]);
    CHECK("RTR: Compare asn in prefix array after adding asn",
          val_table.asn_pfx[x].asn == (asn + x));
    CHECK("RTR: Compare number of used prefix after adding asn",
          val_table.asn_pfx[x].pfx_used == 0);
    CHECK("RTR: Compare max number of prefix after adding asn",
          val_table.asn_pfx[x].pfx_size == 2);

    bgpstream_rpki_validation_result_insert_asn(&val_table, asn + x);
    CHECK("RTR: Compare used asn size after adding duplicate asn",
          val_table.asn_used == (x + 1));
    CHECK("RTR: Compare max asn size after adding duplicate asn",
          val_table.asn_size == memory_test_sizes[x]);
    CHECK("RTR: Compare number of used prefix after adding asn",
          val_table.asn_pfx[x].pfx_used == 0);
    CHECK("RTR: Compare max number of prefix after adding asn",
          val_table.asn_pfx[x].pfx_size == 2);
    int y;
    for (y = 0; y < 10; y++) {
      bgpstream_rpki_validation_result_insert_pfx(
          &val_table, asn + x, (bgpstream_pfx_t *)&pfxv4, max_pfx_len - y);
      CHECK("RTR: Compare used asn size after adding prefix",
            val_table.asn_used == (x + 1));
      CHECK("RTR: Compare max asn size after adding prefix",
            val_table.asn_size == memory_test_sizes[x]);
      CHECK("RTR: Compare asn after adding  prefix",
            val_table.asn_pfx[x].asn == (asn + x));
      CHECK("RTR: Compare number of used prefix of asn after adding prefix",
            val_table.asn_pfx[x].pfx_used == (y + 1));
      CHECK("RTR: Compare max number of prefix of asn after adding prefix",
            val_table.asn_pfx[x].pfx_size == memory_test_sizes[y]);
      CHECK("RTR: Compare prefix in prefix array of asn after adding prefix",
            inet_ntoa(val_table.asn_pfx[x].pfxs[y].pfx.address.ipv4) ==
                inet_ntoa(pfxv4.ipv4));
      CHECK("RTR: Compare max len of prefix of asn after adding prefix",
            val_table.asn_pfx[x].pfxs[y].max_pfx_len == (max_pfx_len - y));

      bgpstream_rpki_validation_result_insert_pfx(
          &val_table, asn + x, (bgpstream_pfx_t *)&pfxv4, max_pfx_len - y);
      CHECK("RTR: Compare used asn size after adding duplicate prefix",
            val_table.asn_used == (x + 1));
      CHECK("RTR: Compare max asn size after adding duplicate prefix",
            val_table.asn_size == memory_test_sizes[x]);
      CHECK("RTR: Compare asn after adding duplicate prefix",
            val_table.asn_pfx[x].asn == (asn + x));
      CHECK("RTR: Compare number of used prefix of asn after adding duplicate "
            "prefix",
            val_table.asn_pfx[x].pfx_used == (y + 1));
      CHECK("RTR: Compare max number of prefix of asn after adding duplicate "
            "prefix",
            val_table.asn_pfx[x].pfx_size == memory_test_sizes[y]);
      CHECK("RTR: Compare prefix in prefix array of asn after adding duplicate "
            "prefix",
            inet_ntoa(val_table.asn_pfx[x].pfxs[y].pfx.address.ipv4) ==
                inet_ntoa(pfxv4.ipv4));
      CHECK(
          "RTR: Compare max len of prefix of asn after adding duplicate prefix",
          val_table.asn_pfx[x].pfxs[y].max_pfx_len == (max_pfx_len - y));
    }
  }
  bgpstream_rpki_validation_result_free(&val_table);

  return 0;
}

int test_validation_process()
{
  bgpstream_set_rtr_config(TEST_RPKI_VALIDATOR_URL, TEST_RPKI_VALIDATOR_PORT,
                           NULL, NULL, NULL, true);

  cfg_tr =
      bgpstream_rtr_start_connection(rtr_server_conf.host, rtr_server_conf.port,
                                     NULL, NULL, NULL, NULL, NULL, NULL);
  struct pfx_table *pfxt = cfg_tr->groups[0].sockets[0]->pfx_table;
  bgpstream_elem_t *elem = bgpstream_elem_create();
  elem->annotations.rpki_validation_status =
      BGPSTREAM_ELEM_RPKI_VALIDATION_STATUS_NOTVALIDATED;

  uint8_t number_of_tests = 3;
  uint8_t pfxs_size = number_of_tests * number_of_tests;
  struct pfx_record pfxs[pfxs_size];

  int test_asn_index;
  for (test_asn_index = 0; test_asn_index < number_of_tests; test_asn_index++) {
    int test_pfx_index;
    for (test_pfx_index = 0; test_pfx_index < number_of_tests;
         test_pfx_index++) {
      struct pfx_record pfx;
      pfx.min_len = TEST_MIN_LEN - test_pfx_index;
      pfx.max_len = TEST_MAX_LEN - test_pfx_index;
      pfx.socket = TEST_SOCKET;
      pfx.asn = TEST_ASN + test_asn_index;
      char ip_address[INET_ADDRSTRLEN];
      strcpy(ip_address, TEST_IP_ADDR);
      lrtr_ip_str_to_addr(ip_address, &pfx.prefix);
      CHECK("RTR: Add a valid pfx_record to pfx_table",
            pfx_table_add(pfxt, &pfx) == PFX_SUCCESS);
      pfxs[(test_asn_index * number_of_tests) + test_pfx_index] = pfx;
      bgpstream_elem_get_rpki_validation_result(elem, ip_address, pfx.asn,
                                                pfx.max_len);
    }
  }

  bgpstream_rpki_validation_result_t val_table;
  val_table = elem->annotations.rpki_validation_result;
  int test_index;
  for (test_index = 0; test_index < pfxs_size; test_index++) {
    int asn_index;
    for (asn_index = 0; asn_index < val_table.asn_used; asn_index++) {
      if (val_table.asn_pfx[asn_index].asn == pfxs[test_index].asn) {
        CHECK("RTR: Inspect result struct to find added asn", true);
        int pfx_index;
        for (pfx_index = 0; pfx_index < val_table.asn_pfx[asn_index].pfx_used;
             pfx_index++) {
          char pfx_string[INET6_ADDRSTRLEN];
          lrtr_ip_addr_to_str(&pfxs[test_index].prefix, pfx_string,
                              INET6_ADDRSTRLEN);
          if (inet_ntoa(val_table.asn_pfx[asn_index]
                            .pfxs[pfx_index]
                            .pfx.address.ipv4) == pfx_string) {
            CHECK("RTR: Inspect result struct to find added pfx", true);
            CHECK("RTR: Compare max len of prefix ",
                  val_table.asn_pfx[asn_index].pfxs[pfx_index].max_pfx_len ==
                      pfxs[test_index].max_len);
          }
        }
      }
    }
  }
  return 0;
}
#endif

int main()
{
#if defined(FOUND_RTR)
  CHECK_SECTION("RTRLIB VALIDATION", test_rtr_validation() == 0);
  CHECK_SECTION("RTRLIB PFXT STRUCT", test_val_res_struct() == 0);
  CHECK_SECTION("RTRLIB VALIDATION PROCESS", test_validation_process() == 0);
#endif
  return 0;
}
