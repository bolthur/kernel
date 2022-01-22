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
 * @brief Method to get element from list like pop without removal
 *
 * @param list list to use
 * @return void* data of first element or NULL if empty
 */
void* list_peek_front( list_manager_ptr_t list ) {
  list_item_ptr_t first;

  // check parameter
  if ( ! list ) {
    return NULL;
  }
  // get first element
  first = list->first;

  // handle empty list
  if ( ! first ) {
    return NULL;
  }
  // return data of first element
  return first->data;
}

/**
 * @brief Method to get element from list like pop without removal
 *
 * @param list list to use
 * @return void* data of first element or NULL if empty
 */
void* list_peek_back( list_manager_ptr_t list ) {
  list_item_ptr_t last;

  // check parameter
  if ( ! list ) {
    return NULL;
  }
  // get last element
  last = list->last;

  // handle empty list
  if ( ! last ) {
    return NULL;
  }
  // return data of first element
  return last->data;
}
