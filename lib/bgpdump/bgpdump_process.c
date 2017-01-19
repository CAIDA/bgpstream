/*
  Copyright (c) 2007 - 2010 RIPE NCC - All Rights Reserved

  Permission to use, copy, modify, and distribute this software and its
  documentation for any purpose and without fee is hereby granted, provided
  that the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation, and that the name of the author not be used in advertising or
  publicity pertaining to distribution of the software without specific,
  written prior permission.

  THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS; IN NO EVENT SHALL
  AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
  DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  Parts of this code have been engineered after analiyzing GNU Zebra's
  source code and therefore might contain declarations/code from GNU
  Zebra, Copyright (C) 1999 Kunihiro Ishiguro. Zebra is a free routing
  software, distributed under the GNU General Public License. A copy of
  this license is included with libbgpdump.

  Original Author: Shufu Mao(msf98@mails.tsinghua.edu.cn)
*/

#include "config.h"

#include "bgpdump_lib.h"
#include "bgpdump_util.h"

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static void show_attr(attributes_t *attr);
static void show_prefixes(int count, struct prefix *prefix);
static void table_line_announce_1(struct mp_nlri *prefix, int count,
                                  BGPDUMP_ENTRY *entry, char *time_str);
static void table_line_announce(struct prefix *prefix, int count,
                                BGPDUMP_ENTRY *entry, char *time_str);
static void table_line_withdraw(struct prefix *prefix, int count,
                                BGPDUMP_ENTRY *entry, char *time_str);
static void table_line_mrtd_route(BGPDUMP_MRTD_TABLE_DUMP *route,
                                  BGPDUMP_ENTRY *entry);
static void table_line_dump_v2_prefix(BGPDUMP_TABLE_DUMP_V2_PREFIX *e,
                                      BGPDUMP_ENTRY *entry);

void show_prefixes6(int count, struct prefix *prefix);
static void table_line_withdraw6(struct prefix *prefix, int count,
                                 BGPDUMP_ENTRY *entry, char *time_str);
static void table_line_announce6(struct mp_nlri *prefix, int count,
                                 BGPDUMP_ENTRY *entry, char *time_str);

/* If no aspath was present as a string in the packet, return an empty string
 * so everything stays machine parsable */
static char *attr_aspath(attributes_t *a)
{
  if (a->flag & ATTR_FLAG_BIT(BGP_ATTR_AS_PATH) && a->aspath) {
    if (!a->aspath->str) {
      process_attr_aspath_string(a->aspath, 1);
    }
    return a->aspath->str;
  }
  return "";
}

static int mode = 1;     // m option - H = 0, m =1, M =2
static int timetype = 0; // dump = 0 [default], change = 1

const char *bgp_state_name[] = {"Unknown",     "Idle",     "Connect",
                                "Active",      "Opensent", "Openconfirm",
                                "Established", NULL};

