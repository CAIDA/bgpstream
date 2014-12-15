from distutils.core import setup, Extension

_pybgpstream_module = Extension("_pybgpstream",
                                libraries = ["bgpstream"],
                                sources = ["src/_pybgpstream.c"])

setup(name = "_pybgpstream",
      description = "TODO",
      version = "1.0",
      author = "Alistair King",
      author_email = "bgpstream-info@caida.org",
      ext_modules = [_pybgpstream_module,])
