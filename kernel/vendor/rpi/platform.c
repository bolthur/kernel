
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

#include <stdint.h>
#include <stdio.h>

#include "kernel/panic.h"

#include "vendor/rpi/platform.h"
#include "vendor/rpi/mailbox-property.h"

/**
 * @brief Boot parameter data set during startup
 */
platform_boot_parameter_t boot_parameter_data;

/**
 * @brief Platform depending initialization routine
 */
void platform_init( void ) {
  printf( "\r\n0x%08x - 0x%08x - 0x%08x\r\n",
    boot_parameter_data.zero,
    boot_parameter_data.machine,
    boot_parameter_data.atag );

  // Get arm memory
  mailbox_property_init();
  mailbox_property_add_tag( TAG_GET_ARM_MEMORY );
  mailbox_property_add_tag( TAG_GET_VC_MEMORY );
  mailbox_property_process();

  rpi_mailbox_property_t *buffer = mailbox_property_get( TAG_GET_ARM_MEMORY );
  printf ( "buffer->byte_length: %d\n", buffer->byte_length );
  printf ( "buffer->data.buffer_32[ 0 ]: 0x%08x\n", buffer->data.buffer_32[ 0 ] );
  printf ( "buffer->data.buffer_32[ 1 ]: 0x%08x\n", buffer->data.buffer_32[ 1 ] );
  printf ( "buffer->tag: 0x%08x\n", buffer->tag );

  buffer = mailbox_property_get( TAG_GET_VC_MEMORY );
  printf ( "buffer->byte_length: %d\n", buffer->byte_length );
  printf ( "buffer->data.buffer_32[ 0 ]: 0x%08x\n", buffer->data.buffer_32[ 0 ] );
  printf ( "buffer->data.buffer_32[ 1 ]: 0x%08x\n", buffer->data.buffer_32[ 1 ] );
  printf ( "buffer->tag: 0x%08x\n", buffer->tag );

  // FIXME: Load firmware revision, board model, board revision, board serial from mailbox
  // FIXME: Load memory information from mailbox regarding arm and gpu and populate memory map
}
