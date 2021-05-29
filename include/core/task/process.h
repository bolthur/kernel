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
#include <core/mm/virt.h>
#include <core/event.h>

#if ! defined( __CORE_TASK_PROCESS__ )
#define __CORE_TASK_PROCESS__

typedef struct task_thread task_thread_t;
typedef struct task_thread *task_thread_ptr_t;
typedef struct task_thread_manager task_thread_manager_t;
typedef struct task_thread_manager *task_thread_manager_ptr_t;

typedef struct task_stack_manager task_stack_manager_t;
typedef struct task_stack_manager *task_stack_manager_ptr_t;

typedef enum {
  TASK_PROCESS_STATE_INIT = 0,
  TASK_PROCESS_STATE_READY,
  TASK_PROCESS_STATE_ACTIVE,
  TASK_PROCESS_STATE_HALT_SWITCH,
  TASK_PROCESS_STATE_KILL,
} task_process_state_t;

struct task_process {
  avl_node_t node_id;
  avl_tree_ptr_t thread_manager;
  task_stack_manager_ptr_t thread_stack_manager;
  pid_t id;
  pid_t parent;
  pid_t current_thread_id;
  size_t priority;
  char* name;
  virt_context_ptr_t virtual_context;
  task_process_state_t state;
  list_manager_ptr_t message_queue;
};

struct task_process_name {
  avl_node_t node_name;
  char* name;
  list_manager_ptr_t process;
};

struct task_manager {
  avl_tree_ptr_t process_id;
  avl_tree_ptr_t process_name;
  avl_tree_ptr_t thread_priority;
  list_manager_ptr_t process_to_cleanup;
  list_manager_ptr_t thread_to_cleanup;
};

typedef struct task_process task_process_t;
typedef struct task_process *task_process_ptr_t;

typedef struct task_process_name task_process_name_t;
typedef struct task_process_name *task_process_name_ptr_t;
typedef struct task_manager task_manager_t;
typedef struct task_manager *task_manager_ptr_t;

#define TASK_PROCESS_GET_BLOCK_ID( n ) \
  ( task_process_ptr_t )( ( uint8_t* )n - offsetof( task_process_t, node_id ) )

#define TASK_PROCESS_GET_BLOCK_NAME( n ) \
  ( task_process_name_ptr_t )( ( uint8_t* )n - offsetof( task_process_name_t, node_name ) )

extern task_manager_ptr_t process_manager;

bool task_process_init( void );
void task_process_schedule( event_origin_t, void* );
void task_process_cleanup( event_origin_t, void* );
void task_process_start( void );
pid_t task_process_generate_id( void );
task_process_ptr_t task_process_create( size_t, pid_t, const char* );
task_process_ptr_t task_process_fork( task_thread_ptr_t );
bool task_process_prepare_init( task_process_ptr_t );
uintptr_t task_process_prepare_init_arch( task_process_ptr_t );
task_process_ptr_t task_process_get_by_id( pid_t );
list_manager_ptr_t task_process_get_by_name( const char* );
task_process_name_ptr_t task_process_get_name_list( const char* );

#endif
