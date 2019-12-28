
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

#include <arch/arm/mm/virt.h>
#include <core/entry.h>
#include <arch/arm/v6/boot/mm/virt/short.h>

/**
 * @brief Supported mode
 */
static uint32_t supported_mode __bootstrap_data;

/**
 * @brief Wrapper to setup short descriptor mapping if supported
 *
 * @param max_memory maximum memory to map starting from 0
 */
void __bootstrap boot_virt_setup( uintptr_t max_memory ) {
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

  // setup short memory
  boot_virt_setup_short( max_memory );

  // setup platform related
  boot_virt_platform_setup();
}

/**
 * @brief Wrapper method calling short mapping if supported
 *
 * @param phys physical address
 * @param virt virtual address
 */
void __bootstrap boot_virt_map( uint64_t phys, uintptr_t virt ) {
  // check for invalid paging support
  if ( ID_MMFR0_VSMA_V6_PAGING != supported_mode ) {
    return;
  }

  // map it
  boot_virt_map_short( ( uintptr_t )phys, virt );
}
