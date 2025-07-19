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

#include "../lib/string.h"
#include "../lib/stdlib.h"
#include "backup.h"
#include "data.h"
#if defined( PRINT_RPC )
  #include "../debug/debug.h"
#endif

/**
 * @fn void rpc_backup_destroy(rpc_backup_t*)
 * @brief backup to destroy
 *
 * @param backup
 */
void rpc_backup_destroy( rpc_backup_t* backup ) {
  // handle null
  if ( ! backup ) {
    return;
  }
  // free context
  if ( backup->context ) {
    free( backup->context );
  }
  // free backup
  free( backup );
}

/**
 * @fn rpc_backup_t* rpc_backup_get_active(task_thread_t*, size_t)
 * @brief Get active rpc backup
 *
 * @param thread thread to get backup from
 * @param data_id data id to get backup from
 * @return active backup or null if no rpc is active or not found
 */
rpc_backup_t* rpc_backup_get_active( task_thread_t* thread, size_t data_id ) {
  // ensure proper states
  if (
    TASK_THREAD_STATE_RPC_ACTIVE != thread->state
    && TASK_THREAD_STATE_RPC_QUEUED != thread->state
    && TASK_THREAD_STATE_RPC_HALT_SWITCH != thread->state
    && TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN != thread->state
  ) {
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "thread->state = %d\r\n", thread->state )
    #endif
    return NULL;
  }
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "thread->state = %d\r\n", thread->state )
  #endif
  // variables
  list_item_t* current = thread->process->rpc_queue->first;
  rpc_backup_t* found = NULL;
  // handle data id set
  if ( data_id ) {
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Trying to get rpc backup by data id %zu\r\n", data_id )
    #endif
    while ( current ) {
      rpc_backup_t* entry = current->data;
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT( "entry->data_id = %zu, entry->origin_data_id = %zu\r\n",
          entry->data_id, entry->origin_data_id )
      #endif
      // handle data id match
      if ( entry->origin_data_id == data_id ) {
        #if defined( PRINT_RPC )
          DEBUG_OUTPUT( "Found rpc backup by data id %zu\r\n", data_id )
        #endif
        // return found entry
        return entry;
      }
      // go to next
      current = current->next;
    }
    // return null
    return found;
  }
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Trying to get last active thread\r\n" )
  #endif
  // try to get active rpc backup
  while( current ) {
    rpc_backup_t* entry = current->data;
    // handle usual return not nested
    if ( entry->active ) {
      found = entry;
    }
    // go to next
    current = current->next;
  }
  // return null
  return found;
}
