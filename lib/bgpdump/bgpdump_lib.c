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
*/

#include "config.h"

#include "bgpdump_cfile_tools.h"
#include "bgpdump_lib.h"
#include "bgpdump_mstream.h"
#include "bgpdump_util.h"

#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <assert.h>
#include <zlib.h>

void bgpdump_free_attr(attributes_t *attr);
static int process_mrtd_table_dump(struct mstream *s, BGPDUMP_ENTRY *entry);
static int process_mrtd_table_dump_v2(struct mstream *s, BGPDUMP_ENTRY *entry);
static int process_mrtd_table_dump_v2_peer_index_table(struct mstream *s,
                                                       BGPDUMP_ENTRY *entry);
static int process_mrtd_table_dump_v2_ipv4_unicast(struct mstream *s,
                                                   BGPDUMP_ENTRY *entry);
static int process_mrtd_table_dump_v2_ipv6_unicast(struct mstream *s,
                                                   BGPDUMP_ENTRY *entry);
static int process_zebra_bgp(struct mstream *s, BGPDUMP_ENTRY *entry);
static int process_zebra_bgp_state_change(struct mstream *s,
                                          BGPDUMP_ENTRY *entry,
                                          u_int8_t asn_len);

static int process_zebra_bgp_message(struct mstream *s, BGPDUMP_ENTRY *entry,
                                     u_int8_t asn_len);
static int process_zebra_bgp_message_update(struct mstream *s,
                                            BGPDUMP_ENTRY *entry,
                                            u_int8_t asn_len);
static int process_zebra_bgp_message_open(struct mstream *s,
                                          BGPDUMP_ENTRY *entry,
                                          u_int8_t asn_len);
static int process_zebra_bgp_message_notify(struct mstream *s,
                                            BGPDUMP_ENTRY *entry);

static int process_zebra_bgp_entry(struct mstream *s, BGPDUMP_ENTRY *entry);
static int process_zebra_bgp_snapshot(struct mstream *s, BGPDUMP_ENTRY *entry);

static attributes_t *process_attributes(struct mstream *s, u_int8_t asn_len,
                                        struct zebra_incomplete *incomplete);

static char aspath_delimiter_char(u_char type, u_char which);

static void process_mp_announce(struct mstream *s, struct mp_info *info,
                                struct zebra_incomplete *incomplete);
static void process_mp_withdraw(struct mstream *s, struct mp_info *info,
                                struct zebra_incomplete *incomplete);
static int read_prefix_list(struct mstream *s, u_int16_t af,
                            struct prefix *prefixes,
                            struct zebra_incomplete *incomplete);

static as_t read_asn(struct mstream *s, as_t *asn, u_int8_t len);
static struct aspath *create_aspath(u_int16_t len, u_int8_t asn_len);
static void aspath_error(struct aspath *as);
static int check_new_aspath(struct aspath *aspath);
static void process_asn32_trans(attributes_t *attr, u_int8_t asn_len);
static struct aspath *asn32_merge_paths(struct aspath *path,
                                        struct aspath *newpath);
static void asn32_expand_16_to_32(char *dst, char *src, int len);

// comments here
// BGPDUMP_TABLE_DUMP_V2_PEER_INDEX_TABLE *table_dump_v2_peer_index_table =
// NULL;

#if defined(linux)
static size_t strlcat(char *dst, const char *src, size_t size);
#endif

char *bgpdump_version(void)
{
  return PACKAGE_VERSION;
}

BGPDUMP *bgpdump_open_dump(const char *filename)
{

  CFRFILE *f = cfr_open(filename);
  if (!f) {
    fprintf(stderr, "Cannot open dumpfile %s: ", filename);
    perror("");
    return NULL;
  }

  BGPDUMP *this_dump = malloc(sizeof(BGPDUMP));
  strcpy(this_dump->filename, "[STDIN]");
  if (filename && strcmp(filename, "-")) {
    strcpy(this_dump->filename, filename);
  }

  this_dump->f = f;
  this_dump->eof = 0;
  this_dump->parsed = 0;
  this_dump->parsed_ok = 0;
  this_dump->corrupted_read = false;

  // peer index table shared among entries
  this_dump->table_dump_v2_peer_index_table = NULL;

  return this_dump;
}

void bgpdump_close_dump(BGPDUMP *dump)
{

  if (dump == NULL) {
    return;
  }

  // destroy current index table, if any
  if (dump->table_dump_v2_peer_index_table) {
    if (dump->table_dump_v2_peer_index_table->entries) {
      free(dump->table_dump_v2_peer_index_table->entries);
      dump->table_dump_v2_peer_index_table->entries = NULL;
    }
    free(dump->table_dump_v2_peer_index_table);
    dump->table_dump_v2_peer_index_table = NULL;
  }
  cfr_close(dump->f);
  free(dump);
}

BGPDUMP_ENTRY *bgpdump_entry_create(BGPDUMP *dump)
{
  BGPDUMP_ENTRY *this_entry = malloc(sizeof(BGPDUMP_ENTRY));
  memset(this_entry, 0, sizeof(BGPDUMP_ENTRY));
  this_entry->dump = dump;
  return this_entry;
}

BGPDUMP_ENTRY *bgpdump_read_next(BGPDUMP *dump)
{

  assert(dump);

  struct mstream s;
  u_char *buffer;
  int ok = 0;
  u_int32_t bytes_read;

  BGPDUMP_ENTRY *this_entry = bgpdump_entry_create(dump);

  // initialize corrupted_read
  dump->corrupted_read = false;

  assert(this_entry);

  bytes_read = cfr_read_n(dump->f, &(this_entry->time), 4);
  bytes_read += cfr_read_n(dump->f, &(this_entry->type), 2);
  bytes_read += cfr_read_n(dump->f, &(this_entry->subtype), 2);
  bytes_read += cfr_read_n(dump->f, &(this_entry->length), 4);
  if (bytes_read != 12) {
    if (bytes_read > 0) {
      /* Malformed record */
      dump->parsed++;
      bgpdump_err("bgpdump_read_next: %s incomplete MRT header (%d bytes read, "
                  "expecting 12)",
                  dump->filename, bytes_read);
      dump->corrupted_read = true;
    }
    /* Nothing more to read, quit */
    bgpdump_free_mem(this_entry);
    this_entry = NULL;
    dump->eof = 1;
    // printf("case 1\n");
    return (NULL);
  }

  dump->parsed++;

  /* Intel byte ordering stuff ... */
  this_entry->type = ntohs(this_entry->type);
  this_entry->subtype = ntohs(this_entry->subtype);
  this_entry->time = ntohl(this_entry->time);
  this_entry->length = ntohl(this_entry->length);
  this_entry->attr = NULL;
  buffer = malloc(this_entry->length);
  bytes_read = cfr_read_n(dump->f, buffer, this_entry->length);
  if (bytes_read != this_entry->length) {
    bgpdump_err("bgpdump_read_next: %s incomplete dump record (%d bytes read, "
                "expecting %d)",
                dump->filename, bytes_read, this_entry->length);
    dump->corrupted_read = true;
    // printf("case 2\n");
    bgpdump_free_mem(this_entry);
    this_entry = NULL;
    free(buffer);
    dump->eof = 1;
    return (NULL);
  }

  ok = 0;
  mstream_init(&s, buffer, this_entry->length);

  switch (this_entry->type) {
  case BGPDUMP_TYPE_MRTD_BGP:
    break;
  case BGPDUMP_TYPE_MRTD_TABLE_DUMP:
    ok = process_mrtd_table_dump(&s, this_entry);
    break;
  case BGPDUMP_TYPE_ZEBRA_BGP:
    ok = process_zebra_bgp(&s, this_entry);
    break;
  case BGPDUMP_TYPE_TABLE_DUMP_V2:
    ok = process_mrtd_table_dump_v2(&s, this_entry);
    break;
  }

  free(buffer);
  if (ok > 0) {
    dump->parsed_ok++;
  } else if (ok < 0) {
    // corrupted
    dump->corrupted_read = true;
    bgpdump_free_mem(this_entry);
    this_entry = NULL;
    dump->eof = 1;
    return NULL;
  } else {
    // printf("case 3 - not corrupted, just empty beginning\n");
    bgpdump_free_mem(this_entry);
    this_entry = NULL;
    return NULL;
  }
  return this_entry;
}

