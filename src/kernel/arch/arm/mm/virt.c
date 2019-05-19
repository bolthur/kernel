
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

#include "lib/stdio.h"
#include "lib/string.h"
#include "kernel/kernel/debug.h"
#include "kernel/kernel/entry.h"
#include "kernel/kernel/panic.h"
#include "kernel/kernel/mm/phys.h"
#include "kernel/kernel/mm/placement.h"
#include "kernel/kernel/mm/virt.h"
#include "kernel/arch/arm/mm/virt.h"

/**
 * @brief Supported modes
 */
static uint32_t supported_modes;

/**
 * @brief user context
 */
vaddr_t user_context;

/**
 * @brief kernel context
 */
vaddr_t kernel_context;

/**
 * @brief Internal v6 mapping function
 *
 * @param context pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flags flags used for mapping
 */
static void v6_map(
  vaddr_t context,
  vaddr_t vaddr,
  paddr_t paddr,
  uint32_t flags
) {
  ( void )context;
  ( void )vaddr;
  ( void )paddr;
  ( void )flags;

  PANIC( "v6 mmu mapping not yet supported!" );
}

/**
 * @brief Internal v7 short descriptor mapping function
 *
 * @param context pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flags flags used for mapping
 */
static void v7_short_map(
  vaddr_t context,
  vaddr_t vaddr,
  paddr_t paddr,
  uint32_t flags
) {
  ( void )context;
  ( void )vaddr;
  ( void )paddr;
  ( void )flags;

  PANIC( "v7 mmu short descriptor mapping not yet supported!" );
}

/**
 * @brief Internal v7 long descriptor mapping function
 *
 * @param context pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flags flags used for mapping
 */
static void v7_long_map(
  vaddr_t context,
  vaddr_t vaddr,
  paddr_t paddr,
  uint32_t flags
) {
  ( void )context;
  ( void )vaddr;
  ( void )paddr;
  ( void )flags;

  PANIC( "v7 mmu long descriptor mapping not yet supported!" );
}

/**
 * @brief Internal v6 unmapping function
 *
 * @param context pointer to page context
 * @param vaddr pointer to virtual address
 */
static void v6_unmap( vaddr_t context, vaddr_t vaddr ) {
  ( void )context;
  ( void )vaddr;

  PANIC( "v6 mmu mapping not yet supported!" );
}

/**
 * @brief Internal v7 short descriptor unmapping function
 *
 * @param context pointer to page context
 * @param vaddr pointer to virtual address
 */
static void v7_short_unmap( vaddr_t context, vaddr_t vaddr ) {
  ( void )context;
  ( void )vaddr;

  PANIC( "v7 mmu short descriptor mapping not yet supported!" );
}

/**
 * @brief Internal v7 long descriptor unmapping function
 *
 * @param context pointer to page context
 * @param vaddr pointer to virtual address
 */
static void v7_long_unmap( vaddr_t context, vaddr_t vaddr ) {
  ( void )context;
  ( void )vaddr;

  PANIC( "v7 mmu long descriptor unmapping not yet supported!" );
}

/**
 * @brief
 *
 * @param context pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flags flags used for mapping
 */
void virt_map_address(
  vaddr_t context,
  vaddr_t vaddr,
  paddr_t paddr,
  uint32_t flags
) {
  #if defined( ELF32 )
    ASSERT(
      ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes
      || ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
      || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
      || ID_MMFR0_VSMA_V6_PAGING & supported_modes
    );

    // check for v7 long descriptor format
    if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
      v7_long_map( context, vaddr, paddr, flags );
    // check v7 short descriptor format
    } else if (
      ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
      || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
    ) {
      v7_short_map( context, vaddr, paddr, flags );
    // check for old v6 paging
    } else if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
      v6_map( context, vaddr, paddr, flags );
    }
  #elif defined( ELF64 )
    #error
  #endif
}

/**
 * @brief unmap virtual address
 *
 * @param context pointer to page context
 * @param vaddr pointer to virtual address
 */
void virt_unmap_address( vaddr_t context, vaddr_t vaddr ) {
  #if defined( ELF32 )
    ASSERT(
      ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes
      || ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
      || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
      || ID_MMFR0_VSMA_V6_PAGING & supported_modes
    );

    // check for v7 long descriptor format
    if ( ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
      v7_long_unmap( context, vaddr );
    // check v7 short descriptor format
    } else if (
      ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes
      || ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes
    ) {
      v7_short_unmap( context, vaddr );
    // check for old v6 paging
    } else if ( ID_MMFR0_VSMA_V6_PAGING & supported_modes ) {
      v6_unmap( context, vaddr );
    }
  #elif defined( ELF64 )
    #error
  #endif
}

/**
 * @brief Method to create virtual context
 *
 * @param type context type
 * @return vaddr_t address of context
 */
vaddr_t virt_create_context( virt_context_type_t type ) {
  ( void )type;
  return NULL;
}

/**
 * @brief Method to create table
 *
 * @return vaddr_t address of table
 */
vaddr_t virt_create_table( void ) {
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
 * @brief Method to prepare virtual memory management by architecture
 */
void virt_arch_init( void ) {
  // setup supported mode global
  virt_setup_supported_modes();

  // create new kernel context
  kernel_context = ( vaddr_t )PHYS_2_VIRT(
    placement_alloc( SD_TTBR_SIZE_4G, SD_TTBR_ALIGNMENT_4G )
  );

  // create temporary user context
  user_context = ( vaddr_t )PHYS_2_VIRT(
    placement_alloc( SD_TTBR_SIZE_2G, SD_TTBR_ALIGNMENT_2G )
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "kernel_context: 0x%08x\r\n", kernel_context );
    DEBUG_OUTPUT( "user_context: 0x%08x\r\n", user_context );
  #endif

  // initialize with zero
  memset( kernel_context, 0, SD_TTBR_SIZE_4G );
  memset( user_context, 0, SD_TTBR_SIZE_2G );
}
