
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

int memcmp(const void* aptr, const void* bptr, size_t size) {
  const uint8_t* a = ( const uint8_t* )aptr;
  const uint8_t* b = ( const uint8_t* )bptr;

  for ( size_t i = 0; i < size; i++ ) {
    if ( a[ i ] < b[ i ] ) {
      return -1;
    } else if ( b[ i ] < a[ i ] ) {
      return 1;
    }
  }

  return 0;
}