static void bgpdump_free_mp_info(struct mp_info *info)
{
  u_int16_t afi;
  u_int8_t safi;

  for (afi = 1; afi <= BGPDUMP_MAX_AFI; afi++) {
    for (safi = 1; safi < BGPDUMP_MAX_SAFI; safi++) {
      if (info->announce[afi][safi])
        free(info->announce[afi][safi]);
      info->announce[afi][safi] = NULL;
      if (info->withdraw[afi][safi]) {
        free(info->withdraw[afi][safi]);
        info->withdraw[afi][safi] = NULL;
      }
    }
  }

  free(info);
}

void bgpdump_free_mem(BGPDUMP_ENTRY *entry)
{

  if (entry != NULL) {
    bgpdump_free_attr(entry->attr);
    entry->attr = NULL;
    switch (entry->type) {
    case BGPDUMP_TYPE_ZEBRA_BGP:
      switch (entry->subtype) {
      case BGPDUMP_SUBTYPE_ZEBRA_BGP_MESSAGE:
        switch (entry->body.zebra_message.type) {
        case BGP_MSG_NOTIFY:
          if (entry->body.zebra_message.notify_data)
            free(entry->body.zebra_message.notify_data);
          break;
        case BGP_MSG_OPEN:
          if (entry->body.zebra_message.opt_data)
            free(entry->body.zebra_message.opt_data);
          break;
        }
        break;
      }
      break;
    case BGPDUMP_TYPE_TABLE_DUMP_V2:
      if (entry->subtype == BGPDUMP_SUBTYPE_TABLE_DUMP_V2_RIB_IPV4_UNICAST ||
          entry->subtype == BGPDUMP_SUBTYPE_TABLE_DUMP_V2_RIB_IPV6_UNICAST) {

        BGPDUMP_TABLE_DUMP_V2_PREFIX *e;
        e = &entry->body.mrtd_table_dump_v2_prefix;
        int i;

        for (i = 0; i < e->entry_count; i++) {
          bgpdump_free_attr(e->entries[i].attr);
          e->entries[i].attr = NULL;
        }
        free(e->entries);
      }
      break;
    }

    entry->dump = NULL;
    free(entry);
  }
}

void bgpdump_free_attr(attributes_t *attr)
{
  if (attr != NULL) {
    u_int16_t i;
    struct aspath *path,
      *pathstofree[3] = {attr->aspath, attr->old_aspath, attr->new_aspath};
    for (i = 0; i < (sizeof(pathstofree) / sizeof(pathstofree[0])); i++) {
      path = pathstofree[i];
      if (path) {
        if (path->data)
          free(path->data);
        if (path->str) {
          free(path->str);
        }
        free(path);
      }
    }

    if (attr->community != NULL) {
      if (attr->community->val != NULL) {
        free(attr->community->val);
        attr->community->val = NULL;
      }

      if (attr->community->str != NULL) {
        free(attr->community->str);
        attr->community->str = NULL;
      }

      free(attr->community);
      attr->community = NULL;
    }

    if (attr->data != NULL)
      free(attr->data);

    if (attr->mp_info != NULL)
      bgpdump_free_mp_info(attr->mp_info);

    if (attr->cluster != NULL) {
      free(attr->cluster->list);
      free(attr->cluster);
    }

    if (attr->unknown_num) {
      for (i = 0; i < attr->unknown_num; i++)
        free(attr->unknown[i].raw);
      free(attr->unknown);
    }

    free(attr);
  }
}

int process_mrtd_table_dump(struct mstream *s, BGPDUMP_ENTRY *entry)
{
  int afi = entry->subtype;
  u_int8_t asn_len;
  u_int32_t temp_time = 0;
  mstream_getw(s, &entry->body.mrtd_table_dump.view);
  mstream_getw(s, &entry->body.mrtd_table_dump.sequence);
  switch (afi) {
  case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP:
  case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP_32BIT_AS:
    entry->body.mrtd_table_dump.prefix.v4_addr = mstream_get_ipv4(s);
    break;
  case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP6:
  case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP6_32BIT_AS:
    mstream_get(s, &entry->body.mrtd_table_dump.prefix.v6_addr.s6_addr, 16);
    break;
  default:
    bgpdump_warn("process_mrtd_table_dump: unknown AFI %d", afi);
    mstream_get(s, NULL, mstream_can_read(s));
    return 0;
  }
  mstream_getc(s, &entry->body.mrtd_table_dump.mask);
  mstream_getc(s, &entry->body.mrtd_table_dump.status);
  mstream_getl(s, &temp_time);
  (entry->body).mrtd_table_dump.uptime = temp_time;

  switch (afi) {
  case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP:
  case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP_32BIT_AS:
    entry->body.mrtd_table_dump.peer_ip.v4_addr = mstream_get_ipv4(s);
    break;
  case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP6:
  case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP6_32BIT_AS:
    mstream_get(s, &entry->body.mrtd_table_dump.peer_ip.v6_addr.s6_addr, 16);
    break;
  }

  switch (afi) {
  case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP:
  case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP6:
    asn_len = ASN16_LEN;
    break;
  case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP_32BIT_AS:
  case BGPDUMP_SUBTYPE_MRTD_TABLE_DUMP_AFI_IP6_32BIT_AS:
    asn_len = ASN32_LEN;
    break;
  default:
    assert(0); // unreachable
    return 0;
  }

  read_asn(s, &entry->body.mrtd_table_dump.peer_as, asn_len);

  if ((entry->attr = process_attributes(s, asn_len, NULL)) == NULL) {
    return -1;
  }

  return 1;
}

