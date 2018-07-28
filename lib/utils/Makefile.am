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

SUBDIRS =

AM_CPPFLAGS = -I$(top_srcdir)             \
	      -I$(top_srcdir)/lib/bgpdump \
	      -I$(top_srcdir)/common


noinst_LTLIBRARIES = libbgpstream-utils.la

CONDITIONAL_LIBS=

include_HEADERS= bgpstream_utils.h 		     \
		 bgpstream_utils_addr.h 	     \
		 bgpstream_utils_addr_set.h	     \
		 bgpstream_utils_as_path.h	     \
		 bgpstream_utils_as_path_store.h     \
		 bgpstream_utils_community.h	     \
		 bgpstream_utils_id_set.h     	     \
		 bgpstream_utils_peer_sig_map.h      \
		 bgpstream_utils_pfx.h		     \
		 bgpstream_utils_pfx_set.h	     \
		 bgpstream_utils_str_set.h	     \
		 bgpstream_utils_ip_counter.h	     \
	         bgpstream_utils_patricia.h  \
		 bgpstream_utils_time.h


libbgpstream_utils_la_SOURCES =             \
	bgpstream_utils.h                   \
	bgpstream_utils_addr.c              \
	bgpstream_utils_addr.h              \
	bgpstream_utils_addr_set.c 	    \
	bgpstream_utils_addr_set.h	    \
	bgpstream_utils_as_path.c	    \
	bgpstream_utils_as_path.h	    \
	bgpstream_utils_as_path_store.c	    \
	bgpstream_utils_as_path_store.h	    \
	bgpstream_utils_as_path_int.h	    \
	bgpstream_utils_community.h	    \
	bgpstream_utils_community.c	    \
	bgpstream_utils_community_int.h	    \
	bgpstream_utils_id_set.c     	    \
	bgpstream_utils_id_set.h     	    \
	bgpstream_utils_peer_sig_map.c      \
	bgpstream_utils_peer_sig_map.h      \
	bgpstream_utils_pfx.c		    \
	bgpstream_utils_pfx.h		    \
	bgpstream_utils_pfx_set.c  	    \
	bgpstream_utils_pfx_set.h	    \
	bgpstream_utils_str_set.c  	    \
	bgpstream_utils_str_set.h	    \
	bgpstream_utils_ip_counter.c	    \
	bgpstream_utils_ip_counter.h	    \
	bgpstream_utils_patricia.c	    \
	bgpstream_utils_patricia.h		\
	bgpstream_utils_time.c    \
	bgpstream_utils_time.h


libbgpstream_utils_la_LIBADD = $(CONDITIONAL_LIBS)

ACLOCAL_AMFLAGS = -I m4

CLEANFILES = *~
