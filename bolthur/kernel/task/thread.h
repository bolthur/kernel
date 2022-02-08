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

#include <stdint.h>
#include <stddef.h>
#include <stdnoreturn.h>
#include <unistd.h>
#include "../lib/collection/avl.h"
#include "../event.h"
#include "state.h"

#if ! defined( _TASK_THREAD_H )
#define _TASK_THREAD_H

typedef struct task_process task_process_t;
typedef struct task_process* task_process_ptr_t;
typedef struct task_priority_queue task_priority_queue_t;
typedef struct task_priority_queue* task_priority_queue_ptr_t;

struct task_thread {
  void* current_context;
  avl_node_t node_id;
  pid_t id;
  size_t priority;
  uintptr_t stack_virtual;
  uint64_t stack_physical;
  size_t stack_size;
  uintptr_t entry;
  task_thread_state_t state;
  task_thread_state_t state_backup;
  task_state_data_t state_data;
  task_process_ptr_t process;
};

typedef struct task_thread task_thread_t;
typedef struct task_thread* task_thread_ptr_t;

extern task_thread_ptr_t task_thread_current_thread;

#define TASK_THREAD_GET_BLOCK( n ) \
  ( task_thread_ptr_t )( ( uint8_t* )n - offsetof( task_thread_t, node_id ) )
#define TASK_THREAD_GET_CONTEXT  \
  ( task_thread_current_thread ? task_thread_current_thread->current_context : NULL )

bool task_thread_set_current( task_thread_ptr_t, task_priority_queue_ptr_t );
void task_thread_reset_current( void );
pid_t task_thread_generate_id( task_process_ptr_t );
avl_tree_ptr_t task_thread_init( void );
void task_thread_destroy( avl_tree_ptr_t );
task_thread_ptr_t task_thread_create( uintptr_t, task_process_ptr_t, size_t );
task_thread_ptr_t task_thread_fork( task_process_ptr_t, task_thread_ptr_t );
task_thread_ptr_t task_thread_next( void );
noreturn void task_thread_switch_to( uintptr_t );
bool task_thread_push_arguments( task_thread_ptr_t, char**, char** );
void task_thread_cleanup( event_origin_t, void* );
void task_thread_block( task_thread_ptr_t, task_thread_state_t, task_state_data_t );
void task_thread_unblock( task_thread_ptr_t, task_thread_state_t, task_state_data_t );
task_thread_ptr_t task_thread_get_blocked( task_thread_state_t, task_state_data_t );
void task_thread_kill( task_thread_ptr_t, bool, void* );

#endif
