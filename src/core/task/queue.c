
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
#include <stdlib.h>
#include <string.h>
#include <core/debug/debug.h>
#include <core/task/queue.h>

/**
 * @brief Compare id callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t queue_compare_priority_callback(
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
 * @brief Initialize task process manager
 */
avl_tree_ptr_t task_queue_init( void ) {
  return avl_create_tree( queue_compare_priority_callback );
}

/**
 * @brief Get the thread queue object
 *
 * @param manager
 * @param priority
 * @return task_priority_queue_ptr_t
 */
task_priority_queue_ptr_t task_queue_get_queue(
  task_manager_ptr_t manager,
  size_t priority
) {
  // assert manager existance
  assert( NULL != manager );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Called task_queue_get_queue( %d )\r\n", priority );
  #endif
  // get correct tree to use
  avl_tree_ptr_t tree = manager->thread_priority_tree;

  // try to find node
  avl_node_ptr_t node = avl_find_by_data( tree, ( void* )priority );
  task_priority_queue_ptr_t queue;
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Found node 0x%08p\r\n", node );
  #endif
  // handle not yet added
  if ( NULL == node ) {
    // allocate block
    queue = ( task_priority_queue_ptr_t )malloc(
      sizeof( task_priority_queue_t ) );
    // assert initialization
    assert( NULL != queue );
    // prepare memory
    memset( ( void* )queue, 0, sizeof( task_priority_queue_t ) );
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "Initialized new node at 0x%08p\r\n", queue );
    #endif
    // populate queue
    queue->priority = priority;
    queue->thread_list = list_construct();
    queue->current = NULL;
    queue->last_handled = NULL;
    // prepare and insert node
    avl_prepare_node( &queue->node, ( void* )priority );
    avl_insert_by_node( tree, &queue->node );
  // existing? => gather block
  } else {
    queue = TASK_QUEUE_GET_PRIORITY( node );
  }

  // assert existance
  assert( NULL != queue );
  // return queue
  return queue;
}
