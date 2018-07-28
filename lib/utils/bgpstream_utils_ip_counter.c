#include "bgpstream_utils_ip_counter.h"
#include "bgpstream_utils_addr.h"
#include "utils.h"
#include <inttypes.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>

/* static char buffer[INET6_ADDRSTRLEN]; */

typedef struct struct_v4pfx_int_t {
  uint32_t start;
  uint32_t end;
  struct struct_v4pfx_int_t *next;
} v4pfx_int_t;

typedef struct struct_v6pfx_int_t {
  uint64_t start_ms;
  uint64_t start_ls;
  uint64_t end_ms;
  uint64_t end_ls;
  struct struct_v6pfx_int_t *next;
} v6pfx_int_t;

/* IP Counter Linked List */
struct bgpstream_ip_counter {
  v4pfx_int_t *v4list;
  v6pfx_int_t *v6list;
};

/* static void */
/* print_pfx_int_list(bgpstream_ip_counter_t *ipc) */
/*  {  */
/*    v4pfx_int_t *current4 = ipc->v4list; */
/*    bgpstream_ipv4_addr_t ip4; */

/*    ip4.version = BGPSTREAM_ADDR_VERSION_IPV4;  */
/*    while(current4 != NULL)  */
/*      {  */
/*        ip4.ipv4.s_addr = htonl(current4->start);  */
/*        bgpstream_addr_ntop(buffer, INET_ADDRSTRLEN, &ip4);  */
/*        printf("IP space:\t%s\t", buffer);  */
/*        ip4.ipv4.s_addr = htonl(current4->end);  */
/*        bgpstream_addr_ntop(buffer, INET_ADDRSTRLEN, &ip4);  */
/*        printf("%s\n", buffer);  */
/*        current4 = current4->next;  */
/*      } */

/*    v6pfx_int_t *current6 = ipc->v6list;  */
/*    bgpstream_ipv6_addr_t ip6; */
/*    uint64_t tmp; */
/*    ip6.version = BGPSTREAM_ADDR_VERSION_IPV6; */

/*    while(current6 != NULL)  */
/*      { */
/*        tmp = htonll(current6->start_ms); */
/*        memcpy(&ip6.ipv6.s6_addr[0], &tmp, sizeof(uint64_t)); */
/*        tmp = htonll(current6->start_ls); */
/*        memcpy(&ip6.ipv6.s6_addr[8], &tmp, sizeof(uint64_t)); */
/*        bgpstream_addr_ntop(buffer, INET6_ADDRSTRLEN, &ip6);  */
/*        printf("IP space:\t%s\t", buffer); */
/*        tmp = htonll(current6->end_ms); */
/*        memcpy(&ip6.ipv6.s6_addr[0], &tmp, sizeof(uint64_t)); */
/*        tmp = htonll(current6->end_ls); */
/*        memcpy(&ip6.ipv6.s6_addr[8],&tmp, sizeof(uint64_t)); */
/*        bgpstream_addr_ntop(buffer, INET6_ADDRSTRLEN, &ip6);  */
/*        printf("%s\n", buffer); */
/*        current6 = current6->next;  */
/*      } */
/*  }  */

static void one_step_merge4(v4pfx_int_t *pil)
{
  v4pfx_int_t *current = pil;
  v4pfx_int_t *previous = pil;
  if (current->next == NULL) {
    return;
  }
  current = current->next;

  while (current != NULL) {
    if (current->start <= previous->end) {
      if (current->end > previous->end) {
        previous->end = current->end;
      }
      previous->next = current->next;
      free(current);
      current = previous->next;
    } else {
      break;
    }
  }
}

static void one_step_merge6(v6pfx_int_t *pil)
{
  v6pfx_int_t *current = pil;
  v6pfx_int_t *previous = pil;
  if (current->next == NULL) {
    return;
  }
  current = current->next;

  while (current != NULL) {

    /* current->start <= previous->end */
    if (current->start_ms < previous->end_ms ||
        (current->start_ms == previous->end_ms &&
         current->start_ls <= previous->end_ls)) {
      /* current->end > previous->end */
      if (current->end_ms > previous->end_ms ||
          (current->end_ms == previous->end_ms &&
           current->end_ls > previous->end_ls)) {
        previous->end_ms = current->end_ms;
        previous->end_ls = current->end_ls;
      }
      previous->next = current->next;
      free(current);
      current = previous->next;
    } else {
      break;
    }
  }
}

