
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
#include <arch/arm/mm/virt/short.h>
#include <arch/arm/v6/mm/virt/short.h>

/**
 * @brief
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flag flags used for mapping
 */
void virt_map_address(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uint64_t paddr,
  uint32_t flag
) {
  // check for v6 long descriptor format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    v6_short_map( ctx, vaddr, paddr, flag );
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
 * @param flag flags used for mapping
 */
void virt_map_address_random(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uint32_t flag
) {
  // check for v6 long descriptor format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    v6_short_map_random( ctx, vaddr, flag );
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
 * @param addr virtual address
 */
void virt_flush_complete( uintptr_t addr ) {
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