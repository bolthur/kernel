
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

#include <kernel/mm/phys.h>
#include <arch/arm/mm/virt/long.h>
#include <arch/arm/v7/boot/mm/virt/long.h>
#include <kernel/entry.h>

/**
 * @brief Initial kernel context
 */
static ld_global_page_directory_t initial_context
  SECTION_ALIGNED( ".data.boot", PAGE_SIZE );

static ld_middle_page_directory inital_middle_directory[ 4 ]
  SECTION_ALIGNED( ".data.boot", PAGE_SIZE );

/**
 * @brief Helper to setup initial paging with large page address extension
 *
 * @todo Add initial mapping for paging with lpae
 * @todo Remove call for setup short paging
 */
void SECTION( ".text.boot" )
boot_virt_setup_long( uintptr_t max_memory ) {
  // variables
  ld_ttbcr_t ttbcr;
  uint32_t reg, x, y;

  // Prepare initial context
  for ( x = 0; x < 512; x++ ) {
    initial_context.raw[ x ] = 0ULL;
  }
  // Prepare middle directories
  for ( x = 0; x < 4; x++ ) {
    for ( y = 0; y < 512; y++ ) {
      inital_middle_directory[ x ].raw[ y ] = 0ULL;
    }
  }

  // Set middle directory tables
  for ( x = 0; x < 4; x++ ) {
    // set next level table
    initial_context.table[ x ].raw =
      ( ( uintptr_t )inital_middle_directory[ x ].raw & 0xFFFFF000 )
      | LD_TYPE_TABLE;
  }

  // Map initial memory
  for ( x = 0; x < ( max_memory >> 20 ); x++ ) {
    boot_virt_map_long( x << 20, x << 20 );

    if ( 0 < KERNEL_OFFSET ) {
      boot_virt_map_long( x << 20, ( x + ( KERNEL_OFFSET >> 20 ) ) << 20 );
    }
  }

  initial_context.section[ 0 ].raw = 0;
  initial_context.section[ 0 ].data.type = 0x1;
  initial_context.section[ 0 ].data.lower_attribute = 0x1C7;
  initial_context.section[ 1 ].raw = 0;
  initial_context.section[ 1 ].data.type = 0x1;
  initial_context.section[ 1 ].data.lower_attribute = 0x1C7;
  initial_context.section[ 2 ].raw = 0;
  initial_context.section[ 2 ].data.type = 0x1;
  initial_context.section[ 2 ].data.lower_attribute = 0x1C7;
  initial_context.section[ 3 ].raw = 0;
  initial_context.section[ 3 ].data.type = 0x1;
  initial_context.section[ 3 ].data.lower_attribute = 0x1C7;

  // Set initial context
  __asm__ __volatile__(
    "mcrr p15, 0, %0, %1, c2"
    : : "r" ( initial_context.raw ), "r" ( 0 )
    : "memory"
  );

  // prepare ttbcr
  ttbcr.raw = 0;
  // set large physical address extension bit
  ttbcr.data.large_physical_address_extension = 1;
  // push value to ttbcr
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
 * @brief Method to perform identity nap
 */
void SECTION( ".text.boot" )
boot_virt_map_long( uintptr_t phys, uintptr_t virt ) {
  // determine index for getting middle directory
  uint32_t pmd_index = LD_VIRTUAL_PMD_INDEX( virt );
  // determine index for getting table directory
  uint32_t tbl_index = LD_VIRTUAL_TABLE_INDEX( virt );

  // get middle directory
  ld_middle_page_directory* next_level = ( ld_middle_page_directory* )
    ( ( uintptr_t )initial_context.table[ pmd_index ].data.next_level_table );

  // skip if already set
  if ( 0 != next_level[ tbl_index ].raw ) {
    return;
  }

  // FIXME: Set section
  ( void )phys;
  /*next_level[ tbl_index ].section[ tbl_index ].raw =
    ( phys & 0xFFFFF000 ) | 0x0000074D;*/
}
