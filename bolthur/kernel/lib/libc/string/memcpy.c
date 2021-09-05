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

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <panic.h>
#include <mm/virt.h>

/**
 * @fn void memcpy*(void* restrict, const void* restrict, size_t)
 * @brief memcpy implementation
 *
 * @param dst
 * @param src
 * @param size
 */
void* memcpy( void* restrict dst, const void* restrict src, size_t size ) {
  uint8_t* _dst = ( uint8_t * )dst;
  const uint8_t* _src = ( const uint8_t * )src;

  for ( size_t i = 0; i < size; i++ ) {
    _dst[ i ] = _src[ i ];
  }

  return dst;
}

/**
 * @fn void memcpy_unsafe*(void* restrict, const void* restrict, size_t)
 * @brief memcpy unsafe implementation with additional checks to prevent issues by malformed addresses
 *
 * @param dst
 * @param src
 * @param size
 */
void* memcpy_unsafe( void* restrict dst, const void* restrict src, size_t size ) {
  // check if ranges are mapped
  if (
    ! virt_is_mapped_range( ( uintptr_t )dst, size )
    || ! virt_is_mapped_range( ( uintptr_t )src, size )
  ) {
    return NULL;
  }
  // copy with normal memcpy
  return memcpy( dst, src, size );
}
