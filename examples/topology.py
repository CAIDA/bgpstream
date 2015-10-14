#!/usr/bin/env python
#
# This file is part of pybgpstream
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

from _pybgpstream import BGPStream, BGPRecord, BGPElem

# create a new bgpstream instance
stream = BGPStream()

# create a reusable bgprecord instance
rec = BGPRecord()

# configure the stream to retrieve RIB records from the RRC06 collector at
# 2015/05/01 00:00 UTC
stream.add_filter('collector', 'rrc06')
stream.add_filter('record-type', 'ribs')
stream.add_interval_filter(1427846400, 1427846700)

# start the stream
stream.start()

as_topology = set()
rib_entries = 0

# Process data
while(stream.get_next_record(rec)):
     elem = rec.get_next_elem()
     while(elem):
         rib_entries += 1
         # get the AS path
         path = elem.fields['as-path']
         # get the list of ASes in the path
         ases = path.split(" ")
         for i in range(0,len(ases)-1):
             # avoid multiple prepended ASes
             if(ases[i] != ases[i+1]):
                 as_topology.add(tuple(sorted([ases[i],ases[i+1]])))
         # get next elem
         elem = rec.get_next_elem()

# Output results
print "Processed", rib_entries, "rib entries"
print "Found", len(as_topology), "AS adjacencies"
