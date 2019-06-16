
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

#include <stddef.h>
#include <assert.h>

#include <kernel/kernel/panic.h>
#include <mm/kernel/kernel/placement.h>
#include <mm/kernel/kernel/virt.h>
#include <mm/kernel/kernel/heap.h>

/**
 * @brief Malloc implementation
 *
 * @param size size to allocate
 * @return void* allocated address or NULL
 */
void *malloc( size_t size ) {
  // adjust size to base of 2
  size += size % 2;
  // return allocated heap block
  if ( true == heap_initialized_get() ) {
    return heap_allocate_block( size );
  }
  // check for no virtual memory when heap is not yet ready
  assert( true != virt_initialized_get() );
  // use normal placement alloc
  return placement_alloc( size, size );
}
