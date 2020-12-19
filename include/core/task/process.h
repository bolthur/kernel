
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#if ! defined( __CORE_TASK_PROCESS__ )
#define __CORE_TASK_PROCESS__

#include <stddef.h>
#include <collection/avl.h>
#include <collection/list.h>
#include <core/mm/virt.h>
#include <core/mm/shared.h>
#include <core/event.h>

typedef struct task_thread
  task_thread_t, *task_thread_ptr_t;
typedef struct task_thread_manager
  task_thread_manager_t, *task_thread_manager_ptr_t;

typedef struct task_stack_manager
  task_stack_manager_t, *task_stack_manager_ptr_t;

typedef enum {
  TASK_PROCESS_STATE_READY = 0,
  TASK_PROCESS_STATE_ACTIVE,
  TASK_PROCESS_STATE_HALT_SWITCH,
  TASK_PROCESS_STATE_KILL,
} task_process_state_t;

typedef struct process {
  avl_node_t node_id;
  avl_tree_ptr_t thread_manager;
  task_stack_manager_ptr_t thread_stack_manager;
  size_t id;
  size_t priority;
  virt_context_ptr_t virtual_context;
  task_process_state_t state;
  avl_tree_ptr_t shared_memory_entry;
  avl_tree_ptr_t shared_memory_mapped;
} task_process_t, *task_process_ptr_t;

typedef struct {
  avl_tree_ptr_t process_id;
  avl_tree_ptr_t thread_priority;
} task_manager_t, *task_manager_ptr_t;

#define TASK_PROCESS_GET_BLOCK_ID( n ) \
  ( task_process_ptr_t )( ( uint8_t* )n - offsetof( task_process_t, node_id ) )

extern task_manager_ptr_t process_manager;

bool task_process_init( void );
void task_process_schedule( event_origin_t, void* );
void task_process_cleanup( event_origin_t, void* );
void task_process_start( void );
size_t task_process_generate_id( void );
task_process_ptr_t task_process_create( uintptr_t, size_t );
bool task_process_prepare_init( task_process_ptr_t );
bool task_process_prepare_init_arch( task_process_ptr_t );

#endif
