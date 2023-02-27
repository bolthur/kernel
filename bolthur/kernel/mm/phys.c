/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include "../lib/assert.h"
#include "../lib/inttypes.h"
#if defined( PRINT_MM_PHYS )
  #include "../debug/debug.h"
#endif
#include "../entry.h"
#include "../initrd.h"
#include "../mm/phys.h"

/**
 * @brief Physical bitmap
 */
uint32_t* phys_bitmap;

/**
 * @brief Physical bitmap duplicate used for checking
 */
uint32_t* phys_bitmap_check;

/**
 * @brief physical bitmap length set by platform
 */
uint32_t phys_bitmap_length;

/**
 * @brief Physical bitmap
 */
uint32_t* phys_dma_bitmap;

/**
 * @brief physical bitmap length set by platform
 */
uint32_t phys_dma_length;

/**
 * @brief physical dma start
 */
uint64_t phys_dma_start;

/**
 * @brief physical dma start
 */
uint64_t phys_dma_end;

/**
 * @brief static initialized flag
 */
static bool phys_initialized = false;

/**
 * @fn void phys_mark_page_used(uint64_t)
 * @brief Mark physical page as used
 *
 * @param address address to mark as free
 */
void phys_mark_page_used( uint64_t address ) {
  // get frame, index and offset
  uint64_t frame = address / PAGE_SIZE;
  uint64_t index = PAGE_INDEX( frame );
  uint64_t offset = PAGE_OFFSET( frame );

  // dma handling
  if ( address >= phys_dma_start && address < phys_dma_end ) {
    // update variables
    frame = ( address - phys_dma_start ) / PAGE_SIZE;
    index = PAGE_INDEX( frame );
    offset = PAGE_OFFSET( frame );
    // mark used
    phys_dma_bitmap[ index ] |= ( 1U << offset );
    // debug output
    #if defined( PRINT_MM_PHYS )
      DEBUG_OUTPUT(
        "frame: %06"PRIu64", index: %04"PRIu64", offset: %02"PRIu64", "
        "address: %#"PRIx64", bitmap[ %04"PRIu64" ]: %"PRIx32"\r\n",
        frame, index, offset, address, index, phys_dma_bitmap[ index ]
      )
    #endif
  // normal handling
  } else {
    phys_bitmap[ index ] |= ( 1U << offset );
    phys_bitmap_check[ index ] |= ( 1U << offset );
    // debug output
    #if defined( PRINT_MM_PHYS )
      DEBUG_OUTPUT(
        "frame: %06"PRIu64", index: %04"PRIu64", offset: %02"PRIu64", "
        "address: %#"PRIx64", phys_bitmap[ %04"PRIu64" ]: %"PRIx32"\r\n",
        frame, index, offset, address, index, phys_bitmap[ index ]
      )
    #endif
  }
}

/**
 * @fn void phys_mark_page_free(uint64_t)
 * @brief Mark physical page as free
 *
 * @param address address to mark as free
 */
void phys_mark_page_free( uint64_t address ) {
  // get frame, index and offset
  uint64_t frame = address / PAGE_SIZE;
  uint64_t index = PAGE_INDEX( frame );
  uint64_t offset = PAGE_OFFSET( frame );


  // dma handling
  if ( address >= phys_dma_start && address < phys_dma_end ) {
    // update variables
    frame = ( address - phys_dma_start ) / PAGE_SIZE;
    index = PAGE_INDEX( frame );
    offset = PAGE_OFFSET( frame );
    // mark used
    phys_dma_bitmap[ index ] &= ( uint32_t )( ~( 1U << offset ) );
    // debug output
    #if defined( PRINT_MM_PHYS )
      DEBUG_OUTPUT(
        "frame: %06"PRIu64", index: %04"PRIu64", offset: %02"PRIu64", "
        "address: %#"PRIx64", phys_bitmap[ %04"PRIu64" ]: %"PRIx32"\r\n",
        frame, index, offset, address, index, phys_bitmap[ index ]
      )
    #endif
  // normal handling
  } else {
    // mark page as free
    if ( ! phys_free_check_only( address ) ) {
      phys_bitmap[ index ] &= ( uint32_t )( ~( 1U << offset ) );
    }
    phys_bitmap_check[ index ] &= ( uint32_t )( ~( 1U << offset ) );
    // debug output
    #if defined( PRINT_MM_PHYS )
      DEBUG_OUTPUT(
        "frame: %06"PRIu64", index: %04"PRIu64", offset: %02"PRIu64", "
        "address: %#"PRIx64", phys_bitmap[ %04"PRIu64" ]: %"PRIx32"\r\n",
        frame, index, offset, address, index, phys_bitmap[ index ]
      )
    #endif
  }
}

/**
 * @fn void phys_mark_page_free(uint64_t)
 * @brief Mark physical page as free
 *
 * @param address address to mark as free
 */
void phys_mark_page_free_check( uint64_t address ) {
  // get frame, index and offset
  uint64_t frame = address / PAGE_SIZE;
  uint64_t index = PAGE_INDEX( frame );
  uint64_t offset = PAGE_OFFSET( frame );

  // dma handling
  if ( address >= phys_dma_start && address < phys_dma_end ) {
    return;
  }
  // mark page as free
  phys_bitmap_check[ index ] &= ( uint32_t )( ~( 1U << offset ) );

  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT(
      "frame: %06"PRIu64", index: %04"PRIu64", offset: %02"PRIu64", "
      "address: %#"PRIx64", phys_bitmap_check[ %04"PRIu64" ]: %#"PRIx32"\r\n",
      frame, index, offset, address, index, phys_bitmap_check[ index ]
    )
  #endif
}

