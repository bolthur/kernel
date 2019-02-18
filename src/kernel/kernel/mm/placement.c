
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include <stdint.h>
#include <stddef.h>

#include "lib/stdc/stdio.h"
#include "kernel/kernel/panic.h"
#include "kernel/kernel/mm/placement.h"
#include "kernel/kernel/mm/phys.h"
#include "kernel/kernel/mm/virt.h"
#include "kernel/kernel/mm/heap.h"
#include "kernel/kernel/entry.h"

/**
 * @brief placement address starting at kernel end
 */
uintptr_t placement_address = ( uintptr_t )&__kernel_end;

/**
 * @brief Placement allocator
 *
 * @param size amount of memory to align
 * @param alignment alignment
 * @return void* found address
 */
void* placement_alloc( size_t size, size_t alignment ) {
  // check for heap not initialized
  if ( heap_initialized_get() || virt_initialized_get() ) {
    PANIC( "placement_alloc used with initialized heap or virtual manager!" );
  }

  // determine offset for alignment
  size_t offset = 0;

  // states of memory managers
  bool phys = phys_initialized_get();

  // build return address
  void* address = ( void* )placement_address;

  // debug output
  #if defined( PRINT_MM_PLACEMENT )
    printf( "[ %s ]: content of placement address: 0x%08x\r\n", __func__, placement_address );
    printf( "[ %s ]: wanted size: 0x%08x\r\n", __func__, size );
    printf( "[ %s ]: set address: 0x%08x\r\n", __func__, address );
  #endif

  // handle alignment
  if ( PLACEMENT_NO_ALIGN != alignment ) {
    // increase offset
    offset += ( alignment - ( uintptr_t )placement_address % alignment );

    // increase address
    address = ( void* )( ( uintptr_t )address + offset );

    // debug output
    #if defined( PRINT_MM_PLACEMENT )
      printf( "[ %s ]: alignment offset: 0x%08x\r\n", __func__, offset );
      printf( "[ %s ]: set address: 0x%08x\r\n", __func__, address );
    #endif
  }

  // add size to offset
  offset += size;

  // mark as used at physical memory manager if initialized
  if ( phys ) {
    phys_use_page_range( ( void* )VIRT_2_PHYS( placement_address ), offset );
  }

  // move up placement address
  placement_address += offset;

  // debug output
  #if defined( PRINT_MM_PLACEMENT )
    printf( "[ %s ]: content of placement address: 0x%08x\r\n", __func__, placement_address );
    printf( "[ %s ]: return address: 0x%08x\r\n", __func__, address );
  #endif

  // finally return address
  return address;
}
