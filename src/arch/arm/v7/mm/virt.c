
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
#include <kernel/debug.h>

#include <kernel/mm/phys.h>
#include <kernel/mm/placement.h>
#include <kernel/mm/virt.h>
#include <arch/arm/mm/virt.h>
#include <arch/arm/v7/mm/virt/short.h>
#include <arch/arm/v7/mm/virt/long.h>

/**
 * @brief Map physical address to virtual one
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
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
    v7_long_map( ctx, vaddr, paddr, type, page );
  // check v7 short descriptor format
  } else if (
    ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
    || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
  ) {
    v7_short_map( ctx, vaddr, paddr, type, page );
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
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
    v7_long_map_random( ctx, vaddr, type, page );
  // check v7 short descriptor format
  } else if (
    ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
    || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
  ) {
    v7_short_map_random( ctx, vaddr, type, page );
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
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
    v7_long_unmap( ctx, addr );
  // check v7 short descriptor format
  } else if (
    ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
    || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
  ) {
    v7_short_unmap( ctx, addr );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Method to create virtual context
 *
 * @param type context type
 * @return virt_context_ptr_t address of context
 */
virt_context_ptr_t virt_create_context( virt_context_type_t type ) {
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
    return v7_long_create_context( type );
  // check v7 short descriptor format
  } else if (
    ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
    || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
  ) {
    return v7_short_create_context( type );
  // Panic when mode is unsupported
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
 * @return uint64_t address of table
 */
uint64_t virt_create_table(
  virt_context_ptr_t ctx,
  uintptr_t addr,
  uint64_t table
) {
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
    return v7_long_create_table( ctx, addr, table );
  // check v7 short descriptor format
  } else if (
    ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
    || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
  ) {
    return v7_short_create_table( ctx, addr, table );
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
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
    v7_long_set_context( ctx );
  // check v7 short descriptor format
  } else if (
    ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
    || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
  ) {
    v7_short_set_context( ctx );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Flush set context
 */
void virt_flush_complete( void ) {
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
    v7_long_flush_complete();
  // check v7 short descriptor format
  } else if (
    ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
    || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
  ) {
    v7_short_flush_complete();
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Flush specific address mapping
 *
 * @param ctx used context
 * @param addr virtual address to flush
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

  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
    v7_long_flush_address( addr );
  // check v7 short descriptor format
  } else if (
    ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
    || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
  ) {
    v7_short_flush_address( addr );
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
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
    v7_long_prepare_temporary( ctx );
  // check v7 short descriptor format
  } else if (
    ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
    || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
  ) {
    v7_short_prepare_temporary( ctx );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}

/**
 * @brief Method to prepare
 */
void virt_arch_prepare( void ) {
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
    v7_long_prepare();
  // check v7 short descriptor format
  } else if (
    ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
    || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
  ) {
    v7_short_prepare();
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" );
  }
}
