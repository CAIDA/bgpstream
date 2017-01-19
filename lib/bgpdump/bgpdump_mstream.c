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

Original Author: Dan Ardelean (dan@ripe.net)
*/

#include "bgpdump_mstream.h"
#include "config.h"

#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

void mstream_init(struct mstream *s, u_char *buffer, u_int32_t len)
{
  s->start = buffer;
  s->position = 0;
  s->len = len;
}

u_char mstream_getc(struct mstream *s, u_char *d)
{
  u_char data;

  mstream_get(s, &data, sizeof(data));
  if (d != NULL)
    memcpy(d, &data, sizeof(data));
  return data;
}

u_int16_t mstream_getw(struct mstream *s, u_int16_t *d)
{
  u_int16_t data;

  mstream_get(s, &data, sizeof(data));
  data = ntohs(data);
  if (d != NULL)
    memcpy(d, &data, sizeof(data));
  return data;
}

u_int32_t mstream_getl(struct mstream *s, u_int32_t *d)
{
  u_int32_t data;

  mstream_get(s, &data, sizeof(data));
  data = ntohl(data);
  if (d != NULL)
    memcpy(d, &data, sizeof(data));
  return data;
}

struct in_addr mstream_get_ipv4(struct mstream *s)
{
  struct in_addr addr;

  mstream_get(s, &addr.s_addr, 4);
  return addr;
}

u_int32_t mstream_can_read(struct mstream *s)
{
  return s->len - s->position;
}

// construct a partial mstream
mstream_t mstream_copy(mstream_t *s, int len)
{
  mstream_t copy = {0};
  copy.start = s->start + s->position;
  copy.len = mstream_get(s, NULL, len);
  return copy;
}

u_int32_t mstream_get(struct mstream *s, void *d, u_int32_t len)
{
  int room = mstream_can_read(s);

  if (room >= len) {
    if (d)
      memcpy(d, s->start + s->position, len);
    s->position += len;
    return len;
  } else {
    /* Reading past end of buffer!
       Zero out extra bytes and set position to end of buffer */
    if (d) {
      memcpy(d, s->start + s->position, room);
      memset((char *)d + room, 0, len - room);
    }
    s->position = s->len;
    return room;
  }
}