static int merge_in_sorted_queue4(v4pfx_int_t **pil, uint32_t start,
                                  uint32_t end)
{
  // create new v4pfx_int_t
  v4pfx_int_t *p = NULL;
  v4pfx_int_t *previous = *pil;
  v4pfx_int_t *current = *pil;

  while (current != NULL) {
    if (start > current->end) {
      previous = current;
      current = current->next;
      continue;
    }
    if (end < current->start) {
      /* found a position where to insert  */
      if ((p = (v4pfx_int_t *)malloc_zero(sizeof(v4pfx_int_t))) == NULL) {
        fprintf(stderr, "ERROR: can't malloc v4pfx_int_t structure\n");
        return -1;
      }
      p->start = start;
      p->end = end;
      p->next = NULL;
      if (previous != current) {
        previous->next = p;
      } else {
        *pil = p;
      }
      p->next = current;
      return 0;
    }
    /* At this point S <= cE and E >= cS
     * so we can merge them */
    if (current->start > start) {
      current->start = start;
    }
    if (current->end < end) {
      current->end = end;
    }
    /* Now check from here on if there are other things
     * to merge */
    one_step_merge4(current);
    return 0;
  }

  /*   if here we didn't find a place where to insert the
   * interval, it is either the first position or the last */
  if ((p = (v4pfx_int_t *)malloc_zero(sizeof(v4pfx_int_t))) == NULL) {
    fprintf(stderr, "ERROR: can't malloc v4pfx_int_t structure\n");
    return -1;
  }
  p->start = start;
  p->end = end;
  p->next = NULL;
  /* adding at the beginning of the queue */
  if (previous == current) {
    *pil = p;
    return 0;
  }
  // adding at the end of the queue
  previous->next = p;
  return 0;
}

static int merge_in_sorted_queue6(v6pfx_int_t **pil, uint64_t start_ms,
                                  uint64_t start_ls, uint64_t end_ms,
                                  uint64_t end_ls)
{
  // create new v4pfx_int_t
  v6pfx_int_t *p = NULL;
  v6pfx_int_t *previous = *pil;
  v6pfx_int_t *current = *pil;

  while (current != NULL) {
    /* start > current->end */
    if (start_ms > current->end_ms ||
        (start_ms == current->end_ms && start_ls > current->end_ls)) {
      previous = current;
      current = current->next;
      continue;
    }
    /* end < current->start */
    if (end_ms < current->start_ms ||
        (end_ms == current->start_ms && end_ls < current->start_ls)) {
      /* found a position where to insert  */
      if ((p = (v6pfx_int_t *)malloc_zero(sizeof(v6pfx_int_t))) == NULL) {
        fprintf(stderr, "ERROR: can't malloc v6pfx_int_t structure\n");
        return -1;
      }
      p->start_ms = start_ms;
      p->start_ls = start_ls;
      p->end_ms = end_ms;
      p->end_ls = end_ls;
      p->next = NULL;
      if (previous != current) {
        previous->next = p;
      } else {
        *pil = p;
      }
      p->next = current;
      return 0;
    }
    /* At this point S <= cE and E >= cS
     * so we can merge them */
    /* current->start > start */
    if (current->start_ms > start_ms ||
        (current->start_ms == start_ms && current->start_ls > start_ls)) {
      current->start_ms = start_ms;
      current->start_ls = start_ls;
    }
    /* current->end < end */
    if (current->end_ms < end_ms ||
        (current->end_ms == end_ms && current->end_ls < end_ls)) {
      current->end_ms = end_ms;
      current->end_ls = end_ls;
    }
    /* Now check from here on if there are other things
     * to merge */
    one_step_merge6(current);
    return 0;
  }

  /*   if here we didn't find a place where to insert the
   * interval, it is either the first position or the last */
  if ((p = (v6pfx_int_t *)malloc_zero(sizeof(v6pfx_int_t))) == NULL) {
    fprintf(stderr, "ERROR: can't malloc v6pfx_int_t structure\n");
    return -1;
  }
  p->start_ms = start_ms;
  p->start_ls = start_ls;
  p->end_ms = end_ms;
  p->end_ls = end_ls;
  p->next = NULL;
  /* adding at the beginning of the queue */
  if (previous == current) {
    *pil = p;
    return 0;
  }
  // adding at the end of the queue
  previous->next = p;
  return 0;
}

