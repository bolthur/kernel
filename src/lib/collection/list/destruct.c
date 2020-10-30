
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
#include <collection/list.h>

/**
 * @brief Method to destruct list
 *
 * @param list list to use
 */
void list_destruct( list_manager_ptr_t list ) {
  list_item_ptr_t current, next;

  // check parameter
  if ( ! list ) {
    return;
  }
  // populate current
  current = list->first;

  // loop through list until end
  while ( current ) {
    // get next element
    next = current->next;

    // additional cleanup
    list->cleanup( current );

    // overwrite current with next
    current = next;
  }

  // finally free list
  free( list );
}
