
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
#include <stdbool.h>
#include <tar.h>

/**
 * @brief Method to get total size of tar
 *
 * @param address
 * @return uint64_t
 */
uint64_t tar_total_size( uintptr_t address ) {
  uint64_t total_size = 0;

  while ( true ) {
    // get tar header
    tar_header_ptr_t header = ( tar_header_ptr_t )address;

    // check for end reached
    if ( '\0' == header->file_name[ 0 ] ) {
      break;
    }

    // calculate size
    uint64_t size = octal_size_to_int( header->file_size, 11 );
    total_size += ( ( ( ( size + 511 ) / 512 ) + 1 ) * 512 );

    // get to next file
    address += ( uintptr_t )( ( ( ( size + 511 ) / 512 ) + 1 ) * 512 );
  }

  return total_size;
}

/**
 * @brief Method to get size of file by header
 *
 * @param header
 * @return uint64_t
 */
uint64_t tar_size( tar_header_ptr_t header ) {
  // check for end reached
  if ( '\0' == header->file_name[ 0 ] ) {
    return 0;
  }

  return octal_size_to_int( header->file_size, 11 );
}
