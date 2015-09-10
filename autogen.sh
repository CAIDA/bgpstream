#!/bin/sh
#
# libbgpstream
#
# Chiara Orsini, CAIDA, UC San Diego
# chiara@caida.org
#
# Copyright (C) 2013 The Regents of the University of California.
#
# This file is part of libbgpstream.
#
# libbgpstream is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# libbgpstream is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with libbgpstream.  If not, see <http://www.gnu.org/licenses/>.
#

# The --force option rebuilds the configure script regardless of its timestamp in relation to that of the file configure.ac.
# The option --install copies some missing files to the directory, including the text files COPYING and INSTALL.
# We add -I config and -I m4 so that the tools find the files that we're going to place in those subdirectories instead of in the project root.

autoreconf --force --install -I config -I m4


