
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

#include <mm/kernel/arch/arm/virt.h>
#include <kernel/entry.h>
#include <mm/boot/arch/arm/v6/short.h>

/**
 * @brief Supported mode
 */
static uint32_t supported_mode SECTION( ".data.boot" );

/**
 * @brief Method wraps setup of short / long descriptor mode
 */
void SECTION( ".text.boot" ) boot_virt_setup( paddr_t max_memory ) {
  // get paging support from mmfr0
  __asm__ __volatile__(
    "mrc p15, 0, %0, c0, c1, 4"
    : "=r" ( supported_mode )
    : : "cc"
  );

  // strip out everything not needed
  supported_mode &= 0xF;
  // check for invalid paging support
  if ( ID_MMFR0_VSMA_V6_PAGING != supported_mode ) {
    return;
  }
  boot_virt_setup_short( max_memory );
}

/**
 * @brief
 *
 * @param phys
 * @param virt
 */
void SECTION( ".text.boot" )
boot_virt_map( paddr_t phys, vaddr_t virt ) {
  // check for invalid paging support
  if ( ID_MMFR0_VSMA_V6_PAGING != supported_mode ) {
    return;
  }

  // map it
  boot_virt_map_short( phys, virt );
}
