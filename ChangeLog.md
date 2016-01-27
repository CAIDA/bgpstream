BGPStream Change Log
====================

v1.1.0
------
 - Released 2016-01-28
 - All users should upgrade to this release. Contact bgpstream-info@caida.org
	for help upgrading.

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
