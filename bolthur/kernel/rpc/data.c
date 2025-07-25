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
#include "../mm/phys.h"
#include "../mm/virt.h"
#if defined( PRINT_RPC )
  #include "../debug/debug.h"
#endif

/**
 * @fn size_t rpc_data_queue_generate_id(void)
 * @brief Generates new rpc data queue id
 *
 * @return
 */
size_t rpc_data_queue_generate_id( void ) {
  static size_t id = 1;
  return id++;
}

/**
 * @fn void rpc_data_queue_ready(task_process_t*)
 * @brief Method to check if rpc data queue is ready for process
 *
 * @param proc
 */
bool rpc_data_queue_ready( task_process_t* proc ) {
  return proc->rpc_mailbox > 0 && proc->rpc_mailbox_virt > 0;
}

/**
 * @fn int rpc_data_queue_add(pid_t, const char*, size_t, size_t*)
 * @brief Method to add rpc data queue entry
 *
 * @param target
 * @param data
 * @param data_length
 * @param rpc_data_queue_id
 * @return
 */
int rpc_data_queue_add(
  const pid_t target,
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

  // handle invalid length
  if ( 0 == data_length ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Invalid length passed!\r\n" )
    #endif
    return EINVAL;
  }

  size_t message_id;
  // prepare message_id
  if ( ! rpc_data_queue_id || 0 == *rpc_data_queue_id ) {
    message_id = rpc_data_queue_generate_id();
    // set message_id
    if ( rpc_data_queue_id ) {
      *rpc_data_queue_id = message_id;
    }
  } else {
    message_id = *rpc_data_queue_id;
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
  auto rpc_data_mailbox_entry_t* entry = ( rpc_data_mailbox_entry_t* )mailbox;
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
      DEBUG_OUTPUT( "Mailbox full of %d!\r\n", target )
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
      DEBUG_OUTPUT( "Mailbox full of %d!\r\n", target )
    #endif
    return ENOMEM;
  }
  // set id and length
  entry->id = message_id;
  entry->length = data_length;
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

  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Everything done!\r\n" )
  #endif
  // return success
  return 0;
}
