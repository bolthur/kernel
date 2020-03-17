
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

#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <core/event.h>
#include <core/serial.h>
#include <core/debug/debug.h>
#include <core/debug/gdb.h>
#include <core/debug/breakpoint.h>

/**
 * @brief debug breakpoint manager
 */
list_manager_ptr_t debug_breakpoint_manager = NULL;

/**
 * @brief Setup breakpoint manager
 */
void debug_breakpoint_init( void ) {
  // handle initialized
  if ( NULL != debug_breakpoint_manager ) {
    return;
  }
  // setup list
  debug_breakpoint_manager = list_construct();
}

/**
 * @brief Helper to get a possible breakpoint
 *
 * @param address
 * @return debug_breakpoint_entry_ptr_t
 */
debug_breakpoint_entry_ptr_t debug_breakpoint_find( uintptr_t address ) {
  // handle not existing
  if ( NULL == debug_breakpoint_manager ) {
    return NULL;
  }
  // check for possible existance
  list_item_ptr_t current = debug_breakpoint_manager->first;
  // loop through list of entries
  while ( NULL != current ) {
    // get entry value
    debug_breakpoint_entry_ptr_t entry =
      ( debug_breakpoint_entry_ptr_t )current->data;
    // check for match
    if ( entry->address == address ) {
      return entry;
    }
    // next entry
    current = current->next;
  }
  // return found / not found entry
  return NULL;
}

/**
 * @brief Method to remove all stepping breakpoints
 */
void debug_breakpoint_remove_step( void ) {
  // check for initialized
  if ( NULL == debug_breakpoint_manager ) {
    return;
  }
  // variables
  debug_breakpoint_entry_ptr_t entry = NULL;
  list_item_ptr_t current = debug_breakpoint_manager->first;
  list_item_ptr_t next = NULL;

  // loop through list of entries
  while ( NULL != current ) {
    // get entry value
    entry = ( debug_breakpoint_entry_ptr_t )current->data;
    // next entry
    next = current->next;
    // handle only stepping breakpoints
    if ( entry->step ) {
      // remove from breakpoint manager list
      list_remove( debug_breakpoint_manager, current );
      // free stuff
      free( entry );
      free( current );
    }
    // set current to next
    current = next;
  }
}
