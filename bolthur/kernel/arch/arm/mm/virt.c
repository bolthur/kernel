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

#include <assert.h>
#if defined( PRINT_MM_VIRT )
  #include <debug/debug.h>
#endif
#include <entry.h>
#include <panic.h>
#include <mm/phys.h>
#include <mm/virt.h>
#include <arch/arm/mm/virt.h>

/**
 * @brief Supported mode
 */
uint32_t virt_startup_supported_mode __bootstrap_data;

/**
 * @brief Supported modes
 */
uint32_t virt_supported_mode;

/**
 * @brief user context
 */
virt_context_ptr_t virt_current_user_context;

/**
 * @brief kernel context
 */
virt_context_ptr_t virt_current_kernel_context;

/**
 * @brief Method to get supported modes
 */
void __bootstrap virt_startup_setup_supported_modes( void ) {
  #if defined( ELF32 )
    // get paging support from mmfr0
    __asm__ __volatile__(
      "mrc p15, 0, %0, c0, c1, 4"
      : "=r" ( virt_startup_supported_mode )
      : : "cc"
    );

    // strip out everything not needed
    virt_startup_supported_mode &= 0xF;

    // check for invalid paging support
    if (
      ! (
        ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == virt_startup_supported_mode
        || ID_MMFR0_VSMA_V7_PAGING_PXN == virt_startup_supported_mode
        || ID_MMFR0_VSMA_V7_PAGING_LPAE == virt_startup_supported_mode
      )
    ) {
      virt_startup_supported_mode = 0;
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
      virt_startup_supported_mode = ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS;
    }

    // FIXME: uncomment for testing short descriptor mode
    //virt_startup_supported_mode = ID_MMFR0_VSMA_V7_PAGING_PXN;
  #elif defined( ELF64 )
    #error "Unsupported"
  #endif
}

/**
 * @brief Method to get supported modes
 */
void virt_setup_supported_modes( void ) {
  #if defined( ELF32 )
    // variable for return
    uint32_t reg = 0;

    // get paging support from mmfr0
    __asm__ __volatile__(
      "mrc p15, 0, %0, c0, c1, 4"
      : "=r" ( reg )
      : : "cc"
    );

    // save supported modes
    virt_supported_mode = reg & 0xF;

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "reg = %#08x, virt_supported_mode = %#08x\r\n",
        reg, virt_supported_mode )
    #endif

    // get memory size from mmfr3
    __asm__ __volatile__(
      "mrc p15, 0, %0, c0, c1, 7"
      : "=r" ( reg )
      : : "cc"
    );

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "reg = %#08x\r\n", reg )
    #endif

    // get only cpu address bus size
    reg = ( reg >> 24 ) & 0xf;

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "reg = %#08x\r\n", reg )
    #endif

    // set paging to v7 short descriptor if more
    // than 32 bit physical addresses arent supported
    if ( 0 == reg && ( ID_MMFR0_VSMA_V7_PAGING_LPAE & virt_supported_mode ) ) {
      if ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & virt_supported_mode ) {
        virt_supported_mode = ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS;
      } else if ( ID_MMFR0_VSMA_V7_PAGING_PXN & virt_supported_mode ) {
        virt_supported_mode = ID_MMFR0_VSMA_V7_PAGING_PXN;
      } else {
        PANIC( "Not supported!" )
      }
    }

    // FIXME: uncomment for testing short descriptor mode
    //virt_supported_mode = ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS;
  #elif defined( ELF64 )
    #error "Unsupported"
  #endif
}

/**
 * @fn void virt_arch_init(void)
 * @brief Method to prepare virtual memory management by architecture
 *
 * @todo move calls to virt_create_context into core init
 */
void virt_arch_init( void ) {
  // setup supported mode global
  virt_setup_supported_modes();

  // prepare
  virt_arch_prepare();

  // create a kernel context
  virt_current_kernel_context = virt_create_context( VIRT_CONTEXT_TYPE_KERNEL );
  assert( virt_current_kernel_context )

  // create a dummy user context for all cores
  virt_current_user_context = virt_create_context( VIRT_CONTEXT_TYPE_USER );
  assert( virt_current_user_context )

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "virt_current_kernel_context: %p\r\n", ( void* )virt_current_kernel_context )
    DEBUG_OUTPUT( "virt_current_user_context: %p\r\n", ( void* )virt_current_user_context )
  #endif
}

/**
 * @brief Method checks whether address is mapped or not without generating exceptions
 *
 * @param addr
 * @return true
 * @return false
 */
bool virt_is_mapped( uintptr_t addr ) {
  return
    virt_is_mapped_in_context( virt_current_kernel_context, addr )
    || virt_is_mapped_in_context( virt_current_user_context, addr );
}
