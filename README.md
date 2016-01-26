libemf2svg
==========

[![Join the chat at https://gitter.im/kakwa/libemf2svg](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/kakwa/libemf2svg?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Build Status](https://travis-ci.org/kakwa/libemf2svg.svg?branch=master)](https://travis-ci.org/kakwa/libemf2svg)

MS EMF (Enhanced Metafile) to SVG conversion library.

Motivation
----------

By themselves, EMF/EMF+ files are rare in the wild. However, they are frequently embedded inside other MS file formats.

This project was started to properly convert Visio stencils (.VSS) to svg and be able to reuse public stencils 
in other environments than MS Visio (like inkscape, dia, calligraflow, yED...).

However this project could be use beyond its original motivations to handle emf blobs in any MS formats.

Disclaimer
----------

**Work in progress.**

Output example
--------------

![Example](https://cdn.rawgit.com/kakwa/libemf2svg/master/goodies/demo-example.svg)

Dependencies
------------

* libiconv
* libpng

Building
--------

Commands to build this project:

```bash

# options: 
# * [-DUSE_CLANG=on]: use clang instead of gcc
# * [-DSTATIC=on]: build static library
# * [-DDEBUG=on]: compile with debugging symbols
#
# CMAKE_INSTALL_PREFIX is optional, default is /usr/local/
$ cmake . -DCMAKE_INSTALL_PREFIX=/usr/

# compilation
$ make

# installation
$ make install
```

Command line tool
-----------------

```bash
$ ./emf2svg-conv --help
Usage: emf2svg-conv [OPTION...] -i FILE -o FILE
emf2svg -- Enhanced Metafile to SVG converter

  -h, --height=HEIGHT        Max height in px
  -i, --input=FILE           Input EMF file
  -o, --output=FILE          Output SVG file
  -p, --emfplus              Handle EMF+ records
  -v, --verbose              Produce verbose output
  -w, --width=WIDTH          Max width in px
  -?, --help                 Give this help list
      --usage                Give a short usage message
      --version              Print program version
  -V, --version              Print emf2svg version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Report bugs to https://github.com/kakwa/libemf2svg/issues.

# usage example:
$ ./emf2svg-conv -i ./tests/resources/emf/test-037.emf -o example.svg -v
```

Library
-------

Shorten example ([complete example here](https://github.com/kakwa/libemf2svg/blob/master/goodies/example.c)):

```C
#include <emf2svg.h>
//[...]
int main(int argc, char *argv[]){

    /* emf content size */
    size_t emf_size;
    /* emf content */
    char * emf_content;
    /* svg output string */
    char *svg_out = NULL;

    //[...]

    /*************************** options settings **************************/

    /* allocate the options structure) */
    generatorOptions *options = (generatorOptions *)calloc(1, \
            sizeof(generatorOptions));
    /* debugging flag (prints the emf record in stdout if true) */
    options->verbose = true;
    /* emf+ flag (handles emf+ records if true) */
    options->emfplus = true;
    /* if a custom xml/svg namespace is needed (keep empty in doubt) */
    options->nameSpace = (char *)"svg";
    /* includes the svg start and stop tags (set to false if the result
     * of this call is meant to be used inside another svg) */
    options->svgDelimiter = true;
    /* image width in px (set to 0 to use the original emf device width) */
    options->imgWidth = 0;
    /* image height in px (set to 0 to use the original emf device height) */
    options->imgHeight = 0;

    /***************************** conversion ******************************/

    int ret = emf2svg(emf_content, emf_size, &svg_out, options);

    /***********************************************************************/

    //[...]
}
```

See [./src/conv/emf2svg.cpp](https://github.com/kakwa/libemf2svg/blob/master/src/conv/emf2svg.cpp) for a real life example.

EMF/EMF+ record type coverage
-----------------------------

EMF RECORDS:

|   Status  | Count | Percent |
|:---------:|:-----:|:-------:|
| Supported |   30  | [  28%] |
| Partial   |   31  | [  29%] |
| Unused    |    2  | [   1%] |
| Ignored   |   42  | [  40%] |
| Total     |  105  |         |

EMF+ RECORDS:

|   Status  | Count | Percent |
|:---------:|:-----:|:-------:|
| Supported |    0  | [   0%] |
| Partial   |    0  | [   0%] |
| Unused    |    0  | [   0%] |
| Ignored   |   85  | [ 100%] |
| Total     |   85  |         |

Development
-----------

General source code organisation:

* [./src/lib/emf2svg.c](https://github.com/kakwa/libemf2svg/blob/master/src/lib/emf2svg.c): API entry point.
* [./src/lib/emf2svg_rec_*](https://github.com/kakwa/libemf2svg/blob/master/src/lib/): EMF record handlers
* [./src/lib/emf2svg_print.c](https://github.com/kakwa/libemf2svg/blob/master/src/lib/emf2svg_print.c): EMF record printer (debugging).
* [./src/lib/pmf2svg.c](https://github.com/kakwa/libemf2svg/blob/master/src/lib/pmf2svg.c): EMF+ record handler.
* [./src/lib/pmf2svg_print.c](https://github.com/kakwa/libemf2svg/blob/master/src/lib/pmf2svg_print.c): EMF+ record printer (debugging).
* [./src/conv/emf2svg.cpp](https://github.com/kakwa/libemf2svg/blob/master/src/conv/emf2svg.cpp): Command line tool.
* [./deps](https://github.com/kakwa/libemf2svg/blob/master/deps): external dependencies.

Useful links:

* [MS-EMF](http://msdn.microsoft.com/en-us/library/cc230514.aspx): EMF specifications.
* [MS-EMF+](http://msdn.microsoft.com/en-us/library/cc230724.aspx): EMF+ specifications.
* [MS-WMF](http://msdn.microsoft.com/en-us/library/cc250370.aspx): WMF specifications.
* [SVG](http://www.w3.org/TR/SVG/Overview.html): SVG specifications.

Testing
-------

* Stats on the number of emf records covered:

```bash
$ ./tests/resources/coverage.sh
```

* Fuzzing on the library:

```bash
$ ./tests/resources/check_corrupted.sh

# generated corrupted files crashing the library are stored here:
ls ./tests/out/bad*
tests/out/bad_corrupted_2014-12-01-063258.emf

```

* Check correctness and memory leaks (xmllint and valgrind needed):

```bash
# options: -n to disable valgrind tests, -v for verbose output 
# see -h for complete list of options
$ ./tests/resources/check_correctness.sh #[-n] [-v]

# generated svg:
$ ls tests/out/test-*
tests/out/test-000.emf.svg  tests/out/test-051.emf.svg
[...]
```

The emf files used for these checks are located in [./tests/resources/emf/](https://github.com/kakwa/libemf2svg/blob/master/tests/resources/emf/).

Useful Commands
---------------

To build, run on emf test files and visualize (with geeqie):
```bash
$ cmake .&& \
    make &&\
    "./tests/resources/check_correctness.sh" -n &&\
    geeqie "tests/out"
```

To check against corrupted emf:
```bash
$ cmake -DDEBUG=ON . &&\
    make &&\
    "./tests/resources/check_correctness.sh" -sxN \
    -e "./tests/resources/emf-corrupted/"
```

To print records index in svg as comments:

```bash
$ cmake -DINDEX=ON . && make
```
