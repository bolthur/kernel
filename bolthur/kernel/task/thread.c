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

#include <stdlib.h>
#include <string.h>
#if defined( PRINT_PROCESS )
  #include <debug/debug.h>
#endif
#include <event.h>
#include <task/queue.h>
#include <task/thread.h>
#include <task/stack.h>
#include <mm/virt.h>

/**
 * @brief Current running thread
 * @todo Transform to pointer to multiple threads ( depending on cpu size )
 */
task_thread_ptr_t task_thread_current_thread = NULL;

/**
 * @fn int32_t thread_compare_id_callback(const avl_node_ptr_t, const avl_node_ptr_t)
 * @brief Helper necessary for avl thread manager tree
 *
 * @param a node a
 * @param b node b
 * @return
 */
static int32_t thread_compare_id_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "a = %p, b = %p\r\n", ( void* )a, ( void* )b );
    DEBUG_OUTPUT( "a->data = %zu, b->data = %zu\r\n",
      ( size_t )a->data,
      ( size_t )b->data );
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
 * @fn void thread_destroy_callback(const avl_node_ptr_t)
 * @brief Helper to destroy avl node
 *
 * @param a
 */
static void thread_destroy_callback( const avl_node_ptr_t node ) {
  // get thread and context
  task_thread_ptr_t thread = TASK_THREAD_GET_BLOCK( node );
  task_process_ptr_t proc = thread->process;
  virt_context_ptr_t ctx = proc->virtual_context;
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Destroy thread with id %d of process with id %d!\r\n",
      thread->id,
      thread->process->id );
  #endif
  // unmap thread stack
  while (
    proc->virtual_context
    && ! virt_unmap_address( ctx, thread->stack_virtual, true )
  ) {
    // loop until successful unmapped!
  }
  // get thread queue by priority
  task_priority_queue_ptr_t queue = task_queue_get_queue(
    process_manager, proc->priority );
  while( queue && ! list_remove_data( queue->thread_list, thread ) ) {
    // loop until successfully removed
  }
  // remove from stack address from manager
  while ( ! task_stack_manager_remove(
    thread->stack_virtual,
    proc->thread_stack_manager
  ) ) {
    // wait until removal was successful
  }
  // free allocated space
  if ( thread->current_context ) {
    free( thread->current_context );
  }
  free( thread );
}

/**
 * @fn pid_t task_thread_generate_id(task_process_ptr_t)
 * @brief Method to generate new thread id
 *
 * @param proc process to generate id for
 * @return
 */
pid_t task_thread_generate_id( task_process_ptr_t proc ) {
  // return new pid by simple increment
  return ++proc->current_thread_id;
}

/**
 * @fn bool task_thread_set_current(task_thread_ptr_t, task_priority_queue_ptr_t)
 * @brief Sets current running thread
 *
 * @param thread
 * @param queue
 * @return
 */
bool task_thread_set_current(
  task_thread_ptr_t thread,
  task_priority_queue_ptr_t queue
) {
  // check parameter
  if ( ! thread || ! queue ) {
    return false;
  }
  // set current thread
  task_thread_current_thread = thread;
  // update queue current
  queue->current = thread;
  // set state
  if ( task_thread_current_thread->state )
  task_thread_current_thread->state =
    task_thread_current_thread->state == TASK_THREAD_STATE_RPC_QUEUED
      ? TASK_THREAD_STATE_RPC_ACTIVE
      : TASK_THREAD_STATE_ACTIVE;
  task_thread_current_thread->process->state =
    task_thread_current_thread->process->state == TASK_PROCESS_STATE_RPC_QUEUED
      ? TASK_PROCESS_STATE_RPC_ACTIVE
      : TASK_PROCESS_STATE_ACTIVE;
  return true;
}

/**
 * @brief Reset current process
 *
 * @todo Rename to task_reset and move to somewhere more generic
 */
void task_thread_reset_current( void ) {
  // reset queue
  task_process_queue_reset();
  // set state
  if ( task_thread_current_thread ) {
    if ( TASK_THREAD_STATE_HALT_SWITCH == task_thread_current_thread->state ) {
      task_thread_current_thread->state = TASK_THREAD_STATE_READY;
    } else if ( TASK_THREAD_STATE_RPC_HALT_SWITCH == task_thread_current_thread->state ) {
      task_thread_current_thread->state = TASK_THREAD_STATE_RPC_QUEUED;
    }
    if ( TASK_PROCESS_STATE_HALT_SWITCH == task_thread_current_thread->process->state ) {
      task_thread_current_thread->process->state = TASK_PROCESS_STATE_READY;
    } else if ( TASK_PROCESS_STATE_RPC_HALT_SWITCH == task_thread_current_thread->process->state ) {
      task_thread_current_thread->process->state = TASK_PROCESS_STATE_RPC_QUEUED;
    }
  }
  // unset current thread
  task_thread_current_thread = NULL;
}

/**
 * @fn avl_tree_ptr_t task_thread_init(void)
 * @brief Create thread manager for task
 *
 * @return
 */
avl_tree_ptr_t task_thread_init( void ) {
  return avl_create_tree(
    thread_compare_id_callback,
    NULL,
    thread_destroy_callback
  );
}

/**
 * @fn void task_thread_destroy(avl_tree_ptr_t)
 * @brief Destroy thread manager tree
 *
 * @param tree
 */
void task_thread_destroy( avl_tree_ptr_t tree ) {
  avl_destroy_tree( tree );
}

