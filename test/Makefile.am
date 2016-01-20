#
# This file is part of bgpstream
#
# CAIDA, UC San Diego
# bgpstream-info@caida.org
#
# Copyright (C) 2012 The Regents of the University of California.
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

AM_CPPFLAGS = 	-I$(top_srcdir) \
	 	-I$(top_srcdir)/lib \
	 	-I$(top_srcdir)/lib/utils \
	 	-I$(top_srcdir)/common

TESTS = 				\
	bgpstream-test 			\
	bgpstream-test-filters		\
	bgpstream-test-utils-addr 	\
	bgpstream-test-utils-pfx	\
	bgpstream-test-utils-patricia

check_PROGRAMS =  			\
	bgpstream-test 			\
	bgpstream-test-filters		\
	bgpstream-test-utils-addr 	\
	bgpstream-test-utils-pfx	\
	bgpstream-test-utils-patricia

bgpstream_test_SOURCES = bgpstream-test.c bgpstream_test.h
bgpstream_test_LDADD   = $(top_builddir)/lib/libbgpstream.la

bgpstream_test_filters_SOURCES = bgpstream-test-filters.c bgpstream_test.h
bgpstream_test_filters_LDADD   = $(top_builddir)/lib/libbgpstream.la

bgpstream_test_utils_addr_SOURCES = bgpstream-test-utils-addr.c bgpstream_test.h
bgpstream_test_utils_addr_LDADD   = $(top_builddir)/lib/libbgpstream.la

bgpstream_test_utils_pfx_SOURCES = bgpstream-test-utils-pfx.c bgpstream_test.h
bgpstream_test_utils_pfx_LDADD   = $(top_builddir)/lib/libbgpstream.la

bgpstream_test_utils_patricia_SOURCES = bgpstream-test-utils-patricia.c bgpstream_test.h
bgpstream_test_utils_patricia_LDADD   = $(top_builddir)/lib/libbgpstream.la

ACLOCAL_AMFLAGS = -I m4

CLEANFILES = *~



