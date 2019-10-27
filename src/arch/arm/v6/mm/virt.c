
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

#include <string.h>

#include <kernel/entry.h>
#include <kernel/panic.h>
#include <kernel/debug/debug.h>

#include <kernel/mm/phys.h>
#include <kernel/mm/placement.h>
#include <kernel/mm/virt.h>
#include <arch/arm/mm/virt.h>
#include <arch/arm/mm/virt/short.h>
#include <arch/arm/v6/mm/virt/short.h>

/**
 * @brief
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param type memory type
 * @param page page attributes
 */
void virt_map_address(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uint64_t paddr,
  virt_memory_type_t type,
  uint32_t page
) {
  // check for v6 long descriptor format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    v6_short_map( ctx, vaddr, paddr, type, page );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Map virtual address with random physical one
 *
 * @param ctx pointer to context
 * @param vaddr virtual address to map
 * @param type memory type
 * @param page page attributes
 */
void virt_map_address_random(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  virt_memory_type_t type,
  uint32_t page
) {
  // check for v6 long descriptor format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    v6_short_map_random( ctx, vaddr, type, page );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Map a physical address within temporary space
 *
 * @param paddr physicall address
 * @param size size to map
 * @return uintptr_t
 */
uintptr_t virt_map_temporary( uint64_t paddr, size_t size ) {
  // check for v6 short descriptor format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    return v6_short_map_temporary( paddr, size );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief unmap virtual address
 *
 * @param ctx pointer to page context
 * @param addr pointer to virtual address
 */
void virt_unmap_address( virt_context_ptr_t ctx, uintptr_t addr ) {
  // check for v6 long descriptor format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    v6_short_unmap( ctx, addr );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Unmap temporary mapped page again
 *
 * @param addr virtual temporary address
 * @param size size to unmap
 */
void virt_unmap_temporary( uintptr_t addr, size_t size ) {
  // check for v6 short descriptor format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    v6_short_unmap_temporary( addr, size );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Method to create virtual context
 *
 * @param type context type
 * @return uintptr_t address of context
 */
virt_context_ptr_t virt_create_context( virt_context_type_t type ) {
  // Panic when mode is unsupported
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    return v6_short_create_context( type );
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Method to create table
 *
 * @param ctx context to create table for
 * @param addr address the table is necessary for
 * @param table page table address
 * @return uintptr_t address of table
 */
uint64_t virt_create_table(
  virt_context_ptr_t ctx,
  uintptr_t addr,
  uint64_t table
) {
  // check for v6 format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    return v6_short_create_table( ctx, addr, table );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Method to enable given context
 *
 * @param ctx context structure
 */
void virt_set_context( virt_context_ptr_t ctx ) {
  // check for v6 format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    v6_short_set_context( ctx );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Flush set context
 */
void virt_flush_complete( void ) {
  // check for v6 format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    v6_short_flush_complete();
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Flush address
 *
 * @param ctx used context
 * @param addr virtual address
 */
void virt_flush_address( virt_context_ptr_t ctx, uintptr_t addr ) {
  // no flush if not initialized or context currently not active
  if (
    ! virt_initialized_get()
    || (
      ctx != kernel_context
      && ctx != user_context
    )
  ) {
    return;
  }

  // check for v6 format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    v6_short_flush_address( addr );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Method to prepare temporary area
 *
 * @param ctx context structure
 */
void virt_prepare_temporary( virt_context_ptr_t ctx ) {
  // check for v6 format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    v6_short_prepare_temporary( ctx );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Method to prepare
 */
void virt_arch_prepare( void ) {
  // Panic when mode is unsupported
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    return v6_short_prepare();
  } else {
    PANIC( "Unsupported mode!" );
  }
}
