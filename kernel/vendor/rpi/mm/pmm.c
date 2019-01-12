
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stddef.h>

#include "kernel/mm/kmalloc.h"
#include "kernel/mm/pmm.h"

#include "arch/arm/mm/pmm.h"

#include "vendor/rpi/platform.h"
#include "vendor/rpi/mailbox-property.h"

/**
 * @brief Initialize physical memory manager
 */
void pmm_init_vendor( void ) {
  // Get arm memory
  mailbox_property_init();
  mailbox_property_add_tag( TAG_GET_ARM_MEMORY );
  mailbox_property_add_tag( TAG_GET_VC_MEMORY );
  mailbox_property_process();

  // max memory
  uint32_t memory_amount = 0;

  // get arm memory
  rpi_mailbox_property_t *buffer = mailbox_property_get( TAG_GET_ARM_MEMORY );

  // debug output
  #if defined( DEBUG )
    printf ( "buffer->byte_length: %d\n", buffer->byte_length );
    printf ( "buffer->data.buffer_32[ 0 ]: 0x%08x\n", buffer->data.buffer_32[ 0 ] );
    printf ( "buffer->data.buffer_32[ 1 ]: 0x%08x\n", buffer->data.buffer_32[ 1 ] );
    printf ( "buffer->tag: 0x%08x\n", buffer->tag );
  #endif

  // increase amount by arm amount
  memory_amount = ( uint32_t )buffer->data.buffer_32[ 1 ];

  // debug output
  #if defined( DEBUG )
    printf( "memory amount: 0x%8x\r\n", memory_amount );
  #endif

  // get video core memory
  buffer = mailbox_property_get( TAG_GET_VC_MEMORY );

  // debug output
  #if defined( DEBUG )
    printf ( "buffer->byte_length: %d\n", buffer->byte_length );
    printf ( "buffer->data.buffer_32[ 0 ]: 0x%08x\n", buffer->data.buffer_32[ 0 ] );
    printf ( "buffer->data.buffer_32[ 1 ]: 0x%08x\n", buffer->data.buffer_32[ 1 ] );
    printf ( "buffer->tag: 0x%08x\n", buffer->tag );
  #endif

  // increase amount by video core amount
  memory_amount += ( uint32_t )buffer->data.buffer_32[ 1 ];

  // determine amount of pages for bitmap
  pmm_bitmap_length = memory_amount / PMM_PAGE_SIZE / ( sizeof( pmm_bitmap_length ) * 8 );

  // prepare bitmap
  pmm_bitmap = ( uintptr_t* ) kmalloc( pmm_bitmap_length );

  // debug output
  #if defined( DEBUG )
    printf( "total memory amount: 0x%8x\r\n", memory_amount );
    printf( "bitmap length: %d\r\n", pmm_bitmap_length );
    printf( "pmm bitmap address: 0x%08x\r\n", pmm_bitmap );
  #endif
}
