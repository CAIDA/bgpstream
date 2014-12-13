from distutils.core import setup, Extension
setup(name="_pybgpstream", version="1.0",
      ext_modules=[
         Extension("_pybgpstream", ["src/_pybgpstream.c"]),
         ])
