
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

#include <mm/kernel/kernel/phys.h>
#include <mm/kernel/kernel/placement.h>
#include <mm/kernel/kernel/virt.h>
#include <mm/kernel/arch/arm/virt.h>
#include <mm/kernel/arch/arm/v6/short.h>

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
  vaddr_t vaddr,
  paddr_t paddr,
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
 * @brief unmap virtual address
 *
 * @param ctx pointer to page context
 * @param addr pointer to virtual address
 */
void virt_unmap_address( virt_context_ptr_t ctx, vaddr_t addr ) {
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
 * @return vaddr_t address of context
 */
virt_context_ptr_t virt_create_context( virt_context_type_t type ) {
  #if defined( ELF64 )
    #error "Unsupported architecture"
  #endif

  // Panic when mode is unsupported
  if ( ! ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) ) {
    PANIC( "Unsupported mode!" );
  }

  // variables
  size_t size, alignment;

  // determine size
  size = type == CONTEXT_TYPE_KERNEL
    ? SD_TTBR_SIZE_4G
    : SD_TTBR_SIZE_2G;

  // determine alignment
  alignment = type == CONTEXT_TYPE_KERNEL
    ? SD_TTBR_ALIGNMENT_4G
    : SD_TTBR_ALIGNMENT_2G;

  // create new context
  vaddr_t ctx = PHYS_2_VIRT(
    placement_alloc( size, alignment )
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "type: %d, ctx: 0x%08x\r\n", type, ctx );
  #endif

  // initialize with zero
  memset( ctx, 0, size );

  // create new context structure for return
  virt_context_ptr_t context = PHYS_2_VIRT(
    placement_alloc(
      sizeof( virt_context_t ),
      sizeof( virt_context_t )
    )
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "context: 0x%08x\r\n", context );
  #endif

  // initialize with zero
  memset( context, 0, sizeof( virt_context_t ) );

  // populate type and context
  context->context = ctx;
  context->type = type;

  // return blank context
  return context;
}

/**
 * @brief Method to create table
 *
 * @param ctx context to create table for
 * @param addr address the table is necessary for
 * @param table page table address
 * @return vaddr_t address of table
 */
vaddr_t virt_create_table(
  virt_context_ptr_t ctx,
  vaddr_t addr,
  vaddr_t table
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
void virt_flush_context( void ) {
  // check for v6 format
  if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
    v6_short_flush_context();
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
