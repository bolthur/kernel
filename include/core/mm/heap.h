
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#if ! defined( __CORE_MM_HEAP__ )
#define __CORE_MM_HEAP__

#include <stddef.h>
#include <stdbool.h>
#include <avl.h>
#include <core/entry.h>

#if defined( ELF32 )
  #define HEAP_START 0xD0000000
  #define HEAP_MAX_SIZE 0xFFFFFFF
  #define HEAP_MIN_SIZE 0x10000
  #define HEAP_EXTENSION 0x1000
#elif defined( ELF64 )
  #error "Heap not ready for x64"
#endif

typedef struct {
  uintptr_t start;
  size_t size;
  avl_tree_t free_address;
  avl_tree_t free_size;
  avl_tree_t used_area;
} heap_manager_t, *heap_manager_ptr_t;

typedef struct {
  avl_node_t node_address;
  avl_node_t node_size;
  uintptr_t address;
  size_t size;
} heap_block_t, *heap_block_ptr_t;

#define HEAP_GET_BLOCK_ADDRESS( n ) \
  ( heap_block_ptr_t )( ( uint8_t* )n - offsetof( heap_block_t, node_address ) )
#define HEAP_GET_BLOCK_SIZE( n ) \
  ( heap_block_ptr_t )( ( uint8_t* )n - offsetof( heap_block_t, node_size ) )

extern heap_manager_ptr_t kernel_heap;

bool heap_initialized_get( void );
void heap_init( void );
uintptr_t heap_allocate_block( size_t, size_t );
void heap_free_block( uintptr_t );

#endif
