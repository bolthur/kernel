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

#include "duplicate.h"
#include "string.h"
#include "stdlib.h"
#include "../mm/virt.h"

/**
 * @fn char duplicate**(const char**)
 * @brief Helper to duplicate 2d array
 *
 * @param src
 * @return
 */
char** duplicate( const char** src ) {
  char** dst = NULL;
  size_t len = 0;
  size_t count;
  size_t src_count = 0;
  // handle no source
  if ( ! src ) {
    // allocate buffer
    dst = ( char** )malloc( ( src_count + 1 ) * sizeof( char* ) + len );
    if ( ! dst ) {
      return NULL;
    }
    // fill in termination only
    dst[ src_count ] = NULL;
    // return
    return dst;
  }
  // determine row count
  while ( true ) {
    // return nothing if not mapped
    if ( ! virt_is_mapped_range(
      ( uintptr_t )&src[ src_count ],
      sizeof( char* )
    ) ) {
      return NULL;
    }
    // break if NULL termination reached
    if ( ! src[ src_count ] ) {
      break;
    }
    // increment line count
    src_count++;
  }
  // Sum up all strings
  for ( count = 0; count < src_count; count++ ) {
    // unsafe strlen with check for error
    size_t tmp_len = strlen_unsafe( src[ count ] );
    if ( ! tmp_len ) {
      return NULL;
    }
    // increase total length
    len += tmp_len + 1;
  }
  // allocate new buffer
  dst = ( char** )malloc( ( src_count + 1 ) * sizeof( char* ) + len );
  if ( ! dst ) {
    return NULL;
  }
  // copy stuff over into contiguous buffer
  len = 0;
  for ( count = 0; count < src_count; count++ ) {
    // set pointer to string
    dst[ count ] =
      &( ( ( char* )dst )[ ( src_count + 1 ) * sizeof( char* ) + len ] );
    // get length from unsafe
    size_t tmp_len = strlen_unsafe( src[ count ] );
    if ( 0 == tmp_len ) {
      free( dst );
      return NULL;
    }
    // copy string content with unsafe copy
    if ( ! memcpy_unsafe( dst[ count ], src[ count ], tmp_len ) ) {
      free( dst );
      return NULL;
    }
    // increase length for next offset
    len += strlen( src[ count ] ) + 1;
  }
  // append null termination
  dst[ src_count ] = NULL;
  // return buffer
  return dst;
}
