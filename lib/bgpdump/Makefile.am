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

SUBDIRS=

# to allow #include <config.h>
AM_CPPFLAGS= -I$(top_srcdir)

noinst_LTLIBRARIES = libbgpstream-bgpdump.la

IO_SOURCE=bgpdump_cfile_tools_wandio.c

# no wandio source
# IO_SOURCE=bgpdump_cfile_tools.c


libbgpstream_bgpdump_la_SOURCES = 	\
	bgpdump_attr.h    		\
	bgpdump_cfile_tools.h     	\
	$(IO_SOURCE)	  		\
	bgpdump_formats.h 		\
	bgpdump_lib.c     		\
	bgpdump_lib.h     		\
	bgpdump_mstream.c 		\
	bgpdump_mstream.h 		\
	bgpdump_process.c		\
	bgpdump_process.h		\
	bgpdump_util.c			\
	bgpdump_util.h

ACLOCAL_AMFLAGS = -I m4

CLEANFILES = *~
