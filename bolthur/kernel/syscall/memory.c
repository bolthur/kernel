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
#include <syscall.h>
#if defined( PRINT_SYSCALL )
  #include <debug/debug.h>
#endif
#include <mm/phys.h>
#include <mm/virt.h>
#include <mm/shared.h>
#include <task/process.h>
#include <task/thread.h>

#define MEMORY_ACQUIRE_PROTECTION_NONE 0x0
#define MEMORY_ACQUIRE_PROTECTION_READ 0x1
#define MEMORY_ACQUIRE_PROTECTION_WRITE 0x2
#define MEMORY_ACQUIRE_PROTECTION_EXECUTABLE 0x4

#define MEMORY_FLAG_NONE 0x0
#define MEMORY_FLAG_PHYS 0x1
#define MEMORY_FLAG_DEVICE 0x2

/**
 * @fn void syscall_memory_acquire(void*)
 * @brief Acquire memory
 *
 * @param context
 */
void syscall_memory_acquire( void* context ) {
  // get parameter
  void* addr = ( void* )syscall_get_parameter( context, 0 );
  size_t len = ( size_t )syscall_get_parameter( context, 1 );
  int protection = ( int )syscall_get_parameter( context, 2 );
  int flag = ( int )syscall_get_parameter( context, 3 );
  // get context
  virt_context_ptr_t virtual_context = task_thread_current_thread
    ->process
    ->virtual_context;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall memory acquire( %#p, %#x, %d, %d )\r\n",
      addr, len, protection, flag )
  #endif

  // handle invalid length
  if ( 0 == len ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid length specified\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }

  // get full page count
  len = ROUND_UP_TO_FULL_PAGE( len );
  // variable for physical mapping
  uint64_t phys = 0;
  // check if physical range is already in use for map physical
  if ( flag & MEMORY_FLAG_PHYS ) {
    // set phys to given address
    phys = ( uintptr_t )addr;
    // overwrite address with NULL
    uint64_t offset = phys - ROUND_DOWN_TO_FULL_PAGE( addr );
    phys -= offset;
    // overwrite address with NULL
    addr = NULL;
    // check if already used
    if ( phys_is_range_used( phys, len ) ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "Physical address already in use by some other\r\n" )
      #endif
      syscall_populate_error( context, ( size_t )-EADDRINUSE );
      return;
    }
  }
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
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "Invalid address received!\r\n" )
      #endif
      return;
    }
  // find free page range starting after thread entry point
  } else {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "entry = %#x, address = %p\r\n",
        task_thread_current_thread->entry, addr )
    #endif
    // set address
    uintptr_t tmp_addr = ROUND_UP_TO_FULL_PAGE(
      task_thread_current_thread->entry
    );
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "entry = %#x, address = %p\r\n", tmp_addr, addr )
    #endif
    start = virt_find_free_page_range( virtual_context, len, tmp_addr );
  }

  // handle no address found
  if ( ( uintptr_t )NULL == start ) {
    syscall_populate_error( context, ( size_t )-ENOMEM );
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No start address found!\r\n" )
    #endif
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

  virt_memory_type_t map_type = VIRT_MEMORY_TYPE_NORMAL;
  if ( flag & MEMORY_FLAG_DEVICE ) {
    map_type = VIRT_MEMORY_TYPE_DEVICE;
  }
  // handle physical memory allocation
  if ( flag & MEMORY_FLAG_PHYS ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "mapping %#016llx to address %p with type %d, flag %d and len %x\r\n",
        phys, start,  map_type, map_flag, len
      )
    #endif
    if ( ! virt_map_address_range(
      virtual_context,
      start,
      phys,
      len,
      map_type,
      map_flag
    ) ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "Error during map of address!\r\n" )
      #endif
      syscall_populate_error( context, ( size_t )-EIO );
      return;
    }
  // map address range with random physical memory
  } else {
    // map address range with random physical memory
    if ( ! virt_map_address_range_random(
      virtual_context,
      start,
      len,
      map_type,
      map_flag
    ) ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "Error during map of address!\r\n" )
      #endif
      syscall_populate_error( context, ( size_t )-EIO );
      return;
    }
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
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "invalid length passed for address %#"PRIxPTR"\r\n",
        address
      )
    #endif
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // get at least full page size
  len = ROUND_UP_TO_FULL_PAGE( len );
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
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid address!\r\n" )
    #endif
    return;
  }

  // check for shared area
  if ( shared_memory_address_is_shared(
    task_thread_current_thread->process,
    address,
    len
  ) ) {
    syscall_populate_error( context, ( size_t )-EADDRNOTAVAIL );
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Address is shared and handled differently!\r\n" )
    #endif
    return;
  }

  // check if range is mapped in context
  if ( ! virt_is_mapped_in_context_range( virtual_context, address, len ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Not mapped in context range!\r\n" )
    #endif
    return;
  }
  // try to unmap
  if ( ! virt_unmap_address_range( virtual_context, address, len, unmap_phys ) ) {
    syscall_populate_error( context, ( size_t )-EIO );
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Error during unmap!\r\n" )
    #endif
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Success!\r\n" )
  #endif
  // return success
  syscall_populate_success( context, 0 );
}

/**
 * @brief Acquire new or existing shared memory area
 *
 * @param context
 */
void syscall_memory_shared_create( void* context ) {
  // get parameter rounded to full page
  size_t len = ROUND_UP_TO_FULL_PAGE(
    ( size_t )syscall_get_parameter( context, 0 )
  );
  if ( 0 == len ) {
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
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
    task_thread_current_thread->process,
    task_thread_current_thread,
    id,
    start
  );
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
  // return success
  syscall_populate_success( context, 0 );
}

/**
 * @fn void syscall_memory_translate_physical(void*)
 * @brief Translate virtual into physical address
 *
 * @param context
 */
void syscall_memory_translate_physical( void* context ) {
  // get parameters
  uintptr_t address = ( uintptr_t )syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_memory_translate_physical( %#"PRIxPTR" )\r\n", address )
  #endif
  // get context
  virt_context_ptr_t virtual_context = task_thread_current_thread
    ->process
    ->virtual_context;
  // get min and max address of context
  uintptr_t min = virt_get_context_min_address( virtual_context );
  uintptr_t max = virt_get_context_max_address( virtual_context );
  // ensure that address is in context
  if (
    min > address
    || max <= address
    || ! virt_is_mapped_in_context( virtual_context, address )
  ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid address received!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // get mapped address
  uint64_t phys = virt_get_mapped_address_in_context( virtual_context, address );
  // populate success
  syscall_populate_success( context, ( uintptr_t )phys  );
}
