
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
#include <kernel/task/process.h>

/**
 * @brief Process management structure
 */
task_process_manager_ptr_t process_manager = NULL;

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
 * @brief Initialize task process manager
 */
void task_process_init( void ) {
  // assert no initialized process manager
  assert( NULL == process_manager );

  // allocate management structures
  process_manager = ( task_process_manager_ptr_t )malloc(
    sizeof( task_process_manager_t )
  );
  // assert malloc result
  assert( NULL != process_manager );
  // prepare structure
  memset( ( void* )process_manager, 0, sizeof( task_process_manager_t ) );

  // create tree for managing processes by id
  process_manager->tree_id = avl_create_tree( compare_id_callback );
}