/**
 * @fn void phys_free_page_range(uint64_t, size_t)
 * @brief Method to free phys page range
 *
 * @param address start address
 * @param amount amount of memory
 */
void phys_free_page_range( uint64_t address, size_t amount ) {
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "address: %#"PRIx64", amount: %zu\r\n", address, amount )
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
 * @fn void phys_free_page_range(uint64_t, size_t)
 * @brief Method to free phys page range
 *
 * @param address start address
 * @param amount amount of memory
 */
void phys_free_page_range_check( uint64_t address, size_t amount ) {
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "address: %#"PRIx64", amount: %zu\r\n", address, amount )
  #endif

  // loop until amount and mark as free
  for (
    size_t idx = 0;
    idx < amount / PAGE_SIZE;
    idx++, address += PAGE_SIZE
  ) {
    phys_mark_page_free_check( address );
  }
}

/**
 * @fn void phys_use_page_range(uint64_t, size_t)
 * @brief Method to mark phys page range as used
 *
 * @param address start address
 * @param amount amount of memory
 */
void phys_use_page_range( uint64_t address, size_t amount ) {
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "address: %#"PRIx64", amount: %zu\r\n", address, amount )
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
 * @fn uint64_t phys_find_free_page_range(size_t, size_t, phys_memory_type_t)
 * @brief Method to find free page range
 *
 * @param alignment wanted memory alignment
 * @param memory_amount amount of memory to find free page range for
 * @param type
 * @return address of found memory
 */
uint64_t phys_find_free_page_range( size_t alignment, size_t memory_amount, phys_memory_type_t type ) {
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT(
      "memory_amount: %zu, alignment: %#zx\r\n",
      memory_amount,
      alignment
    )
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

  size_t max_idx = phys_bitmap_length;
  uint32_t* bitmap = phys_bitmap;
  if ( PHYS_MEMORY_TYPE_DMA == type ) {
    max_idx = phys_dma_length;
    bitmap = phys_dma_bitmap;
  }

  // loop through bitmap to find free continuous space
  for ( size_t idx = 0; idx < max_idx && !stop; idx++ ) {
    // skip completely used entries
    if ( PHYS_ALL_PAGES_OF_INDEX_USED == bitmap[ idx ] ) {
      continue;
    }

    // loop through bits per entry
    for ( size_t offset = 0; offset < PAGE_PER_ENTRY && !stop; offset++ ) {
      // not free? => reset counter and continue
      if ( bitmap[ idx ] & ( uint32_t )( 1U << offset ) ) {
        found_amount = 0;
        address = 0;
        continue;
      }

      // set address if found is 0
      if ( 0 == found_amount ) {
        address = idx * PAGE_SIZE * PAGE_PER_ENTRY + offset * PAGE_SIZE;
        #if defined( PRINT_MM_PHYS )
          DEBUG_OUTPUT( "idx = %zu\r\n", idx )
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
  // possible offset
  uint64_t offset = 0;
  if ( PHYS_MEMORY_TYPE_DMA == type ) {
    offset = phys_dma_start;
  }
  // return found / not found address
  return address + offset;
}

/**
 * @fn uint64_t phys_find_free_page(size_t, phys_memory_type_t)
 * @brief Shorthand to find single free page
 *
 * @param alignment
 * @param type
 * @return
 */
uint64_t phys_find_free_page( size_t alignment, phys_memory_type_t type ) {
  return phys_find_free_page_range( alignment, PAGE_SIZE, type );
}

/**
 * @fn void phys_free_page(uint64_t)
 * @brief Shorthand for free one single page
 *
 * @param address address to free
 */
void phys_free_page( uint64_t address ) {
  phys_free_page_range( address, PAGE_SIZE );
}

/**
 * @fn bool phys_is_range_used(uint64_t, size_t)
 * @brief Helper to check if range is in use
 *
 * @param address start address
 * @param len length
 * @return
 */
bool phys_is_range_used( uint64_t address, size_t len ) {
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT(
      "len: %zu, address: %#"PRIx64"\r\n",
      len,
      address
    )
  #endif
  // handle invalid size
  if ( 0 == len ) {
    return false;
  }
  // determine index and offset start
  // calculate end address
  uint64_t end = address + len;
  while ( address < end ) {
    uint64_t frame = address / PAGE_SIZE;
    uint64_t index = PAGE_INDEX( frame );
    uint64_t offset = PAGE_OFFSET( frame );
    // debug output
    #if defined( PRINT_MM_PHYS )
      DEBUG_OUTPUT(
        "index = %"PRIu64", offset = %"PRIu64", frame = %#"PRIx64", "
        "address = %#"PRIx64", end = %#"PRIx64"\r\n",
        index, offset, frame, address, end
      )
    #endif
    // stop and return false if already used
    if ( phys_bitmap_check[ index ] & ( 1U << offset ) ) {
      return true;
    }
    // increase by page size
    address += PAGE_SIZE;
  }
  // not yet mapped
  return false;
}

/**
 * @fn void phys_init(void)
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
    DEBUG_OUTPUT( "start: %#"PRIxPTR"\r\n", start )
    DEBUG_OUTPUT( "end: %#"PRIxPTR"\r\n", end )
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
      DEBUG_OUTPUT( "start: %#"PRIxPTR"\r\n", start )
      DEBUG_OUTPUT( "end: %#"PRIxPTR"\r\n", end )
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
