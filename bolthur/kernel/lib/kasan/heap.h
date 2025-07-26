/**
 * Copyright (C) 2018 - 2025 bolthur project.
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

#ifndef _LIB_KASAN_HEAP_H
#define _LIB_KASAN_HEAP_H

#include <stdint.h>
#include <stddef.h>

#define HEAP_MEMORY_MAX_LEVELS 32
#define HEAP_MEMORY_BLOCKS_PER_LEVEL( level ) ( 1 << ( level ))
#define HEAP_MEMORY_SIZE_OF_BLOCKS_AT_LEVEL( level, total_size ) ( ( total_size ) / ( 1 << ( level ) ) )
#define HEAP_MEMORY_INDEX_OF_POINTER_IN_LEVEL( pointer, level, memory_start, total_size) \
  ( ( ( ( uintptr_t )pointer ) - ( ( uintptr_t )memory_start ) ) / ( HEAP_MEMORY_SIZE_OF_BLOCKS_AT_LEVEL( level, total_size ) ) )

typedef struct {
  size_t size;
} memory_block_t;

int32_t kasan_heap_get_level( size_t size );
void kasan_heap_init( void*, size_t );
void* kasan_heap_alloc( size_t );
void kasan_heap_free( void* ptr );

#endif
