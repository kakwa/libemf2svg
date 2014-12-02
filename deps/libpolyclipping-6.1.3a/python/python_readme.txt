
The clipper.py file included in this distribution is a Python translation of the Clipper Library. 
Because the Python code is interpreted (ie not compiled) it is about 100 times slower than compiled versions of Clipper.
(Even when using the PyPy Just-In-Time compiler the code is still about 50 times slower.)

Alternatively, Maxime Chalon <maxime.chalon@gmail.com> has written a Python extension module for Clipper:
https://sites.google.com/site/maxelsbackyard/home/pyclipper
This module provides a Python interface to the C++ compiled Clipper Library (and runs about 100 times faster than clipper.py).

