
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

#include <collection/list.h>

/**
 * @brief Method to push node with data into list
 *
 * @param list list to use
 * @param data data to push into list
 * @return true
 * @return false
 */
bool list_push_front( list_manager_ptr_t list, void* data ) {
  list_item_ptr_t first, node;

  // handle invalid parameter
  if ( ! list || ! data ) {
    return false;
  }
  // set list head
  first = list->first;

  // create new node
  node = list_node_create( data );
  // handle error
  if ( ! node ) {
    return false;
  }

  // set next to first
  node->next = first;
  // set previous for first element
  if ( first ) {
    first->previous = node;
  }

  // overwrite first element within list pointer
  list->first = node;
  // set last element if NULL
  if ( ! list->last ) {
    list->last = list->first;
  }

  return true;
}

/**
 * @brief Method to push node with data into list
 *
 * @param list list to use
 * @param node item to push
 * @return true
 * @return false
 */
bool list_push_front_node( list_manager_ptr_t list, list_item_ptr_t node ) {
  list_item_ptr_t first;

  // handle invalid parameter
  if ( ! list || ! node ) {
    return false;
  }
  // set list head
  first = list->first;

  // set next to first
  node->next = first;
  // set previous for first element
  if ( first ) {
    first->previous = node;
  }

  // overwrite first element within list pointer
  list->first = node;
  // set last element if NULL
  if ( ! list->last ) {
    list->last = list->first;
  }

  return true;
}

/**
 * @brief Method to push node with data into list
 *
 * @param list list to use
 * @param data data to push into list
 * @return true
 * @return false
 */
bool list_push_back( list_manager_ptr_t list, void* data ) {
  list_item_ptr_t last, node;

  // handle invalid parameter
  if ( ! list || ! data ) {
    return false;
  }
  // set list head
  last = list->last;

  // create new node
  node = list_node_create( data );
  // handle error
  if ( ! node ) {
    return false;
  }

  // set previous to last
  node->previous = last;
  // set next for last element
  if ( last ) {
    last->next = node;
  }

  // overwrite last element within list pointer
  list->last = node;
  // set first element if NULL
  if ( ! list->first ) {
    list->first = list->last;
  }

  return true;
}

/**
 * @brief Method to push node with data into list
 *
 * @param list list to use
 * @param node node to push into list
 * @return true
 * @return false
 */
bool list_push_back_node( list_manager_ptr_t list, list_item_ptr_t node ) {
  list_item_ptr_t last;
  // handle invalid parameter
  if ( ! list || ! node ) {
    return false;
  }

  // set list head
  last = list->last;

  // set previous to last
  node->previous = last;
  // set next for last element
  if ( last ) {
    last->next = node;
  }

  // overwrite last element within list pointer
  list->last = node;
  // set first element if NULL
  if ( ! list->first ) {
    list->first = list->last;
  }

  return true;
}