void bgpdump_print_entry(BGPDUMP_ENTRY *entry)
{

  assert(entry);
  bgpdump_log_to_stderr();

  struct tm *date;
  char time_str[128];
  char time_str2[128];
  char time_str_fixed[128];
  char prefix[BGPDUMP_ADDRSTRLEN];

  date = gmtime(&entry->time);
  bgpdump_time2str(date, time_str);
  bgpdump_time2str(date, time_str_fixed);
  if (mode == 0) {
    printf("TIME: %s\n", time_str);
  }
  // printf("TIME: %s",asctime(gmtime(&entry->time)));
  // printf("LENGTH          : %u\n", entry->length);
  switch (entry->type) {
  case BGPDUMP_TYPE_MRTD_TABLE_DUMP:
    if (mode == 0) {
      const char *prefix_str = NULL;
      switch (entry->subtype) {
      case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP6:
        printf("TYPE: TABLE_DUMP/INET6\n");
        prefix_str =
          bgpdump_fmt_ipv6(entry->body.mrtd_table_dump.prefix, prefix);
        break;

      case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP6_32BIT_AS:
        printf("TYPE: TABLE_DUMP/INET6_32BIT_AS\n");
        prefix_str =
          bgpdump_fmt_ipv6(entry->body.mrtd_table_dump.prefix, prefix);
        break;
      case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP:
        printf("TYPE: TABLE_DUMP/INET\n");
        prefix_str = inet_ntoa(entry->body.mrtd_table_dump.prefix.v4_addr);
        break;

      case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP_32BIT_AS:
        printf("TYPE: TABLE_DUMP/INET_32BIT_AS\n");
        prefix_str = inet_ntoa(entry->body.mrtd_table_dump.prefix.v4_addr);
        break;

      default:
        printf("Error: unknown table type %d\n", entry->subtype);
        return;
      }
      printf("VIEW: %d\n", entry->body.mrtd_table_dump.view);
      printf("SEQUENCE: %d\n", entry->body.mrtd_table_dump.sequence);
      printf("PREFIX: %s/%d\n", prefix_str, entry->body.mrtd_table_dump.mask);
      printf("FROM:");
      switch (entry->subtype) {
      case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP6:
      case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP6_32BIT_AS:

        bgpdump_fmt_ipv6(entry->body.mrtd_table_dump.peer_ip, prefix);
        printf("%s ", prefix);
        break;
      case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP:
      case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP_32BIT_AS:
        if (entry->body.mrtd_table_dump.peer_ip.v4_addr.s_addr != 0x00000000L)
          printf("%s ", inet_ntoa(entry->body.mrtd_table_dump.peer_ip.v4_addr));
        else
          printf("N/A ");
      }
      printf("AS%u\n", entry->body.mrtd_table_dump.peer_as);

      // printf("FROM: %s
      // AS%d\n",inet_ntoa(entry->body.mrtd_table_dump.peer_ip.v4_addr),entry->body.mrtd_table_dump.peer_as);
      // time2str(localtime(&entry->body.mrtd_table_dump.uptime),time_str2);
      bgpdump_time2str(gmtime(&entry->body.mrtd_table_dump.uptime), time_str2);
      printf("ORIGINATED: %s\n", time_str2);
      if (entry->attr && entry->attr->len)
        show_attr(entry->attr);

      printf("STATUS: 0x%x\n", entry->body.mrtd_table_dump.status);

      // printf("    UPTIME      :
      // %s",asctime(gmtime(&entry->body.mrtd_table_dump.uptime)));
      // printf("    PEER IP     :
      // %s\n",inet_ntoa(entry->body.mrtd_table_dump.peer_ip));
      // printf("    PEER IP     :
      // %s\n",inet_ntoa(entry->body.mrtd_table_dump.peer_ip.v4_addr));
      // printf("    PEER AS     : %d\n",entry->body.mrtd_table_dump.peer_as);
    } else if (mode == 1 || mode == 2) // -m -M
    {
      table_line_mrtd_route(&entry->body.mrtd_table_dump, entry);
    }
    break;

  case BGPDUMP_TYPE_TABLE_DUMP_V2:
    if (mode == 0) {
      char peer_ip[BGPDUMP_ADDRSTRLEN];
      // char time_str[30];
      int i;

      BGPDUMP_TABLE_DUMP_V2_PREFIX *e;
      e = &entry->body.mrtd_table_dump_v2_prefix;

      if (e->afi == AFI_IP) {
        strncpy(prefix, inet_ntoa(e->prefix.v4_addr), BGPDUMP_ADDRSTRLEN);
      } else if (e->afi == AFI_IP6) {
        bgpdump_fmt_ipv6(e->prefix, prefix);
      }

      for (i = 0; i < e->entry_count; i++) {
        // This is slightly nasty - as we want to print multiple entries
        // for multiple peers, we may need to print another TIME ourselves
        if (i)
          printf("\nTIME: %s\n", time_str_fixed);
        if (e->afi == AFI_IP) {
          printf("TYPE: TABLE_DUMP_V2/IPV4_UNICAST\n");
        } else if (e->afi == AFI_IP6) {
          printf("TYPE: TABLE_DUMP_V2/IPV6_UNICAST\n");
        }
        printf("PREFIX: %s/%d\n", prefix, e->prefix_length);
        printf("SEQUENCE: %d\n", e->seq);

        if (e->entries[i].peer.afi == AFI_IP) {
          bgpdump_fmt_ipv4(e->entries[i].peer.peer_ip, peer_ip);
        } else if (e->entries[i].peer.afi == AFI_IP6) {
          bgpdump_fmt_ipv6(e->entries[i].peer.peer_ip, peer_ip);
        } else {
          sprintf(peer_ip, "[N/A, unsupported AF]");
        }
        printf("FROM: %s AS%u\n", peer_ip, e->entries[i].peer.peer_as);
        time_t time_temp = (time_t)((e->entries[i]).originated_time);
        bgpdump_time2str(gmtime(&time_temp), time_str);
        printf("ORIGINATED: %s\n", time_str);
        if (e->entries[i].attr && e->entries[i].attr->len)
          show_attr(e->entries[i].attr);
      }
    } else if (mode == 1 || mode == 2) { // -m -M
      table_line_dump_v2_prefix(&entry->body.mrtd_table_dump_v2_prefix, entry);
    }
    break;

  case BGPDUMP_TYPE_ZEBRA_BGP:

    switch (entry->subtype) {
    case BGPDUMP_SUBTYPE_ZEBRA_BGP_MESSAGE:
    case BGPDUMP_SUBTYPE_ZEBRA_BGP_MESSAGE_AS4:

      switch (entry->body.zebra_message.type) {
      case BGP_MSG_UPDATE:
        if (mode == 0) {
          printf("TYPE: BGP4MP/MESSAGE/Update\n");
          if (entry->body.zebra_message.source_as) {
            printf("FROM:");
            switch (entry->body.zebra_message.address_family) {
            case AFI_IP6:
              bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, prefix);
              printf(" %s ", prefix);
              break;
            case AFI_IP:
            default:
              if (entry->body.zebra_message.source_ip.v4_addr.s_addr !=
                  0x00000000L)
                printf(" %s ",
                       inet_ntoa(entry->body.zebra_message.source_ip.v4_addr));
              else
                printf(" N/A ");
            }
            printf("AS%u\n", entry->body.zebra_message.source_as);
          }
          if (entry->body.zebra_message.destination_as) {
            printf("TO:");
            switch (entry->body.zebra_message.address_family) {
            case AFI_IP6:
              bgpdump_fmt_ipv6(entry->body.zebra_message.destination_ip,
                               prefix);
              printf(" %s ", prefix);
              break;
            case AFI_IP:
            default:
              if (entry->body.zebra_message.destination_ip.v4_addr.s_addr !=
                  0x00000000L)
                printf(
                  " %s ",
                  inet_ntoa(entry->body.zebra_message.destination_ip.v4_addr));
              else
                printf(" N/A ");
            }
            printf("AS%u\n", entry->body.zebra_message.destination_as);
          }
          if (entry->attr && entry->attr->len)
            show_attr(entry->attr);
          if (entry->body.zebra_message.cut_bytes) {
            u_int16_t cutted, idx;
            u_int8_t buf[128];

            printf("   INCOMPLETE PACKET: %d bytes cutted\n",
                   entry->body.zebra_message.cut_bytes);
            printf("   INCOMPLETE PART: ");
            if (entry->body.zebra_message.incomplete.orig_len) {
              cutted = entry->body.zebra_message.incomplete.prefix.len / 8 + 1;
              buf[0] = entry->body.zebra_message.incomplete.orig_len;
              memcpy(buf + 1,
                     &entry->body.zebra_message.incomplete.prefix.address,
                     cutted - 1);

              for (idx = 0; idx < cutted; idx++) {
                if (buf[idx] < 0x10)
                  printf("0%x ", buf[idx]);
                else
                  printf("%x ", buf[idx]);
              }
            }
            printf("\n");
          }
          if (!entry->attr)
            return;
          if ((entry->body.zebra_message.withdraw_count) ||
              (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_UNREACH_NLRI))) {
            if ((entry->body.zebra_message.withdraw_count) ||
                (entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST] &&
                 entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST]
                   ->prefix_count) ||
                (entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST] &&
                 entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST]
                   ->prefix_count) ||
                (entry->attr->mp_info
                   ->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST] &&
                 entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST]
                   ->prefix_count) ||
                (entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST] &&
                 entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST]
                   ->prefix_count) ||
                (entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST] &&
                 entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST]
                   ->prefix_count) ||
                (entry->attr->mp_info
                   ->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST] &&
                 entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST]
                   ->prefix_count))
              printf("WITHDRAW\n");
            if (entry->body.zebra_message.withdraw_count)
              show_prefixes(entry->body.zebra_message.withdraw_count,
                            entry->body.zebra_message.withdraw);

            if (entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST] &&
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST]
                  ->prefix_count)
              show_prefixes(
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST]
                  ->prefix_count,
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST]->nlri);

            if (entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST] &&
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST]
                  ->prefix_count)
              show_prefixes(
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST]
                  ->prefix_count,
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST]->nlri);

            if (entry->attr->mp_info
                  ->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST] &&
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST]
                  ->prefix_count)
              show_prefixes(
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST]
                  ->prefix_count,
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST]
                  ->nlri);

            if (entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST] &&
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST]
                  ->prefix_count)
              show_prefixes6(
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST]
                  ->prefix_count,
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST]->nlri);

            if (entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST] &&
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST]
                  ->prefix_count)
              show_prefixes6(
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST]
                  ->prefix_count,
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST]->nlri);

            if (entry->attr->mp_info
                  ->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST] &&
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST]
                  ->prefix_count)
              show_prefixes6(
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST]
                  ->prefix_count,
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST]
                  ->nlri);
          }
          if ((entry->body.zebra_message.announce_count) ||
              (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_REACH_NLRI))) {
            printf("ANNOUNCE\n");
            if (entry->body.zebra_message.announce_count)
              show_prefixes(entry->body.zebra_message.announce_count,
                            entry->body.zebra_message.announce);

            if (entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST] &&
                entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST]
                  ->prefix_count)
              show_prefixes(
                entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST]
                  ->prefix_count,
                entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST]->nlri);

            if (entry->attr->mp_info->announce[AFI_IP][SAFI_MULTICAST] &&
                entry->attr->mp_info->announce[AFI_IP][SAFI_MULTICAST]
                  ->prefix_count)
              show_prefixes(
                entry->attr->mp_info->announce[AFI_IP][SAFI_MULTICAST]
                  ->prefix_count,
                entry->attr->mp_info->announce[AFI_IP][SAFI_MULTICAST]->nlri);

            if (entry->attr->mp_info
                  ->announce[AFI_IP][SAFI_UNICAST_MULTICAST] &&
                entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST]
                  ->prefix_count)
              show_prefixes(
                entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST]
                  ->prefix_count,
                entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST]
                  ->nlri);

            if (entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST] &&
                entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]
                  ->prefix_count)
              show_prefixes6(
                entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]
                  ->prefix_count,
                entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]->nlri);

            if (entry->attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST] &&
                entry->attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST]
                  ->prefix_count)
              show_prefixes6(
                entry->attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST]
                  ->prefix_count,
                entry->attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST]->nlri);

            if (entry->attr->mp_info
                  ->announce[AFI_IP6][SAFI_UNICAST_MULTICAST] &&
                entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST]
                  ->prefix_count)
              show_prefixes6(
                entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST]
                  ->prefix_count,
                entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST]
                  ->nlri);
          }
        } else if (mode == 1 || mode == 2) //-m -M
        {
          if ((entry->body.zebra_message.withdraw_count) ||
              (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_UNREACH_NLRI))) {

            table_line_withdraw(entry->body.zebra_message.withdraw,
                                entry->body.zebra_message.withdraw_count, entry,
                                time_str);

            if (entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST] &&
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST]
                  ->prefix_count)
              table_line_withdraw(
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST]->nlri,
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST]
                  ->prefix_count,
                entry, time_str);

            if (entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST] &&
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST]
                  ->prefix_count)
              table_line_withdraw(
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST]->nlri,
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST]
                  ->prefix_count,
                entry, time_str);

            if (entry->attr->mp_info
                  ->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST] &&
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST]
                  ->prefix_count)
              table_line_withdraw(
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST]
                  ->nlri,
                entry->attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST]
                  ->prefix_count,
                entry, time_str);

            if (entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST] &&
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST]
                  ->prefix_count)
              table_line_withdraw6(
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST]->nlri,
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST]
                  ->prefix_count,
                entry, time_str);

            if (entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST] &&
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST]
                  ->prefix_count)
              table_line_withdraw6(
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST]->nlri,
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST]
                  ->prefix_count,
                entry, time_str);

            if (entry->attr->mp_info
                  ->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST] &&
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST]
                  ->prefix_count)
              table_line_withdraw6(
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST]
                  ->nlri,
                entry->attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST]
                  ->prefix_count,
                entry, time_str);
          }
          if ((entry->body.zebra_message.announce_count) ||
              (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_REACH_NLRI))) {
            table_line_announce(entry->body.zebra_message.announce,
                                entry->body.zebra_message.announce_count, entry,
                                time_str);
            if (entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST] &&
                entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST]
                  ->prefix_count)
              table_line_announce_1(
                entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST],
                entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST]
                  ->prefix_count,
                entry, time_str);
            if (entry->attr->mp_info->announce[AFI_IP][SAFI_MULTICAST] &&
                entry->attr->mp_info->announce[AFI_IP][SAFI_MULTICAST]
                  ->prefix_count)
              table_line_announce_1(
                entry->attr->mp_info->announce[AFI_IP][SAFI_MULTICAST],
                entry->attr->mp_info->announce[AFI_IP][SAFI_MULTICAST]
                  ->prefix_count,
                entry, time_str);
            if (entry->attr->mp_info
                  ->announce[AFI_IP][SAFI_UNICAST_MULTICAST] &&
                entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST]
                  ->prefix_count)
              table_line_announce_1(
                entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST],
                entry->attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST]
                  ->prefix_count,
                entry, time_str);
            if (entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST] &&
                entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]
                  ->prefix_count)
              table_line_announce6(
                entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST],
                entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]
                  ->prefix_count,
                entry, time_str);
            if (entry->attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST] &&
                entry->attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST]
                  ->prefix_count)
              table_line_announce6(
                entry->attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST],
                entry->attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST]
                  ->prefix_count,
                entry, time_str);
            if (entry->attr->mp_info
                  ->announce[AFI_IP6][SAFI_UNICAST_MULTICAST] &&
                entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST]
                  ->prefix_count)
              table_line_announce6(
                entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST],
                entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST]
                  ->prefix_count,
                entry, time_str);
          }
        }
        break;

      case BGP_MSG_OPEN:
        if (mode != 0)
          break;
        printf("TYPE: BGP4MP/MESSAGE/Open\n");
        if (entry->body.zebra_message.source_as) {
          printf("FROM:");
          switch (entry->body.zebra_message.address_family) {
          case AFI_IP6:
            bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, prefix);
            printf(" %s ", prefix);
            break;
          case AFI_IP:
          default:
            if (entry->body.zebra_message.source_ip.v4_addr.s_addr !=
                0x00000000L)
              printf(" %s ",
                     inet_ntoa(entry->body.zebra_message.source_ip.v4_addr));
            else
              printf(" N/A ");
          }
          printf("AS%u\n", entry->body.zebra_message.source_as);
        }
        if (entry->body.zebra_message.destination_as) {
          printf("TO:");
          switch (entry->body.zebra_message.address_family) {
          case AFI_IP6:
            bgpdump_fmt_ipv6(entry->body.zebra_message.destination_ip, prefix);
            printf(" %s ", prefix);
            break;
          case AFI_IP:
          default:
            if (entry->body.zebra_message.destination_ip.v4_addr.s_addr !=
                0x00000000L)
              printf(
                " %s ",
                inet_ntoa(entry->body.zebra_message.destination_ip.v4_addr));
            else
              printf(" N/A ");
          }
          printf("AS%u\n", entry->body.zebra_message.destination_as);
        }

        printf("VERSION: %d\n", entry->body.zebra_message.version);
        printf("AS: %u\n", entry->body.zebra_message.my_as);
        printf("HOLD_TIME: %d\n", entry->body.zebra_message.hold_time);
        printf("ID: %s\n", inet_ntoa(entry->body.zebra_message.bgp_id));
        printf("OPT_PARM_LEN: %d\n", entry->body.zebra_message.opt_len);
        break;

      case BGP_MSG_NOTIFY:
        if (mode != 0)
          break;
        printf("TYPE: BGP4MP/MESSAGE/Notify\n");
        if (entry->body.zebra_message.source_as) {
          printf("FROM:");
          switch (entry->body.zebra_message.address_family) {
          case AFI_IP6:
            bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, prefix);
            printf(" %s ", prefix);
            break;
          case AFI_IP:
          default:
            if (entry->body.zebra_message.source_ip.v4_addr.s_addr !=
                0x00000000L)
              printf(" %s ",
                     inet_ntoa(entry->body.zebra_message.source_ip.v4_addr));
            else
              printf(" N/A ");
          }
          printf("AS%u\n", entry->body.zebra_message.source_as);
        }
        if (entry->body.zebra_message.destination_as) {
          printf("TO:");
          switch (entry->body.zebra_message.address_family) {
          case AFI_IP6:
            bgpdump_fmt_ipv6(entry->body.zebra_message.destination_ip, prefix);
            printf(" %s ", prefix);
            break;
          case AFI_IP:
          default:
            if (entry->body.zebra_message.destination_ip.v4_addr.s_addr !=
                0x00000000L)
              printf(
                " %s ",
                inet_ntoa(entry->body.zebra_message.destination_ip.v4_addr));
            else
              printf(" N/A ");
          }
          printf("AS%u\n", entry->body.zebra_message.destination_as);
        }

        switch (entry->body.zebra_message.error_code) {
        case 1:
          printf("    ERROR CODE  : 1 (Message Header Error)\n");
          switch (entry->body.zebra_message.sub_error_code) {
          case 1:
            printf("    SUB ERROR   : 1 (Connection Not Synchronized)\n");
            break;

          case 2:
            printf("    SUB ERROR   : 2 (Bad Message Length)\n");
            break;

          case 3:
            printf("    SUB ERROR   : 3 (Bad Message Type)\n");
            break;

          default:
            printf("    SUB ERROR   : %d\n",
                   entry->body.zebra_message.sub_error_code);
            break;
          }
          break;
        case 2:
          printf("    ERROR CODE  : 2 (OPEN Message Error)\n");
          switch (entry->body.zebra_message.sub_error_code) {
          case 1:
            printf("    SUB ERROR   : 1 (Unsupported Version Number)\n");
            break;

          case 2:
            printf("    SUB ERROR   : 2 (Bad Peer AS)\n");
            break;

          case 3:
            printf("    SUB ERROR   : 3 (Bad BGP Identifier)\n");
            break;

          case 4:
            printf("    SUB ERROR   : 4 (Unsupported Optional Parameter)\n");
            break;

          case 5:
            printf("    SUB ERROR   : 5 (Authentication Failure)\n");
            break;

          case 6:
            printf("    SUB ERROR   : 6 (Unacceptable Hold Time)\n");
            break;

          default:
            printf("    SUB ERROR   : %d\n",
                   entry->body.zebra_message.sub_error_code);
            break;
          }
          break;
        case 3:
          printf("    ERROR CODE  : 3 (UPDATE Message Error)\n");
          switch (entry->body.zebra_message.sub_error_code) {
          case 1:
            printf("    SUB ERROR   : 1 (Malformed Attribute List)\n");
            break;

          case 2:
            printf("    SUB ERROR   : 2 (Unrecognized Well-known Attribute)\n");
            break;

          case 3:
            printf("    SUB ERROR   : 3 (Missing Well-known Attribute)\n");
            break;

          case 4:
            printf("    SUB ERROR   : 4 (Attribute Flags Error)\n");
            break;

          case 5:
            printf("    SUB ERROR   : 5 (Attribute Length Error)\n");
            break;

          case 6:
            printf("    SUB ERROR   : 6 (Invalid ORIGIN Attribute)\n");
            break;

          case 7:
            printf("    SUB ERROR   : 7 (AS Routing Loop)\n");
            break;

          case 8:
            printf("    SUB ERROR   : 8 (Invalid NEXT-HOP Attribute)\n");
            break;

          case 9:
            printf("    SUB ERROR   : 9 (Optional Attribute Error)\n");
            break;

          case 10:
            printf("    SUB ERROR   : 10 (Invalid Network Field)\n");
            break;

          case 11:
            printf("    SUB ERROR   : 11 (Malformed AS-PATH)\n");
            break;

          default:
            printf("    SUB ERROR   : %d\n",
                   entry->body.zebra_message.sub_error_code);
            break;
          }
          break;
        case 4:
          printf("    ERROR CODE  : 4 (Hold Timer Expired)\n");
          break;
        case 5:
          printf("    ERROR CODE  : 5 (Finite State Machine Error)\n");
          break;
        case 6:
          printf("    ERROR CODE  : 6 (Cease)\n");
          break;
        default:
          printf("    ERROR CODE  : %d\n",
                 entry->body.zebra_message.error_code);
          break;
        }
        break;

      case BGP_MSG_KEEPALIVE:
        if (mode != 0)
          break;

        printf("TYPE: BGP4MP/MESSAGE/Keepalive\n");
        if (entry->body.zebra_message.source_as) {
          printf("FROM:");
          switch (entry->body.zebra_message.address_family) {
          case AFI_IP6:
            bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, prefix);
            printf(" %s ", prefix);
            break;
          case AFI_IP:
          default:
            if (entry->body.zebra_message.source_ip.v4_addr.s_addr !=
                0x00000000L)
              printf(" %s ",
                     inet_ntoa(entry->body.zebra_message.source_ip.v4_addr));
            else
              printf(" N/A ");
          }
          printf("AS%u\n", entry->body.zebra_message.source_as);
        }
        if (entry->body.zebra_message.destination_as) {
          printf("TO:");
          switch (entry->body.zebra_message.address_family) {
          case AFI_IP6:
            bgpdump_fmt_ipv6(entry->body.zebra_message.destination_ip, prefix);
            printf(" %s ", prefix);
            break;
          case AFI_IP:
          default:
            if (entry->body.zebra_message.destination_ip.v4_addr.s_addr !=
                0x00000000L)
              printf(
                " %s ",
                inet_ntoa(entry->body.zebra_message.destination_ip.v4_addr));
            else
              printf(" N/A ");
          }
          printf("AS%u\n", entry->body.zebra_message.destination_as);
        }

        break;
      }
      break;

    case BGPDUMP_SUBTYPE_ZEBRA_BGP_STATE_CHANGE:
    case BGPDUMP_SUBTYPE_ZEBRA_BGP_STATE_CHANGE_AS4:
      if (mode == 0) {
        printf("TYPE: BGP4MP/STATE_CHANGE\n");

        printf("PEER:");
        switch (entry->body.zebra_state_change.address_family) {
        case AFI_IP6:
          bgpdump_fmt_ipv6(entry->body.zebra_state_change.source_ip, prefix);
          printf(" %s ", prefix);
          break;
        case AFI_IP:
        default:
          if (entry->body.zebra_state_change.source_ip.v4_addr.s_addr !=
              0x00000000L)
            printf(" %s ",
                   inet_ntoa(entry->body.zebra_message.source_ip.v4_addr));
          else
            printf(" N/A ");
        }
        // if (entry->body.zebra_message.source_ip.s_addr != 0x00000000L)
        //	printf(" %s ",inet_ntoa(entry->body.zebra_message.source_ip));
        // else
        //	printf(" N/A ");
        printf("AS%u\n", entry->body.zebra_state_change.source_as);

        printf("STATE: %s/%s\n",
               bgp_state_name[entry->body.zebra_state_change.old_state],
               bgp_state_name[entry->body.zebra_state_change.new_state]);
      } else if (mode == 1 || mode == 2) //-m -M
      {
        switch (entry->body.zebra_state_change.address_family) {
        case AFI_IP6:
          bgpdump_fmt_ipv6(entry->body.zebra_state_change.source_ip, prefix);
          if (mode == 1)
            printf("BGP4MP|%ld|STATE|%s|%u|%d|%d\n", entry->time, prefix,
                   entry->body.zebra_state_change.source_as,
                   entry->body.zebra_state_change.old_state,
                   entry->body.zebra_state_change.new_state);
          else
            printf("BGP4MP|%s|STATE|%s|%u|%d|%d\n", time_str, prefix,
                   entry->body.zebra_state_change.source_as,
                   entry->body.zebra_state_change.old_state,
                   entry->body.zebra_state_change.new_state);
          break;
        case AFI_IP:
        default:
          if (mode == 1)
            printf("BGP4MP|%ld|STATE|%s|%u|%d|%d\n", entry->time,
                   inet_ntoa(entry->body.zebra_state_change.source_ip.v4_addr),
                   entry->body.zebra_state_change.source_as,
                   entry->body.zebra_state_change.old_state,
                   entry->body.zebra_state_change.new_state);
          else
            printf("BGP4MP|%s|STATE|%s|%u|%d|%d\n", time_str,
                   inet_ntoa(entry->body.zebra_state_change.source_ip.v4_addr),
                   entry->body.zebra_state_change.source_as,
                   entry->body.zebra_state_change.old_state,
                   entry->body.zebra_state_change.new_state);
          break;
        }
      }
      break;
    }
    break;
  }
  if (mode == 0)
    printf("\n");
}

