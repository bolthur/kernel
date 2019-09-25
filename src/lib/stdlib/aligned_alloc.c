
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

#include <kernel/panic.h>
#include <kernel/mm/placement.h>
#include <kernel/mm/virt.h>
#include <kernel/mm/heap.h>

/**
 * @brief alligned memory allocation
 *
 * @param alignment
 * @param size
 * @return void*
 */
void* aligned_alloc( size_t alignment, size_t size ) {
  // return allocated heap block
  if ( true == heap_initialized_get() ) {
    return ( void* )heap_allocate_block( size, alignment );
  }
  assert( true != virt_initialized_get() );
  // use normal placement alloc
  return ( void* )PHYS_2_VIRT( placement_alloc( alignment, size ) );
}
