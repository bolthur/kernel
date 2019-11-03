
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

#if ! defined( __KERNEL_TASK_THREAD__ )
#define __KERNEL_TASK_THREAD__

#include <stdint.h>
#include <stddef.h>
#include <avl.h>

typedef struct process task_process_t, *task_process_ptr_t;
typedef struct task_priority_queue task_priority_queue_t, *task_priority_queue_ptr_t;

typedef enum {
  TASK_THREAD_STATE_READY = 0,
  TASK_THREAD_STATE_ACTIVE,
} task_thread_state_t;

typedef struct task_thread {
  avl_node_t node_id;
  size_t id;
  size_t priority;
  void* context;
  task_thread_state_t state;
  task_process_ptr_t process;
} task_thread_t, *task_thread_ptr_t;

#define TASK_THREAD_GET_BLOCK( n ) \
  ( task_thread_ptr_t )( ( uint8_t* )n - offsetof( task_thread_t, node ) )

task_thread_ptr_t task_thread_current( void );
void task_thread_set_current( task_thread_ptr_t, task_priority_queue_ptr_t );
size_t task_thread_generate_id( void );
avl_tree_ptr_t task_thread_init( void );
task_thread_ptr_t task_thread_create( uintptr_t, task_process_ptr_t, size_t );
task_thread_ptr_t task_thread_next( void );

#endif