static void show_attr(attributes_t *attr)
{

  if (attr != NULL) {

    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_ORIGIN)) != 0) {
      switch (attr->origin) {
      case 0:
        printf("ORIGIN: IGP\n");
        break;
      case 1:
        printf("ORIGIN: EGP\n");
        break;
      case 2:
        printf("ORIGIN: INCOMPLETE\n");
      }
    }

    if (!attr->aspath->str) {
      process_attr_aspath_string(attr->aspath, 1);
    }

    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_AS_PATH)) != 0)
      printf("ASPATH: %s\n", attr->aspath->str);

    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_NEXT_HOP)) != 0)
      printf("NEXT_HOP: %s\n", inet_ntoa(attr->nexthop));

    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MULTI_EXIT_DISC)) != 0)
      printf("MULTI_EXIT_DISC: %u\n", attr->med);

    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_LOCAL_PREF)) != 0)
      printf("LOCAL_PREF: %u\n", attr->local_pref);

    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_ATOMIC_AGGREGATE)) != 0)
      printf("ATOMIC_AGGREGATE\n");

    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_AGGREGATOR)) != 0)
      printf("AGGREGATOR: AS%u %s\n", attr->aggregator_as,
             inet_ntoa(attr->aggregator_addr));

    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_ORIGINATOR_ID)) != 0)
      printf("ORIGINATOR_ID: %s\n", inet_ntoa(attr->originator_id));

    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_CLUSTER_LIST)) != 0) {
      int cluster_index;

      printf("CLUSTER_LIST: ");

      for (cluster_index = 0; cluster_index < attr->cluster->length;
           cluster_index++)
        printf("%s ", inet_ntoa(attr->cluster->list[cluster_index]));
      printf("\n");
    }

    int idx;
    for (idx = 0; idx < attr->unknown_num; idx++) {
      struct unknown_attr *unknown = attr->unknown + idx;
      printf("   UNKNOWN_ATTR(%i, %i, %i):", unknown->flag, unknown->type,
             unknown->len);
      int b;
      for (b = 0; b < unknown->len; ++b)
        printf(" %02x", unknown->raw[b]);
      printf("\n");
    }

    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_REACH_NLRI)) != 0) {
      printf("MP_REACH_NLRI");
      if (attr->mp_info->announce[AFI_IP6][SAFI_UNICAST] ||
          attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST] ||
          attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST])

      {
        char buf[128];

        if (attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]) {
          printf("(IPv6 Unicast)\n");
          printf(
            "NEXT_HOP: %s\n",
            bgpdump_fmt_ipv6(
              attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]->nexthop, buf));
          if (attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]->nexthop_len == 32)
            printf(
              "NEXT_HOP: %s\n",
              bgpdump_fmt_ipv6(
                attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]->nexthop_local,
                buf));
        } else if (attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST]) {
          printf("(IPv6 Multicast)\n");
          printf(
            "NEXT_HOP: %s\n",
            bgpdump_fmt_ipv6(
              attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST]->nexthop, buf));
          if (attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST]->nexthop_len ==
              32)
            printf(
              "NEXT_HOP: %s\n",
              bgpdump_fmt_ipv6(
                attr->mp_info->announce[AFI_IP6][SAFI_MULTICAST]->nexthop_local,
                buf));

        } else {
          printf("(IPv6 Both unicast and multicast)\n");
          printf(
            "NEXT_HOP: %s\n",
            bgpdump_fmt_ipv6(
              attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST]->nexthop,
              buf));
          if (attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST]
                ->nexthop_len == 32)
            printf("NEXT_HOP: %s\n",
                   bgpdump_fmt_ipv6(
                     attr->mp_info->announce[AFI_IP6][SAFI_UNICAST_MULTICAST]
                       ->nexthop_local,
                     buf));
        }
      } else {

        if (attr->mp_info->announce[AFI_IP][SAFI_UNICAST]) {
          printf("(IPv4 Unicast)\n");
          printf(
            "NEXT_HOP: %s\n",
            inet_ntoa(
              attr->mp_info->announce[AFI_IP][SAFI_UNICAST]->nexthop.v4_addr));
          if (attr->mp_info->announce[AFI_IP][SAFI_UNICAST]->nexthop_len == 32)
            printf("NEXT_HOP: %s\n",
                   inet_ntoa(attr->mp_info->announce[AFI_IP][SAFI_UNICAST]
                               ->nexthop_local.v4_addr));

        } else if (attr->mp_info->announce[AFI_IP][SAFI_MULTICAST]) {
          printf("(IPv4 Multicast)\n");
          printf("NEXT_HOP: %s\n",
                 inet_ntoa(attr->mp_info->announce[AFI_IP][SAFI_MULTICAST]
                             ->nexthop.v4_addr));
          if (attr->mp_info->announce[AFI_IP][SAFI_MULTICAST]->nexthop_len ==
              32)
            printf("NEXT_HOP: %s\n",
                   inet_ntoa(attr->mp_info->announce[AFI_IP][SAFI_MULTICAST]
                               ->nexthop_local.v4_addr));

        } else if (attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST]) {
          printf("(IPv4 Both unicast and multicast)\n");
          printf(
            "NEXT_HOP: %s\n",
            inet_ntoa(attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST]
                        ->nexthop.v4_addr));
          if (attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST]
                ->nexthop_len == 32)
            printf(
              "NEXT_HOP: %s\n",
              inet_ntoa(attr->mp_info->announce[AFI_IP][SAFI_UNICAST_MULTICAST]
                          ->nexthop_local.v4_addr));
        }
      }
    }

    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_UNREACH_NLRI)) != 0) {
      printf("MP_UNREACH_NLRI");
      if (attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST] ||
          attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST] ||
          attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST_MULTICAST])

      {

        if (attr->mp_info->withdraw[AFI_IP6][SAFI_UNICAST]) {
          printf("(IPv6 Unicast)\n");
        } else if (attr->mp_info->withdraw[AFI_IP6][SAFI_MULTICAST]) {
          printf("(IPv6 Multicast)\n");

        } else {
          printf("(IPv6 Both unicast and multicast)\n");
        }
      } else {

        if (attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST]) {
          printf("(IPv4 Unicast)\n");

        } else if (attr->mp_info->withdraw[AFI_IP][SAFI_MULTICAST]) {
          printf("(IPv4 Multicast)\n");

        } else if (attr->mp_info->withdraw[AFI_IP][SAFI_UNICAST_MULTICAST]) {
          printf("(IPv4 Both unicast and multicast)\n");
        }
      }
    }
    if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES)) != 0) {
      if (attr->community->str == NULL) {
        process_attr_community_string(attr->community);
      }
      printf("COMMUNITY:%s\n", attr->community->str);
    }
  }
}

