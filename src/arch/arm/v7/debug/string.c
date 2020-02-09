
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
 * @brief Copy of memcpy for remote debugging
 *
 * @param dstptr
 * @param srcptr
 * @param size
 * @return void*
 */
void* debug_memcpy( void* restrict dstptr, const void* restrict srcptr, size_t size ) {
  uint8_t* dst = ( uint8_t * ) dstptr;
  const uint8_t* src = ( const uint8_t * ) srcptr;

  for ( size_t i = 0; i < size; i++ ) {
    dst[ i ] = src[ i ];
  }

  return dstptr;
}

/**
 * @brief Copy of memset for remote debugging
 *
 * @param bufptr
 * @param value
 * @param size
 * @return void*
 */
void* debug_memset ( void* bufptr, int value, size_t size ) {
  uint8_t* buf = ( uint8_t* ) bufptr;

  for ( size_t i = 0; i < size; i++ ) {
    buf[ i ] = ( uint8_t ) value;
  }

  return bufptr;
}
