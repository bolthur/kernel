
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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <collection/list.h>

/**
 * @brief Default lookup if not passed during creation
 *
 * @param a
 * @param b
 * @return int32_t
 */
int32_t list_default_lookup(
  const list_item_ptr_t a,
  const void* b
) {
  return a->data == b ? 0 : 1;
}

/**
 * @brief Default cleanup if not passed during creation
 *
 * @param a
 */
void list_default_cleanup(
  const list_item_ptr_t a
) {
  // free current element
  free( a );
}

/**
 * @brief Method to construct list
 *
 * @param lookup
 * @param cleanup
 * @return list_manager_ptr_t pointer to created list
 */
list_manager_ptr_t list_construct(
  list_lookup_func_t lookup,
  list_cleanup_func_t cleanup
) {
  list_manager_ptr_t list;

  // allocate list
  list = ( list_manager_ptr_t )malloc( sizeof( list_manager_t ) );
  // handle error
  if ( ! list ) {
    return NULL;
  }
  // overwrite with zero
  memset( ( void* )list, 0, sizeof( list_manager_t ) );

  // preset elements
  list->first = NULL;
  list->last = NULL;
  // lookup function
  if( lookup ) {
    list->lookup = lookup;
  } else {
    list->lookup = list_default_lookup;
  }
  // cleanup function
  if( cleanup ) {
    list->cleanup = cleanup;
  } else {
    list->cleanup = list_default_cleanup;
  }

  // return created list
  return list;
}
