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
#include <string.h>
#include <stdlib.h>
#include <mm/heap.h>

/**
 * @fn void realloc*(void*, size_t)
 * @brief realloc implementation
 *
 * @param ptr ptr to allocated space
 * @param size new size
 * @return
 */
void* realloc( void* ptr, size_t size ) {
  // simply use malloc if nothing is there
  if ( ! ptr ) {
    return malloc( size );
  }
  // allocate new memory
  void* new_ptr = malloc( size );
  // handle error
  if ( ! new_ptr ) {
    return NULL;
  }
  // query original length
  size_t original_length = heap_block_length( ( uintptr_t )ptr );
  // determine amount to copy
  size_t to_copy = original_length;
  if ( original_length > size ) {
    to_copy = size;
  }
  // copy data
  new_ptr = memcpy( new_ptr, ptr, to_copy );
  // mark current as free
  free( ptr );
  // return new pointer
  return new_ptr;
}
