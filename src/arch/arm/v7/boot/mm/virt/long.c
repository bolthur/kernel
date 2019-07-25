
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
 * @brief Initial middle directory for context
 */
static ld_middle_page_directory initial_middle_directory[ 4 ]
  __bootstrap_data __aligned( PAGE_SIZE );

/**
 * @brief Initial context
 */
static ld_global_page_directory_t initial_context
  __bootstrap_data __aligned( PAGE_SIZE );

/**
 * @brief Helper to setup initial paging with large page address extension
 *
 * @param max_memory maximum memory to map starting from 0
 */
void __bootstrap boot_virt_setup_long( uintptr_t max_memory ) {
  // variables
  ld_ttbcr_t ttbcr;
  uint32_t reg, x, y;

  // Prepare initial context
  for ( x = 0; x < 512; x++ ) {
    initial_context.raw[ x ] = 0;
  }
  // Prepare middle directories
  for ( x = 0; x < 4; x++ ) {
    for ( y = 0; y < 512; y++ ) {
      initial_middle_directory[ x ].raw[ y ] = 0;
    }
  }

  // Set middle directory tables
  for ( x = 0; x < 4; x++ ) {
    initial_context.table[ x ].data.type = LD_TYPE_TABLE;
    initial_context.table[ x ].raw |= ( uint64_t )(
      ( uintptr_t )initial_middle_directory[ x ].raw
    );
  }

  // Map initial memory
  for ( x = 0; x < ( max_memory >> 21 ); x++ ) {
    boot_virt_map_long( x << 21, x << 21 );

    if ( 0 < KERNEL_OFFSET ) {
      boot_virt_map_long( x << 21, ( x + ( KERNEL_OFFSET >> 21 ) ) << 21 );
    }
  }

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
 *
 * @param phys physical address
 * @param virt virtual address
 */
void __bootstrap boot_virt_map_long( uint64_t phys, uintptr_t virt ) {
  // determine index for getting middle directory
  uint32_t pmd_index = LD_VIRTUAL_PMD_INDEX( virt );
  // determine index for getting table directory
  uint32_t tbl_index = LD_VIRTUAL_TABLE_INDEX( virt );

  // skip if already set
  if ( 0 != initial_middle_directory[ pmd_index ].raw[ tbl_index ] ) {
    return;
  }

  ld_context_block_level2_t* section = &initial_middle_directory[ pmd_index ]
    .section[ tbl_index ];

  // set section
  section->data.type = 0x1;
  section->data.lower_attr_access = 0x1;
  section->raw |= phys;
}
