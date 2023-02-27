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
#include "../../../lib/string.h"
#include "../../../lib/stdlib.h"
#include "../../../lib/inttypes.h"
#if defined( PRINT_MM_PHYS )
  #include "../../../debug/debug.h"
#endif
#include "../../../entry.h"
#include "../../../mm/phys.h"
#include "../peripheral.h"
#include "../mailbox/mailbox.h"
#include "../mailbox/property.h"

#define DMA_POOL_SIZE 0x400000

/**
 * @fn bool phys_platform_init(void)
 * @brief Initialize physical memory manager for raspi
 *
 * @return
 */
bool phys_platform_init( void ) {
  // Get arm memory
  mailbox_property_init();
  mailbox_property_add_tag( TAG_GET_ARM_MEMORY );
  mailbox_property_add_tag( TAG_GET_VC_MEMORY );
  if ( MAILBOX_ERROR == mailbox_property_process() ) {
    return false;
  }
  // max memory
  uint32_t memory_amount;
  // video core memory
  uint32_t vc_memory_start;
  uint32_t vc_memory_end;
  // get arm memory
  raspi_mailbox_property_t* buffer = mailbox_property_get( TAG_GET_ARM_MEMORY );
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "buffer->byte_length: %"PRId32"\r\n", buffer->byte_length )
    DEBUG_OUTPUT(
      "buffer->data.buffer_u32[ 0 ]: %#"PRIx32"\r\n",
      buffer->data.buffer_u32[ 0 ]
    )
    DEBUG_OUTPUT(
      "buffer->data.buffer_u32[ 1 ]: %#"PRIx32"\r\n",
      buffer->data.buffer_u32[ 1 ]
    )
    DEBUG_OUTPUT( "buffer->tag: %#"PRIx32"\r\n", ( uint32_t )buffer->tag )
  #endif
  // increase amount by arm amount
  memory_amount = buffer->data.buffer_u32[ 1 ];
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "memory amount: %"PRIx32"\r\n", memory_amount )
  #endif
  // get video core memory
  buffer = mailbox_property_get( TAG_GET_VC_MEMORY );
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "buffer->byte_length: %"PRId32"\r\n", buffer->byte_length )
    DEBUG_OUTPUT(
      "buffer->data.buffer_u32[ 0 ]: %"PRIx32"\r\n",
      buffer->data.buffer_u32[ 0 ]
    )
    DEBUG_OUTPUT(
      "buffer->data.buffer_u32[ 1 ]: %"PRIx32"\r\n",
      buffer->data.buffer_u32[ 1 ]
    )
    DEBUG_OUTPUT( "buffer->tag: %"PRIx32"\r\n", ( uint32_t )buffer->tag )
  #endif
  // populate video core start and end
  vc_memory_start = buffer->data.buffer_u32[ 0 ];
  vc_memory_end = vc_memory_start + buffer->data.buffer_u32[ 1 ];
  // increase amount by video core amount
  memory_amount += buffer->data.buffer_u32[ 1 ];
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "memory amount: %#"PRIx32"\r\n", memory_amount )
  #endif

  // determine amount of pages for bitmap
  phys_bitmap_length = memory_amount / PAGE_SIZE / PAGE_PER_ENTRY;
  // reserve space for bitmap and align it to pointer
  phys_bitmap = aligned_alloc(
    sizeof( phys_bitmap ),
    phys_bitmap_length * sizeof( uint32_t ) );
  if ( ! phys_bitmap ) {
    return false;
  }
  phys_bitmap_check = aligned_alloc(
    sizeof( phys_bitmap ),
    phys_bitmap_length * sizeof( uint32_t ) );
  if ( ! phys_bitmap_check ) {
    return false;
  }
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "total memory amount: %#"PRIx32"\r\n", memory_amount )
    DEBUG_OUTPUT( "bitmap length: %"PRIu32"\r\n", phys_bitmap_length )
    DEBUG_OUTPUT( "phys bitmap address: %p\r\n", phys_bitmap )
    DEBUG_OUTPUT( "content of __kernel_start: %p\r\n", &__kernel_start )
    DEBUG_OUTPUT( "content of __kernel_end: %p\r\n", &__kernel_end )
  #endif
  // overwrite all bitmaps completely with zero
  memset( phys_bitmap, 0, phys_bitmap_length * sizeof( uint32_t ) );
  memset( phys_bitmap_check, 0, phys_bitmap_length * sizeof( uint32_t ) );
  // set start and end for peripherals
  uintptr_t start = PERIPHERAL_GPIO_BASE;
  uintptr_t end = start + PERIPHERAL_GPIO_SIZE + 1;
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "start: %#"PRIxPTR"\r\n", start )
    DEBUG_OUTPUT( "end: %#"PRIxPTR"\r\n", end )
  #endif
  // mark as used in bitmap and free in check
  phys_use_page_range( start, end - start );
  phys_free_page_range_check( start, end - start );
  // set start and end video core
  start = vc_memory_start;
  end = vc_memory_end + 1;
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "start: %#"PRIxPTR"\r\n", start )
    DEBUG_OUTPUT( "end: %#"PRIxPTR"\r\n", end )
  #endif
  // mark as used in bitmap and as free in check
  phys_use_page_range( start, end - start );
  phys_free_page_range_check( start, end - start );
  // return success
  return true;
}

/**
 * @fn bool phys_free_check_only(uintptr_t)
 * @brief Method to check whether free has to be done for check only
 *
 * @param address
 * @return
 */
bool phys_free_check_only( uint64_t address ) {
  return
    PERIPHERAL_GPIO_BASE <= address
    && address < ( PERIPHERAL_GPIO_BASE + PERIPHERAL_GPIO_SIZE + 1 );
}

/**
 * @fn uint64_t phys_address_to_bus(uint64_t, size_t)
 * @brief Translate physical address to bus
 *
 * @param address
 * @param size
 * @return
 */
uint64_t phys_address_to_bus( uint64_t address, size_t size ) {
  // check for in range
  if (
    address >= phys_dma_start
    && address <= phys_dma_end
    && address + size <= phys_dma_start
  ) {
    return 0;
  }
  // return bus address
  return address + 0xC0000000;
}

/**
 * @fn bool phys_init_dma(void)
 * @brief Setup dma
 *
 * @return
 */
bool phys_dma_init( void ) {
  // just 4 MB of dma
  phys_dma_length = DMA_POOL_SIZE / PAGE_SIZE / PAGE_PER_ENTRY * sizeof( uint32_t );
  phys_dma_bitmap = malloc( phys_dma_length );
  if ( ! phys_dma_bitmap ) {
    return false;
  }
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "phys dma bitmap address: %p\r\n", phys_dma_bitmap )
    DEBUG_OUTPUT( "phys dma length: %"PRIu32"\r\n", phys_dma_length )
  #endif
  // clear area
  memset( phys_dma_bitmap, 0, phys_dma_length );
  // use contiguous area for dma
  size_t dma_size = DMA_POOL_SIZE;
  uint64_t dma_start = phys_find_free_page_range( PAGE_SIZE, dma_size, PHYS_MEMORY_TYPE_NORMAL );
  if ( ! dma_start ) {
    return false;
  }
  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "dma_start: %#"PRIx64"\r\n", dma_start )
    DEBUG_OUTPUT( "dma_size: %zu\r\n", dma_size )
  #endif
  // set start and end of dma
  phys_dma_start = dma_start;
  phys_dma_end = dma_start + dma_size;
  // return success
  return true;
}
