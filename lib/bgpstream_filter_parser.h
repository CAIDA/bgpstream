#ifndef BGPSTREAM_FILTER_PARSER_H_
#define BGPSTREAM_FILTER_PARSER_H_

#include "bgpstream.h"

typedef enum {
  FAIL = 0,
  TERM = 1,
  PREFIXEXT = 2,
  VALUE = 3,
  QUOTEDVALUE = 4,
  ENDVALUE = 5
} fp_state_t;

typedef struct single_filter {

  bgpstream_filter_type_t termtype;
  char *value;
} bgpstream_filter_item_t;

#endif
