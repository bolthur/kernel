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

#include <stddef.h>
#include <stdnoreturn.h>
#include <unistd.h>
#include <collection/avl.h>
#include <collection/list.h>
#include <mm/virt.h>
#include <event.h>
#include <task/state.h>

#if ! defined( _TASK_PROCESS_H )
#define _TASK_PROCESS_H

typedef struct task_thread task_thread_t;
typedef struct task_thread* task_thread_ptr_t;
typedef struct task_thread_manager task_thread_manager_t;
typedef struct task_thread_manager* task_thread_manager_ptr_t;

typedef struct task_stack_manager task_stack_manager_t;
typedef struct task_stack_manager* task_stack_manager_ptr_t;

struct task_process {
  avl_node_t node_id;
  avl_tree_ptr_t thread_manager;
  task_stack_manager_ptr_t thread_stack_manager;
  pid_t id;
  pid_t parent;
  pid_t current_thread_id;
  size_t priority;
  virt_context_ptr_t virtual_context;
  task_process_state_t state;
  task_state_data_t state_data;
  list_manager_ptr_t message_queue;
  uintptr_t message_handler;
};

struct task_manager {
  avl_tree_ptr_t process_id;
  avl_tree_ptr_t thread_priority;
  list_manager_ptr_t process_to_cleanup;
  list_manager_ptr_t thread_to_cleanup;
};

typedef struct task_process task_process_t;
typedef struct task_process* task_process_ptr_t;

typedef struct task_manager task_manager_t;
typedef struct task_manager* task_manager_ptr_t;

#define TASK_PROCESS_GET_BLOCK_ID( n ) \
  ( task_process_ptr_t )( ( uint8_t* )n - offsetof( task_process_t, node_id ) )

extern task_manager_ptr_t process_manager;

bool task_process_init( void );
void task_process_schedule( event_origin_t, void* );
void task_process_cleanup( event_origin_t, void* );
void task_process_start( void );
pid_t task_process_generate_id( void );
task_process_ptr_t task_process_create( size_t, pid_t );
task_process_ptr_t task_process_fork( task_thread_ptr_t );
bool task_process_prepare_init( task_process_ptr_t );
uintptr_t task_process_prepare_init_arch( task_process_ptr_t );
task_process_ptr_t task_process_get_by_id( pid_t );
void task_process_prepare_kill( void*, task_process_ptr_t );
int task_process_replace( task_process_ptr_t, uintptr_t, const char**, const char**, void* );
void task_unblock_threads( task_process_ptr_t, task_thread_state_t, task_state_data_t );

#endif
