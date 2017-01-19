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

 Created by Devin Bayer on 9/1/10.
*/

#ifndef _BGPDUMP_UTIL_H
#define _BGPDUMP_UTIL_H
#include <time.h>

#include "bgpdump_attr.h"

void bgpdump_log_to_stderr(void);
void bgpdump_log_to_syslog(void);

void bgpdump_err(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void bgpdump_warn(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void bgpdump_debug(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// system inet_ntop() functions format IPv6 addresses
// inconsistently, so use these versions
char *bgpdump_fmt_ipv4(BGPDUMP_IP_ADDRESS addr, char *buffer);
char *bgpdump_fmt_ipv6(BGPDUMP_IP_ADDRESS addr, char *buffer);
void bgpdump_test_fmt_ip(void);

void bgpdump_time2str(struct tm *date, char *time_str);
int bgpdump_int2str(uint32_t value, char *str);
void bgpdump_test_utils(void);

#endif
