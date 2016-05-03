/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* vss2svg
 * work based on vss2xhtml from libvisio
 */

// <<<<<<<<<<<<<<<<<<< START ORIGINAL HEADER >>>>>>>>>>>>>>>>>>>>>>>>>>>

/*
 * This file is part of the libvisio project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// <<<<<<<<<<<<<<<<<<< END ORIGINAL HEADER >>>>>>>>>>>>>>>>>>>>>>>>>>>

#include <iostream>
#include <sstream>
#include <string.h>
#include <fstream>
#include <argp.h>
#include "emf2svg.h"

#ifdef MINGW
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

using namespace std;

const char *argp_program_version = E2S_VERSION;

const char *argp_program_bug_address =
    "https://github.com/kakwa/libemf2svg/issues";

static char doc[] = "emf2svg -- Enhanced Metafile to SVG converter";

static struct argp_option options[] = {
    {"verbose", 'v', 0, 0, "Produce verbose output"},
    {"version", 'V', 0, 0, "Print emf2svg version"},
    {"emfplus", 'p', 0, 0, "Handle EMF+ records"},
    {"input", 'i', "FILE", 0, "Input EMF file"},
    {"output", 'o', "FILE", 0, "Output SVG file"},
    {"width", 'w', "WIDTH", 0, "Max width in px"},
    {"height", 'h', "HEIGHT", 0, "Max height in px"},
    {0}};

/* A description of the arguments we accept. */
static char args_doc[] = "-i FILE -o FILE";

struct arguments {
    char *args[2]; /* arg1 & arg2 */
    bool verbose, emfplus, version;
    char *output;
    char *input;
    int width;
    int height;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct arguments *arguments = (struct arguments *)state->input;

    switch (key) {
    case 'v':
        arguments->verbose = 1;
        break;
    case 'p':
        arguments->emfplus = 1;
        break;
    case 'o':
        arguments->output = arg;
        break;
    case 'i':
        arguments->input = arg;
        break;
    case 'w':
        arguments->width = std::atoi(arg);
        break;
    case 'h':
        arguments->height = std::atoi(arg);
        break;
    case 'V':
        arguments->version = 1;
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* Our argp parser. */
static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char *argv[]) {
    struct arguments arguments;
    arguments.width = 0;
    arguments.height = 0;
    arguments.verbose = 0;
    arguments.version = 0;
    arguments.input = NULL;
    arguments.output = NULL;
    arguments.emfplus = 0;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (arguments.version) {
        std::cout << "emf2svg version: " << E2S_VERSION << "\n";
        return 0;
    }

    if (arguments.input == NULL) {
        std::cerr << "[ERROR] "
                  << "Missing --input=FILE argument\n";
        return 1;
    }

    if (arguments.output == NULL) {
        std::cerr << "[ERROR] "
                  << "Missing --output=FILE argument\n";
        return 1;
    }

    #ifdef MINGW
    struct stat fst;
    int fd = open(arguments.input, O_RDONLY);
    if (fd < 0){fprintf(stderr, "file access failed\n"); exit(1);}
    fstat (fd, &fst); 

    /* emf content size */
    size_t emf_size = fst.st_size;   
    char * emf_content = (char *)calloc(1,emf_size);    
    read( fd , emf_content , emf_size );
    close(fd);
    #else
    std::ifstream in(arguments.input);
    if (!in.is_open()) {
        std::cerr << "[ERROR] "
                  << "Impossible to open input file '" << arguments.input
                  << "'\n";
        return 1;
    }
    std::string contents((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());

    #endif
    
    char *svg_out = NULL;
    generatorOptions *options =
        (generatorOptions *)calloc(1, sizeof(generatorOptions));
    options->verbose = arguments.verbose;
    options->emfplus = arguments.emfplus;
    // options->nameSpace = (char *)"svg";
    options->svgDelimiter = true;
    options->imgWidth = arguments.width;
    options->imgHeight = arguments.height;
    
    #ifdef MINGW
    int ret =
        emf2svg(emf_content, emf_size, &svg_out, options);
    free(emf_content);    
    #else
    int ret =
        emf2svg((char *)contents.c_str(), contents.size(), &svg_out, options);
    #endif
    if (ret != 0) {
        std::ofstream out(arguments.output);
        if (!out.is_open()) {
            std::cerr << "[ERROR] "
                      << "Impossible to open output file '" << arguments.output
                      << "'\n";
            return 1;
        }
        out << std::string(svg_out);
        out.close();
    }
    free(svg_out);
    free(options);

    #ifndef MINGW
    in.close();
    #endif
    if (ret == 0)
        return 1;
    return 0;
}
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
