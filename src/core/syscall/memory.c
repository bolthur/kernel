
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
#include <core/syscall.h>
#if defined( PRINT_SYSCALL )
  #include <core/debug/debug.h>
#endif
#include <core/mm/phys.h>
#include <core/mm/virt.h>
#include <core/mm/shared.h>
#include <core/task/process.h>
#include <core/task/thread.h>

#define MEMORY_ACQUIRE_PROTECTION_NONE 0x0
#define MEMORY_ACQUIRE_PROTECTION_READ 0x1
#define MEMORY_ACQUIRE_PROTECTION_WRITE 0x2
#define MEMORY_ACQUIRE_PROTECTION_EXECUTABLE 0x4

/**
 * @brief Acquire memory
 *
 * @param context
 */
void syscall_memory_acquire( void* context ) {
  // get parameter
  void* addr = ( void* )syscall_get_parameter( context, 0 );
  size_t len = ( size_t )syscall_get_parameter( context, 1 );
  int protection = ( int )syscall_get_parameter( context, 2 );
  // get context
  virt_context_ptr_t virtual_context = task_thread_current_thread
    ->process
    ->virtual_context;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall memory acquire( %#p, %zu, %d )\r\n",
      addr, len, protection )
  #endif

  // handle invalid length
  if ( 0 == len ) {
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }

  // get full page count
  ROUND_UP_TO_FULL_PAGE( len )
  // determine start
  uintptr_t start;
  // fixed handling means take address as start
  if ( addr ) {
    start = ( uintptr_t )addr;
    // get min and max address of context
    uintptr_t min = virt_get_context_min_address( virtual_context );
    uintptr_t max = virt_get_context_max_address( virtual_context );
    // ensure that address is in context
    if ( min > start || max <= start || max <= start + len ) {
      syscall_populate_error( context, ( size_t )-ENOMEM );
      return;
    }
  // find free page range
  } else {
    start = virt_find_free_page_range( virtual_context, len, ( uintptr_t )addr );
  }

  // handle no address found
  if ( ( uintptr_t )NULL == start ) {
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }

  // mapping flags
  uint32_t map_flag = 0;
  if ( protection & MEMORY_ACQUIRE_PROTECTION_READ ) {
    map_flag |= VIRT_PAGE_TYPE_READ;
  }
  if ( protection & MEMORY_ACQUIRE_PROTECTION_WRITE ) {
    map_flag |= VIRT_PAGE_TYPE_WRITE;
  }
  if ( protection & MEMORY_ACQUIRE_PROTECTION_EXECUTABLE ) {
    map_flag |= VIRT_PAGE_TYPE_EXECUTABLE;
  }

  // map address range with random physical memory
  if ( ! virt_map_address_range_random(
    virtual_context,
    start,
    len,
    VIRT_MEMORY_TYPE_NORMAL,
    map_flag
  ) ) {
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "start = %#"PRIxPTR"\r\n", start )
  #endif
  // return to process with address
  syscall_populate_success( context, start );
}

/**
 * @brief Release memory
 *
 * @param context
 */
void syscall_memory_release( void* context ) {
  // get parameters
  uintptr_t address = ( uintptr_t )syscall_get_parameter( context, 0 );
  size_t len = ( size_t )syscall_get_parameter( context, 1 );
  bool unmap_phys = true;
  // context
  virt_context_ptr_t virtual_context = task_thread_current_thread
    ->process
    ->virtual_context;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_memory_release( %#"PRIxPTR", %zx )\r\n",
      address, len )
  #endif
  // handle invalid stuff
  if ( 0 == len ) {
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // get at least full page size
  ROUND_UP_TO_FULL_PAGE( len )
  // check range
  uintptr_t min = virt_get_context_min_address( virtual_context );
  uintptr_t max = virt_get_context_max_address( virtual_context );
  // handle invalid address
  if (
    ! (
      min <= address
      && max > address
      && max > ( address + len )
    )
  ) {
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }

  // check for shared area
  if ( shared_memory_address_is_shared(
    task_thread_current_thread->process,
    address,
    len
  ) ) {
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }

  // check if range is mapped in context
  if ( ! virt_is_mapped_in_context_range( virtual_context, address, len ) ) {
    syscall_populate_success( context, 0 );
    return;
  }
  // try to unmap
  if ( ! virt_unmap_address_range( virtual_context, address, len, unmap_phys ) ) {
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // return success
  syscall_populate_success( context, 0 );
}

/**
 * @brief Acquire new or existing shared memory area
 *
 * @param context
 */
void syscall_memory_shared_create( void* context ) {
  // get parameters
  size_t len = ( size_t )syscall_get_parameter( context, 0 );
  // round up to full page
  ROUND_UP_TO_FULL_PAGE( len )
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_memory_shared_acquire( %zx )\r\n", len )
  #endif
  // create shared area
  size_t id = shared_memory_create( len );
  // handle error
  if ( 0 == id ) {
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  // create and populate return
  syscall_populate_success( context, id );
}

/**
 * @brief attach shared memory
 *
 * @param context
 */
void syscall_memory_shared_attach( void* context ) {
  // get parameters
  size_t id = ( size_t )syscall_get_parameter( context, 0 );
  uintptr_t start = ( uintptr_t )syscall_get_parameter( context, 1 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_memory_shared_attach( %d, %#"PRIxPTR" )\r\n",
      id, start )
  #endif
  uintptr_t addr = shared_memory_attach(
    task_thread_current_thread->process, id, start );
  // handle error
  if ( 0 == addr ) {
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  // attach
  syscall_populate_success( context, ( size_t )addr );
}

/**
 * @brief release shared memory
 *
 * @param context
 */
void syscall_memory_shared_detach( void* context ) {
  // get parameters
  size_t id = ( size_t )syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_memory_shared_detach( %d )\r\n", id )
  #endif
  // try to detach
  if ( ! shared_memory_detach( task_thread_current_thread->process, id ) ) {
    syscall_populate_error( context, ( size_t )-EIO );
    return;
  }
}
