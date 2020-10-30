
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

#include <stdlib.h>
#include <string.h>

/**
 * @brief Calloc routine
 *
 * @param num amount of entries
 * @param size size of one entry
 * @return void* allocated address or NULL
 */
void *calloc( size_t num, size_t size ) {
  // allocate memory
  void *ptr = malloc( num * size );
  // handle malloc error
  if ( ! ptr ) {
    return NULL;
  }

  // overwrite memory with 0
  ptr = memset( ptr, 0, num * size );

  // return prepared memory area
  return ptr;
}
