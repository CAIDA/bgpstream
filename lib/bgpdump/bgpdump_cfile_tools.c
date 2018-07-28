/**
  cfile_tools.c

  A library to deal transparently with possibly compressed files.
  Documentation in the function headers and in cfile_tools.h

  Copyright (C) 2004 by Arno Wagner <arno.wagner@acm.org>
  Distributed under the Gnu Public License version 2 or the modified
  BSD license (see file COPYING)

  Support for gzip added by Bernhard Tellenbach <bernhard.tellenbach@gmail.com>
*/

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

//#ifndef DONT_HAVE_BZ2
//#include <bzlib.h>
//#endif

#include "bgpdump_cfile_tools.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// Concrete formats. remember to adjust CFR_NUM_FORMATS if changed!
// Note: 0, 1 are special entries.

const char *cfr_formats[CFR_NUM_FORMATS] = {
  "not open",     //  0
  "uncompressed", //  1
#ifndef DONT_HAVE_BZ2
  "bzip2", //  2
#endif
#ifndef DONT_HAVE_GZ
  "gzip", //  3
#endif
};

const char *cfr_extensions[CFR_NUM_FORMATS] = {
  "", //  0
  "", //  1
#ifndef DONT_HAVE_BZ2
  ".bz2", //  2
#endif
#ifndef DONT_HAVE_GZ
  ".gz" //  3
#endif
};

// Prototypes of non API functions (don't use these from outside this file)
const char *_cfr_compressor_strerror(int format, int err);
const char *_bz2_strerror(int err);

// API Functions

CFRFILE *cfr_open(const char *path)
{
  /*******************************/
  // Analog to 'fopen'. Error in result has to be tested using
  // 'cfr_error' on the result!
  // Note: The user needs to free the reurn value!
  // Opens a possibly compressed file for reading.
  // File type is determined by file name ending

  int format, ext_len, name_len;
  CFRFILE *retval = NULL;

  // determine file format
  name_len = strlen(path);
  format = 2; // skip specials 0, 1

  // Do action dependent on file format
  retval = (CFRFILE *)calloc(1, sizeof(CFRFILE));
  retval->eof = 0;
  retval->error1 = 0;
  retval->error2 = 0;

#ifndef DONT_HAVE_GZ
  if ((path == NULL) || (strcmp(path, "-") == 0)) {
    /* dump from stdin */
    gzFile f;
    while (format < CFR_NUM_FORMATS) {
      if (strcmp(cfr_extensions[format], ".gz") == 0)
        break;
      format++;
    }
    f = gzdopen(0, "r");
    if (f == NULL) {
      free(retval);
      return (NULL);
    }
    retval->data2 = f;
    retval->format = format;
    return (retval);
  }
#endif
  while (format < CFR_NUM_FORMATS) {
    ext_len = strlen(cfr_extensions[format]);
    if (strncmp(cfr_extensions[format], path + (name_len - ext_len), ext_len) ==
        0)
      break;
    format++;
  }
  if (format >= CFR_NUM_FORMATS)
    format = 1; // uncompressed
  retval->format = format;

  switch (format) {
  case 1: // uncompressed
  {
    FILE *in;
    in = fopen(path, "r");
    if (in == NULL) {
      free(retval);
      return (NULL);
    }
    retval->data1 = in;
    return (retval);
  } break;
#ifndef DONT_HAVE_BZ2
  case 2: // bzip2
  {
    int bzerror;
    BZFILE *bzin;
    FILE *in;

    retval->bz2_stream_end = 0;

    // get file
    in = fopen(path, "r");
    if (in == NULL) {
      free(retval);
      return (NULL);
    }
    retval->data1 = in;

    // bzip2ify file
    bzin = BZ2_bzReadOpen(&bzerror, in, 0, 0, NULL, 0);
    if (bzerror != BZ_OK) {
      errno = bzerror;
      BZ2_bzReadClose(&bzerror, bzin);
      fclose(in);
      free(retval);
      return (NULL);
    }
    retval->data2 = bzin;
    return (retval);
  } break;
#endif
#ifndef DONT_HAVE_GZ
  case 3: // gzip
  {
    gzFile f;
    // get file
    f = gzopen(path, "r");
    if (f == NULL) {
      free(retval);
      return (NULL);
    }
    retval->data2 = f;
    return (retval);
  } break;
#endif
  default: // this is an internal error, no diag yet.
    fprintf(stderr, "illegal format '%d' in cfr_open!\n", format);
    exit(1);
  }
  return NULL;
}

int cfr_close(CFRFILE *stream)
{
  /**************************/
  // Analog to 'fclose'.
  // FIXME - why is stream->* set, then freed?
  if (stream == NULL || stream->closed) {
    errno = EBADF;
    return -1;
  }

  int retval = -1;

  switch (stream->format) {
  case 1: // uncompressed
    retval = fclose((FILE *)(stream->data1));
    stream->error1 = retval;
    break;
  case 2: // bzip2
#ifndef DONT_HAVE_BZ2
    BZ2_bzReadClose(&stream->error2, (BZFILE *)stream->data2);
    stream->error1 = retval = fclose((FILE *)(stream->data1));
    break;
#endif
  case 3: // gzip
#ifndef DONT_HAVE_GZ
    if (stream->data2 != NULL)
      retval = gzclose(stream->data2);
    stream->error2 = retval;
    break;
#endif
  default: // internal error
    assert("illegal stream->format" && 0);
  }
  free(stream);
  return (retval);
}

