
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#include <core/mm/phys.h>
#include <core/mm/virt.h>
#include <core/task/process.h>
#include <core/syscall.h>
#include <core/interrupt.h>
#if defined( PRINT_SYSCALL )
  #include <core/debug/debug.h>
#endif

// libc header for defines
#include <inttypes.h>
#include <sys/mman.h>
#include <errno.h>

/**
 * @brief Acquire memory
 *
 * @param context
 *
 * @todo Add support for file descriptor
 * @todo add support for lock
 */
void syscall_posix_mman_mmap( void* context ) {
  // get parameter
  void* addr = ( void* )syscall_get_parameter( context, 0 );
  size_t len = ( size_t )syscall_get_parameter( context, 1 );
  __maybe_unused int prot = ( int )syscall_get_parameter( context, 2 );
  int flags = ( int )syscall_get_parameter( context, 3 );
  int filedes = ( int )syscall_get_parameter( context, 4 );
  __maybe_unused off_t off = ( off_t )syscall_get_parameter( context, 5 );
  // get context
  virt_context_ptr_t virtual_context = task_thread_current_thread
    ->process
    ->virtual_context;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall mmap( %#p, %zu, %d, %d, %d, %lx )\r\n",
      addr, len, prot, flags, filedes, off )
  #endif
  // handle not supported file descriptor
  if ( -1 != filedes || ! ( flags & MAP_ANONYMOUS ) ) {
    syscall_populate_single_return( context, ( uintptr_t )-EINVAL );
    return;
  }

  if (
    // handle invalid length
    0 == len
    // handle missing private or shared
    || (
      ! ( flags & MAP_SHARED )
      && ! ( flags & MAP_PRIVATE )
      && ! ( flags & MAP_ANONYMOUS )
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
  if ( flags & PROT_READ ) {
    map_flag |= VIRT_PAGE_TYPE_READ;
  }
  if ( flags & PROT_WRITE ) {
    map_flag |= VIRT_PAGE_TYPE_WRITE;
  }
  if ( flags & PROT_EXEC ) {
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
 *
 * @todo check for length to be multiple of page size
 * @todo handle memory locks
 * @todo add support for typed memory
 */
void syscall_posix_mman_munmap( void* context ) {
  // get parameters
  uintptr_t address = ( uintptr_t )syscall_get_parameter( context, 0 );
  size_t len = ( size_t )syscall_get_parameter( context, 1 );
  // context
  virt_context_ptr_t virtual_context = task_thread_current_thread
      ->process
      ->virtual_context;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall munmap ( %#"PRIxPTR", %zx )\r\n", address, len )
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