bgpstream_ip_counter_t *bgpstream_ip_counter_create()
{
  bgpstream_ip_counter_t *ipc;
  if ((ipc = (bgpstream_ip_counter_t *)malloc_zero(
         sizeof(bgpstream_ip_counter_t))) == NULL) {
    fprintf(stderr, "ERROR: can't malloc bgpstream_ip_counter_t structure\n");
    return NULL;
  }
  ipc->v4list = NULL;
  ipc->v6list = NULL;
  return ipc;
}

int bgpstream_ip_counter_add(bgpstream_ip_counter_t *ipc, bgpstream_pfx_t *pfx)
{
  uint32_t start;
  uint32_t end;
  uint32_t mask;
  unsigned char *tmp;
  uint64_t start_ms = 0;
  uint64_t start_ls = 0;
  uint64_t end_ms = 0;
  uint64_t end_ls = 0;
  uint64_t mask_ms;
  uint64_t mask_ls;

  if (pfx->address.version == BGPSTREAM_ADDR_VERSION_IPV4) {
    mask = ~(((uint64_t)1 << (32 - pfx->mask_len)) - 1);
    start = ntohl(((bgpstream_ipv4_pfx_t *)pfx)->address.ipv4.s_addr);
    start = start & mask;
    end = start | (~mask);
    return merge_in_sorted_queue4(&ipc->v4list, start, end);
  } else {
    if (pfx->address.version == BGPSTREAM_ADDR_VERSION_IPV6) {
      tmp = &((bgpstream_ipv6_pfx_t *)pfx)->address.ipv6.s6_addr[0];
      start_ms = ntohll(*((uint64_t *)tmp));
      tmp = &((bgpstream_ipv6_pfx_t *)pfx)->address.ipv6.s6_addr[8];
      start_ls = ntohll(*((uint64_t *)tmp));
      /* printf("+++++++++  %"PRIu64" %"PRIu64"\n", start_ms, start_ls); */
      if (pfx->mask_len > 64) {
        /* len_ms = 0 */
        mask_ms = ~((uint64_t)0);
        /* len_ls = 64 - (pfx->mask_len - 64); */
        mask_ls = ~(((uint64_t)1 << (64 - (pfx->mask_len - 64))) - 1);
      } else {
        /* len_ms = 64 - pfx->mask_len; */
        mask_ms = ~(((uint64_t)1 << (64 - pfx->mask_len)) - 1);
        /* len_ls = 64; */
        /* mask_ls = ~( ((uint64_t)1 << 64) - 1); */
        mask_ls = 0;
      }

      /* printf("MASKS:  %"PRIu64" %"PRIu64"\n", mask_ms, mask_ls); */

      /* most significant */
      start_ms = start_ms & mask_ms;
      end_ms = start_ms | (~mask_ms);
      /* printf("MS:  %"PRIu64" %"PRIu64"\n", start_ms, end_ms); */

      /* least significant */
      start_ls = start_ls & mask_ls;
      end_ls = start_ls | (~mask_ls);
      /* printf("LS:  %"PRIu64" %"PRIu64"\n", start_ls, end_ls); */

      return merge_in_sorted_queue6(&ipc->v6list, start_ms, start_ls, end_ms,
                                    end_ls);
    }
  }
  /* print_pfx_int_list(ipc); */
  return 0;
}

