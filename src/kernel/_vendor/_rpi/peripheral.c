
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
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

#include <_vendor/_rpi/peripheral.h>

// initial setup of peripheral base
#if defined( PLATFORM_RPI2_B ) || defined( PLATFORM_RPI2_B_REV2 ) || defined( PLATFORM_RPI3_B ) || defined( PLATFORM_RPI3_B_PLUS )
  uint32_t peripheral_base = 0x3F000000;
#else
  uint32_t peripheral_base = 0x20000000;
#endif

void peripheral_base_set( uint32_t addr ) {
  peripheral_base = addr;
}

uint32_t peripheral_base_get( void ) {
  return peripheral_base;
}
