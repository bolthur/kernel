/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#ifndef _LIB_STRING_H
#define _LIB_STRING_H

#include <stddef.h>
#include <stdint.h>

void* memchr( const void*, int, size_t );
int memcmp( const void*, const void*, size_t );
void* memcpy( void* restrict, const void* restrict, size_t );
void* memcpy_unsafe( void* restrict, const void* restrict, size_t );
void* memcpy_unsafe_src( void* restrict, const void* restrict, size_t );
void* memcpy_unsafe_dst( void* restrict, const void* restrict, size_t );
void* memset( void*, int, size_t );
char* strchr( const char*, int );
char* strcpy( char*, const char* );
size_t strlen( const char* );
size_t strlen_unsafe( const char* str );
int strncmp( const char*, const char*, size_t );
void* memmove( void*, const void*, size_t );
size_t strnlen( const char*, size_t );
char* strrchr( const char*, int );

#endif