uint32_t bgpstream_ip_counter_is_overlapping4(bgpstream_ip_counter_t *ipc,
                                              bgpstream_ipv4_pfx_t *pfx,
                                              uint8_t *more_specific)
{
  v4pfx_int_t *current = ipc->v4list;
  uint32_t start = 0;
  uint32_t end = 0;
  uint32_t len = 0;
  uint32_t pfx_size;
  uint32_t overlap_count = 0;
  *more_specific = 0;
  len = 32 - pfx->mask_len;
  start = ntohl(pfx->address.ipv4.s_addr);
  start = (start >> len) << len;
  end = 0;
  end = ~end;
  end = (end >> len) << len;
  end = start | (~end);
  pfx_size = end - start + 1;
  /* intersection endpoints */
  uint32_t int_start;
  uint32_t int_end;
  while (current != NULL) {
    if (current->start > end) {
      break;
    }
    if (current->end < start) {
      current = current->next;
      continue;
    }
    /* there is some overlap
     * max(start) and min(end) */
    int_start = current->start;
    int_end = current->end;
    if (current->start < start) {
      int_start = start;
    }
    if (current->end > end) {
      int_end = end;
    }
    if ((int_end - int_start + 1) == pfx_size) {
      *more_specific = 1;
    }
    overlap_count += int_end - int_start + 1;
    current = current->next;
  }
  return overlap_count;
}

uint64_t bgpstream_ip_counter_is_overlapping6(bgpstream_ip_counter_t *ipc,
                                              bgpstream_ipv6_pfx_t *pfx,
                                              uint8_t *more_specific)
{
  v6pfx_int_t *current = ipc->v6list;
  v6pfx_int_t *previous = ipc->v6list;

  uint64_t start_ms = 0;
  uint64_t start_ls = 0;
  uint64_t end_ms = 0;
  uint64_t end_ls = 0;
  uint64_t mask_ms;
  uint64_t mask_ls;
  unsigned char *tmp;
  uint64_t overlap_count = 0;
  uint64_t pfx_size;
  *more_specific = 0;

  tmp = &(pfx->address.ipv6.s6_addr[0]);
  start_ms = ntohll(*((uint64_t *)tmp));
  tmp = &(pfx->address.ipv6.s6_addr[8]);
  start_ls = ntohll(*((uint64_t *)tmp));

  /* printf("+++++++++  %"PRIu64" %"PRIu64"\n", start_ms, start_ls); */
  if (pfx->mask_len > 64) {
    /* len_ms = 0 */
    mask_ms = ~((uint64_t)0);
    /* len_ls = 64 - (pfx->mask_len - 64); */
    mask_ls = ~(((uint64_t)1 << (64 - (pfx->mask_len - 64))) - 1);
  } else {
    /* len_ms = 64 - pfx->mask_len; */
    mask_ms = ~(((uint64_t)1 << (64 - pfx->mask_len)) - 1);
    /* len_ls = 64; */
    /* mask_ls = ~( ((uint64_t)1 << 64) - 1); */
    mask_ls = 0;
  }

  /* printf("MASKS:  %"PRIu64" %"PRIu64"\n", mask_ms, mask_ls); */

  /* most significant */
  start_ms = start_ms & mask_ms;
  end_ms = start_ms | (~mask_ms);
  /* printf("MS:  %"PRIu64" %"PRIu64"\n", start_ms, end_ms); */

  /* least significant */
  start_ls = start_ls & mask_ls;
  end_ls = start_ls | (~mask_ls);
  /* printf("LS:  %"PRIu64" %"PRIu64"\n", start_ls, end_ls); */

  pfx_size = end_ms - start_ms + 1;

  /* intersection endpoints
   * (only most significant)*/
  uint64_t int_start_ms;
  uint64_t int_end_ms;

  while (current != NULL) {
    /* current->start > end */
    if (current->start_ms > end_ms ||
        (current->start_ms == end_ms && current->start_ls > end_ls)) {
      break;
    }
    /* current->end < start */
    if (current->end_ms < start_ms ||
        (current->end_ms == start_ms && current->end_ms < start_ls)) {
      previous = current;
      current = current->next;
      continue;
    }
    /* there is some overlap
     * max(start) and min(end) */
    int_start_ms = current->start_ms;
    int_end_ms = current->end_ms;
    /* int_start_ls = current->start_ls; */
    /* int_end_ls = current->end_ls; */
    /* current->start < start */
    if (current->start_ms < start_ms ||
        (current->start_ms == start_ms && current->start_ms < start_ls)) {
      int_start_ms = start_ms;
      /* int_start_ls = start_ls; */
    }
    /* current->end > end */
    if (current->end_ms > end_ms ||
        (current->end_ms == end_ms && current->end_ls > end_ls)) {
      int_end_ms = end_ms;
      /* int_end_ls = end_ls; */
    }
    if (previous != current) {
      if (current->start_ms != previous->start_ms ||
          current->end_ms != previous->end_ms) {
        if ((int_end_ms - int_start_ms + 1) == pfx_size) {
          *more_specific = 1;
        }
        overlap_count += int_end_ms - int_start_ms + 1;
      }
    } else {
      if ((int_end_ms - int_start_ms + 1) == pfx_size) {
        *more_specific = 1;
      }
      overlap_count += int_end_ms - int_start_ms + 1;
    }
    previous = current;
    current = current->next;
  }
  return overlap_count;
}

