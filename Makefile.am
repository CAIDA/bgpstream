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

SUBDIRS = common	\
	  lib		\
	  tools		\
	  bgpcorsaro	\
	  test

AM_CPPFLAGS =

EXTRA_DIST = 	test/sqlite_test.db \
		test/csv_test.csv \
		test/tutorial.c \
		test/routeviews.route-views.jinx.ribs.1427846400.bz2 \
		test/routeviews.route-views.jinx.updates.1427846400.bz2 \
		test/ris.rrc06.updates.1427846400.gz \
		test/ris.rrc06.ribs.1427846400.gz

include_HEADERS =


ACLOCAL_AMFLAGS = -I m4

CLEANFILES = *~

format:
	find . -type f -name "*.[ch]" -not -path "./common/*" -exec \
		clang-format -style=file -i {} \;

.PHONY: format
