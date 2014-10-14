#include "range.h"

const int RANGE_ERRORMAX = -5;
const char *RANGE_ERRORCODES[] = {
	"OK",
	"Invalid range string",
	"Something bad happened and there's a bug",
	"Couldn't allocate memory",
	"Invalid range string format found after verification, this is a bug",
	"End of string reached unexpectedly during parsing, this is a bug"
};

FileRange **range_new(int count) {
	FileRange **ranges;
	int i, j;

	ranges = (FileRange **)malloc(sizeof(FileRange *) * count);
	if(ranges == NULL) {
		return(NULL); /* couldn't allocate memory */
	}
	for(i = 0; i < count; i++) {
		ranges[i] = (FileRange *)malloc(sizeof(FileRange));
		if(ranges[i] == NULL) {
			for(j = 0; j < i; j++) {
				free(ranges[j]);
			}
			free(ranges);
		}
	}

	return(ranges);
}

void range_delete(FileRange ** ranges, int count) {
	int i;

	for(i = 0; i < count; i++) {
		free(ranges[i]);
	}
	free(ranges);
}

/*
 * A range string must be in the format:
 * int-int,int-int,...
 *
 * returns a count on success
 * -1 invalid range string
 * -2 something really bad happened
 */
int range_verify(char *rangeString) {
	int stage, numfound;
	int i;
	int count;

	count = 0; stage = 0; numfound = 0;
	for(i = 0; rangeString[i] != '\0'; i++) {
		switch(stage) {
			case 0:
				if(rangeString[i] == '-') {
					if(numfound == 0) {
						return(-1);
					}
					stage = 1;
					numfound = 0;
				} else if(rangeString[i] == ',') {
					if(numfound == 0) {
						return(-1);
					}
					count++;
					numfound = 0;
				} else if(ISDIGIT(rangeString[i])) {
					numfound = 1;
				} else {
					return(-1);
				}
				break;
			case 1:
				if(rangeString[i] == ',') {
					if(numfound == 0) {
						return(-1);
					}
					count++;
					stage = 0;
					numfound = 0;
				} else if(ISDIGIT(rangeString[i])) {
					numfound = 1;
				} else {
					return(-1);
				}
				break;
			default:
				/* this should never ever be reached */
				return(-2);
		}
	}

	if(numfound == 1) {
		count++;
	}

	return(count);
}

/*
 * returns the length of the string parsed, so it's easy to continue on to the next.
 * -1 if an invalid character was found.
 */
int range_parse_next(char *rangeString, FileRange *range) {
	char numbuffer[256];
	int i, j;
	j = 0;

	for(i = 0; i < 256; i++) {
		if(ISDIGIT(rangeString[i])) {
			numbuffer[i] = rangeString[i];
		} else if(rangeString[i] == '-') {
			numbuffer[i] = '\0';
			range->start = (off64_t)atol(numbuffer);

			i++;
			for(j = 0; j < 256; j++) {
				if(ISDIGIT(rangeString[i + j])) {
					numbuffer[j] = rangeString[i + j];
				} else if(rangeString[i + j] == '\0' || rangeString[i + j] == ',') {
					numbuffer[j] = '\0';
					range->end = (off64_t)atol(numbuffer);

					if(rangeString[i + j] != '\0') {
						j++;
					}
					break;
				} else {
					return(-1);
				}
			}

			break;
		} else if(rangeString[i] == '\0' || rangeString[i] == ',') {
			numbuffer[i] = '\0';
			range->start = (off64_t)atol(numbuffer);
			range->end = range->start;

			if(rangeString[i] != '\0') {
				i++;
			}
			break;
		} else {
			return(-1);
		}
	}

	return(i + j);
}

/*
 * parses a range string in to an array of ranges
 *
 * returns a count of ranges on success
 * -1 invalid range string
 * -2 something bad happened (BUG)
 * -3 couldn't allocate memory
 * -4 invalid range string format after verification (BUG)
 * -5 end of range string reached before total count reached (BUG)
 */
int range_parse(char *rangeString, FileRange ***ranges) {
	int count;
	int i;
	int ret;
	int parsepos;

	count = range_verify(rangeString);
	if(count < 1) {
		return(count); /* returns -1 or -2 */
	}

	*ranges = range_new(count);
	if(*ranges == NULL) {
		return(-3);
	}

	parsepos = 0;
	for(i = 0; i < count; i++) {
		ret = range_parse_next(&(rangeString[parsepos]), (*ranges)[i]);
		if(ret < 1) {
			range_delete(*ranges, count);
			return(-4); /* invalid string found after validation */
		}
		parsepos += ret;

		if(rangeString[parsepos] == '\0' && i < count - 1) {
			range_delete(*ranges, count);
			return(-5); /* end of string reached */
		}

		if(i == 0) {
			(*ranges)[i]->totalstart = 0;
			(*ranges)[i]->totalend = (*ranges)[i]->end - (*ranges)[i]->start;
		} else {
			(*ranges)[i]->totalstart = (*ranges)[i - 1]->totalend + 1;
			(*ranges)[i]->totalend = (*ranges)[i]->totalstart + ((*ranges)[i]->end - (*ranges)[i]->start);
		}
	}

	return(count);
}

const char *range_error_string(int errorcode) {
	if(errorcode > -1 || errorcode < RANGE_ERRORMAX) {
		return(RANGE_ERRORCODES[0]);
	}

	return(RANGE_ERRORCODES[-errorcode]);
}
