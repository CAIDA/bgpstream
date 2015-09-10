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

#ifndef _BGPDUMP_MSTREAM_H
#define _BGPDUMP_MSTREAM_H

#include <sys/types.h>
#include <netinet/in.h>

typedef struct mstream {
    u_char	*start;
    u_int16_t	position;
    u_int32_t	len;
} mstream_t;

void      mstream_init(struct mstream *s, u_char *buffer,u_int32_t len);
u_char    mstream_getc(struct mstream *s, u_char *d);
u_int16_t mstream_getw(struct mstream *s, u_int16_t *d);
u_int32_t mstream_getl(struct mstream *s, u_int32_t *d);
struct in_addr mstream_get_ipv4(struct mstream *s);
u_int32_t mstream_can_read(struct mstream *s);
u_int32_t mstream_get (struct mstream *s, void *d, u_int32_t len);
mstream_t mstream_copy(mstream_t *s, int len);

#endif
