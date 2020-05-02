
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

#include <stddef.h>
#include <stdbool.h>

#include <assert.h>
#include <core/debug/debug.h>
#include <core/panic.h>
#include <core/entry.h>
#include <core/initrd.h>
#include <core/mm/phys.h>
#include <core/mm/virt.h>

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
  uintptr_t start = 0;
  uintptr_t end = VIRT_2_PHYS( &__kernel_end );
  // round up to page size if necessary
  if ( end % PAGE_SIZE ) {
    end += ( PAGE_SIZE - end % PAGE_SIZE );
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "Map kernel space %p - %p to %p - %p \r\n",
      ( void* )start,
      ( void* )end,
      ( void* )PHYS_2_VIRT( start ),
      ( void* )PHYS_2_VIRT( end )
    );
  #endif

  // map from start to end addresses as used
  while ( start < end ) {
    // map page
    virt_map_address(
      kernel_context,
      PHYS_2_VIRT( start ),
      start,
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_EXECUTABLE
    );

    // get next page
    start += PAGE_SIZE;
  }

  // consider possible initrd
  if ( initrd_exist() ) {
    // set start and end from initrd
    start = initrd_get_start_address();
    end = initrd_get_end_address();

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "Map initrd space %p - %p to %p - %p \r\n",
        ( void* )start,
        ( void* )end,
        ( void* )PHYS_2_VIRT( start ),
        ( void* )PHYS_2_VIRT( end )
      );
    #endif

    // map from start to end addresses as used
    while ( start < end ) {
      // map page
      virt_map_address(
        kernel_context,
        PHYS_2_VIRT( start ),
        start,
        VIRT_MEMORY_TYPE_NORMAL,
        VIRT_PAGE_TYPE_EXECUTABLE
      );

      // get next page
      start += PAGE_SIZE;
    }

    // change initrd location
    initrd_set_start_address(
      PHYS_2_VIRT( initrd_get_start_address() )
    );
  }

  // initialize platform related
  virt_platform_init();
  // prepare temporary area
  virt_prepare_temporary( kernel_context );

  // set kernel context
  virt_set_context( kernel_context );
  // flush contexts to take effect
  virt_flush_complete();

  // set dummy user context
  virt_set_context( user_context );
  // flush contexts to take effect
  virt_flush_complete();

  // post init
  virt_platform_post_init();

  // set static
  virt_initialized = true;
}

/**
 * @brief Get initialized flag
 *
 * @return true virtual memory management has been set up
 * @return false virtual memory management has been not yet set up
 */
bool virt_init_get( void ) {
  return virt_initialized;
}
