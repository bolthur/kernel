
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
#include <kernel/event.h>
#include <kernel/debug/debug.h>
#include <kernel/task/process.h>
#include <kernel/task/thread.h>

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
static int32_t compare_id_callback(
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
void task_process_init( void ) {
  // assert no initialized process manager
  assert( NULL == process_manager );

  // allocate management structures
  process_manager = ( task_manager_ptr_t )malloc( sizeof( task_manager_t ) );
  // assert malloc result
  assert( NULL != process_manager );
  // prepare structure
  memset( ( void* )process_manager, 0, sizeof( task_manager_t ) );

  // create tree for managing processes by id
  process_manager->tree_process_id = avl_create_tree( compare_id_callback );
  // create thread list
  process_manager->thread_list = list_construct();

  // register timer event
  event_bind( EVENT_TIMER, task_process_schedule );
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
 * @param type process type
 */
void task_process_create(
  uintptr_t entry,
  task_process_type_t type
) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "task_process_create( 0x%08x, %d ) called\r\n",
      entry, type );
  #endif

  // allocate process structure
  task_process_ptr_t process = ( task_process_ptr_t )malloc(
    sizeof( task_process_t ) );
  // assert initialization
  assert( NULL != process );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Allocated process structure at 0x%08x\r\n", process );
  #endif

  // prepare structure
  memset( ( void* )process, 0, sizeof( task_process_t ) );
  // populate process structure
  process->id = task_process_generate_id();
  process->thread_manager = task_thread_init();
  process->type = type;
  // create context
  process->virtual_context = virt_create_context(
    TASK_PROCESS_TYPE_USER == type
      ? VIRT_CONTEXT_TYPE_USER
      : VIRT_CONTEXT_TYPE_KERNEL );
  // prepare node
  avl_prepare_node( &process->node_id, ( void* )process->id );

  // Setup thread with entry
  task_thread_ptr_t thread = task_thread_create( entry, process );
  // add to tree
  avl_insert_by_node( process_manager->tree_process_id, &process->node_id );

  // add thread to thread list for switching
  list_push( process_manager->thread_list, ( void* )thread );
}
