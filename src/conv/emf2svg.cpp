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
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <string> 
#include <stdlib.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "EMFSVG.h"
#include "PMFSVG.h"
#include "uemf_utf.h"
#include "upmf.h"

using namespace std;

const char *argp_program_version = "emf2svg 1.0";

const char *argp_program_bug_address = "<carpentier.pf@gmail.com>";

static char doc[] = "emf2svg -- Enhanced Metafile to SVG converter";

     static struct argp_option options[] = {
       {"verbose",  'v', 0,      0, "Produce verbose output"},
       {"emfplus",  'p', 0, 0, "Output file"},
       {"input",    'i', "FILE", 0, "Input EMF file"},
       {"output",   'o', "FILE", 0, "Output SVG file"},
       { 0 }
     };

/* A description of the arguments we accept. */
static char args_doc[] = "ARG1 ARG2";

struct arguments
{
  char *args[2];                /* arg1 & arg2 */
  bool  verbose, emfplus;
  char *output;
  char *input;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = (struct arguments *)state->input;

  switch (key)
    {
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


    case ARGP_KEY_ARG:
      if (state->arg_num >= 6)
        /* Too many arguments. */
        argp_usage (state);

      arguments->args[state->arg_num] = arg;

      break;

    case ARGP_KEY_END:
      if (state->arg_num < 0)
        /* Not enough arguments. */
        argp_usage (state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char *argv[])
{

  struct arguments arguments;
  arguments.verbose = 0;
  arguments.emfplus = 0;
  argp_parse (&argp, argc, argv, 0, 0, &arguments);


  std::ifstream in(arguments.input);
  std::string contents((std::istreambuf_iterator<char>(in)), 
              std::istreambuf_iterator<char>());
  std::ofstream out(arguments.output);
  char *svg_out = NULL;
  generatorOptions * options = (generatorOptions *)calloc(1,sizeof(generatorOptions));
  options->verbose = arguments.verbose; 
  options->emfplus = arguments.emfplus; 
  //options->nameSpace = (char *)"svg"; 
  options->svgDelimiter = 1; 
  emf2svg((char *)contents.c_str(), contents.size(), &svg_out, options);
  out << std::string(svg_out);
  free(svg_out);
  free(options);
  
  in.close();
  out.close();
  return 0;
}
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
