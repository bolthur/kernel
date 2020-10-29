
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
#include <core/event.h>
#include <core/elf/common.h>
#include <core/debug/debug.h>
#include <core/task/queue.h>
#include <core/task/process.h>
#include <core/task/thread.h>
#include <core/task/stack.h>

/**
 * @brief Process management structure
 */
task_manager_ptr_t process_manager = NULL;

/**
 * @brief Compare id callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t process_compare_id_callback(
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
 * @return true
 * @return false
 */
bool task_process_init( void ) {
  // check parameter
  if ( NULL != process_manager ) {
    return false;
  }

  // allocate management structures
  process_manager = ( task_manager_ptr_t )malloc( sizeof( task_manager_t ) );
  // check parameter
  if ( NULL == process_manager ) {
    return false;
  }
  // prepare structure
  memset( ( void* )process_manager, 0, sizeof( task_manager_t ) );

  // create tree for managing processes by id
  process_manager->tree_process_id = avl_create_tree(
    process_compare_id_callback );
  // handle error
  if ( NULL == process_manager->tree_process_id ) {
    free( process_manager );
    return false;
  }
  // create thread queue tree
  process_manager->thread_priority_tree = task_queue_init();
  // handle error
  if ( NULL == process_manager->thread_priority_tree ) {
    avl_destroy_tree( process_manager->tree_process_id );
    free( process_manager );
    return false;
  }

  // register timer event
  if ( ! event_bind( EVENT_TIMER, task_process_schedule, true ) ) {
    avl_destroy_tree( process_manager->thread_priority_tree );
    avl_destroy_tree( process_manager->tree_process_id );
    free( process_manager );
    return false;
  }
  return true;
}

/**
 * @brief Method to generate new process id
 *
 * @return size_t generated process id
 */
size_t task_process_generate_id( void ) {
  // current pid
  static size_t current = 0;
  // return new pid by simple increment
  return ++current;
}

/**
 * @brief Method to create new process
 *
 * @param entry process entry address
 * @param priority process priority
 * @return true
 * @return false
 */
bool task_process_create( uintptr_t entry, size_t priority ) {
  // check manager
  if ( NULL == process_manager ) {
    return false;
  }

  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "task_process_create( %p, %zu ) called\r\n", ( void* )entry, priority );
  #endif

  // check for valid header
  if ( ! elf_check( entry ) ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "No valid elf header found\r\n" );
    #endif
    // return
    return false;
  }

  // allocate process structure
  task_process_ptr_t process = ( task_process_ptr_t )malloc(
    sizeof( task_process_t ) );
  // check allocation
  if ( NULL == process ) {
    return false;
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Allocated process structure at %p\r\n", ( void* )process );
  #endif

  // prepare structure
  memset( ( void* )process, 0, sizeof( task_process_t ) );
  // populate process structure
  process->id = task_process_generate_id();
  process->thread_manager = task_thread_init();
  // handle error
  if ( NULL == process->thread_manager ) {
    free( process );
    return false;
  }
  process->state = TASK_PROCESS_STATE_READY;
  process->priority = priority;
  process->thread_stack_manager = task_stack_manager_create();
  // handle error
  if ( NULL == process->thread_stack_manager ) {
    task_thread_destroy( process->thread_manager );
    free( process );
    return false;
  }
  // create context only for user processes
  process->virtual_context = virt_create_context( VIRT_CONTEXT_TYPE_USER );
  // handle error
  if ( NULL == process->virtual_context ) {
    task_stack_manager_destroy( process->thread_stack_manager );
    task_thread_destroy( process->thread_manager );
    free( process );
    return false;
  }

  // load elf executable
  uintptr_t program_entry = elf_load( entry, process );
  // handle error
  if ( 0 == program_entry ) {
    virt_destroy_context( process->virtual_context );
    task_stack_manager_destroy( process->thread_stack_manager );
    task_thread_destroy( process->thread_manager );
    free( process );
    return false;
  }

  // prepare node
  avl_prepare_node( &process->node_id, ( void* )process->id );
  // add process to tree
  if ( ! avl_insert_by_node( process_manager->tree_process_id, &process->node_id ) ) {
    virt_destroy_context( process->virtual_context );
    task_stack_manager_destroy( process->thread_stack_manager );
    task_thread_destroy( process->thread_manager );
    free( process );
    return false;
  }

  // Setup thread with entry
  if ( NULL == task_thread_create( program_entry, process, priority ) ) {
    avl_remove_by_node( process_manager->tree_process_id, &process->node_id );;
    virt_destroy_context( process->virtual_context );
    task_stack_manager_destroy( process->thread_stack_manager );
    task_thread_destroy( process->thread_manager );
    free( process );
    return false;
  }
  return true;
}

/**
 * @brief Resets process priority queues
 */
void task_process_queue_reset( void ) {
  // min / max queue
  task_priority_queue_ptr_t min_queue = NULL;
  task_priority_queue_ptr_t max_queue = NULL;
  avl_node_ptr_t min = NULL;
  avl_node_ptr_t max = NULL;

  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "task_process_queue_reset()\r\n" );
  #endif

  // get min and max priority queue
  min = avl_get_min( process_manager->thread_priority_tree->root );
  max = avl_get_max( process_manager->thread_priority_tree->root );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "min: %p, max: %p\r\n", ( void* )min, ( void* )max );
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
    // check for empty
    if ( list_empty( current->thread_list ) ) {
      // prevent endless loop by checking against 0
      if ( 0 == priority ) {
        break;
      }
      // skip if queue is handled
      continue;
    }

    // reset last handled
    current->last_handled = NULL;
    // prevent endless loop by checking against 0
    if ( 0 == priority ) {
      break;
    }
  }
}
