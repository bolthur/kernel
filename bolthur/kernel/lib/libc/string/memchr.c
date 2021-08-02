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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief Locate character in memory
 *
 * @param buf
 * @param c
 * @param n
 * @return void*
 */
void* memchr( const void* buf, int32_t c, size_t n ) {
  uint8_t* current = ( uint8_t* )buf;
  uint8_t* end = current + n;

  while ( current != end ) {
    // check for match
    if ( *current == c ) {
      return current;
    }
    // next
    current++;
  }

  // nothing found
  return NULL;
}
