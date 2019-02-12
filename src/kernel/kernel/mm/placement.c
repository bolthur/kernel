
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include <stdint.h>
#include <stddef.h>

#include "kernel/kernel/panic.h"
#include "kernel/kernel/mm/placement.h"
#include "kernel/kernel/mm/phys.h"
#include "kernel/kernel/mm/virt.h"
#include "kernel/kernel/mm/heap.h"
#include "kernel/kernel/entry.h"

void* placement_alloc( size_t size, size_t alignment ) {
  // check for heap not initialized
  if ( heap_initialized_get() || virt_initialized_get() ) {
    PANIC( "placement_alloc used with initialized heap/virtual manager!" );
  }

  // determine offset for alignment
  size_t offset = size;

  // states of memory managers
  bool phys = phys_initialized_get();

  // build return address
  void* address = ( void* )placement_address;

  // handle alignment
  if ( 0 < alignment ) {
    // increase offset
    offset += ( ( size_t )placement_address % alignment );

    // increase address
    address = ( void* )( ( uintptr_t )address + offset );
  }

  // add size to offset
  offset += size;

  // move up placement address
  placement_address += offset;

  // mark as used at physical memory manager if initialized
  if ( phys ) {
    phys_use_page_range( address, offset );
  }

  // finally return address
  return address;
}
