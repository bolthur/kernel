
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

#include <avl.h>
#include <arch/arm/stack.h>
#include <kernel/panic.h>
#include <kernel/task/queue.h>
#include <kernel/task/process.h>
#include <kernel/debug/debug.h>

/**
 * @brief Task process scheduler
 *
 * @param context cpu context
 *
 * @todo add logic
 */
void task_process_schedule( __unused void** context ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Entered task_process_schedule( 0x%08p )\r\n", context );
  #endif
  // min / max queue
  task_priority_queue_ptr_t min_queue = NULL;
  task_priority_queue_ptr_t max_queue = NULL;

  // get min and max priority queue
  avl_node_ptr_t min = avl_get_min(
    process_manager->thread_priority_tree->root );
  avl_node_ptr_t max = avl_get_max(
    process_manager->thread_priority_tree->root );
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
    return;
  }
  // loop through priorities
  for(
    size_t priority = min_queue->priority;
    priority <= max_queue->priority;
    priority++
  ) {
    // try to find queue for priority
    avl_node_ptr_t current_node = avl_find_by_data(
      process_manager->thread_priority_tree,
      ( void* )priority );
    // skip if not existing
    if ( NULL == current_node ) {
      continue;
    }
    // get queue
    task_priority_queue_ptr_t current = TASK_QUEUE_GET_PRIORITY( current_node );
    ( void )current;
    // FIXME: GET NEXT
  }

  // on thread / process switch map process stack temporary and copy over stack content
}
