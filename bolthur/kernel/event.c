/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include "lib/stdlib.h"
#include "lib/string.h"
#include "lib/inttypes.h"
#include "../library/collection/list/list.h"
#include "panic.h"
#include "event.h"
#include "stack.h"
#if defined( PRINT_EVENT )
  #include "debug/debug.h"
#endif

/**
 * @brief event manager structure
 */
static event_manager_t* event = nullptr;

/**
 * @fn int32_t compare_event_callback(const avl_node_t*, const avl_node_t*)
 * @brief Compare event callback necessary for avl tree
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_event_callback(
  const avl_node_t* a,
  const avl_node_t* b
) {
  // get blocks
  const event_block_t* block_a = EVENT_GET_BLOCK( a );
  const event_block_t* block_b = EVENT_GET_BLOCK( b );
  // -1 if address of a->type is greater than address of b->type
  if ( block_a->type > block_b->type ) {
    return -1;
  // 1 if address of b->type is greater than address of a->type
  } else if ( block_b->type > block_a->type ) {
    return 1;
  }
  // equal => return 0
  return 0;
}

/**
 * @fn bool event_init( void )
 * @brief Method to setup event system
 * @return true
 * @return false
 */
bool event_init( void ) {
  // create manager structure
  event = malloc( sizeof( *event ) );
  // check
  if ( ! event ) {
    return false;
  }
  // prepare
  memset( ( void* )event, 0, sizeof( *event ) );
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Initialized event manager structure at %p\r\n", event )
  #endif
  // create tree
  event->tree = avl_create_tree( compare_event_callback, nullptr, nullptr );
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Created event tree at: %p\r\n", event->tree )
  #endif
  // handle error
  if ( ! event->tree ) {
    free( event );
    return false;
  }
  // return success
  return true;
}

/**
 * @fn bool event_bind(event_type_t, event_callback_t, bool)
 * @brief Bind event callback
 * @param type event type
 * @param callback callback to bind
 * @param post post callback mapping
 * @return true on success
 * @return false on error
 */
bool event_bind( const event_type_t type, const event_callback_t callback, const bool post ) {
  // do nothing if not initialized
  if ( ! event ) {
    return true;
  }

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT(
      "Called event_bind( %d, %#"PRIxPTR", %s )\r\n",
      type,
      ( uintptr_t )callback,
      post ? "true" : "false"
    )
  #endif
  // get correct tree to use
  avl_tree_t* tree = event->tree;

  // try to find node
  avl_node_t* node = avl_find_by_data( tree, type );
  event_block_t* block;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Found node %p\r\n", node )
  #endif
  // handle not yet added
  if ( ! node ) {
    // create new block
    block = malloc( sizeof( *block ) );
    // check
    if ( ! block ) {
      return false;
    }
    // prepare memory
    memset( ( void* )block, 0, sizeof( *block ) );
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Initialized new node at %p\r\n", block )
    #endif
    // populate block
    block->type = type;
    block->handler = list_construct( nullptr, nullptr, nullptr );
    if ( ! block->handler ) {
      free( block );
      return false;
    }
    block->post = list_construct( nullptr, nullptr, nullptr );
    if ( ! block->post ) {
      free( block->handler );
      free( block );
      return false;
    }
    // prepare and insert node
    avl_prepare_node( &block->node, type );
    if ( ! avl_insert_by_node( tree, &block->node ) ) {
      free( block->handler );
      free( block->post );
      free( block );
      return false;
    }
  // existing? => gather block
  } else {
    block = EVENT_GET_BLOCK( node );
  }

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Checking for already bound event callback\r\n" )
  #endif
  // get first element
  list_item_t* current = true != post
      ? block->handler->first
      : block->post->first;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Used first element for looping at %p\r\n", current )
  #endif
  // loop through list for check callback
  while ( current ) {
    // get callback from data
    auto wrapper = ( event_callback_wrapper_t* )current->data;
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Check bound callback at %p\r\n", wrapper )
    #endif
    // handle match
    if ( wrapper->callback == callback ) {
      // debug output
      #if defined( PRINT_EVENT )
        DEBUG_OUTPUT( "Callback already existing\r\n" )
      #endif
      // return success
      return true;
    }
    // get to next
    current = current->next;
  }

  // create wrapper
  event_callback_wrapper_t* wrapper = malloc( sizeof( *wrapper ) );
  // check
  if ( ! wrapper ) {
    return false;
  }
  // prepare memory
  memset( ( void* )wrapper, 0, sizeof( *wrapper ) );
  // populate wrapper
  wrapper->callback = callback;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Created wrapper container at %p\r\n", wrapper )
  #endif
  // push to list
  return list_push_back_data(
    true != post
      ? block->handler
      : block->post,
    ( void* )wrapper );
}

