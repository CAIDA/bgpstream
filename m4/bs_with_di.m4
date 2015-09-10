# SYNOPSIS
#
#   BS_WITH_DI
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
AC_DEFUN([BS_WITH_DI],
[
	AC_MSG_CHECKING([whether to build with data interface: $2])

if test "x$4" == xyes; then
        AC_ARG_WITH([$2],
	  [AS_HELP_STRING([--without-$2],
            [build without the $2 data interface])],
            [with_di_$2=$with_$2],
            [with_di_$2=yes])
else
        AC_ARG_WITH([$2],
	  [AS_HELP_STRING([--with-$2],
            [build with the $2 data interface])],
            [with_di_$2=$with_$2],
            [with_di_$2=no])
fi

        WITH_DATA_INTERFACE_$3=
        AS_IF([test "x$with_di_$2" != xno],
              [
		AC_DEFINE([WITH_DATA_INTERFACE_$3],[1],[Building with $2])
		BS_DI_INIT_ALL_ENABLED="${BS_DI_INIT_ALL_ENABLED}DI_INIT_ADD($1);"
                bs_di_valid=yes
	      ])
	AC_MSG_RESULT([$with_di_$2])

	AM_CONDITIONAL([WITH_DATA_INTERFACE_$3], [test "x$with_di_$2" != xno])
])
