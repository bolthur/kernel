
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

#include <vendor/rpi/peripheral.h>

// initial setup of peripheral base
#if defined( BCM2709 ) || defined( BCM2710 )
  vaddr_t peripheral_base = ( vaddr_t )0x3F000000;
  uint32_t peripheral_size = 0xFFFFFF;
#else
  vaddr_t peripheral_base = ( vaddr_t )0x20000000;
  uint32_t peripheral_size = 0xFFFFFF;
#endif

/**
 * @brief Method to set peripheral base address
 *
 * @param addr Address to set peripheral base
 */
void peripheral_base_set( vaddr_t addr ) {
  peripheral_base = addr;
}

/**
 * @brief Method to get peripheral base address
 *
 * @return vaddr_t Peripheral base address
 */
vaddr_t peripheral_base_get( void ) {
  return peripheral_base;
}

/**
 * @brief Method to get peripheral base address
 *
 * @return vaddr_t Peripheral end address
 */
vaddr_t peripheral_end_get( void ) {
  return ( vaddr_t )( ( uint32_t )peripheral_base + peripheral_size );
}
