
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
#include "kernel/kernel/mm/phys.h"
#include "kernel/kernel/mm/virt.h"
#include "kernel/arch/arm/mm/virt.h"

/**
 * @brief
 *
 * @param page_directory pointer to page directory
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flags flags used for mapping
 */
void virt_map_address( void* page_directory, void* vaddr, void* paddr, uint32_t flags ) {
  uint32_t vaddr_off = ( uint32_t )vaddr;

  // subtract offset
  if ( TTBR1_FIRST_ADDRESS <= vaddr_off ) {
    vaddr_off = vaddr_off - TTBR1_FIRST_ADDRESS;
  }

  // determine page table and page index
  uint32_t table_idx = ( ( uint32_t )vaddr_off >> 20 );
  uint32_t page_idx = ( ( uint32_t )vaddr_off >> 12 ) & 0xFF;
  uint32_t *dir = ( uint32_t* )page_directory;

  if ( 0 == dir[ table_idx ] ) {
    // allocate physical page
    uint32_t* ptable = ( uint32_t* )phys_find_free_page( VSMA_SHORT_PAGE_TABLE_SIZE );

    #if defined( PRINT_MM_VIRT )
      //printf( "ptable: 0x%08x\ttbl: 0x%08x\r\n", ptable, dir[ table_idx ] );
    #endif

    // initialize with zero
    memset( ptable, 0, VSMA_SHORT_PAGE_TABLE_SIZE );

    // set table address
    dir[ table_idx ] = ( ( uint32_t )ptable & 0xFFFFFC00 ) | TTBR_L1_IS_PAGETABLE;
  }

  uint32_t *tbl = ( uint32_t* )( ( uint32_t )dir[ table_idx ] & 0xFFFFFC00 );

  ( void )flags;
  // tbl[ page_idx ] = ( ( uint32_t ) paddr & 0xFFFFF000 ) | ( 0xFF0 ) | flags | TTBR_L1_IS_SECTION;
  tbl[ page_idx ] = ( ( uint32_t ) paddr & 0xFFFFF000 ) | 0x10;

  //printf( "%i\t%i\t0x%08x\r\n", table_idx, page_idx, tbl );

  // calculate address of table
  /*uint32_t *tbl = ( uint32_t* )( ( uint32_t )page_directory | table_idx );
  uint32_t *page;

  #if defined( PRINT_MM_VIRT )
    printf( "vaddr: 0x%08x\tpaddr: 0x%08x\r\n", vaddr, paddr );
  #endif

  if ( 0 == *tbl ) {
    // allocate physical page
    uint32_t* ptable = ( uint32_t* )phys_find_free_page( VSMA_SHORT_PAGE_TABLE_SIZE );

    #if defined( PRINT_MM_VIRT )
      printf( "ptable: 0x%08x\ttbl: 0x%08x\r\n", ptable, tbl );
    #endif

    // initialize with zero
    memset( ptable, 0, VSMA_SHORT_PAGE_TABLE_SIZE );

    // set table address
    *tbl = ( ( uint32_t )ptable & 0xFFFFFC00 ) | TTBR_L1_IS_PAGETABLE;
  }

  // determine page
  page = ( uint32_t* )( ( *tbl & 0xFFFFFC00 ) | ( page_idx << 2 ) );

  #if defined( PRINT_MM_VIRT )
    printf( "page: 0x%08x\r\n", page );
  #endif

  // write physical page addr to entry
  *page = ( ( uint32_t ) paddr & 0xFFFFF000 ) | ( 0xFF0 ) | flags | TTBR_L1_IS_SECTION;

  #if defined( PRINT_MM_VIRT )
    printf( "tbl: 0x%08x\tpage: 0x%08x\t*page = 0x%08x\r\n", tbl, page, *page );
  #endif*/
}
