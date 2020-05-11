
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#include <core/mm/phys.h>
#include <arch/arm/mm/virt/short.h>
#include <arch/arm/v6/boot/mm/virt/short.h>
#include <core/entry.h>

/**
 * @brief Initial kernel context
 */
static sd_context_total_t initial_context
  __bootstrap_data __aligned( SD_TTBR_ALIGNMENT_4G );

/**
 * @brief Method to setup short descriptor paging
 */
void __bootstrap boot_virt_setup_short( void ) {
  uint32_t x;
  sd_ttbcr_t ttbcr;

  for ( x = 0; x < 4096; x++ ) {
    initial_context.raw[ x ] = 0;
  }

  // determine max
  uintptr_t max = VIRT_2_PHYS( &__kernel_end );
  // round up to page size if necessary
  if ( max % PAGE_SIZE ) {
    max += ( PAGE_SIZE - max % PAGE_SIZE );
  }
  // shift max
  max >>= 20;
  // minimum is 1
  if ( 0 == max ) {
    max = 1;
  }

  // map all memory
  for ( x = 0; x < max; x++ ) {
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
  // push back value with ttbcr
  __asm__ __volatile__(
    "mcr p15, 0, %0, c2, c0, 2"
    : : "r" ( ttbcr.raw )
    : "cc"
  );
}

/**
 * @brief Method to perform map
 *
 * @param phys physical address
 * @param virt virtual address
 */
void __bootstrap boot_virt_map_short( uintptr_t phys, uintptr_t virt ) {
  uint32_t x = virt >> 20;
  uint32_t y = phys >> 20;

  sd_context_section_ptr_t sec = &initial_context.section[ x ];
  sec->data.type = SD_TTBR_TYPE_SECTION;
  sec->data.execute_never = 0;
  sec->data.access_permision_0 = SD_MAC_APX0_PRIVILEGED_RW;
  sec->data.frame = y & 0xFFF;
}

/**
 * @brief Method to enable initial virtual memory
 */
void __bootstrap boot_virt_enable_short( void ) {
  uint32_t reg;
  // Get content from control register
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( reg ) : : "cc" );
  // enable mmu by setting bit 0
  reg |= 1;
  // push back value with mmu enabled bit set
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( reg ) : "cc" );
}
