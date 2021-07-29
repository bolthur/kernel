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

#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <core/interrupt.h>
#include <core/syscall.h>
#include <core/mm/phys.h>
#include <core/mm/virt.h>
#include <core/task/process.h>
#include <core/task/thread.h>
#if defined( PRINT_SYSCALL )
  #include <core/debug/debug.h>
#endif
#include <platform/rpi/syscall.h>
#include <platform/rpi/mailbox/mailbox.h>
#include <platform/rpi/mailbox/property.h>

/**
 * @fn void syscall_mailbox_action(void*)
 * @brief Mailbox action interface
 *
 * @param context
 *
 * @todo revise dump implementation below
 * @todo data buffer is unsafe, so unsafe copy is needed here
 * @todo add mailbox request validation
 * @todo add allowed mailbox action check here
 * @todo fail in case action is not allowed explicitly
 */
void syscall_mailbox_action( void* context ) {
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_mailbox_action()\r\n" );
  #endif
  // get index
  ptb_index = ( int32_t )syscall_get_parameter( context, 1 );
  int32_t* data_buffer = ( int32_t* )syscall_get_parameter( context, 0 );
  // copy action data
  memcpy(
    ( uint8_t* )ptb_buffer,
    ( uint8_t* )data_buffer,
    ( size_t )ptb_index * 4
  );
  // get context
  virt_context_ptr_t virtual_context = task_thread_current_thread
    ->process
    ->virtual_context;
  // process property
  mailbox_property_process();

  // special handling
  size_t count = 2;
  while ( ptb_buffer[ count ] ) {

    if ( TAG_ALLOCATE_BUFFER == ptb_buffer[ count ] ) {
      uintptr_t buffer_start = ( uintptr_t )ptb_buffer[ count + 3 ];
      size_t buffer_size = ( size_t )ptb_buffer[ count + 4 ];
      uintptr_t mapping_start = virt_find_free_page_range( virtual_context, buffer_size, 0 );
      if ( ! mapping_start ) {
        syscall_populate_error( context, ( size_t )-ENOMEM );
        return;
      }
      uintptr_t start = mapping_start;
      uintptr_t end = start + buffer_size;
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT(
          "Allocated buffer: %#"PRIxPTR" with size of %#x\r\n",
          buffer_start, buffer_size )
        DEBUG_OUTPUT( "Mapping buffer to %#"PRIxPTR"\r\n", start )
      #endif
      // loop from start to end
      while( start < end ) {
        // map framebuffer
        if ( ! virt_map_address(
          virtual_context,
          start,
          buffer_start,
          VIRT_MEMORY_TYPE_DEVICE,
          VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
        ) ) {
          syscall_populate_error( context, ( size_t )-ENOMEM );
          return;
        }
        // increase start and virtual
        start += PAGE_SIZE;
        buffer_start += PAGE_SIZE;
      }
      // replace buffer address with mapped one
      ptb_buffer[ count + 3 ] = ( int32_t )mapping_start;
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "count + 3 = %d\r\n", count + 3 )
        DEBUG_OUTPUT( "count + 4 = %d\r\n", count + 4 )
        buffer_start = ( uintptr_t )ptb_buffer[ count + 3 ];
        buffer_size = ( size_t )ptb_buffer[ count + 4 ];
        DEBUG_OUTPUT(
          "Allocated mapped buffer at %#"PRIxPTR" with size of %#x\r\n",
          buffer_start, buffer_size )
      #endif
    }
    // get to next tag ( tag + size + value + buffer size )
    count += 3 + ( ( size_t )( ptb_buffer[ count + 1 ] >> 2 ) );
  }
  memcpy(
    ( uint8_t* )data_buffer,
    ( uint8_t* )ptb_buffer,
    ( size_t )ptb_index * 4
  );
}
