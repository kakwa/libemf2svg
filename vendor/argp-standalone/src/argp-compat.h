/*
 * SPDX-FileCopyrightText: 2020 Thomas Mathys
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * argp-standalone: standalone version of glibc's argp functions.
 */

#ifndef ARGP_COMPAT_H_INCLUDED
#define ARGP_COMPAT_H_INCLUDED

#if defined(_WIN32)
/* Spell Windows.h all lowercase for MinGW on case sensitive systems. */
#include <windows.h>
#endif
#include <stddef.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_SSIZE_T) && !HAVE_SSIZE_T
#if defined(_WIN32)
typedef SSIZE_T ssize_t;
#else
#error No replacement for ssize_t is available for the target platform
#endif
#endif

#if defined(_WIN32)
#define ARGP_PATH_SEPARATOR '\\'
#else
#define ARGP_PATH_SEPARATOR '/'
#endif

#if defined(HAVE_MEMPCPY) && !HAVE_MEMPCPY
void* argp_compat_mempcpy(void* out, const void* in, size_t n);
#define mempcpy argp_compat_mempcpy
#endif

#if defined(HAVE_SLEEP) && !HAVE_SLEEP
void argp_compat_sleep(unsigned int seconds);
#define sleep argp_compat_sleep
#endif

#if defined(HAVE_STRCASECMP) && !HAVE_STRCASECMP
int argp_compat_strcasecmp(const char* s1, const char* s2);
#define strcasecmp argp_compat_strcasecmp
#endif

#if defined(HAVE_STRCHRNUL) && !HAVE_STRCHRNUL
const char* argp_compat_strchrnul(const char* s, int c);
#define strchrnul argp_compat_strchrnul
#endif

#if defined(HAVE_STRNDUP) && !HAVE_STRNDUP
char* argp_compat_strndup(const char* s, size_t n);
#define strndup argp_compat_strndup
#endif

#endif
