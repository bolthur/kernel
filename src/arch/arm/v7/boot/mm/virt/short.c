
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

#include <arch/arm/mm/virt/short.h>
#include <arch/arm/v7/boot/mm/virt/short.h>
#include <kernel/entry.h>

/**
 * @brief Initial kernel context
 */
static sd_context_total_t initial_context
  SECTION_ALIGNED( ".data.boot", SD_TTBR_ALIGNMENT_4G );

/**
 * @brief Method to setup short descriptor paging
 *
 * @param max_memory max physical memory of the board
 */
void __bootstrap boot_virt_setup_short( uintptr_t max_memory ) {
  uint32_t x, reg;
  sd_ttbcr_t ttbcr;

  for ( x = 0; x < 4096; x++ ) {
    initial_context.raw[ x ] = 0;
  }

  // map all memory
  for ( x = 0; x < ( max_memory >> 20 ); x++ ) {
    boot_virt_map_short( x << 20, x << 20 );

    if ( 0 < KERNEL_OFFSET ) {
      boot_virt_map_short( x << 20, ( x + ( KERNEL_OFFSET >> 20 ) ) << 20 );
    }
  }

  // Copy page table address to cp15
  __asm__ __volatile__(
    "mcr p15, 0, %0, c2, c0, 0"
    : : "r" ( initial_context.raw )
    : "memory"
  );
  // Set the access control to all-supervisor
  __asm__ __volatile__( "mcr p15, 0, %0, c3, c0, 0" : : "r" ( ~0 ) );

  // read ttbcr register
  __asm__ __volatile__(
    "mrc p15, 0, %0, c2, c0, 2"
    : "=r" ( ttbcr.raw )
    : : "cc"
  );
  // set split to no split
  ttbcr.data.ttbr_split = SD_TTBCR_N_TTBR0_4G;
  ttbcr.data.large_physical_address_extension = 0;
  // push back value with ttbcr
  __asm__ __volatile__(
    "mcr p15, 0, %0, c2, c0, 2"
    : : "r" ( ttbcr.raw )
    : "cc"
  );

  // Get content from control register
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( reg ) : : "cc" );
  // enable mmu by setting bit 0
  reg |= 0x1;
  // push back value with mmu enabled bit set
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( reg ) : "cc" );
}

/**
 * @brief Method to perform map
 *
 * @param max_memory max physical memory of the board
 */
void __bootstrap boot_virt_map_short( uintptr_t phys, uintptr_t virt ) {
  uint32_t x = virt >> 20;
  uint32_t y = phys >> 20;

  initial_context.section[ x ].data.type = SD_TTBR_TYPE_SECTION;
  initial_context.section[ x ].data.execute_never = 0;
  initial_context.section[ x ].data.access_permision_0 = SD_MAC_APX0_FULL_RW;
  initial_context.section[ x ].data.frame = y & 0xFFF;
}
