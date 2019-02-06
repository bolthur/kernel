
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

#include <stdint.h>

#include "lib/tar/tar.h"

uint32_t tar_get_size( const uint8_t* in ) {
  uint32_t size = 0;

  // calculate size
  for (
    uint32_t loop = 11,
    count = 1;
    loop > 0;
    loop--, count *= 8
  ) {
    size += ( ( uint32_t )( in[ loop - 1 ] - '0' ) * count );
  }

  // return calculated
  return size;
}
