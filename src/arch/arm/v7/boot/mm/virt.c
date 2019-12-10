
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
#include <arch/arm/v7/boot/mm/virt/long.h>
#include <arch/arm/v7/boot/mm/virt/short.h>

/**
 * @brief Supported mode
 */
static uint32_t supported_mode __bootstrap_data;

/**
 * @brief Method wraps setup of short / long descriptor mode
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
  if (
    ! (
      ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_mode
      || ID_MMFR0_VSMA_V7_PAGING_PXN == supported_mode
      || ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_mode
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
    supported_mode = ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS;
  }

  // kick start
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_mode ) {
    boot_virt_setup_long( max_memory );
  } else {
    boot_virt_setup_short( max_memory );
  }

  // setup platform related
  boot_virt_platform_setup();
}

/**
 * @brief Mapper function using short or long descriptor mapping depending on support
 *
 * @param phys physical address
 * @param virt virtual address
 */
void __bootstrap boot_virt_map( uint64_t phys, uintptr_t virt ) {
  // check for invalid paging support
  if (
    ! (
      ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_mode
      || ID_MMFR0_VSMA_V7_PAGING_PXN == supported_mode
      || ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_mode
    )
  ) {
    return;
  }

  // map it
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_mode ) {
    boot_virt_map_long( phys, virt );
  } else {
    boot_virt_map_short( ( uintptr_t )phys, virt );
  }
}
