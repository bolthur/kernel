
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

#include <stddef.h>

#include <string.h>
#include <stdlib.h>
#include <kernel/debug.h>
#include <kernel/entry.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/placement.h>
#include <platform/rpi/platform.h>
#include <platform/rpi/peripheral.h>
#include <platform/rpi/mailbox/property.h>

/**
 * @brief Initialize physical memory manager for rpi
 */
void phys_platform_init( void ) {
  // Get arm memory
  mailbox_property_init();
  mailbox_property_add_tag( TAG_GET_ARM_MEMORY );
  mailbox_property_add_tag( TAG_GET_VC_MEMORY );
  mailbox_property_process();

  // max memory
  uint32_t memory_amount = 0;

  // video core memory
  uint32_t vc_memory_start = 0;
  uint32_t vc_memory_end = 0;

  // get arm memory
  rpi_mailbox_property_t *buffer = mailbox_property_get( TAG_GET_ARM_MEMORY );

  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "buffer->byte_length: %d\r\n", buffer->byte_length );
    DEBUG_OUTPUT(
      "buffer->data.buffer_u32[ 0 ]: 0x%08x\r\n",
      buffer->data.buffer_u32[ 0 ]
    );
    DEBUG_OUTPUT(
      "buffer->data.buffer_u32[ 1 ]: 0x%08x\r\n",
      buffer->data.buffer_u32[ 1 ]
    );
    DEBUG_OUTPUT( "buffer->tag: 0x%08x\r\n", buffer->tag );
  #endif

  // increase amount by arm amount
  memory_amount = buffer->data.buffer_u32[ 1 ];

  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "memory amount: 0x%08x\r\n", memory_amount );
  #endif

  // get video core memory
  buffer = mailbox_property_get( TAG_GET_VC_MEMORY );

  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT(
      "buffer->byte_length: %d\r\n",
      buffer->byte_length
    );
    DEBUG_OUTPUT(
      "buffer->data.buffer_u32[ 0 ]: 0x%08x\r\n",
      buffer->data.buffer_u32[ 0 ]
    );
    DEBUG_OUTPUT(
      "buffer->data.buffer_u32[ 1 ]: 0x%08x\r\n",
      buffer->data.buffer_u32[ 1 ]
    );
    DEBUG_OUTPUT(
      "buffer->tag: 0x%08x\r\n",
      buffer->tag
    );
  #endif

  // populate video core start and end
  vc_memory_start = buffer->data.buffer_u32[ 0 ];
  vc_memory_end = vc_memory_start + buffer->data.buffer_u32[ 1 ];

  // increase amount by video core amount
  memory_amount += buffer->data.buffer_u32[ 1 ];

  // determine amount of pages for bitmap
  size_t pages_per_size = sizeof( phys_bitmap_length ) * 8;
  phys_bitmap_length = memory_amount / PAGE_SIZE / pages_per_size;

  // allocate bitmap manually via placement address after kernel
  // align it to page size
  phys_bitmap = ( uint32_t* )aligned_alloc(
    sizeof( phys_bitmap ),
    phys_bitmap_length );

  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "total memory amount: 0x%08x\r\n", memory_amount );
    DEBUG_OUTPUT( "bitmap length: %u\r\n", phys_bitmap_length );
    DEBUG_OUTPUT( "phys bitmap address: 0x%08x\r\n", ( uint32_t )phys_bitmap );
    DEBUG_OUTPUT( "content of __kernel_start: 0x%08lx\r\n", ( uintptr_t )&__kernel_start );
    DEBUG_OUTPUT( "content of __kernel_end: 0x%08lx\r\n", ( uintptr_t ) &__kernel_end );
  #endif

  // overwrite physical bitmap completely with zero
  memset( phys_bitmap, 0, sizeof( phys_bitmap_length ) * phys_bitmap_length );

  // set start and end for peripherals
  uintptr_t start = peripheral_base_get( PERIPHERAL_GPIO );
  uintptr_t end = peripheral_end_get( PERIPHERAL_GPIO ) + 1;

  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "start: 0x%lx\r\n", start );
    DEBUG_OUTPUT( "end: 0x%lx\r\n", end );
  #endif

  // map from start to end addresses as used
  while( start < end ) {
    // mark used
    phys_mark_page_used( start );

    // get next page
    start += PAGE_SIZE;
  }

  // set start and end video core
  start = vc_memory_start;
  end = vc_memory_end + 1;

  // debug output
  #if defined( PRINT_MM_PHYS )
    DEBUG_OUTPUT( "start: 0x%lx\r\n", start );
    DEBUG_OUTPUT( "end: 0x%lx\r\n", end );
  #endif

  // map from start to end addresses as used
  while( start < end ) {
    // mark used
    phys_mark_page_used( start );

    // get next page
    start += PAGE_SIZE;
  }
}
