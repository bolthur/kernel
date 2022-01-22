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
 * @brief Search a list item by data
 *
 * @param list list to lookup
 * @param data data to find
 * @return list_item_ptr_t
 */
list_item_ptr_t list_lookup_data( list_manager_ptr_t list, void* data ) {
  list_item_ptr_t current;

  // check parameter
  if ( ! list ) {
    return NULL;
  }
  // populate current
  current = list->first;

  // loop through list until end
  while ( current ) {
    if ( 0 == list->lookup( current, data ) ) {
      return current;
    }
    // check next one
    current = current->next;
  }

  // return not found
  return NULL;
}

/**
 * @brief Search a list item by item
 *
 * @param list list to lookup
 * @param item item to find
 * @return list_item_ptr_t
 */
list_item_ptr_t list_lookup_item( list_manager_ptr_t list, list_item_ptr_t item ) {
  list_item_ptr_t current;

  // check parameter
  if ( ! list ) {
    return NULL;
  }
  // populate current
  current = list->first;


  // loop through list until end
  while ( current ) {
    if ( item == current ) {
      return current;
    }
    // check next one
    current = current->next;
  }

  // return not found
  return NULL;
}
