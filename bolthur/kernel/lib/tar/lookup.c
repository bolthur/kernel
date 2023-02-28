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

#include <stdint.h>
#include "../string.h"
#include "../tar.h"

/**
 * @brief Lookup for specific tar file
 *
 * @param address
 * @param file_name
 * @return tar_header_t*
 */
tar_header_t* tar_lookup_file( uintptr_t address, const char* file_name ) {
  // iterator
  tar_header_t* iter = ( tar_header_t* )address;
  // loop through tar
  while ( iter ) {
    // check for file
    if ( ! memcmp( iter->file_name, file_name, strlen( file_name ) + 1 ) ) {
      break;
    }
    // next iterator
    iter = tar_next( iter );
  }
  // return iter
  return iter;
}
