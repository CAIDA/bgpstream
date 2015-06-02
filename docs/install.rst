Installation
============

pybgpstream is still in active development so is not available in PyPI, pip,
etc.

It can be downloaded `here 
<http://www.caida.org/~chiara/bgpstream-doc/_pybgpstream-1.0.tar.gz>`_.

To install, you will first need to have built and installed
bgpstream. Then, you will need to unzip the pybgpstream distribution and do
something like this:

::

   python setup.py build
   python setup.py install

Use

::

   python setup.py install --user

to install the library in your home directory.

To check if pybgpstream is installed correctly run the tutorial print script:

::

   $ cd examples
   $ python tutorial_print.py
      valid singlefile_ds.singlefile_ds 1427846570
         W 202.249.2.185 25152 W {'prefix': '144.104.37.0/24'}
      valid singlefile_ds.singlefile_ds 1427846573
	 A 2001:200:0:fe00::6249:0 25152 A {'next-hop': '2001:200:0:fe00::9c1:0', 'prefix': '2a00:bdc0:e004::/48', 'as-path': '25152 2497 6939 47541 28709'}
         ...

The tutorial print script reads the MRT updates file
provided with the distribution ris.rrc06.updates.1427846400.gz and
outputs part of the file based on filters.

-------------
Tips
-------------
If  libbgpstream is not installed into a directory where Python can
find it, you can do something like:

::

   python setup.py build_ext --include-dirs=$HOME/testing/bgpstream/include --library-dirs=$HOME/testing/bgpstream/lib
   python setup.py install [--user]

If you receive this error when running a script:

::
   
   ImportError: libbgpstream.so.1: cannot open shared object file: No such file or directory

then add bgpstream library path to the LD_LIBRARY PATH:

::

   $ export LD_LIBRARY_PATH=$HOME/testing/bgpstream/lib:$LD_LIBRARY_PATH
   $python a_bgpstream_script.py

-------------
Documentation
-------------

You can also build this documentation (`sphinx` is required) using the
following commands:

::
   
   cd docs
   make html
   open _build/html/index.html

