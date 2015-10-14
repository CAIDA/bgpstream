# SYNOPSIS
#
#   BS_WITH_DI
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
