
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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
#include <core/panic.h>
#include <core/debug/debug.h>
#include <core/event.h>
#include <core/task/queue.h>
#include <core/task/thread.h>

/**
 * @brief Current running thread
 */
task_thread_ptr_t task_thread_current_thread = NULL;

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
 * @brief Sets current running thread
 *
 * @param thread thread to set
 * @param queue thread queue
 */
void task_thread_set_current(
  task_thread_ptr_t thread,
  task_priority_queue_ptr_t queue
) {
  // set current
  task_thread_current_thread = thread;
  // update queue
  queue->current = thread;
  // set state
  task_thread_current_thread->state = TASK_THREAD_STATE_ACTIVE;
}

/**
 * @brief Create thread manager for task
 *
 * @return avl_tree_ptr_t
 */
avl_tree_ptr_t task_thread_init( void ) {
  return avl_create_tree( thread_compare_id_callback );
}

/**
 * @brief Destroy thread manager for task
 *
 * @param tree
 */
void task_thread_destroy( avl_tree_ptr_t tree ) {
  avl_destroy_tree( tree );
}

/**
 * @brief Function to get next thread for execution
 *
 * @return task_thread_ptr_t
 */
task_thread_ptr_t task_thread_next( void ) {
  // min / max queue
  task_priority_queue_ptr_t min_queue = NULL;
  task_priority_queue_ptr_t max_queue = NULL;
  avl_node_ptr_t min = NULL;
  avl_node_ptr_t max = NULL;

  // get min and max priority queue
  min = avl_get_min( process_manager->thread_priority_tree->root );
  max = avl_get_max( process_manager->thread_priority_tree->root );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "min: 0x%08p, max: 0x%08p\r\n", min, max );
  #endif

  // get nodes from min/max
  if ( NULL != min ) {
    min_queue = TASK_QUEUE_GET_PRIORITY( min );
  }
  if ( NULL != max ) {
    max_queue = TASK_QUEUE_GET_PRIORITY( max );
  }
  // handle no min or no max queue
  if ( NULL == min_queue || NULL == max_queue ) {
    return NULL;
  }

  // loop through priorities and try to get next task
  for (
    size_t priority = max_queue->priority;
    priority >= min_queue->priority;
    priority--
  ) {
    // try to find queue for priority
    avl_node_ptr_t current_node = avl_find_by_data(
      process_manager->thread_priority_tree,
      ( void* )priority );
    // skip if not existing
    if ( NULL == current_node ) {
      // prevent endless loop by checking against 0
      if ( 0 == priority ) {
        break;
      }
      // skip if no such queue exists
      continue;
    }

    // get queue
    task_priority_queue_ptr_t current = TASK_QUEUE_GET_PRIORITY( current_node );
    // check for no items left or empty list
    if (
      list_empty( current->thread_list )
      || current->last_handled == ( task_thread_ptr_t )current->thread_list->last->data
    ) {
      // prevent endless loop by checking against 0
      if ( 0 == priority ) {
        break;
      }
      // skip if queue is handled
      continue;
    }

    // find next thread in queue ( default case: start with first one )
    list_item_ptr_t item = current->thread_list->first;
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT(
        "current->last_handled = 0x%08x\r\n",
      current->last_handled );
    #endif
    // handle already executed entry
    if ( current->last_handled != NULL ) {
      // try to find element in list
      item = list_lookup_data(
        current->thread_list, ( void* )current->last_handled );
      // debug output
      #if defined( PRINT_PROCESS )
        DEBUG_OUTPUT( "item->data = 0x%08x\r\n", item->data );
      #endif
      // assert result
      assert( NULL != item );
      // head to next
      item = item->next;
    }

    // get next ready task
    while ( NULL != item ) {
      // get task object
      task_thread_ptr_t task = ( task_thread_ptr_t )item->data;
      // check for ready
      if (
        TASK_THREAD_STATE_READY == task->state
        || TASK_THREAD_STATE_HALT_SWITCH == task->state
      ) {
        break;
      }
      // try next if not ready
      item = item->next;
    }

    // skip if nothing is existing after last handled
    if ( NULL == item ) {
      // prevent endless loop by checking against 0
      if ( 0 == priority ) {
        break;
      }
      // skip if nothing is left
      continue;
    }
    // return next thread
    return ( task_thread_ptr_t )item->data;
  }

  // return NULL
  return NULL;
}
