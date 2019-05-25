
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

#include <kernel/mm/placement.h>
#include <kernel/mm/virt.h>
#include <kernel/mm/heap.h>

/**
 * @brief Aligned memory allocation
 *
 * @param alignment alignment address shall match
 * @param size amount of memory
 * @return void* NULL on error or address to memory
 */
void *aligned_alloc( size_t alignment, size_t size ) {
  // normal placement alloc when no heap and no virtual
  if (
    ! heap_initialized_get()
    && ! virt_initialized_get()
  ) {
    return placement_alloc( size, alignment );
  }

  return NULL;
}
