
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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
#include <stdbool.h>

#include <string.h>
#include <stdio.h>
#include <tar.h>

/**
 * @brief Helper to transform size to integer
 *
 * @param in
 * @param size
 * @return uint64_t
 */
static uint64_t octal_size_to_int( const char* in, size_t size ) {
  uint64_t value = 0;

  // skip bullshit data
  while ( ( '0' > *in || '7' < *in ) && 0 < size ) {
    ++in;
    --size;
  }

  // parse octal to int
  while ( '0' <= *in && '7' >= *in  && 0 < size ) {
    // multiply by base
    value *= 8;
    // add number
    value += ( uint64_t )( *in - '0' );
    // step to next
    ++in;
    --size;
  }

  // return calculated size
  return value;
}

/**
 * @brief Method to get total size of tar
 *
 * @param address
 * @return uint64_t
 */
uint64_t tar_get_total_size( uintptr_t address ) {
  uint64_t total_size = 0;
  char* buf = ( char* )address;

  while( true ) {
    // get tar header
    tar_header_ptr_t header = ( tar_header_ptr_t )buf;

    // check for end reached
    if ( '\0' == header->file_name[ 0 ] ) {
      break;
    }

    // calculate size
    uint64_t size = octal_size_to_int( header->file_size, 11 );
    total_size += size;

    // get to next file
    buf += ( ( ( ( size + 511 ) / 512 ) + 1 ) * 512 );
  }

  return total_size;
}
