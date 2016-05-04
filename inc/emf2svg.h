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
    double imgHeight;
    // width of the target image
    double imgWidth;
} generatorOptions;

// covert function
int emf2svg(char *contents, size_t length, char **out,
            generatorOptions *options);
//! \endcond


#ifdef MINGW
typedef void svg_get_t(void *context,const char * text);
int emf2svg_ext(char *contents, size_t length, char *nameSpace , bool emfplus , bool svgDelimiter , int width , int height , void *context , svg_get_t *getter );
#endif


#ifdef __cplusplus
}
#endif

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
