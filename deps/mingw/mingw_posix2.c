/*
 * file:        mingw_posix2.c
 * description: minimal porting layer for posix2 FILE api
 * Copyright 2016 Cian Chambliss, Alpha Software Corporation
 *
 * Support for posix2 FILE api functions (open_memstream etc) that are not supported by the windows posix runtime.
 *
 * Caveats: 
 *
 *   this header macros standard FILE functions to a porting layer, passing a FILE to a library that hasn't also 
 *   been compiled with this header will not work, as will ommitting this header from any module in your project that uses
 *   FILE * passed from a module that does include this header.
 *
 * This file is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation. 
 */
#include "stdio.h"
#include "mingw_posix2.h"
#include <stdlib.h> 
#include <string.h>
#include <assert.h>


typedef int readfn_t (void *cookie, char *buf, int n);
typedef int writefn_t (void *cookie, const char *buf, int n);
typedef fpos_t seekfn_t (void *cookie, fpos_t off, int whence);
typedef int closefn_t (void *cookie);
typedef int flushfn_t (void *cookie);

struct FILE_driver {    
readfn_t   *_read;
writefn_t *_write;
seekfn_t   *_seek;
closefn_t *_close;
flushfn_t *_flush;
};

struct FILE_posix2 {
FILE          _file; // look like a file... 
struct FILE_driver *driver;
void        *cookie;
};
//------------------------------ NUll implementations
int null_flushfn(void *cookie) {
    return 0;
}

//------------------------ memstream
struct FILE_posix2_memstream {
struct FILE_posix2 _file;
//-----------------------
int    position;
int    size;
int    capacity;
char   *contents;
char   **ptr;
size_t  *sizeloc;
};

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))


#define memstream_check(MS) if (!(MS)->contents) { return -1; }

static int memstream_grow(struct FILE_posix2_memstream *ms, int minsize)
{
    memstream_check(ms);
    int newcap= ms->capacity * 2;					
    while (newcap <= minsize) newcap *= 2;
    ms->contents= realloc(ms->contents, newcap);
    if (!ms->contents) return -1;	/* errno == ENOMEM */
    memset(ms->contents + ms->capacity, 0, newcap - ms->capacity);
    ms->capacity= newcap;
    *ms->ptr= ms->contents;		/* size has not changed */
    return 0;
}

int memstream_readfn(void *cookie, char *buf, int n) {
    struct FILE_posix2_memstream *ms= (struct FILE_posix2_memstream *)cookie;
    memstream_check(ms);
    n = min(ms->size - ms->position, n);
    if (n < 1) return 0;
    memcpy(buf, ms->contents, n);
    ms->position += n;
    return n;
}

int memstream_writefn(void *cookie, const char *buf, int n) {
    struct FILE_posix2_memstream *ms= (struct FILE_posix2_memstream *)cookie;			
    memstream_check(ms);
    if (ms->capacity <= ms->position + n)
	if (memstream_grow(ms, ms->position + n) < 0)		/* errno == ENOMEM */
	    return -1;
    memcpy(ms->contents + ms->position, buf, n);
    ms->position += n;
    if (ms->size < ms->position) *ms->sizeloc= ms->size= ms->position;
    assert(ms->size < ms->capacity);
    assert(ms->contents[ms->size] == 0);
    return n;
}

fpos_t memstream_seekfn(void *cookie, fpos_t offset, int whence) {
    struct FILE_posix2_memstream *ms= (struct FILE_posix2_memstream *)cookie;
    memstream_check(ms);
    fpos_t pos= 0;							
    switch (whence) {
	case SEEK_SET:	pos= offset;			break;
	case SEEK_CUR:	pos= ms->position + offset;	break;
	case SEEK_END:	pos= ms->size + offset;		break;
	default:	//errno= EINVAL;			
    return -1;
    }
    if (pos >= ms->capacity) memstream_grow(ms, pos);
    ms->position= pos;
    if (ms->size < ms->position) *ms->sizeloc= ms->size= ms->position;
	assert(ms->size < ms->capacity && ms->contents[ms->size] == 0);
    return pos;
}

int memstream_closefn(void *cookie) {
    struct FILE_posix2_memstream *ms= (struct FILE_posix2_memstream *)cookie; 
    memstream_check(ms);
    if (!ms->contents) { free(ms); return -1; }
    ms->size= min(ms->size, ms->position);
    *ms->ptr= ms->contents;
    *ms->sizeloc= ms->size;
    assert(ms->size < ms->capacity);
    ms->contents[ms->size]= 0;
    free(ms);
    return 0;
}    

static struct FILE_driver memstream_impl = {
  memstream_readfn
, memstream_writefn
, memstream_seekfn
, memstream_closefn
, null_flushfn     
};

FILE *mingw_posix2_open_memstream(char **bufp, size_t *sizep) 
{
    struct FILE_posix2_memstream *ms = (struct FILE_posix2_memstream *)calloc( sizeof(struct FILE_posix2_memstream) , 1 );
    ms->_file.driver = &memstream_impl;
    ms->_file.cookie = ms;
    return &ms->_file._file;
}

int mingw_posix2_fputc( int character, FILE * stream ) 
{
    struct FILE_posix2 *p2s = ((struct FILE_posix2 *)stream);
    // First lets 
    return p2s->driver->_write( p2s->cookie , (const char *)&character , 1);
}

int mingw_posix2_fclose( FILE * stream ) 
{
    struct FILE_posix2 *p2s = ((struct FILE_posix2 *)stream);
    return p2s->driver->_close( p2s->cookie );    
}

int mingw_posix2_fflush( FILE * stream )
{
    struct FILE_posix2 *p2s = ((struct FILE_posix2 *)stream);
    return p2s->driver->_flush( p2s->cookie );    
}

 