
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

#include <stddef.h>
#include <stdio.h>

#include "kernel/mm/phys.h"
#include "arch/arm/mm/phys.h"

#define PAGE_PER_ENTRY ( sizeof( phys_bitmap_length ) * 8 )
#define PAGE_INDEX( address ) ( address / PAGE_PER_ENTRY )
#define PAGE_OFFSET( address ) ( address % PAGE_PER_ENTRY )

/**
 * @brief Mark physical page as used on arm
 *
 * @param address address to mark as free
 */
void phys_mark_page_used( uintptr_t address ) {
  // get frame, index and offset
  size_t frame = address / PHYS_PAGE_SIZE;
  size_t index = PAGE_INDEX( frame );
  size_t offset = PAGE_OFFSET( frame );

  // mark page as used
  phys_bitmap[ index ] |= ( uintptr_t )( 0x1 << offset );

  // debug output
  #if defined( DEBUG )
    printf(
      "[ phys mark used ]: frame: %i, index: %i, offset: %i, address: 0x%08x, phys_bitmap[ %d ]: 0x%08x\r\n",
      frame, index, offset, address, index, phys_bitmap[ index ]
    );
  #endif
}

/**
 * @brief Mark physical page as free on arm
 *
 * @param address address to mark as free
 */
void phys_mark_page_free( uintptr_t address ) {
  // get frame, index and offset
  size_t frame = address / PHYS_PAGE_SIZE;
  size_t index = PAGE_INDEX( frame );
  size_t offset = PAGE_OFFSET( frame );

  // mark page as used
  phys_bitmap[ index ] &= ( uintptr_t )( ~( 0x1 << offset ) );

  // debug output
  #if defined( DEBUG )
    printf(
      "[ phys mark used ]: frame: %i, index: %i, offset: %i, address: 0x%08x, phys_bitmap[ %d ]: 0x%08x\r\n",
      frame, index, offset, address, index, phys_bitmap[ index ]
    );
  #endif
}

