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
 * @fn uintptr_t align_entry( uintptr_t entry, uintptr_t alignment )
 * @brief Helper to align entry
 * @param entry
 * @param alignment
 * @return
 */
static uintptr_t align_entry( const uintptr_t entry, const uintptr_t alignment ) {
  return (entry + ( alignment - 1 )) & ~( alignment - 1 );
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
  const size_t data_length,
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
  // prepare message_id
  size_t message_id;
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
  // determine mailbox address
  uintptr_t mailbox;
  bool mapped = false;
  // handle same thread
  if ( target_process->id == task_thread_current_thread->process->id ) {
    mailbox = task_thread_current_thread->process->rpc_mailbox_virt;
  // map mailbox temporarily
  } else {
    mailbox = virt_map_temporary( target_process->rpc_mailbox, PAGE_SIZE );
    if ( ! mailbox ) {
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT( "Unable to map queue temporarily!\r\n" )
      #endif
      // return inval
      return EINVAL;
    }
    // set mapped flag
    mapped = true;
  }
  const uintptr_t mailbox_end = mailbox + PAGE_SIZE;
  // set pointer to beginning
  auto entry = ( rpc_data_mailbox_entry_t* )mailbox;
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "===================================> %d <================================\r\n", target )
    DEBUG_OUTPUT( "Mailbox temporarily mapped to 0x%"PRIxPTR", looking for free space \r\n", mailbox )
  #endif
  // loop while entry id is not matching
  while ( ( uintptr_t )entry + sizeof( rpc_data_mailbox_entry_t ) < mailbox_end ) {
    // handle end
    if ( ! entry->id ) {
      break;
    }
    // calculate current entry end
    const uintptr_t current_entry_end = ( uintptr_t )entry + sizeof( rpc_data_mailbox_entry_t ) + entry->length;
    // check for overflow / underflow
    if ( current_entry_end > mailbox_end || current_entry_end < ( uintptr_t )entry ) {
      // unmap mailbox again
      if ( mapped ) {
        virt_unmap_temporary( mailbox, PAGE_SIZE );
      }
      // return fault
      return EFAULT;
    }
    // align current entry end properly
    const uintptr_t next_address = align_entry( current_entry_end, alignof( max_align_t ) );
    // set entry to next address
    entry = ( rpc_data_mailbox_entry_t* )next_address;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Looking for free space finished!\r\n" )
  #endif
  const uintptr_t required_space = sizeof( rpc_data_mailbox_entry_t ) + data_length;
  const uintptr_t entry_end_address = ( uintptr_t )entry + required_space;
  // handle exceed
  if ( entry_end_address >= mailbox_end ) {
    // unmap
    if ( mapped ) {
      virt_unmap_temporary( mailbox, PAGE_SIZE );
    }
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Mailbox full of %d!\r\n", target )
    #endif
    // return nu memory
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
  if ( mapped ) {
    virt_unmap_temporary( mailbox, PAGE_SIZE );
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Everything done!\r\n" )
  #endif
  // return success
  return 0;
}
