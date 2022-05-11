/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include <inttypes.h>
#include <errno.h>
#include "../lib/string.h"
#include "../lib/stdlib.h"
#include "data.h"
#include "../panic.h"
#if defined( PRINT_RPC )
  #include "../debug/debug.h"
#endif

/**
 * @fn void rpc_data_queue_cleanup(const list_item_ptr_t)
 * @brief Helper for cleanup
 *
 * @param item
 */
static void rpc_data_queue_cleanup( const list_item_ptr_t item ) {
  if ( item->data ) {
    // transform to entry
    const rpc_data_queue_entry_ptr_t entry = ( rpc_data_queue_entry_ptr_t )item->data;
    // free data if set
    if ( entry->data ) {
      free( ( void* )entry->data );
    }
    // free entry
    free( entry );
  }
  // continue with default list cleanup
  list_default_cleanup( item );
}

/**
 * @fn int32_t rpc_data_queue_lookup(const list_item_ptr_t, const void*)
 * @brief Helper for lookup
 *
 * @param item
 * @param data
 * @return
 */
static int32_t rpc_data_queue_lookup(
  const list_item_ptr_t item,
  const void* data
) {
  // transform to entry
  const rpc_data_queue_entry_ptr_t entry = item->data;
  return entry->id == ( size_t )data ? 0 : 1;
}

/**
 * @brief Generates new rpc data queue id
 *
 * @return
 */
size_t rpc_data_queue_generate_id( void ) {
  static size_t id = 1;
  return id++;
}

/**
 * @fn void rpc_setup_data_queue(task_process_ptr_t)
 * @brief Method to setup rpc data queue for process
 *
 * @param proc
 */
bool rpc_data_queue_setup( task_process_ptr_t proc ) {
  // stop if already setup
  if ( proc->rpc_data_queue ) {
    return true;
  }
  // prepare rpc data queue
  proc->rpc_data_queue = list_construct(
    rpc_data_queue_lookup,
    rpc_data_queue_cleanup,
    NULL
  );
  return proc->rpc_data_queue;
}


/**
 * @fn void rpc_data_queue_ready(task_process_ptr_t)
 * @brief Method to check if rpc data queue is ready for process
 *
 * @param proc
 */
bool rpc_data_queue_ready( task_process_ptr_t proc ) {
  return proc->rpc_data_queue;
}

/**
 * @fn void rpc_destroy_data_queue(task_process_ptr_t)
 * @brief Method to destroy rpc data queue of a process
 *
 * @param proc
 */
void rpc_data_queue_destroy( task_process_ptr_t proc ) {
  // handle no rpc data queue
  if ( ! proc->rpc_data_queue ) {
    return;
  }
  // destroy rpc data queue
  list_destruct( proc->rpc_data_queue );
  // set to NULL after destroy
  proc->rpc_data_queue = NULL;
}

/**
 * @fn rpc_data_queue_entry_ptr_t rpc_data_queue_allocate(size_t, char*, size_t*)
 * @brief Helper to allocate rpc data queue entry
 *
 * @param rpc_data_length
 * @param rpc_data
 * @param rpc_id
 * @return
 */
rpc_data_queue_entry_ptr_t rpc_data_queue_allocate(
  size_t rpc_data_length,
  const char* rpc_data,
  size_t* rpc_id
) {
  // reserve space for data queue structure
  rpc_data_queue_entry_ptr_t data_queue_block = ( rpc_data_queue_entry_ptr_t )malloc(
    sizeof( rpc_data_queue_entry_t ) );
  if ( ! data_queue_block ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "No free space left for rpc data queue structure!\r\n" )
    #endif
    // return NULL
    return NULL;
  }
  // erase
  memset( data_queue_block, 0, sizeof( rpc_data_queue_entry_t ) );
  // reserve space for data_queue_block->data
  char* data = ( char* )malloc( rpc_data_length );
  if ( ! data ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "No free space left for data_queue_block data!\r\n" )
    #endif
    // free
    free( data_queue_block );
    // return NULL
    return NULL;
  }
  // copy over content
  memcpy( data, rpc_data, rpc_data_length );
  // prepare necessary data
  data_queue_block->data = data;
  data_queue_block->length = rpc_data_length;
  // prepare rpc id
  if ( ! rpc_id || 0 == *rpc_id ) {
    data_queue_block->id = rpc_data_queue_generate_id();
    // set rpc id
    if ( rpc_id ) {
      *rpc_id = data_queue_block->id;
    }
  } else {
    data_queue_block->id = *rpc_id;
  }
  // return data queue block
  return data_queue_block;
}

/**
 * @fn int rpc_data_queue_add(pid_t, pid_t, const char*, size_t, size_t*)
 * @brief Method to add rpc data queue entry
 *
 * @param target
 * @param sender
 * @param data
 * @param data_length
 * @param rpc_data_queue_id
 * @return
 */
int rpc_data_queue_add(
  pid_t target,
  pid_t sender,
  const char* data,
  size_t data_length,
  size_t* rpc_data_queue_id
) {
  // get process by pid
  task_process_ptr_t target_process = task_process_get_by_id( target );
  // handle error
  if ( ! target_process ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Target process not found!\r\n" )
    #endif
    return EINVAL;
  }
  // handle error
  if ( ! target_process->rpc_data_queue ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Target process has no queue!\r\n" )
    #endif
    return EINVAL;
  }
  // handle invalid length
  if ( 0 == data_length ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Invalid length passed!\r\n" )
    #endif
    return EINVAL;
  }

  // reserve space for message structure
  rpc_data_queue_entry_ptr_t message = rpc_data_queue_allocate(
    data_length,
    data,
    rpc_data_queue_id
  );
  if ( ! message ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "No free space left for message structure!\r\n" )
    #endif
    return ENOMEM;
  }
  // Save message id in pointer if not null
  if ( rpc_data_queue_id ) {
    *rpc_data_queue_id = message->id;
  }
  // prepare structure
  message->sender = sender;
  // push message to process queue
  list_push_back( target_process->rpc_data_queue, message );
  // return success
  return 0;
}

/**
 * @fn void rpc_data_queue_remove(pid_t, size_t)
 * @brief Helper to remove rpc data queue entry by id
 *
 * @param process
 * @param rpc_id
 */
void rpc_data_queue_remove( pid_t process, size_t rpc_id ) {
  // get process by pid
  task_process_ptr_t target_process = task_process_get_by_id( process );
  // handle error
  if ( ! target_process ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Target process not found!\r\n" )
    #endif
    return;
  }
  // handle error
  if ( ! target_process->rpc_data_queue ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Target process has no queue!\r\n" )
    #endif
    return;
  }
  // Get rpc data queue entry by id
  list_item_ptr_t item = target_process->rpc_data_queue->first;
  rpc_data_queue_entry_ptr_t found = NULL;
  while( item && ! found ) {
    // get entry
    rpc_data_queue_entry_ptr_t rpc = ( rpc_data_queue_entry_ptr_t )item->data;
    // set found when matching
    if( rpc_id == rpc->id ) {
      found = rpc;
    }
    // head over to next
    item = item->next;
  }
  // remove entry if found
  if ( found ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "found something for removal!\r\n" )
    #endif
    list_remove_data( target_process->rpc_data_queue, ( void* )found );
  }
}

