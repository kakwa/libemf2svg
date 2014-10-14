#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <stdio.h>

#define ISDIGIT(X) (X >= '0' && X <= '9')

typedef struct {
	off64_t start;
	off64_t end;
	off64_t totalstart;
	off64_t totalend;
} FileRange;

FileRange **range_new(int count);
void range_delete(FileRange **, int count);
int range_verify(char *rangeString);
int range_parse_next(char *rangeString, FileRange *range);
int range_parse(char *rangeString, FileRange ***ranges);
const char *range_error_string(int errorcode);

