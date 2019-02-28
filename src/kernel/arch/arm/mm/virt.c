
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
#include "kernel/kernel/panic.h"
#include "kernel/kernel/entry.h"
#include "kernel/kernel/mm/phys.h"
#include "kernel/kernel/mm/virt.h"
#include "kernel/arch/arm/mm/virt.h"

/**
 * @brief
 *
 * @param context pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flags flags used for mapping
 */
void virt_map_address( vaddr_t context, vaddr_t vaddr, paddr_t paddr, uint32_t flags ) {
  // subtract split offset
  if ( ( uint32_t )vaddr > SD_TTBR1_START_TTBR0_2G ) {
    vaddr = ( vaddr_t )( ( uint32_t )vaddr - SD_TTBR1_START_TTBR0_2G );
  }

  ASSERT( 0 == ( uint32_t )vaddr % SD_PAGE_SIZE );
  ASSERT( 0 == ( uint32_t )paddr % SD_PAGE_SIZE );

  // Get table and page
  uint32_t table = SD_VIRTUAL_TO_TABLE( vaddr );
  uint32_t page = SD_VIRTUAL_TO_PAGE( vaddr );

  // transform context into structure
  sd_context_half_t* ctx = ( sd_context_half_t* )context;

  ( void )table;
  ( void )page;
  ( void )ctx;

  ( void )paddr;
  ( void )flags;
}

/**
 * @brief Method to create virtual context
 *
 * @param type context type
 * @return vaddr_t address of context
 */
vaddr_t virt_create_context( virt_context_type_t type ) {
  ( void )type;
  return NULL;
}

/**
 * @brief Method to create table
 *
 * @return vaddr_t address of table
 */
vaddr_t virt_create_table( void ) {
  return NULL;
}

/**
 * @brief Method to get supported modes
 *
 * @return uint32_t supported modes
 */
uint32_t virt_get_supported_modes( void ) {
  #if defined( ELF32 )
    // variable for return
    uint32_t reg;

    // get paging support from mmfr0
    __asm__ __volatile__( "mrc p15, 0, %0, c0, c1, 4" : "=r" ( reg ) : : "cc" );

    // strip out everything not needed
    reg &= 0xF;
  #endif

  // return supported
  return reg;
}
