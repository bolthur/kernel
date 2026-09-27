/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#ifndef _TASK_THREAD_H
#define _TASK_THREAD_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include "../../library/collection/avl/avl.h"
#include "../event.h"
#include "state.h"
#include "../timer.h"

typedef struct task_process task_process_t;

typedef struct rpc_backup rpc_backup_t;

typedef struct  task_thread {
  /** current context */
  void* current_context;
  /** queue avl management node */
  avl_node_t queue_node;
  /** thread id */
  pid_t id;
  /** virtual stack address */
  uintptr_t stack_virtual;
  /** physical stack address array */
  uint64_t* stack_physical;
  /** stack size */
  size_t stack_size;
  /** root entry point */
  uintptr_t entry;
  /** current thread state */
  task_thread_state_t state;
  /** thread state backup */
  task_thread_state_t state_backup;
  /** thread state data */
  task_state_data_t state_data;
  /** pointer to process structure */
  task_process_t* process;
  /** flag indicating thread is handling an interrupt */
  bool handling_interrupt;
  /** interruptable sleep timer */
  timer_callback_entry_t* interruptable_sleep_timer;
  /** currently active rpc */
  rpc_backup_t* current_active_backup;
  /** virtual total runtime */
  uint64_t vruntime;
  /** weight */
  uint32_t weight;
  /** nice level of the task */
  uint32_t nice_level;
} task_thread_t;

extern task_thread_t* task_thread_current_thread;
extern task_thread_t* task_thread_try_switch_to;
extern const uint32_t task_thread_priority_weight[ 40 ];

#define TASK_THREAD_NICE_LEVEL_0 1024

#define TASK_THREAD_GET_QUEUE_BLOCK( n ) \
  ( task_thread_t* )( ( uint8_t* )n - offsetof( task_thread_t, queue_node ) )

bool task_thread_set_current( task_thread_t* );
void task_thread_reset_current( void );
pid_t task_thread_generate_id( void );
list_manager_t* task_thread_init( void );
void task_thread_destroy( avl_tree_t* );
task_thread_t* task_thread_create( uintptr_t, task_process_t*, size_t );
task_thread_t* task_thread_fork( task_process_t*, const task_thread_t* );
[[noreturn]] void task_thread_switch_to( uintptr_t );
bool task_thread_push_arguments( const task_thread_t*, char**, char** );
void task_thread_cleanup( event_origin_t, void* );
void task_thread_block( task_thread_t*, task_thread_state_t, task_state_data_t );
void task_thread_unblock( task_thread_t*, task_thread_state_t, task_state_data_t );
task_thread_t* task_thread_get_blocked( task_thread_state_t, task_state_data_t );
void task_thread_kill( task_thread_t*, bool );
bool task_thread_is_ready( const task_thread_t* );
bool task_thread_is_active( const task_thread_t* );
void task_thread_set_state( task_thread_t*, task_thread_state_t );

#endif