int process_mrtd_table_dump_v2(struct mstream *s, BGPDUMP_ENTRY *entry)
{

  switch (entry->subtype) {
  case BGPDUMP_SUBTYPE_TABLE_DUMP_V2_PEER_INDEX_TABLE:
    return process_mrtd_table_dump_v2_peer_index_table(s, entry);
    break;
  case BGPDUMP_SUBTYPE_TABLE_DUMP_V2_RIB_IPV4_UNICAST:
    return process_mrtd_table_dump_v2_ipv4_unicast(s, entry);
    break;
  case BGPDUMP_SUBTYPE_TABLE_DUMP_V2_RIB_IPV6_UNICAST:
    return process_mrtd_table_dump_v2_ipv6_unicast(s, entry);
    break;
  case BGPDUMP_SUBTYPE_TABLE_DUMP_V2_RIB_GENERIC:
    // return process_mrtd_table_dump_v2_generic(s, entry);
    break;
  }

  return 0;
}

int process_mrtd_table_dump_v2_peer_index_table(struct mstream *s,
                                                BGPDUMP_ENTRY *entry)
{

  assert(entry->dump);

  BGPDUMP_TABLE_DUMP_V2_PEER_INDEX_TABLE *t;
  uint16_t i;
  uint8_t peertype;
  uint16_t view_name_len;

  if (entry->dump->table_dump_v2_peer_index_table) {
    if (entry->dump->table_dump_v2_peer_index_table->entries)
      free(entry->dump->table_dump_v2_peer_index_table->entries);
    free(entry->dump->table_dump_v2_peer_index_table);
  }

  entry->dump->table_dump_v2_peer_index_table =
    malloc(sizeof(BGPDUMP_TABLE_DUMP_V2_PEER_INDEX_TABLE));
  t = entry->dump->table_dump_v2_peer_index_table;
  t->entries = NULL;

  t->local_bgp_id = mstream_get_ipv4(s);

  mstream_getw(s, &view_name_len);
  strcpy(t->view_name, "");

  // view_name_len is without trailing \0
  if (view_name_len + 1 > BGPDUMP_TYPE_TABLE_DUMP_V2_MAX_VIEWNAME_LEN) {
    bgpdump_warn("process_mrtd_table_dump_v2_peer_index_table: view name "
                 "length more than maximum length (%d), ignoring view name",
                 BGPDUMP_TYPE_TABLE_DUMP_V2_MAX_VIEWNAME_LEN);
  } else {
    mstream_get(s, t->view_name, view_name_len);
    t->view_name[view_name_len] = 0;
  }

  mstream_getw(s, &t->peer_count);

  t->entries = malloc(sizeof(BGPDUMP_TABLE_DUMP_V2_PEER_INDEX_TABLE_ENTRY) *
                      t->peer_count);
  if (t->entries == NULL) {
    bgpdump_err("process_mrtd_table_dump_v2_peer_index_table: failed to "
                "allocate memory for index table");
    return -1;
  }

  for (i = 0; i < t->peer_count; i++) {
    mstream_getc(s, &peertype);
    t->entries[i].afi = AFI_IP;
    if (peertype & BGPDUMP_PEERTYPE_TABLE_DUMP_V2_AFI_IP6)
      t->entries[i].afi = AFI_IP6;

    t->entries[i].peer_bgp_id = mstream_get_ipv4(s);

    if (t->entries[i].afi == AFI_IP)
      t->entries[i].peer_ip.v4_addr = mstream_get_ipv4(s);
    else
      mstream_get(s, &t->entries[i].peer_ip.v6_addr.s6_addr, 16);

    if (peertype & BGPDUMP_PEERTYPE_TABLE_DUMP_V2_AS4)
      read_asn(s, &t->entries[i].peer_as, 4);
    else
      read_asn(s, &t->entries[i].peer_as, 2);
  }
  return 0;
}

int process_mrtd_table_dump_v2_ipv4_unicast(struct mstream *s,
                                            BGPDUMP_ENTRY *entry)
{
  BGPDUMP_TABLE_DUMP_V2_PREFIX *prefixdata;
  prefixdata = &entry->body.mrtd_table_dump_v2_prefix;
  uint16_t i;

  /* in case there is a corrupt dump that does not include the peer table... */
  if (entry->dump->table_dump_v2_peer_index_table == NULL) {
    bgpdump_err(
      "process_mrtd_table_dump_v2_ipv4_unicast: missing peer index table");
    return -1;
  }

  prefixdata->afi = AFI_IP;
  prefixdata->safi = SAFI_UNICAST;

  mstream_getl(s, &prefixdata->seq);
  mstream_getc(s, &prefixdata->prefix_length);
  bzero(&prefixdata->prefix.v4_addr.s_addr, 4);
  mstream_get(s, &prefixdata->prefix.v4_addr.s_addr,
              (prefixdata->prefix_length + 7) / 8);
  mstream_getw(s, &prefixdata->entry_count);

  prefixdata->entries =
    calloc(prefixdata->entry_count, sizeof(BGPDUMP_TABLE_DUMP_V2_ROUTE_ENTRY));
  if (prefixdata->entries == NULL) {
    bgpdump_err("process_mrtd_table_dump_v2_ipv4_unicast: failed to allocate "
                "memory for entry table");
    return -1;
  }

  for (i = 0; i < prefixdata->entry_count; i++) {
    BGPDUMP_TABLE_DUMP_V2_ROUTE_ENTRY *e;
    e = &prefixdata->entries[i];

    mstream_getw(s, &e->peer_index);
    e->peer =
      entry->dump->table_dump_v2_peer_index_table->entries[e->peer_index];
    mstream_getl(s, &e->originated_time);

    if((e->attr = process_attributes(s, 4, NULL)) == NULL) {
      bgpdump_err("process_attributes failed");
      return -1;
    }
  }

  return 1;
}

int process_mrtd_table_dump_v2_ipv6_unicast(struct mstream *s,
                                            BGPDUMP_ENTRY *entry)
{
  BGPDUMP_TABLE_DUMP_V2_PREFIX *prefixdata;
  prefixdata = &entry->body.mrtd_table_dump_v2_prefix;
  uint16_t i;

  /* in case there is a corrupt dump that does not include the peer table... */
  if (entry->dump->table_dump_v2_peer_index_table == NULL) {
    bgpdump_err(
      "process_mrtd_table_dump_v2_ipv6_unicast: missing peer index table");
    return -1;
  }

  prefixdata->afi = AFI_IP6;
  prefixdata->safi = SAFI_UNICAST;

  mstream_getl(s, &prefixdata->seq);

  mstream_getc(s, &prefixdata->prefix_length);
  bzero(&prefixdata->prefix.v6_addr.s6_addr, 16);
  mstream_get(s, &prefixdata->prefix.v6_addr.s6_addr,
              (prefixdata->prefix_length + 7) / 8);

  mstream_getw(s, &prefixdata->entry_count);

  prefixdata->entries =
    malloc(sizeof(BGPDUMP_TABLE_DUMP_V2_ROUTE_ENTRY) * prefixdata->entry_count);
  if (prefixdata->entries == NULL) {
    bgpdump_err("process_mrtd_table_dump_v2_ipv6_unicast: failed to allocate "
                "memory for entry table");
    return -1;
  }

  for (i = 0; i < prefixdata->entry_count; i++) {
    BGPDUMP_TABLE_DUMP_V2_ROUTE_ENTRY *e;
    e = &prefixdata->entries[i];

    mstream_getw(s, &e->peer_index);
    e->peer =
      entry->dump->table_dump_v2_peer_index_table->entries[e->peer_index];
    mstream_getl(s, &e->originated_time);

    if ((e->attr = process_attributes(s, 4, NULL)) == NULL) {
      return -1;
    }
  }

  return 1;
}

