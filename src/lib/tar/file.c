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
 * @brief Method to get buffer
 *
 * @param header
 * @return uint8_t*
 */
uint8_t* tar_file( tar_header_ptr_t header ) {
  // check for invalid
  if ( '\0' == header->file_name[ 0 ] ) {
    return NULL;
  }

  // build return
  return ( uint8_t* )( ( uintptr_t )header + TAR_HEADER_SIZE );
}
