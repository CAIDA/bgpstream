#!/usr/bin/env python
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

from _pybgpstream import BGPStream, BGPRecord, BGPElem

# create a new bgpstream instance
stream = BGPStream()

# create a reusable bgprecord instance
rec = BGPRecord()

# configure the stream to retrieve Updates records from the RRC06 collector
stream.add_filter('collector', 'rrc06')
stream.add_filter('record-type', 'updates')

# select the time interval to process:
# Wed Apr 1 00:02:50 UTC 2015 -> Wed Apr 1 00:04:30
stream.add_interval_filter(1427846570, 1427846670)

# start the stream
stream.start()

# print the stream
while(stream.get_next_record(rec)):
    print rec.status, rec.project +"."+ rec.collector, rec.time
    elem = rec.get_next_elem()
    while(elem):
        print "\t", elem.type, elem.peer_address, elem.peer_asn, \
            elem.type, elem.fields
        elem = rec.get_next_elem()

