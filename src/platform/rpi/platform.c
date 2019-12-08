
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

#include <stdio.h>
#include <atag.h>
#include <kernel/initrd.h>
#include <kernel/debug/debug.h>
#include <platform/rpi/platform.h>

/**
 * @brief Boot parameter data set during startup
 */
platform_loader_parameter_t loader_parameter_data;

/**
 * @brief Platform depending initialization routine
 */
void platform_init( void ) {
  #if defined( PRINT_PLATFORM )
    DEBUG_OUTPUT(
      "0x%08x - 0x%08x - 0x%08x\r\n",
      loader_parameter_data.atag,
      loader_parameter_data.machine,
      loader_parameter_data.zero
    );
  #endif

  // get first atag
  atag_ptr_t atag = ( atag_ptr_t )loader_parameter_data.atag;
  // loop until atag end reached
  while ( atag ) {
    // handle initrd
    if ( atag->tag_type == ATAG_TAG_INITRD2 ) {
      #if defined( PRINT_PLATFORM )
        DEBUG_OUTPUT( "atag initrd start: 0x%08x, size: 0x%08x\r\n",
          atag->initrd.start, atag->initrd.size );
      #endif
      initrd_set_start_address( atag->initrd.start );
      initrd_set_size( atag->initrd.size );
    }

    // get next
    atag = atag_next( atag );
  }
}
