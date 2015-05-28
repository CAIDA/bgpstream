#!/usr/bin/env python

from _pybgpstream import BGPStream, BGPRecord, BGPElem

stream = BGPStream()
rec = BGPRecord()
stream.set_data_interface('singlefile')
stream.set_data_interface_option('singlefile', 'upd-file', './examples/ris.rrc06.updates.1427846400.gz')
stream.start()

elem_cnt = 0
while(stream.get_next_record(rec)):
    elem = rec.get_next_elem()
    while(elem):
        print rec.collector, elem.peer_address, elem.peer_asn, elem.type,
        # apply bgp message
        if elem.type != 'S':
            print elem.fields['prefix']
        else:
            print ""
        # get next element
        elem_cnt += 1
        elem = rec.get_next_elem()

print "Processed elems: ", elem_cnt
