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

#include <stddef.h>
#include <stdint.h>
#include "../../string.h"
#include "../../../mm/virt.h"
#if defined( HAS_SANITIZER )
  #include "../../kasan/kasan.h"
#endif

/**
 * @fn void memcpy*(void* restrict, const void* restrict, size_t)
 * @brief memcpy implementation
 *
 * @param dst
 * @param src
 * @param size
 */
__attribute__((__optimize__("O3")))
__no_sanitize
void* memcpy( void* restrict dst, const void* restrict src, size_t size ) {
  #if defined( HAS_SANITIZER )
    kasan_check_memory( ( uintptr_t )dst, size, 1, KASAN_CALLER_PC );
    kasan_check_memory( ( uintptr_t )src, size, 1, KASAN_CALLER_PC );
  #endif
  // cache destination and source
  auto u8_dst = ( uint8_t* )dst;
  auto u8_src = ( uint8_t* )src;
  // handle both have same unalignment
  if ( BUFFER_UNALIGNED( u8_dst ) == BUFFER_UNALIGNED( u8_src ) ) {
    // copy until we have proper 64 bit alignment
    while ( size > 0 && BUFFER_UNALIGNED( u8_dst ) ) {
      *u8_dst++ = *u8_src++;
      size--;
    }
    // copy in 4 byte chunks
    if ( ! SIZE_TOO_SMALL( size ) ) {
      // set as much as possible at once
      while ( size >= U256_BLOCK_SIZE ) {
        volatile uint64_t chunk[ 4 ];
        chunk[ 0 ] = ( ( const uint64_t* )u8_src)[ 0 ];
        chunk[ 1 ] = ( ( const uint64_t* )u8_src)[ 1 ];
        chunk[ 2 ] = ( ( const uint64_t* )u8_src)[ 2 ];
        chunk[ 3 ] = ( ( const uint64_t* )u8_src)[ 3 ];
        ( ( uint64_t* )u8_dst )[ 0 ] = chunk[ 0 ];
        ( ( uint64_t* )u8_dst )[ 1 ] = chunk[ 1 ];
        ( ( uint64_t* )u8_dst )[ 2 ] = chunk[ 2 ];
        ( ( uint64_t* )u8_dst )[ 3 ] = chunk[ 3 ];
        u8_src += U256_BLOCK_SIZE;
        u8_dst += U256_BLOCK_SIZE;
        size -= U256_BLOCK_SIZE;
      }
      // set remaining 64bit blocks
      while ( size >= U64_BLOCK_SIZE ) {
        *( volatile uint64_t* )u8_dst = *( const volatile uint64_t* )u8_src;
        u8_src += U64_BLOCK_SIZE;
        u8_dst += U64_BLOCK_SIZE;
        size -= U64_BLOCK_SIZE;
      }
    }
  }
  // copy byte wise
  while ( size-- ) {
    *u8_dst++ = *u8_src++;
  }
  // return pointer to destination
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
    return nullptr;
  }
  // copy with normal memcpy
  return memcpy( dst, src, size );
}


/**
 * @fn void memcpy_unsafe_dst*(void* restrict, const void* restrict, size_t)
 * @brief memcpy unsafe implementation with additional checks to prevent issues by malformed addresses
 *
 * @param dst
 * @param src
 * @param size
 */
void* memcpy_unsafe_dst( void* restrict dst, const void* restrict src, size_t size ) {
  // check if ranges are mapped
  if ( ! virt_is_mapped_range( ( uintptr_t )dst, size ) ) {
    return nullptr;
  }
  // copy with normal memcpy
  return memcpy( dst, src, size );
}


/**
 * @fn void memcpy_unsafe_src*(void* restrict, const void* restrict, size_t)
 * @brief memcpy unsafe implementation with additional checks to prevent issues by malformed addresses
 *
 * @param dst
 * @param src
 * @param size
 */
void* memcpy_unsafe_src( void* restrict dst, const void* restrict src, size_t size ) {
  // check if ranges are mapped
  if ( ! virt_is_mapped_range( ( uintptr_t )dst, size ) ) {
    return nullptr;
  }
  // copy with normal memcpy
  return memcpy( dst, src, size );
}
