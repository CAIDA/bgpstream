Installation
============

pybgpstream is still in active development so is not available in PyPI, pip,
etc. To install, you will first need to have built and installed
[libbgpstream](https://github.com/caida/bgpstream).
Then, you will need to unzip the pybgpstream distribution and do
something like this:

::

   python setup.py build
   python setup.py install [--user]


If  libbgpstream is not installed into a directory where Python can
find it, you can do something like:

::

   python setup.py build_ext --include-dirs=$HOME/testing/bgpstream/include:$HOME/testing/bgpstream/include --library-dirs=$HOME/testing/bgpstream/lib
   python setup.py install --user

   
You can also build the documentation (`sphinx` is required) like this:

::
   
   cd docs
   make html
   open _build/html/index.html



Work in progress...

See https://pythonhosted.org/netaddr/installation.html for ideas
