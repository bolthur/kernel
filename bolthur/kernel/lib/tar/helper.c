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

#include <stdint.h>
#include <stddef.h>
#include <tar.h>

/**
 * @brief Helper to transform size to integer
 *
 * @param in
 * @param size
 * @return uint64_t
 */
size_t octal_size_to_int( const char* in, size_t size ) {
  size_t value = 0;

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
    value += ( size_t )( *in - '0' );
    // step to next
    ++in;
    --size;
  }

  // return calculated size
  return value;
}