static void show_prefixes(int count, struct prefix *prefix)
{
  int i;
  for (i = 0; i < count; i++)
    printf("  %s/%d\n", inet_ntoa(prefix[i].address.v4_addr), prefix[i].len);
}

void show_prefixes6(int count, struct prefix *prefix)
{
  int i;
  char buf[128];

  for (i = 0; i < count; i++)
    printf("  %s/%d\n", bgpdump_fmt_ipv6(prefix[i].address, buf),
           prefix[i].len);
}

static void table_line_withdraw(struct prefix *prefix, int count,
                                BGPDUMP_ENTRY *entry, char *time_str)
{
  int idx;
  char buf[128];

  for (idx = 0; idx < count; idx++) {
    if (mode == 1) {
      switch (entry->body.zebra_message.address_family) {
      case AFI_IP6:
        printf("BGP4MP|%ld|W|%s|%u|", entry->time,
               bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, buf),
               entry->body.zebra_message.source_as);
        break;
      case AFI_IP:
      default:
        printf("BGP4MP|%ld|W|%s|%u|", entry->time,
               inet_ntoa(entry->body.zebra_message.source_ip.v4_addr),
               entry->body.zebra_message.source_as);
        break;
      }
      printf("%s/%d\n", inet_ntoa(prefix[idx].address.v4_addr),
             prefix[idx].len);
    } else {
      switch (entry->body.zebra_message.address_family) {
      case AFI_IP6:
        printf("BGP4MP|%s|W|%s|%u|", time_str,
               bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, buf),
               entry->body.zebra_message.source_as);
        break;
      case AFI_IP:
      default:
        printf("BGP4MP|%s|W|%s|%u|", time_str,
               inet_ntoa(entry->body.zebra_message.source_ip.v4_addr),
               entry->body.zebra_message.source_as);
        break;
      }
      printf("%s/%d\n", inet_ntoa(prefix[idx].address.v4_addr),
             prefix[idx].len);
    }
  }
}

