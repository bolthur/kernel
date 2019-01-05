
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

#include <string.h>
#include <stdint.h>

void* memcpy( void* restrict dstptr, const void* restrict srcptr, size_t size ) {
  uint8_t* dst = ( uint8_t * ) dstptr;
  const uint8_t* src = ( const uint8_t * ) srcptr;

  for ( size_t i = 0; i < size; i++ ) {
    dst[ i ] = src[ i ];
  }

  return dstptr;
}