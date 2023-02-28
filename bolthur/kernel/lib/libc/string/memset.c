/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#define U64_BLOCK_SIZE sizeof( uint64_t )
#define BUFFER_UNALIGNED(val) (( uintptr_t )val & ( U64_BLOCK_SIZE - 1 ))
#define SIZE_TOO_SMALL(size) ( size < U64_BLOCK_SIZE )

/**
 * @brief Fill address with value
 *
 * @param buf buffer to fill
 * @param value value to set
 * @param size length
 * @return void* address to buffer
 */
void* memset( void* buf, int value, size_t size ) {
  uint8_t* u8_buf = ( uint8_t* )buf;
  uint8_t u8_value = ( uint8_t )value;

  // set until alignment fits
  while( BUFFER_UNALIGNED( u8_buf ) ) {
    // set if not reached end
    if ( size-- ) {
      *u8_buf++ = u8_value;
    // return buf if end reached
    } else {
      return buf;
    }
  }
  // set in 8 byte steps as it's now aligned
  if ( ! SIZE_TOO_SMALL( size ) ) {
    // prepare value for set
    uint64_t u64_value = ( uint64_t )u8_value << 56
      | ( uint64_t )u8_value << 48
      | ( uint64_t )u8_value << 40
      | ( uint64_t )u8_value << 32
      | ( uint64_t )u8_value << 24
      | ( uint64_t )u8_value << 16
      | ( uint64_t )u8_value << 8
      | ( uint64_t )u8_value;
    // set pointer
    uint64_t* u64_buf = ( uint64_t* )u8_buf;
    // set as much as possible at once
    while ( size >= U64_BLOCK_SIZE * 4 ) {
      *u64_buf++ = u64_value;
      *u64_buf++ = u64_value;
      *u64_buf++ = u64_value;
      *u64_buf++ = u64_value;
      size -= 4 * U64_BLOCK_SIZE;
    }
    // set remaining 64bit blocks
    while ( size >= U64_BLOCK_SIZE ) {
      *u64_buf++ = u64_value;
      size -= U64_BLOCK_SIZE;
    }
  }
  // set rest
  while( size-- ) {
    *u8_buf++ = u8_value;
  }
  // return buffer
  return buf;
}
