
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

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Memory compare
 *
 * @param a
 * @param b
 * @param size
 * @return int
 */
int memcmp( const void* a, const void* b, size_t size ) {
  const uint8_t* ua = ( uint8_t* )a;
  const uint8_t* ub = ( uint8_t* )b;

  // loop through strings
  for ( size_t idx = 0; idx < size; idx++ ) {
    // handle smaller
    if ( ua[ idx ] < ub[ idx ] ) {
      return -1;
    }
    // handle greater
    if ( ua[ idx ] > ub[ idx ] ) {
      return 1;
    }
  }

  // equal
  return 0;
}
