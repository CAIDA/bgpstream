Tutorial
========

TODO

The following shows a simple example of using `_pybgpstream` from a Python
interactive shell
::
   >>> from _pybgpstream import BGPStream, BGPRecord, BGPElem
   >>> stream = BGPStream()
   >>> rec = BGPRecord()
   >>> stream.add_filter('project', 'ris')
   >>> stream.add_filter('collector', 'rrc00')
   >>> stream.add_interval_filter(1415664000, 1415750400)
   >>> stream.start()
   >>> stream.get_next_record(rec)
   True
   >>> elem = rec.get_next_elem()
   >>> print elem
   <_pybgpstream.BGPElem object at 0x20007725d0>
   >>> print elem.type
   R
   >>> print elem.fields
   {'next-hop': '193.150.22.1', 'prefix': '0.0.0.0/0', 'as-path': '57381 50304 42708'}
   >>> record_cnt = 0
   >>> elem_cnt = 0
   >>> while(stream.get_next_record(rec)):
   ...     record_cnt += 1
   ...     elem = rec.get_next_elem()
   ...     while(elem):
   ...             elem_cnt += 1
   ...             elem = rec.get_next_elem()
   ...
   >>> print "record_cnt: " + str(record_cnt)
   record_cnt: 8735143
   >>> print "elem_cnt: " + str(elem_cnt)
   elems_cnt: 48068956
