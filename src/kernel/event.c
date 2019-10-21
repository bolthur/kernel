
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

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <list.h>
#include <kernel/panic.h>
#include <kernel/debug/debug.h>
#include <kernel/event.h>

static bool event_initialized = false;
event_manager_ptr_t event = NULL;

/**
 * @brief Get initialized flag
 *
 * @return true event management has been set up
 * @return false event management has not been set up yet
 */
bool event_initialized_get( void ) {
  return event_initialized;
}

/**
 * @brief Method to setup event system
 *
 * @todo Add debug output encapsulated by define PRINT_EVENT
 */
void event_init( void ) {
  // assert not initialized
  assert( true != event_initialized );

  // set count to 0
  size_t count = 0;
  // count amount of types
  for ( int32_t type = EVENT_TIMER; type < EVENT_DUMMY_LAST; type++ ) {
    count++;
  }

  // create manager structure
  event = ( event_manager_ptr_t )malloc( sizeof( event_manager_t ) );
  // assert result
  assert( NULL != event );
  // prepare
  memset( ( void* )event, 0, sizeof( event_manager_t ) );

  // create lists for each type
  event->list = ( list_item_ptr_t* )malloc( count * sizeof( list_item_ptr_t ) );
  // assert result
  assert( NULL != event->list );
  // prepare
  memset( ( void* )event->list, 0, count * sizeof( list_item_ptr_t ) );

  // set flag
  event_initialized = true;
}

/**
 * @brief Bind event callback
 *
 * @param type event type
 * @param callback callback to bind
 * @return true on success
 * @return false on error
 *
 * @todo check for already bound callback
 *
 * @todo Add debug output encapsulated by define PRINT_EVENT
 */
bool event_bind( event_type_t type, event_callback_t callback ) {
  // do nothing if not initialized
  if ( ! event_initialized ) {
    return true;
  }

  // create callback wrapper
  event_callback_wrapper_ptr_t wrapper = ( event_callback_wrapper_ptr_t )malloc(
    sizeof( event_callback_wrapper_t ) );
  DEBUG_OUTPUT( "wrapper = 0x%08x\r\n", wrapper );
  // assert malloc
  assert( NULL != malloc );
  // prepare
  memset( ( void* )wrapper, 0, sizeof( event_callback_wrapper_t ) );
  // populate callback
  wrapper->callback = callback;

  // Get list of type
  list_item_ptr_t current = event->list[ type ];

  // construct list
  if ( NULL == current ) {
    current = *list_construct( ( void* )wrapper );
  // push item
  } else {
    list_push( &current, ( void* )wrapper );
  }

  // overwrite event list entry
  event->list[ type ] = current;

  // return success
  return true;
}

/**
 * @brief Unbind event if existing
 *
 * @param event_type_t event type
 * @param event_callback_t bound callback
 *
 * @todo Add debug output encapsulated by define PRINT_EVENT
 */
void event_unbind( __unused event_type_t type, __unused event_callback_t callback ) {
  // do nothing if not initialized
  if ( ! event_initialized ) {
    return;
  }

  PANIC( "event_unbind not yet implemented!" );
}

/**
 * @brief Fire event by type with data
 *
 * @param type type to fire
 * @param data data to pass through
 *
 * @todo Add debug output encapsulated by define PRINT_EVENT
 */
void event_fire( event_type_t type, void* data ) {
  // do nothing if not initialized
  if ( ! event_initialized ) {
    return;
  }

  // Get list of type
  list_item_ptr_t current = event->list[ type ];

  // loop through list
  while ( NULL != current ) {
    // get callback from data
    event_callback_wrapper_ptr_t wrapper =
      ( event_callback_wrapper_ptr_t )current->data;

    // fire with data
    wrapper->callback( data );

    // step to next
    current = current->next;
  }
}
