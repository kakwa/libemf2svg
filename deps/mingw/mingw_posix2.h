/*
 * file:        mingw_posix2.h
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
#ifndef MINGW_POSIX2
#define MINGW_POSIX2

struct mingw_posix2_file {
     unsigned _unused;  
};

struct mingw_posix2_file *mingw_posix2_fopen(const char *filename,const char *flags);
struct mingw_posix2_file *mingw_posix2_open_memstream(char **bufp, size_t *sizep);
int mingw_posix2_fread( void * ptr, size_t size, size_t count, struct mingw_posix2_file *stream );
int mingw_posix2_fwrite( const void * ptr, size_t size, size_t count, struct mingw_posix2_file *stream );
int mingw_posix2_fseek( struct mingw_posix2_file *stream, long int offset, int origin );
int mingw_posix2_fputc( int character, struct mingw_posix2_file * stream );
int mingw_posix2_fputs( const char *str, struct mingw_posix2_file * stream );
int mingw_posix2_fclose( struct mingw_posix2_file * stream );
int mingw_posix2_fflush( struct mingw_posix2_file * stream );
int mingw_posix2_fprintf( struct mingw_posix2_file * stream , const char *fmt , ... );
//--------------------- FILE porting library

#ifndef MINGW_POSIX2_IMPL
#define FILE struct mingw_posix2_file
#define fopen(_filename,_flags) mingw_posix2_fopen(_filename,_flags)
#define open_memstream(bufp, sizep) mingw_posix2_open_memstream(bufp,sizep)
#define fread(_ptr,_size,_count,_stream) mingw_posix2_fread(_ptr,_size,_count,_stream)
#define fwrite( _ptr, _size, _count, _stream ) mingw_posix2_fwrite(_ptr,_size,_count,_stream)
#define fseek(_stream,_offset,_origin) mingw_posix2_fseek(_stream,_offset,_origin)
#define fputc(ch,stream)  mingw_posix2_fputc(ch,stream)
#define fputs(_str,_stream)  mingw_posix2_fputs(_str,_stream)
#define fprintf  mingw_posix2_fprintf
#define fclose(_stream)    mingw_posix2_fclose(_stream)
#define fflush(_stream)    mingw_posix2_fflush(_stream)
#define ftell(_stream) mingw_posix2_fseek(_stream,0,SEEK_CUR)
#endif


#endif