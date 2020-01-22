
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
 * @brief Remove list item
 *
 * @param list
 * @param item
 */
void list_remove( list_manager_ptr_t list, list_item_ptr_t item ) {
  // assert existance
  assert( NULL != list && NULL != item );

  // stop if not existing
  if ( NULL == list_lookup_item( list, item ) ) {
    return;
  }

  // set previous of next
  if ( NULL != item->next ) {
    item->next->previous = item->previous;
  }

  // set next of previous
  if ( NULL != item->previous ) {
    item->previous->next = item->next;
  }

  // handle head removal
  if ( item == list->first ) {
    list->first = item->next;
  }
  // handle foot removal
  if ( item == list->last ) {
    list->last = item->previous;
  }
}
