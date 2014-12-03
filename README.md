libemf2svg
==========

[![Build Status](https://travis-ci.org/kakwa/libemf2svg.svg?branch=master)](https://travis-ci.org/kakwa/libemf2svg)

EMF to SVG conversion library

**Work in progress.**

![SVG example](https://cdn.rawgit.com/kakwa/libemf2svg/master/goodies/out-example.svg)

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

usage example:

```C
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <EMFSVG.h>

/* to compile: gcc -Wall -std=c99 -lm -lEMFSVG ./example.c -o test */
int main(int argc, char *argv[]){

    if (argc != 2){fprintf(stderr, "file missing\n"); exit(1);}

    /* emf content */
    char *emf_content;
    /* emf content size */
    size_t  emf_size;

    /* allocate the options structure) */
    generatorOptions *options = (generatorOptions *)calloc(1, 
        sizeof(generatorOptions));
    /* debugging flag (prints the emf record in stdout if true) */
    options->verbose = true;
    /* emf+ flag (handles emf+ records if true) */
    options->emfplus = true;
    /* if a custom xml/svg namespace is needed (keep empty in doubt) */
    options->nameSpace = (char *)"svg";
    /* includes the svg start and stop balise
       (set to false if the result of this call is meant to be used in another svg) */
    options->svgDelimiter = true;
    /* image width in px (set to 0 to use the original emf device width) */
    options->imgWidth = 0;
    /* image height in px (set to 0 to use the original emf device height) */
    options->imgHeight = 0;

    /* svg output string */
    char *svg_out = NULL;

    // quick and dirty way to load the file in memory
    struct stat s; const char * file_name = argv[1];
    int fd = open(file_name, O_RDONLY);
    if (fd < 0){fprintf(stderr, "file access failed\n"); exit(1);}
    fstat (fd, &s); emf_size = s.st_size;
    emf_content = mmap(0, emf_size, PROT_READ, MAP_PRIVATE, fd, 0);

    fprintf(stdout,"##### BEGIN CONVERSION ####\n");
    /* conversion function */
    int ret = emf2svg(emf_content, emf_size, &svg_out, options);
    /* end conversion */
    fprintf(stdout,"#####  END CONVERSION  ####\n");

    // do something with the generated SVG
    fprintf(stdout,"#####  BEGIN SVG DUMP  ####\n");
    fprintf(stdout,"%s", svg_out);
    fprintf(stdout,"#####   END SVG DUMP   ####\n");

    // free the allocated structures
    free(svg_out);
    free(options);
    //close file and free content
    close(fd);
    munmap(emf_content, emf_size);
    //return
    exit(!ret);
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
```
cmake .&&make&&./tests/resources/check_correctness.sh -n&&geeqie tests/out
```
