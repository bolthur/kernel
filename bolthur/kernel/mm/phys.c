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

#include <stddef.h>
#include <stdbool.h>

#include <assert.h>
#if defined( PRINT_MM_PHYS )
  #include <debug/debug.h>
#endif
#include <entry.h>
#include <initrd.h>
#include <mm/phys.h>

/**
 * @brief Physical bitmap
 */
uint32_t *phys_bitmap;

/**
 * @brief physical bitmap length set by platform
 */
uint32_t phys_bitmap_length;

/**
 * @brief static initialized flag
 */
static bool phys_initialized = false;

/**
 * @brief Mark physical page as used on arm
 *
 * @param address address to mark as free
 */
void phys_mark_page_used( uint64_t address ) {
  // get frame, index and offset
  uint64_t frame = address / PAGE_SIZE;
  uint64_t index = PAGE_INDEX( frame );
  uint64_t offset = PAGE_OFFSET( frame );

  // mark page as used
  phys_bitmap[ index ] |= ( 1U << offset );

  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT(
      "frame: %06llu, index: %04llu, offset: %02llu, address: %#016llx, phys_bitmap[ %04llu ]: %#08x\r\n",
      frame, index, offset, address, index, phys_bitmap[ index ]
    );
  #endif
}

/**
 * @brief Mark physical page as free on arm
 *
 * @param address address to mark as free
 */
void phys_mark_page_free( uint64_t address ) {
  // get frame, index and offset
  uint64_t frame = address / PAGE_SIZE;
  uint64_t index = PAGE_INDEX( frame );
  uint64_t offset = PAGE_OFFSET( frame );

  // mark page as free
  phys_bitmap[ index ] &= ( uint32_t )( ~( 1U << offset ) );

  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT(
      "frame: %06llu, index: %04llu, offset: %02llu, address: %#016llx, phys_bitmap[ %04llu ]: %#08x\r\n",
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
void phys_free_page_range( uint64_t address, size_t amount ) {
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "address: %#016llx, amount: %zu\r\n", address, amount );
  #endif

  // loop until amount and mark as free
  for (
    size_t idx = 0;
    idx < amount / PAGE_SIZE;
    idx++, address += PAGE_SIZE
  ) {
    phys_mark_page_free( address );
  }
}

/**
 * @brief Method to free phys page range
 *
 * @param address start address
 * @param amount amount of memory
 */
void phys_use_page_range( uint64_t address, size_t amount ) {
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "address: %#016llu, amount: %zu\r\n", address, amount );
  #endif

  // round down address to page start
  address = ROUND_DOWN_TO_FULL_PAGE( address );
  // round up amount if necessary
  amount = ROUND_UP_TO_FULL_PAGE( amount );

  // loop until amount and mark as free
  for (
    size_t idx = 0;
    idx < amount / PAGE_SIZE;
    idx++, address += PAGE_SIZE
  ) {
    phys_mark_page_used( address );
  }
}

/**
 * @brief Method to find free page range
 *
 * @param alignment wanted memory alignment
 * @param memory_amount amount of memory to find free page range for
 * @return uint64_t address of found memory
 */
uint64_t phys_find_free_page_range( size_t alignment, size_t memory_amount ) {
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT(
      "memory_amount: %zu, alignment: %#016zx\r\n",
      memory_amount, alignment
    );
  #endif

  // round up to full page
  memory_amount = ROUND_UP_TO_FULL_PAGE( memory_amount );

  // determine amount of pages
  size_t page_amount = memory_amount / PAGE_SIZE;
  size_t found_amount = 0;

  // found address range
  uint64_t address = 0;
  uint64_t tmp;
  bool stop = false;

  // loop through bitmap to find free continuous space
  for ( size_t idx = 0; idx < phys_bitmap_length && !stop; idx++ ) {
    // skip completely used entries
    if ( PHYS_ALL_PAGES_OF_INDEX_USED == phys_bitmap[ idx ] ) {
      continue;
    }

    // loop through bits per entry
    for ( size_t offset = 0; offset < PAGE_PER_ENTRY && !stop; offset++ ) {
      // not free? => reset counter and continue
      if ( phys_bitmap[ idx ] & ( uint32_t )( 1U << offset ) ) {
        found_amount = 0;
        address = 0;
        continue;
      }

      // set address if found is 0
      if ( 0 == found_amount ) {
        address = idx * PAGE_SIZE * PAGE_PER_ENTRY + offset * PAGE_SIZE;
        #if defined( PRINT_MM_PHYS )
          DEBUG_OUTPUT( "idx = %zu\r\n", idx );
        #endif

        // check for alignment
        if ( 0 < alignment && 0 != address % alignment ) {
          found_amount = 0;
          address = 0;
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

  // handle no address
  if ( 0 == address ) {
    return address;
  }

  // set temporary address
  tmp = address;

  // loop until amount and mark as used
  for ( size_t idx = 0; idx < found_amount; idx++, tmp += PAGE_SIZE ) {
    phys_mark_page_used( tmp );
  }

  // return found / not found address
  return address;
}

/**
 * @brief Shorthand to find single free page
 *
 * @param alignment
 * @return uint64_t
 */
uint64_t phys_find_free_page( size_t alignment ) {
  return phys_find_free_page_range( alignment, PAGE_SIZE );
}

/**
 * @brief Shorthand for free one single page
 *
 * @param address address to free
 */
void phys_free_page( uint64_t address ) {
  phys_free_page_range( address, PAGE_SIZE );
}

/**
 * @brief Generic initialization of physical memory manager
 */
void phys_init( void ) {
  // execute platform initialization
  assert( phys_platform_init() )

  // determine start and end for kernel mapping
  uintptr_t start = 0;
  uintptr_t end = ROUND_UP_TO_FULL_PAGE( VIRT_2_PHYS( &__kernel_end ) );

  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "start: %p\r\n", ( void* )start );
    DEBUG_OUTPUT( "end: %p\r\n", ( void* )end );
  #endif

  // map from start to end addresses as used
  while ( start < end ) {
    // mark used
    phys_mark_page_used( start );

    // get next page
    start += PAGE_SIZE;
  }

  // consider possible initrd
  if ( initrd_exist() ) {
    // set start and end from initrd
    start = initrd_get_start_address();
    end = initrd_get_end_address();

    // debug output
    #if defined( PRINT_MM_PHYS )
      DEBUG_OUTPUT( "start: %p\r\n", ( void* )start );
      DEBUG_OUTPUT( "end: %p\r\n", ( void* )end );
    #endif

    // map from start to end addresses as used
    while ( start < end ) {
      // mark used
      phys_mark_page_used( start );

      // get next page
      start += PAGE_SIZE;
    }
  }

  // mark initialized
  phys_initialized = true;
}

/**
 * @brief Get initialized flag
 *
 * @return true physical memory management has been set up
 * @return false physical memory management has been not yet set up
 */
bool phys_init_get( void ) {
  return phys_initialized;
}
