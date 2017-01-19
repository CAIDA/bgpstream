/**
  cfile_tools.h
  A small library containing tools for dealing with files
  that are compressed with different compressors or not compressed
  at all. The idea is to recognize the compression automagically
  and transparently. Files can be opened for reading or writing,
  but not both. Reading and writing is by different function classes.
  Access is sequential only.

  Copyright (C) 2004 by Arno Wagner <arno.wagner@acm.org>
  Distributed under the Gnu Public License version 2 or the modified
  BSD license (see file COPYING)

  Support for gzip added by Bernhard Tellenbach <bernhard.tellenbach@gmail.com>

  Function prefixes are:
     cfr_ = compressed file read

  Supported:
  Reading:
  - type recognition from file name extension
  - standard input (filename: '-' )
  - no compression
  - bzip2
  - gzip
*/

#ifndef _CFILE_TOOLS_DEFINES
#define _CFILE_TOOLS_DEFINES

#include <stdio.h>
#include <unistd.h>

// Types

struct _CFRFILE {
  int format;  // 0 = not open, 1 = uncompressed, 2 = bzip2, 3 = gzip
  int eof;     // 0 = not eof
  int closed;  // indicates whether fclose has been called, 0 = not yet
  int error1;  // errors from the sytem, 0 = no error
  int error2;  // for error messages from the compressor
  FILE *data1; // for filehandle of the system
  void *data2; // addtional handle(s) of the compressor
  // compressor specific stuff
  int bz2_stream_end; // True when a bz2 stream has ended. Needed since
                      // further reading returns error and not eof.
};

typedef struct _CFRFILE CFRFILE;

// Functions

CFRFILE *cfr_open(const char *path);
int cfr_close(CFRFILE *stream);
size_t cfr_read_n(CFRFILE *stream, void *ptr, size_t bytes);

#endif
