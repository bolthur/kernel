
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
#include <kernel/type.h>
#include <kernel/panic.h>
#include <kernel/debug.h>
#include <vendor/rpi/peripheral.h>
#include <mm/kernel/arch/arm/virt.h>
#include <mm/kernel/kernel/phys.h>
#include <mm/kernel/kernel/virt.h>

#define GPIO_PERIPHERAL_BASE ( vaddr_t )0xF2000000
#if defined( BCM2709 ) || defined( BCM2710 )
  #define CPU_PERIPHERAL_BASE ( vaddr_t )0xF3000000
#endif

/**
 * @brief Initialize virtual memory management
 */
void virt_vendor_init( void ) {
  paddr_t start;
  vaddr_t virtual;

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "Map peripherals 0x%08x - 0x%08x\r\n",
      peripheral_base_get( PERIPHERAL_GPIO ),
      peripheral_end_get( PERIPHERAL_GPIO )
    );
  #endif

  // set start and virtual
  start = ( paddr_t )peripheral_base_get( PERIPHERAL_GPIO );
  virtual = GPIO_PERIPHERAL_BASE;

  // map peripherals
  while ( start < ( paddr_t )peripheral_end_get( PERIPHERAL_GPIO ) ) {
    // map
    virt_map_address( kernel_context, virtual, start, PAGE_FLAG_NONE );

    // increase start and virtual
    start += PAGE_SIZE;
    virtual = ( vaddr_t )( ( paddr_t )virtual + PAGE_SIZE );
  }

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
    start = ( paddr_t )peripheral_base_get( PERIPHERAL_LOCAL );
    virtual = CPU_PERIPHERAL_BASE;

    // map peripherals
    while ( start < ( paddr_t )peripheral_end_get( PERIPHERAL_LOCAL ) ) {
      // map
      virt_map_address( kernel_context, virtual, start, PAGE_FLAG_NONE );

      // increase start and virtual
      start += PAGE_SIZE;
      virtual = ( vaddr_t )( ( paddr_t )virtual + PAGE_SIZE );
    }
  #endif
}

/**
 * @brief Vendor post initialization routine
 */
void virt_vendor_post_init( void ) {
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
