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

#include <errno.h>
#include "../lib/inttypes.h"
#include "../lib/string.h"
#include "../lib/stdlib.h"
#include "data.h"
#include "../panic.h"
#include "../mm/phys.h"
#include "../mm/virt.h"
#if defined( PRINT_RPC )
  #include "../debug/debug.h"
#endif

/**
 * @fn void rpc_data_queue_cleanup(list_item_t*)
 * @brief Helper for cleanup
 *
 * @param item
 */
static void rpc_data_queue_cleanup( list_item_t* item ) {
  if ( item->data ) {
    // transform to entry
    rpc_data_queue_entry_t* entry = item->data;
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
 * @fn int32_t rpc_data_queue_lookup(const list_item_t*, const void*)
 * @brief Helper for lookup
 *
 * @param item
 * @param data
 * @return
 */
static int32_t rpc_data_queue_lookup(
  const list_item_t* item,
  const void* data
) {
  // transform to entry
  const rpc_data_queue_entry_t* entry = item->data;
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
 * @fn void rpc_setup_data_queue(task_process_t*)
 * @brief Method to setup rpc data queue for process
 *
 * @param proc
 */
bool rpc_data_queue_setup( task_process_t* proc ) {
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
 * @fn void rpc_data_queue_ready(task_process_t*)
 * @brief Method to check if rpc data queue is ready for process
 *
 * @param proc
 */
bool rpc_data_queue_ready( task_process_t* proc ) {
  return proc->rpc_data_queue;
}

/**
 * @fn void rpc_destroy_data_queue(task_process_t*)
 * @brief Method to destroy rpc data queue of a process
 *
 * @param proc
 */
void rpc_data_queue_destroy( task_process_t* proc ) {
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
 * @fn rpc_data_queue_entry_t* rpc_data_queue_allocate(size_t, char*, size_t*)
 * @brief Helper to allocate rpc data queue entry
 *
 * @param rpc_data_length
 * @param rpc_data
 * @param rpc_id
 * @return
 */
rpc_data_queue_entry_t* rpc_data_queue_allocate(
  size_t rpc_data_length,
  const char* rpc_data,
  size_t* rpc_id
) {
  // reserve space for data queue structure
  rpc_data_queue_entry_t* data_queue_block = malloc( sizeof( *data_queue_block ) );
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
  char* data = malloc( rpc_data_length );
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
  task_process_t* target_process = task_process_get_by_id( target );
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
  rpc_data_queue_entry_t* message = rpc_data_queue_allocate(
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

  // handle no mailbox
  if ( !target_process->rpc_mailbox ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "No message queue allocated!\r\n" )
    #endif
    return EINVAL;
  }
  // map mailbox temporarily
  uintptr_t mailbox = virt_map_temporary( target_process->rpc_mailbox, PAGE_SIZE );
  if ( ! mailbox ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Unable to map queue temporarily!\r\n" )
    #endif
    return EINVAL;
  }
  // set pointer to beginning
  rpc_data_mailbox_entry_t* entry = ( rpc_data_mailbox_entry_t* )mailbox;
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Mailbox temporarily mapped to 0x%"PRIxPTR", looking for free space \r\n", mailbox )
  #endif
  // handle empty
  if ( entry->id ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Mailbox not empty, checking ...\r\n" )
      DEBUG_OUTPUT( "current id: %zx\r\n", entry->id )
      DEBUG_OUTPUT( "current size: %zx\r\n", entry->length )
    #endif
    // loop while there is an entry
    while ( entry->id && data_length < PAGE_SIZE - ( ( uintptr_t )entry - mailbox - sizeof( rpc_data_mailbox_entry_t ) ) ) {
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT( "entry: %#"PRIxPTR"\r\n", (uintptr_t)entry )
      #endif
      entry = ( rpc_data_mailbox_entry_t* )( ( uintptr_t )entry + sizeof( rpc_data_mailbox_entry_t ) + entry->length );
      uintptr_t offset = ( uintptr_t )entry % sizeof( rpc_data_mailbox_entry_t );
      if ( offset ) {
        // debug output
        #if defined( PRINT_RPC )
          DEBUG_OUTPUT(
            "entry: %#"PRIxPTR", offset = %#"PRIxPTR", "
            "sizeof( rpc_data_mailbox_entry_t ) - offset = %#"PRIx32"\r\n",
            (uintptr_t)entry, offset, sizeof( rpc_data_mailbox_entry_t ) - offset )
        #endif
        entry = ( rpc_data_mailbox_entry_t* )( ( uintptr_t )entry + ( sizeof( rpc_data_mailbox_entry_t ) - offset ) );
        if ( ! ( ( uintptr_t )entry >= mailbox && ( uintptr_t )entry < mailbox + PAGE_SIZE ) ) {
          #if defined( PRINT_RPC )
            DEBUG_OUTPUT( "Entry malformed!\r\n" )
            virt_unmap_temporary( mailbox, PAGE_SIZE );
          #endif
          return EFAULT;
        }
        #if defined( PRINT_RPC )
          DEBUG_OUTPUT( "current id: %zx\r\n", entry->id )
          DEBUG_OUTPUT( "current size: %zx\r\n", entry->length )
        #endif
      }
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT(
          "entry: %#"PRIxPTR", offset = %#"PRIxPTR", "
          "sizeof( rpc_data_mailbox_entry_t ) = %#"PRIx32"\r\n",
          (uintptr_t)entry, offset, sizeof( rpc_data_mailbox_entry_t ) )
      #endif
    }
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Looking for free space finished!\r\n" )
  #endif
  // handle no free entry found
  if ( entry->id ) {
    // unmap temporary again
    virt_unmap_temporary( mailbox, PAGE_SIZE );
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Mailbox full!\r\n" )
    #endif
    return ENOMEM;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "data_length = %#zx, max = %#zx!\r\n", data_length, PAGE_SIZE - ( ( uintptr_t )entry - mailbox - sizeof( rpc_data_mailbox_entry_t ) ) )
    DEBUG_OUTPUT( "entry = %#"PRIxPTR", mailbox = %#"PRIxPTR", sizeof( rpc_data_mailbox_entry_t ) = %#zx\r\n", ( uintptr_t )entry, mailbox, sizeof( rpc_data_mailbox_entry_t ) )
  #endif
  // handle to big
  if ( data_length > PAGE_SIZE - ( ( uintptr_t )entry - mailbox - sizeof( rpc_data_mailbox_entry_t ) ) ) {
    // unmap temporary again
    virt_unmap_temporary( mailbox, PAGE_SIZE );
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Mailbox full!\r\n" )
    #endif
    return ENOMEM;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Populating entry id and length( %zd, %#zx )\r\n", message->id, message->length )
  #endif
  // set id and length
  entry->id = message->id;
  entry->length = message->length;
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Copying over data from %#"PRIxPTR" to %#"PRIxPTR"\r\n", ( uintptr_t )data, ( uintptr_t )entry->data )
  #endif
  // copy over data
  memcpy( ( void* )entry->data, data, data_length );
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Unmapping temporary again\r\n" )
  #endif
  // unmap temporary again
  virt_unmap_temporary( mailbox, PAGE_SIZE );

  // prepare structure
  message->sender = sender;
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "pushing back data\r\n" )
  #endif
  // push message to process queue
  list_push_back_data( target_process->rpc_data_queue, message );
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Everything done!\r\n" )
  #endif
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
  task_process_t* target_process = task_process_get_by_id( process );
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
  list_item_t* item = target_process->rpc_data_queue->first;
  rpc_data_queue_entry_t* found = NULL;
  while( item && ! found ) {
    // get entry
    rpc_data_queue_entry_t* rpc = ( rpc_data_queue_entry_t* )item->data;
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
      DEBUG_OUTPUT(
        "found something for removal on process %d with id %zu ( %zu )!\r\n",
        process,
        rpc_id,
        found->id
      )
    #endif
    list_remove_data( target_process->rpc_data_queue, ( void* )found->id );
  }
}
