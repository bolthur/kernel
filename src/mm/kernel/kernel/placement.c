
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
#include <mm/kernel/kernel/placement.h>
#include <mm/kernel/kernel/phys.h>
#include <mm/kernel/kernel/virt.h>
#include <mm/kernel/kernel/heap.h>
#include <kernel/entry.h>
#include <kernel/type.h>

/**
 * @brief placement address starting at kernel end
 */
paddr_t placement_address = VIRT_2_PHYS( &__kernel_end );

/**
 * @brief Placement allocator
 *
 * @param size amount of memory to align
 * @param alignment alignment
 * @return vaddr_t found address
 */
vaddr_t placement_alloc( size_t size, size_t alignment ) {
  // assert no heap and no virtual memory
  assert( ! heap_initialized_get() );
  assert( ! virt_initialized_get() );

  // assert alignment
  assert( 0 < alignment );

  // determine offset for alignment
  size_t offset = 0;

  // states of memory managers
  bool phys = phys_initialized_get();

  // build return address
  vaddr_t address = ( vaddr_t )placement_address;

  // debug output
  #if defined( PRINT_MM_PLACEMENT )
    DEBUG_OUTPUT( "content of placement address: 0x%08x\r\n", placement_address );
    DEBUG_OUTPUT( "wanted size: 0x%08x\r\n", size );
    DEBUG_OUTPUT( "set address: 0x%08x\r\n", address );
  #endif

  // handle alignment
  if ( ( paddr_t )placement_address % alignment ) {
    // increase offset
    offset += ( alignment - ( paddr_t )placement_address % alignment );

    // increase address
    address = ( vaddr_t )( ( paddr_t )address + offset );

    // debug output
    #if defined( PRINT_MM_PLACEMENT )
      DEBUG_OUTPUT( "alignment offset: 0x%08x\r\n", offset );
      DEBUG_OUTPUT( "set address: 0x%08x\r\n", address );
    #endif
  }

  // add size to offset
  offset += size;

  // mark as used at physical memory manager if initialized
  if ( phys ) {
    phys_use_page_range( ( vaddr_t )placement_address, offset );
  }

  // move up placement address
  placement_address += offset;

  // debug output
  #if defined( PRINT_MM_PLACEMENT )
    DEBUG_OUTPUT( "content of placement address: 0x%08x\r\n", placement_address );
    DEBUG_OUTPUT( "return address: 0x%08x\r\n", address );
  #endif

  // finally return address
  return address;
}