size_t cfr_read_n(CFRFILE *stream, void *ptr, size_t bytes)
{
  /******************************************************************/
  // Wrapper, will return either 'bytes' (the number of bytes to read) or 0
  return (cfr_read(ptr, bytes, 1, stream) * bytes);
}

size_t cfr_read(void *ptr, size_t size, size_t nmemb, CFRFILE *stream)
{
  /******************************************************************/
  // Analog to 'fread'. Will not return with partial elements, only
  // full ones. Hence calling this function with one large element
  // size will result in a complete or no read.

  size_t retval = 0;
  if (stream == NULL)
    return (0);

  // shortcut
  if (stream->eof)
    return (0);

  switch (stream->format) {
  case 1: // uncompressed
  {
    FILE *in;
    in = (FILE *)(stream->data1);
    retval = fread(ptr, size, nmemb, in);
    if (retval != nmemb) {
      // fprintf(stderr,"short read!!!\n");
      stream->eof = feof(in);
      stream->error1 = ferror(in);
      retval = 0;
    }
    return (retval);
  } break;
#ifndef DONT_HAVE_BZ2
  case 2: // bzip2
  {
    BZFILE *bzin;
    int bzerror;
    int buffsize;

    if (stream->bz2_stream_end == 1) {
      // feof-behaviour: Last read did consume last byte but not more
      stream->eof = 1;
      return (0);
    }

    bzerror = BZ_OK;
    bzin = (BZFILE *)(stream->data2);
    buffsize = size * nmemb;

    retval = BZ2_bzRead(&bzerror, bzin, ptr, buffsize);

    if (bzerror == BZ_STREAM_END) {
      stream->bz2_stream_end = 1;
      stream->error2 = bzerror;
      if (retval == buffsize) {
        // feof-behaviour: no eof yet
      } else {
        // feof-behaviour: read past end, set eof
        stream->eof = 1;
        retval = 0;
      }
      return (retval / size);
    }
    if (bzerror == BZ_OK) {
      // Normal case, no error.
      // A short read here is an error, so catch it
      if (retval == buffsize) {
        return (retval / size);
      }
    }

    // Other error...
    stream->error2 = bzerror;
    BZ2_bzReadClose(&bzerror, bzin);
    if (bzerror != BZ_OK) {
      stream->error2 = bzerror;
    }
    retval = fclose((FILE *)(stream->data1));
    stream->error1 = retval;
    stream->closed = 1;
    return (0);
  } break;
#endif
#ifndef DONT_HAVE_GZ
  case 3: // gzip
  {
    gzFile in;
    in = (gzFile)(stream->data2);
    retval = gzread(in, ptr, size * nmemb);
    if (retval != nmemb * size) {
      // fprintf(stderr,"short read!!!\n");
      stream->eof = gzeof(in);
      stream->error2 = errno;
      retval = 0;
    }
    return (retval / size);
  } break;
#endif
  default: // this is an internal error, no diag yet.
    fprintf(stderr, "illegal format '%d' in cfr_read!\n", stream->format);
    exit(1);
  }
}

ssize_t cfr_getline(char **lineptr, size_t *n, CFRFILE *stream)
{
  /************************************************************/
  // May not be very efficient, since it uses single-char reads
  // for formats where there is no native getline in the library.
  // For bzip2 the speedup for additional buffering was only 5%
  // so I dropped it.
  // Returns -1 in case of an error.

  if (stream == NULL)
    return (-1);

  switch (stream->format) {
  case 1: // uncompressed
  {
    if (fgets(*lineptr, *n, (FILE *)(stream->data1)) == NULL) {
      stream->error1 = errno;
      return -1;
    }
    return 0;
  } break;

#ifndef DONT_HAVE_BZ2
  case 2: // bzip2
  {
    size_t count;
    char c;
    size_t ret;

    // bzin = (BZFILE *) (stream->data2);

    // allocate initial buffer if none was passed or size was zero
    if (*lineptr == NULL) {
      *lineptr = (char *)calloc(120, 1);
      *n = 120;
    }
    if (*n == 0) {
      *n = 120;
      *lineptr = (char *)realloc(*lineptr, *n); // to avoid memory-leaks
    }

    count = 0;
    // read until '\n'
    do {
      ret = cfr_read(&c, 1, 1, stream);
      if (ret != 1) {
        return (-1);
      }
      count++;
      if (count >= *n) {
        *n = 2 * *n;
        *lineptr = (char *)realloc(*lineptr, *n);
        if (*lineptr == NULL) {
          stream->error1 = errno;
          return (-1);
        }
      }
      (*lineptr)[count - 1] = c;
    } while (c != '\n');
    (*lineptr)[count] = 0;
    return (count);
  } break;
#endif
#ifndef DONT_HAVE_GZ
  case 3: // gzip
  {
    char *return_ptr = gzgets((gzFile)(stream->data2), *lineptr, *n);
    if (return_ptr == Z_NULL) {
      stream->error2 = errno;
      return (-1);
    }
    return *n;

  } break;
#endif
  default: // this is an internal error, no diag yet.
    fprintf(stderr, "illegal format '%d' in cfr_getline!\n", stream->format);
    exit(1);
    return (-1);
  }
}

