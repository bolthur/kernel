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

#include "../atag.h"

/**
 * @brief Small helper to get atag if existing
 *
 * @param head
 * @param tag
 * @return atag_t*
 */
atag_t* atag_find( atag_t* head, atag_tag_t tag ) {
  // loop until atag end reached
  while ( head ) {
    // return if found
    if ( head->header.tag == tag ) {
      return head;
    }
    // next entry
    head = atag_next( head );
  }

  return NULL;
}
