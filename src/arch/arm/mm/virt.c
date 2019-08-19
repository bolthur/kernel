
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

/**
 * @brief Supported modes
 */
uint32_t supported_modes;

/**
 * @brief user context
 */
virt_context_ptr_t user_context;

/**
 * @brief kernel context
 */
virt_context_ptr_t kernel_context;

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

  // prepare
  virt_arch_prepare();

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
