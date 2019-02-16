
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

#include "lib/stdc/stdio.h"
#include "lib/stdc/string.h"
#include "kernel/kernel/panic.h"
#include "kernel/kernel/entry.h"
#include "kernel/kernel/mm/phys.h"
#include "kernel/kernel/mm/virt.h"
#include "kernel/arch/arm/mm/virt.h"

/**
 * @brief
 *
 * @param context pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flags flags used for mapping
 */
void virt_map_address( void* context, void* vaddr, void* paddr, uint32_t flags ) {
  ( void )context;
  ( void )vaddr;
  ( void )paddr;
  ( void )flags;
}

/**
 * @brief Method to create virtual context
 *
 * @param type context type
 * @return void* address of context
 */
void* virt_create_context( virt_context_type_t type ) {
  ( void )type;
  return NULL;
}

/**
 * @brief Method to create table
 *
 * @return void* address of table
 */
void* virt_create_table( void ) {
  return NULL;
}