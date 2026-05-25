/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include "min.h"

/**
 * @fn uint32_t uint32_min( uint32_t, uint32_t )
 * @brief uint32 min implementation
 * @param a
 * @param b
 * @return
 */
uint32_t uint32_min( const uint32_t a, const uint32_t b ) {
  if ( a < b ) {
    return a;
  }
  return b;
}

/**
 * @fn size_t size_min( size_t, size_t )
 * @brief size min implementation
 * @param a
 * @param b
 * @return
 */
size_t size_min( const size_t a, const size_t b ) {
  if ( a < b ) {
    return a;
  }
  return b;
}
