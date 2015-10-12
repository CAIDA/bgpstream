 SYNOPSIS
#
#   CHECK_WANDIO
#
# DESCRIPTION
#
#   This macro
#
# LICENSE
#
# This file is part of bgpstream
#
# Copyright (C) 2015 The Regents of the University of California.
# Authors: Alistair King, Chiara Orsini
#
# All rights reserved.
#
# This code has been developed by CAIDA at UC San Diego.
# For more information, contact bgpstream-info@caida.org
#
# This source code is proprietary to the CAIDA group at UC San Diego and may
# not be redistributed, published or disclosed without prior permission from
# CAIDA.
#
# Report any bugs, questions or comments to bgpstream-info@caida.org
#
AC_DEFUN([CHECK_WANDIO_HTTP],
[
  AC_MSG_CHECKING([for wandio HTTP support])

  LIBS_STASH=$LIBS
  LIBS="-lwandio"
  # here we have some version of wandio, check that it has http support
  AC_TRY_RUN([
    #include <wandio.h>
    int main() {
      io_t *file = wandio_create($1);
      return (file == NULL);
    }
  ],
  [with_wandio_http=yes],
  [AC_MSG_ERROR(
      [wandio HTTP support required (install libcurl before building wandio)]
    )
  ])
  LIBS=$LIBS_STASH

  AC_MSG_RESULT([$with_wandio_http])
])
