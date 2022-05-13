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

#if ! defined( _MM_SHARED_H )
#define _MM_SHARED_H

#include <stddef.h>
#include <stdbool.h>
#include "../../library/collection/list/list.h"
#include "../../library/collection/avl/avl.h"
#include "../task/process.h"
#include "../task/thread.h"

typedef struct {
  avl_node_t node;
  uint64_t* address;
  size_t id;
  size_t size;
  size_t use_count;
  list_manager_t* process_mapping;
} shared_memory_entry_t;

typedef struct {
  uintptr_t start;
  size_t size;
  task_process_t* process;
} shared_memory_entry_mapped_t;

#define SHARED_ENTRY_GET_BLOCK( n ) \
  ( shared_memory_entry_t* )( ( uint8_t* )n - offsetof( shared_memory_entry_t, node ) )

bool shared_memory_init( void );
size_t shared_memory_create( size_t );
uintptr_t shared_memory_attach( task_process_t*, task_thread_t*, size_t, uintptr_t );
bool shared_memory_detach( task_process_t*, size_t );
bool shared_memory_address_is_shared( task_process_t*, uintptr_t, size_t );
bool shared_memory_fork( task_process_t*, task_process_t* );
bool shared_memory_cleanup_process( task_process_t* );

#endif
