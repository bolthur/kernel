
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LIBC_STDIO__
#define __LIBC_STDIO__

#include <sys/cdefs.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define EOF ( -1 )
#define SEEK_SET 0

typedef struct {
  int unused;
} FILE;

#if defined( __cplusplus )
extern "C" {
#endif

extern FILE* stderr;
#define stderr stderr

int fclose( FILE* );
int fflush( FILE* );
FILE* fopen( const char*, const char* );
int fprintf( FILE*, const char*, ... );
size_t fread( void*, size_t, size_t, FILE* );
int fseek( FILE*, long int, int );
long int ftell( FILE* );
size_t fwrite( const void*, size_t, size_t, FILE* );
int printf( const char* __restrict, ... );
int putchar( int );
int puts( const char* );
void setbuf( FILE*, char* );
int vfprintf( FILE*, const char*, va_list );

#if defined( __cplusplus )
}
#endif

#endif