/**
 * @fn task_thread_ptr_t task_thread_next(void)
 * @brief Function to get next thread for execution
 *
 * @return
 */
task_thread_ptr_t task_thread_next( void ) {
  // check process manager
  if ( ! process_manager ) {
    return NULL;
  }

  // min / max queue
  task_priority_queue_ptr_t min_queue = NULL;
  task_priority_queue_ptr_t max_queue = NULL;
  avl_node_ptr_t min = NULL;
  avl_node_ptr_t max = NULL;

  // get min and max priority queue
  min = avl_get_min( process_manager->thread_priority->root );
  max = avl_get_max( process_manager->thread_priority->root );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "min: %p, max: %p\r\n", ( void* )min, ( void* )max );
  #endif

  // get nodes from min/max
  if ( min ) {
    min_queue = TASK_QUEUE_GET_PRIORITY( min );
  }
  if ( max ) {
    max_queue = TASK_QUEUE_GET_PRIORITY( max );
  }
  // handle no min or no max queue
  if ( ! min_queue || ! max_queue ) {
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
      process_manager->thread_priority,
      ( void* )priority );
    // skip if not existing
    if ( ! current_node ) {
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
        "current->last_handled = %p\r\n",
        ( void* )current->last_handled );
    #endif
    // handle already executed entry
    if ( current->last_handled ) {
      // try to find element in list
      item = list_lookup_data(
        current->thread_list, ( void* )current->last_handled );
      // check return
      if ( ! item ) {
        // prevent endless loop by checking against 0
        if ( 0 == priority ) {
          break;
        }
        // skip due to an error
        continue;
      }
      // debug output
      #if defined( PRINT_PROCESS )
        DEBUG_OUTPUT( "item->data = %p\r\n", item->data );
      #endif
      // head to next
      item = item->next;
    }

    // get next ready task
    while ( item ) {
      // get task object
      task_thread_ptr_t task = ( task_thread_ptr_t )item->data;
      task_process_ptr_t proc = ( task_process_ptr_t )task->process;
      // check for ready
      if (
        (
          TASK_THREAD_STATE_READY == task->state
          || TASK_THREAD_STATE_HALT_SWITCH == task->state
          || TASK_THREAD_STATE_RPC_QUEUED == task->state
          || TASK_THREAD_STATE_RPC_HALT_SWITCH == task->state
        ) && (
          TASK_PROCESS_STATE_READY == proc->state
          || TASK_PROCESS_STATE_HALT_SWITCH == proc->state
          || TASK_PROCESS_STATE_RPC_QUEUED == proc->state
          || TASK_PROCESS_STATE_RPC_HALT_SWITCH == proc->state
        )
      ) {
        break;
      }
      // try next if not ready
      item = item->next;
    }

    // skip if nothing is existing after last handled
    if ( ! item ) {
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

  return NULL;
}

/**
 * @fn void task_thread_cleanup(event_origin_t, void*)
 * @brief thread cleanup handling
 *
 * @param origin
 * @param context
 */
void task_thread_cleanup(
  __unused event_origin_t origin,
  __unused void* context
) {
  list_item_ptr_t current = process_manager->process_to_cleanup->first;
  // loop
  while ( current ) {
    // get process from item
    task_thread_ptr_t thread = ( task_thread_ptr_t )current->data;
    // skip running
    if ( thread->state != TASK_THREAD_STATE_KILL ) {
      continue;
    }
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "Cleanup thread with id %d of process with id %d!\r\n",
        thread->id,
        thread->process->id );
    #endif
    // cache current as remove
    list_item_ptr_t remove = current;
    // head over to next
    current = current->next;
    // remove node from tree and cleanup
    avl_remove_by_node( thread->process->thread_manager, &thread->node_id );
    thread->process->thread_manager->cleanup( &thread->node_id );
    // remove list item
    list_remove( process_manager->thread_to_cleanup, remove );
  }
}

/**
 * @fn void task_thread_block(task_thread_ptr_t, task_thread_state_t, task_state_data_t)
 * @brief Block a thread with specific state and data
 *
 * @param thread
 * @param state
 * @param data
 */
void task_thread_block(
  task_thread_ptr_t thread,
  task_thread_state_t state,
  task_state_data_t data
) {
  // no block if state is not ready or active
  if (
    TASK_THREAD_STATE_READY != thread->state
    && TASK_THREAD_STATE_ACTIVE != thread->state
    && TASK_THREAD_STATE_RPC_ACTIVE != thread->state
  ) {
    return;
  }
  // set state and data
  thread->state = state;
  thread->state_data = data;
}

/**
 * @fn void task_thread_unblock(task_thread_ptr_t, task_thread_state_t, task_state_data_t)
 * @brief Unblock a thread with set state passed as parameter
 *
 * @param thread
 * @param necessary_state
 * @param necessary_data
 */
void task_thread_unblock(
  task_thread_ptr_t thread,
  task_thread_state_t necessary_state,
  task_state_data_t necessary_data
) {
  // validate state
  if (
    necessary_state != thread->state
    || (
      necessary_state != TASK_THREAD_STATE_WAITING_FOR_MESSAGE
      && necessary_state != TASK_THREAD_STATE_RPC_WAITING
    )
  ) {
    return;
  }
  // validate data
  if ( thread->state_data.data_ptr != necessary_data.data_ptr ) {
    return;
  }
  // set back to ready
  thread->state = TASK_THREAD_STATE_READY;
}
