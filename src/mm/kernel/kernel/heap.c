
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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
#include <assert.h>
#include <mm/kernel/kernel/heap.h>

/**
 * @brief Kernel heap
 */
heap_ptr_t kernel_heap = NULL;

/**
 * @brief Getter for heap initialized flag
 *
 * @return true
 * @return false
 */
bool heap_initialized_get( void ) {
  return NULL != kernel_heap;
}

/**
 * @brief Initialize heap
 */
void heap_init( void ) {
  // assert not initialized
  assert( NULL == kernel_heap );
}
