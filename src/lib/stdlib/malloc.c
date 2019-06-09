
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

void *malloc( size_t size ) {
  bool use_heap = heap_initialized_get();
  bool use_virt = virt_initialized_get();

  // check for no vmm when heap is not yet ready
  if ( ! use_heap ) {
    assert( true != use_virt );
  }

  // normal placement alloc when no heap and no virtual
  if ( true == use_heap ) {
    PANIC( "Heap not yet initialized!" );
  }

  // no heap and no virt?
  return placement_alloc(
    size,
    size + size - size % 2
  );
}
