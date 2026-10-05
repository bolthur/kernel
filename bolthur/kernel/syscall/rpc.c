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

#include <errno.h>
#include <sys/types.h>
#include "../lib/stdlib.h"
#include "../lib/string.h"
#include "../syscall.h"
#include "../debug/debug.h"
#include "../rpc/data.h"
#include "../rpc/generic.h"
#include "../task/process.h"
#include "../task/queue.h"
#include "../task/thread.h"
#if defined( PRINT_SYSCALL )
  #include "../lib/inttypes.h"
  #include "../debug/debug.h"
#endif

/**
 * @fn void syscall_rpc_set_handler(void*)
 * @brief System call to set rpc handler
 *
 * @param context
 */
void syscall_rpc_set_handler( void* context ) {
  const uintptr_t handler = ( uintptr_t )syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_rpc_set_handler( %#"PRIxPTR" )\r\n", handler )
  #endif
  // create queue if not existing
  if ( ! rpc_generic_setup( task_thread_current_thread->process ) ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "rpc generic setup failed!\r\n" )
    #endif
    return;
  }
  // set handler
  task_thread_current_thread->process->rpc_handler = handler;
  // return success
  syscall_populate_success( context, task_thread_current_thread->process->rpc_mailbox_virt );
}

/**
 * @fn void syscall_rpc_wait_for_call(void*)
 * @brief Halt thread and wait for rpc call
 * @param context
 */
void syscall_rpc_wait_for_call( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_wait_for_call() from %d\r\n",
      task_thread_current_thread->process->id
    )
  #endif
  // only active state allows block for next syscall
  if ( TASK_THREAD_STATE_ACTIVE != task_thread_current_thread->state ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "Current thread not in active state, expected %d, but got %d!\r\n",
        TASK_THREAD_STATE_ACTIVE, task_thread_current_thread->state
      )
    #endif
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // set state
  task_thread_set_state( task_thread_current_thread, TASK_THREAD_STATE_RPC_WAIT_FOR_CALL );
  // insert in wait queue
  task_queue_enqueue_blocked( task_thread_current_thread );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "Switched state from %d to %d!\r\n",
      TASK_THREAD_STATE_ACTIVE, task_thread_current_thread->state
    )
  #endif
  // set dummy return
  syscall_populate_success( context, 0 );
  // enqueue scheduler
  event_enqueue( EVENT_PROCESS );
}

/**
 * @fn void syscall_process_rpc_ready(void*)
 * @brief System call to set rpc ready flag
 * @param context
 */
void syscall_rpc_set_ready( void* context ) {
  const bool ready = syscall_get_parameter( context, 0 );
  // cache process
  task_process_t* process = task_thread_current_thread->process;
  // set ready flag
  process->rpc_ready = ready;
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "process %d ready %d\r\n",
      task_thread_current_thread->process->id,
      task_thread_current_thread->process->rpc_ready ? 1 : 0
    )
  #endif
  // unblock parent which might wait for process to be rpc ready!
  const task_process_t* parent = task_process_get_by_id( process->parent );
  if ( parent ) {
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Unblocking process %d\r\n", parent->id )
    #endif
    task_unblock_threads(
      parent,
      TASK_THREAD_STATE_RPC_WAIT_FOR_READY,
      ( task_state_data_t ){ .data_size = ( size_t )process->id }
    );
  }
  // return success
  syscall_populate_success( context, 0 );
}

/**
 * @fn void syscall_rpc_end(void*)
 * @brief RPC ended, return to previous execution or next rpc if queued
 * @param context
 */
void syscall_rpc_end( [[maybe_unused]] void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_end() from %d\r\n",
      task_thread_current_thread->process->id
    )
  #endif
  // check for correct state for rpc end
  if ( TASK_THREAD_STATE_RPC_ACTIVE != task_thread_current_thread->state ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid state set for rpc end" )
    #endif
    return;
  }
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "restore %d\r\n", task_thread_current_thread->process->id )
  #endif
  // try to restore
  if ( ! rpc_generic_restore( task_thread_current_thread ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Error during rpc restore or no rpc for restore -> kill!\r\n" )
    #endif
    // kill thread and trigger scheduling
    task_thread_kill( task_thread_current_thread, true );
    // skip rest
    return;
  }
  // enqueue scheduler
  if ( ! task_thread_is_active( task_thread_current_thread ) ) {
    // insert in wait queue
    task_queue_enqueue_blocked( task_thread_current_thread );
    // enqueue process
    event_enqueue( EVENT_PROCESS );
  }
}

/**
 * @fn void syscall_rpc_wait_for_ready(void*)
 * @brief Wait for pid to be ready for rpc
 * @param context
 */
void syscall_rpc_wait_for_ready( void* context ) {
  // get parameter
  const pid_t process = ( pid_t )syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_wait_for_ready( %d ) from %d\r\n",
      process, task_thread_current_thread->process->id
    )
  #endif
  // get target process
  task_process_t* target = task_process_get_by_id( process );
  // handle no target
  if ( ! target ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "target with id %d not found!\r\n", process )
    #endif
    syscall_populate_error( context, ( size_t )-ESRCH );
    return;
  }
  // handle not parent
  if ( target->parent != task_thread_current_thread->process->id ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "target with id %d is not a child of current process!\r\n",
        target->id
      )
    #endif
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // populate success
  syscall_populate_success( context, 0 );
  // handle already switched to ready state
  if ( target->rpc_ready ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "target %d already ready!\r\n", target->id )
    #endif
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "blocking current thread!\r\n" )
  #endif
  // block thread
  task_thread_block(
    task_thread_current_thread,
    TASK_THREAD_STATE_RPC_WAIT_FOR_READY,
    ( task_state_data_t ){ .data_size = ( uint64_t )process }
  );
  // enqueue schedule
  event_enqueue( EVENT_PROCESS );
}

/**
 * @fn void syscall_rpc_cleanup(void*)
 * @brief Syscall to clean up possible active rpc
 * @param context
 */
void syscall_rpc_cleanup( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_cleanup() from %d\r\n",
      task_thread_current_thread->process->id
    )
  #endif
  // get current active rpc
  auto const active = task_thread_current_thread->current_active_backup;
  #if defined( PRINT_SYSCALL )
    if ( active ) {
      DEBUG_OUTPUT( "cleanup %zu of %d\r\n", active->data_id, task_thread_current_thread->process->id )
    } else {
      DEBUG_OUTPUT( "cleanup %d\r\n", task_thread_current_thread->process->id )
    }
  #endif
  // cleanup if active
  if ( active ) {
    rpc_generic_destroy_source_info( rpc_generic_source_info( active->data_id ) );
  }
  // populate dummy success
  syscall_populate_success( context, 0 );
}
