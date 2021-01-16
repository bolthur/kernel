
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
#define MEMORY_ACQUIRE_MAP_FIXED 0x3
#define MEMORY_ACQUIRE_MAP_ANONYMOUS 0x4

/**
 * @brief Acquire memory
 *
 * @param context
 *
 * @todo add map shared handling
 * @todo add map fixed handling
 * @todo handle map private correctly
 */
void syscall_memory_acquire( void* context ) {
  // get parameter
  void* addr = ( void* )syscall_get_parameter( context, 0 );
  size_t len = ( size_t )syscall_get_parameter( context, 1 );
  int protection = ( int )syscall_get_parameter( context, 2 );
  int flags = ( int )syscall_get_parameter( context, 3 );
  // get context
  virt_context_ptr_t virtual_context = task_thread_current_thread
    ->process
    ->virtual_context;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall memory acquire( %#p, %zu, %d, %d )\r\n",
      addr, len, protection, flags )
  #endif

  if (
    // handle invalid length
    0 == len
    // handle missing private or shared
    || (
      ! ( flags & MEMORY_ACQUIRE_MAP_SHARED )
      && ! ( flags & MEMORY_ACQUIRE_MAP_PRIVATE )
      && ! ( flags & MEMORY_ACQUIRE_MAP_ANONYMOUS )
    )
  ) {
    syscall_populate_single_return( context, ( uintptr_t )-EINVAL );
    return;
  }
  // get full page count
  ROUND_UP_TO_FULL_PAGE( len )
  // find free page range
  uintptr_t start = virt_find_free_page_range(
    virtual_context,
    len,
    ( uintptr_t )addr
  );
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

  // map address range random
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
  // context
  virt_context_ptr_t virtual_context = task_thread_current_thread
    ->process
    ->virtual_context;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall memory_release ( %#"PRIxPTR", %zx )\r\n", address, len )
  #endif
  // handle invalid stuff
  if ( 0 == len ) {
    syscall_populate_single_return( context, ( uintptr_t )-EINVAL );
    return;
  }

  // FIXME: CHECK FOR MULTIPLE OF PAGE SIZE
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
  // check if range is mapped in context
  if ( ! virt_is_mapped_in_context_range( virtual_context, address, len ) ) {
    syscall_populate_single_return( context, 0 );
    return;
  }
  // try to unmap
  if ( ! virt_unmap_address_range( virtual_context, address, len, true ) ) {
    syscall_populate_single_return( context, ( uintptr_t )-EINVAL );
    return;
  }
  // return success
  syscall_populate_single_return( context, 0 );
}

/**
 * @brief Acquire shared memory
 *
 * @param context
 *
 * @todo add logic for shared memory again
 */
void syscall_memory_shared_acquire( __unused void* context ) {
  /*
-  // get parameter
-  __maybe_unused const char* name = ( const char* )syscall_get_parameter( context, 0 );
-  __maybe_unused int flag = ( int )syscall_get_parameter( context, 1 );
-  __maybe_unused mode_t mode = ( mode_t )syscall_get_parameter( context, 2 );
-
-  // debug output
-  #if defined( PRINT_SYSCALL )
-    DEBUG_OUTPUT(
-      "acquire shared memory with name \"%s\", flag %d and mode %#"PRIxPTR"\r\n",
-      name, flag, ( uintptr_t )mode )
-  #endif
-
-  // create shared memory and populate return
-  if ( ! shared_memory_create( name, size ) ) {
-    syscall_populate_single_return( context, false );
-  }
-  // attach to current thread
-  syscall_populate_single_return(
-    context,
-    shared_memory_acquire(
-      task_thread_current_thread->process,
-      name
-    )
-  );*/
}

/**
 * @brief Release shared memory
 *
 * @param context
 *
 * @todo add logic for shared memory again
 */
void syscall_memory_shared_release( __unused void* context ) {
  /*
-  // get parameter
-  const char* name = ( const char* )syscall_get_parameter( context, 0 );
-
-  // debug output
-  #if defined( PRINT_SYSCALL )
-    DEBUG_OUTPUT( "release shared memory with name \"%s\"\r\n", name )
-  #endif
-
-  // release and populate return
-  syscall_populate_single_return(
-    context, shared_memory_release(
-      task_thread_current_thread->process,
-      name
-    )
-  );
   */
}
