
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

#include <stdint.h>

#include <kernel/boot/virt.h>
#include <arch/arm/mm/virt.h>

/**
 * @brief Method to prepare section during initial boot
 * @todo Read amount of physical memory initially with a helper from mailbox
 * @todo Pass fetched amount of physical memory to vmm setup functions
 */
void SECTION( ".text.boot" ) boot_vendor_prepare( void ) {
  uint32_t reg;

  // get paging support from mmfr0
  __asm__ __volatile__( "mrc p15, 0, %0, c0, c1, 4" : "=r" ( reg ) : : "cc" );

  // strip out everything not needed
  reg &= 0xF;

  // check for invalid paging support
  if (
    ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS != reg
    && ID_MMFR0_VSMA_V7_PAGING_PXN != reg
    && ID_MMFR0_VSMA_V7_PAGING_LPAE != reg
  ) {
    return;
  }

  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == reg ) {
    boot_setup_long_vmm( MAX_PHYSICAL_MEMORY );
  } else {
    boot_setup_short_vmm( MAX_PHYSICAL_MEMORY );
  }
}
