# SYNOPSIS
#
#   BS_DI_OPT
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
AC_DEFUN([BS_DI_OPT],
[
opt_val=$4
AC_MSG_CHECKING([data interface option: $3])
AC_ARG_WITH([$1],
[AS_HELP_STRING([--with-$1=$opt_val],
  [$3 (defaults to $opt_val)])],
  [opt_val=$withval],
  [])

AC_DEFINE_UNQUOTED([BGPSTREAM_DS_$2],
                    ["$opt_val"], [$3])

AC_MSG_RESULT([$opt_val])
])
