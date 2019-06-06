
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

#include <mm/kernel/arch/arm/virt.h>
#include <kernel/entry.h>

/**
 * @brief Initial kernel context
 */
static sd_context_total_t initial_kernel_context
  SECTION_ALIGNED( ".data.boot", SD_TTBR_ALIGNMENT_4G );

static sd_context_half_t initial_user_context
  SECTION_ALIGNED( ".data.boot", SD_TTBR_ALIGNMENT_2G );

/**
 * @brief Method to setup short descriptor paging
 */
void SECTION( ".text.boot" ) boot_setup_short_vmm( paddr_t max_memory ) {
  uint32_t x, y, reg;
  sd_ttbcr_t ttbcr;

  for ( x = 0; x < 4096; x++ ) {
    initial_kernel_context.list[ x ] = 0;
  }

  for ( x = 0; x < 2048; x++ ) {
    initial_user_context.list[ x ] = 0;
  }

  // map first gb
  for ( x = 0; x < ( max_memory >> 20 ); x++ ) {
    // offset
    y = x + ( KERNEL_OFFSET >> 20 );

    initial_user_context.section[ x ].data.type = SD_TTBR_TYPE_SECTION;
    initial_user_context.section[ x ].data.execute_never = 0;
    initial_user_context.section[ x ].data.access_permision_0 = SD_MAC_APX0_FULL_RW;
    initial_user_context.section[ x ].data.frame = x & 0xFFF;

    if ( 0 < KERNEL_OFFSET ) {
      initial_kernel_context.section[ y ].data.type = SD_TTBR_TYPE_SECTION;
      initial_kernel_context.section[ y ].data.execute_never = 0;
      initial_kernel_context.section[ y ].data.access_permision_0 = SD_MAC_APX0_FULL_RW;
      initial_kernel_context.section[ y ].data.frame = x & 0xFFF;
    }
  }

  // FIXME: Move part to vendor ?
  #if defined( BCM2709 ) || defined( BCM2710 )
    x = ( 0x40000000 >> 20 );
    initial_user_context.section[ x ].data.type = SD_TTBR_TYPE_SECTION;
    initial_user_context.section[ x ].data.execute_never = 0;
    initial_user_context.section[ x ].data.access_permision_0 = SD_MAC_APX0_FULL_RW;
    initial_user_context.section[ x ].data.frame = x & 0xFFF;
  #endif

  // Copy page table address to cp15
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 0" : : "r" ( initial_user_context.list ) : "memory" );
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 1" : : "r" ( initial_kernel_context.list ) : "memory" );
  // Set the access control to all-supervisor
  __asm__ __volatile__( "mcr p15, 0, %0, c3, c0, 0" : : "r" ( ~0 ) );

  // read ttbcr register
  __asm__ __volatile__( "mrc p15, 0, %0, c2, c0, 2" : "=r" ( ttbcr.raw ) : : "cc" );
  // set split to use ttbr1 and ttbr2 as it will be used later on
  ttbcr.data.ttbr_split = SD_TTBCR_N_TTBR0_2G;
  ttbcr.data.large_physical_address_extension = 0;
  // push back value with ttbcr
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 2" : : "r" ( ttbcr.raw ) : "cc" );

  // Get content from control register
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( reg ) : : "cc" );
  // enable mmu by setting bit 0
  reg |= 0x1;
  // push back value with mmu enabled bit set
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( reg ) : "cc" );
}
