
/**
 * Copyright (C) 2017 - 2019 bolthur project.
 *
 * This file is part of bolthur/kernel.
 *
 * bolthur/kernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bolthur/kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bolthur/kernel.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LIBC_STDLIB__
#define __LIBC_STDLIB__

#include <sys/cdefs.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>

#define RAND_MAX 32767

#if defined( __cplusplus )
extern "C" {
#endif

typedef struct {
  int quot;
  int rem;
} div_t;

typedef struct {
  long int quot;
  long int rem;
} ldiv_t;

void abort( void );
double atof( const char* );
int abs( int );
int atexit( void ( *func )( void ) );
double atof( const char* );
int atoi( const char* );
long int atol( const char* );
void *bsearch( const void*, const void*, size_t, size_t, int ( *compare )( const void*, const void* ) );
void *calloc( size_t, size_t );
div_t div ( int, int );
void exit( int32_t );
void free( void* );
char *ftoa( float, char*, int32_t );
char *getenv( const char* );
imaxdiv_t imaxdiv( intmax_t, intmax_t );
char *itoa( int32_t, char*, int32_t );
long int labs( long int );
ldiv_t ldiv ( long int, long int );
void *malloc( size_t );
int mblen( const char*, size_t );
size_t mbstowcs( wchar_t*, const char*, size_t );
int mbtowc( wchar_t*, const char*, size_t );
void qsort( void*, size_t, size_t, int ( *compare )( const void*, const void* ) );
int rand( void );
void *realloc( void*, size_t );
void srand( unsigned int );
double strtod( const char* restrict, char** restrict );
long int strtol( const char* restrict, char** restrict, int );
unsigned long int strtoul( const char* restrict, char** restrict, int );
int system( const char* );
char *utoa( uint32_t, char*, int32_t );
size_t wcstombs( char*, const wchar_t*, size_t );
int wctomb( char*, wchar_t );

#if defined( __cplusplus )
}
#endif

#endif
