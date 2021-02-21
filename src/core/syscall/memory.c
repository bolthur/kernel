
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

#include <core/syscall.h>
#if defined( PRINT_SYSCALL )
  #include <core/debug/debug.h>
#endif
#include <core/mm/phys.h>
#include <core/mm/virt.h>
#include <core/task/process.h>
#include <core/task/thread.h>

#include <inttypes.h>
#include <errno.h>

#define MEMORY_ACQUIRE_PROTECTION_NONE 0x0
#define MEMORY_ACQUIRE_PROTECTION_READ 0x1
#define MEMORY_ACQUIRE_PROTECTION_WRITE 0x2
#define MEMORY_ACQUIRE_PROTECTION_EXECUTABLE 0x4
#define MEMORY_ACQUIRE_MAP_SHARED 0x1
#define MEMORY_ACQUIRE_MAP_PRIVATE 0x2
#define MEMORY_ACQUIRE_MAP_FIXED 0x4
#define MEMORY_ACQUIRE_MAP_ANONYMOUS 0x8

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
  int flags = ( int )syscall_get_parameter( context, 3 );
  const char* name = ( const char* )syscall_get_parameter( context, 4 );
  // get context
  virt_context_ptr_t virtual_context = task_thread_current_thread
    ->process
    ->virtual_context;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall memory acquire( %#p, %zu, %d, %d, %s )\r\n",
      addr, len, protection, flags, name )
  #endif

  // parameter checks
  if (
    // handle invalid length
    0 == len
    // handle missing private or shared
    || (
      ! ( flags & MEMORY_ACQUIRE_MAP_SHARED )
      && ! ( flags & MEMORY_ACQUIRE_MAP_PRIVATE )
      && ! ( flags & MEMORY_ACQUIRE_MAP_ANONYMOUS )
    )
    // invalid fixed usage handling
    || (
      ( flags & MEMORY_ACQUIRE_MAP_FIXED )
      && 0 == addr
    )
    // invalid shared usage
    || (
      ( flags & MEMORY_ACQUIRE_MAP_SHARED )
      && NULL == name
    )
  ) {
    syscall_populate_single_return( context, ( uintptr_t )-EINVAL );
    return;
  }

  // get full page count
  ROUND_UP_TO_FULL_PAGE( len )
  // determine start
  uintptr_t start;
  // fixed handling means take address as start
  if ( flags & MEMORY_ACQUIRE_MAP_FIXED ) {
    start = ( uintptr_t )addr;
    // get min and max address of context
    uintptr_t min = virt_get_context_min_address( virtual_context );
    uintptr_t max = virt_get_context_max_address( virtual_context );
    // ensure that address is in context
    if ( min > start || max <= start || max <= start + len ) {
      syscall_populate_single_return( context, ( uintptr_t )-ENOMEM );
      return;
    }
  // find free page range
  } else {
    start = virt_find_free_page_range( virtual_context, len, ( uintptr_t )addr );
  }

  // handle no address
  if ( ( uintptr_t )NULL == start ) {
    syscall_populate_single_return( context, ( uintptr_t )-ENOMEM );
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

  // handle shared memory
  if ( flags & MEMORY_ACQUIRE_MAP_SHARED ) {
    // acquire shared memory
    uintptr_t shared_start = shared_memory_acquire(
      task_thread_current_thread->process, name, start, map_flag );
    // handle error
    if ( ( uintptr_t )NULL == shared_start ) {
      syscall_populate_single_return( context, ( uintptr_t )-ENOMEM );
      return;
    }
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "shared start = %#"PRIxPTR"\r\n", start )
    #endif
    // return
    syscall_populate_single_return( context, shared_start );
    return;
  }

  // map address range with random physical memory
  if ( ! virt_map_address_range_random(
    virtual_context,
    start,
    len,
    VIRT_MEMORY_TYPE_NORMAL,
    map_flag
  ) ) {
    syscall_populate_single_return( context, ( uintptr_t )-ENOMEM );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "start = %#"PRIxPTR"\r\n", start )
  #endif
  // return to process with address
  syscall_populate_single_return( context, start );
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
      "syscall memory_release ( %#"PRIxPTR", %zx )\r\n",
      address, len )
  #endif
  // handle invalid stuff
  if ( 0 == len ) {
    syscall_populate_single_return( context, ( uintptr_t )-EINVAL );
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
    syscall_populate_single_return( context, ( uintptr_t )-EINVAL );
    return;
  }

  // handle possible shared mapping
  shared_memory_entry_mapped_ptr_t mapping = shared_memory_retrieve_by_address(
    task_thread_current_thread->process,
    address
  );
  // handle shared
  if ( NULL != mapping ) {
    shared_memory_entry_ptr_t area = shared_memory_retrieve_deleted(
      mapping->reference );
    // skip unmap of physical
    unmap_phys = false;
    // handle area in deletion
    if ( area ) {
      // cleanup completely
      if ( 1 >= area->use_count ) {
        // cleanup deleted entry
        if ( ! shared_memory_cleanup_deleted( area ) ) {
          syscall_populate_single_return( context, 0 );
          return;
        }
        // change flag again to true
        unmap_phys = true;
      // decrement use count only
      } else {
        area->use_count--;
      }
    }
  }

  // check if range is mapped in context
  if ( ! virt_is_mapped_in_context_range( virtual_context, address, len ) ) {
    syscall_populate_single_return( context, 0 );
    return;
  }
  // try to unmap
  if ( ! virt_unmap_address_range( virtual_context, address, len, unmap_phys ) ) {
    syscall_populate_single_return( context, ( uintptr_t )-EINVAL );
    return;
  }
  // return success
  syscall_populate_single_return( context, 0 );
}

/**
 * @brief Acquire new or existing shared memory area
 *
 * @param context
 */
void syscall_memory_shared_acquire( void* context ) {
  // get parameters
  const char* name = ( const char* )syscall_get_parameter( context, 0 );
  uint32_t access = ( uint32_t )syscall_get_parameter( context, 1 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_memory_shared_acquire( %s, %zx  )\r\n", name, access )
  #endif
  // create entry
  syscall_populate_single_return(
    context, shared_memory_create( name, access )
  );
}

/**
 * @brief release shared memory
 *
 * @param context
 */
void syscall_memory_shared_release( void* context ) {
  // get parameters
  const char* name = ( const char* )syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_memory_shared_release( %s )\r\n", name )
  #endif
  // check if existing
  shared_memory_entry_ptr_t entry = shared_memory_retrieve_by_name( name ) ;
  if ( ! entry ) {
    syscall_populate_single_return( context, ( uintptr_t )-ENOENT );
    return;
  }
  // release shared memory in process
  syscall_populate_single_return( context, shared_memory_release( name ) );
}

/**
 * @brief initialize shared memory if not yet initialized
 *
 * @param context
 */
void syscall_memory_shared_initialize( void* context ) {
  const char* name = ( const char* )syscall_get_parameter( context, 0 );
  size_t size = ( size_t )syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_memory_shared_release( %s, %zx )\r\n", name, size )
  #endif
  // initialize
  syscall_populate_single_return(
    context, shared_memory_initialize( name, size )
  );
}
