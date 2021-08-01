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
 * @brief Helper for creating a list node
 *
 * @param data data to populate
 * @return list_item_ptr_t pointer to created node
 */
list_item_ptr_t list_node_create( void* data ) {
  // allocate new node
  list_item_ptr_t node = ( list_item_ptr_t )malloc( sizeof( list_item_t ) );
  // check malloc result
  if ( ! node ) {
    return NULL;
  }
  // overwrite allocated memory with 0
  memset( ( void* )node, 0, sizeof( list_item_t ) );

  // populate created node
  node->next = NULL;
  node->previous = NULL;
  node->data = data;

  // return created node
  return node;
}
