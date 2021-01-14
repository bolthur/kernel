
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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief Compare two strings
 *
 * @param a
 * @param b
 * @return int
 */
int strcmp( const char* a, const char* b ) {
  size_t idx = 0;
  // loop and check
  while ( true ) {
    uint8_t ac = a[ idx ];
    uint8_t bc = b[ idx ];
    // end reached? => equal
    if ( '\0' == ac && '\0' == bc ) {
      return 0;
    }
    // handle a smaller b
    if ( ac < bc ) {
      return -1;
    }
    // handle a greater b
    if ( ac > bc ) {
      return 1;
    }
    // next one
    idx++;
  }
}
