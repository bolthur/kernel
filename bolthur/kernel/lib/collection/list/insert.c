/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include "../list.h"

/**
 * @fn bool list_default_insert(list_manager_ptr_t, void*)
 * @brief Default insert callback
 *
 * @param list
 * @param data
 * @return
 */
bool list_default_insert( list_manager_ptr_t list, void* data ) {
  return list_push_back( list, data );
}


/**
 * @fn bool list_insert(list_manager_ptr_t, void*)
 * @brief Insert data with insert callback
 *
 * @param list
 * @param data
 * @return
 */
bool list_insert( list_manager_ptr_t list, void* data ) {
  return list->insert( list, data );
}

/**
 * @fn bool list_insert_before(list_manager_ptr_t, list_item_ptr_t, void*)
 * @brief Insert data before item
 *
 * @param list
 * @param item
 * @param data
 * @return
 */
bool list_insert_before(
  list_manager_ptr_t list,
  list_item_ptr_t item,
  void* data
) {
  // handle item is first one
  if ( list->first == item ) {
    return list_push_front( list, data );
  }
  // create new node
  list_item_ptr_t to_insert = list_node_create( data );
  // handle error
  if ( ! to_insert ) {
    return false;
  }
  // cache next item
  list_item_ptr_t next = item->next;
  // set previous if next is valid
  if ( next ) {
    next->previous = to_insert;
  }
  // set next passed item
  item->next = to_insert;
  // set next and previous of item to insert
  to_insert->next = next;
  to_insert->previous = item;
  // success
  return true;
}
