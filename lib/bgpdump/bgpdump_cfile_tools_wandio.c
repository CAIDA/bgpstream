/**
 * Wrapper for libwandio that implements enough of the cfile_tools api to
 * satisfy bgpdump.
 *
 * Author: Alistair King <alistair@caida.org>
 * 2014-08-14
 */

#include "bgpdump_cfile_tools.h"
#include "config.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <wandio.h>

#define WFILE(x) ((io_t *)(x)->data2)

// API Functions

CFRFILE *cfr_open(const char *path)
{
  CFRFILE *cfr = NULL;

  if ((cfr = malloc(sizeof(CFRFILE))) == NULL) {
    return NULL;
  }
  memset(cfr, 0, sizeof(CFRFILE));

  /* sweet hax */
  if ((cfr->data2 = wandio_create(path)) == NULL) {
    free(cfr);
    return NULL;
  }

  return cfr;
}

int cfr_close(CFRFILE *stream)
{
  if (stream == NULL) {
    return 0;
  }

  wandio_destroy(WFILE(stream));
  stream->closed = 1;

  free(stream);
  return 0;
}

size_t cfr_read_n(CFRFILE *stream, void *ptr, size_t bytes)
{
  return wandio_read(WFILE(stream), ptr, bytes);
}