static void table_line_withdraw6(struct prefix *prefix, int count,
                                 BGPDUMP_ENTRY *entry, char *time_str)
{
  int idx;
  char buf[128];
  char buf1[128];

  for (idx = 0; idx < count; idx++) {
    if (mode == 1) {
      switch (entry->body.zebra_message.address_family) {
      case AFI_IP6:
        printf("BGP4MP|%ld|W|%s|%u|%s/%d\n", entry->time,
               bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, buf1),
               entry->body.zebra_message.source_as,
               bgpdump_fmt_ipv6(prefix[idx].address, buf), prefix[idx].len);
        break;
      case AFI_IP:
      default:
        printf("BGP4MP|%ld|W|%s|%u|%s/%d\n", entry->time,
               bgpdump_fmt_ipv4(entry->body.zebra_message.source_ip, buf1),
               entry->body.zebra_message.source_as,
               bgpdump_fmt_ipv6(prefix[idx].address, buf), prefix[idx].len);
        break;
      }
    } else {
      switch (entry->body.zebra_message.address_family) {
      case AFI_IP6:
        printf("BGP4MP|%s|W|%s|%u|%s/%d\n", time_str,
               bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, buf1),
               entry->body.zebra_message.source_as,
               bgpdump_fmt_ipv6(prefix[idx].address, buf), prefix[idx].len);
        break;
      case AFI_IP:
      default:
        printf("BGP4MP|%s|W|%s|%u|%s/%d\n", time_str,
               bgpdump_fmt_ipv4(entry->body.zebra_message.source_ip, buf1),
               entry->body.zebra_message.source_as,
               bgpdump_fmt_ipv6(prefix[idx].address, buf), prefix[idx].len);
        break;
      }
    }
  }
}

