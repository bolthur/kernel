
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
#include <string.h>
#include <stdlib.h>
#include <kernel/debug/debug.h>
#include <kernel/task/stack.h>

/**
 * @brief Stack management structure
 */
task_stack_manager_ptr_t task_stack_manager = NULL;

/**
 * @brief Compare stack callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t task_stack_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "a = 0x%08p, b = 0x%08p\r\n", a, b );
    DEBUG_OUTPUT( "a->data = 0x%08x, b->data = 0x%08x\r\n",
      ( uintptr_t )a->data,
      ( uintptr_t )b->data );
  #endif

  // -1 if address of a is greater than address of b
  if ( ( uintptr_t )a->data > ( uintptr_t )b->data ) {
    return -1;
  // 1 if address of b is greater than address of a
  } else if ( ( uintptr_t )b->data > ( uintptr_t )a->data ) {
    return 1;
  }

  // equal => return 0
  return 0;
}

/**
 * @brief Initialize stack manager
 */
void task_stack_manager_init( void ) {
  // allocate manager
  task_stack_manager = ( task_stack_manager_ptr_t )malloc(
    sizeof( task_stack_manager_t ) );
  // assert allocation
  assert( NULL != task_stack_manager );
  // prepare
  memset( ( void* )task_stack_manager, 0, sizeof( task_stack_manager_t ) );
  // prepare tree
  task_stack_manager->tree = avl_create_tree( task_stack_callback );
}

void task_stack_manager_add( uintptr_t stack ) {
  // create node
  avl_node_ptr_t node = ( avl_node_ptr_t )malloc( sizeof( avl_node_t ) );
  // assert allocation
  assert( NULL != node );
  // prepare allocated space
  memset( ( void* )node, 0, sizeof( avl_node_t ) );
  // preparre node
  avl_prepare_node( node, ( void* )stack );
  /// insert node
  avl_insert_by_node( task_stack_manager->tree, node );
}
