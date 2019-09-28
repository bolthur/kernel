
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

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <tar.h>

#include <kernel/entry.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/placement.h>

/**
 * @brief Method to initialize placement address
 */
void placement_init( void ) {
  // set placement address to kernel end
  placement_address = ( uintptr_t )VIRT_2_PHYS( &__kernel_end );

  // calculate initrd size
  uint64_t initrd_size = tar_get_total_size( placement_address );

  // move placement address beyond initrd if existing
  if ( 0 < initrd_size ) {
    placement_address += ( uint32_t )initrd_size;
  }
}
