libemf2svg
==========

![Build status](https://github.com/metanorma/libemf2svg/actions/workflows/ubuntu-build.yml/badge.svg)
![Build status](https://github.com/metanorma/libemf2svg/actions/workflows/macos-build.yml/badge.svg)
![Build status](https://github.com/metanorma/libemf2svg/actions/workflows/windows-build.yml/badge.svg)
[![Coverage Status](https://coveralls.io/repos/github/metanorma/libemf2svg/badge.svg?branch=master)](https://coveralls.io/github/metanorma/libemf2svg?branch=master)

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
* libfontconfig
* libfreetype
* fmem (https://github.com/Snaipe/fmem) -- a cross-platform library for opening memory-backed libc streams
* argp-standalone (https://github.com/bigcat26/argp-standalone) -- a standalone version of the argp argument parsing functions from glibc, Windows only

fmem and argp-standalone libraries are integrated as CMake external projects.  No additional installation or handling is required.

Installing the dependencies on Debian:

```bash
# compiler
apt-get install gcc g++
# or
apt-get install clang

# build deps
apt-get install cmake pkg-config

# library deps with their headers
apt-get install libpng-dev libc6-dev libfontconfig1-dev libfreetype6-dev zlib1g-dev
```

Installing the dependencies on OS X:
```bash
$ brew install argp-standalone
```

Installing the dependencies on RHEL/CentOS/Fedora:
```bash
yum install cmake libpng-devel freetype-devel fontconfig-devel gcc-c++ gcc
```

Installing the dependencies on Windows for MSVC native builds
Dependencies are installed by vcpkg package manager. Installation is implemented as a step of CMake configuration procedure.

Also note that in some rare cases, to properly handle text fields (ETO_GLYPH_INDEX flag), the ttf font
used by the documents must be present and indexed (fontconfig) on your system.

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

Shorten examples:


Conversion from EMF to SVG ([complete example here](https://github.com/kakwa/libemf2svg/blob/master/goodies/example.c)):
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
    /* svg output length */
    size_t svg_out_len = 0;

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

    int ret = emf2svg(emf_content, emf_size, &svg_out, &svg_out_len, options);

    /***********************************************************************/

    //[...]
}
```

Check document for EMF+ record presence ([complete example here](https://github.com/kakwa/libemf2svg/blob/master/goodies/check_emfp.c)):
```C
int main(int argc, char *argv[]){

    /* emf content size */
    size_t emf_size;
    /* emf content */
    char * emf_content;
    /* svg output string */
    char *svg_out = NULL;
    /* svg output length */
    size_t svg_out_len = 0;

    bool emfplus;
    int ret = emf2svg_is_emfplus(emf_content, emf_size, &emfplus);
    if(emfplus)
        fprintf(stdout,"%s contains EMF+ records\n", file_name);
}
```

See [./src/conv/emf2svg.cpp](https://github.com/kakwa/libemf2svg/blob/master/src/conv/emf2svg.cpp) for a real life example.

EMF/EMF+ record type coverage
-----------------------------

EMF RECORDS:

|   Status  | Count | Percent |
|:---------:|:-----:|:-------:|
| Supported |   37  | [  35%] |
| Partial   |   33  | [  31%] |
| Unused    |    2  | [   1%] |
| Ignored   |   33  | [  31%] |
| Total     |  105  |         |

EMF+ RECORDS:

|   Status  | Count | Percent |
|:---------:|:-----:|:-------:|
| Supported |    0  | [   0%] |
| Partial   |    0  | [   0%] |
| Unused    |    0  | [   0%] |
| Ignored   |   85  | [ 100%] |
| Total     |   85  |         |

ChangeLogs
----------

1.3.1: 

* add MSVC 17 (2022) support

1.3.0: 

* add MSVC Windows native build

1.X.X:  (forked to metanorma)

* add support for EMF images without an initial viewport setup
* add handling of EMF images with wrong transformation applied (Wine-generated)

1.1.0:

* add handling of font index encoding
* add fontconfig dependency
* add freetype dependency
* add common variables LIB_INSTALL_DIR, BIN_INSTALL_DIR, INCLUDE_INSTALL_DIR to set install directories

1.0.3:

* Fixing compilation on CentOS 7 (work around argp bug)

1.0.2:

* broken release, please don't use

1.0.1:

* cleaner handling of memstream on OSX (don't install libmemstream, just embed it)

1.0.0:

* better cmake regarding finding dependency libraries (libpng)
* /!\ API break, must pass an additionnal argument to emf2svg function:
```diff
--- a/goodies/old.c
+++ b/goodies/new.c
@@ -22,6 +22,8 @@ int main(int argc, char *argv[]){
     char * emf_content = mmap(0, emf_size, PROT_READ, MAP_PRIVATE, fd, 0);
     /* svg output string */
     char *svg_out = NULL;
+    /* svg output length */
+    size_t svg_out_len;

     /*************************** options settings **************************/

@@ -44,7 +46,7 @@ int main(int argc, char *argv[]){

     /***************************** conversion ******************************/

-    int ret = emf2svg(emf_content, emf_size, &svg_out, options);
+    int ret = emf2svg(emf_content, emf_size, &svg_out, &svg_out_len, options);

     /***********************************************************************/
```
* general cleanup of the project (remove external files not needed)

0.5.1:

* fix build on OS X

0.5.0:

* add alpha layer handling in bitmap blobs conversion
* add brush patterns

0.4.0:

* fix text orientation
* fix origin handling in special case

0.3.0:

* completly rework how the origin is calculated, it now takes correctly into account both viewport and window orgs

0.2.0:

* code reorganization
* add support for ANGLEARC, EMRSTRETCHBLT, EMRBITBLT and more
* add handling of bitmap, RLE4 and RLE8 image blobs
* add some rough handling of clipping forms
* fix text rendering to not collapse spaces

0.1.0:

* first version

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


Y-coordinates repair in EMF files
---------------------------------

In EMF coordinates are specified using an origin (`[0,0]` point) located at
the upper-left corner: x-coordinates increase to the right; y-coordinates
increase from top to bottom.

The SVG coordinate system, on the other hand, uses the same origin (`[0,0]`
point) at the bottom-left corner: x-coordinates increase to the right; but
y-coordinates increase from top to bottom.

Typically, a simple shift of the y-axis through a single SVG/CSS
transformation is used to transform from EMF coordinates to SVG coordinates.

However, under certain circumstances some tools (for instance, SparxSystem
Enterprise Architect in Wine) will generate EMF files with malformed
coordinates. These images have an origin at the top-left corner with
y-coordinates increasing from top to bottom, yet these y-coordinates are
inverted (multiplied by `-1`) to simulate a normal EMF look.

Furthermore, this inversion phenomenon cannot be solved with plain mirroring
as it occurs to all (complex) objects of the hierarchy. For example, text
boxes have only their y-coordinate anchor point mirrored, but the text
direction is set properly.

This specific layout issue cannot be fixed by a single SVG/CSS
transformation, and therefore the processing code is required to detect and
invert only the affected y-coordinates, while keeping other attributes
intact.


Contributing
------------

Contribution are welcomed.
Nothing special here, it's the usual "fork; commit(s); pull request".
Only one thing however, run `./goodies/format` (clang-format) before the pull request.
