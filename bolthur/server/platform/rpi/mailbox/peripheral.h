/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#if ! defined( _PERIPHERAL_H )
#define _PERIPHERAL_H

// initial setup of peripheral base
#if defined( BCM2836 ) || defined( BCM2837 )
  #define PERIPHERAL_BASE 0x3F000000
  #define PERIPHERAL_SIZE 0xFFFFFF
#else
  #define PERIPHERAL_BASE 0x20000000
  #define PERIPHERAL_SIZE 0xFFFFFF
#endif

#endif
