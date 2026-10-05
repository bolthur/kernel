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

#include "../../../../task/stack.h"
#include "../../stack.h"
#include "../../../../entry.h"

#if defined( ELF32 )
  #define THREAD_STACK_START_ADDRESS USER_AREA_END
#elif defined( ELF64 )
  #error "Unsupported"
#endif

/**
 * @fn uintptr_t task_stack_manager_next(task_stack_manager_t*)
 * @brief Get next virtual stack address
 * @param manager
 * @return uintptr_t
 */
uintptr_t task_stack_manager_next( task_stack_manager_t* manager ) {
  // check parameter
  if ( ! manager ) {
    return 0;
  }
  // cache current
  uintptr_t current_top = THREAD_STACK_START_ADDRESS;
  // get min nodes
  const avl_node_t* min = avl_get_min( manager->tree->root );
  if ( min ) {
    current_top = ( uintptr_t )min->data - THREAD_STACK_MAX_SIZE;
  }
  // check if it is mapped
  if ( virt_is_mapped_range( current_top - THREAD_STACK_MAX_SIZE, THREAD_STACK_MAX_SIZE ) ) {
    return 0;
  }
  // return new one
  return current_top;
}
