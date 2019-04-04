from setuptools import setup, Extension

_pybgpstream_module = Extension("_pybgpstream",
                                libraries = ["bgpstream"],
                                sources = ["src/_pybgpstream_module.c",
                                           "src/_pybgpstream_bgpstream.c",
                                           "src/_pybgpstream_bgprecord.c",
                                           "src/_pybgpstream_bgpelem.c"])

setup(name = "pybgpstream",
      description = "A Python interface to BGPStream",
      long_description = "Provides a high-level interface for live and historical BGP data analysis. PyBGPStream requires the libBGPStream C library, available at http://bgpstream.caida.org/download.",
      version = "1.2.3",
      author = "Alistair King",
      author_email = "bgpstream-info@caida.org",
      url="http://bgpstream.caida.org",
      license="GPLv2",
      classifiers=[
          'Development Status :: 5 - Production/Stable',
          'Environment :: Console',
          'Intended Audience :: Science/Research',
          'Intended Audience :: System Administrators',
          'Intended Audience :: Telecommunications Industry',
          'Intended Audience :: Information Technology',
          'License :: OSI Approved :: GNU General Public License v2 (GPLv2)',
          'Operating System :: POSIX',
          ],
      keywords='_pybgpstream pybgpstream bgpstream bgp mrt routeviews route-views ris routing',
      ext_modules = [_pybgpstream_module,])
