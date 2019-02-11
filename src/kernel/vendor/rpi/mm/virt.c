
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
#include "kernel/kernel/entry.h"
#include "kernel/kernel/panic.h"
#include "kernel/kernel/mm/phys.h"
#include "kernel/kernel/mm/virt.h"
#include "kernel/arch/arm/mm/virt.h"
#include "kernel/vendor/rpi/peripheral.h"
#include "kernel/vendor/rpi/framebuffer.h"

/**
 * @brief Initialize virtual memory management
 */
void virt_init( void ) {
  // Get page directory
  uint32_t* ttbr1 = ( uint32_t* )phys_find_free_page_range(
    VSMA_SHORT_PAGE_DIRECTORY_SIZE,
    VSMA_SHORT_PAGE_DIRECTORY_ALIGNMENT
  );

  // Debug output
  #if defined( PRINT_MM_VIRT )
    printf( "ttbr1: 0x%08x\r\n", ttbr1 );
  #endif

  // Transform to virtual
  ttbr1 = ( uint32_t* )PHYS_2_VIRT( ttbr1 );

  // Initialize with zero
  memset( ttbr1, 0, VSMA_SHORT_PAGE_DIRECTORY_SIZE );

  // Debug output
  #if defined( PRINT_MM_VIRT )
    printf( "ttbr1: 0x%08x\r\n", ttbr1 );
  #endif

  // map kernel
  for (
    uintptr_t loop = 0;
    loop < placement_address + 1;
    loop += PHYS_PAGE_SIZE
  ) {
    // map
    // FIXME: Add correct flags
    virt_map_address(
      ( void* )ttbr1,
      ( void* )PHYS_2_VIRT( loop ),
      ( void* )loop,
      0
    );
  }

  // map peripherals
  for (
    uintptr_t loop = peripheral_base_get(), start = 0xF2000000;
    loop < peripheral_end_get() + 1;
    loop += PHYS_PAGE_SIZE, start += PHYS_PAGE_SIZE
  ) {
    // FIXME: Map with no cache!
    // FIXME: Add correct flags
    virt_map_address(
      ( void* )ttbr1,
      ( void* )start,
      ( void* )loop,
      0
    );
  }

  // update peripheral base
  peripheral_base_set( 0xF2000000 );

  // map framebuffer if enabled
  #if defined( TTY_FRAMEBUFFER )
    // determine end
    uintptr_t framebuffer_end = framebuffer_base_get() + framebuffer_size_get();

    // round up as usual
    if ( 0 < framebuffer_end % PHYS_PAGE_SIZE ) {
      framebuffer_end -= PHYS_PAGE_SIZE - ( framebuffer_end % PHYS_PAGE_SIZE );
    }

    // loop and map
    for (
      uintptr_t loop = framebuffer_base_get(), start = 0xF3000000;
      loop < framebuffer_end + 1;
      loop += PHYS_PAGE_SIZE, start += PHYS_PAGE_SIZE
    ) {
      // FIXME: map with no cache!
      // FIXME: Add correct flags
      virt_map_address(
        ( void* )ttbr1,
        ( void* )start,
        ( void* )loop,
        0
      );
    }

    // update framebuffer base
    framebuffer_set( 0xF3000000 );
  #endif
}
