
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

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <kernel/debug.h>
#include <kernel/entry.h>
#include <kernel/panic.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/placement.h>
#include <kernel/mm/virt.h>
#include <arch/arm/mm/virt.h>

#include <arch/arm/mm/v6/short.h>
#include <arch/arm/mm/v7/long.h>
#include <arch/arm/mm/v7/short.h>

/**
 * @brief Supported modes
 */
static uint32_t supported_modes;

/**
 * @brief user context
 */
virt_context_ptr_t user_context;

/**
 * @brief kernel context
 */
virt_context_ptr_t kernel_context;

/**
 * @brief
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flags flags used for mapping
 */
void virt_map_address(
  virt_context_ptr_t ctx, vaddr_t vaddr, paddr_t paddr, uint32_t flags
) {
  #if defined( ELF32 )
    // check for v7 long descriptor format
    if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
      v7_long_map( ctx, vaddr, paddr, flags );
    // check v7 short descriptor format
    } else if (
      ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
      || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
    ) {
      v7_short_map( ctx, vaddr, paddr, flags );
    // check for old v6 paging
    } else if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
      v6_short_map( ctx, vaddr, paddr, flags );
    }
  #elif defined( ELF64 )
    #error "Unsupported"
  #endif
}

/**
 * @brief unmap virtual address
 *
 * @param ctx pointer to page context
 * @param addr pointer to virtual address
 */
void virt_unmap_address( virt_context_ptr_t ctx, vaddr_t addr ) {
  #if defined( ELF32 )
    // check for v7 long descriptor format
    if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
      v7_long_unmap( ctx, addr );
    // check v7 short descriptor format
    } else if (
      ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
      || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
    ) {
      v7_short_unmap( ctx, addr );
    // check for old v6 paging
    } else if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
      v6_short_unmap( ctx, addr );
    }
  #elif defined( ELF64 )
    #error "Unsupported"
  #endif
}

/**
 * @brief Method to create virtual context
 *
 * @param type context type
 * @return vaddr_t address of context
 */
virt_context_ptr_t virt_create_context( virt_context_type_t type ) {
  // different handling, when initial setup is done not yet added
  if ( true != virt_use_physical_table ) {
    return NULL;
  }

  #if defined( ELF32 )
    if ( ! (
        ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes
        || ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
        || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
        || ID_MMFR0_VSMA_V6_PAGING & supported_modes
      )
    ) {
      return NULL;
    }
  #elif defined( ELF64 )
    #error "Unsupported"
  #endif

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
 * @return vaddr_t address of table
 */
vaddr_t virt_create_table( virt_context_ptr_t ctx, vaddr_t addr ) {
  #if defined( ELF32 )
    // check for v7 long descriptor format
    if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
      return v7_long_create_table( ctx, addr );
    // check v7 short descriptor format
    } else if (
      ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
      || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
    ) {
      return v7_short_create_table( ctx, addr );
    // check for old v6 paging
    } else if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
      return v6_short_create_table( ctx, addr );
    }
  #elif defined( ELF64 )
    #error "Unsupported"
  #endif

  return NULL;
}

/**
 * @brief Method to get supported modes
 *
 * @return uint32_t supported modes
 */
void virt_setup_supported_modes( void ) {
  // variable for return
  uint32_t reg = 0;

  #if defined( ELF32 )
    // get paging support from mmfr0
    __asm__ __volatile__( "mrc p15, 0, %0, c0, c1, 4" : "=r" ( reg ) : : "cc" );

    // strip out everything not needed
    reg &= 0xF;
  #elif defined( ELF64 )
    #error "Unsupported"
  #endif

  // set supported modes
  supported_modes = reg;
}

/**
 * @brief Method to enable given context
 *
 * @param ctx context structure
 */
void virt_activate_context( virt_context_ptr_t ctx ) {
  // mark parameter as unused
  ( void )ctx;

  #if defined( ELF32 )
    // check for v7 long descriptor format
    if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
      PANIC( "Enable v7 long descriptor context not yet supported!" );
    // check v7 short descriptor format
    } else if (
      ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
      || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
    ) {
      PANIC( "Enable v7 short descriptor context not yet supported!" );
    // check for old v6 paging
    } else if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
      PANIC( "Enable v6 context not yet supported!" );
    }
  #elif defined( ELF64 )
    #error "Unsupported"
  #endif
}

/**
 * @brief Method to prepare virtual memory management by architecture
 */
void virt_arch_init( void ) {
  // setup supported mode global
  virt_setup_supported_modes();

  // create a kernel context
  kernel_context = virt_create_context( CONTEXT_TYPE_KERNEL );
  assert( NULL != kernel_context );

  // create a dummy user context for all cores
  user_context = virt_create_context( CONTEXT_TYPE_USER );
  assert( NULL != user_context );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "kernel_context: 0x%08x\r\n", kernel_context );
    DEBUG_OUTPUT( "user_context: 0x%08x\r\n", user_context );
  #endif
}