static void table_line_announce(struct prefix *prefix, int count,
                                BGPDUMP_ENTRY *entry, char *time_str)
{
  int idx;
  char buf[128];
  // char buf1[128];
  // char buf2[128];
  char tmp1[20];
  char tmp2[20];
  unsigned int npref;
  unsigned int nmed;

  switch (entry->attr->origin) {

  case 0:
    sprintf(tmp1, "IGP");
    break;
  case 1:
    sprintf(tmp1, "EGP");
    break;
  case 2:
  default:
    sprintf(tmp1, "INCOMPLETE");
    break;
  }
  if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_ATOMIC_AGGREGATE))
    sprintf(tmp2, "AG");
  else
    sprintf(tmp2, "NAG");

  for (idx = 0; idx < count; idx++) {
    if (mode == 1) {
      switch (entry->body.zebra_message.address_family) {
      case AFI_IP6:
        printf("BGP4MP|%ld|A|%s|%u|", entry->time,
               bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, buf),
               entry->body.zebra_message.source_as);
        break;
      case AFI_IP:
      default:
        printf("BGP4MP|%ld|A|%s|%u|", entry->time,
               inet_ntoa(entry->body.zebra_message.source_ip.v4_addr),
               entry->body.zebra_message.source_as);
        break;
      }
      printf("%s/%d|%s|%s|", inet_ntoa(prefix[idx].address.v4_addr),
             prefix[idx].len, attr_aspath(entry->attr), tmp1);
      npref = entry->attr->local_pref;
      if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_LOCAL_PREF)) == 0)
        npref = 0;
      nmed = entry->attr->med;
      if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MULTI_EXIT_DISC)) == 0)
        nmed = 0;

      printf("%s|%u|%u|", inet_ntoa(entry->attr->nexthop), npref, nmed);
      if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES)) != 0) {
        if (entry->attr->community->str == NULL) {
          process_attr_community_string(entry->attr->community);
        }
        printf("%s|%s|", entry->attr->community->str + 1, tmp2);
      } else
        printf("|%s|", tmp2);

      if (entry->attr->aggregator_addr.s_addr != -1)
        printf("%u %s|\n", entry->attr->aggregator_as,
               inet_ntoa(entry->attr->aggregator_addr));
      else
        printf("|\n");
    } else {
      switch (entry->body.zebra_message.address_family) {
      case AFI_IP6:
        printf("BGP4MP|%s|A|%s|%u|", time_str,
               bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, buf),
               entry->body.zebra_message.source_as);
        break;
      case AFI_IP:
      default:
        printf("BGP4MP|%s|A|%s|%u|", time_str,
               inet_ntoa(entry->body.zebra_message.source_ip.v4_addr),
               entry->body.zebra_message.source_as);
        break;
      }
      printf("%s/%d|%s|%s\n", inet_ntoa(prefix[idx].address.v4_addr),
             prefix[idx].len, attr_aspath(entry->attr), tmp1);
    }
  }
}
static void table_line_announce_1(struct mp_nlri *prefix, int count,
                                  BGPDUMP_ENTRY *entry, char *time_str)
{
  int idx;
  char buf[128];
  // char buf1[128];
  // char buf2[128];
  char tmp1[20];
  char tmp2[20];
  unsigned int npref;
  unsigned int nmed;

  switch (entry->attr->origin) {

  case 0:
    sprintf(tmp1, "IGP");
    break;
  case 1:
    sprintf(tmp1, "EGP");
    break;
  case 2:
  default:
    sprintf(tmp1, "INCOMPLETE");
    break;
  }
  if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_ATOMIC_AGGREGATE))
    sprintf(tmp2, "AG");
  else
    sprintf(tmp2, "NAG");

  for (idx = 0; idx < count; idx++) {
    if (mode == 1) {
      if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_REACH_NLRI)) {
        switch (entry->body.zebra_message.address_family) {
        case AFI_IP6:
          printf("BGP4MP|%ld|A|%s|%u|", entry->time,
                 bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, buf),
                 entry->body.zebra_message.source_as);
          break;
        case AFI_IP:
        default:
          printf("BGP4MP|%ld|A|%s|%u|", entry->time,
                 inet_ntoa(entry->body.zebra_message.source_ip.v4_addr),
                 entry->body.zebra_message.source_as);
          break;
        }
        printf("%s/%d|%s|%s|", inet_ntoa(prefix->nlri[idx].address.v4_addr),
               prefix->nlri[idx].len, attr_aspath(entry->attr), tmp1);

        npref = entry->attr->local_pref;
        if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_LOCAL_PREF)) == 0)
          npref = 0;
        nmed = entry->attr->med;
        if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MULTI_EXIT_DISC)) == 0)
          nmed = 0;

        printf("%s|%d|%d|", inet_ntoa(entry->attr->nexthop), npref, nmed);
        // printf("%s|%d|%d|",inet_ntoa(prefix->nexthop.v4_addr),entry->attr->local_pref,entry->attr->med);
        if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES)) != 0) {
          if (entry->attr->community->str == NULL) {
            process_attr_community_string(entry->attr->community);
          }
          printf("%s|%s|", entry->attr->community->str + 1, tmp2);
        } else
          printf("|%s|", tmp2);

      } else {
        switch (entry->body.zebra_message.address_family) {
        case AFI_IP6:
          printf("BGP4MP|%ld|A|%s|%u|", entry->time,
                 bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, buf),
                 entry->body.zebra_message.source_as);
          break;
        case AFI_IP:
        default:
          printf("BGP4MP|%ld|A|%s|%u|", entry->time,
                 inet_ntoa(entry->body.zebra_message.source_ip.v4_addr),
                 entry->body.zebra_message.source_as);
          break;
        }
        printf("%s/%d|%s|%s|", inet_ntoa(prefix->nlri[idx].address.v4_addr),
               prefix->nlri[idx].len, attr_aspath(entry->attr), tmp1);

        npref = entry->attr->local_pref;
        if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_LOCAL_PREF)) == 0)
          npref = 0;
        nmed = entry->attr->med;
        if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MULTI_EXIT_DISC)) == 0)
          nmed = 0;

        printf("%s|%d|%d|", inet_ntoa(entry->attr->nexthop), npref, nmed);
        // printf("%s|%d|%d|",inet_ntoa(entry->attr->nexthop),entry->attr->local_pref,entry->attr->med);
        if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES)) != 0) {
          if (entry->attr->community->str == NULL) {
            process_attr_community_string(entry->attr->community);
          }
          printf("%s|%s|", entry->attr->community->str + 1, tmp2);
        } else
          printf("|%s|", tmp2);
      }
      if (entry->attr->aggregator_addr.s_addr != -1)
        printf("%u %s|\n", entry->attr->aggregator_as,
               inet_ntoa(entry->attr->aggregator_addr));
      else
        printf("|\n");
    } else {
      switch (entry->body.zebra_message.address_family) {
      case AFI_IP6:
        printf("BGP4MP|%s|A|%s|%u|", time_str,
               bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, buf),
               entry->body.zebra_message.source_as);
        break;
      case AFI_IP:
      default:
        printf("BGP4MP|%s|A|%s|%u|", time_str,
               inet_ntoa(entry->body.zebra_message.source_ip.v4_addr),
               entry->body.zebra_message.source_as);
        break;
      }
      printf("%s/%d|%s|%s\n", inet_ntoa(prefix->nlri[idx].address.v4_addr),
             prefix->nlri[idx].len, attr_aspath(entry->attr), tmp1);
    }
  }
}