int process_zebra_bgp(struct mstream *s, BGPDUMP_ENTRY *entry)
{
  switch (entry->subtype) {
  case BGPDUMP_SUBTYPE_ZEBRA_BGP_STATE_CHANGE:
    return process_zebra_bgp_state_change(s, entry, ASN16_LEN);
  case BGPDUMP_SUBTYPE_ZEBRA_BGP_STATE_CHANGE_AS4:
    return process_zebra_bgp_state_change(s, entry, ASN32_LEN);
  case BGPDUMP_SUBTYPE_ZEBRA_BGP_MESSAGE:
    return process_zebra_bgp_message(s, entry, ASN16_LEN);
  case BGPDUMP_SUBTYPE_ZEBRA_BGP_MESSAGE_AS4:
    return process_zebra_bgp_message(s, entry, ASN32_LEN);
  case BGPDUMP_SUBTYPE_ZEBRA_BGP_ENTRY:
    return process_zebra_bgp_entry(s, entry);
  case BGPDUMP_SUBTYPE_ZEBRA_BGP_SNAPSHOT:
    return process_zebra_bgp_snapshot(s, entry);
  default:
    bgpdump_warn("process_zebra_bgp: unknown subtype %d", entry->subtype);
    return 0;
  }
}

int process_zebra_bgp_state_change(struct mstream *s, BGPDUMP_ENTRY *entry,
                                   u_int8_t asn_len)
{
  read_asn(s, &entry->body.zebra_state_change.source_as, asn_len);
  read_asn(s, &entry->body.zebra_state_change.destination_as, asn_len);

  /* Work around Zebra dump corruption.
   * N.B. I don't see this in quagga 0.96.4 any more. Is it fixed? */
  if (entry->length == 8) {
    bgpdump_warn(
      "process_zebra_bgp_state_change: 8-byte state change (zebra bug?)");

    mstream_getw(s, &entry->body.zebra_state_change.old_state);
    mstream_getw(s, &entry->body.zebra_state_change.new_state);

    /* Fill in with dummy values */
    entry->body.zebra_state_change.interface_index = 0;
    entry->body.zebra_state_change.address_family = AFI_IP;
    entry->body.zebra_state_change.source_ip.v4_addr.s_addr = 0;
    entry->body.zebra_state_change.destination_ip.v4_addr.s_addr = 0;

    return 1;
  }

  mstream_getw(s, &entry->body.zebra_state_change.interface_index);
  mstream_getw(s, &entry->body.zebra_state_change.address_family);

  switch (entry->body.zebra_state_change.address_family) {
  case AFI_IP:
    // length could be 20 or 24 (asn16 vs asn32)
    if (entry->length != 20 && entry->length != 24) {
      bgpdump_warn("process_zebra_bgp_state_change: bad length %d",
                   entry->length);
      return 0;
    }

    entry->body.zebra_state_change.source_ip.v4_addr = mstream_get_ipv4(s);
    entry->body.zebra_state_change.destination_ip.v4_addr = mstream_get_ipv4(s);
    break;
  case AFI_IP6:
    // length could be 44 or 48 (asn16 vs asn32)
    if (entry->length != 44 && entry->length != 48) {
      bgpdump_warn("process_zebra_bgp_state_change: bad length %d",
                   entry->length);
      return 0;
    }

    mstream_get(s, &entry->body.zebra_state_change.source_ip.v6_addr.s6_addr,
                16);
    mstream_get(
      s, &entry->body.zebra_state_change.destination_ip.v6_addr.s6_addr, 16);
    break;
  default:
    bgpdump_warn("process_zebra_bgp_state_change: unknown AFI %d",
                 entry->body.zebra_state_change.address_family);
    return 0;
  }
  mstream_getw(s, &entry->body.zebra_state_change.old_state);
  mstream_getw(s, &entry->body.zebra_state_change.new_state);

  return 1;
}

int process_zebra_bgp_message(struct mstream *s, BGPDUMP_ENTRY *entry,
                              u_int8_t asn_len)
{
  u_char marker[16]; /* BGP marker */

  read_asn(s, &entry->body.zebra_message.source_as, asn_len);
  read_asn(s, &entry->body.zebra_message.destination_as, asn_len);
  mstream_getw(s, &entry->body.zebra_message.interface_index);
  mstream_getw(s, &entry->body.zebra_message.address_family);

  entry->body.zebra_message.opt_len = 0;
  entry->body.zebra_message.opt_data = NULL;
  entry->body.zebra_message.notify_len = 0;
  entry->body.zebra_message.notify_data = NULL;

  switch (entry->body.zebra_message.address_family) {
  case AFI_IP:
    entry->body.zebra_message.source_ip.v4_addr = mstream_get_ipv4(s);
    entry->body.zebra_message.destination_ip.v4_addr = mstream_get_ipv4(s);
    mstream_get(s, marker, 16);
    break;
  case AFI_IP6:
    mstream_get(s, &entry->body.zebra_message.source_ip.v6_addr.s6_addr, 16);
    mstream_get(s, &entry->body.zebra_message.destination_ip.v6_addr.s6_addr,
                16);
    mstream_get(s, marker, 16);
    break;
  case 0xFFFF:
    /* Zebra doesn't dump ifindex or src/dest IPs in OPEN
     * messages. Work around it. */
    if (entry->body.zebra_message.interface_index == 0xFFFF) {
      memset(marker, 0xFF, 4);
      mstream_get(s, marker + 4, 12);
      entry->body.zebra_message.interface_index = 0;
      entry->body.zebra_message.address_family = AFI_IP;
      entry->body.zebra_message.source_ip.v4_addr.s_addr = 0;
      entry->body.zebra_message.destination_ip.v4_addr.s_addr = 0;
      break;
    }
  /* Note fall through! If we don't recognize this type of data corruption, we
   * say
   * the address family is unsupported (since FFFF is not a valid address
   * family) */
  default:
    /* unsupported address family */
    bgpdump_warn("process_zebra_bgp_message: unsupported AFI %d",
                 entry->body.zebra_message.address_family);
    return 0;
  }

  if (memcmp(marker,
             "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377",
             16) != 0) {
    /* bad marker... ignore packet */
    bgpdump_warn("bgp_message: bad marker: "
                 "%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%"
                 "02x.%02x.%02x.%02x",
                 marker[0], marker[1], marker[2], marker[3], marker[4],
                 marker[5], marker[6], marker[7], marker[8], marker[9],
                 marker[10], marker[11], marker[12], marker[13], marker[14],
                 marker[15]);
    return 0;
  }

  mstream_getw(s, &entry->body.zebra_message.size);

  int expected =
    entry->body.zebra_message.size - sizeof(marker) - sizeof(u_int16_t);

  mstream_t copy = mstream_copy(s, expected);

  entry->body.zebra_message.cut_bytes = expected - mstream_can_read(&copy);

  switch (mstream_getc(&copy, &entry->body.zebra_message.type)) {
  case BGP_MSG_OPEN:
    return process_zebra_bgp_message_open(&copy, entry, asn_len);
  case BGP_MSG_UPDATE:
    return process_zebra_bgp_message_update(&copy, entry, asn_len);
  case BGP_MSG_NOTIFY:
    return process_zebra_bgp_message_notify(&copy, entry);
  case BGP_MSG_KEEPALIVE:
    /* Nothing to do */
    return 1;
  case BGP_MSG_ROUTE_REFRESH_01:
    /* Not implemented yet */
    bgpdump_warn("bgp_message: MSG_ROUTE_REFRESH_01 not implemented yet");
    return 0;
  case BGP_MSG_ROUTE_REFRESH:
    /* Not implemented yet */
    bgpdump_warn("bgp_message: MSG_ROUTE_REFRESH not implemented yet");
    return 0;
  default:
    bgpdump_warn("bgp_message: unknown BGP message type %d",
                 entry->body.zebra_message.type);
    return 0;
  }
}

