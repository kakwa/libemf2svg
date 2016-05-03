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
#define MINGW_POSIX2_IMPL 
#include "stdio.h"
#include "mingw_posix2.h"
#include <stdlib.h> 
#include <string.h>
#include <stdarg.h> 
#include <assert.h>

//#define DEBUG_POSIX2_LAYER 

typedef int readfn_t (void *cookie, char *buf, int n);
typedef int writefn_t (void *cookie, const char *buf, int n);
typedef fpos_t seekfn_t (void *cookie, fpos_t off, int whence);
typedef int closefn_t (void *cookie,void *container);
typedef int flushfn_t (void *cookie);

struct FILE_driver {
const char  *name;    
readfn_t   *_read;
writefn_t *_write;
seekfn_t   *_seek;
closefn_t *_close;
flushfn_t *_flush;
};

struct FILE_posix2 {
struct mingw_posix2_file _file; 
struct FILE_driver *driver;
void        *cookie;
};
//------------------------------ NUll implementations
int null_flushfn(void *cookie) {
    return 0;
}

int std_readfn(void *cookie, char *buf, int n) {
    return fread( (void *)buf , n , 1 , (FILE *)cookie );
}

int std_writefn(void *cookie, const char *buf, int n) {
    return fwrite( (void *)buf , n , 1 , (FILE *)cookie );
}

fpos_t std_seekfn(void *cookie, fpos_t offset , int origin ) {
    return fseek( (FILE *)cookie , offset , origin);
}

int std_closefn(void *cookie,void *container)
{
    int result = fclose( (FILE *)cookie );
    free( container ); // container is a wrapper, free it
    return result;
}

int std_fflush(void *cookie)
{
    return fflush( (FILE *)cookie );
}

static struct FILE_driver std_impl = {
  "std"  
, std_readfn
, std_writefn
, std_seekfn
, std_closefn
, std_fflush
};


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

int memstream_closefn(void *cookie,void *container) {
    struct FILE_posix2_memstream *ms= (struct FILE_posix2_memstream *)cookie; 
    if (!ms->contents) { free(ms); return -1; }
    ms->size= min(ms->size, ms->position);
    *ms->ptr= ms->contents;
    *ms->sizeloc= ms->size;
    assert(ms->size < ms->capacity);
    ms->contents[ms->size]= 0;
#ifdef DEBUG_POSIX2_LAYER
    printf("close memory %d bytes\r\n",ms->size);
#endif    
    
    free(ms);
    return 0;
}    

static struct FILE_driver memstream_impl = {
  "memstream"
, memstream_readfn
, memstream_writefn
, memstream_seekfn
, memstream_closefn
, null_flushfn     
};

struct mingw_posix2_file *mingw_posix2_fopen(const char *filename,const char *flags) {
#ifdef DEBUG_POSIX2_LAYER
    printf("mingw_posix2_fopen(\"%s\",\"%s\")\r\n",filename,flags);
#endif    
    FILE *realFile = fopen(filename,flags);
    if( realFile ) {
        struct FILE_posix2 *stdfile = (struct FILE_posix2 *)calloc( sizeof(struct FILE_posix2) , 1 );
        stdfile->driver = &std_impl;        
        stdfile->cookie = realFile;
        return &stdfile->_file;
    }
    return NULL;
}

struct mingw_posix2_file *mingw_posix2_open_memstream(char **bufp, size_t *sizep) 
{
#ifdef DEBUG_POSIX2_LAYER
    printf("mingw_posix2_open_memstream(...)\r\n");
#endif    
    struct FILE_posix2_memstream *ms = (struct FILE_posix2_memstream *)calloc( sizeof(struct FILE_posix2_memstream) , 1 );
    ms->_file.driver = &memstream_impl;
    ms->_file.cookie = ms;
	ms->position = ms->size = 0;
	ms->capacity = 1024*16;
	ms->contents = calloc(ms->capacity, 1);	
    if (!ms->contents) { free(ms);  return 0; } /* errno == ENOMEM */
	ms->ptr= bufp;
	ms->sizeloc= sizep;    
    memstream_seekfn(ms->_file.cookie,0,0);
    memstream_impl._seek(ms->_file.cookie,0,0);
    return &ms->_file._file;
}

