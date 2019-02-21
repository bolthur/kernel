
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

#if ! defined( __LIBC_STRING__ )
#define __LIBC_STRING__

#include <stddef.h>
#include <stdint.h>

#if defined( __cplusplus )
extern "C" {
#endif

void *memchr( const void*, int, size_t );
int memcmp( const void*, const void*, size_t );
void* memcpy( void* restrict, const void* restrict, size_t );
void* memmove( void*, const void*, size_t );
void* memset( void*, int, size_t );
int strcmp( const char*, const char* );
size_t strlen( const char*);
char *strrev( char* );

#if defined( __cplusplus )
}
#endif

#endif
