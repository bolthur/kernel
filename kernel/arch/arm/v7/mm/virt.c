
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
#include <stdio.h>

#include "kernel/panic.h"
#include "kernel/mm/virt.h"
#include "arch/arm/mm/phys.h"
#include "arch/arm/mm/virt.h"
#include "arch/arm/v7/mm/virt.h"

/**
 * @brief
 *
 * @param page_directory pointer to page directory
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flags flags used for mapping
 */
void virt_map_address( void* page_directory, void* vaddr, void* paddr, uint32_t flags ) {
  // mark parameter as unused
  ( void )page_directory;
  ( void )vaddr;
  ( void )paddr;
  ( void )flags;
}