uint64_t bgpstream_ip_counter_is_overlapping(bgpstream_ip_counter_t *ipc,
                                             bgpstream_pfx_t *pfx,
                                             uint8_t *more_specific)
{
  *more_specific = 0;
  if (pfx->address.version == BGPSTREAM_ADDR_VERSION_IPV4) {
    return bgpstream_ip_counter_is_overlapping4(
      ipc, (bgpstream_ipv4_pfx_t *)pfx, more_specific);
  } else {
    if (pfx->address.version == BGPSTREAM_ADDR_VERSION_IPV6) {
      return bgpstream_ip_counter_is_overlapping6(
        ipc, (bgpstream_ipv6_pfx_t *)pfx, more_specific);
    }
  }
  return 0;
}

uint64_t bgpstream_ip_counter_get_ipcount(bgpstream_ip_counter_t *ipc,
                                          bgpstream_addr_version_t v)
{
  uint64_t ip_count = 0;

  v4pfx_int_t *current4 = ipc->v4list;
  v6pfx_int_t *current6 = ipc->v6list;
  v6pfx_int_t *previous6 = ipc->v6list;

  if (v == BGPSTREAM_ADDR_VERSION_IPV4) {
    while (current4 != NULL) {
      ip_count += (current4->end - current4->start) + 1;
      current4 = current4->next;
    }
  } else {
    if (v == BGPSTREAM_ADDR_VERSION_IPV6) {
      while (current6 != NULL) {

        if (previous6 != current6) {
          /* add a new /64 to the count if the previous one
           * was different (it could have been a /64+ */
          if (current6->start_ms != previous6->start_ms ||
              current6->end_ms != previous6->end_ms) {
            ip_count += (current6->end_ms - current6->start_ms) + 1;
          }
        } else { /* if it is the first interval, than we always add it */
          ip_count += (current6->end_ms - current6->start_ms) + 1;
        }
        previous6 = current6;
        current6 = current6->next;
      }
    }
  }
  return ip_count;
}

void bgpstream_ip_counter_clear(bgpstream_ip_counter_t *ipc)
{
  v4pfx_int_t *current4 = ipc->v4list;
  v4pfx_int_t *tmp4 = ipc->v4list;
  v6pfx_int_t *current6 = ipc->v6list;
  v6pfx_int_t *tmp6 = ipc->v6list;

  while (tmp4 != NULL) {
    current4 = tmp4;
    tmp4 = tmp4->next;
    free(current4);
  }
  ipc->v4list = NULL;

  while (tmp6 != NULL) {
    current6 = tmp6;
    tmp6 = tmp6->next;
    free(current6);
  }
  ipc->v6list = NULL;
}

void bgpstream_ip_counter_destroy(bgpstream_ip_counter_t *ipc)
{
  bgpstream_ip_counter_clear(ipc);
  free(ipc);
}
