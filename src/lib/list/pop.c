
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

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <list.h>

/**
 * @brief Method to pop element from list
 *
 * @param list list to use
 * @return void* data of first element or NULL if empty
 */
void* list_pop( list_manager_ptr_t list ) {
  void* data;
  list_item_ptr_t first;

  // assert list is initialized
  assert( NULL != list );
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