int process_zebra_bgp_message_notify(struct mstream *s, BGPDUMP_ENTRY *entry)
{
  mstream_getc(s, &entry->body.zebra_message.error_code);
  mstream_getc(s, &entry->body.zebra_message.sub_error_code);
  entry->body.zebra_message.notify_len = entry->body.zebra_message.size - 21;

  if (entry->body.zebra_message.notify_len > 0) {
    entry->body.zebra_message.notify_data =
      malloc(entry->body.zebra_message.notify_len);
    mstream_get(s, entry->body.zebra_message.notify_data,
                entry->body.zebra_message.notify_len);
  }

  return 1;
}

int process_zebra_bgp_message_open(struct mstream *s, BGPDUMP_ENTRY *entry,
                                   u_int8_t asn_len)
{
  mstream_getc(s, &entry->body.zebra_message.version);
  read_asn(s, &entry->body.zebra_message.my_as, asn_len);
  mstream_getw(s, &entry->body.zebra_message.hold_time);
  entry->body.zebra_message.bgp_id = mstream_get_ipv4(s);
  mstream_getc(s, &entry->body.zebra_message.opt_len);

  if (entry->body.zebra_message.opt_len) {
    entry->body.zebra_message.opt_data =
      malloc(entry->body.zebra_message.opt_len);
    mstream_get(s, entry->body.zebra_message.opt_data,
                entry->body.zebra_message.opt_len);
  }

  return 1;
}

int process_zebra_bgp_message_update(struct mstream *s, BGPDUMP_ENTRY *entry,
                                     u_int8_t asn_len)
{
  entry->body.zebra_message.incomplete.orig_len = 0;

  mstream_t withdraw_stream = mstream_copy(s, mstream_getw(s, NULL));
  entry->body.zebra_message.withdraw_count = read_prefix_list(
    &withdraw_stream, AFI_IP, entry->body.zebra_message.withdraw,
    &entry->body.zebra_message.incomplete);

  if ((entry->attr = process_attributes(
         s, asn_len, &entry->body.zebra_message.incomplete)) == NULL) {
    return -1;
  }

  entry->body.zebra_message.announce_count =
    read_prefix_list(s, AFI_IP, entry->body.zebra_message.announce,
                     &entry->body.zebra_message.incomplete);

  return 1;
}

int process_zebra_bgp_entry(struct mstream *s, BGPDUMP_ENTRY *entry)
{
  bgpdump_warn("process_zebra_bgp_entry: record type not implemented yet");
  return 0;
}

int process_zebra_bgp_snapshot(struct mstream *s, BGPDUMP_ENTRY *entry)
{
  bgpdump_warn("process_zebra_bgp_snapshot: record type not implemented yet");
  return 0;
}

static attributes_t *attr_init(struct mstream *s, int len)
{

  attributes_t *attr = malloc(sizeof(struct attr));

  attr->data = malloc(len);
  memcpy(attr->data, &s->start[s->position], len);

  attr->len = len;
  attr->flag = 0;
  attr->origin = -1;
  attr->nexthop.s_addr = INADDR_NONE;
  attr->med = -1;
  attr->local_pref = -1;
  attr->aggregator_as = -1;
  attr->aggregator_addr.s_addr = INADDR_NONE;
  attr->weight = -1;

  attr->originator_id.s_addr = -1;
  attr->cluster = NULL;

  attr->aspath = NULL;
  attr->community = NULL;
  attr->transit = NULL;
  attr->mp_info = calloc(1, sizeof(struct mp_info));
  ;

  attr->unknown_num = 0;
  attr->unknown = NULL;

  attr->new_aspath = NULL;
  attr->old_aspath = NULL;
  attr->new_aggregator_as = -1;
  attr->new_aggregator_addr.s_addr = INADDR_NONE;

  return attr;
}

static void process_unknown_attr(struct mstream *s, attributes_t *attr,
                                 int flag, int type, int len)
{
  /* Unknown attribute. Save as is */
  attr->unknown_num++;
  attr->unknown =
    realloc(attr->unknown, attr->unknown_num * sizeof(struct unknown_attr));

  /* Pointer to the unknown attribute we want to fill in */
  struct unknown_attr unknown = {
    .flag = flag, .type = type, .len = len, .raw = malloc(len)};

  attr->unknown[attr->unknown_num - 1] = unknown;

  mstream_get(s, unknown.raw, len);
}

static int process_one_attr(struct mstream *outer_stream, attributes_t *attr,
                             u_int8_t asn_len,
                             struct zebra_incomplete *incomplete)
{
  int flag = mstream_getc(outer_stream, NULL);
  int type = mstream_getc(outer_stream, NULL);
  int len;

  if (flag & BGP_ATTR_FLAG_EXTLEN)
    len = mstream_getw(outer_stream, NULL);
  else
    len = mstream_getc(outer_stream, NULL);

  // info("flag:%-2i type:%-2i length:%i", flag, type, len);

  mstream_t ms = mstream_copy(outer_stream, len), *s = &ms;
  if (mstream_can_read(s) != len) {
    bgpdump_warn("ERROR attribute is truncated: expected=%u remaining=%u\n",
                 len, mstream_can_read(s));
    return -1;
  }

  /* Take note of all attributes, including unknown ones */
  if (type <= sizeof(attr->flag) * 8)
    attr->flag |= ATTR_FLAG_BIT(type);