int mingw_posix2_fread ( void * ptr, size_t size, size_t count, struct mingw_posix2_file * stream )
{
    struct FILE_posix2 *p2s = ((struct FILE_posix2 *)stream);
#ifdef DEBUG_POSIX2_LAYER
    printf("mingw_posix2_fread(...%s)\r\n",p2s->driver->name);
#endif    
    return (*p2s->driver->_read)( p2s->cookie , ptr , size * count );    
}

int mingw_posix2_fwrite( const void * ptr, size_t size, size_t count, struct mingw_posix2_file *stream )
{    
    struct FILE_posix2 *p2s = ((struct FILE_posix2 *)stream);
#ifdef DEBUG_POSIX2_LAYER
    printf("mingw_posix2_fwrite(...%s)\r\n",p2s->driver->name);
#endif    
    return (*p2s->driver->_write)( p2s->cookie , ptr , size * count );    
}

int mingw_posix2_fseek( struct mingw_posix2_file *stream, long int offset, int origin )
{
    struct FILE_posix2 *p2s = ((struct FILE_posix2 *)stream);
#ifdef DEBUG_POSIX2_LAYER
    printf("mingw_posix2_fseek(%s...)\r\n",p2s->driver->name);
#endif    
    return (*p2s->driver->_seek)( p2s->cookie , offset , origin );   
    
}

int mingw_posix2_fputc( int character, struct mingw_posix2_file * stream ) 
{
    struct FILE_posix2 *p2s = ((struct FILE_posix2 *)stream);
#ifdef DEBUG_POSIX2_LAYER
    printf("mingw_posix2_fputc(...,%s)\r\n",p2s->driver->name);
#endif    
    // First lets 
    return (*p2s->driver->_write)( p2s->cookie , (const char *)&character , 1);
}

int mingw_posix2_fputs( const char *str, struct mingw_posix2_file * stream )
{
    struct FILE_posix2 *p2s = ((struct FILE_posix2 *)stream);
#ifdef DEBUG_POSIX2_LAYER
    printf("mingw_posix2_fputs(\"%s\",%s)\r\n",str,p2s->driver->name);
#endif    
    // First lets 
    return (*p2s->driver->_write)( p2s->cookie , str , strlen(str));    
}

int mingw_posix2_fclose( struct mingw_posix2_file * stream ) 
{
    struct FILE_posix2 *p2s = ((struct FILE_posix2 *)stream);
#ifdef DEBUG_POSIX2_LAYER
    printf("mingw_posix2_fclose(%s)\r\n",p2s->driver->name);
#endif    
    void *cookie = p2s->cookie;
    p2s->cookie = NULL;
    return (*p2s->driver->_close)( cookie , (void *)stream );    
}

int mingw_posix2_fflush( struct mingw_posix2_file * stream )
{
#ifdef DEBUG_POSIX2_LAYER
    printf("mingw_posix2_fflush(...)\r\n");
#endif    
    struct FILE_posix2 *p2s = ((struct FILE_posix2 *)stream);
    return (*p2s->driver->_flush)( p2s->cookie );    
}

int mingw_posix2_fprintf( struct mingw_posix2_file * stream , const char *fmt , ... )
{
#ifdef DEBUG_POSIX2_LAYER
    printf("mingw_posix2_fprintf(\"%s\")\r\n",fmt);
#endif    
    int	cnt = 0;
    va_list argptr;
    va_start(argptr, fmt);
    
    const char *singleFmt = strchr(fmt,'%');    
    if( !singleFmt || (singleFmt[1] == 's' && !strchr(singleFmt+1,'%'))  )
    {
        if( singleFmt ) {
            const char *txt = va_arg(argptr,const char *);
            if( fmt < singleFmt ) {
                cnt = mingw_posix2_fwrite(fmt,1,(int)(singleFmt-fmt),stream);
            }
            if( *txt )
                cnt += mingw_posix2_fputs(txt,stream);
            if( singleFmt[2] ) {
                cnt += mingw_posix2_fputs(singleFmt+2,stream);    
            }
        } else {
            cnt = mingw_posix2_fputs(fmt,stream);
        }        
    }
    else
    {
        char buffer[ 8*1024 ];
        cnt = vsprintf(buffer, fmt, argptr);
        va_end(argptr);
        mingw_posix2_fputs(buffer,stream);
    }

	return cnt;    
} 