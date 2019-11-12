
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

#include <avl.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/panic.h>
#include <kernel/debug/debug.h>
#include <kernel/mm/heap.h>
#include <kernel/interrupt/interrupt.h>

/**
 * @brief Interrupt management structure
 */
interrupt_manager_ptr_t interrupt_manager = NULL;

/**
 * @brief Compare interrupt callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_interrupt_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // get blocks
  interrupt_block_ptr_t block_a = INTERRUPT_GET_BLOCK( a );
  interrupt_block_ptr_t block_b = INTERRUPT_GET_BLOCK( b );

  // -1 if address of a is greater than address of b
  if ( block_a->interrupt > block_b->interrupt ) {
    return -1;
  // 1 if address of b is greater than address of a
  } else if ( block_b->interrupt > block_a->interrupt ) {
    return 1;
  }

  // equal => return 0
  return 0;
}

/**
 * @brief Unregister interrupt handler
 *
 * @param num interrupt to unbind
 * @param callback Callback to unbind
 * @param fast flag to unbind fast
 * @param post flag to bind as post callback
 */
void interrupt_unregister_handler(
  __unused size_t num,
  __unused interrupt_callback_t callback,
  __unused bool fast,
  __unused bool post
) {
  PANIC( "Unregister handling is not yet implemented!" );
}

/**
 * @brief Register interrupt handler
 *
 * @param num Interrupt to bind
 * @param callback Callback to bind
 * @param fast flag to bind FIQ
 * @param post flag to bind as post callback
 */
void interrupt_register_handler(
  size_t num,
  interrupt_callback_t callback,
  bool fast,
  bool post
) {
  // assert heap existance
  assert( true == heap_initialized_get() );
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT(
      "Called interrupt_register_handler( %d, 0x%08p, %s, %s )\r\n",
      num, callback, fast ? "true" : "false", post  ? "true" : "false" );
  #endif

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Try to map callback for interrupt %d\r\n", num );
  #endif
  // setup interrupt manager if not done existing
  if ( NULL == interrupt_manager ) {
    // initialize interrupt manager
    interrupt_manager = ( interrupt_manager_ptr_t )malloc(
      sizeof( interrupt_manager_t ) );
    // assert initialization
    assert( NULL != interrupt_manager );
    // prepare memory
    memset( ( void* )interrupt_manager, 0, sizeof( interrupt_manager_t ) );
    // create trees for interrupt types
    interrupt_manager->normal_interrupt = avl_create_tree(
      compare_interrupt_callback );
    interrupt_manager->fast_interrupt = avl_create_tree(
      compare_interrupt_callback );
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT(
        "Initialized interrupt manager with address 0x%08p\r\n",
        interrupt_manager
      );
    #endif
  }

  // validate interrupt number by fendor
  assert( interrupt_validate_number( num ) );

  // get correct tree to use
  avl_tree_ptr_t tree = interrupt_manager->normal_interrupt;
  // use fast tree if set
  if ( true == fast ) {
    tree = interrupt_manager->fast_interrupt;
  }
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT(
      "Using interrupt tree \"%s\" for lookup!\r\n",
      ( true == fast ) ? "fast_interrupt" : "normal_interrupt"
    );
  #endif

  // try to find node
  avl_node_ptr_t node = avl_find_by_data( tree, ( void* )num );
  interrupt_block_ptr_t block;
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Found node 0x%08p\r\n", node );
  #endif
  // handle not yet added
  if ( NULL == node ) {
    // allocate block
    block = ( interrupt_block_ptr_t )malloc( sizeof( interrupt_block_t ) );
    // assert initialization
    assert( NULL != block );
    // prepare memory
    memset( ( void* )block, 0, sizeof( interrupt_block_t ) );
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "Initialized new node at 0x%08p\r\n", block );
    #endif
    // populate block
    block->interrupt = num;
    block->handler = list_construct();
    block->post = list_construct();
    // prepare and insert node
    avl_prepare_node( &block->node, ( void* )num );
    avl_insert_by_node( tree, &block->node );
    // overwrite node
    node = &block->node;
  // existing? => gather block
  } else {
    block = INTERRUPT_GET_BLOCK( node );
  }

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Checking for already bound interrupt callback\r\n" );
  #endif
  // get first element
  list_item_ptr_t current =
    true != post
      ? block->handler->first
      : block->post->first;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Used first element for looping at 0x%08p\r\n", current );
  #endif
  // loop through list for check callback
  while ( NULL != current ) {
    // get callback from data
    interrupt_callback_wrapper_ptr_t wrapper =
      ( interrupt_callback_wrapper_ptr_t )current->data;
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Check bound callback at 0x%08p\r\n", wrapper );
    #endif
    // handle match
    if ( wrapper->callback == callback ) {
      return;
    }
    // get to next
    current = current->next;
  }

  // create wrapper
  interrupt_callback_wrapper_ptr_t wrapper = ( interrupt_callback_wrapper_ptr_t )
    malloc( sizeof( interrupt_callback_wrapper_t ) );
  // assert initialization
  assert( NULL != wrapper );
  // prepare memory
  memset( ( void* )wrapper, 0, sizeof( interrupt_callback_wrapper_t ) );

  // populate wrapper
  wrapper->callback = callback;

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Created wrapper container at 0x%08p\r\n", wrapper );
  #endif

  // push to list
  list_push_back(
    true != post
      ? block->handler
      : block->post,
    ( void* )wrapper );
}

/**
 * @brief Handle interrupt
 *
 * @param num interrupt number
 * @param fast fast interrupt flag
 * @param context interrupt context
 */
void interrupt_handle( size_t num, bool fast, void* context ) {
  // handle no interrupt manager as not bound
  if ( NULL == interrupt_manager ) {
    return;
  }

  // validate interrupt number by fendor
  assert( interrupt_validate_number( num ) );

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Handle interrupt %d\r\n", num );
  #endif

  // get correct tree to use
  avl_tree_ptr_t tree = interrupt_manager->normal_interrupt;
  // use fast tree if set
  if ( true == fast ) {
    tree = interrupt_manager->fast_interrupt;
  }

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT(
      "Using interrupt tree \"%s\" for lookup!\r\n",
      ( true == fast ) ? "fast_interrupt" : "normal_interrupt"
    );
  #endif

  // try to get node by interrupt
  avl_node_ptr_t node = avl_find_by_data( tree, ( void* )num );
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Found node 0x%08p\r\n", node );
  #endif

  // handle nothing found which means nothing bound
  if ( NULL == node ) {
    return;
  }
  // get interrupt block
  interrupt_block_ptr_t block = INTERRUPT_GET_BLOCK( node );

  // get first element of normal handler
  list_item_ptr_t current = block->handler->first;
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Looping through mapped callbacks and execute them\r\n" );
  #endif
  // loop through list
  while ( NULL != current ) {
    // get callback from data
    interrupt_callback_wrapper_ptr_t wrapper =
      ( interrupt_callback_wrapper_ptr_t )current->data;
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "Handling wrapper container 0x%08p\r\n", wrapper );
    #endif
    // fire with data
    wrapper->callback( context );
    // step to next
    current = current->next;
  }

  // get first element of post callbacks
  current = block->post->first;
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Looping through mapped callbacks and execute them\r\n" );
  #endif
  // loop through list
  while ( NULL != current ) {
    // get callback from data
    interrupt_callback_wrapper_ptr_t wrapper =
      ( interrupt_callback_wrapper_ptr_t )current->data;
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "Handling wrapper container 0x%08p\r\n", wrapper );
    #endif
    // fire with data
    wrapper->callback( context );
    // step to next
    current = current->next;
  }
}
