
/**
 * mist-system/kernel
 * Copyright (C) 2017 mist-system project.
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

// default includes
#include <stdint.h>
#include <stdio.h>

uint32_t boot_parameter_data[ 3 ];

void platform_init( void ) {
  // printf( "0x%x - 0x%x - 0x%x\r\n", boot_parameter_data[ 0 ], boot_parameter_data[ 1 ], boot_parameter_data[ 2 ] );
}
