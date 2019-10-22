
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

/**
 * @brief event manager structure
 */
event_manager_ptr_t event = NULL;

/**
 * @brief Method to setup event system
 */
void event_init( void ) {
  // set count to 0
  size_t count = 0;
  // count amount of types
  for ( int32_t type = EVENT_TIMER; type < EVENT_DUMMY_LAST; type++ ) {
    count++;
  }

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Determined count of event lists: %d\r\n", count );
  #endif

  // create manager structure
  event = ( event_manager_ptr_t )malloc( sizeof( event_manager_t ) );
  // assert result
  assert( NULL != event );
  // prepare
  memset( ( void* )event, 0, sizeof( event_manager_t ) );

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Initialized event manager structure at 0x%08p\r\n", event );
  #endif

  // create lists for each type
  event->list = ( list_manager_ptr_t* )malloc(
    count * sizeof( list_manager_ptr_t ) );
  // assert result
  assert( NULL != event->list );
  // prepare
  memset( ( void* )event->list, 0, count * sizeof( list_manager_ptr_t ) );

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Initialized list for types at: 0x%08p\r\n", event->list );
  #endif
}

/**
 * @brief Bind event callback
 *
 * @param type event type
 * @param callback callback to bind
 * @return true on success
 * @return false on error
 */
bool event_bind( event_type_t type, event_callback_t callback ) {
  // do nothing if not initialized
  if ( NULL == event ) {
    return true;
  }

  // Get list of type
  list_manager_ptr_t list = event->list[ type ];
  // construct list if not created
  if ( NULL == list ) {
    // create list
    list = list_construct();
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Created list at 0x%08p\r\n", list );
    #endif
  }

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Checking for already bound callback\r\n" );
  #endif
  // get first element
  list_item_ptr_t current = list->first;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Used first element for looping at 0x%08p\r\n", current );
  #endif
  // loop through list for check callback
  while ( NULL != current ) {
    // get callback from data
    event_callback_wrapper_ptr_t wrapper =
      ( event_callback_wrapper_ptr_t )current->data;
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Check bound callback at 0x%08p\r\n", wrapper );
    #endif
    // handle match
    if ( wrapper->callback == callback ) {
      return true;
    }
    // get to next
    current = current->next;
  }

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Push wrapper into list\r\n" );
  #endif
  // create callback wrapper
  event_callback_wrapper_ptr_t wrapper = ( event_callback_wrapper_ptr_t )malloc(
    sizeof( event_callback_wrapper_t ) );
  // assert malloc
  assert( NULL != malloc );
  // prepare
  memset( ( void* )wrapper, 0, sizeof( event_callback_wrapper_t ) );
  // populate callback
  wrapper->callback = callback;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Binding callback for type %d\r\n", type );
    DEBUG_OUTPUT( "Created wrapper at 0x%08p\r\n", wrapper );
  #endif

  // push item
  list_push( list, ( void* )wrapper );

  // overwrite event list entry
  event->list[ type ] = list;

  // return success
  return true;
}

/**
 * @brief Unbind event if existing
 *
 * @param event_type_t event type
 * @param event_callback_t bound callback
 *
 * @todo Add logic for method
 */
void event_unbind( __unused event_type_t type, __unused event_callback_t callback ) {
  // do nothing if not initialized
  if ( NULL == event ) {
    return;
  }

  PANIC( "event_unbind not yet implemented!" );
}

/**
 * @brief Fire event by type with data
 *
 * @param type type to fire
 * @param data data to pass through
 */
void event_fire( event_type_t type, void** data ) {
  // do nothing if not initialized
  if ( NULL == event ) {
    return;
  }

  // Get list of type
  list_manager_ptr_t list = event->list[ type ];

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Use list 0x%08p for fire\r\n", list );
  #endif

  // handle NULL
  if ( NULL == list ) {
    return;
  }

  // get first element
  list_item_ptr_t current = list->first;

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Used first element for looping at 0x%08p\r\n", current );
  #endif

  // loop through list
  while ( NULL != current ) {
    // get callback from data
    event_callback_wrapper_ptr_t wrapper =
      ( event_callback_wrapper_ptr_t )current->data;

    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Executing bound callback 0x%08p\r\n", wrapper );
    #endif

    // fire with data
    wrapper->callback( data );

    // step to next
    current = current->next;
  }
}
