
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

#include <string.h>
#include <stdlib.h>

/**
 * @brief Realloc routine
 *
 * @param ptr ptr to allocated space
 * @param size new size
 * @return void* allocated address or NULL
 */
void *realloc( void *ptr, size_t size ) {
  // simply use malloc if nothing is there
  if ( ! ptr ) {
    return malloc( size );
  }

  // allocate new memory
  void *new_ptr = malloc( size );

  // handle error
  if ( ! new_ptr ) {
    free( ptr );
    return NULL;
  }

  // copy data
  new_ptr = memcpy( new_ptr, ptr, size );

  // mark current as free
  free( ptr );

  // return new pointer
  return new_ptr;
}
