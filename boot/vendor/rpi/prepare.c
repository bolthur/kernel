
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

#include <stdint.h>

#include "boot/mm/virt.h"
#include "boot/entry.h"

void __attribute__( ( section( ".text.boot" ) ) ) boot_vendor_prepare( void ) {
  uint32_t x;

  // Check for supported mmu
  if ( ! boot_virt_available() ) {
    return;
  }

  // map first gb
  for ( x = 0; x < ( MAX_PHYSICAL_MEMORY >> 20 ); x++ ) {
    // identity map of x
    boot_virt_map_address(
      ( void* )( x << 20 ),
      ( void* )( x << 20 )
    );

    // higher half if set
    if ( 0 < KERNEL_OFFSET ) {
      boot_virt_map_address(
        ( void* )( ( x << 20 ) + KERNEL_OFFSET ),
        ( void* )( x << 20 )
      );
    }
  }

  // map cpu peripheral if set
  #if defined( PLATFORM_RPI2_B ) || defined( PLATFORM_RPI3_B )
    boot_virt_map_address(
      ( void* )0x40000000,
      ( void* )0x40000000
    );
  #endif

  boot_virt_enable();
}
