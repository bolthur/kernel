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

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <mm/phys.h>
#include <mm/virt.h>
#include <debug/debug.h>

#define U64_BLOCK_SIZE sizeof( uint64_t )
#define BUFFER_UNALIGNED(val) ( ( uintptr_t )val & ( U64_BLOCK_SIZE - 1 ) )
#define DETECT_NULL_ENDING(x) ( ( x - 0x0101010101010101) & ~x & 0x8080808080808080 )

/**
 * @fn size_t strlen(const char*)
 * @brief Get string length
 *
 * @param str
 * @return
 */
size_t strlen( const char* str ) {
  const char* start = str;
  // handle invalid
  if ( ! str ) {
    return 0;
  }
  // loop until alignment
  while( BUFFER_UNALIGNED( str ) ) {
    // handle end reached
    if ( !*str ) {
      return ( size_t )( str - start );
    }
    // increment
    str++;
  }
  // use 64bit for checks
  uint64_t* aligned = ( uint64_t* )str;
  while ( ! DETECT_NULL_ENDING( *aligned ) ) {
    aligned++;
  }
  // update str
  str = ( const char* )str;
  while ( *str ) {
    str++;
  }
  // return difference
  return ( size_t )( str - start );
}

/**
 * @fn size_t strlen_unsafe(const char*)
 * @brief Unsafe strlen
 *
 * @param str
 * @return
 */
size_t strlen_unsafe( const char* str ) {
  size_t len = 0;
  const char* start = str;
  // handle null
  if ( ! str ) {
    return len;
  }
  uintptr_t last_check = ROUND_DOWN_TO_FULL_PAGE( str );
  const char* next_check = ( const char* )( last_check + PAGE_SIZE );
  do {
    // check page size
    if ( ! virt_is_mapped_range( last_check, PAGE_SIZE ) ) {
      return 0;
    }
    // loop until alignment
    while( BUFFER_UNALIGNED( str ) && str < next_check ) {
      // handle end reached
      if ( !*str ) {
        return ( size_t )( str - start );
      }
      // increment
      str++;
    }
    // handle page boundary reached
    if ( str == next_check ) {
      last_check = ( uintptr_t )next_check;
      next_check = ( const char* )( last_check + PAGE_SIZE );
      continue;
    }
    // use 64bit for checks
    uint64_t* aligned = ( uint64_t* )str;
    while ( ! DETECT_NULL_ENDING( *aligned ) && str < next_check ) {
      aligned++;
    }
    // handle page boundary reached
    if ( str == next_check ) {
      last_check = ( uintptr_t )next_check;
      next_check = ( const char* )( last_check + PAGE_SIZE );
      continue;
    }
    // update str
    str = ( const char* )str;
    while ( *str && str < next_check ) {
      str++;
    }
    // handle page boundary reached
    if ( str == next_check ) {
      last_check = ( uintptr_t )next_check;
      next_check = ( const char* )( last_check + PAGE_SIZE );
      continue;
    }
    // return difference
    return ( size_t )( str - start );
  } while( true );
}
