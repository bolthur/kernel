
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

#include "lib/stdc/stdio.h"
#include "lib/stdc/string.h"
#include "lib/stdc/stdlib.h"
#include "kernel/kernel/entry.h"
#include "kernel/kernel/mm/phys.h"
#include "kernel/vendor/rpi/platform.h"
#include "kernel/vendor/rpi/peripheral.h"
#include "kernel/vendor/rpi/mailbox/property.h"

/**
 * @brief Initialize physical memory manager for rpi
 */
void phys_init( void ) {
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
    printf ( "buffer->byte_length: %d\r\n", buffer->byte_length );
    printf ( "buffer->data.buffer_32[ 0 ]: 0x%08x\r\n", buffer->data.buffer_32[ 0 ] );
    printf ( "buffer->data.buffer_32[ 1 ]: 0x%08x\r\n", buffer->data.buffer_32[ 1 ] );
    printf ( "buffer->tag: 0x%08x\r\n", buffer->tag );
  #endif

  // increase amount by arm amount
  memory_amount = buffer->data.buffer_u32[ 1 ];

  // debug output
  #if defined( PRINT_MM_PHYS )
    printf( "memory amount: 0x%8x\r\n", memory_amount );
  #endif

  // get video core memory
  buffer = mailbox_property_get( TAG_GET_VC_MEMORY );

  // debug output
  #if defined( PRINT_MM_PHYS )
    printf ( "buffer->byte_length: %d\r\n", buffer->byte_length );
    printf ( "buffer->data.buffer_32[ 0 ]: 0x%08x\r\n", buffer->data.buffer_32[ 0 ] );
    printf ( "buffer->data.buffer_32[ 1 ]: 0x%08x\r\n", buffer->data.buffer_32[ 1 ] );
    printf ( "buffer->tag: 0x%08x\r\n", buffer->tag );
  #endif

  // populate video core start and end
  vc_memory_start = buffer->data.buffer_u32[ 0 ];
  vc_memory_end = vc_memory_start + buffer->data.buffer_u32[ 1 ];

  // increase amount by video core amount
  memory_amount += buffer->data.buffer_u32[ 1 ];

  // determine amount of pages for bitmap
  phys_bitmap_length = memory_amount / PHYS_PAGE_SIZE / ( sizeof( phys_bitmap_length ) * 8 );

  // allocate bitmap manually via placement address after kernel
  phys_bitmap = ( uintptr_t* )PHYS_2_VIRT( placement_address );
  placement_address += phys_bitmap_length;

  // overwrite physical bitmap completely with zero
  memset( phys_bitmap, 0, sizeof( phys_bitmap_length ) * phys_bitmap_length );

  // debug output
  #if defined( PRINT_MM_PHYS )
    printf( "total memory amount: 0x%8x\r\n", memory_amount );
    printf( "bitmap length: %d\r\n", phys_bitmap_length );
    printf( "phys bitmap address: 0x%08x\r\n", phys_bitmap );
    printf( "content of __kernel_start: 0x%08x\r\n", &__kernel_start );
    printf( "content of __kernel_end: 0x%08x\r\n", &__kernel_end );
    printf( "content of placement address: 0x%08x\r\n", placement_address );
  #endif

  // determine start and end for kernel mapping
  uintptr_t start = 0;
  uintptr_t end = placement_address + placement_address % PHYS_PAGE_SIZE;

  // adjust placement address
  placement_address = end;

  // map from start to end addresses as used
  while( start < end ) {
    // mark used
    phys_mark_page_used( ( void* )start );

    // get next page
    start += PHYS_PAGE_SIZE;
  }

  // set start and end for peripherals
  start = peripheral_base_get();
  end = peripheral_end_get() + 1;

  // map from start to end addresses as used
  while( start < end ) {
    // mark used
    phys_mark_page_used( ( void* )start );

    // get next page
    start += PHYS_PAGE_SIZE;
  }

  // set start and end video core
  start = vc_memory_start;
  end = vc_memory_end + 1;

  // map from start to end addresses as used
  while( start < end ) {
    // mark used
    phys_mark_page_used( ( void* )start );

    // get next page
    start += PHYS_PAGE_SIZE;
  }
}
