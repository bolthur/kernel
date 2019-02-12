
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

#include <stdbool.h>
#include <stdint.h>

#include "kernel/arch/arm/mm/virt.h"

uint32_t
  __attribute__(
    ( section( ".boot.data" ),
    aligned( VSMA_SHORT_PAGE_DIRECTORY_ALIGNMENT ) )
  ) ttbr[ 4096 ];

/**
 * @brief Method to check for virtual memory is supported
 */
bool __attribute__( ( section( ".text.boot" ) ) ) boot_virt_available( void ) {
  uint32_t reg;

  // get paging support from mmfr0
  __asm__ __volatile__( "mrc p15, 0, %0, c0, c1, 4" : "=r" ( reg ) : : "cc" );

  // strip out everything not needed
  reg &= 0xF;

  return (
    ID_MMFR0_VSMA_SUPPORT_V7_PAGING_REMAP_ACCESS == reg
    || ID_MMFR0_VSMA_SUPPORT_V7_PAGING_PXN == reg
    || ID_MMFR0_VSMA_SUPPORT_V7_PAGING_LPAE == reg );
}

/**
 * @brief Method to map virtual address to physical during initial boot
 *
 * @param virtual virtual address
 * @param physical physical address
 */
void __attribute__( ( section( ".text.boot" ) ) ) boot_virt_map_address( void* virtual, void* physical ) {
  // transform virtual and physical address to 1MB
  uint32_t v = ( uint32_t )virtual & 0xFFF00000;
  uint32_t p = ( uint32_t )physical & 0xFFF00000;

  ttbr[ v >> 20 ] = p | ( 3 << 10 ) | 0x10 | TTBR_L1_IS_SECTION;
}

/**
 * @brief Method to enable virtual memory during initial boot
 */
void __attribute__( ( section( ".text.boot" ) ) ) boot_virt_enable( void ) {
  uint32_t reg;

  // invalidate cache
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c7, 0" : : "r" ( 0 ) : "memory" );
  // invalidate tlb
  __asm__ __volatile__( "mcr p15, 0, %0, c8, c7, 0" : : "r" ( 0 ) : "memory" );

  // Copy page table address to cp15
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 0" : : "r" ( &ttbr[ 0 ] ) : "memory" );
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 1" : : "r" ( &ttbr[ 2048 ] ) : "memory" );

  // Set the access control to all-supervisor
  __asm__ __volatile__( "mcr p15, 0, %0, c3, c0, 0" : : "r" ( ~0 ) );

  // set ttbcr.n bit
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 2" : : "r" ( 0x1 || ( 0x1 << 16 ) ) : "cc" );

  // Get content from control register
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( reg ) : : "cc" );

  // enable mmu by setting bit 0
  reg |= 0x1;

  // push back value with mmu enabled bit set
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( reg ) : "cc" );
}
