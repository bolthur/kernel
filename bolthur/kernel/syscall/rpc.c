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

#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <syscall.h>
#include <errno.h>
#include <ipc/rpc.h>
#include <ipc/rpc.h>
#include <task/process.h>
#include <task/thread.h>
#if defined( PRINT_SYSCALL )
  #include <debug/debug.h>
#endif

/**
 * @fn void syscall_rpc_acquire(void*)
 * @brief Acquire rpc slot by name
 *
 * @param context
 */
void syscall_rpc_acquire( void* context ) {
  char* identifier = ( char* )syscall_get_parameter( context, 0 );
  uintptr_t handler = ( uintptr_t )syscall_get_parameter( context, 1 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_acquire( %s, %#"PRIxPTR" )\r\n",
      identifier, handler )
  #endif
  // register handler
  if ( ! rpc_register_handler(
    identifier,
    task_thread_current_thread->process,
    handler
  ) ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  syscall_populate_success( context, 0 );
}

/**
 * @fn void syscall_rpc_release(void*)
 * @brief Release rpc handler by name
 *
 * @param context
 */
void syscall_rpc_release( void* context ) {
  char* identifier = ( char* )syscall_get_parameter( context, 0 );
  uintptr_t handler = ( uintptr_t )syscall_get_parameter( context, 1 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_release( %s, %#"PRIxPTR" )\r\n",
      identifier, handler )
  #endif
  // register handler
  if ( ! rpc_unregister_handler(
    identifier,
    task_thread_current_thread->process,
    handler
  ) ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  syscall_populate_success( context, 0 );
}

/**
 * @fn void syscall_rpc_raise(void*)
 * @brief Raise rpc call by name
 *
 * @param context
 */
void syscall_rpc_raise( void* context ) {
  char* identifier = ( char* )syscall_get_parameter( context, 0 );
  pid_t target = ( pid_t )syscall_get_parameter( context, 1 );
  void* data = ( void* )syscall_get_parameter( context, 2 );
  size_t length = ( size_t )syscall_get_parameter( context, 3 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_raise( %s, %d, %#p, %#x )\r\n",
      identifier, target, data, length )
  #endif
  // call rpc
  if ( ! rpc_raise(
    identifier,
    task_thread_current_thread,
    task_process_get_by_id( target ),
    data,
    length
  ) ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
}

/**
 * @fn void syscall_rpc_raise_wait(void*)
 * @brief Raise rpc call and wait for response
 *
 * @param context
 */
void syscall_rpc_raise_wait( void* context ) {
  char* identifier = ( char* )syscall_get_parameter( context, 0 );
  pid_t target = ( pid_t )syscall_get_parameter( context, 1 );
  void* data = ( void* )syscall_get_parameter( context, 2 );
  size_t length = ( size_t )syscall_get_parameter( context, 3 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_raise_wait( %s, %d, %#p, %#x )\r\n",
      identifier, target, data, length )
  #endif
  // call rpc
  rpc_backup_ptr_t rpc = rpc_raise(
    identifier,
    task_thread_current_thread,
    task_process_get_by_id( target ),
    data,
    length
  );
  // handle error
  if ( ! rpc ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // block source thread
  if ( task_thread_current_thread != rpc->thread ) {
    // block thread
    task_thread_block(
      task_thread_current_thread,
      TASK_THREAD_STATE_RPC_WAITING,
      ( task_state_data_t ){ .data_size = rpc->message_id }
    );
    // enqueue schedule
    event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
  }
}

/**
 * @fn void syscall_rpc_ret(void*)
 * @brief Return rpc value
 *
 * @param context
 */
void syscall_rpc_ret( void* context ) {
  void* data = ( void* )syscall_get_parameter( context, 0 );
  size_t length = ( size_t )syscall_get_parameter( context, 1 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_rpc_ret( %#p, %#x )\r\n", data, length )
  #endif
  // get current active rpc
  rpc_backup_ptr_t active = rpc_get_active( task_thread_current_thread );
  if ( ! active ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // source not blocked? => error
  if (
    (
      active->source->state != TASK_THREAD_STATE_RPC_WAITING
      && active->source != task_thread_current_thread
    ) || active->source->state_data.data_size != active->message_id
  ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }

  // create response message
  size_t message_id = 0;
  int err = message_send_by_pid(
    active->source->process->id,
    task_thread_current_thread->process->id,
    0, /// FIXME: ENSURE THAT THIS WON'T BREAL ANYTHING
    data,
    length,
    0, /// FIXME: ENSURE THAT THIS WON'T BREAL ANYTHING
    &message_id
  );
  // handle error
  if ( err ) {
    syscall_populate_error( context, ( size_t )-err );
    return;
  }
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "message_id = %d\r\n", message_id )
  #endif
  // populate return
  if ( active->source != task_thread_current_thread ) {
    syscall_populate_success( active->source->current_context, message_id );
  } else {
    syscall_populate_success( active->context, message_id );
  }
  // unblock if necessary
  task_unblock_threads(
    active->source->process,
    TASK_THREAD_STATE_RPC_WAITING,
    ( task_state_data_t ){ .data_size = active->message_id }
  );
}

/**
 * @fn void syscall_rpc_get_data(void*)
 * @brief Get message by message id
 *
 * @param context
 */
void syscall_rpc_get_data( void* context ) {
  // parameters
  char* data = ( char* )syscall_get_parameter( context, 0 );
  size_t length = ( size_t )syscall_get_parameter( context, 1 );
  size_t message_id = ( size_t )syscall_get_parameter( context, 2 );

  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_rpc_get_data( %#p, %#x, %d )\r\n",
      data, length, message_id )
  #endif
  // cache process
  task_process_ptr_t target_process = task_thread_current_thread->process;
  // handle error
  if ( ! target_process || 0 == length ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process not found!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-ESRCH );
    return;
  }
  // handle error
  if ( ! target_process->message_queue ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process has no queue!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // Get message by id
  list_item_ptr_t item = target_process->message_queue->first;
  message_entry_ptr_t found = NULL;
  while( item && ! found ) {
    // get message
    message_entry_ptr_t msg = ( message_entry_ptr_t )item->data;
    // set found when matching
    if( message_id == msg->id ) {
      found = msg;
    }
    // head over to next
    item = item->next;
  }
  // handle not found
  if ( ! found ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No message with id found!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-ENOMSG );
    return;
  }
  // check for length mismatch
  if ( found->length > length ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "Not enough space for message existing ( %#zx - %#zx )!\r\n",
        found->length,
        length );
    #endif
    syscall_populate_error( context, ( size_t )-EMSGSIZE );
    return;
  }
  // copy over message content
  memcpy( data, found->data, found->length );
  // remove list element
  list_remove_data(
    task_thread_current_thread->process->message_queue,
    ( void* )found
  );
}
