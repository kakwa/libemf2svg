/*
 * file:        argp.h
 * description: minimal replacement for GNU Argp library
 * Copyright 2011 Peter Desnoyers, Northeastern University
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */
#ifndef __ARGP_H__

#include <getopt.h>
#include <string.h>
#include <cstdlib>

/* This only includes the features I've used in my programs to date;
 * in particular, it totally ignores any sort of flag.
 */

#ifndef __error_t_defined
typedef int error_t;
# define __error_t_defined
#endif

struct argp_option {
    const char *name;
    char  key;
    const char *arg;
    int   flags;
    const char *doc;
    int   group;                /* ignored */
};

struct argp_state {
    void *input;
    char *name;
    struct argp *argp;
    int  maxlen;
};

struct argp {
    struct argp_option *options;
    error_t (*parser)(int key, char *arg, struct argp_state *state);
    char *arg_doc;
    char *prog_doc;
};

void argp_help(struct argp_state *state)
{
    printf("Usage: %s [OPTIONS...] %s\n%s\n\n",
           state->name, state->argp->arg_doc, state->argp->prog_doc);

    struct argp_option *o = state->argp->options;
    int i;
    
    for (i = 0; o[i].name != NULL; i++) {
        char tmp[80], *p = tmp;
        p += sprintf(tmp, "--%s", o[i].name);
        if (o[i].arg)
            sprintf(p, "=%s", o[i].arg);
        printf("  %-*s%s\n", state->maxlen + 8, tmp, o[i].doc);
    }
    printf("  --%-*s%s\n", state->maxlen+6, "help", "Give this help list");
    printf("  --%-*s%s\n", state->maxlen+6, "usage",
           "Give a short usage message");

    //exit(1);
}

void argp_usage(struct argp_state *state)
{
    char buf[1024], *p = buf, *col0 = buf;
    p += sprintf(p, "Usage: %s", state->name);
    int i, indent = p-buf;
    struct argp_option *o = state->argp->options;

    for (i = 0; o[i].name != NULL; i++) {
        p += sprintf(p, " [--%s%s%s]", o[i].name, o[i].arg ? "=":"",
                     o[i].arg ? o[i].arg : "");
        if (p-col0 > (78-state->maxlen)) {
            p += sprintf(p, "\n");
            col0 = p;
            p += sprintf(p, "%*s", indent, "");
        }
    }
    sprintf(p, " %s\n", state->argp->arg_doc);
    printf("%s", buf);
    //exit(0);
}

char *cleanarg(char *s)
{
    char *v = strrchr(s, '/');
    return v ? v+1 : s;
}

enum {ARGP_KEY_ARG, ARGP_KEY_END, ARGP_ERR_UNKNOWN, ARGP_IN_ORDER};

void argp_parse(struct argp *argp, int argc, char **argv, int flags, int tmp,
                void *input)
{
    int n_opts, c;
    struct argp_state state = {};
    state.name = cleanarg(argv[0]);
    state.input = input;
    state.argp = argp;

    /* calculate max "--opt=var" length */
    int i, max = 0;
    struct argp_option *opt = argp->options;
    for (i = 0; opt[i].name != NULL; i++) {
        int m = strlen(opt[i].name) +
            (opt[i].arg ? 1+strlen(opt[i].arg) : 0);
        max = (max < m) ? m : max;
    }
    state.maxlen = max+2;
    n_opts = i;
    
    struct option *long_opts = (struct option *)calloc((n_opts+3) * sizeof(*long_opts), 1);
    
    i = 0;
    long_opts[i].name = "usage";
    long_opts[i++].has_arg = no_argument;
    
    long_opts[i].name = "help";
    long_opts[i++].has_arg = no_argument;
    
    for (opt = argp->options; opt->name != NULL; opt++, i++) {
        int has_arg = opt->arg != NULL;
        long_opts[i].name = opt->name;
        long_opts[i].has_arg = has_arg ? required_argument : no_argument;
    }

    /* we only accept long arguments - return value is zero, and 'i'
     * gives us the index into 'long_opts[]'
     */
    while ((c = getopt_long(argc, argv, "", long_opts, &i)) != -1) {
        if (i == 0)
            argp_usage(&state);
        else if (i == 1)
            argp_help(&state);
        else
            argp->parser(argp->options[i-2].key, optarg, &state);
    }
    
    while (optind < argc) 
        argp->parser(ARGP_KEY_ARG, argv[optind++], &state);
    argp->parser(ARGP_KEY_END, NULL, &state);

    free(long_opts);
}

#endif /* __ARGP_H__ */

