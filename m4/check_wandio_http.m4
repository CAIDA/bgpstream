 SYNOPSIS
#
#   CHECK_WANDIO_HTTP
#
# DESCRIPTION
#
#   This macro checks that WANDIO HTTP read support actually works.
#
# LICENSE
#
# This file is part of bgpstream
#
# CAIDA, UC San Diego
# bgpstream-info@caida.org
#
# Copyright (C) 2015 The Regents of the University of California.
# Authors: Alistair King, Chiara Orsini
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.
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
      [wandio HTTP support required. Ensure you have a working Internet connection and that libcurl is installed before building wandio.]
    )
  ])
  LIBS=$LIBS_STASH

  AC_MSG_RESULT([$with_wandio_http])
])
