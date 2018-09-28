
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

#ifndef __KERNEL_VENDOR_RPI_PERIPHERAL__
#define __KERNEL_VENDOR_RPI_PERIPHERAL__

#if defined( PLATFORM_RPI2_B ) || defined( PLATFORM_RPI2_B_REV2 ) || defined( PLATFORM_RPI3_B ) || defined( PLATFORM_RPI3_B_PLUS )
  #define PERIPHERAL_BASE 0x3F000000
#else
  #define PERIPHERAL_BASE 0x20000000
#endif

#endif
