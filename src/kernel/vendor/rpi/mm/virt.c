
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
#include "kernel/arch/arm/barrier.h"
#include "kernel/arch/arm/mm/virt.h"
#include "kernel/vendor/rpi/peripheral.h"
#include "kernel/vendor/rpi/framebuffer.h"

extern uint32_t ttbr0[ 2048 ];
extern uint32_t ttbr1[ 2048 ];

/**
 * @brief Initialize virtual memory management
 */
void virt_init( void ) {
  return;

  // Debug output
  #if defined( PRINT_MM_VIRT )
    printf( "ttbr1: 0x%08x\r\n", ttbr1 );
  #endif

  // Initialize with zero
  memset( ttbr1, 0, VSMA_SHORT_PAGE_DIRECTORY_SIZE );

  virt_map_address( ( void* )ttbr1, ( void* )0xC0001000, ( void* )0x1000, 0 );
  printf( "ttbr0[ 1 ]: 0x%08x\tttbr1[ 1 ]: 0x%08x\r\n", ttbr0[ 1 ], ttbr1[ 1024 ] );

  // Debug output
  #if defined( PRINT_MM_VIRT )
    printf( "ttbr0: 0x%08x\r\nttbr1: 0x%08x\r\n", ttbr0, ttbr1 );
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
  #endif

  // remove ttbr0
  // __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 0" : : "r" ( 0 ) : "memory" );

  // Copy page table address to cp15
  #if defined( PRINT_MM_VIRT )
    printf( "ttbr0: 0x%08x\r\nttbr1: 0x%08x\r\n", ttbr0, ttbr1 );
  #endif

  // invalidate cache
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c7, 0" : : "r" ( 0 ) : "memory" );
  // invalidate tlb
  __asm__ __volatile__( "mcr p15, 0, %0, c8, c7, 0" : : "r" ( 0 ) : "memory" );

  // data barrier
  barrier_data_mem();

  #if defined( TTY_FRAMEBUFFER )
    // update framebuffer base
    framebuffer_base_set( 0xF3000000 );
  #endif

  // update peripheral base
  peripheral_base_set( 0xF2000000 );

  printf( "0x%08x\r\n", *( ( uint32_t* )( framebuffer_base_get() + FRAMEBUFFER_SCREEN_WIDTH / 2 ) ) );
  printf( "0x%08x\r\n", *( ( uint32_t* )( 0xF2000000 ) ) );
  PANIC( "FOO" );
}
