
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
#include <collection/list.h>
#include <collection/avl.h>

#define SHARED_MEMORY_ACCESS_READ 0x1
#define SHARED_MEMORY_ACCESS_WRITE 0x2

// forward declaration due to circular include
typedef struct process task_process_t, *task_process_ptr_t;

typedef struct {
  avl_node_t node;
  uint64_t* address;
  char* name;
  size_t size;
  size_t use_count;
  uint32_t access;
} shared_memory_entry_t, *shared_memory_entry_ptr_t;

typedef struct {
  avl_node_t node;
  char* name;
  uintptr_t start;
  size_t size;
  shared_memory_entry_ptr_t reference;
} shared_memory_entry_mapped_t, *shared_memory_entry_mapped_ptr_t;

#define SHARED_ENTRY_GET_BLOCK( n ) \
  ( shared_memory_entry_ptr_t )( ( uint8_t* )n - offsetof( shared_memory_entry_t, node ) )

#define SHARED_MAPPED_GET_BLOCK( n ) \
  ( shared_memory_entry_mapped_ptr_t )( ( uint8_t* )n - offsetof( shared_memory_entry_mapped_t, node ) )

bool shared_init( void );
bool shared_memory_create( const char*, uint32_t );
bool shared_memory_release( const char* );
bool shared_memory_initialize( const char*, size_t );
uintptr_t shared_memory_acquire( task_process_ptr_t, const char*, uintptr_t, uint32_t );
shared_memory_entry_mapped_ptr_t shared_memory_retrieve_by_address( task_process_ptr_t, uintptr_t );
shared_memory_entry_ptr_t shared_memory_retrieve_by_name( const char* );
shared_memory_entry_ptr_t shared_memory_retrieve_deleted( shared_memory_entry_ptr_t );
bool shared_memory_cleanup_deleted( shared_memory_entry_ptr_t );

#endif
