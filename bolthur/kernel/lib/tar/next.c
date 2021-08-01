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

#include <stdint.h>
#include <stddef.h>
#include <tar.h>

/**
 * @brief Method to get next element within tar file
 *
 * @param current
 * @return tar_header_ptr_t
 */
tar_header_ptr_t tar_next( tar_header_ptr_t current ) {
  // variables
  uintptr_t address = ( uintptr_t )current;
  uint64_t size;
  tar_header_ptr_t next = NULL;

  // check for invalid
  if ( tar_end_reached( current ) ) {
    return NULL;
  }

  // get size
  size = octal_size_to_int( current->file_size, 11 );
  // get to next file
  address +=( uintptr_t )( ( ( ( size + 511 ) / 512 ) + 1 ) * 512 );
  // transform to tar header
  next = ( tar_header_ptr_t )address;

  // check for end reached
  if ( '\0' == next->file_name[ 0 ] ) {
    return NULL;
  }

  // return next element
  return next;
}
