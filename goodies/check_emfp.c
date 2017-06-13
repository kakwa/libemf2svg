#include <emf2svg.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

/* to compile: gcc -Wall -std=c99 -lm -lemf2svg ./check_emfp.c -o test */
int main(int argc, char *argv[]){
    /* This script takes the emf file as first argument
     * and returns:
     * 3 -> error opening the file
     * 2 -> file is not a well formed EMF
     * 1 -> file only contains EMF records
     * 0 -> file also contains EMF+ records
     */

    // quick and dirty way to load the file in memory
    if (argc != 2){fprintf(stderr, "file missing\n"); exit(3);}
    struct stat s; const char * file_name = argv[1];
    int fd = open(file_name, O_RDONLY);
    if (fd < 0){fprintf(stderr, "file access failed\n"); exit(3);}
    fstat (fd, &s); 

    /* return code for check and conversion */
    int ret;

    /* emf content size */
    size_t emf_size = s.st_size;
    /* emf content */
    char * emf_content = mmap(0, emf_size, PROT_READ, MAP_PRIVATE, fd, 0);

    /************************** check if emf+ records in emf file **********/

    bool emfplus;
    ret = emf2svg_is_emfplus(emf_content, emf_size, &emfplus);

    // close file and free content
    close(fd);
    munmap(emf_content, emf_size);

    if(!ret){
        fprintf(stdout,"%s is corrupted\n", file_name);
	exit(2);
    }
    if(emfplus){
        fprintf(stdout,"%s contains EMF+ records\n", file_name);
	exit(0);
    } else {
        fprintf(stdout,"%s only contains EMF records\n", file_name);
	exit(1);
    }
}