static void table_line_announce6(struct mp_nlri *prefix, int count,
                                 BGPDUMP_ENTRY *entry, char *time_str)
{
  int idx;
  char buf[128];
  char buf1[128];
  char buf2[128];
  char tmp1[20];
  char tmp2[20];
  unsigned int npref;
  unsigned int nmed;

  switch (entry->attr->origin) {

  case 0:
    sprintf(tmp1, "IGP");
    break;
  case 1:
    sprintf(tmp1, "EGP");
    break;
  case 2:
  default:
    sprintf(tmp1, "INCOMPLETE");
    break;
  }
  if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_ATOMIC_AGGREGATE))
    sprintf(tmp2, "AG");
  else
    sprintf(tmp2, "NAG");

  for (idx = 0; idx < count; idx++) {
    if (mode == 1) {
      switch (entry->body.zebra_message.address_family) {
      case AFI_IP6:

        npref = entry->attr->local_pref;
        if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_LOCAL_PREF)) == 0)
          npref = 0;
        nmed = entry->attr->med;
        if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MULTI_EXIT_DISC)) == 0)
          nmed = 0;

        printf("BGP4MP|%ld|A|%s|%u|%s/%d|%s|%s|%s|%u|%u|", entry->time,
               bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, buf1),
               entry->body.zebra_message.source_as,
               bgpdump_fmt_ipv6(prefix->nlri[idx].address, buf2),
               prefix->nlri[idx].len, attr_aspath(entry->attr), tmp1,
               bgpdump_fmt_ipv6(prefix->nexthop, buf), npref, nmed);
        break;
      case AFI_IP:
      default:

        npref = entry->attr->local_pref;
        if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_LOCAL_PREF)) == 0)
          npref = 0;
        nmed = entry->attr->med;
        if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MULTI_EXIT_DISC)) == 0)
          nmed = 0;

        // printf("%s|%d|%d|",inet_ntoa(entry->attr->nexthop),nprof,nmed);
        printf("BGP4MP|%ld|A|%s|%u|%s/%d|%s|%s|%s|%u|%u|", entry->time,
               bgpdump_fmt_ipv4(entry->body.zebra_message.source_ip, buf1),
               entry->body.zebra_message.source_as,
               bgpdump_fmt_ipv6(prefix->nlri[idx].address, buf2),
               prefix->nlri[idx].len, attr_aspath(entry->attr), tmp1,
               bgpdump_fmt_ipv6(prefix->nexthop, buf), npref, nmed);
        break;
      }
      if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES)) != 0) {
        if (entry->attr->community->str == NULL) {
          process_attr_community_string(entry->attr->community);
        }
        printf("%s|%s|", entry->attr->community->str + 1, tmp2);
      } else
        printf("|%s|", tmp2);

      if (entry->attr->aggregator_addr.s_addr != -1)
        printf("%u %s|\n", entry->attr->aggregator_as,
               inet_ntoa(entry->attr->aggregator_addr));
      else
        printf("|\n");

    } else {
      switch (entry->body.zebra_message.address_family) {
      case AFI_IP6:
        printf("BGP4MP|%s|A|%s|%u|%s/%d|%s|%s\n", time_str,
               bgpdump_fmt_ipv6(entry->body.zebra_message.source_ip, buf1),
               entry->body.zebra_message.source_as,
               bgpdump_fmt_ipv6(prefix->nlri[idx].address, buf),
               prefix->nlri[idx].len, attr_aspath(entry->attr), tmp1);
        break;
      case AFI_IP:
      default:
        printf("BGP4MP|%s|A|%s|%u|%s/%d|%s|%s\n", time_str,
               bgpdump_fmt_ipv4(entry->body.zebra_message.source_ip, buf1),
               entry->body.zebra_message.source_as,
               bgpdump_fmt_ipv6(prefix->nlri[idx].address, buf),
               prefix->nlri[idx].len, attr_aspath(entry->attr), tmp1);
        break;
      }
    }
  }
}

