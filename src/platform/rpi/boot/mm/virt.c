
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
#include <stddef.h>
#include <arch/arm/boot/mm/virt.h>
#include <arch/arm/mm/virt.h>
#include <kernel/entry.h>
#include <kernel/mm/phys.h>

/**
 * @brief Method to setup short descriptor paging
 */
void SECTION( ".text.boot" )
boot_virt_platform_setup( void ) {
  #if defined( BCM2709 ) || defined( BCM2710 )
    uintptr_t gpio_peripheral_base = 0x3F000000;
    size_t gpio_peripheral_size = 0xFFFFFF;
    uintptr_t cpu_peripheral_base = 0x40000000;
    size_t cpu_peripheral_size = 0x3FFFF;
  #else
    uintptr_t gpio_peripheral_base = 0x20000000;
    size_t gpio_peripheral_size = 0xFFFFFF;
    uintptr_t cpu_peripheral_base = 0;
    size_t cpu_peripheral_size = 0;
  #endif

  uintptr_t gpio_peripheral_end = gpio_peripheral_base + gpio_peripheral_size;
  uintptr_t cpu_peripheral_end = cpu_peripheral_base + cpu_peripheral_size;

  // map cpu local peripherals if set
  if ( 0 != cpu_peripheral_base ) {
    while ( cpu_peripheral_base < cpu_peripheral_end ) {
      // identity map gpio
      boot_virt_map( gpio_peripheral_base, gpio_peripheral_base );
      // next page
      cpu_peripheral_base += PAGE_SIZE;
    }
  }

  // map gpio if set
  while ( gpio_peripheral_base < gpio_peripheral_end ) {
    // identity map gpio
    boot_virt_map( gpio_peripheral_base, gpio_peripheral_base );
    // next page
    gpio_peripheral_base += PAGE_SIZE;
  }
}
