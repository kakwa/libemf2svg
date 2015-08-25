#include <emf2svg.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

/* to compile: gcc -Wall -std=c99 -lm -lemf2svg ./example.c -o test */
int main(int argc, char *argv[]){

    // quick and dirty way to load the file in memory
    if (argc != 2){fprintf(stderr, "file missing\n"); exit(1);}
    struct stat s; const char * file_name = argv[1];
    int fd = open(file_name, O_RDONLY);
    if (fd < 0){fprintf(stderr, "file access failed\n"); exit(1);}
    fstat (fd, &s); 

    /* emf content size */
    size_t emf_size = s.st_size;
    /* emf content */
    char * emf_content = mmap(0, emf_size, PROT_READ, MAP_PRIVATE, fd, 0);
    /* svg output string */
    char *svg_out = NULL;

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

    // do something with the generated SVG
    fprintf(stdout,"%s", svg_out);

    // free the allocated structures
    free(svg_out);
    free(options);
    // close file and free content
    close(fd);
    munmap(emf_content, emf_size);
    // return
    exit(!ret);
}
