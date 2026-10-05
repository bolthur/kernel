/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include "pool.h"
#include "../entry.h"
#include "../mm/virt.h"
#include "../mm/phys.h"
#include "../lib/assert.h"

/**
 * @brief pool start
 */
uintptr_t cpu_pool_start;

/**
 * @brief current pool size
 */
uintptr_t cpu_pool_size;

/**
 * @brief backup list head
 */
cpu_pool_t* cpu_pool_head;

/**
 * @brief backup list tail
 */
cpu_pool_t* cpu_pool_tail;

/**
 * @fn void cpu_pool_setup( void )
 * @brief Setup cpu pool
 */
void cpu_pool_setup( void ) {
  // rpc pool starts with one page => ~64 possible rpc
  cpu_pool_start = KERNEL_CPU_POOL_START;
  cpu_pool_size = PAGE_SIZE;
  // map rpc pool
  for ( uintptr_t addr = cpu_pool_start; addr < cpu_pool_start + cpu_pool_size; addr += PAGE_SIZE ) {
    assert( virt_map_address_random(
      virt_current_kernel_context,
      addr,
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
    ) );
  }
  // setup head and tail
  cpu_pool_head = cpu_pool_tail = nullptr;
  // init pool
  cpu_pool_init();
}
