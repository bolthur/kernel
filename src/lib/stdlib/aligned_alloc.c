
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
#include <assert.h>

#include <core/panic.h>
#include <core/mm/placement.h>
#include <core/mm/virt.h>
#include <core/mm/heap.h>

/**
 * @brief alligned memory allocation
 *
 * @param alignment alignment
 * @param size size to allocate
 * @return void* reserved memory
 */
void* aligned_alloc( size_t alignment, size_t size ) {
  // check for initialized heap
  if ( true == heap_initialized_get() ) {
    // use heap allocation
    return ( void* )heap_allocate_block( alignment, size );
  }

  // placement allocator requires that  virtual memory is not initialized
  assert( true != virt_initialized_get() );

  // use normal placement alloc
  return ( void* )PHYS_2_VIRT( placement_alloc( alignment, size ) );
}
