
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

#include <vendor/rpi/peripheral.h>

// initial setup of peripheral base
#if defined( BCM2709 ) || defined( BCM2710 )
  vaddr_t gpio_peripheral_base = ( vaddr_t )0x3F000000;
  uint32_t gpio_peripheral_size = 0xFFFFFF;
  vaddr_t cpu_peripheral_base = ( vaddr_t )0x40000000;
  uint32_t cpu_peripheral_size = 0x3FFFF;
#else
  vaddr_t gpio_peripheral_base = ( vaddr_t )0x20000000;
  uint32_t gpio_peripheral_size = 0xFFFFFF;
  vaddr_t cpu_peripheral_base = NULL;
  uint32_t cpu_peripheral_size = 0;
#endif

/**
 * @brief Method to set peripheral base address
 *
 * @param addr Address to set peripheral base
 * @param type peripheral type
 */
void peripheral_base_set( vaddr_t addr, peripheral_type_t type ) {
  if ( PERIPHERAL_LOCAL == type ) {
    cpu_peripheral_base = addr;
  } else if ( PERIPHERAL_GPIO == type ) {
    gpio_peripheral_base = addr;
  }
}

/**
 * @brief Method to get peripheral base address
 *
 * @return vaddr_t Peripheral base address
 * @param type peripheral type
 */
vaddr_t peripheral_base_get( peripheral_type_t type ) {
  if ( PERIPHERAL_LOCAL == type ) {
    return cpu_peripheral_base;
  } else if ( PERIPHERAL_GPIO == type ) {
    return gpio_peripheral_base;
  }
  return NULL;
}

/**
 * @brief Method to get peripheral base address
 *
 * @return vaddr_t Peripheral end address
 * @param type peripheral type
 */
vaddr_t peripheral_end_get( peripheral_type_t type ) {
  if ( PERIPHERAL_LOCAL == type ) {
    return ( vaddr_t )( ( uint32_t )cpu_peripheral_base + cpu_peripheral_size );
  } else if ( PERIPHERAL_GPIO == type ) {
    return ( vaddr_t )( ( uint32_t )gpio_peripheral_base + gpio_peripheral_size );
  }
  return NULL;
}
