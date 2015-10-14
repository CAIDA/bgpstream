PyBGPStream
===========

PyBGPStream is a Python library that provides a high-level interface for live
and historical BGP data analysis. See http://bgpstream.caida.org for more
information about BGPStream.

PyBGPStream currently provides one Python module, `_pybgpstream`, a low-level
(almost) direct interface to the [libBGPStream](http://bgpstream.caida.org) C
API.

There are plans to add another module, `pybgpstream`, a high-level 'Pythonic'
interface to the functionality provided by `_pybgpstream`.

Quick Start
-----------

To get started using PyBGPStream, first
[install libBGPStream](http://bgpstream.caida.org/docs/installing).

Then, you should be able to install PyBGPStream using pip:
~~~
$ pip install pybgpstream
~~~

To install from source, please see the detailed
[installation instructions](http://bgpstream.caida.org/docs/installing) on the
BGPStream website.

Please see the
[PyBGPStream API documentation](http://bgpstream.caida.org/docs/pybgpstream-api)
and [PyBGPStream tutorial](http://bgpstream.caida.org/tutorials/pybgpstream) for
more information about using PyBGPStream.

