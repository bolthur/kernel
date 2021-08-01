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

#include <duplicate.h>
#include <string.h>
#include <stdlib.h>

/**
 * @fn char duplicate**(const char**)
 * @brief Helper to duplicate 2d array
 *
 * @param src
 * @return
 *
 * @todo Since this is some sort of unsafe copy it needs to be checked whether the area is mapped before copying it
 */
char** duplicate( const char** src ) {
  char** dst = NULL;
  size_t len = 0;
  size_t count;
  size_t src_count = 0;
  // determine source count
  while ( src && src[ src_count ] ) {
    src_count++;
  }
  // Sum up all strings
  for ( count = 0; count < src_count; count++ ) {
    len += strlen( src[ count ] ) + 1;
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
    // copy string content
    strcpy( dst[ count ], src[ count ] );
    // increase length for next offset
    len += strlen( src[ count ] ) + 1;
  }
  // append null termination
  dst[ src_count ] = NULL;
  // return buffer
  return dst;
}
