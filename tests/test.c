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
    if (argc < 2){fprintf(stderr, "not enough args\n"); exit(1);}
    int ret = 0;

    for(int i = 0; i < argc; i++){
        struct stat s; const char * file_name = argv[i];
        fprintf(stderr, "opening file '%s'\n", argv[i]);
        int fd = open(file_name, O_RDONLY);
        if (fd < 0){fprintf(stderr, "file access failed\n"); exit(1);}
        fstat (fd, &s); 

        /* return code for check and conversion */

        /* emf content size */
        size_t emf_size = s.st_size;
        /* emf content */
        char * emf_content = mmap(0, emf_size, PROT_READ, MAP_PRIVATE, fd, 0);

        /***********************************************************************/

        /* svg output string */
        char *svg_out = NULL;
        /* svg output length */
        size_t svg_out_len;

        /*************************** options settings **************************/

        /* allocate the options structure) */
        generatorOptions *options = (generatorOptions *)calloc(1, \
                sizeof(generatorOptions));
        options->verbose = true;
        options->emfplus = true;
        options->nameSpace = (char *)"svg";
        options->svgDelimiter = true;
        options->imgWidth = 0;
        options->imgHeight = 0;

        /***************************** conversion ******************************/

        fclose(stdout);
        ret =+ emf2svg(emf_content, emf_size, &svg_out, &svg_out_len, options);
        fopen("/dev/console", "w");

        /***********************************************************************/

        // free the allocated structures
        free(svg_out);
        free(options);
        close(fd);
        munmap(emf_content, emf_size);
    }
    // return
    exit(!ret);
}
