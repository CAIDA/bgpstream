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
[install libBGPStream](http://bgpstream.caida.org/docs/install/pybgpstream).

Then, you should be able to install PyBGPStream using pip:
~~~
$ pip install pybgpstream
~~~

Alternatively, to install PyBGPStream from source either clone the
[GitHub repository](https://github.com/CAIDA/bgpstream) (PyBGPStream is located
in the `pybgpstream` subdirectory), or download a
[source tarball](http://bgpstream.caida.org/download) and then run:
~~~
$ python setup.py build
# python setup.py install
~~~

For more information about installing PyBGPStream, please see the detailed
[installation instructions](http://bgpstream.caida.org/docs/install/pybgpstream) on the
BGPStream website.

Please see the
[PyBGPStream API documentation](http://bgpstream.caida.org/docs/api/pybgpstream)
and [PyBGPStream tutorial](http://bgpstream.caida.org/docs/tutorials/pybgpstream) for
more information about using PyBGPStream.

