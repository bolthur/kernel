
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

#ifndef __LIBC_STRING__
#define __LIBC_STRING__

#include <sys/cdefs.h>
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
char *strcat( char* restrict, const char* restrict );
char *strchr( const char*, int );
int strcmp( const char*, const char* );
int strcoll( const char*, const char* );
char *strcpy( char* restrict, const char* restrict );
size_t strcspn( const char*, const char* );
char *strerror( int );
size_t strlen( const char*);
char *strncat( char* restrict, const char * restrict, size_t );
int strncmp( const char*, const char*, size_t );
char *strncpy( char* restrict, const char* restrict, size_t );
size_t strnlen( const char*, size_t );
char *strpbrk( const char*, const char * );
char *strrchr( const char*, int );
char *strrev( char* );
size_t strspn( const char*, const char* );
char *strstr( const char*, const char* );
char *strtok( char*, const char* );
size_t strxfrm( char*, const char*, size_t );

#if defined( __cplusplus )
}
#endif

#endif
