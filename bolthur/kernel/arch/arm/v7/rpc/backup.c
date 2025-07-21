/**
 * Copyright (C) 2018 - 2025 bolthur project.
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

#include "../../../../lib/stdlib.h"
#include "../../../../lib/string.h"
#include "../../../../lib/inttypes.h"
#include "../cpu.h"
#include "../../../../mm/virt.h"
#include "../../../../rpc/backup.h"
#include "../../../../rpc/data.h"
#if defined( PRINT_RPC )
  #include "../../../../debug/debug.h"
#endif

/**
 * @fn rpc_backup_t* rpc_backup_create(task_thread_t*, task_process_t*, size_t, void*, size_t, task_thread_t*, bool, size_t, bool)
 * @brief Helper to create rpc backup
 *
 * @param source
 * @param target
 * @param type
 * @param data
 * @param data_size
 * @param target_thread
 * @param sync
 * @param origin_data_id
 * @param disable_data
 * @return
 */
rpc_backup_t* rpc_backup_create(
  task_thread_t* source,
  task_process_t* target,
  size_t type,
  void* data,
  size_t data_size,
  task_thread_t* target_thread,
  bool sync,
  size_t origin_data_id,
  bool disable_data
) {
  // get first inactive thread
  avl_node_t* current = avl_iterate_first( target->thread_manager );
  task_thread_t* thread = target_thread;
  // loop until usable thread has been found
  while ( current && ! thread ) {
    // get thread
    task_thread_t* tmp = TASK_THREAD_GET_BLOCK( current );
    // FIXME: CHECK IF ACTIVE
    thread = tmp;
    // get next thread
    current = avl_iterate_next( target->thread_manager, current );
  }
  // handle no inactive thread
  if ( ! thread ) {
    return NULL;
  }
  // ensure correct state
  if ( ! thread->process->rpc_ready ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "thread not ready %d!\r\n", thread->state )
    #endif
    return NULL;
  }

  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "thread->id = %d\r\n", thread->id )
    DEBUG_OUTPUT( "thread->process->id = %d\r\n", thread->process->id )
  #endif

  // reserve space for backup object
  rpc_backup_t* backup = malloc( sizeof( *backup ) );
  if ( ! backup ) {
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Unable to reserve memory for backup structure!\r\n" )
    #endif
    return NULL;
  }
  // clear out
  memset( backup, 0, sizeof( *backup ) );
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Reserved backup object: %p\r\n", backup )
  #endif

  // variables
  list_item_t* current_list = target->rpc_queue->first;
  rpc_backup_t* active = NULL;
  // try to find matching rpc
  while( current_list ) {
    // get current backup
    rpc_backup_t* tmp = current_list->data;
    // when backup is active, thread is the same and thread state is
    // not wait for rpc call use current entry
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "process = %d, tmp->active = %d, tmp->thread = %p, thread = %p, tmp->data_id = %zu, thread->state = %d\r\n",
        tmp->thread->process->id, tmp->active ? 1 : 0, tmp->thread, thread, tmp->data_id, thread->state )
    #endif
    // handle not active, different thread or wait for return
    if ( ! tmp->active || tmp->thread != thread
      || thread->state == TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN
      || thread->state == TASK_THREAD_STATE_RPC_HALT_SWITCH
    ) {
      // get to next item
      current_list = current_list->next;
      // skip rest
      continue;
    }
    // some debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "tmp = %p\r\n", ( void* )tmp )
      DEBUG_OUTPUT( "process = %d, tmp->active = %d, tmp->thread = %p, thread = %p, tmp->data_id = %zu\r\n",
        tmp->thread->process->id, tmp->active ? 1 : 0, tmp->thread, thread, tmp->data_id )
    #endif
    // set active
    active = tmp;
    // break out of loop
    break;
  }
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "active = %p\r\n", ( void* )active)
  #endif
  // get thread cpu context
  const cpu_register_context_t* cpu = thread->current_context;
  if ( active ) {
    cpu = active->context;
  }
  // reserve space for backup context
  backup->context = malloc( sizeof( cpu_register_context_t ) );
  if ( ! backup->context ) {
    rpc_backup_destroy( backup );
    return NULL;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Reserved backup cpu context: %p\r\n", backup->context )
  #endif
  // prepare and backup context area
  memset( backup->context, 0, sizeof( cpu_register_context_t ) );
  memcpy( backup->context, cpu, sizeof( cpu_register_context_t ) );
  // debug output
  #if defined( PRINT_RPC )
    DUMP_REGISTER( backup->context )
  #endif
  // backup parameter data as message
  backup->data_id = 0;
  if ( ! disable_data ) {
    if ( data && data_size ) {
      int err = rpc_data_queue_add(
        thread->process->id,
        data,
        data_size,
        &backup->data_id
      );
      if ( err ) {
        // debug output
        #if defined( PRINT_RPC )
          DEBUG_OUTPUT( "Adding to queue failed with code %d\r\n", err )
        #endif
        rpc_backup_destroy( backup );
        return NULL;
      }
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT(
          "Sent message from process %d to %d\r\n",
          source->process->id,
          thread->process->id
        )
      #endif
    } else {
      char dummy = '\0';
      int err = rpc_data_queue_add(
        thread->process->id,
        &dummy,
        sizeof( char ),
        &backup->data_id
      );
      if ( err ) {
        // debug output
        #if defined( PRINT_RPC )
          DEBUG_OUTPUT( "Adding to queue failed with code %d\r\n", err )
        #endif
        rpc_backup_destroy( backup );
        return NULL;
      }
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT(
          "Sent dummy message from process %d to %d\r\n",
          source->process->id,
          thread->process->id
        )
        DEBUG_OUTPUT( "type = %zu, data_id = %zu\r\n", type, backup->data_id )
      #endif
    }
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "async: %d, type: %zu\r\n", sync ? 0 : 1, type )
  #endif
  // populate remaining values
  backup->thread = thread;
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "pid: %d, backup->thread_state = %d, thread->state = %d\r\n",
      thread->process->id, backup->thread_state, thread->state )
  #endif
  backup->thread_state = thread->state;
  memcpy( &backup->thread_state_data, &thread->state_data, sizeof( task_state_data_t ) );
  if ( TASK_THREAD_STATE_RPC_WAIT_FOR_CALL == backup->thread_state ) {
    backup->thread_state = TASK_THREAD_STATE_ACTIVE;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT(
      "backup->thread_state = %d, backup->thread_state_data.data_ptr = %p\r\n",
      backup->thread_state,
      backup->thread_state_data.data_ptr
    )
  #endif
  backup->prepared = false;
  backup->source = source;
  backup->type = type;
  backup->sync = sync;
  backup->origin_data_id = origin_data_id;
  backup->sync_return_data_id = 0;
  backup->sync_return_blocked_data_id = 0;
  backup->sync_return_on_end = false;
  // return created backup
  return backup;
}
