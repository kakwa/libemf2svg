libemf2svg
==========

[![Build Status](https://travis-ci.org/kakwa/libemf2svg.svg?branch=master)](https://travis-ci.org/kakwa/libemf2svg)

EMF to SVG conversion library

**Work in progress.**

Building
--------

Commands to build this project:

```bash

# options: 
# * [-DUSE_CLANG=on]: use clang instead of gcc
# * [-DSTATIC=on]: build static library
# * [-DDEBUG=on]: compile with debugging symboles
$ cmake .

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

usage example:

```C

#include "EMFSVG.h"
#include "PMFSVG.h"
#include "uemf_utf.h"
#include "upmf.h"

// allocate the options structure)
generatorOptions * options = (generatorOptions *)calloc(1,sizeof(generatorOptions));
// debugging flag (prints the emf record in stdout if true)
options->verbose = false;
// emf+ flag (handles emf+ records or not)
options->emfplus = false;
// if a custom xml/svg namespace is needed (keep empty in doubt)
options->nameSpace = (char *)"svg";
// includes the svg start and stop balise 
// (set to false if the result of this call is meant to be used in another svg)
options->svgDelimiter = true;
// image width (set to 0 to use the original emf device width)
options->imgWidth = arguments.width;
// image height (set to 0 to use the original emf device height)
options->imgHeight = arguments.height;

// emf content
char *emf_content;
// emf content size
int  emf_size;
// svg output string
char *svg_out = NULL;
// conversion function
int ret = emf2svg(emf_content, emf_size, &svg_out, options);
// free the allocated structures
free(svg_out);
free(options);
```

Test tools:

stats on the number of emf records covered:

```bash
$ ./tests/resources/coverage.sh
```

fuzzing on the library:

```bash
$ ./tests/resources/check_corrupted.sh

# generated corrupted files crashing the library are stored here:
ls ./tests/out/bad*
tests/out/bad_corrupted_2014-12-01-063258.emf  tests/out/bad_corrupted_2014-12-01-070313.emf

```

check correctness and memleaks:

```bash
# options: -n to disable valgrind tests, -v for verbose output
$ ./tests/resources/check_correctness.sh #[-n] [-v]

# generated svg:

$ ls tests/out/test-*
tests/out/test-000.emf.svg  tests/out/test-051.emf.svg  tests/out/test-102.emf.svg
[...]
```

The emf files used for these checks are in *./tests/resources/emf/*.
