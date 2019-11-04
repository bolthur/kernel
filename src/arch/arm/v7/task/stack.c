
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

#include <assert.h>
#include <kernel/debug/debug.h>
#include <kernel/task/stack.h>
#include <arch/arm/stack.h>

#if defined( ELF32 )
  #define THREAD_STACK_START_ADDRESS 0xF3041000
  #define THREAD_STACK_END_ADDRESS 0xF3FFFFFF - STACK_SIZE
#elif defined( ELF64 )
  #error "Unsupported"
#endif

/**
 * @brief Get next virtual stack address
 *
 * @param manager
 * @return uintptr_t
 */
uintptr_t task_stack_manager_next( void ) {
  // assert manager
  assert( NULL != task_stack_manager );

  avl_node_ptr_t min = avl_get_min( task_stack_manager->tree->root );
  avl_node_ptr_t max = avl_get_max( task_stack_manager->tree->root );
  uintptr_t current = THREAD_STACK_START_ADDRESS;

  // handle empty
  if ( NULL == min && NULL == max ) {
    return current;
  }

  // find possible hole
  while ( min != max ) {
    // try to find
    avl_node_ptr_t tmp = avl_find_by_data(
      task_stack_manager->tree, ( void* )current );

    // not found => free
    if ( NULL == tmp ) {
      return current;
    }

    // next to probe
    current += STACK_SIZE;
  }

  // assert address
  assert( current >= THREAD_STACK_START_ADDRESS );
  assert( current <= THREAD_STACK_END_ADDRESS );

  // return new one
  return current + STACK_SIZE;
}
