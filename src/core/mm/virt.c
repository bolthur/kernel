/**
 * Copyright (C) 2018 - 2021 bolthur project.
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
#if defined( PRINT_MM_VIRT )
  #include <core/debug/debug.h>
#endif
#include <core/panic.h>
#include <core/entry.h>
#include <core/initrd.h>
#include <core/firmware.h>
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
  if ( virt_initialized ) {
    return;
  }

  // set global context to null
  virt_current_kernel_context = NULL;
  virt_current_user_context = NULL;

  // architecture related initialization
  virt_arch_init();

  // determine start and end for kernel mapping
  uintptr_t start = 0;
  uintptr_t end = ROUND_UP_TO_FULL_PAGE( VIRT_2_PHYS( &__kernel_end ) );

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
    assert( virt_map_address(
      virt_current_kernel_context,
      PHYS_2_VIRT( start ),
      start,
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_EXECUTABLE
    ) )

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
      assert( virt_map_address(
        virt_current_kernel_context,
        PHYS_2_VIRT( start ),
        start,
        VIRT_MEMORY_TYPE_NORMAL,
        VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
      ) )

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
  assert( virt_prepare_temporary( virt_current_kernel_context ) )

  // firmware init stuff
  assert( firmware_init() )

  // set kernel context
  assert( virt_set_context( virt_current_kernel_context ) )
  // flush contexts to take effect
  virt_flush_complete();

  // set dummy user context
  assert( virt_set_context( virt_current_user_context ) )
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

/**
 * @brief Method to check for range is mapped in context
 *
 * @param ctx context to use
 * @param address start address of range
 * @param size range size
 * @return true range is completely mapped
 * @return false range is not or incompletely mapped
 */
bool virt_is_mapped_in_context_range(
  virt_context_ptr_t ctx,
  uintptr_t address,
  size_t size
) {
  uintptr_t start = address;
  uintptr_t end = start + size;
  // loop until end
  while ( start < end ) {
    // return false if not mapped
    if ( ! virt_is_mapped_in_context( ctx, start ) ) {
      return false;
    }
    // get next page
    start  += PAGE_SIZE;
  }
  // return success
  return true;
}

/**
 * @brief Method to check for range is mapped
 *
 * @param address start address of range
 * @param size range size
 * @return true range is completely mapped
 * @return false range is not or incompletely mapped
 */
bool virt_is_mapped_range( uintptr_t address, size_t size ) {
  uintptr_t start = address;
  uintptr_t end = start + size;
  // loop until end
  while ( start < end ) {
    // return false if not mapped
    if ( ! virt_is_mapped( start ) ) {
      return false;
    }
    // get next page
    start  += PAGE_SIZE;
  }
  // return success
  return true;
}

/**
 * @brief Method to unmap address range
 *
 * @param ctx context to perform unmap in
 * @param address start address of range
 * @param size range size
 * @param free_phys free physical memory
 * @return true
 * @return false
 */
bool virt_unmap_address_range(
  virt_context_ptr_t ctx,
  uintptr_t address,
  size_t size,
  bool free_phys
) {
  uintptr_t start = address;
  uintptr_t end = start + size;
  // loop until end
  while ( start < end ) {
    // unmap virtual
    if ( ! virt_unmap_address( ctx, start, free_phys ) ) {
      return false;
    }
    // get next page
    start  += PAGE_SIZE;
  }
  return true;
}

/**
 * @brief Find free page range within context
 *
 * @param ctx context to use for lookup
 * @param size necessary amount
 * @param start start address to use
 * @return uintptr_t
 */
