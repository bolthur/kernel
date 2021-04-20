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

#include <core/entry.h>
#include <core/panic.h>
#include <core/initrd.h>
#include <core/mm/virt.h>
#include <arch/arm/mm/virt.h>
#include <arch/arm/v7/mm/virt/short.h>
#include <arch/arm/v7/mm/virt/long.h>

/**
 * @brief Supported mode
 */
static uint32_t initial_supported_mode __bootstrap_data;

/**
 * @brief Initial setup done flag
 */
static bool initial_setup_done __bootstrap_data = false;

/**
 * @brief Method wraps setup of short / long descriptor mode
 *
 * @todo fix issue within startup setup and remove custom dtb mapping here
 */
void __bootstrap virt_startup_setup( void ) {
  // get paging support from mmfr0
  __asm__ __volatile__(
    "mrc p15, 0, %0, c0, c1, 4"
    : "=r" ( initial_supported_mode )
    : : "cc"
  );

  // strip out everything not needed
  initial_supported_mode &= 0xF;

  // check for invalid paging support
  if (
    ! (
      ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == initial_supported_mode
      || ID_MMFR0_VSMA_V7_PAGING_PXN == initial_supported_mode
      || ID_MMFR0_VSMA_V7_PAGING_LPAE == initial_supported_mode
    )
  ) {
    return;
  }

  // get memory size from mmfr3
  uint32_t reg;
  __asm__ __volatile__(
    "mrc p15, 0, %0, c0, c1, 7"
    : "=r" ( reg )
    : : "cc"
  );
  // get only cpu address bus size
  reg = ( reg >> 24 ) & 0xf;
  // use short if physical address bus is not 36 bit at least
  if ( 0 == reg ) {
    initial_supported_mode = ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS;
  }

  // FIXME: uncomment for testing short descriptor mode
  //initial_supported_mode = ID_MMFR0_VSMA_V7_PAGING_PXN;

  // kick start
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == initial_supported_mode ) {
    v7_long_startup_setup();
  } else {
    v7_short_startup_setup();
  }

  // setup platform related
  virt_startup_platform_setup();

  // enable initial mapping
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == initial_supported_mode ) {
    v7_long_startup_enable();
  } else {
    v7_short_startup_enable();
  }

  // set flag
  initial_setup_done = true;
}

/**
 * @brief Mapper function using short or long descriptor mapping depending on support
 *
 * @param phys physical address
 * @param virt virtual address
 */
void __bootstrap virt_startup_map( uint64_t phys, uintptr_t virt ) {
  // check for invalid paging support
  if (
    ! (
      ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == initial_supported_mode
      || ID_MMFR0_VSMA_V7_PAGING_PXN == initial_supported_mode
      || ID_MMFR0_VSMA_V7_PAGING_LPAE == initial_supported_mode
    )
  ) {
    return;
  }

  // map it
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == initial_supported_mode ) {
    v7_long_startup_map( phys, virt );
  } else {
    v7_short_startup_map( ( uintptr_t )phys, virt );
  }

  if ( initial_setup_done ) {
    virt_startup_flush();
  }
}

/**
 * @brief Flush set context
 */
void __bootstrap virt_startup_flush( void ) {
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == initial_supported_mode ) {
    v7_long_startup_flush();
  } else {
    v7_short_startup_flush();
  }
}

/**
 * @brief Map physical address to virtual one
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param type memory type
 * @param page page attributes
 * @return true
 * @return false
 */
