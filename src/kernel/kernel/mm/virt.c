
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

#include <stddef.h>
#include <stdbool.h>

#include <assert.h>
#include <kernel/debug.h>
#include <kernel/entry.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/placement.h>
#include <kernel/mm/virt.h>

/**
 * @brief static initialized flag
 */
static bool virt_initialized = false;

/**
 * @brief Generic initialization of virtual memory manager
 */
void virt_init( void ) {
  // assert no initialize
  assert( true != virt_initialized );

  // set global context to null
  kernel_context = NULL;
  user_context = NULL;

  // architecture related initialization
  virt_arch_init();

  // determine start and end for kernel mapping
  paddr_t start = 0;
  paddr_t end = placement_address;

  // round up to page size if necessary
  if ( end % PAGE_SIZE ) {
    end += ( PAGE_SIZE - placement_address % PAGE_SIZE );
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "Map kernel space 0x%08x - 0x%08x to 0x%08x - 0x%08x \r\n",
      start,
      end,
      PHYS_2_VIRT( start ),
      PHYS_2_VIRT( end )
    );
  #endif

  // map from start to end addresses as used
  while( start < end ) {
    // map page
    virt_map_address(
      kernel_context,
      PHYS_2_VIRT( start ),
      start,
      PAGE_FLAG_BUFFERABLE | PAGE_FLAG_CACHEABLE
    );

    // get next page
    start += PAGE_SIZE;
  }

  // initialize vendor init
  virt_vendor_init();

  // prepare temporary area
  virt_prepare_temporary( kernel_context );

  // set contexts
  virt_set_context( kernel_context );
  virt_set_context( user_context );

  // flush contexts to take effect
  virt_flush_context();

  // post init
  virt_vendor_post_init();

  // set static
  virt_initialized = true;
}

/**
 * @brief Get initialized flag
 *
 * @return true virtual memory management has been set up
 * @return false virtual memory management has been not yet set up
 */
bool virt_initialized_get( void ) {
  return virt_initialized;
}