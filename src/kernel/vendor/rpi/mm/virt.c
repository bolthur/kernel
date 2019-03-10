
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

#include "lib/k/string.h"
#include "kernel/kernel/debug.h"
#include "kernel/kernel/entry.h"
#include "kernel/kernel/panic.h"
#include "kernel/kernel/mm/placement.h"
#include "kernel/kernel/mm/phys.h"
#include "kernel/kernel/mm/virt.h"
#include "kernel/arch/arm/barrier.h"
#include "kernel/arch/arm/mm/virt.h"
#include "kernel/vendor/rpi/peripheral.h"
#include "kernel/vendor/rpi/framebuffer.h"

vaddr_t user_context;
vaddr_t kernel_context;

/**
 * @brief Initialize virtual memory management
 */
void virt_vendor_init( void ) {
  // get new kernel context and temporary user context
  kernel_context = ( vaddr_t )PHYS_2_VIRT(
    placement_alloc( SD_TTBR_SIZE_4G, SD_TTBR_ALIGNMENT_4G )
  );
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

  //PANIC( "To be implemented" );

  /**
   * Short descriptor initialization:
   *  - Setup new ttbr0 with 8KB size ( 2GB )
   *  - Setup new ttbr1 with 16KB size ( according to specs normal, when using ttbr0 and ttbr1 )
   *  - Set TTBCR.N to SD_TTBCR_N_TTBR0_2G
   *  - Set TTBCR.EAE to 0
   *  - Map Kernel starting from 0 + KERNEL_OFFSET to placement_address + KERNEL_OFFSET
   *  - Map peripherals to 0xF2000000 ( without caching )
   *  - Map framebuffer if built in to 0xF3000000 ( without caching )
   *  - Map additional with specific core mappings on rpi 2 and 3 ( without caching )
   *  - Replace ttbr0 and ttbr1 with new ones
   *  - Invalidate cache and tlb
   *  - Handle data barriers
   *  - Reset framebuffer base and peripheral base according to mappings
   */
}