static void table_line_mrtd_route(BGPDUMP_MRTD_TABLE_DUMP *route,
                                  BGPDUMP_ENTRY *entry)
{

  struct tm *date = NULL;
  char tmp1[20];
  char tmp2[20];
  unsigned int npref;
  unsigned int nmed;
  char time_str[20];
  char peer[BGPDUMP_ADDRSTRLEN], prefix[BGPDUMP_ADDRSTRLEN],
    nexthop[BGPDUMP_ADDRSTRLEN];

  switch (entry->attr->origin) {

  case 0:
    sprintf(tmp1, "IGP");
    break;
  case 1:
    sprintf(tmp1, "EGP");
    break;
  case 2:
  default:
    sprintf(tmp1, "INCOMPLETE");
    break;
  }
  if (entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_ATOMIC_AGGREGATE))
    sprintf(tmp2, "AG");
  else
    sprintf(tmp2, "NAG");

  if (entry->subtype == AFI_IP6) {
    bgpdump_fmt_ipv6(route->peer_ip, peer);
    bgpdump_fmt_ipv6(route->prefix, prefix);
  } else {
    strncpy(peer, inet_ntoa(route->peer_ip.v4_addr), BGPDUMP_ADDRSTRLEN);
    strncpy(prefix, inet_ntoa(route->prefix.v4_addr), BGPDUMP_ADDRSTRLEN);
  }

  if (mode == 1) {
    if (timetype == 0) {
      printf("TABLE_DUMP|%ld|B|%s|%u|", entry->time, peer, route->peer_as);
    } else if (timetype == 1) {
      printf("TABLE_DUMP|%ld|B|%s|%u|", route->uptime, peer, route->peer_as);
    }
    printf("%s/%d|%s|%s|", prefix, route->mask, attr_aspath(entry->attr), tmp1);

    npref = entry->attr->local_pref;
    if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_LOCAL_PREF)) == 0)
      npref = 0;
    nmed = entry->attr->med;
    if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MULTI_EXIT_DISC)) == 0)
      nmed = 0;

    if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_REACH_NLRI)) &&
        entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]) {
      bgpdump_fmt_ipv6(
        entry->attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]->nexthop,
        nexthop);
    } else {
      strncpy(nexthop, inet_ntoa(entry->attr->nexthop), BGPDUMP_ADDRSTRLEN);
    }
    printf("%s|%u|%u|", nexthop, npref, nmed);

    if ((entry->attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES)) != 0) {
      if (entry->attr->community->str == NULL) {
        process_attr_community_string(entry->attr->community);
      }
      printf("%s|%s|", entry->attr->community->str + 1, tmp2);
    } else
      printf("|%s|", tmp2);

    if (entry->attr->aggregator_addr.s_addr != -1)
      printf("%u %s|\n", entry->attr->aggregator_as,
             inet_ntoa(entry->attr->aggregator_addr));
    else
      printf("|\n");
  } else {
    if (timetype == 0) {
      date = gmtime(&entry->time);
    } else if (timetype == 1) {
      date = gmtime(&route->uptime);
    }
    bgpdump_time2str(date, time_str);
    printf("TABLE_DUMP|%s|A|%s|%u|", time_str, peer, route->peer_as);
    printf("%s/%d|%s|%s\n", prefix, route->mask, attr_aspath(entry->attr),
           tmp1);
  }
}

static char *describe_origin(int origin)
{
  if (origin == 0)
    return "IGP";
  if (origin == 1)
    return "EGP";
  return "INCOMPLETE";
}

static void table_line_dump_v2_prefix(BGPDUMP_TABLE_DUMP_V2_PREFIX *e,
                                      BGPDUMP_ENTRY *entry)
{
  struct tm *date = NULL;
  unsigned int npref;
  unsigned int nmed;
  char time_str[20];
  char peer[BGPDUMP_ADDRSTRLEN], prefix[BGPDUMP_ADDRSTRLEN],
    nexthop[BGPDUMP_ADDRSTRLEN];

  int i;

  for (i = 0; i < e->entry_count; i++) {
    attributes_t *attr = e->entries[i].attr;
    if (!attr)
      continue;

    char *origin = describe_origin(attr->origin);
    if (attr->aspath && !attr->aspath->str) { // AK HAX
      process_attr_aspath_string(attr->aspath, 1);
    }
    char *aspath_str = (attr->aspath) ? attr->aspath->str : "";
    char *aggregate =
      attr->flag & ATTR_FLAG_BIT(BGP_ATTR_ATOMIC_AGGREGATE) ? "AG" : "NAG";

    if (e->entries[i].peer.afi == AFI_IP) {
      bgpdump_fmt_ipv4(e->entries[i].peer.peer_ip, peer);
    } else if (e->entries[i].peer.afi == AFI_IP6) {
      bgpdump_fmt_ipv6(e->entries[i].peer.peer_ip, peer);
    }

    if (e->afi == AFI_IP) {
      bgpdump_fmt_ipv4(e->prefix, prefix);
    } else if (e->afi == AFI_IP6) {
      bgpdump_fmt_ipv6(e->prefix, prefix);
    }

    if (mode == 1) {
      if (timetype == 0) {
        printf("TABLE_DUMP2|%ld|B|%s|%u|", entry->time, peer,
               e->entries[i].peer.peer_as);
      } else if (timetype == 1) {
        printf("TABLE_DUMP2|%u|B|%s|%u|", e->entries[i].originated_time, peer,
               e->entries[i].peer.peer_as);
      }
      printf("%s/%d|%s|%s|", prefix, e->prefix_length, aspath_str, origin);

      npref = attr->local_pref;
      if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_LOCAL_PREF)) == 0)
        npref = 0;
      nmed = attr->med;
      if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MULTI_EXIT_DISC)) == 0)
        nmed = 0;

      if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_MP_REACH_NLRI)) &&
          attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]) {
        bgpdump_fmt_ipv6(
          attr->mp_info->announce[AFI_IP6][SAFI_UNICAST]->nexthop, nexthop);
      } else {
        strncpy(nexthop, inet_ntoa(attr->nexthop), BGPDUMP_ADDRSTRLEN);
      }
      printf("%s|%u|%u|", nexthop, npref, nmed);

      if ((attr->flag & ATTR_FLAG_BIT(BGP_ATTR_COMMUNITIES)) != 0) {
        if (attr->community->str == NULL) {
          process_attr_community_string(attr->community);
        }
        printf("%s|%s|", attr->community->str + 1, aggregate);
      } else
        printf("|%s|", aggregate);

      if (attr->aggregator_addr.s_addr != -1)
        printf("%u %s|\n", attr->aggregator_as,
               inet_ntoa(attr->aggregator_addr));
      else
        printf("|\n");
    } else {
      if (timetype == 0) {
        date = gmtime(&entry->time);
      } else if (timetype == 1) {
        time_t time_temp = (time_t)((e->entries[i]).originated_time);
        date = gmtime(&time_temp);
      }
      bgpdump_time2str(date, time_str);
      printf("TABLE_DUMP_V2|%s|A|%s|%u|", time_str, peer,
             e->entries[i].peer.peer_as);
      printf("%s/%d|%s|%s\n", prefix, e->prefix_length, aspath_str, origin);
    }
  }
}
