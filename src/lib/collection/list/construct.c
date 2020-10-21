
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

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <list.h>

/**
 * @brief Method to construct list
 *
 * @return list_manager_ptr_t pointer to created list
 */
list_manager_ptr_t list_construct( void ) {
  list_manager_ptr_t list;

  // allocate list
  list = ( list_manager_ptr_t )malloc( sizeof( list_manager_t ) );
  // assert malloc result
  assert( NULL != list );
  // overwrite with zero
  memset( ( void* )list, 0, sizeof( list_manager_t ) );

  // preset elements
  list->first = NULL;
  list->last = NULL;

  // return created list
  return list;
}