  switch (type) {
  case BGP_ATTR_MP_REACH_NLRI:
    process_mp_announce(s, attr->mp_info, incomplete);
    break;
  case BGP_ATTR_MP_UNREACH_NLRI:
    process_mp_withdraw(s, attr->mp_info, incomplete);
    break;
  case BGP_ATTR_ORIGIN:
    if(attr->origin != -1) {
      bgpdump_err("ERROR attr->origin is already set");
      return -1;
    }
    attr->origin = mstream_getc(s, NULL);
    break;
  case BGP_ATTR_AS_PATH:
    if (attr->aspath) {
      bgpdump_err("ERROR attr->aspath is already set");
      return -1;
    }
    attr->aspath = create_aspath(len, asn_len);
    mstream_get(s, attr->aspath->data, len);
    /*process_attr_aspath_string(attr->aspath);*/
    break;
  case BGP_ATTR_NEXT_HOP:
    if (INADDR_NONE != attr->nexthop.s_addr) {
      bgpdump_err("ERROR attr->nexthop is already set");
      return -1;
    }
    attr->nexthop = mstream_get_ipv4(s);
    break;
  case BGP_ATTR_MULTI_EXIT_DISC:
    if (-1 != attr->med) {
      bgpdump_err("ERROR attr->med is already set");
      return -1;
    }
    mstream_getl(s, &attr->med);
    break;
  case BGP_ATTR_LOCAL_PREF:
    if (-1 != attr->local_pref) {
      bgpdump_err("ERROR attr->local_pref is already set");
      return -1;
    }
    mstream_getl(s, &attr->local_pref);
    break;
  case BGP_ATTR_ATOMIC_AGGREGATE:
    break;
  case BGP_ATTR_AGGREGATOR:
    // wrong aggregator as was checked here
    if (-1 != attr->aggregator_as) {
      bgpdump_err("ERROR attr->aggregator_as is already set");
      return -1;
    }
    read_asn(s, &attr->aggregator_as, asn_len);
    attr->aggregator_addr = mstream_get_ipv4(s);
    break;
  case BGP_ATTR_COMMUNITIES:
    if (attr->community) {
      bgpdump_err("ERROR attr->community is already set");
      return -1;
    }
    attr->community = malloc(sizeof(struct community));
    attr->community->size = len / 4;
    attr->community->val = malloc(len);
    mstream_get(s, attr->community->val, len);
    attr->community->str = NULL;
    /*process_attr_community_string(attr->community);*/
    break;
  case BGP_ATTR_NEW_AS_PATH:
    if (attr->new_aspath) {
      bgpdump_err("ERROR attr->new_aspath is already set");
      return -1;
    }
    attr->new_aspath = create_aspath(len, ASN32_LEN);
    mstream_get(s, attr->new_aspath->data, len);
    /*process_attr_aspath_string(attr->new_aspath);*/
    /* AS_CONFED_SEQUENCE and AS_CONFED_SET segments invalid in NEW_AS_PATH */
    check_new_aspath(attr->new_aspath);
    break;
  case BGP_ATTR_NEW_AGGREGATOR:
    if (-1 != attr->new_aggregator_as) {
      bgpdump_err("ERROR attr->new_aggregator_as is already set");
      return -1;
    }
    read_asn(s, &attr->new_aggregator_as, ASN32_LEN);
    attr->new_aggregator_addr = mstream_get_ipv4(s);
    break;
  case BGP_ATTR_ORIGINATOR_ID:
    if (INADDR_NONE != attr->originator_id.s_addr) {
      bgpdump_err("ERROR attr->originator_id is already set");
      return -1;
    }
    attr->originator_id = mstream_get_ipv4(s);
    break;
  case BGP_ATTR_CLUSTER_LIST:
    if (attr->cluster) {
      bgpdump_err("ERROR attr->cluster is already set");
      return -1;
    }
    attr->cluster = malloc(sizeof(struct cluster_list));
    attr->cluster->length = len / 4;
    attr->cluster->list =
      malloc((attr->cluster->length) * sizeof(struct in_addr));

    int i; // cluster index
    for (i = 0; i < attr->cluster->length; i++)
      attr->cluster->list[i] = mstream_get_ipv4(s);
    break;
  default:
    process_unknown_attr(s, attr, flag, type, len);
  }

  return 0;
}

attributes_t *process_attributes(struct mstream *s, u_int8_t asn_len,
                                 struct zebra_incomplete *incomplete)
{
  int total = mstream_getw(s, NULL);

  attributes_t *attr = attr_init(s, total);
  mstream_t copy = mstream_copy(s, total);

  if (mstream_can_read(&copy) != total)
    bgpdump_warn("entry is truncated: expected=%u remaining=%u", total,
                 mstream_can_read(&copy));

  while (mstream_can_read(&copy)) {
    if (process_one_attr(&copy, attr, asn_len, incomplete) != 0) {
      bgpdump_warn("process_one_attr failed, stopping attribute processing");
      return NULL;
    }
  }

  // Once all attributes have been read, take care of ASN32 transition
  process_asn32_trans(attr, asn_len);

  return attr;
}

struct aspath *create_aspath(u_int16_t len, u_int8_t asn_len)
{
  struct aspath *aspath = malloc(sizeof(struct aspath));
  if (aspath) {
    aspath->asn_len = asn_len;
    aspath->length = len;
    aspath->count = 0;
    aspath->str = NULL;
    if (len > 0)
      aspath->data = malloc(len);
    else
      aspath->data = NULL;
  }
  return aspath;
}

void aspath_error(struct aspath *as)
{
  as->count = 0;

  if (as->str) {
    free(as->str);
    as->str = NULL;
  }

  as->str = malloc(strlen(ASPATH_STR_ERROR) + 1);
  strcpy(as->str, ASPATH_STR_ERROR);
}

/* AK adds buildstring flag as this func is used by bgpdump just to get the
   number of hops (as->count) */
void process_attr_aspath_string(struct aspath *as, int buildstring)
{

  const int MAX_ASPATH_LEN = 8000;

  if (buildstring) {
    /* We could probably just skip processing, but lets play it safe  --AK */
    if (as->str != NULL) {
      free(as->str);
      as->str = NULL;
    }
    as->str = malloc(MAX_ASPATH_LEN);
  }

  as->count = 0;

  /* Set default values */
  int space = 0;
  u_char type = AS_SEQUENCE;
  int pos = 0;

  /* Set initial pointer. */
  caddr_t pnt = as->data;
  caddr_t end = pnt + as->length;
  struct assegment *segment = NULL;

  u_int16_t tmp16;
  u_int32_t tmp32;

  int i;
  while (pnt < end) {

    /* For fetch value. */
    segment = (struct assegment *)pnt;

    /* Check AS type validity. */
    if ((segment->type != AS_SET) && (segment->type != AS_SEQUENCE) &&
        (segment->type != AS_CONFED_SET) &&
        (segment->type != AS_CONFED_SEQUENCE)) {
      aspath_error(as);
      return;
    }

    /* Check AS length. */
    if ((pnt + (segment->length * as->asn_len) + AS_HEADER_SIZE) > end) {
      aspath_error(as);
      return;
    }

    if (buildstring) {
      /* If segment type is changed, print previous type's end
         character. */
      if (type != AS_SEQUENCE)
        as->str[pos++] = aspath_delimiter_char(type, AS_SEG_END);

      if (space)
        as->str[pos++] = ' ';

      if (segment->type != AS_SEQUENCE)
        as->str[pos++] = aspath_delimiter_char(segment->type, AS_SEG_START);

      space = 0;
    }

    /* Increment as->count - NOT ignoring CONFED_SETS/SEQUENCES any more.
       I doubt anybody was relying on this behaviour anyway. */
    switch (segment->type) {
    case AS_SEQUENCE:
    case AS_CONFED_SEQUENCE:
      as->count += segment->length;
      break;
    case AS_SET:
    case AS_CONFED_SET:
      as->count += 1;
      break;
    }

    if (buildstring) {
      for (i = 0; i < segment->length; i++) {
        as_t asn;

        if (space) {
          if (segment->type == AS_SET || segment->type == AS_CONFED_SET)
            as->str[pos++] = ',';
          else
            as->str[pos++] = ' ';
        } else
          space = 1;

        int asn_pos = i * as->asn_len;
        switch (as->asn_len) {
        case ASN16_LEN:
          memcpy(&tmp16, segment->data + asn_pos, sizeof(u_int16_t));
          asn = ntohs(tmp16);
          break;
        case ASN32_LEN:
          memcpy(&tmp32, segment->data + asn_pos, sizeof(u_int32_t));
          asn = ntohl(tmp32);
          break;
        default:
          assert("invalid asn_len" && false);
          return;
        }

        pos += bgpdump_int2str(asn, as->str + pos);
        if (pos > MAX_ASPATH_LEN - 100) {
          strcpy(as->str + pos, "...");
          return;
        };
      }
    }

    type = segment->type;
    pnt += (segment->length * as->asn_len) + AS_HEADER_SIZE;
  }

  if (buildstring) {
    if (segment && segment->type != AS_SEQUENCE)
      as->str[pos++] = aspath_delimiter_char(segment->type, AS_SEG_END);

    as->str[pos] = '\0';
  }
}

