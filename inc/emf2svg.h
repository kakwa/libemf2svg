#include <stdlib.h>
#include <stdio.h>
#include <stddef.h> /* for offsetof() macro */
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// structure containing generator arguments
typedef struct {
    // SVG namespace (the '<something>:' before each fields)
    char *nameSpace;
    // Verbose mode, output fields and fields values if True
    bool verbose;
    // Handle emf+ records or not
    bool emfplus;
    // draw svg document delimiter or not
    bool svgDelimiter;
    // height of the target image
    int imgHeight;
    // width of the target image
    int imgWidth;
} generatorOptions;

// covert function
int emf2svg(char *contents, size_t length, char **out,
            generatorOptions *options);
//! \endcond

#ifdef __cplusplus
}
#endif

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
