/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#include <task/stack.h>
#include <arch/arm/stack.h>

#if defined( ELF32 )
  #define THREAD_STACK_START_ADDRESS 0x00001000
  #define THREAD_STACK_END_ADDRESS 0x00200000 - STACK_SIZE
#elif defined( ELF64 )
  #error "Unsupported"
#endif

/**
 * @brief Get next virtual stack address
 *
 * @param manager
 * @return uintptr_t
 *
 * @todo revise stack handling
 */
uintptr_t task_stack_manager_next( task_stack_manager_ptr_t manager ) {
  // check parameter
  if ( ! manager ) {
    return 0;
  }

  // determine min and max
  uintptr_t current = THREAD_STACK_START_ADDRESS;
  uintptr_t min_stack = current;
  uintptr_t max_stack = THREAD_STACK_END_ADDRESS;

  // get min and max nodes
  avl_node_ptr_t min = avl_get_min( manager->tree->root );
  avl_node_ptr_t max = avl_get_max( manager->tree->root );

  // handle empty
  if ( ! min && ! max ) {
    return current;
  }

  // find possible hole
  while ( min != max ) {
    // try to find
    avl_node_ptr_t tmp = avl_find_by_data( manager->tree, ( void* )current );
    // not found => free
    if ( ! tmp ) {
      return current;
    }
    // next to probe
    current += STACK_SIZE;
  }

  // check address
  if (
    current < min_stack
    || current > max_stack
  ) {
    return 0;
  }

  // return new one
  return current + STACK_SIZE;
}