char aspath_delimiter_char(u_char type, u_char which)
{
  int i;
  struct {
    int type;
    char start;
    char end;
  } aspath_delim_char[] = {{AS_SET, '{', '}'},
                           {AS_SEQUENCE, ' ', ' '},
                           {AS_CONFED_SET, '[', ']'},
                           {AS_CONFED_SEQUENCE, '(', ')'},
                           {0, '\0', '\0'}};

  for (i = 0; aspath_delim_char[i].type != 0; i++) {
    if (aspath_delim_char[i].type == type) {
      if (which == AS_SEG_START)
        return aspath_delim_char[i].start;
      else if (which == AS_SEG_END)
        return aspath_delim_char[i].end;
    }
  }
  return ' ';
}

void process_attr_community_string(struct community *com)
{
  if (com == NULL) {
    return;
  }

  char buf[BUFSIZ];
  int i;
  u_int32_t comval;
  u_int16_t as;
  u_int16_t val;

  memset(buf, 0, BUFSIZ);

  for (i = 0; i < com->size; i++) {
    memcpy(&comval, com_nthval(com, i), sizeof(u_int32_t));
    comval = ntohl(comval);
    switch (comval) {
    case COMMUNITY_NO_EXPORT:
      strlcat(buf, " no-export", BUFSIZ);
      break;
    case COMMUNITY_NO_ADVERTISE:
      strlcat(buf, " no-advertise", BUFSIZ);
      break;
    case COMMUNITY_LOCAL_AS:
      strlcat(buf, " local-AS", BUFSIZ);
      break;
    default:
      as = (comval >> 16) & 0xFFFF;
      val = comval & 0xFFFF;
      snprintf(buf + strlen(buf), BUFSIZ - strlen(buf), " %d:%d", as, val);
      break;
    }
  }

  com->str = malloc(strlen(buf) + 1);
  strcpy(com->str, buf);
}

static struct mp_nlri *get_nexthop(struct mstream *s, u_int16_t afi)
{
  struct mp_nlri *nlri = calloc(1, sizeof(struct mp_nlri));

  nlri->nexthop_len = mstream_getc(s, NULL);

  // sometimes nexthop_len is 0 - not sure what this means (see IS-626)
  // if(mp_nlri->nexthop_len == 0)
  //    return len;

  if (afi == AFI_IP) {
    assert(nlri->nexthop_len == 4);
    nlri->nexthop.v4_addr = mstream_get_ipv4(s);
    return nlri;
  }
  assert(afi == AFI_IP6);
  mstream_get(s, &nlri->nexthop.v6_addr, 16);
  if (nlri->nexthop_len == 32) {
    /* Is there also a link-local address? */
    mstream_get(s, &nlri->nexthop_local.v6_addr.s6_addr, 16);
  } else if (nlri->nexthop_len != 16) {
    bgpdump_warn("process_mp_announce: unknown MP nexthop length %d",
                 nlri->nexthop_len);
  }
  return nlri;
}

void process_mp_announce(struct mstream *s, struct mp_info *info,
                         struct zebra_incomplete *incomplete)
{
  u_int16_t afi;
  u_int8_t safi;

  // look for MRT abbreviated MP_NLRI packets
  if (s->start[s->position] != 0) {
    assert(info->announce[AFI_IP6][SAFI_UNICAST] == NULL);
    info->announce[AFI_IP6][SAFI_UNICAST] = get_nexthop(s, AFI_IP6);
    return;
  }

  mstream_getw(s, &afi);
  mstream_getc(s, &safi);

  if (afi == 0 || afi > BGPDUMP_MAX_AFI || safi > BGPDUMP_MAX_SAFI) {
    bgpdump_warn("process_mp_announce: unknown protocol(AFI=%d, SAFI=%d)!", afi,
                 safi);
    return;
  }

  if (info->announce[afi][safi] != NULL) {
    bgpdump_warn(
      "process_mp_announce: two MP_NLRI for the same protocol(%d, %d)!", afi,
      safi);
    return;
  }

  info->announce[afi][safi] = get_nexthop(s, afi);

  // SNPA is defunct and num_snpa should always be 0
  u_int8_t num_snpa;
  if (mstream_getc(s, &num_snpa))
    bgpdump_warn("process_mp_announce: MP_NLRI contains SNPAs, skipping");
  for (; num_snpa > 0; --num_snpa) {
    mstream_get(s, NULL, mstream_getc(s, NULL));
  }

  info->announce[afi][safi]->prefix_count =
    read_prefix_list(s, afi, info->announce[afi][safi]->nlri, incomplete);
}

void process_mp_withdraw(struct mstream *s, struct mp_info *info,
                         struct zebra_incomplete *incomplete)
{
  u_int16_t afi;
  u_int8_t safi;
  struct mp_nlri *mp_nlri;

  mstream_getw(s, &afi);
  mstream_getc(s, &safi);

  /* Do we know about this address family? */
  if (afi > BGPDUMP_MAX_AFI || safi > BGPDUMP_MAX_SAFI) {
    bgpdump_warn("process_mp_withdraw: unknown AFI,SAFI %d,%d!", afi, safi);
    return;
  }

  /* If there are 2 NLRI's for the same protocol, fail but don't burn and die */
  if (info->withdraw[afi][safi] != NULL) {
    bgpdump_warn("process_mp_withdraw: update contains more than one MP_NLRI "
                 "with AFI,SAFI %d,%d!",
                 afi, safi);
    return;
  }

  /* Allocate structure */
  mp_nlri = malloc(sizeof(struct mp_nlri));
  memset(mp_nlri, 0, sizeof(struct mp_nlri));
  info->withdraw[afi][safi] = mp_nlri;

  mp_nlri->prefix_count = read_prefix_list(s, afi, mp_nlri->nlri, incomplete);
}

static int read_prefix_list(struct mstream *s, u_int16_t afi,
                            struct prefix *prefixes,
                            struct zebra_incomplete *incomplete)
{
  int count = 0;

