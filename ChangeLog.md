BGPStream Change Log
====================

v1.1.0
------
 - Released 2016-01-28
 - All users should upgrade to this release. Contact bgpstream-info@caida.org
	for help upgrading.

# Major Features
 - Adds native support for communities.
 - Adds proof-of-concept filters for peers, prefixes and communities.

# Minor Features
 - Adds various utility functions, including patricia tree.
 - Improved testing of various features (make check)
 - Adds AS monitor plugin to BGPCorsaro

# Bug Fixes
 - Fix segmentation fault when dump file could not be opened.
 - Make the broker datasource more resilient to temporary network failures
   (i.e. retry failed downloads)
 - Fix warning issued by GCC 4.8.4 on Ubuntu 14.04
 - Fix warning issued when building PyBGPStream on Ubuntu
 - Fix incorrect return code in `bgpstream_start`

v1.0.0
------
 - Released 2015-10-26
 - Initial public release
