/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#pragma GCC optimize ("no-tree-loop-distribute-patterns")

#include <stdint.h>
#include "do_string.h"

/**
 * @fn void memcpy*(void* restrict, const void* restrict, size_t)
 * @brief memcpy implementation
 *
 * @param dst
 * @param src
 * @param size
 */
void* do_memcpy( void* restrict dst, const void* restrict src, size_t size ) {
  uint8_t* u8_dst = ( uint8_t * )dst;
  const uint8_t* u8_src = ( const uint8_t * )src;
  // copy byte wise
  while ( size-- ) {
    *u8_dst++ = *u8_src++;
  }
  return dst;
}

/**
 * @brief Fill address with value
 *
 * @param buf buffer to fill
 * @param value value to set
 * @param size length
 * @return void* address to buffer
 */
void* do_memset( void* buf, int value, size_t size ) {
  uint8_t* u8_buf = ( uint8_t* )buf;
  uint8_t u8_value = ( uint8_t )value;
  // set byte wise
  while( size-- ) {
    *u8_buf++ = u8_value;
  }
  // return buffer
  return buf;
}
