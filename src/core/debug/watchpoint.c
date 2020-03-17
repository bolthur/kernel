
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
#include <list.h>
#include <core/debug/watchpoint.h>

/**
 * @brief debug watchpoint manager
 */
list_manager_ptr_t debug_watchpoint_manager = NULL;

/**
 * @brief Setup watchpoint manager
 */
void debug_watchpoint_init( void ) {
  // handle initialized
  if ( NULL != debug_watchpoint_manager ) {
    return;
  }
  // setup list
  debug_watchpoint_manager = list_construct();
}

/**
 * @brief looks up for a watch point at address
 *
 * @param address address
 * @return debug_watchpoint_entry_ptr_t
 */
debug_watchpoint_entry_ptr_t debug_watchpoint_find( uintptr_t address ) {
  // handle not existing
  if ( NULL == debug_watchpoint_manager ) {
    return NULL;
  }
  // check for possible existance
  list_item_ptr_t current = debug_watchpoint_manager->first;
  // loop through list of entries
  while ( NULL != current ) {
    // get entry value
    debug_watchpoint_entry_ptr_t entry =
      ( debug_watchpoint_entry_ptr_t )current->data;
    // check for match
    if ( entry->address == address ) {
      return entry;
    }
    // next entry
    current = current->next;
  }
  // return not found
  return NULL;
}
