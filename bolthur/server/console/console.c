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

#include <stdlib.h>
#include <string.h>
#include "console.h"

/**
 * @fn void console_destroy(console_ptr_t)
 * @brief Helper to destroy console entry
 *
 * @param console
 */
void console_destroy( console_ptr_t console ) {
  if ( ! console ) {
    return;
  }
  if ( console->path ) {
    free( console->path );
  }
  free( console );
}

/**
 * @fn console_ptr_t console_get_active(void)
 * @brief Helper to get active console
 *
 * @return
 */
console_ptr_t console_get_active( void ) {
  list_item_ptr_t current = console_list->first;
  while ( current ) {
    console_ptr_t found = current->data;
    if ( found->active ) {
      return found;
    }
    current = current->next;
  }
  return NULL;
}

/**
 * @fn console_ptr_t console_get_by_path(const char*)
 * @brief Get console by path
 *
 * @param path
 * @return
 */
console_ptr_t console_get_by_path( const char* path ) {
  list_item_ptr_t current = console_list->first;
  while ( current ) {
    console_ptr_t found = current->data;
    if ( 0 == strcmp( found->path, path ) ) {
      return found;
    }
    current = current->next;
  }
  return NULL;
}
