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

AC_PREREQ([2.68])

AC_INIT([bgpstream], [1.2.1], [bgpstream-info@caida.org])

BGPSTREAM_MAJOR_VERSION=1
BGPSTREAM_MID_VERSION=2
BGPSTREAM_MINOR_VERSION=1

LIBBGPSTREAM_MAJOR_VERSION=2
LIBBGPSTREAM_MID_VERSION=0
LIBBGPSTREAM_MINOR_VERSION=0

AC_DEFINE_UNQUOTED([BGPSTREAM_MAJOR_VERSION],$BGPSTREAM_MAJOR_VERSION,
        [bgpstream major version])
AC_DEFINE_UNQUOTED([BGPSTREAM_MID_VERSION],$BGPSTREAM_MID_VERSION,
        [bgpstream mid version])
AC_DEFINE_UNQUOTED([BGPSTREAM_MINOR_VERSION],$BGPSTREAM_MINOR_VERSION,
        [bgpstream minor version])

AC_SUBST([BGPSTREAM_MAJOR_VERSION])
AC_SUBST([BGPSTREAM_MID_VERSION])
AC_SUBST([BGPSTREAM_MINOR_VERSION])

AC_DEFINE_UNQUOTED([LIBBGPSTREAM_MAJOR_VERSION],$LIBBGPSTREAM_MAJOR_VERSION,
        [libbgpstream major version])
AC_DEFINE_UNQUOTED([LIBBGPSTREAM_MID_VERSION],$LIBBGPSTREAM_MID_VERSION,
        [libbgpstream mid version])
AC_DEFINE_UNQUOTED([LIBBGPSTREAM_MINOR_VERSION],$LIBBGPSTREAM_MINOR_VERSION,
        [libbgpstream minor version])

AC_SUBST([LIBBGPSTREAM_MAJOR_VERSION])
AC_SUBST([LIBBGPSTREAM_MID_VERSION])
AC_SUBST([LIBBGPSTREAM_MINOR_VERSION])

LT_INIT

# Check if we should disable rpath.
#
# For advanced users: In certain configurations (e.g. when one of BGPStream's
# dependencies is located in a directory which also holds an old
# libbgpstream.so), the rpath attributes added by libtool cause problems with
# make check (or indeed with any non-installed binaries) as rpath will be
# preferred over LD_LIBRARY_PATH. This does not seem to be a problem with
# clang. When using --disable-rpath you will likely need to set LD_LIBRARY_PATH
# if you are using libraries in non-system locations. YMMV.
#
DISABLE_RPATH

AM_INIT_AUTOMAKE

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_SRCDIR([lib/bgpstream.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_LIBTOOL
AC_PROG_CXX
AM_PROG_LIBTOOL
AC_PROG_CC_C99 # C99 is required because of bool

AC_SYS_LARGEFILE

AH_VERBATIM([_GNU_SOURCE],
[/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif])

AC_CHECK_FUNCS([gettimeofday memset strdup strstr strsep strlcpy vasprintf])

# should we dump debug output to stderr and not optmize the build?

AC_MSG_CHECKING([whether to build with debug information])
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
        [enable debug data generation (def=no)])],
    [debugit="$enableval"],
    [debugit=no])
AC_MSG_RESULT([$debugit])

if test x"$debugit" = x"yes"; then
    AC_DEFINE([DEBUG],[],[Debug Mode])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h limits.h math.h stdlib.h string.h \
			      time.h sys/time.h])

# Checks for mandatory libraries

# this code is needed to get the right threading library on a mac
STASH_CFLAGS="$CFLAGS"
CFLAGS=
AX_PTHREAD(, [AC_MSG_ERROR([pthreads required])])
CFLAGS="$STASH_CFLAGS"
LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