uintptr_t virt_find_free_page_range(
  virt_context_ptr_t ctx,
  size_t size,
  uintptr_t start
) {
  // get min and max by context
  uintptr_t min = virt_get_context_min_address( ctx );
  uintptr_t max = virt_get_context_max_address( ctx );

  // handle start
  if (
    0 != start
    && (
      min > start
      || max <= start
      || max <= start + size
    )
  ) {
    return ( uintptr_t )NULL;
  }

  // consider start correctly
  if ( min < start ) {
    min = start;
  }
  // round up to full page
  size= ROUND_UP_TO_FULL_PAGE( size );
  // determine amount of pages
  size_t page_amount = size / PAGE_SIZE;
  size_t found_amount = 0;
  // found address range
  uintptr_t address = 0;
  bool stop = false;

  while ( min <= max && !stop ) {
    // skip if mapped
    if ( virt_is_mapped_in_context( ctx, min ) ) {
      // reset possible found amount and set address
      found_amount = 0;
      address = 0;
      // next page
      min += PAGE_SIZE;
      continue;
    }
    // set address if we start
    if ( 0 == found_amount ) {
      address = min;
    }
    // increase found amount
    found_amount += 1;
    // handle necessary amount reached
    if ( found_amount == page_amount ) {
      stop = true;
    }
    // next page
    min += PAGE_SIZE;
  }

  // return found address or null
  return address;
}

/**
 * @brief Map physical address range to virtual address range
 *
 * @param ctx context
 * @param address virtual start address
 * @param phys physical address
 * @param size size
 * @param type memory type
 * @param page page attributes
 * @return true
 * @return false
 */
bool virt_map_address_range(
  virt_context_ptr_t ctx,
  uintptr_t address,
  uint64_t phys,
  size_t size,
  virt_memory_type_t type,
  uint32_t page
) {
  // mark range as used
  phys_use_page_range( phys, size );
  // determine end
  uintptr_t start = address;
  uintptr_t end = address + size;
  // loop and map
  while ( start < end ) {
    if ( ! virt_map_address( ctx, start, phys, type, page ) ) {
      // free already mapped stuff
      while ( address < end ) {
        virt_unmap_address( ctx, address, true );
        address += PAGE_SIZE;
      }
      return false;
    }
    // next page
    start += PAGE_SIZE;
    phys += PAGE_SIZE;
  }
  return true;
}

/**
 * @brief Map random physical pages to virtual range
 *
 * @param ctx context
 * @param address start address
 * @param size size
 * @param type memory type
 * @param page page attributes
 * @return true
 * @return false
 */
bool virt_map_address_range_random(
  virt_context_ptr_t ctx,
  uintptr_t address,
  size_t size,
  virt_memory_type_t type,
  uint32_t page
) {
  // determine end
  uintptr_t start = address;
  uintptr_t end = start + size;
  // loop and map
  while ( start < end ) {
    // get physical page
    uint64_t phys = phys_find_free_page( PAGE_SIZE );
    // handle error
    if (
      // handle physical error
      0 == phys
      // try to map and handle error
      || ! virt_map_address( ctx, start, phys, type, page )
    ) {
      // free page
      if ( 0 != phys ) {
        phys_free_page( phys );
      }
      // free already mapped stuff
      while ( address < end ) {
        virt_unmap_address( ctx, start, true );
        address += PAGE_SIZE;
      }
      // return error
      return false;
    }
    // next page
    start += PAGE_SIZE;
  }
  return true;
}

/**
 * @brief Get context min address
 *
 * @param ctx
 * @return uintptr_t
 */
uintptr_t virt_get_context_min_address( virt_context_ptr_t ctx ) {
  if ( ctx->type == VIRT_CONTEXT_TYPE_KERNEL ) {
    return KERNEL_AREA_START;
  } else if ( ctx->type == VIRT_CONTEXT_TYPE_USER ) {
    return USER_AREA_START;
  } else {
    PANIC( "Unsupported context type!" )
  }
}

/**
 * @brief Get context max address
 *
 * @param ctx
 * @return uintptr_t
 */
uintptr_t virt_get_context_max_address( virt_context_ptr_t ctx ) {
  if ( ctx->type == VIRT_CONTEXT_TYPE_KERNEL ) {
    return KERNEL_AREA_END;
  } else if ( ctx->type == VIRT_CONTEXT_TYPE_USER ) {
    return USER_AREA_END;
  } else {
    PANIC( "Unsupported context type!" )
  }
}
