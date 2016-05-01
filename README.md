libemf2svg
==========

[![Join the chat at https://gitter.im/kakwa/libemf2svg](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/kakwa/libemf2svg?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Build Status](https://travis-ci.org/kakwa/libemf2svg.svg?branch=master)](https://travis-ci.org/kakwa/libemf2svg)

MS EMF (Enhanced Metafile) to SVG conversion library.

Motivation
----------

By themselves, EMF/EMF+ files are rare in the wild. However, they are frequently embedded inside other MS file formats.

This project was started to properly convert Visio stencils (.VSS) to svg and be able to reuse public stencils 
in other environments than MS Visio (see [libvisio2svg](https://github.com/kakwa/libvisio2svg)).

However this project could be use beyond its original motivations to handle emf blobs in any MS formats.

Output example
--------------

![Example](https://cdn.rawgit.com/kakwa/libemf2svg/master/goodies/demo-example.svg)

Dependencies
------------

* libiconv
* libpng

Installing the dependencies on Debian:

```bash
# compiler
$ apt-get install gcc g++ 
# or 
$ apt-get install clang

# build deps
$ apt-get install cmake

# library deps with their headers
$ apt-get install libpng-dev libc6-dev
```

Installing the dependencies on OS X:
```bash
$ brew install argp-standalone
```

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
| Supported |   36  | [  34%] |
| Partial   |   33  | [  31%] |
| Unused    |    2  | [   1%] |
| Ignored   |   34  | [  32%] |
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
* [GDI](https://msdn.microsoft.com/fr-fr/library/windows/desktop/dd145203(v=vs.85).aspx): GDI specification (clearer than EMF in explaining how it works).
* [SVG](http://www.w3.org/TR/SVG/Overview.html): SVG specifications.

Testing
-------

* Stats on the number of emf records covered:

```bash
$ ./tests/resources/coverage.sh
```

* Fuzzing on the library:

Using American Fuzzy Lop:


```bash
# remove big files from test pool
$ mkdir ./tmp
$ find tests/resources/emf -size +1M -name "*.emf" -exec mv {} ./tmp \; 

# compile with afl compiler
$ cmake -DCMAKE_CXX_COMPILER=afl-clang++ -DCMAKE_C_COMPILER=afl-clang .
$ make

# run afl (see man for more advanced usage)
$ afl-fuzz -i tests/resources/emf -o out/ -t 10000 -- ./emf2svg-conv -i '@@' -o out/ 

# restore the files
mv ./tmp/* tests/resources/emf
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

To reformat/reindent the code (clang-format):

```bash
$ ./goodies/format
```

Contributing
------------

Contribution are welcomed.
Nothing special here, it's the usual "fork; commit(s); pull request".
Only one thing however, run `./goodies/format` (clang-format) before the pull request.
