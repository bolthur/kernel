
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
  #define THREAD_KERNEL_STACK_START_ADDRESS 0xF3041000
  #define THREAD_KERNEL_STACK_END_ADDRESS 0xF4000000 - STACK_SIZE

  #define THREAD_USER_STACK_START_ADDRESS 0x00001000
  #define THREAD_USER_STACK_END_ADDRESS 0x00200000 - STACK_SIZE
#elif defined( ELF64 )
  #error "Unsupported"
#endif

/**
 * @brief Get next virtual stack address
 *
 * @param manager
 * @return uintptr_t
 */
uintptr_t task_stack_manager_next(
  task_process_type_t type,
  task_stack_manager_ptr_t manager
) {
  if ( NULL == manager ) {
    manager = task_stack_manager;
  }
  // assert manager
  assert( NULL != manager );

  // determine min and max
  uintptr_t current, min_stack, max_stack;
  if ( TASK_PROCESS_TYPE_KERNEL == type ) {
    current = THREAD_KERNEL_STACK_START_ADDRESS;
    min_stack = current;
    max_stack = THREAD_KERNEL_STACK_END_ADDRESS;
  } else {
    current = THREAD_USER_STACK_START_ADDRESS;
    min_stack = current;
    max_stack = THREAD_USER_STACK_END_ADDRESS;
  }

  // get min and max nodes
  avl_node_ptr_t min = avl_get_min( manager->tree->root );
  avl_node_ptr_t max = avl_get_max( manager->tree->root );

  // handle empty
  if ( NULL == min && NULL == max ) {
    return current;
  }

  // find possible hole
  while ( min != max ) {
    // try to find
    avl_node_ptr_t tmp = avl_find_by_data( manager->tree, ( void* )current );
    // not found => free
    if ( NULL == tmp ) {
      return current;
    }
    // next to probe
    current += STACK_SIZE;
  }

  // assert address
  assert( current >= min_stack );
  assert( current <= max_stack );

  // return new one
  return current + STACK_SIZE;
}
