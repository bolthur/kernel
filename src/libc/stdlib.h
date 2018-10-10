
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

#ifndef __LIBC_STDLIB__
#define __LIBC_STDLIB__

#include <sys/cdefs.h>
#include <stdint.h>
#include <stddef.h>

#if defined( __cplusplus )
extern "C" {
#endif

void abort( void );
void exit( int32_t status );
char *itoa( int32_t, char *, int32_t );
char *utoa( uint32_t, char *, int32_t );

void free( void *ptr );
void *calloc( size_t nitems, size_t size );
void *malloc( size_t size );
void *realloc( void *ptr, size_t size );

#if defined( __cplusplus )
}
#endif

#endif
