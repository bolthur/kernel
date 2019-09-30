
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#include <assert.h>
#include <kernel/debug.h>
#include <kernel/mm/placement.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/virt.h>
#include <kernel/mm/heap.h>
#include <kernel/entry.h>

/**
 * @brief placement address starting at kernel end
 */
uintptr_t placement_address = VIRT_2_PHYS( &__kernel_end );

/**
 * @brief Placement allocator
 *
 * @param alignment alignment
 * @param size amount of memory to align
 * @return uintptr_t found address
 *
 * @todo ensure that initrd won't be overwritten acidentally
 */
uintptr_t placement_alloc( size_t alignment, size_t size ) {
  // assert alignment
  assert( 0 < alignment );

  // assert no virtual and heap
  assert( true != virt_initialized_get() );
  assert( true != heap_initialized_get() );

  // determine offset for alignment
  size_t offset = 0;

  // states of memory managers
  bool phys = phys_initialized_get();

  // build return address
  uintptr_t address = placement_address;

  // debug output
  #if defined( PRINT_MM_PLACEMENT )
    DEBUG_OUTPUT( "content of placement address: 0x%lx\r\n", placement_address );
    DEBUG_OUTPUT( "wanted size: %zu\r\n", size );
    DEBUG_OUTPUT( "set address: 0x%lx\r\n", address );
  #endif

  // handle alignment
  if ( placement_address % alignment ) {
    // increase offset
    offset += ( alignment - placement_address % alignment );

    // increase address
    address += offset;

    // debug output
    #if defined( PRINT_MM_PLACEMENT )
      DEBUG_OUTPUT( "alignment offset: 0x%zu\r\n", offset );
      DEBUG_OUTPUT( "set address: 0x%lx\r\n", address );
    #endif
  }

  // add size to offset
  offset += size;

  // mark as used at physical memory manager if initialized
  if ( phys ) {
    phys_use_page_range( placement_address, offset );
  }

  // move up placement address
  placement_address += offset;

  // debug output
  #if defined( PRINT_MM_PLACEMENT )
    DEBUG_OUTPUT( "content of placement address: 0x%lx\r\n", placement_address );
    DEBUG_OUTPUT( "return address: 0x%lx\r\n", address );
  #endif

  // finally return address
  return address;
}
