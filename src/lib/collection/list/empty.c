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
 * @brief Method checks for list is empty
 *
 * @return true empty list
 * @return false at least one item
 */
bool list_empty( list_manager_ptr_t list ) {
  // check parameter
  if ( ! list ) {
    return true;
  }

  // check first and last item
  if (
    ! list->first
    && ! list->last
  ) {
    // list empty
    return true;
  }

  // list not empty
  return false;
}
