
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
#include <assert.h>
#include <string.h>
#include <arch/arm/stack.h>
#include <kernel/mm/virt.h>
#include <kernel/panic.h>
#include <kernel/task/queue.h>
#include <kernel/task/process.h>
#include <kernel/debug/debug.h>
#include <arch/arm/v7/cpu.h>

/**
 * @brief Task process scheduler
 *
 * @param context cpu context
 *
 * @todo add reset of queues, when all priority queues are handled
 * @todo split up function into multiple smaller ones
 */
void task_process_schedule( void** context ) {
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

  // get running thread
  task_thread_ptr_t running_thread = task_thread_get_current();

  // get running queue if set
  task_priority_queue_ptr_t running_queue = NULL;
  if ( NULL != running_thread ) {
    running_queue = task_queue_get_queue(
      process_manager, running_thread->priority );
    // assert queue
    assert( NULL != running_queue );
    // set last handled within running queue
    running_queue->last_handled = running_thread;
  }

  // variable for next thread
  task_thread_ptr_t next_thread = NULL;
  task_priority_queue_ptr_t next_queue = NULL;

  // loop through priorities and try to get next task
  for(
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
      DEBUG_OUTPUT( "current->last_handled = 0x%08x\r\n", current->last_handled );
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

    // skip if nothing is existing after last handled
    if ( NULL == item ) {
      // prevent endless loop by checking against 0
      if ( 0 == priority ) {
        break;
      }
      // skip if nothing is left
      continue;
    }

    // determine next thread
    next_thread = ( task_thread_ptr_t )item->data;
    // set next queue
    next_queue = current;

    // prevent endless loop by checking against 0
    if ( 0 == priority ) {
      break;
    }
  }

  // FIXME: RESET THREAD LIST AT ALL QUEUES, WHEN ALL LISTS ARE HANDLED AND NO ONE IS LEFT

  // assert next queue and next thread to exist
  assert( NULL != next_queue );
  assert( NULL != next_thread );

  // update last executed within running queue
  if ( NULL != running_queue ) {
    // reset current if queue changed
    if ( running_queue != next_queue ) {
      running_queue->current = NULL;
    }
    // set last handled within running queue
    running_queue->last_handled = running_thread;
  }

  // save context of current thread
  if ( NULL != running_thread ) {
    memcpy(
      running_thread->context,
      *context,
      sizeof( cpu_register_context_t ) );
  }

  // update next queue current
  next_queue->current = next_thread;
  // overwrite current thread
  task_thread_set_current( next_thread );

  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Process of next thread: %d\r\n", next_thread->process->id );
  #endif

  // unmap current thread stack
  virt_unmap_address( kernel_context, THREAD_STACK_ADDRESS, false );
  // Map next thread stack
  virt_map_address(
    kernel_context,
    THREAD_STACK_ADDRESS,
    next_thread->stack,
    VIRT_MEMORY_TYPE_NORMAL,
    VIRT_PAGE_TYPE_EXECUTABLE
  );
  // debug output
  #if defined( PRINT_PROCESS )
    dump_register( *context );
  #endif
  // overwrite context
  memcpy( *context, next_thread->context, sizeof( cpu_register_context_t ) );
  // debug output
  #if defined( PRINT_PROCESS )
    dump_register( *context );
  #endif
}
