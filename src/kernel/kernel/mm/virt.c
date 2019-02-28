
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

#include <stddef.h>
#include <stdbool.h>

#include "kernel/kernel/mm/virt.h"
#include "kernel/kernel/panic.h"

/**
 * @brief static initialized flag
 */
static bool virt_initialized = false;

/**
 * @brief Flag for indicating usage of physical tables
 */
bool virt_use_physical_table = false;

/**
 * @brief Generic initialization of virtual memory manager
 */
void virt_init( void ) {
  // assert no initialize
  ASSERT( true != virt_initialized );

  // set use physical to true
  virt_use_physical_table = true;

  // initialize vendor init
  virt_vendor_init();

  // set static
  virt_initialized = true;

  // reset use physical table flag
  virt_use_physical_table = false;
}

/**
 * @brief Get initialized flag
 *
 * @return true virtual memory management has been set up
 * @return false virtual memory management has been not yet set up
 */
bool virt_initialized_get( void ) {
  return virt_initialized;
}
