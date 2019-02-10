
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
#include "kernel/kernel/entry.h"
#include "kernel/kernel/panic.h"
#include "kernel/kernel/mm/phys.h"
#include "kernel/kernel/mm/virt.h"
#include "kernel/arch/arm/mm/virt.h"

/**
 * @brief Initialize virtual memory management
 */
void virt_init( void ) {
  // Get page directory
  void *pdir = phys_find_free_page_range(
    VSMA_SHORT_PAGE_DIRECTORY_SIZE,
    VSMA_SHORT_PAGE_DIRECTORY_ALIGNMENT
  );
  void *ptable = phys_find_free_page_range(
    VSMA_SHORT_PAGE_TABLE_SIZE,
    VSMA_SHORT_PAGE_TABLE_ALIGNMENT
  );

  // Debug output
  #if defined( PRINT_MM_VIRT )
    printf( "pdir: 0x%08x\r\n", pdir );
    printf( "ptable: 0x%08x\r\n", ptable );
  #endif

  // Transform to virtual
  pdir = ( void* )PHYS_2_VIRT( pdir );
  ptable = ( void* )PHYS_2_VIRT( ptable );

  // Initialize with zero
  memset( pdir, 0, VSMA_SHORT_PAGE_DIRECTORY_SIZE );

  // Debug output
  #if defined( PRINT_MM_VIRT )
    printf( "pdir: 0x%08x\r\n", pdir );
    printf( "ptable: 0x%08x\r\n", ptable );
  #endif
}
