/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#include <unistd.h>
#include <errno.h>
#include "../lib/inttypes.h"
#include "../lib/stdlib.h"
#include "../lib/string.h"
#include "../syscall.h"
#include "../rpc/data.h"
#include "../rpc/generic.h"
#include "../task/process.h"
#include "../task/thread.h"
#if defined( PRINT_SYSCALL )
  #include "../debug/debug.h"
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
  // return success
  syscall_populate_success( context, 0 );
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
  size_t origin_rpc_data_id = syscall_get_parameter( context, 4 );
  bool synchronous = ( bool )syscall_get_parameter( context, 5 );
  bool no_return = ( bool )syscall_get_parameter( context, 6 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_raise( %zu, %d, %p, %#zx, %zu, %d, %d ) from %d\r\n",
      type,
      process,
      data,
      length,
      origin_rpc_data_id,
      synchronous ? 1 : 0,
      no_return ? 1 : 0,
      task_thread_current_thread->process->id
    )
  #endif
  // create queue if not existing
  if ( ! rpc_generic_setup( task_thread_current_thread->process ) ) {
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Error while preparing process!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // handle invalid type
  if ( type <= UINT8_MAX  ) {
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Interrupts are not allowed to be raised!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // validate target
  task_process_t* target = task_process_get_by_id( process );
  if ( ! target ) {
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "Target not existing / not found: %p - %d!\r\n",
        target,
        process
      )
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
    dup_data = malloc( sizeof( char ) * length );
    if ( ! dup_data ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT(
          "dup_data alloc failed %#zx / %#zx!\r\n",
          length,
          sizeof( char ) * length
        )
      #endif
      syscall_populate_error( context, ( size_t )-ENOMEM );
      return;
    }
    // copy from unsafe source
    if ( ! memcpy_unsafe_src( dup_data, data, length ) ) {
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
  rpc_backup_t* rpc = rpc_generic_raise(
    task_thread_current_thread,
    target,
    type,
    dup_data,
    length,
    NULL,
    synchronous,
    origin_rpc_data_id,
    false
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
  // handle no return
  if ( no_return ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "no return rpc call\r\n" )
    #endif
    syscall_populate_success( context, 0 );
    return;
  // block source thread if synchronous
  } else if ( synchronous && task_thread_current_thread != rpc->thread ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "blocking process with id %d ( %d / %zu )!\r\n",
        task_thread_current_thread->process->id,
        TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN,
        rpc->data_id
      )
    #endif
    // block thread
    task_thread_block(
      task_thread_current_thread,
      TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN,
      ( task_state_data_t ){ .data_size = rpc->data_id }
    );
  // return data id for async request to allow handling in user space
  } else if ( ! synchronous ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "rpc->data_id = %zu\r\n", rpc->data_id )
    #endif
    syscall_populate_success( context, rpc->data_id );
  }
  // switch it
  if ( task_thread_current_thread != rpc->thread ) {
    // enqueue scheduler
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
  size_t original_rpc_id = syscall_get_parameter( context, 3 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_ret( %zu, %p, %#zx, %zu ) from %d\r\n",
      type,
      data,
      length,
      original_rpc_id,
      task_thread_current_thread->process->id
    )
  #endif
  if ( ! data || 0 == length ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No data / length passed!\r\n" )
    #endif
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
  rpc_backup_t* active = rpc_backup_get_active( task_thread_current_thread );
  if ( ! active ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No activ rpc found!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // create data duplicate
  char* dup_data = malloc( sizeof( char ) * length );
  if ( ! dup_data ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "dup_data alloc failed!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  // copy from unsafe source
  if ( ! memcpy_unsafe_src( dup_data, data, length ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "memcpy unsafe failed!\r\n" )
    #endif
    free( dup_data );
    syscall_populate_error( context, ( size_t )-EIO );
    return;
  }
  // overwrite target in case original rpc id is set for correct unblock
  task_thread_t* target = active->source;
  size_t blocked_data_id = active->data_id;
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "blocked_data_id = %zu, original_rpc_id = %zu\r\n",
      blocked_data_id,
      original_rpc_id
    )
  #endif
  rpc_origin_source_t* info = rpc_generic_source_info(
    original_rpc_id ? original_rpc_id : active->data_id );
  if ( ! active->sync || original_rpc_id ) {
    // overwrite blocked data id
    blocked_data_id = original_rpc_id ? original_rpc_id : active->data_id;
    target = task_thread_get_blocked(
      TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN,
      ( task_state_data_t ){ .data_size = blocked_data_id }
    );
    // handle no target
    if ( ! target ) {
      if ( ! info ) {
        #if defined( PRINT_SYSCALL )
          DEBUG_OUTPUT(
            "No blocked thread found with %d / %zu\r\n",
            TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN,
            original_rpc_id
          )
        #endif
        // free duplicate
        free( dup_data );
        syscall_populate_error( context, ( size_t )-EINVAL );
        return;
      }
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT(
          "rpc_id: %zu, source: %d\r\n",
          info->rpc_id,
          info->source_process
        )
      #endif
      // try to get pid
      task_process_t* proc = task_process_get_by_id( info->source_process );
      if ( ! proc ) {
        #if defined( PRINT_SYSCALL )
          DEBUG_OUTPUT(
            "No process found by id %d\r\n",
            info->source_process
          )
        #endif
        // free duplicate
        free( dup_data );
        syscall_populate_error( context, ( size_t )-ESRCH );
        return;
      }
      // in case there is no target, use source and treat it as async
      // use first possible process
      avl_node_t* current = avl_iterate_first( proc->thread_manager );
      target = NULL;
      // loop until usable thread has been found
      while ( current && ! target ) {
        // get thread
        task_thread_t* tmp = TASK_THREAD_GET_BLOCK( current );
        // FIXME: CHECK IF ACTIVE
        target = tmp;
        // get next thread
        current = avl_iterate_next( proc->thread_manager, current );
      }
      // handle no inactive thread
      if ( ! target ) {
        #if defined( PRINT_SYSCALL )
          DEBUG_OUTPUT( "No thread found for id %d\r\n", info->source_process )
        #endif
        // free duplicate
        free( dup_data );
        syscall_populate_error( context, ( size_t )-ESRCH );
        return;
      }
      // reset sync to false
      active->sync = false;
      blocked_data_id = info->rpc_id;
    }
  }
  // destroy info
  rpc_generic_destroy_source_info( info );
  // find and destroy possible for current
  if ( original_rpc_id ) {
    rpc_generic_destroy_source_info(
      rpc_generic_source_info( active->data_id )
    );
  }
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "blocked_data_id = %zu, original_rpc_id = %zu\r\n",
      blocked_data_id,
      original_rpc_id
    )
    DEBUG_OUTPUT( "active = %p!\r\n", active )
  #endif

  // handle synchronous stuff
  if ( active->sync ) {
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Sync return!\r\n" )
    #endif
    // generate data queue entry
    size_t data_id = 0;
    int err = rpc_data_queue_add(
      target->process->id,
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
      return;
    }
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "data_id = %zu\r\n", data_id )
      DEBUG_OUTPUT(
        "target->state = %d, target->state_data = %p\r\n",
        target->state,
        target->state_data.data_ptr
      )
    #endif
    // populate return for sync request ( rpc raise is waiting at source )
    syscall_populate_success(
      target != task_thread_current_thread
        ? target->current_context
        : active->context,
      data_id
    );
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "unblock threads of process %d and blocked data %zu\r\n",
        target->process->id,
        blocked_data_id
      )
    #endif
    // unblock if necessary
    task_unblock_threads(
      target->process,
      TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN,
      ( task_state_data_t ){ .data_size = blocked_data_id }
    );
  // handle async stuff
  } else {
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "async return!\r\n" )
    #endif
    // raise target
    rpc_backup_t* backup = rpc_generic_raise(
      active->thread,
      target->process,
      type,
      dup_data,
      length,
      NULL,
      true,
      blocked_data_id,
      false
    );
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "active->data_id = %zu, backup->data_id = %zu, type = %zu\r\n",
        active->data_id,
        backup->data_id,
        type
      )
    #endif
    // handle error
    if ( ! backup ) {
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "Unable to perform async rpc\r\n" )
      #endif
      free( dup_data );
      syscall_populate_error( context, ( size_t )-EAGAIN );
      return;
    }
  }
  // free duplicate
  free( dup_data );
  // return success
  if ( target != task_thread_current_thread ) {
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Set dummy success value!\r\n" )
    #endif
    syscall_populate_success( context, 0 );
    // enqueue scheduler
    //event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
  }
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
  bool peek = ( size_t )syscall_get_parameter( context, 3 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_get_data( %p, %#zx, %zu, %d ) from %d\r\n",
      data,
      len,
      rpc_data_id,
      peek ? 1 : 0,
      task_thread_current_thread->process->id
    )
  #endif
  // cache process
  task_process_t* target_process = task_thread_current_thread->process;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Validating parameter!\r\n" )
  #endif
  // handle error
  if ( 0 == len || ! data || ! rpc_data_id ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process not found!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Check for rpc data queue!\r\n" )
  #endif
  // handle target not yet ready
  if ( ! target_process->rpc_data_queue ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process not ready for rpc!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EAGAIN );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "validate address!\r\n" )
  #endif
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
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Searching for rpc data id %zu!\r\n", rpc_data_id )
  #endif
  // Get message by id
  list_item_t* item = list_lookup_data(
    target_process->rpc_data_queue,
    ( void* )rpc_data_id
  );
  // handle not found
  if ( ! item ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No item with id %zu found!\r\n", rpc_data_id )
    #endif
    syscall_populate_error( context, ( size_t )-ENOMSG );
    return;
  }
  rpc_data_queue_entry_t* found = item->data;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Validate data length of found item!\r\n" )
  #endif
  // check for len mismatch
  if ( found->length > len ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "Not enough space for message existing ( %#zx - %#zx )!\r\n",
        found->length,
        len
      )
    #endif
    syscall_populate_error( context, ( size_t )-EMSGSIZE );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Copy content!\r\n" )
  #endif
  // copy over message content
  if ( ! memcpy_unsafe_dst( data, found->data, found->length ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Unsafe copy failed!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EIO );
    return;
  }
  // remove list element
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Removing from queue!\r\n" )
  #endif
  if ( ! peek && ! list_remove_data(
    task_thread_current_thread->process->rpc_data_queue,
    ( void* )rpc_data_id
  ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Unable to remove data!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EIO );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "return success!\r\n" )
  #endif
  // return success
  syscall_populate_success( context, 0 );
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
    DEBUG_OUTPUT( "syscall_rpc_get_data_size( %zu )\r\n", rpc_data_id )
  #endif
  // cache process
  task_process_t* target_process = task_thread_current_thread->process;
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
  list_item_t* item = target_process->rpc_data_queue->first;
  rpc_data_queue_entry_t* found = NULL;
  while( item && ! found ) {
    // get message
    rpc_data_queue_entry_t* rpc = ( rpc_data_queue_entry_t* )item->data;
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
  task_thread_current_thread->state = TASK_THREAD_STATE_RPC_WAIT_FOR_CALL;
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
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}

/**
 * @fn void syscall_process_rpc_ready(void*)
 * @brief System call to set rpc ready flag
 *
 * @param context
 */
void syscall_rpc_set_ready( void* context ) {
  bool ready = ( bool )syscall_get_parameter( context, 0 );
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
  task_process_t* parent = task_process_get_by_id( process->parent );
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
 *
 * @param context
 */
void syscall_rpc_end( void* context ) {
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
  // try to restore
  if ( ! rpc_generic_restore( task_thread_current_thread ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Error during rpc restore or no rpc for restore -> kill!\r\n" )
    #endif
    // kill thread and trigger scheduling
    task_thread_kill( task_thread_current_thread, true, context );
  }
  // enqueue scheduler
  if ( ! task_thread_is_active( task_thread_current_thread ) ) {
    event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
  }
}

/**
 * @fn void syscall_rpc_wait_for_ready(void*)
 * @brief Wait for pid to be ready for rpc
 *
 * @param context
 */
void syscall_rpc_wait_for_ready( void* context ) {
  // get parameter
  pid_t process = ( pid_t )syscall_get_parameter( context, 0 );
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
    ( task_state_data_t ){ .data_size = ( size_t )process }
  );
  // enqueue schedule
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}

/**
 * @fn void syscall_rpc_clear_data(void*)
 * @brief Method to clear rpc data
 *
 * @param context
 */
void syscall_rpc_clear_data( void* context ) {
  // parameters
  size_t rpc_data_id = ( size_t )syscall_get_parameter( context, 0 );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_clear_data( %zu ) from %d\r\n",
      rpc_data_id,
      task_thread_current_thread->process->id
    )
  #endif
  // cache process
  task_process_t* target_process = task_thread_current_thread->process;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Validating parameter!\r\n" )
  #endif
  // handle error
  if ( ! rpc_data_id ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process not found!\r\n" )
    #endif
    syscall_populate_success( context, 0 );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Check for rpc data queue!\r\n" )
  #endif
  // handle target not yet ready
  if ( ! target_process->rpc_data_queue ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process not ready for rpc!\r\n" )
    #endif
    syscall_populate_success( context, 0 );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Searching for rpc data id %zu!\r\n", rpc_data_id )
  #endif
  // Get message by id
  list_item_t* item = list_lookup_data(
    target_process->rpc_data_queue,
    ( void* )rpc_data_id
  );
  // handle not found
  if ( ! item ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No item with id %zu found!\r\n", rpc_data_id )
    #endif
    syscall_populate_success( context, 0 );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Removing from queue!\r\n" )
  #endif
  // remove list element
  while (
    ! list_remove_data(
      task_thread_current_thread->process->rpc_data_queue,
      ( void* )rpc_data_id
    )
  ) {}
  // return success
  syscall_populate_success( context, 0 );
}
