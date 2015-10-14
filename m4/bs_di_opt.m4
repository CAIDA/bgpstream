# SYNOPSIS
#
#   BS_DI_OPT
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
