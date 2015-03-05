`pybgpstream` is a Python library that provides a high-level interface to an
archive of BGP routing information.

`pybgpstream` provides two Python modules:
 - `_pybgpstream`, a low-level (almost) direct interface to the C API of
   [libbgpstream](https://github.com/caida/bgpstream)
 - `pybgpstream`, a high-level 'Pythonic' interface to the functionality
   provided by `_pybgpstream`


Quick Start
-----------

pybgpstream is still in active development so is not available in PyPI, pip,
etc. To install, you will first need to have built and installed
[libbgpdump](https://github.com/caida/bgpdump) and
[libbgpstream](https://github.com/caida/bgpstream). Then, you will need to clone
this repository, and then do something like:

~~~
python setup.py build
python setup.py install [--user]
~~~

If libbgpdump or libbgpstream is not installed into a directory where Python can
find it, you can do something like:

~~~
python setup.py build_ext --include-dirs=$HOME/testing/bgpdump/include:$HOME/testing/bgpstream/include --library-dirs=$HOME/testing/bgpstream/lib
python setup.py install --user
~~~

You can also build the documentation (`sphinx` is required) like this:

~~~
cd docs
make html

open _build/html/index.html
~~~


To test the installation you can use the python shell (on `loki` or `thor` you
will need `LD_LIBRARY_PATH=/usr/local/pkg/ioda-tools/lib:$LD_LIBRARY_PATH`):

```python
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
>>>
```