bool virt_map_address(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uint64_t paddr,
  virt_memory_type_t type,
  uint32_t page
) {
  // check context
  if ( ! ctx ) {
    return false;
  }
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_map( ctx, vaddr, paddr, type, page );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_map( ctx, vaddr, paddr, type, page );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Map virtual address with random physical one
 *
 * @param ctx pointer to context
 * @param vaddr virtual address to map
 * @param type memory type
 * @param page page attributes
 * @return true
 * @return false
 */
bool virt_map_address_random(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  virt_memory_type_t type,
  uint32_t page
) {
  // check context
  if ( ! ctx ) {
    return false;
  }
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_map_random( ctx, vaddr, type, page );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_map_random( ctx, vaddr, type, page );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Map a physical address within temporary space
 *
 * @param paddr physical address
 * @param size size to map
 * @return uintptr_t
 */
uintptr_t virt_map_temporary( uint64_t paddr, size_t size ) {
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_map_temporary( paddr, size );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_map_temporary( paddr, size );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief unmap virtual address
 *
 * @param ctx pointer to page context
 * @param addr pointer to virtual address
 * @param free_phys flag to free also physical memory
 * @return true
 * @return false
 */
bool virt_unmap_address( virt_context_ptr_t ctx, uintptr_t addr, bool free_phys ) {
  // check context
  if ( ! ctx ) {
    return false;
  }
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_unmap( ctx, addr, free_phys );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_unmap( ctx, addr, free_phys );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Unmap temporary mapped page again
 *
 * @param addr virtual temporary address
 * @param size size to unmap
 */
void virt_unmap_temporary( uintptr_t addr, size_t size ) {
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    v7_long_unmap_temporary( addr, size );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    v7_short_unmap_temporary( addr, size );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
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
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_create_context( type );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_create_context( type );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @fn virt_context_ptr_t virt_fork_context(virt_context_ptr_t)
 * @brief Fork a virtual context
 * @param ctx context to fork
 * @return
 */
virt_context_ptr_t virt_fork_context( virt_context_ptr_t ctx ) {
  // check context
  if ( ! ctx || ctx->type != VIRT_CONTEXT_TYPE_USER ) {
    return NULL;
  }

  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_fork_context( ctx );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_fork_context( ctx );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Method to destroy virtual context
 *
 * @param ctx
 *
 * @todo add error return, when sub functions have been implemented
 */
bool virt_destroy_context( virt_context_ptr_t ctx ) {
  // check context
  if ( ! ctx ) {
    return false;
  }

  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    v7_long_destroy_context( ctx );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    v7_short_destroy_context( ctx );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }

  return true;
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
  // check context
  if ( ! ctx ) {
    return 0;
  }

  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_create_table( ctx, addr, table );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_create_table( ctx, addr, table );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Method to enable given context
 *
 * @param ctx context structure
 * @return true
 * @return false
 */
bool virt_set_context( virt_context_ptr_t ctx ) {
  // handle invalid context
  if ( ! ctx ) {
    return false;
  }

  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_set_context( ctx );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_set_context( ctx );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Flush set context
 */
void virt_flush_complete( void ) {
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    v7_long_flush_complete();
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    v7_short_flush_complete();
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
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
    ! virt_init_get()
    || ! ctx
    || (
      ctx != kernel_context
      && ctx != user_context
    )
  ) {
    return;
  }

  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    v7_long_flush_address( addr );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    v7_short_flush_address( addr );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Method to prepare temporary area
 *
 * @param ctx context structure
 * @return true
 * @return false
 */
bool virt_prepare_temporary( virt_context_ptr_t ctx ) {
  // check context
  if ( ! ctx ) {
    return false;
  }
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_prepare_temporary( ctx );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_prepare_temporary( ctx );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Method to prepare
 */
void virt_arch_prepare( void ) {
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    v7_long_prepare();
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    v7_short_prepare();
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Method checks whether address is mapped or not without generating exceptions
 *
 * @param ctx
 * @param addr
 * @return true
 * @return false
 */
bool virt_is_mapped_in_context( virt_context_ptr_t ctx, uintptr_t addr ) {
  // check context
  if ( ! ctx ) {
    return false;
  }

  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_is_mapped_in_context( ctx, addr );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_is_mapped_in_context( ctx, addr );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Get mapped physical address
 *
 * @param ctx
 * @param addr
 * @return
 */
uint64_t virt_get_mapped_address_in_context(
  virt_context_ptr_t ctx,
  uintptr_t addr
) {
  // check context
  if ( ! ctx ) {
    return ( uint64_t )-1;
  }

  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_get_mapped_address_in_context( ctx, addr );
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_get_mapped_address_in_context( ctx, addr );
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Get prefetch fault address
 *
 * @return
 */
uintptr_t virt_prefetch_fault_address( void ) {
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_prefetch_fault_address();
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_long_prefetch_fault_address();
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Get prefetch abort status
 *
 * @return
 */
uintptr_t virt_prefetch_status( void ) {
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_prefetch_status();
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_prefetch_status();
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Get data abort status
 *
 * @return
 */
uintptr_t virt_data_fault_address( void ) {
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_data_fault_address();
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_data_fault_address();
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}

/**
 * @brief Get data abort status
 *
 * @return data abort address
 */
uintptr_t virt_data_status( void ) {
  // check for v7 long descriptor format
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_modes ) {
    return v7_long_data_status();
  // check v7 short descriptor format
  } else if (
    ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_modes )
    || ( ID_MMFR0_VSMA_V7_PAGING_PXN == supported_modes )
  ) {
    return v7_short_data_status();
  // Panic when mode is unsupported
  } else {
    PANIC( "Unsupported mode!" )
  }
}