# check that wandio is installed and HTTP support is enabled
AC_SEARCH_LIBS([wandio_create], [wandio], [with_wandio=yes],
               [AC_MSG_ERROR(
  [libwandio required (http://research.wand.net.nz/software/libwandio.php)]
)])
AM_CONDITIONAL([WITH_WANDIO], [test "x$with_wandio" == xyes])

AC_MSG_NOTICE([])
AC_MSG_NOTICE([---- BGPStream configuration ----])

BS_WITH_DI([bgpstream_broker],[broker],[BROKER],[yes])
BS_WITH_DI([bgpstream_singlefile],[singlefile],[SINGLEFILE],[yes])
BS_WITH_DI([bgpstream_csvfile],[csvfile],[CSVFILE],[yes])
BS_WITH_DI([bgpstream_sqlite],[sqlite],[SQLITE],[no])

if test "x$bs_di_valid" != xyes; then
   AC_MSG_ERROR([At least one data interface must be enabled])
fi

# check for data interface dependencies
AC_MSG_NOTICE([checking data interface dependencies...])

if test "x$with_di_broker" == xyes; then
  CHECK_WANDIO_HTTP("http://bgpstream.caida.org/broker")
fi

if test "x$with_di_sqlite" == xyes; then
   # check for sqlite library
   AC_CHECK_LIB([sqlite3], [sqlite3_open_v2], ,
               [AC_MSG_ERROR( [libsqlite3 required for sqlite data interface])])
fi

# configure enabled data interfaces
AC_MSG_NOTICE([configuring data interface parameters...])

# broker options
if test "x$with_di_broker" == xyes; then
   BS_DI_OPT(broker-url, BROKER_URL, Broker URL, https://bgpstream.caida.org/broker)
AC_ARG_WITH([broker-debug],
        [AS_HELP_STRING([--with-broker-debug], [Enable broker debugging output])],
            [with_broker_debug=$with_broker_debug],
            [with_broker_debug=no])
AS_IF([test "x$with_broker_debug" != xno],
	[
	    AC_DEFINE_UNQUOTED([WITH_BROKER_DEBUG],[1],
		[Enable broker debugging output])
	])
fi

# singlefile options
if test "x$with_di_singlefile" == xyes; then
   BS_DI_OPT(singlefile-rib-file, SINGLEFILE_RIB_FILE, RIB MRT file to read, not-set)
   BS_DI_OPT(singlefile-upd-file, SINGLEFILE_UPDATE_FILE, Updates MRT to read, not-set)
fi

# sqlite options
if test "x$with_di_sqlite" == xyes; then
   BS_DI_OPT(sqlite-db-file, SQLITE_DB_FILE, SQLite database, bgp_data.db)
fi

# csvfile options
if test "x$with_di_csvfile" == xyes; then
   BS_DI_OPT(csvfile-csv-file, CSVFILE_CSV_FILE, CSV file listing the MRT data to read, not-set)
fi

AC_MSG_NOTICE([---------------------------------])

# BGPCorsaro configuration
AC_MSG_NOTICE([])
AC_MSG_NOTICE([---- BGPCorsaro configuration ----])
# monitor name
AC_MSG_CHECKING([for the monitor name to use])
AC_ARG_WITH([bgpcorsaro-monitorname],
	[AS_HELP_STRING([--with-bgpcorsaro-monitorname=NAME],
	  [explicitly define the monitor name (defaults to system hostname)])],
	  [if test "x$withval" == xyes || test "x$withval" == ""; then
	  with_monitorname=`hostname`
	  else
	  with_monitorname=$withval
	  fi],
	  [with_monitorname=`hostname`])
AC_MSG_RESULT([$with_monitorname])

AC_DEFINE_UNQUOTED([BGPCORSARO_MONITOR_NAME],[$with_monitorname],[Monitor Name])

# are we going to dump some plugin timing information? (useful for debugging
# where bgpcorsaro spends its time given a complicated plugin chain)
AC_MSG_CHECKING([whether to monitor plugin runtimes])
AC_ARG_WITH([bgpcorsaro-plugin-timing],
	[AS_HELP_STRING([--with-bgpcorsaro-plugin-timing],
	  [time how long each plugin takes to process packets (defaults to no)])],
	  [],
	  [with_plugin_timing=no])

AS_IF([test "x$with_plugin_timing" != xno],
	[
	    AC_DEFINE_UNQUOTED([WITH_PLUGIN_TIMING],[1],
		[Monitor plugin timing])
	])

AC_MSG_RESULT([$with_plugin_timing])

# configure options to disable plugins
#AC_DEFINE([ED_PLUGIN_INIT_ALL_ENABLED],[],[Plugin init functions])

################################################################################
# Plugin Configuration Macros                                                  #
# ---------------------------                                                  #
#                                                                              #
# Each ED_WITH_PLUGIN macro defines a plugin which can be compiled into        #
# Bgpcorsaro.                                                                  #
#                                                                              #
# The order in which these plugins are listed is the _default_ order in which  #
# they will be run.                                                            #
################################################################################

ED_WITH_PLUGIN([bgpcorsaro_pfxmonitor],[pfxmonitor],[PFXMONITOR],[yes])
ED_WITH_PLUGIN([bgpcorsaro_pacifier],[pacifier],[PACIFIER],[yes])
ED_WITH_PLUGIN([bgpcorsaro_asmonitor],[asmonitor],[ASMONITOR],[yes])

# this MUST go after all the ED_WITH_PLUGIN macro calls
AC_DEFINE_UNQUOTED([ED_PLUGIN_INIT_ALL_ENABLED], $ED_PLUGIN_INIT_ALL_ENABLED,
		   [plugins to call the init macro for in bgpcorsaro_plugin.c])

#------------------------------------------------------------------------------#
# Extra tests or config needed for plugins should go below here                #
#------------------------------------------------------------------------------#

AC_MSG_NOTICE([----------------------------------])

# we may want to come back later and add compile-time configuration for things
# like datastructure providers, but for now it will all get compiled

AC_HEADER_ASSERT

AC_CONFIG_FILES([Makefile
                bgpcorsaro/Makefile
                bgpcorsaro/lib/Makefile
                bgpcorsaro/lib/plugins/Makefile
                bgpcorsaro/tools/Makefile
		common/Makefile
		common/libpatricia/Makefile
		common/libinterval3/Makefile
		common/libinterval3/rb_tree/Makefile
		common/libcsv/Makefile
                common/libjsmn/Makefile
		lib/Makefile
                lib/bgpdump/Makefile
                lib/datasources/Makefile
                lib/utils/Makefile
		tools/Makefile
		test/Makefile])
AC_OUTPUT