int cfr_eof(CFRFILE *stream)
{
  // Returns true on end of file/end of compressed data.
  // The end of the compressed data is regarded as end of file
  // in this library, embedded or multiple compressed data per
  // file is not supported by this library.
  //
  // Note: The sematics is that cfr_eof is true only after
  // the first byte after the end of file was read. Some compressors
  // report EOF already when the last availale character has been
  // read (far more sensible IMO), but for consistency we follow the
  // convention of the standard c library here.

  return (stream->eof);
}

int cfr_error(CFRFILE *stream)
{
  // Returns true on error.
  // Errors can be ordinary errors from fopen.fclose/fread
  // or can originate from the underlying compression.
  // This function just returns 0 when there is no error or
  // 1 in case of error.
  // To get a more detailed report cfr_strerror will try to
  // come up with a description of the whole situation.
  // For numeric details, more query functions would need to be
  // implemented.

  if (stream == NULL)
    return (1);
  return (stream->error1 || stream->error2);
}

char *cfr_strerror(CFRFILE *stream)
{
  // Result is "stream-i/o: <stream-error> <compressor>[: <compressor error>]"
  // Do not modify result.
  // Result may change on subsequent call to this function.

  static char res[120];
  char *msg, *msg2;

  if (stream == NULL) {
    asprintf(&msg, "Error: stream is NULL, i.e. not opened");
    return (msg);
  }

  asprintf(&msg, "stream-i/o: %s, %s  [%s]", stream->eof ? "EOF" : "",
           strerror(stream->error1), cfr_compressor_str(stream));
  if (stream->format == 2) {
    asprintf(&msg2, "%s: %s", msg,
             _cfr_compressor_strerror(stream->format, stream->error2));
    free(msg);
    msg = msg2;
  }
  if (stream->format == 3) {
    asprintf(&msg2, "%s: %s", msg,
             gzerror((gzFile)(stream->data2), &(stream->error2)));
    free(msg);
    msg = msg2;
  }
  snprintf(res, 120, "%s", msg);
  res[119] = 0;
  free(msg);
  return (res);
}

const char *cfr_compressor_str(CFRFILE *stream)
{
  // Returns the name of the compressor used

  if ((stream->format < 0) || (stream->format >= CFR_NUM_FORMATS)) {
    return ("undefined compression type");
  } else {
    return (cfr_formats[stream->format]);
  }
}

// Utility functions for compressor errors.
// * Not part of the API, do not call directly as they may change! *

const char *_cfr_compressor_strerror(int format, int err)
{
  // Transforms error code to string for all compressors

  switch (format) {
  case 0:
    return ("file not open");
    break;

  case 1:
    return ("file not compressed");
    break;

#ifndef DONT_HAVE_BZ2
  case 2:
    return (_bz2_strerror(err));
    break;
#endif
#ifndef DONT_HAVE_GZ
  case 3:
    return NULL;
    break;
#endif
  default:
    return ("unknowen compressor code");
  }
}

#ifndef DONT_HAVE_BZ2
const char *_bz2_strerror(int err)
{
  // Since bzlib does not have strerror, we do it here manually.
  // This works for version 1.0 of 21 March 2000 of bzlib.h

  switch (err) {
  case BZ_OK:
    return ("BZ_OK");
  case BZ_RUN_OK:
    return ("BZ_RUN_OK");
  case BZ_FLUSH_OK:
    return ("BZ_FLUSH_OK");
  case BZ_FINISH_OK:
    return ("BZ_FINISH_OK");
  case BZ_STREAM_END:
    return ("BZ_STREAM_END");
  case BZ_SEQUENCE_ERROR:
    return ("BZ_SEQUENCE_ERROR");
  case BZ_PARAM_ERROR:
    return ("BZ_PARAM_ERROR");
  case BZ_MEM_ERROR:
    return ("BZ_MEM_ERROR");
  case BZ_DATA_ERROR:
    return ("BZ_DATA_ERROR");
  case BZ_DATA_ERROR_MAGIC:
    return ("BZ_DATA_ERROR_MAGIC");
  case BZ_IO_ERROR:
    return ("BZ_IO_ERROR");
  case BZ_UNEXPECTED_EOF:
    return ("BZ_UNEXPECTED_EOF");
  case BZ_OUTBUFF_FULL:
    return ("BZ_OUTBUFF_FULL");
  case BZ_CONFIG_ERROR:
    return ("BZ_CONFIG_ERROR");
  default:
    return ("unknowen bzip2 error code");
  }
}
#endif
