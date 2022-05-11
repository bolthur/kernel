/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include "../lib/stdlib.h"
#include "../lib/string.h"
#if defined( PRINT_PROCESS )
  #include "../debug/debug.h"
#endif
#include "queue.h"

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
    DEBUG_OUTPUT( "a = %p, b = %p\r\n", ( void* )a, ( void* )b )
    DEBUG_OUTPUT( "a->data = %zu, b->data = %zu\r\n",
      ( size_t )a->data,
      ( size_t )b->data )
  #endif

  // -1 if address of a->data is greater than address of b->data
  if ( ( size_t )a->data > ( size_t )b->data ) {
    return -1;
  // 1 if address of b->data is greater than address of a->data
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
  return avl_create_tree( queue_compare_priority_callback, NULL, NULL );
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
  // check parameter
  if ( ! manager ) {
    return NULL;
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Called task_queue_get_queue( %zu )\r\n", priority )
  #endif
  // get correct tree to use
  avl_tree_ptr_t tree = manager->thread_priority;

  // try to find node
  avl_node_ptr_t node = avl_find_by_data( tree, ( void* )priority );
  task_priority_queue_ptr_t queue;
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Found node %p\r\n", ( void* )node )
  #endif
  // handle not yet added
  if ( ! node ) {
    // reserve block
    queue = ( task_priority_queue_ptr_t )malloc(
      sizeof( task_priority_queue_t ) );
    // check parameter
    if ( ! queue ) {
      return NULL;
    }
    // prepare memory
    memset( ( void* )queue, 0, sizeof( task_priority_queue_t ) );
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "Initialized new node at %p\r\n", ( void* )queue )
    #endif
    // populate queue
    queue->priority = priority;
    queue->thread_list = list_construct( NULL, NULL, NULL );
    if ( ! queue->thread_list ) {
      free( queue );
      return NULL;
    }
    queue->current = NULL;
    queue->last_handled = NULL;
    // prepare and insert node
    avl_prepare_node( &queue->node, ( void* )priority );
    if ( ! avl_insert_by_node( tree, &queue->node ) ) {
      list_destruct( queue->thread_list );
      free( queue );
      return NULL;
    }
  // existing? => gather block
  } else {
    queue = TASK_QUEUE_GET_PRIORITY( node );
  }

  // return queue
  return queue;
}
