
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

#if ! defined( __KERNEL_TASK_PROCESS__ )
#define __KERNEL_TASK_PROCESS__

#include <stddef.h>
#include <avl.h>
#include <list.h>
#include <kernel/mm/virt.h>

typedef struct task_thread
  task_thread_t, *task_thread_ptr_t;
typedef struct task_thread_manager
  task_thread_manager_t, *task_thread_manager_ptr_t;

typedef enum {
  TASK_PROCESS_STATE_READY = 0,
  TASK_PROCESS_STATE_ACTIVE,
} task_process_state_t;

typedef enum {
  TASK_PROCESS_TYPE_KERNEL = 1,
  TASK_PROCESS_TYPE_USER,
} task_process_type_t;

typedef struct process {
  avl_node_t node_id;
  avl_tree_ptr_t thread_manager;
  size_t id;
  size_t priority;
  virt_context_ptr_t virtual_context;
  task_process_type_t type;
  task_process_state_t state;
} task_process_t, *task_process_ptr_t;

typedef struct {
  avl_tree_ptr_t tree_process_id;
  avl_tree_ptr_t thread_priority_tree;
} task_manager_t, *task_manager_ptr_t;

#define TASK_PROCESS_GET_BLOCK_ID( n ) \
  ( task_process_ptr_t )( ( uint8_t* )n - offsetof( task_process_t, node_id ) )

task_manager_ptr_t process_manager;
void task_process_init( void );
void task_process_schedule( void* );
size_t task_process_generate_id( void );
void task_process_create( uintptr_t, task_process_type_t, size_t );

#endif
