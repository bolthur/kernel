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

#if ! defined( __CORE_MM_HEAP__ )
#define __CORE_MM_HEAP__

#include <stddef.h>
#include <stdbool.h>
#include <collection/avl.h>
#include <core/entry.h>

#if defined( ELF32 )
  #define HEAP_START 0xD0000000
  #define HEAP_MAX_SIZE 0xFFFFFFF
  #define HEAP_MIN_SIZE 0x10000
  #define HEAP_EXTENSION 0x1000
#elif defined( ELF64 )
  #error "Heap not ready for x64"
#endif

typedef enum {
  HEAP_INIT_EARLY = 0,
  HEAP_INIT_NORMAL,
  HEAP_INIT_SIZE,
} heap_init_state_t;

struct heap_manager {
  uintptr_t start;
  size_t size;
  heap_init_state_t state;
  avl_tree_t free_address[ HEAP_INIT_SIZE ];
  avl_tree_t free_size[ HEAP_INIT_SIZE ];
  avl_tree_t used_area[ HEAP_INIT_SIZE ];
};

struct heap_block {
  avl_node_t node_address;
  avl_node_t node_size;
  uintptr_t address;
  size_t size;
};

typedef struct heap_manager heap_manager_t;
typedef struct heap_manager *heap_manager_ptr_t;
typedef struct heap_block heap_block_t;
typedef struct heap_block *heap_block_ptr_t;

#define HEAP_GET_BLOCK_ADDRESS( n ) \
  ( heap_block_ptr_t )( ( uint8_t* )n - offsetof( heap_block_t, node_address ) )
#define HEAP_GET_BLOCK_SIZE( n ) \
  ( heap_block_ptr_t )( ( uint8_t* )n - offsetof( heap_block_t, node_size ) )

extern heap_manager_ptr_t kernel_heap;

extern uintptr_t __initial_heap_start;
extern uintptr_t __initial_heap_end;

bool heap_init_get( void );
void heap_init( heap_init_state_t );
uintptr_t heap_allocate_block( size_t, size_t );
void heap_free_block( uintptr_t );
size_t heap_block_length( uintptr_t );

#endif
