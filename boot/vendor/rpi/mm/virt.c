
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

// FIXME: Find a better place
#if defined( IS_HIGHER_HALF )
  #if defined( ELF32 )
    #define KERNEL_OFFSET 0xC0000000
  #elif defined( ELF64 )
    #define KERNEL_OFFSET 0xffffffff80000000
  #endif
#else
  #define KERNEL_OFFSET 0
#endif

static uint32_t
  __attribute__( ( section( ".boot.data" ), aligned( 0x4000 ) ) )
  page_table[ 4096 ];

void __attribute__( ( section( ".text.boot" ) ) ) boot_create_mmu( void ) {
  uint32_t x;

  // map first gb
  for ( x = 0; x < 1024; x++ ) {
    // identity map for first gb
    page_table[ x ] = x << 20 | ( 3 << 10 ) | 0x12;

    // skip when no offset is existing
    if ( 0 >= KERNEL_OFFSET ) {
      continue;
    }

    // higher half virtual map
    page_table[ x + ( KERNEL_OFFSET >> 20 ) ] = x << 20 | ( 3 << 10 ) | 0x12;
  }

  // map cpu peripheral space
  for ( x = 1024; x < 1025; x += 0x100000 ) {
    page_table[ x ] = x << 20 | ( 3 << 10 ) | 0x12;
  }

  // Copy the page table address to cp15
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 0" : : "r" ( page_table ) : "memory" );
  // Set the access control to all-supervisor
  __asm__ __volatile__( "mcr p15, 0, %0, c3, c0, 0" : : "r" ( ~0 ) );

  uint32_t reg;
  // Enable the MMU
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( reg ) : : "cc" );
  reg |= 0x1;
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( reg ) : "cc" );
}
