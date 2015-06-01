`pybgpstream` is a Python library that provides a high-level interface to an
archive of BGP routing information.

`pybgpstream` provides two Python modules:
 - `_pybgpstream`, a low-level (almost) direct interface to the C API of
   [libbgpstream](https://github.com/caida/bgpstream)
 - `pybgpstream`, a high-level 'Pythonic' interface to the functionality
   provided by `_pybgpstream` (work in progress)


Please take a look at http://www.caida.org/~chiara/bgpstream-doc/pybgpstream/
to read the latest version of the documentation.



Work in Progress...

Quick Start - internal use
-----------

pybgpstream is still in active development so is not available in PyPI, pip,
etc. To install, you will first need to have built and installed
[libbgpstream](https://github.com/caida/bgpstream).
Then, you will need to clone this repository, and then do something like:

~~~
python setup.py build
python setup.py install [--user]
~~~

If  libbgpstream is not installed into a directory where Python can
find it, you can do something like:

~~~
python setup.py build_ext --include-dirs=$HOME/testing/bgpstream/include --library-dirs=$HOME/testing/bgpstream/lib
python setup.py install --user
~~~

You can also build the documentation (`sphinx` is required) like this:

~~~
cd docs
make html

open _build/html/index.html
~~~


To test the installation you can run the test_updates.py example
( if libbgpstream is not in `LD_LIBRARY_PATH` than you will need
`LD_LIBRARY_PATH=$HOME/testing/bgpstream/lib:$LD_LIBRARY_PATH`):

```engine='bash'
python examples/tutorial_print.py
singlefile_ds 202.249.2.185 25152 A 192.108.199.0/24
singlefile_ds 2001:200:0:fe00::6249:0 25152 A 2a02:2158::/32
singlefile_ds 2001:200:0:fe00::6249:0 25152 A 2a02:2158::/32
singlefile_ds 202.249.2.185 25152 A 199.38.164.0/23
singlefile_ds 2001:200:0:fe00::6249:0 25152 A 2a02:2158::/32
singlefile_ds 202.249.2.185 25152 A 202.180.14.0/24
...
singlefile_ds 202.249.2.185 25152 A 205.69.236.0/24
singlefile_ds 202.249.2.185 25152 A 205.73.223.0/24
singlefile_ds 202.249.2.185 25152 A 199.10.192.0/24
singlefile_ds 202.249.2.185 25152 A 205.115.193.0/24
singlefile_ds 202.249.2.185 25152 A 214.1.211.0/24
Processed elems:  1561
```
