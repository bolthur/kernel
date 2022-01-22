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

#include <stdint.h>
#include <stddef.h>
#include "bss.h"
#include "entry.h"

#if defined( ELF32 )
  #define BLOCK_TYPE uint32_t
#elif defined( ELF64 )
  #define BLOCK_TYPE uint64_t
#endif
#define BLOCK_SIZE sizeof( BLOCK_TYPE )
#define BUFFER_UNALIGNED(val) (( uintptr_t )val & ( BLOCK_SIZE - 1 ))
#define SIZE_TOO_SMALL(size) ( size < BLOCK_SIZE )

/**
 * @fn void bss_startup_clear(void)
 * @brief Method to clear bss during initial boot
 */
void __bootstrap bss_startup_clear( void ) {
  // start and end values
  uintptr_t start = VIRT_2_PHYS( &__bss_start );
  uintptr_t end = VIRT_2_PHYS( &__bss_end );
  // calculate size to unset
  size_t size = end - start;
  // set buf and value
  uint8_t* u8_buf = ( uint8_t* )start;
  uint8_t u8_value = ( uint8_t )0;

  // set until alignment fits
  while( BUFFER_UNALIGNED( u8_buf ) ) {
    // set if not reached end
    if ( size-- ) {
      *u8_buf++ = u8_value;
    // return, because end was reached
    } else {
      return;
    }
  }
  // set in 8 byte steps as it's now aligned
  if ( ! SIZE_TOO_SMALL( size ) ) {
    // prepare value for set and set temporary pointer
    BLOCK_TYPE block_value = 0;
    BLOCK_TYPE* block_buf = ( uint32_t* )u8_buf;
    // set as much as possible at once
    while ( size >= BLOCK_SIZE * 4 ) {
      *block_buf++ = block_value;
      *block_buf++ = block_value;
      *block_buf++ = block_value;
      *block_buf++ = block_value;
      size -= 4 * BLOCK_SIZE;
    }
    // set remaining 64bit blocks
    while ( size >= BLOCK_SIZE ) {
      *block_buf++ = block_value;
      size -= BLOCK_SIZE;
    }
  }
  // set rest
  while( size-- ) {
    *u8_buf++ = u8_value;
  }
}
