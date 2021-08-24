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
#include <task/process.h>
#include <task/thread.h>

struct shared_memory_entry {
  avl_node_t node;
  uint64_t* address;
  size_t id;
  size_t size;
  size_t use_count;
  list_manager_ptr_t process_mapping;
};

struct shared_memory_entry_mapped {
  uintptr_t start;
  size_t size;
  task_process_ptr_t process;
};

typedef struct shared_memory_entry shared_memory_entry_t;
typedef struct shared_memory_entry *shared_memory_entry_ptr_t;
typedef struct shared_memory_entry_mapped shared_memory_entry_mapped_t;
typedef struct shared_memory_entry_mapped *shared_memory_entry_mapped_ptr_t;

#define SHARED_ENTRY_GET_BLOCK( n ) \
  ( shared_memory_entry_ptr_t )( ( uint8_t* )n - offsetof( shared_memory_entry_t, node ) )

bool shared_memory_init( void );
size_t shared_memory_create( size_t );
uintptr_t shared_memory_attach( task_process_ptr_t, task_thread_ptr_t, size_t, uintptr_t );
bool shared_memory_detach( task_process_ptr_t, size_t );
bool shared_memory_address_is_shared( task_process_ptr_t, uintptr_t, size_t );
bool shared_memory_fork( task_process_ptr_t, task_process_ptr_t );
bool shared_memory_cleanup_process( task_process_ptr_t );

#endif
