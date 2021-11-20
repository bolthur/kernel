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

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <syscall.h>
#include <errno.h>
#include <rpc/data.h>
#include <rpc/generic.h>
#include <task/process.h>
#include <task/thread.h>
#if defined( PRINT_SYSCALL )
  #include <debug/debug.h>
#endif

/**
 * @fn void syscall_rpc_set_handler(void*)
 * @brief System call to set rpc handler
 *
 * @param context
 */
void syscall_rpc_set_handler( void* context ) {
  uintptr_t handler = ( uintptr_t )syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_rpc_set_handler( %#"PRIxPTR" )\r\n", handler )
  #endif
  // create queue if not existing
  if ( ! rpc_generic_setup( task_thread_current_thread->process ) ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // set handler
  task_thread_current_thread->process->rpc_handler = handler;
}

/**
 * @fn void syscall_rpc_raise(void*)
 * @brief Raise rpc system call
 *
 * @param context
 */
void syscall_rpc_raise( void* context ) {
  size_t type = syscall_get_parameter( context, 0 );
  pid_t process = ( pid_t )syscall_get_parameter( context, 1 );
  void* data = ( void* )syscall_get_parameter( context, 2 );
  size_t length = syscall_get_parameter( context, 3 );
  bool sync = ( bool )syscall_get_parameter( context, 4 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_raise( %d, %d, %#p, %#x )\r\n",
      type, process, data, length )
  #endif
  // create queue if not existing
  if ( ! rpc_generic_setup( task_thread_current_thread->process ) ) {
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Error while preparing process!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // validate target
  task_process_ptr_t target = task_process_get_by_id( process );
  if ( ! target ) {
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target not existing / not found: %p - %d!\r\n", target, process )
    #endif
    syscall_populate_error( context, ( size_t )-ESRCH );
    return;
  }
  // check if prepared
  if ( ! rpc_generic_ready( target ) ) {
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target not ready!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // validate addresses
  if ( data && length && ! syscall_validate_address( ( uintptr_t )data, length ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid parameters received / not mapped!\r\n" )
    #endif
    // set return and exit
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // create data duplicate
  char* dup_data = NULL;
  if ( data && length ) {
    dup_data = ( char* )malloc( sizeof( char ) * length );
    if ( ! dup_data ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "dup_data alloc failed %#task_process_get_by_id( process )x / %#x!\r\n",
          length, sizeof( char ) * length )
      #endif
      syscall_populate_error( context, ( size_t )-ENOMEM );
      return;
    }
    // copy from unsafe source
    if ( ! memcpy_unsafe( dup_data, data, length ) ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "memcpy unsafe failed!\r\n" )
      #endif
      free( dup_data );
      syscall_populate_error( context, ( size_t )-EIO );
      return;
    }
  }
  // call rpc
  rpc_backup_ptr_t rpc = rpc_generic_raise(
    task_thread_current_thread,
    target,
    type,
    dup_data,
    length,
    NULL,
    sync
  );
  // free duplicate again
  if ( dup_data ) {
    free( dup_data );
  }
  // handle error
  if ( ! rpc ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "rpc raise failed!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  // block source thread if sync
  if ( sync && task_thread_current_thread != rpc->thread ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "blocking thread with id %d!\r\n",
        task_thread_current_thread->process->id
      )
    #endif
    // block thread
    task_thread_block(
      task_thread_current_thread,
      TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN,
      ( task_state_data_t ){ .data_size = rpc->data_id }
    );
    // enqueue schedule
    event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
  }
}

/**
 * @fn void syscall_rpc_ret(void*)
 * @brief Return rpc data
 *
 * @param context
 */
void syscall_rpc_ret( void* context ) {
  size_t type = syscall_get_parameter( context, 0 );
  void* data = ( void* )syscall_get_parameter( context, 1 );
  size_t length = syscall_get_parameter( context, 2 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_rpc_ret( %#x, %#p, %#x )\r\n", type, data, length )
  #endif
  if ( ! data || 0 == length ) {
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // validate addresses
  if ( ! syscall_validate_address( ( uintptr_t )data, length ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid parameters received / not mapped!\r\n" )
    #endif
    // set return and exit
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // get current active rpc
  rpc_backup_ptr_t active = rpc_backup_get_active( task_thread_current_thread );
  if ( ! active ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No activ rpc found!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // create data duplicate
  char* dup_data = ( char* )malloc( sizeof( char ) * length );
  if ( ! dup_data ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "dup_data alloc failed!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  // copy from unsafe source
  if ( ! memcpy_unsafe( dup_data, data, length ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "memcpy unsafe failed!\r\n" )
    #endif
    free( dup_data );
    syscall_populate_error( context, ( size_t )-EIO );
    return;
  }
  // handle sync stuff
  if ( active->sync ) {
    // generate data queue entry
    size_t data_id = 0;
    int err = rpc_data_queue_add(
      active->source->process->id,
      active->thread->process->id,
      dup_data,
      length,
      &data_id
    );
    if ( err ) {
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "Unable to push return to source data queue\r\n" )
      #endif
      // free duplicate
      free( dup_data );
      syscall_populate_error( context, ( size_t )-EAGAIN );
    }
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "data_id = %d\r\n", data_id )
    #endif
    // populate return
    if ( active->source != task_thread_current_thread ) {
      syscall_populate_success( active->source->current_context, data_id );
    } else {
      syscall_populate_success( active->context, data_id );
    }
  // handle async stuff
  } else {
    // raise target
    rpc_backup_ptr_t backup = rpc_generic_raise(
      active->thread,
      active->source->process,
      type,
      dup_data,
      length,
      NULL,
      false
    );
    // handle error
    if ( ! backup ) {
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "Unable to perform async rpc\r\n" )
      #endif
      free( dup_data );
      syscall_populate_error( context, ( size_t )-EAGAIN );
      return;
    }
    // populate return
    if ( active->source != task_thread_current_thread ) {
      syscall_populate_success( active->source->current_context, 0 );
    } else {
      syscall_populate_success( active->context, 0 );
    }
  }
  // free duplicate
  free( dup_data );
  // unblock if necessary
  task_unblock_threads(
    active->source->process,
    TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN,
    ( task_state_data_t ){ .data_size = active->data_id }
  );
}

/**
 * @fn void syscall_rpc_get_data(void*)
 * @brief Get rpc block data
 *
 * @param context
 */
void syscall_rpc_get_data( void* context ) {
  // parameters
  char* data = ( char* )syscall_get_parameter( context, 0 );
  size_t len = ( size_t )syscall_get_parameter( context, 1 );
  size_t rpc_data_id = ( size_t )syscall_get_parameter( context, 2 );
  bool peek = ( bool )syscall_get_parameter( context, 3 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_rpc_get_data( %#p, %#x, %d )\r\n",
      data, len, rpc_data_id )
  #endif
  // cache process
  task_process_ptr_t target_process = task_thread_current_thread->process;
  // handle error
  if ( 0 == len || ! data || ! rpc_data_id ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process not found!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // handle target not yet ready
  if ( ! target_process->rpc_data_queue ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process not ready for rpc!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // validate addresses
  if ( ! syscall_validate_address( ( uintptr_t )data, len ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid parameters received / not mapped!\r\n" )
    #endif
    // set return and exit
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // Get message by id
  list_item_ptr_t item = list_lookup_data(
    target_process->rpc_data_queue,
    ( void* )rpc_data_id
  );
  // handle not found
  if ( ! item ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No item with id found!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-ENOMSG );
    return;
  }
  rpc_data_queue_entry_ptr_t found = item->data;
  // check for len mismatch
  if ( found->length > len ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "Not enough space for message existing ( %#zx - %#zx )!\r\n",
        found->length,
        len )
    #endif
    syscall_populate_error( context, ( size_t )-EMSGSIZE );
    return;
  }
  uint8_t* tmp = malloc( found->length );
  if ( ! tmp ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "heap allocation error!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  // copy over message content
  memcpy( tmp, found->data, found->length );
  size_t tmp_length = found->length;
  // remove list element
  if ( peek ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "peek rpc parameter only with id %d!\r\n", rpc_data_id )
    #endif
  } else {
    if ( ! list_remove_data(
      task_thread_current_thread->process->rpc_data_queue,
      ( void* )rpc_data_id
    ) ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "Unable to remove data!\r\n" )
      #endif
      free( tmp );
      syscall_populate_error( context, ( size_t )-EIO );
      return;
    }
  }
  // copy over message content
  if ( ! memcpy_unsafe( data, tmp, tmp_length ) ) {
    if ( ! peek && ! list_push_back(
      task_thread_current_thread->process->rpc_data_queue,
      ( void* )tmp
    ) ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "Add back after unsafe copy failed!\r\n" )
      #endif
      free( tmp );
      syscall_populate_error( context, ( size_t )-EIO );
      return;
    }
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Unsafe copy failed!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EIO );
    free( tmp );
    return;
  }
  free( tmp );
}

/**
 * @fn void syscall_rpc_get_data_size(void*)
 * @brief Get rpc data block size
 *
 * @param context
 */
void syscall_rpc_get_data_size( void* context ) {
  size_t rpc_data_id = ( size_t )syscall_get_parameter( context, 0 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_rpc_get_data_size( %#x )\r\n", rpc_data_id )
  #endif
  // cache process
  task_process_ptr_t target_process = task_thread_current_thread->process;
  // handle target not yet ready
  if ( ! rpc_generic_ready( target_process ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process not ready for rpc!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // Get message by id
  list_item_ptr_t item = target_process->rpc_data_queue->first;
  rpc_data_queue_entry_ptr_t found = NULL;
  while( item && ! found ) {
    // get message
    rpc_data_queue_entry_ptr_t rpc = ( rpc_data_queue_entry_ptr_t )item->data;
    // set found when matching
    if( rpc_data_id == rpc->id ) {
      found = rpc;
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
  // return size of found rpc data queue block
  syscall_populate_success( context, found->length );
}

/**
 * @fn void syscall_rpc_wait_for_call(void*)
 * @brief Halt thread and wait for rpc call
 *
 * @param context
 */
void syscall_rpc_wait_for_call( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_rpc_wait_for_call()\r\n" )
  #endif
  // only active state allows block for next syscall
  if ( TASK_THREAD_STATE_ACTIVE != task_thread_current_thread->state ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // set state
  task_thread_current_thread->state = TASK_THREAD_STATE_RPC_WAIT_FOR_CALL;
  // enqueue schedule
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}

/**
 * @fn void syscall_rpc_route(void*)
 * @brief Route call to other process
 *
 * @param context
 */
void syscall_rpc_route( void* context ) {
  // parameter
  size_t rpc_data_id = ( size_t )syscall_get_parameter( context, 0 );
  pid_t process_id = ( pid_t )syscall_get_parameter( context, 1 );
  // get process
  task_process_ptr_t target = task_process_get_by_id( process_id );
  if ( ! target ) {
    syscall_populate_error( context, ( size_t )-ESRCH );
    return;
  }
  // handle target not yet ready
  if ( ! rpc_generic_ready( target ) ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // get data by id
  list_item_ptr_t item = list_lookup_data(
    task_thread_current_thread->process->rpc_data_queue,
    ( void* )rpc_data_id );
  // handle data has already been removed
  if ( ! item ) {
    syscall_populate_error( context, ( size_t )-ENOMSG );
    return;
  }
  rpc_data_queue_entry_ptr_t entry = item->data;
  // get current backup
  rpc_backup_ptr_t current_backup = rpc_backup_get_active(
    task_thread_current_thread );
  if ( ! current_backup ) {
    syscall_populate_error( context, ( size_t )-ESRCH );
    return;
  }
  // raise target
  rpc_backup_ptr_t backup = rpc_generic_raise(
    current_backup->thread,
    target,
    current_backup->type,
    ( void* )entry->data,
    entry->length,
    NULL,
    current_backup->sync
  );
  // handle error
  if ( ! backup ) {
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // return, message has been passed over to process
}

/**
 * @fn void syscall_process_rpc_ready(void*)
 * @brief System call to set rpc ready flag
 *
 * @param context
 */
void syscall_rpc_set_ready( void* context ) {
  bool ready = ( bool )syscall_get_parameter( context, 0 );
  task_thread_current_thread->process->rpc_ready = ready;
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "process %d ready %d\r\n",
      task_thread_current_thread->process->id,
      task_thread_current_thread->process->rpc_ready ? 1 : 0 )
  #endif
}
