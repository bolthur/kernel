
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <vendor/rpi/peripheral.h>

// initial setup of peripheral base
#if defined( PLATFORM_RPI2_B ) || defined( PLATFORM_RPI2_B_REV2 ) || defined( PLATFORM_RPI3_B ) || defined( PLATFORM_RPI3_B_PLUS )
  uint32_t peripheral_base = 0x3F000000;
#else
  uint32_t peripheral_base = 0x20000000;
#endif

/**
 * @brief Method to set peripheral base address
 *
 * @param addr Address to set peripheral base
 */
void peripheral_base_set( uint32_t addr ) {
  peripheral_base = addr;
}

/**
 * @brief Method to get peripheral base address
 *
 * @return uint32_t Peripheral base address
 */
uint32_t peripheral_base_get( void ) {
  return peripheral_base;
}