/**
 * @fn void event_unbind(event_type_t, event_callback_t, bool)
 * @brief Unbind event if existing
 * @param type event type
 * @param callback bound callback
 * @param post post callback
 *
 * @todo add avl tree cleanup helper
 * @todo check whether avl removal is enough as logic for this function
 */
void event_unbind(
  [[maybe_unused]] const event_type_t type,
  [[maybe_unused]] const event_callback_t callback,
  [[maybe_unused]] const bool post
) {
  // do nothing if not initialized
  if ( ! event ) {
    return;
  }

  PANIC( "event_unbind not yet implemented!" )
}

/**
 * @fn void event_enqueue(const event_type_t type )
 * @brief Enqueue event
 * @param type type to enqueue
 */
void event_enqueue( const event_type_t type ) {
  // do nothing if not initialized
  if ( ! event ) {
    return;
  }
  // mark event as queued
  event->queue_bitmap |= ( 1U << type );
}

/**
 * @fn void event_handle(void*)
 * @brief Handle enqueued events with data
 * @param data data to pass through
 */
void event_handle( void* data ) {
  // do nothing if not initialized
  if ( ! event ) {
    return;
  }
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Enter event_handle( %p )\r\n", data )
  #endif
  // determine origin
  const event_origin_t origin = event_determine_origin( data );
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "origin = %d\r\n", origin )
  #endif
  // get bitmap
  uint32_t bitmap = event->queue_bitmap;
  // duplicate for post calls
  uint32_t post_bitmap = bitmap;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "bitmap = %#"PRIx32"\r\n", bitmap )
  #endif
  // execute regular events
  while ( bitmap ) {
    auto const type = ( event_type_t )__builtin_ctz( bitmap );
    // get type node
    avl_node_t* node = avl_find_by_data( event->tree, type );
    if ( ! node ) {
      // mask bit
      bitmap &= ( bitmap - 1 );
      event->queue_bitmap = bitmap;
      // skip rest
      continue;
    }
    auto const block = EVENT_GET_BLOCK( node );
    // get first element of normal callback list
    auto current = block->handler->first;
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Used first normal element for looping at %p\r\n", current )
    #endif
    // loop through list
    while ( current ) {
      // get callback from data
      auto const wrapper = ( event_callback_wrapper_t* )current->data;
      // debug output
      #if defined( PRINT_EVENT )
        DEBUG_OUTPUT( "Executing bound callback %p\r\n", wrapper )
      #endif
      // fire with data
      wrapper->callback( origin, data );
      // step to next
      current = current->next;
    }
    // mask bit
    bitmap &= ( bitmap - 1 );
    event->queue_bitmap = bitmap;
  }
  // execute post events
  while ( post_bitmap ) {
    auto const type = ( event_type_t )__builtin_ctz( post_bitmap );
    // mask bit
    post_bitmap &= ( post_bitmap - 1 );
    // get type node
    avl_node_t* node = avl_find_by_data( event->tree, type );
    if ( ! node ) {
      // skip rest
      continue;
    }
    auto const block = EVENT_GET_BLOCK( node );
    // get first element of post callback list
    auto current = block->post->first;
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Used first post element for looping at %p\r\n", current )
    #endif
    // loop through list
    while ( current ) {
      // get callback from data
      auto const wrapper = ( event_callback_wrapper_t* )current->data;
      // debug output
      #if defined( PRINT_EVENT )
        DEBUG_OUTPUT( "Executing bound post callback %p\r\n", wrapper )
      #endif
      // fire with data
      wrapper->callback( origin, data );
      // step to next
      current = current->next;
    }
  }
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Leave event_handle\r\n" )
  #endif
  // recheck bitmap for recursion, because handlers might trigger other events
  if ( event->queue_bitmap ) {
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Further outstanding events, recursive call!\r\n" )
    #endif
    // recursive call for handle remaining events
    event_handle( data );
  }
}

/**
 * @fn event_origin_t event_determine_origin( const void* context )
 * @brief Helper to determine origin
 * @param context context to check
 * @return
 */
__no_stack_protector event_origin_t event_determine_origin( const void* context ) {
  if ( ! context ) {
    return EVENT_ORIGIN_USER;
  }
  if ( ! stack_is_kernel( ( uintptr_t )context ) ) {
    return EVENT_ORIGIN_USER;
  }
  return EVENT_ORIGIN_KERNEL;
}
