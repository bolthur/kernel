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
#include "../../../../syscall.h"
#include "../../../../rpc/generic.h"
#include "../../../../rpc/data.h"
#include "../../../../lib/stdlib.h"
#include "../../../../lib/string.h"
#if defined( PRINT_SYSCALL )
  #include "../../../../debug/debug.h"
  #include "../../../../lib/inttypes.h"
#endif

/**
 * @fn void syscall_rpc_raise(void*)
 * @brief Raise rpc system call
 * @param context
 */
void syscall_rpc_raise( void* context ) {
  const uint32_t lower_origin_rpc_data_id = syscall_get_parameter( context, 0 );
  const uint32_t upper_origin_rpc_data_id = syscall_get_parameter( context, 1 );
  const uint64_t origin_rpc_data_id = ( uint64_t )upper_origin_rpc_data_id << 32 | lower_origin_rpc_data_id;
  const size_t type = syscall_get_parameter( context, 2 );
  const pid_t process = ( pid_t )syscall_get_parameter( context, 3 );
  auto const data = ( void* )syscall_get_parameter( context, 4 );
  const size_t length = syscall_get_parameter( context, 5 );
  const bool synchronous = syscall_get_parameter( context, 6 );
  const bool no_return = syscall_get_parameter( context, 7 );
  const bool cleanup_current_id = syscall_get_parameter( context, 8 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_rpc_raise( %zu, %d, %p, %#zx, %zu, %d, %d, %d ) from %d\r\n",
      type,
      process,
      data,
      length,
      origin_rpc_data_id,
      synchronous ? 1 : 0,
      no_return ? 1 : 0,
      no_return ? 1 : 0,
      task_thread_current_thread->process->id
    )
  #endif
  // create queue if not existing
  if ( ! rpc_generic_setup( task_thread_current_thread->process ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Error while preparing process!\r\n" )
    #endif
    // error return
    syscall_populate_error( context, ( size_t )-EAGAIN );
    // early exit
    return;
  }
  // handle invalid type
  if ( type <= UINT8_MAX ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Interrupts are not allowed to be raised!\r\n" )
    #endif
    // populate error
    syscall_populate_error( context, ( size_t )-EINVAL );
    // early exit
    return;
  }
  // validate target
  task_process_t* target = task_process_get_by_id( process );
  if ( ! target ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "Target not existing / not found: %p - %d!\r\n",
        target,
        process
      )
    #endif
    // populate error
    syscall_populate_error( context, ( size_t )-ESRCH );
    // early exit
    return;
  }
  // check if prepared
  if ( ! rpc_generic_ready( target ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target not ready!\r\n" )
    #endif
    // populate error
    syscall_populate_error( context, ( size_t )-EAGAIN );
    // early exit
    return;
  }
  // FIXME: handle possible kill
  // validate addresses
  if ( data && length && ! syscall_validate_address( ( uintptr_t )data, length ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid parameters received / not mapped!\r\n" )
    #endif
    // set return and exit
    syscall_populate_error( context, ( size_t )-EINVAL );
    // early exit
    return;
  }
  // create data duplicate
  char* dup_data = nullptr;
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
      // populate error
      syscall_populate_error( context, ( size_t )-ENOMEM );
      // early exit
      return;
    }
    // copy from unsafe source
    if ( ! memcpy_unsafe_src( dup_data, data, length ) ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "memcpy unsafe failed!\r\n" )
      #endif
      // free again duplicated data
      free( dup_data );
      // populate error
      syscall_populate_error( context, ( size_t )-EIO );
      // early exit
      return;
    }
  }
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "calling process %d!\r\n",
      target->id
    )
  #endif
  // call rpc
  rpc_backup_t* rpc = rpc_generic_raise(
    task_thread_current_thread,
    target,
    type,
    dup_data,
    length,
    nullptr,
    synchronous,
    origin_rpc_data_id,
    false,
    false,
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
    // set error response
    syscall_populate_error( context, ( size_t )-ENOMEM );
    // skip rest
    return;
  }
  // handle cleanup
  if ( cleanup_current_id ) {
    // get current active rpc
    const rpc_backup_t* active = task_thread_current_thread->current_active_backup;
    // cleanup if active
    if ( active ) {
      rpc_generic_destroy_source_info( rpc_generic_source_info( active->data_id ) );
    }
  }
  // handle no return
  if ( no_return ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "no return rpc call\r\n" )
    #endif
    // set success response
    syscall_populate_success( context, 0 );
    // skip rest
    return;
  }
  // block source thread if synchronous
  if ( synchronous && task_thread_current_thread != rpc->thread ) {
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
    // populate data id in backup in case it's in current thread and already active
    if ( rpc->thread == task_thread_current_thread && rpc->active ) {
      syscall_populate_success64( rpc->context, rpc->data_id );
    // populate regular success via context
    } else {
      syscall_populate_success64( context, rpc->data_id );
    }
  }
  // switch it
  if ( task_thread_current_thread != rpc->thread && synchronous ) {
    // enqueue scheduler
    if ( ! task_thread_try_switch_to ) {
      task_thread_try_switch_to = rpc->thread;
    }
    // enqueue process event
    event_enqueue( EVENT_PROCESS );
  }
}

/**
 * @fn void syscall_rpc_ret(void*)
 * @brief Return rpc data
 * @param context
 */
