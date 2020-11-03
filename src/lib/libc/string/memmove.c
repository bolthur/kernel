
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
#include <string.h>

/**
 * @brief Move Memory from src to destination with length
 *
 * @param dst destination
 * @param src source
 * @param n length
 * @return address of destination
 */
void* memmove( void* dst, const void* src, size_t n ) {
  uint8_t *cdst = ( uint8_t* )dst;
  const uint8_t *csrc = ( const uint8_t* )src;

  if ( cdst < csrc ) {
    for ( size_t i = 0; i < n; i++ ) {
      cdst[ i ] = csrc[ i ];
    }
    return dst;
  }

  for ( size_t i = n; i != 0; i-- ) {
    cdst[ i - 1 ] = csrc[ i - 1 ];
  }
  return dst;
}
