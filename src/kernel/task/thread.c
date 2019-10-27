
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <kernel/debug/debug.h>
#include <kernel/event.h>
#include <kernel/task/thread.h>

/**
 * @brief Current running thread
 */
task_thread_ptr_t current = NULL;

/**
 * @brief Compare id callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t thread_compare_id_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "a = 0x%08p, b = 0x%08p\r\n", a, b );
    DEBUG_OUTPUT( "a->data = %d, b->data = %d\r\n",
      ( size_t )a->data,
      ( size_t )b->data );
  #endif

  // -1 if address of a is greater than address of b
  if ( ( size_t )a->data > ( size_t )b->data ) {
    return -1;
  // 1 if address of b is greater than address of a
  } else if ( ( size_t )b->data > ( size_t )a->data ) {
    return 1;
  }

  // equal => return 0
  return 0;
}

/**
 * @brief Method to generate new thread id
 *
 * @return size_t generated thread id
 */
size_t task_thread_generate_id( void ) {
  // current pid
  static size_t current = 0;
  // return new pid by simple increment
  return ++current;
}

/**
 * @brief Create thread manager for task
 *
 * @return avl_tree_ptr_t
 */
avl_tree_ptr_t task_thread_init( void ) {
  return avl_create_tree( thread_compare_id_callback );
}
