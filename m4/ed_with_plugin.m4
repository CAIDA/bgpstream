# SYNOPSIS
#
#   ED_WITH_PLUGIN
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
AC_DEFUN([ED_WITH_PLUGIN],
[
	AC_MSG_CHECKING([whether to build with $2 plugin])

if test "x$4" == xyes; then
        AC_ARG_WITH([$2],
	  [AS_HELP_STRING([--without-$2],
            [build without the $2 plugin])],
            [with_plugin_$2=$with_$2],
            [with_plugin_$2=yes])
else
        AC_ARG_WITH([$2],
	  [AS_HELP_STRING([--with-$2],
            [build with the $2 plugin])],
            [with_plugin_$2=$with_$2],
            [with_plugin_$2=no])
fi

        WITH_PLUGIN_$3=
        AS_IF([test "x$with_plugin_$2" != xno],
              [
		AC_DEFINE([WITH_PLUGIN_$3],[1],[Building with $2])
		ED_PLUGIN_INIT_ALL_ENABLED="${ED_PLUGIN_INIT_ALL_ENABLED}PLUGIN_INIT_ADD($1);"
	      ])
	AC_MSG_RESULT([$with_plugin_$2])

	AM_CONDITIONAL([WITH_PLUGIN_$3], [test "x$with_plugin_$2" != xno])
])
