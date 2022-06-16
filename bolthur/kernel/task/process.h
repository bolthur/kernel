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

#include <stddef.h>
#include <stdnoreturn.h>
#include <unistd.h>
#include "../../library/collection/avl/avl.h"
#include "../../library/collection/list/list.h"
#include "../mm/virt.h"
#include "../event.h"
#include "state.h"

#ifndef _TASK_PROCESS_H
#define _TASK_PROCESS_H

typedef struct task_thread task_thread_t;
typedef struct task_thread_manager task_thread_manager_t;

typedef struct task_stack_manager task_stack_manager_t;

typedef struct task_process {
  avl_node_t node_id;
  avl_tree_t* thread_manager;
  task_stack_manager_t* thread_stack_manager;
  pid_t id;
  pid_t parent;
  pid_t current_thread_id;
  size_t priority;
  virt_context_t* virtual_context;
  list_manager_t* rpc_data_queue;
  list_manager_t* rpc_queue;
  uintptr_t rpc_handler;
  bool rpc_ready;
} task_process_t;

typedef struct task_manager {
  // process id tree
  avl_tree_t* process_id;
  // thread priority tree
  avl_tree_t* thread_priority;
  // list of processes to cleanup
  list_manager_t* process_to_cleanup;
  // list of threads to cleanup
  list_manager_t* thread_to_cleanup;
} task_manager_t;

#define TASK_PROCESS_GET_BLOCK_ID( n ) \
  ( task_process_t* )( ( uint8_t* )n - offsetof( task_process_t, node_id ) )

extern task_manager_t* process_manager;

bool task_process_init( void );
void task_process_schedule( event_origin_t, void* );
void task_process_cleanup( event_origin_t, void* );
void task_process_start( void );
pid_t task_process_generate_id( void );
task_process_t* task_process_create( size_t, pid_t );
task_process_t* task_process_fork( task_thread_t* );
bool task_process_prepare_init( task_process_t* );
uintptr_t task_process_prepare_init_arch( task_process_t* );
task_process_t* task_process_get_by_id( pid_t );
void task_process_prepare_kill( void*, task_process_t* );
int task_process_replace( task_process_t*, uintptr_t, const char**, const char**, void* );
void task_unblock_threads( task_process_t*, task_thread_state_t, task_state_data_t );

#endif
