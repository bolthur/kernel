
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
#include <stdbool.h>

#include "lib/stdc/stdio.h"
#include "kernel/kernel/mm/phys.h"
#include "kernel/arch/arm/mm/phys.h"

#define PAGE_PER_ENTRY ( sizeof( phys_bitmap_length ) * 8 )
#define PAGE_INDEX( address ) ( address / PAGE_PER_ENTRY )
#define PAGE_OFFSET( address ) ( address % PAGE_PER_ENTRY )

/**
 * @brief Mark physical page as used on arm
 *
 * @param address address to mark as free
 */
void phys_mark_page_used( void* address ) {
  // get frame, index and offset
  size_t frame = ( uintptr_t )address / PHYS_PAGE_SIZE;
  size_t index = PAGE_INDEX( frame );
  size_t offset = PAGE_OFFSET( frame );

  // mark page as used
  phys_bitmap[ index ] |= ( uintptr_t )( 0x1 << offset );

  // debug output
  #if defined( PRINT_MM_PHYS )
    printf(
      "[ phys mark used ]: frame: %06i, index: %04i, offset: %02i, address: 0x%08x, phys_bitmap[ %04d ]: 0x%08x\r\n",
      frame, index, offset, address, index, phys_bitmap[ index ]
    );
  #endif
}

/**
 * @brief Mark physical page as free on arm
 *
 * @param address address to mark as free
 */
void phys_mark_page_free( void*  address ) {
  // get frame, index and offset
  size_t frame = ( uintptr_t )address / PHYS_PAGE_SIZE;
  size_t index = PAGE_INDEX( frame );
  size_t offset = PAGE_OFFSET( frame );

  // mark page as free
  phys_bitmap[ index ] &= ( uintptr_t )( ~( 0x1 << offset ) );

  // debug output
  #if defined( PRINT_MM_PHYS )
    printf(
      "[ phys mark used ]: frame: %06i, index: %04i, offset: %02i, address: 0x%08x, phys_bitmap[ %04d ]: 0x%08x\r\n",
      frame, index, offset, address, index, phys_bitmap[ index ]
    );
  #endif
}

/**
 * @brief Method to free phys page range
 *
 * @param address start address
 * @param amount amount of memory
 */
void phys_free_range( void* address, size_t amount ) {
  // loop until amount and mark as free
  for (
    size_t idx = 0;
    idx < amount / PHYS_PAGE_SIZE;
    idx++, address = ( void* )( ( uintptr_t ) address + PHYS_PAGE_SIZE )
  ) {
    phys_mark_page_free( address );
  }
}

/**
 * @brief Method to find free page range
 *
 * @param memory_amount amount of memory to find free page range for
 * @return uintptr_t address of found memory
 */
void* phys_find_free_range( size_t memory_amount, size_t alignment ) {
  // round up to full page
  memory_amount += memory_amount % PHYS_PAGE_SIZE;
  size_t page_amount = memory_amount / PHYS_PAGE_SIZE;
  size_t found_amount = 0;

  // found address range
  void* address = NULL;
  bool stop = false;

  // loop through bitmap to find free continouse space
  for ( size_t idx = 0; idx < phys_bitmap_length && !stop; idx++ ) {
    // loop through bits per entry
    for ( size_t offset = 0; offset < PAGE_PER_ENTRY && !stop; offset++ ) {
      // not free? => reset counter and continue
      if ( phys_bitmap[ idx ] & ( uintptr_t )( 0x1 << offset ) ) {
        found_amount = 0;
        address = NULL;
        continue;
      }

      // set address if found is 0
      if ( 0 == found_amount ) {
        address = ( void* )(
          idx * PHYS_PAGE_SIZE * PAGE_PER_ENTRY + offset * PHYS_PAGE_SIZE
        );

        // check for alignment
        if (
          0 < alignment
          && 0 != ( uintptr_t )address % alignment
        ) {
          found_amount = 0;
          address = NULL;
          continue;
        }
      }

      // increase found amount
      found_amount += 1;

      // reached necessary amount? => stop loop
      if ( found_amount == page_amount ) {
        stop = true;
      }
    }
  }

  // found something => mark as used
  if ( NULL != address ) {
    // temporary address variable
    void *tmp = address;

    // loop until amount and mark as used
    for (
      size_t idx = 0;
      idx < found_amount;
      idx++, tmp = ( void* )( ( uintptr_t ) tmp + PHYS_PAGE_SIZE )
    ) {
      phys_mark_page_used( tmp );
    }
  }

  // return found / not found address
  return address;
}
