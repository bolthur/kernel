
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

#include <stdbool.h>
#include <assert.h>
#include <kernel/debug.h>
#include <mm/kernel/kernel/phys.h>
#include <mm/kernel/kernel/virt.h>
#include <mm/kernel/kernel/heap.h>

/**
 * @brief Kernel heap
 */
heap_manager_ptr_t kernel_heap = NULL;

/**
 * @brief Getter for heap initialized flag
 *
 * @return true
 * @return false
 */
bool heap_initialized_get( void ) {
  return NULL != kernel_heap;
}

/**
 * @brief Initialize heap
 */
void heap_init( void ) {
  // assert not initialized
  assert( NULL == kernel_heap );

  // map heap
  for (
    vaddr_t addr = ( vaddr_t )HEAP_START;
    addr < ( vaddr_t )( ( paddr_t )HEAP_START + HEAP_MIN_SIZE );
    addr = ( vaddr_t )( ( paddr_t )addr + PAGE_SIZE )
  ) {
    // Find free page
    paddr_t phys = ( paddr_t )phys_find_free_page( PAGE_SIZE );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "Map 0x%08x to 0x%08x\r\n", phys, addr );
    #endif

    virt_map_address(
      kernel_context, addr, phys, PAGE_FLAG_BUFFERABLE | PAGE_FLAG_CACHEABLE
    );
  }
}