void syscall_rpc_ret( void* context ) {
  const uint32_t lower_original_rpc_id = syscall_get_parameter( context, 0 );
  const uint32_t upper_original_rpc_id = syscall_get_parameter( context, 1 );
  const uint64_t original_rpc_id = ( uint64_t )upper_original_rpc_id << 32 | lower_original_rpc_id;
  const size_t type = syscall_get_parameter( context, 2 );
  auto const data = ( void* )syscall_get_parameter( context, 3 );
  const size_t length = syscall_get_parameter( context, 4 );
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
  rpc_backup_t* active = task_thread_current_thread->current_active_backup;
  if ( ! active ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No activ rpc found for %zu!\r\n", original_rpc_id )
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
  uint64_t blocked_data_id = active->data_id;
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "blocked_data_id = %zu, original_rpc_id = %zu\r\n",
      blocked_data_id,
      original_rpc_id
    )
    DEBUG_OUTPUT( "active->sync = %d\r\n", active->sync ? 1 : 0 )
  #endif
  rpc_origin_source_t* info = rpc_generic_source_info(
    original_rpc_id ? original_rpc_id : active->data_id );
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "active->sync = %d, info->sync = %d, rpc_id: %zu, source: %d, origin_rpc_id: %zu, type: %zu\r\n",
        active->sync ? 1 : 0,
        info->sync ? 1 : 0,
        info->rpc_id,
        info->source_process,
        info->origin_rpc_id,
        info->type )
  #endif
  if ( ! active->sync || original_rpc_id ) {
    // overwrite blocked data id
    blocked_data_id = original_rpc_id ? original_rpc_id : active->data_id;
    target = task_thread_get_blocked(
      TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN,
      ( task_state_data_t ){ .data_size = blocked_data_id }
    );
    // in case we have an original rpc id a valid target and a valid info object
    // we need to overwrite sync and blocked_data_id similar to when no target
    // was initially found
    if ( original_rpc_id && target && info && active->type != type ) {
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT(
          "rpc_id: %zu, source: %d, sync: %d, origin_rpc_id: %zu, type: %zu\r\n",
          info->rpc_id,
          info->source_process,
          info->sync ? 1 : 0,
          info->origin_rpc_id,
          info->type
        )
        DEBUG_OUTPUT(
          "sync = %d, data_id: %zu, original_data_id: %zu, type: %zu / %zu\r\n",
          active->sync ? 1 : 0,
          active->data_id,
          active->origin_data_id,
          active->type,
          type
        )
      #endif
      // reset sync to one from info
      active->sync = info->sync;
    }
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
          "rpc_id: %zu, source: %d, sync: %d, origin_rpc_id: %zu, type: %zu\r\n",
          info->rpc_id,
          info->source_process,
          info->sync ? 1 : 0,
          info->origin_rpc_id,
          info->type
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
      target = nullptr;
      auto current = proc->thread_list->first;
      // loop until usable thread has been found
      while ( current && ! target ) {
        // get thread
        auto const tmp = ( task_thread_t* )current->data;
        // FIXME: CHECK IF ACTIVE
        target = tmp;
        // get next thread
        current = current->next;
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
      // reset sync to one from info
      active->sync = info->sync;
      blocked_data_id = info->rpc_id;
    }
  }
  // destroy found info
  rpc_generic_destroy_source_info( info );
  // find and destroy possible info for current if original rpc id is set
  // in case it's an interrupt it is not possible to clean up active context
  // since we might have multiple returns
  if ( original_rpc_id && ! active->is_interrupt && ! active->is_timer ) {
    rpc_generic_destroy_source_info( rpc_generic_source_info( active->data_id ) );
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
      DEBUG_OUTPUT( "Sync return to %d!\r\n", target->process->id )
    #endif
    // generate data queue entry
    uint64_t data_id = 0;
    const int err = rpc_data_queue_add(
      target->process->id,
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
      // populate error
      syscall_populate_error( context, ( size_t )-err );
      // skip rest
      return;
    }
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "data_id = %zu\r\n", data_id )
      DEBUG_OUTPUT(
        "target->state = %d, target->state_data = %p\r\n",
        target->state,
        target->state_data.data_ptr
      )
      DEBUG_OUTPUT( "Using target: %d, using active: %d\r\n",
        target != task_thread_current_thread ? 1 : 0,
        target != task_thread_current_thread ? 0 : 1 )
    #endif

    // get possible active target backup
    rpc_backup_t* target_active = nullptr;
    if (
      target != task_thread_current_thread
      && target->current_active_backup
      && target->current_active_backup->origin_data_id == blocked_data_id
    ) {
      target_active = target->current_active_backup;
    }
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "target_active = %p\r\n", ( void* )target_active )
    #endif
    // handle not active
    if ( ! target_active ) {
      // populate return for sync request ( rpc raise is waiting at source )
      syscall_populate_success64(
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
    } else {
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "sync return to %d on end!\r\n", target_active->thread->process->id )
      #endif
      target_active->sync_return_on_end = true;
      target_active->sync_return_blocked_data_id = blocked_data_id;
      target_active->sync_return_data_id = data_id;
    }
  // handle async stuff
  } else {
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "async return to %d!\r\n", target->process->id )
    #endif
    // raise target
    rpc_backup_t* backup = rpc_generic_raise(
      active->thread,
      target->process,
      type,
      dup_data,
      length,
      nullptr,
      true,
      blocked_data_id,
      false,
      false,
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
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Set dummy success value!\r\n" )
    #endif
    // dummy success
    syscall_populate_success( context, 0 );
  }
}
