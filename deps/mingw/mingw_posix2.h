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

FILE *mingw_posix2_open_memstream(char **bufp, size_t *sizep);

int mingw_posix2_fputc( int character, FILE * stream );
int mingw_posix2_fclose( FILE * stream );
int mingw_posix2_fflush( FILE * stream );
int mingw_posix2_fprintf( FILE * stream , const char *fmt , ... );

//--------------------- FILE porting library
#define open_memstream(bufp, sizep) mingw_posix2_open_memstream(bufp,sizep)
//#define fputc(ch,stream)  mingw_posix2_fputc(ch,stream)
#define fputc(_c,_stream)  (--(_stream)->_cnt >= 0 ? 0xff & (*(_stream)->_ptr++ = (char)(_c)) :  mingw_posix2_fputc((_c),(_stream)))
#define fputs(_str,_stream)  mingw_posix2_fputs(_str,_stream);
#define fprintf  mingw_posix2_fprintf
#define fclose(_stream)    mingw_posix2_fclose(_stream);
#define fflush(_stream)    mingw_posix2_fflush(_stream);


#endif