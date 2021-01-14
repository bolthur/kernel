
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

#if ! defined( __CORE_IO__ )
#define __CORE_IO__

#include <stdint.h>

uint8_t io_in8( uint32_t );
void io_out8( uint32_t, uint8_t );

uint16_t io_in16( uint32_t );
void io_out16( uint32_t, uint16_t );

uint32_t io_in32( uint32_t );
void io_out32( uint32_t, uint32_t );

#endif
