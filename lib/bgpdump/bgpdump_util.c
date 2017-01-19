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

#include "bgpdump_util.h"

#include <arpa/inet.h>
#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <time.h>

static bool bgpdump_use_syslog = false;

void bgpdump_log_to_syslog()
{
  bgpdump_use_syslog = true;
}

void bgpdump_log_to_stderr()
{
  bgpdump_use_syslog = false;
}

#define log(lvl, lvl_str)                                                      \
  va_list args;                                                                \
  va_start(args, fmt);                                                         \
  _log(LOG_##lvl, #lvl_str, fmt, args)

static char *now_str()
{
  static char buffer[1000];
  time_t now = time(0);
  strftime(buffer, sizeof buffer, "%Y-%m-%d %H:%M:%S", localtime(&now));
  return buffer;
}

static void _log(int lvl, char *lvl_str, const char *fmt, va_list args)
{
  if (bgpdump_use_syslog) {
    syslog(lvl, fmt, args);
  } else {
    char prefix[strlen(fmt) + 1000];
    sprintf(prefix, "%s [%s] %s\n", now_str(), lvl_str, fmt);
    vfprintf(stderr, prefix, args);
  }
}

void bgpdump_err(const char *fmt, ...)
{
  log(ERR, error);
}
void bgpdump_warn(const char *fmt, ...)
{
  log(WARNING, warn);
}
void bgpdump_debug(const char *fmt, ...)
{
  log(INFO, info);
}

void bgpdump_time2str(struct tm *date, char *time_str)
{
  char tmp_str[10];

  if (date->tm_mon + 1 < 10)
    sprintf(tmp_str, "0%d/", date->tm_mon + 1);
  else
    sprintf(tmp_str, "%d/", date->tm_mon + 1);
  strcpy(time_str, tmp_str);

  if (date->tm_mday < 10)
    sprintf(tmp_str, "0%d/", date->tm_mday);
  else
    sprintf(tmp_str, "%d/", date->tm_mday);
  strcat(time_str, tmp_str);

  if (date->tm_year % 100 < 10)
    sprintf(tmp_str, "0%d ", date->tm_year % 100);
  else
    sprintf(tmp_str, "%d ", date->tm_year % 100);
  strcat(time_str, tmp_str);

  if (date->tm_hour < 10)
    sprintf(tmp_str, "0%d:", date->tm_hour);
  else
    sprintf(tmp_str, "%d:", date->tm_hour);
  strcat(time_str, tmp_str);

  if (date->tm_min < 10)
    sprintf(tmp_str, "0%d:", date->tm_min);
  else
    sprintf(tmp_str, "%d:", date->tm_min);
  strcat(time_str, tmp_str);

  if (date->tm_sec < 10)
    sprintf(tmp_str, "0%d", date->tm_sec);
  else
    sprintf(tmp_str, "%d", date->tm_sec);
  strcat(time_str, tmp_str);
}

int bgpdump_int2str(uint32_t value, char *str)
{
  const int MAX_UINT32_STRLEN = 11; /* 2**32-1 => 4294967295 */
  return snprintf(str, MAX_UINT32_STRLEN, "%" PRIu32, value);
}

static void ti2s(uint32_t value)
{
  char buf[100], ref[100];
  sprintf(ref, "%u", value);
  int len = bgpdump_int2str(value, buf);
  printf("%s =?= %s (%i)\n", ref, buf, len);
}

void bgpdump_test_utils()
{
  ti2s(0);
  ti2s(99999);
  ti2s(4294967295L);
}

char *bgpdump_fmt_ipv4(BGPDUMP_IP_ADDRESS addr, char *buffer)
{
  if (inet_ntop(AF_INET, &addr.v4_addr, buffer, INET_ADDRSTRLEN) == NULL) {
    return NULL;
  } else {
    return buffer;
  }
}

char *bgpdump_fmt_ipv6(BGPDUMP_IP_ADDRESS addr, char *buffer)
{
  if (inet_ntop(AF_INET6, &addr.v6_addr, buffer, INET6_ADDRSTRLEN) == NULL) {
    return NULL;
  } else {
    return buffer;
  }
}

static void test_roundtrip(char *str)
{
  BGPDUMP_IP_ADDRESS addr;
  inet_pton(AF_INET6, str, &addr.v6_addr);
  char tmp[1000];
  bgpdump_fmt_ipv6(addr, tmp);
  printf("%s -> %s [%s]\n", str, tmp, strcmp(str, tmp) ? "ERROR" : "ok");
}

void bgpdump_test_fmt_ip()
{
  test_roundtrip("fe80::");
  test_roundtrip("2001:db8::1");
  test_roundtrip("::ffff:192.168.2.1");
  test_roundtrip("::192.168.1.2");
  test_roundtrip("2001:7f8:30::2:1:0:8447");
}
