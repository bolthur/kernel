
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

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Fill address with value
 *
 * @param bufptr
 * @param value
 * @param size
 * @return void*
 */
void* memset( void* bufptr, int value, size_t size ) {
  uint8_t* buf = ( uint8_t* ) bufptr;

  for ( size_t i = 0; i < size; i++ ) {
    buf[ i ] = ( uint8_t ) value;
  }

  return bufptr;
}
