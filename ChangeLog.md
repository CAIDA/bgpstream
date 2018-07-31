BGPStream Change Log
====================

v1.2.1
------
 - Released 2018-07-30

### Bug Fixes
 - Fixed IPv6 address parsing failure caused by `getaddrinfo` (#34)

v1.2.0
------
 - Released 2018-05-08
 - All users should upgrade to this release. Contact bgpstream-info@caida.org
	for help upgrading.

### Major Features
 - Added filtering language and parser

### Minor Features
 - No longer require time interval to be set (default to process all available data)
 - Improved processing performance by avoid round-robin processing among files
 - Improved code style consistency
 - Add example script showing to use BGPStream with Spark
 - Add an accessor for retrieving origin ASN value
 - Improved documentations

### Bug Fixes
 - Fixed assertion failure problem (#62)
 - Improved bgpdump resiliency when it encounters unusual data
 - Fixed broken PyBGPStream includes
 - Fixed segfaults on 32bit machines caused by the usage of 64bit wandio off_t
 - Removed possible memory leak on pybgpstream
 - Fixed segfault in `BGPStream.get_data_interfaces`
 - Fixed compiler warnings when building with Python2
 - Fixed bugs in patricia tree data structure



v1.1.0
------
 - Released 2016-01-28

### Major Features
 - Added native support for communities.
 - Added proof-of-concept filters for peers, prefixes and communities.

### Minor Features
 - Added various utility functions and data structures, including a patricia tree.
 - Improved testing of various features (make check)
 - Added AS monitor plugin to BGPCorsaro

### Bug Fixes
 - Fixed segmentation fault when dump file could not be opened.
 - Made the broker datasource more resilient to temporary network failures
   (i.e. retry failed downloads)
 - Fixed warning issued by GCC 4.8.4 on Ubuntu 14.04
 - Fixed warning issued when building PyBGPStream on Ubuntu
 - Fixed incorrect return code in `bgpstream_start`

v1.0.0
------
 - Released 2015-10-26
 - Initial public release
