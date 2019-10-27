
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
#include <kernel/task/thread.h>

/**
 * @brief Compare id callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_id_callback(
  __unused const avl_node_ptr_t a,
  __unused const avl_node_ptr_t b
) {
  return 0;
}

/**
 * @brief Create thread manager for task
 *
 * @return task_thread_manager_ptr_t
 */
task_thread_manager_ptr_t task_thread_init( void ) {
  // allocate thread manager
  task_thread_manager_ptr_t manager = ( task_thread_manager_ptr_t )malloc(
    sizeof( task_thread_manager_t ) );
  // assert malloc result
  assert( NULL != manager );

  // prepare structure
  memset( ( void* )manager, 0, sizeof( task_thread_manager_t ) );
  // populate
  manager->thread = avl_create_tree( compare_id_callback );

  // return manager
  return manager;
}
