
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

#include <arch/arm/mm/virt.h>
#include <arch/arm/v7/boot/mm/virt/short.h>
#include <kernel/entry.h>

/**
 * @brief Helper to setup initial paging with large page address extension
 *
 * @todo Add initial mapping for paging with lpae
 * @todo Remove call for setup short paging
 */
void SECTION( ".text.boot" )
boot_virt_setup_long( uintptr_t max_memory ) {
  boot_virt_setup_short( max_memory );
  return;

  // variables
  ld_ttbcr_t ttbcr;
  uint32_t reg;

  // FIXME: Prepare initial kernel context
  /*for ( x = 0; x < 4096; x++ ) {
    initial_kernel_context.list[ x ] = 0;
  }*/
  // FIXME: Prepare initial user context
  /*for ( x = 0; x < 2048; x++ ) {
    initial_user_context.list[ x ] = 0;
  } */

  // FIXME: Map initial memory
  /*for ( x = 0; x < ( max_memory >> 20 ); x++ ) {
    boot_virt_map_long( x << 20, x << 20 );

    if ( 0 < KERNEL_OFFSET ) {
      boot_virt_map_long( x << 20, ( x + ( KERNEL_OFFSET >> 20 ) ) << 20 );
    }
  } */


  // FIXME: Set initial context
  // Copy page table address to cp15
  /*__asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 0" : : "r" ( initial_user_context.list ) : "memory" );
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 1" : : "r" ( initial_kernel_context.list ) : "memory" );
  // Set the access control to all-supervisor
  __asm__ __volatile__( "mcr p15, 0, %0, c3, c0, 0" : : "r" ( ~0 ) );*/

  // prepare ttbcr
  ttbcr.raw = 0;
  // set split to use ttbr0 and ttbr1 as it will be used later on
  ttbcr.data.ttbr0_size = LD_TTBCR_SIZE_TTBR_2G;
  ttbcr.data.ttbr1_size = LD_TTBCR_SIZE_TTBR_2G;
  // set large physical address extension bit
  ttbcr.data.large_physical_address_extension = 1;
  // push value to ttbcr
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 2" : : "r" ( ttbcr.raw ) : "cc" );

  // Get content from control register
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( reg ) : : "cc" );
  // enable mmu by setting bit 0
  reg |= 0x1;
  // push back value with mmu enabled bit set
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( reg ) : "cc" );
}

/**
 * @brief Method to perform identity nap
 */
void SECTION( ".text.boot" )
boot_virt_map_long( uintptr_t phys, uintptr_t virt ) {
  boot_virt_map_short( phys, virt );
}
