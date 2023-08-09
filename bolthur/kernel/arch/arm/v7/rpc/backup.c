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

#include "../../../../lib/stdlib.h"
#include "../../../../lib/string.h"
#include "../../../../lib/inttypes.h"
#include "../cpu.h"
#include "../../barrier.h"
#include "../../../../mm/phys.h"
#include "../../../../mm/virt.h"
#include "../../../../rpc/backup.h"
#include "../../../../rpc/data.h"
#include "../../cache.h"
#include "../../../../panic.h"
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
  while( current_list && ! active ) {
    rpc_backup_t* tmp = current_list->data;
    if ( tmp->active && tmp->thread == thread ) {
      active = tmp;
      break;
    }
    // switch to next
    current_list = current_list->next;
  }
  // get thread cpu context
  cpu_register_context_t* cpu = active
    ? active->context
    : thread->current_context;
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
        source->process->id,
        data,
        data_size,
        &backup->data_id
      );
      if ( err ) {
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
        source->process->id,
        &dummy,
        sizeof( char ),
        &backup->data_id
      );
      if ( err ) {
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
  backup->thread_state = thread->state;
  memcpy(
    &backup->thread_state_data,
    &thread->state_data,
    sizeof( task_state_data_t )
  );
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
  // return created backup
  return backup;
}
