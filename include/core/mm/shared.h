
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

#if ! defined( __CORE_MM_SHARED__ )
#define __CORE_MM_SHARED__

#include <stddef.h>
#include <stdbool.h>
#include <core/task/process.h>

// forward declaration due to circular include
typedef struct process task_process_t, *task_process_ptr_t;

typedef struct {
  avl_node_t node;
  uint64_t* address_list;
  char* name;
  size_t size;
  size_t use_count;
} shared_memory_entry_t, *shared_memory_entry_ptr_t;

typedef struct {
  avl_node_t node;
  uintptr_t start;
  char* name;
  size_t size;
} shared_memory_entry_mapped_t, *shared_memory_entry_mapped_ptr_t;

#define SHARED_ENTRY_GET_BLOCK( n ) \
  ( shared_memory_entry_ptr_t )( ( uint8_t* )n - offsetof( event_block_t, node ) )

#define SHARED_MAPPED_GET_BLOCK( n ) \
  ( shared_memory_entry_mapped_ptr_t )( ( uint8_t* )n - offsetof( event_block_t, node ) )

bool shared_init( void );
bool shared_memory_create( const char*, size_t );
uintptr_t shared_memory_acquire( task_process_ptr_t, const char* );
bool shared_memory_release( task_process_ptr_t, const char* );

#endif
