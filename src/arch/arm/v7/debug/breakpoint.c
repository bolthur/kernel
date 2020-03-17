
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
#include <arch/arm/debug/barrier.h>
#include <arch/arm/debug/cache.h>
#include <arch/arm/v7/debug/debug.h>
#include <core/debug/gdb.h>
#include <core/debug/breakpoint.h>
#include <core/debug/string.h>

/**
 * @brief Used instruction for breakpoints ( bkpt #0 )
 */
#define GDB_BREAKPOINT_INSTRUCTION 0xe1200070

/**
 * @brief Helper to get a possible breakpoint
 *
 * @param address
 * @return debug_breakpoint_entry_ptr_t
 *
 * @todo add support for hardware breakpoints
 */
debug_breakpoint_entry_ptr_t debug_breakpoint_find( uintptr_t address ) {
  // handle not existing
  if ( NULL == debug_breakpoint_manager ) {
    return NULL;
  }
  // check for possible existance
  list_item_ptr_t current = debug_breakpoint_manager->first;
  debug_breakpoint_entry_ptr_t entry = NULL;

  // loop through list of entries
  while ( NULL != current ) {
    // get entry value
    debug_breakpoint_entry_ptr_t tmp =
      ( debug_breakpoint_entry_ptr_t )current->data;

    // check for match
    if ( tmp->address == address ) {
      entry = tmp;
      break;
    }

    // next entry
    current = current->next;
  }

  // return found / not found entry
  return entry;
}

/**
 * @brief Method to remove all stepping breakpoints
 *
 * @todo add support for hardware breakpoints
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

/**
 * @brief Helper to remove a breakpoint
 *
 * @param address
 * @param remove
 *
 * @todo add support for hardware breakpoints
 */
void debug_breakpoint_remove( uintptr_t address, bool remove ) {
  // variables
  debug_breakpoint_entry_ptr_t entry = debug_breakpoint_find( address );
  // Do nothing if not existing
  if ( NULL == entry ) {
    return;
  }

  // set enabled to false
  entry->enabled = false;
  // handle removal
  if ( remove ) {
    list_item_ptr_t item = list_lookup_data( debug_breakpoint_manager, entry );
    // remove from list
    list_remove( debug_breakpoint_manager, item );
    // free stuff
    free( entry );
    free( item );
  }
}

/**
 * @brief Method to add breakpoint to list
 *
 * @param address
 * @param step
 * @param enable
 *
 * @todo add support for hardware breakpoints
 */
void debug_breakpoint_add(
  uintptr_t address,
  bool step,
  bool enable
) {
  // variables
  debug_breakpoint_entry_ptr_t entry = debug_breakpoint_find( address );
  // Don't add if already existing
  if ( NULL != entry && true == entry->enabled ) {
    return;
  }

  // create if not existing
  if ( NULL == entry ) {
    // allocate entry
    entry = ( debug_breakpoint_entry_ptr_t )malloc(
      sizeof( debug_breakpoint_entry_t ) );
    // erase allocated memory
    debug_memset( ( void* )entry, 0, sizeof( debug_breakpoint_entry_t ) );
    // push entry back
    list_push_back( debug_breakpoint_manager, ( void* )entry );
  }

  // set attributes
  entry->step = step;
  entry->enabled = enable;
  entry->address = address;
}

/**
 * @brief Method deactivates all breakpoints
 *
 * @todo add support for hardware breakpoints
 */
void debug_breakpoint_disable( void ) {
  // skip if not initialized
  if (
    NULL == debug_breakpoint_manager
    || debug_gdb_get_running_flag()
  ) {
    return;
  }
  // variables
  list_item_ptr_t current = debug_breakpoint_manager->first;

  // loop through list of entries
  while ( NULL != current ) {
    // get entry value
    debug_breakpoint_entry_ptr_t entry =
      ( debug_breakpoint_entry_ptr_t )current->data;
    // replace instruction if enabled
    if ( entry->enabled ) {
      // push back instruction
      debug_memcpy(
        ( void* )entry->address,
        ( void* )&entry->instruction,
        sizeof( entry->instruction ) );
      // data transfer barrier
      debug_barrier_data_mem();
    }
    // next entry
    current = current->next;
  }

  // flush after copy
  debug_cache_invalidate_instruction_cache();
}

/**
 * @brief Method activates all enabled breakpoints
 *
 * @todo add support for hardware breakpoints
 */
void debug_breakpoint_enable( void ) {
  // skip if not initialized
  if (
    NULL == debug_breakpoint_manager
    || debug_gdb_get_running_flag()
  ) {
    return;
  }
  // variables
  uintptr_t bpi = GDB_BREAKPOINT_INSTRUCTION;
  list_item_ptr_t current = debug_breakpoint_manager->first;

  // loop through list of entries
  while ( NULL != current ) {
    // get entry value
    debug_breakpoint_entry_ptr_t entry =
      ( debug_breakpoint_entry_ptr_t )current->data;
    // replace instruction if enabled
    if ( entry->enabled ) {
      // save instruction
      debug_memcpy(
        ( void* )&entry->instruction,
        ( void* )entry->address,
        sizeof( entry->instruction ) );
      // data transfer barrier
      debug_barrier_data_mem();

      // overwrite with breakpoint instruction
      debug_memcpy( ( void* )entry->address, ( void* )&bpi, sizeof( bpi ) );
      // data transfer barrier
      debug_barrier_data_mem();
    }
    // next entry
    current = current->next;
  }

  // flush cache
  debug_cache_invalidate_instruction_cache();
}
