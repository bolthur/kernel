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

#include "../../../../cpu/pool.h"
#include "../../../../entry.h"
#include "../../../../mm/phys.h"
#include "../../../../mm/virt.h"
#include "pool.h"

/**
 * @fn void cpu_pool_push(const void*)
 * @brief Push cpu structure back to pool
 * @param entry
 */
void cpu_pool_push( const void* entry ) {
  auto const container = CPU_POOL_GET_CONTAINER( entry );
  if ( ! cpu_pool_head ) {
    cpu_pool_head = cpu_pool_tail = container;
    return;
  }
  cpu_pool_tail->next = container;
  cpu_pool_tail = container;
}

/**
 * @fn void* cpu_pool_pop(void)
 * @brief Pop cpu structure from pool
 * @return
 */
void* cpu_pool_pop( void ) {
  if ( ! cpu_pool_head ) {
    cpu_pool_expand();
  }
  if ( ! cpu_pool_head ) {
    return nullptr;
  }
  cpu_pool_t* cpu = cpu_pool_head;
  // take last element in list
  if ( cpu_pool_head == cpu_pool_tail ) {
    cpu_pool_head = cpu_pool_tail = nullptr;
    return cpu;
  }
  // push head to next
  cpu_pool_head = cpu_pool_head->next;
  // return backup
  return &cpu->context;
}

/**
 * @fn void cpu_pool_init(void)
 * @brief Init cpu pool
 */
void cpu_pool_init( void ) {
  // get max pool count
  const size_t pool_count = cpu_pool_size / sizeof( cpu_pool_t );
  // iterate through pool count
  for ( size_t i = 0; i < pool_count; i++ ) {
    const uintptr_t addr = ( uintptr_t )( ( ( uint8_t* )cpu_pool_start ) + i * CONTEXT_STRIDE );
    cpu_pool_push( ( cpu_pool_t* )addr );
  }
}

/**
 * @fn void cpu_pool_expand(void)
 * @brief Expand cpu pool
 */
void cpu_pool_expand( void ) {
  const uintptr_t new_size = cpu_pool_size + PAGE_SIZE;
  // handle limit reached
  if ( cpu_pool_start + new_size > KERNEL_CPU_POOL_END ) {
    return;
  }
  // try map it
  if ( ! virt_map_address_random(
      virt_current_kernel_context,
      cpu_pool_start + cpu_pool_size,
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
  ) ) {
    return;
  }
  // get max pool count
  constexpr size_t pool_count = PAGE_SIZE / sizeof( cpu_pool_t );
  const size_t max_existing_index = cpu_pool_size / sizeof( cpu_pool_t );
  // iterate through pool count
  for ( size_t i = 0; i < pool_count; i++ ) {
    const uintptr_t addr = ( uintptr_t )( ( ( uint8_t* )cpu_pool_start ) + ( max_existing_index + i ) * CONTEXT_STRIDE );
    cpu_pool_push( ( cpu_pool_t* )addr );
  }
  // finally set new size
  cpu_pool_size = new_size;
}
