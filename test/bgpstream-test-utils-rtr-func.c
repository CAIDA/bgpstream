#include "bgpstream_test.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define TEST_FILE "ris.rrc06.ribs.1427846400.gz"
#define TEST_W_START 0
#define TEST_W_END -1
#define TEST_BUF_S 1024

#define TEST1_O_ASN 12654
#define TEST1_PFX "93.175.146.0/24"
#define TEST1_MAXL 24

#define TEST2_O_ASN 12654
#define TEST2_PFX "2001:7fb:fd02::/48"
#define TEST2_MAXL 48

#define TEST3_O_ASN 196615
#define TEST3_PFX "93.175.147.0/24"
#define TEST3_MAXL 24

#define TEST4_O_ASN 196615
#define TEST4_PFX "2001:7fb:fd03::/48"
#define TEST4_MAXL 48

#define TEST5_PFX "84.205.83.0/24"
#define TEST6_PFX "2001:7fb:ff03::/48"

#define TEST_RPKI_VALIDATOR_URL "rpki-validator.realmv6.org"
#define TEST_RPKI_VALIDATOR_PORT "8282"

#define SETUP                                   \
  do {                                          \
    bs = bgpstream_create();                    \
    rec = bgpstream_record_create();            \
  } while(0)

#define TEARDOWN                                \
  do {                                          \
    bgpstream_record_destroy(rec);              \
    rec = NULL;                                 \
    bgpstream_destroy(bs);                      \
    bs = NULL;                                  \
  } while(0)

#define SET_INTERFACE(interface)                                        \
  do {                                                                  \
    datasource_id =                                                     \
           bgpstream_get_data_interface_id_by_name(bs, STR(interface)); \
    bgpstream_set_data_interface(bs, datasource_id);                    \
  } while(0)

#ifdef WITH_RTR
bgpstream_t *bs;
bgpstream_elem_t *elem; 
bgpstream_record_t *rec;
bgpstream_data_interface_id_t datasource_id = 0;
bgpstream_data_interface_option_t *option;

int test_RPKI_validation()
{
  int ret;
  uint32_t asn;
  char pfx[TEST_BUF_S];
  char rpki[TEST_BUF_S];
  char buf[TEST_BUF_S];

  SETUP;
  SET_INTERFACE(singlefile);

  option = bgpstream_get_data_interface_option_by_name(bs, datasource_id,
                                                       "rib-file");
  bgpstream_set_data_interface_option(bs, option, TEST_FILE);

  bgpstream_add_interval_filter(bs, TEST_W_START, TEST_W_END);
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PREFIX, TEST1_PFX);
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PREFIX, TEST2_PFX);

  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PREFIX, TEST3_PFX);
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PREFIX, TEST4_PFX);

  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PREFIX, TEST5_PFX);
  bgpstream_add_filter(bs, BGPSTREAM_FILTER_TYPE_ELEM_PREFIX, TEST6_PFX);

  /* Set RTR config and connect to public RPKI cache server */
  bgpstream_set_rtr_config(bs, TEST_RPKI_VALIDATOR_URL, TEST_RPKI_VALIDATOR_PORT,
                           NULL, NULL, NULL, 1);
  bgpstream_start(bs);

  while((ret = bgpstream_get_next_record(bs, rec)) > 0) {
    if(rec->status == BGPSTREAM_RECORD_STATUS_VALID_RECORD) {
      while((elem = bgpstream_record_get_next_elem(rec)) != NULL) {
        bgpstream_pfx_snprintf(pfx, sizeof(pfx), (bgpstream_pfx_t*)&(elem->prefix));
        bgpstream_elem_get_rpki_validation_result_snprintf(rpki, sizeof(rpki), elem);
        bgpstream_as_path_get_origin_val(elem->aspath, &asn);

        /* Validate RPKI-enabled BGP Beacons from RIPE */
        /* see: https://www.ripe.net/analyse/internet-measurements/routing-information-service-ris/current-ris-routing-beacons */
 
        if(!strcmp(pfx, TEST1_PFX) && asn == TEST1_O_ASN) {
          snprintf(buf, sizeof buf, "%s;%i,%s-%i", "valid", TEST1_O_ASN, TEST1_PFX, TEST1_MAXL);
          CHECK("RPKI-Testcase 1: Validation status for valid ROA Beacon",
                !strcmp(rpki, buf));
        } 
        if(!strcmp(pfx, TEST2_PFX) && asn == TEST2_O_ASN) {
          snprintf(buf, sizeof buf, "%s;%i,%s-%i", "valid", TEST2_O_ASN, TEST2_PFX, TEST2_MAXL);
          CHECK("RPKI-Testcase 2: Validation status for valid ROA Beacon",
                !strcmp(rpki, buf));
        }
        if(!strcmp(pfx, TEST3_PFX)) {
          snprintf(buf, sizeof buf, "%s;%i,%s-%i", "invalid", TEST3_O_ASN, TEST3_PFX, TEST3_MAXL);
          CHECK("RPKI-Testcase 3: Validation status for invalid ROA Beacon",
                !strcmp(rpki, buf));
        }

        if(!strcmp(pfx, TEST4_PFX)) {
          snprintf(buf, sizeof buf, "%s;%i,%s-%i", "invalid", TEST4_O_ASN, TEST4_PFX, TEST4_MAXL);
          CHECK("RPKI-Testcase 4: Validation status for invalid ROA Beacon",
                !strcmp(rpki, buf));
        }

        if(!strcmp(pfx, TEST5_PFX)) {
          CHECK("RPKI-Testcase 5: Validation status for non-existing ROA Beacon",
                !strcmp(rpki, "notfound"));
        }

        if(!strcmp(pfx, TEST6_PFX)) {
          CHECK("RPKI-Testcase 6: Validation status for non-existing ROA Beacon",
                !strcmp(rpki, "notfound"));
        }
        buf[0] = '\0';
      }
    }
  }

  bgpstream_stop(bs);

  TEARDOWN;
  return 0;
}
#endif

int main()
{
#if WITH_RTR
  CHECK_SECTION("RPKI validation functional test", !test_RPKI_validation());
#else
  SKIPPED("RPKI validation functional test");
#endif
  return 0;
}
