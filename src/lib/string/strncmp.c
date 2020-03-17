
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

/**
 * @brief Compare two strings until end or max length
 *
 * @param a
 * @param b
 * @param max_length
 * @return int
 */
int strncmp( const char* a, const char* b, size_t max_length ) {
  // loop until max length
  for ( size_t x = 0; x < max_length; x++ ) {
    if ( '\0' == a[ x ] && '\0' == b[ x ] ) {
      return 0;
    } else if ( a[ x ] < b[ x ] ) {
      return -1;
    } else if ( a[ x ] > b[ x ] ) {
      return 1;
    }
  }
  // equal
  return 0;
}
