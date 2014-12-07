libemf2svg
==========

[![Build Status](https://travis-ci.org/kakwa/libemf2svg.svg?branch=master)](https://travis-ci.org/kakwa/libemf2svg)

EMF to SVG conversion library

Disclaimer
----------

**Work in progress.**

Output example
--------------

![Example](https://cdn.rawgit.com/kakwa/libemf2svg/master/goodies/demo-example.svg)

Building
--------

Commands to build this project:

```bash

# options: 
# * [-DUSE_CLANG=on]: use clang instead of gcc
# * [-DSTATIC=on]: build static library
# * [-DDEBUG=on]: compile with debugging symboles
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
$ ./emf2svg --help
Usage: emf2svg [OPTION...] -i FILE -o FILE
emf2svg -- Enhanced Metafile to SVG converter

  -h, --height=HEIGHT        Max height in px
  -i, --input=FILE           Input EMF file
  -o, --output=FILE          Output SVG file
  -p, --emfplus              Handle EMF+ records
  -v, --verbose              Produce verbose output
  -w, --width=WIDTH          Max width in px
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Report bugs to <carpentier.pf@gmail.com>.

# usage example:
$ ./emf2svg -i ./tests/resources/emf/test-037.emf -o example.svg -v
```

Library
-------

Shorten usage example ([complete example here](https://github.com/kakwa/libemf2svg/blob/master/goodies/example.c)):

```C
#include <EMFSVG.h>
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

* Check correctness and memleaks (xmllint and valgrind needed):

```bash
# options: -n to disable valgrind tests, -v for verbose output
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
$ cmake -DDEBUG=ON . -DUSE_CLANG=ON &&\
    make &&\
    "./tests/resources/check_correctness.sh" -sxN \
    -e "./tests/resources/emf-corrupted/"
```
