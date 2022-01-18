/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include "../../string.h"
#include "../../../panic.h"
#include "../../../mm/virt.h"

#define U64_BLOCK_SIZE sizeof( uint64_t )
#define UNALIGNED(a, b) ((( uintptr_t )a & ( U64_BLOCK_SIZE - 1 )) | (( uintptr_t )b & ( U64_BLOCK_SIZE - 1 )))
#define SIZE_TOO_SMALL(size) ( size < U64_BLOCK_SIZE )

/**
 * @fn void memcpy*(void* restrict, const void* restrict, size_t)
 * @brief memcpy implementation
 *
 * @param dst
 * @param src
 * @param size
 */
void* memcpy( void* restrict dst, const void* restrict src, size_t size ) {
  uint8_t* u8_dst = ( uint8_t * )dst;
  const uint8_t* u8_src = ( const uint8_t * )src;
  // copy in 4 byte chunks
  if ( ! SIZE_TOO_SMALL( size ) && ! UNALIGNED( src, dst ) ) {
    // get 64bit pointers
    uint64_t* u64_dst = ( uint64_t * )dst;
    const uint64_t* u64_src = ( const uint64_t * )src;
    // set as much as possible at once
    while ( size >= U64_BLOCK_SIZE * 4 ) {
      *u64_dst++ = *u64_src++;
      *u64_dst++ = *u64_src++;
      *u64_dst++ = *u64_src++;
      *u64_dst++ = *u64_src++;
      size -= 4 * U64_BLOCK_SIZE;
    }
    // set remaining 64bit blocks
    while ( size >= U64_BLOCK_SIZE ) {
      *u64_dst++ = *u64_src++;
      size -= U64_BLOCK_SIZE;
    }
    // Pick up any residual with a byte copier.
    u8_dst = ( uint8_t* )u64_dst;
    u8_src = ( uint8_t* )u64_src;
  }
  // copy byte wise
  while ( size-- ) {
    *u8_dst++ = *u8_src++;
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
