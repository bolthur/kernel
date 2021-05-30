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
#include <core/message.h>
#if defined( PRINT_MESSAGE )
  #include <core/debug/debug.h>
#endif

/**
 * @brief Init message queue stuff
 *
 * @return true
 * @return false
 */
bool message_init( void ) {
  return true;
}

/**
 * @brief list item cleanup helper
 *
 * @param item
 */
void message_cleanup( const list_item_ptr_t item ) {
  if ( item->data ) {
    // transform to entry
    const message_entry_ptr_t entry = ( message_entry_ptr_t )item->data;
    // free message if set
    if ( entry->data ) {
      free( ( void* )entry->data );
    }
    // free entry
    free( entry );
  }
  // continue with default list cleanup
  list_default_cleanup( item );
}

/**
 * @brief Generates new message id
 *
 * @return
 */
size_t message_generate_id( void ) {
  static size_t id = 1;
  return id++;
}
