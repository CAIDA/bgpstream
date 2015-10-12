BGPStream
=========

BGPStream is an open-source software framework for the analysis of both
historical and real-time Border Gateway Protocol (BGP) measurement data.

For a detailed description of BGPStream as well as documentation and tutorials,
please visit http://bgpstream.caida.org.

Quick Start
-----------

To get started using BGPStream, either download the latest
[release tarball](http://bgpstream.caida.org/download), or clone the
[GitHub repository](https://github.com/CAIDA/bgpstream).

You will also need the _libcurl_ and
[wandio](http://research.wand.net.nz/software/libwandio.php) libraries installed
before building BGPStream (libcurl **must** be installed prior to building
wandio).

In most cases, the following will be enough to build and install BGPStream:
~~~
$ ./configure
$ make
# make install
~~~

If you cloned BGPStream from GitHub, you will need to run `./autogen.sh` before
`./configure`.

For further information or support, please visit the
[BGPStream website](http://bgpstream.caida.org), or contact
bgpstream-info@caida.org.
