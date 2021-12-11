/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#if ! defined( _LIB_STDLIB_H )
#define _LIB_STDLIB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdnoreturn.h>

noreturn void abort( void );
void* aligned_alloc( size_t, size_t );
void* calloc( size_t, size_t );
void free( void* );
void* malloc( size_t );
unsigned long int strtoul( const char*, char**, int );

#endif
