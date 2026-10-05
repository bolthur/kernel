/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
#include "peripheral.h"

// initial setup of peripheral base
uintptr_t gpio_peripheral_base = PERIPHERAL_GPIO_BASE;
size_t gpio_peripheral_size = PERIPHERAL_GPIO_SIZE;
uintptr_t cpu_peripheral_base = PERIPHERAL_CPU_BASE;
size_t cpu_peripheral_size = PERIPHERAL_CPU_SIZE;

/**
 * @fn void peripheral_base_set(uintptr_t, peripheral_type_t)
 * @brief Method to set peripheral base address
 *
 * @param addr Address to set peripheral base
 * @param type peripheral type
 */
void peripheral_base_set( const uintptr_t addr, const peripheral_type_t type ) {
  if ( PERIPHERAL_LOCAL == type ) {
    cpu_peripheral_base = addr;
  } else if ( PERIPHERAL_GPIO == type ) {
    gpio_peripheral_base = addr;
  }
}
