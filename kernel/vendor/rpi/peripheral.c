
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include "vendor/rpi/peripheral.h"

// initial setup of peripheral base
#if defined( PLATFORM_RPI2_B ) || defined( PLATFORM_RPI2_B_REV2 ) || defined( PLATFORM_RPI3_B ) || defined( PLATFORM_RPI3_B_PLUS )
  uintptr_t peripheral_base = 0x3F000000;
  uintptr_t peripheral_size = 0xFFFFFF;
#else
  uintptr_t peripheral_base = 0x20000000;
  uintptr_t peripheral_size = 0xFFFFFF;
#endif

/**
 * @brief Method to set peripheral base address
 *
 * @param addr Address to set peripheral base
 */
void peripheral_base_set( uintptr_t addr ) {
  peripheral_base = addr;
}

/**
 * @brief Method to get peripheral base address
 *
 * @return uintptr_t Peripheral base address
 */
uintptr_t peripheral_base_get( void ) {
  return peripheral_base;
}

/**
 * @brief Method to get peripheral base address
 *
 * @return uintptr_t Peripheral base address
 */
uintptr_t peripheral_end_get( void ) {
  return peripheral_base + peripheral_size;
}
