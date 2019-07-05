
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

#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#include <kernel/debug.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/placement.h>
#include <platform/rpi/platform.h>
#include <platform/rpi/mailbox/property.h>

/**
 * @brief Boot parameter data set during startup
 */
platform_loader_parameter_t loader_parameter_data;

/**
 * @brief initrd start from linker script
 */
extern uintptr_t __initrd_start;

/**
 * @brief initrd end from linker script
 */
extern uintptr_t __initrd_end;

/**
 * @brief debug initrd
 */
extern uint32_t debug_initrd;

/**
 * @brief Platform depending initialization routine
 *
 * @todo Move placement address beyond initrd
 */
void platform_init( void ) {
  // default initrd should be at end of placement address
  uintptr_t start = placement_address;
  uintptr_t end = 0;

  // debug output
  #if defined( PRINT_INITRD )
    DEBUG_OUTPUT(
      "placement_address = 0x%08x, start = 0x%08x, end = 0x%08x\r\n",
      placement_address, start, end
    );
  #endif

  // overwrite initrd if debug initrd is enabled and end
  #if defined( DEBUG_INITRD )
    start = ( uintptr_t )&debug_initrd;
    end = ( uintptr_t )&__initrd_end;
  #endif

  // assert set end
  assert( 0 != end && start < end );

  // debug output
  #if defined( PRINT_INITRD )
    DEBUG_OUTPUT(
      "placement_address = 0x%08x, start = 0x%08x, end = 0x%08x\r\n",
      placement_address, start, end
    );
  #endif

  // update placement address to be beyond initrd
  placement_address = end;

  // debug output
  #if defined( PRINT_INITRD )
    DEBUG_OUTPUT(
      "placement_address = 0x%08x, start = 0x%08x, end = 0x%08x\r\n",
      placement_address, start, end
    );
  #endif
}
