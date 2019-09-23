
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
#include <kernel/panic.h>
#include <kernel/debug.h>
#include <platform/rpi/peripheral.h>
#include <platform/rpi/mailbox/property.h>
#include <arch/arm/mm/virt.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/virt.h>

#define GPIO_PERIPHERAL_BASE 0xF2000000
#if defined( BCM2709 ) || defined( BCM2710 )
  #define CPU_PERIPHERAL_BASE 0xF3000000
#endif

/**
 * @brief Initialize virtual memory management
 */
void virt_platform_init( void ) {
  uintptr_t start, end, virtual;

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "Map peripherals 0x%08x - 0x%08x\r\n",
      peripheral_base_get( PERIPHERAL_GPIO ),
      peripheral_end_get( PERIPHERAL_GPIO )
    );
  #endif

  // set start and virtual
  start = peripheral_base_get( PERIPHERAL_GPIO );
  virtual = GPIO_PERIPHERAL_BASE;
  end = peripheral_end_get( PERIPHERAL_GPIO );

  // map peripherals
  while ( start < end ) {
    // map
    virt_map_address(
      kernel_context,
      virtual,
      start,
      MEMORY_TYPE_DEVICE,
      PAGE_TYPE_AUTO );

    // increase start and virtual
    start += PAGE_SIZE;
    virtual += PAGE_SIZE;
  }

  // handle local peripherals
  #if defined( BCM2709 ) || defined( BCM2710 )
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "Map local peripherals 0x%08x - 0x%08x\r\n",
        peripheral_base_get( PERIPHERAL_LOCAL ),
        peripheral_end_get( PERIPHERAL_LOCAL )
      );
    #endif

    // set start and virtual
    start = peripheral_base_get( PERIPHERAL_LOCAL );
    end = peripheral_end_get( PERIPHERAL_LOCAL );
    virtual = CPU_PERIPHERAL_BASE;

    // map peripherals
    while ( start < end ) {
      // map
      virt_map_address(
        kernel_context,
        virtual,
        start,
        MEMORY_TYPE_DEVICE,
        PAGE_TYPE_AUTO );

      // increase start and virtual
      start += PAGE_SIZE;
      virtual += PAGE_SIZE;
    }
  #endif
}

/**
 * @brief Platform post initialization routine
 */
void virt_platform_post_init( void ) {
  // set new peripheral base
  peripheral_base_set( GPIO_PERIPHERAL_BASE, PERIPHERAL_GPIO );
  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "Set new gpio peripheral base to 0x%08x\r\n", GPIO_PERIPHERAL_BASE );
  #endif

  // Adjust base address of cpu peripheral
  peripheral_base_set( CPU_PERIPHERAL_BASE, PERIPHERAL_LOCAL );
  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "Set new cpu peripheral base to 0x%08x\r\n", CPU_PERIPHERAL_BASE );
  #endif
}