  while (mstream_can_read(s)) {
    u_int8_t p_len = mstream_getc(s, NULL); // length in bits
    u_int8_t p_bytes = (p_len + 7) / 8;

    /* Truncated prefix list? */
    if (mstream_can_read(s) < p_bytes) {
      if (!incomplete)
        break;

      /* Put prefix in incomplete structure */
      incomplete->afi = afi;
      incomplete->orig_len = p_len;
      incomplete->prefix = (struct prefix){.len = mstream_can_read(s) * 8};
      mstream_get(s, &incomplete->prefix.address, p_bytes);
      break;
    }

    struct prefix *prefix = prefixes + count;

    if (count++ > MAX_PREFIXES)
      continue;

    *prefix = (struct prefix){.len = p_len};
    mstream_get(s, &prefix->address, p_bytes);
  }

  if (count > MAX_PREFIXES) {
    bgpdump_err("too many prefixes (%i > %i)", count, MAX_PREFIXES);
    assert(0); // AK adds to force a fatal error
    return MAX_PREFIXES;
  }

  return count;
}

static as_t read_asn(struct mstream *s, as_t *asn, u_int8_t len)
{
  u_int16_t asn16;

  assert(len == sizeof(u_int32_t) || len == sizeof(u_int16_t));
  switch (len) {
  case sizeof(u_int32_t):
    return mstream_getl(s, asn);
  case sizeof(u_int16_t):
    mstream_getw(s, &asn16);
    if (asn)
      *asn = asn16;
    return asn16;
  default:
    /* Not reached. Avoid compiler warning */
    return 0;
  }
}

int check_new_aspath(struct aspath *aspath)
{
  struct assegment *segment;
  for (segment = (struct assegment *)aspath->data;
       segment < (struct assegment *)(aspath->data + aspath->length);
       segment = (struct assegment *)((char *)segment + sizeof(*segment) +
                                      segment->length * ASN32_LEN)) {
    if (segment->type == AS_CONFED_SEQUENCE || segment->type == AS_CONFED_SET) {
      bgpdump_warn(
        "check_new_aspath: invalid segment of type AS_CONFED_%s in NEW_AS_PATH",
        segment->type == AS_CONFED_SET ? "SET" : "SEQUENCE");
      return 0;
    }
  }
  return 1;
}

void process_asn32_trans(attributes_t *attr, u_int8_t asn_len)
{
  if (asn_len == ASN32_LEN) {
    /* These attributes "SHOULD NOT" be used with ASN32. */
    if (attr->flag & ATTR_FLAG_BIT(BGP_ATTR_NEW_AS_PATH))
      bgpdump_warn(
        "process_asn32_trans: ASN32 message contains NEW_AS_PATH attribute");

    if (attr->flag & ATTR_FLAG_BIT(BGP_ATTR_NEW_AGGREGATOR))
      bgpdump_warn(
        "process_asn32_trans: ASN32 message contains NEW_AGGREGATOR attribute");

    /* Don't compute anything, just leave AS_PATH and AGGREGATOR as they are */
    return;
  }

  /* Process NEW_AGGREGATOR */
  if (attr->flag & ATTR_FLAG_BIT(BGP_ATTR_AGGREGATOR) &&
      attr->flag & ATTR_FLAG_BIT(BGP_ATTR_NEW_AGGREGATOR)) {
    /* Both AGGREGATOR and NEW_AGGREGATOR present, merge */
    if (attr->aggregator_as != AS_TRAN) {
      /* Don't do anything */
      return;
    } else {
      attr->old_aggregator_as = attr->aggregator_as;
      attr->old_aggregator_addr = attr->aggregator_addr;
      attr->aggregator_as = attr->new_aggregator_as;
      attr->aggregator_addr = attr->new_aggregator_addr;
    }
  }

  /* Process NEW_AS_PATH */
  if (!(attr->flag & ATTR_FLAG_BIT(BGP_ATTR_NEW_AS_PATH)))
    return;

  // AK HAX: This code requries the aspath->count field to be populated. This is
  // (unfortunately) computed by the process_attr_aspath_string function, so we
  // call it here.
  process_attr_aspath_string(attr->aspath, 0);
  process_attr_aspath_string(attr->new_aspath, 0);

  // attr->aspath may be NULL, at least in case of MP_UNREACH_NLRI
  if (attr->aspath == NULL)
    return;
  if (attr->aspath->count < attr->new_aspath->count) {
    return;
  }

  /* Merge paths */
  attr->old_aspath = attr->aspath;
  attr->aspath = asn32_merge_paths(attr->old_aspath, attr->new_aspath);
  // if(attr->aspath) {
  //  process_attr_aspath_string(attr->aspath);
  //}
}

struct aspath *asn32_merge_paths(struct aspath *path, struct aspath *newpath)
{
  struct aspath *mergedpath = create_aspath(0, ASN32_LEN);
  struct assegment *segment, *mergedsegment;
  int newlen;

  /* Keep copying segments from AS_PATH until our path is as long as AS_PATH -
   * NEW_AS_PATH. */
  segment = (struct assegment *)(path->data);
  while (mergedpath->count < (path->count - newpath->count)) {
    /* Make room */
    newlen = mergedpath->length + sizeof(struct assegment) +
             segment->length * ASN32_LEN;
    mergedpath->data = realloc(mergedpath->data, newlen);

    /* Create a new AS-path segment */
    mergedsegment = (struct assegment *)(mergedpath->data + mergedpath->length);

    /* Copy segment over. AS_PATH contains 16-bit ASes, so expand */
    mergedsegment->type = segment->type;
    mergedsegment->length = segment->length;
    asn32_expand_16_to_32(mergedsegment->data, segment->data, segment->length);

    /* Update length */
    mergedpath->length = newlen;
    if (segment->type == AS_SET || segment->type == AS_CONFED_SET) {
      mergedpath->count += 1;
    } else {
      mergedpath->count += segment->length;
      /* Did we copy too many ASes over? */
      if (mergedpath->count > path->count - newpath->count) {
        mergedsegment->length -=
          mergedpath->count - (path->count - newpath->count);
        mergedpath->length -=
          (mergedpath->count - (path->count - newpath->count)) * ASN32_LEN;
      }
    }
  }

  /* Append NEW_AS_PATH to merged path */
  mergedpath->data =
    realloc(mergedpath->data, mergedpath->length + newpath->length);
  memcpy(mergedpath->data + mergedpath->length, newpath->data, newpath->length);
  mergedpath->length += newpath->length;

  return mergedpath;
}

void asn32_expand_16_to_32(char *dst, char *src, int len)
{
  u_int32_t *dstarray = (u_int32_t *)dst;
  u_int16_t *srcarray = (u_int16_t *)src;
  int i;

  for (i = 0; i < len; i++) {
    dstarray[i] = htonl(ntohs(srcarray[i]));
  }
}

#if defined(linux)
size_t strlcat(char *dst, const char *src, size_t size)
{
  if (strlen(dst) + strlen(src) >= size)
    return -1;

  strcat(dst, src);

  return (strlen(dst));
}
#endif
