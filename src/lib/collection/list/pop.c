
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
#include <list.h>

/**
 * @brief Method to pop element from list
 *
 * @param list list to use
 * @return void* data of first element or NULL if empty
 */
void* list_pop_front( list_manager_ptr_t list ) {
  void* data;
  list_item_ptr_t first;

  // check parameter
  if ( NULL == list ) {
    return NULL;
  }
  // get first element
  first = list->first;

  // handle empty list
  if ( NULL == first ) {
    return NULL;
  }

  // cache data of first element
  data = first->data;
  // change previous of next element if existing
  if ( NULL != first->next ) {
    // change previous
    first->next->previous = NULL;
  }

  // change list to next to remove first element from list
  list->first = first->next;
  // change last if no next element is existing
  if ( NULL == list->first || NULL == list->first->next ) {
    list->last = list->first;
  }

  // free first element
  free( ( void* )first );
  // return set data
  return data;
}

/**
 * @brief Method to pop element from list
 *
 * @param list list to use
 * @return void* data of first element or NULL if empty
 */
void* list_pop_back( list_manager_ptr_t list ) {
  void* data;
  list_item_ptr_t last;

  // check parameter
  if ( NULL == list ) {
    return NULL;
  }
  // get last element
  last = list->last;

  // handle empty list
  if ( NULL == last ) {
    return NULL;
  }

  // cache data of first element
  data = last->data;
  // change next of previous element if existing
  if ( NULL != last->previous ) {
    // change previous
    last->previous->next = NULL;
  }

  // change list to next to remove first element from list
  list->last = last->previous;
  // change first if no next element is existing
  if ( NULL == list->last || NULL == list->last->next ) {
    list->first = list->last;
  }

  // free first element
  free( ( void* )last );
  // return set data
  return data;
}
