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

#include <stddef.h>

#include <collection/avl.h>
#include <string.h>
#include <stdlib.h>
#if defined( PRINT_EVENT )
  #include <core/debug/debug.h>
#endif
#include <core/mm/heap.h>
#include <core/interrupt.h>

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
 * @brief Helper to get interrupt manager tree by type
 *
 * @param type type to get tree from
 * @return avl_tree_ptr_t
 */
static avl_tree_ptr_t tree_by_type( interrupt_type_t type ) {
  // check heap existence
  if ( ! heap_init_get() ) {
    return NULL;
  }
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Called tree_by_type( %d )\r\n", type );
  #endif
  // setup interrupt manager if not done existing
  if ( ! interrupt_manager ) {
    // initialize interrupt manager
    interrupt_manager = ( interrupt_manager_ptr_t )malloc(
      sizeof( interrupt_manager_t ) );
    // check allocation
    if ( ! interrupt_manager ) {
      return NULL;
    }
    // prepare memory
    memset( ( void* )interrupt_manager, 0, sizeof( interrupt_manager_t ) );
    // create trees for interrupt types
    interrupt_manager->normal_interrupt = avl_create_tree(
      compare_interrupt_callback, NULL, NULL );
    // check allocation
    if ( ! interrupt_manager->normal_interrupt ) {
      free( interrupt_manager );
      return NULL;
    }
    interrupt_manager->fast_interrupt = avl_create_tree(
      compare_interrupt_callback, NULL, NULL );
    // check allocation
    if ( ! interrupt_manager->fast_interrupt ) {
      free( interrupt_manager->normal_interrupt );
      free( interrupt_manager );
      return NULL;
    }
    interrupt_manager->software_interrupt = avl_create_tree(
      compare_interrupt_callback, NULL, NULL );
    // check allocation
    if ( ! interrupt_manager->software_interrupt ) {
      free( interrupt_manager->normal_interrupt );
      free( interrupt_manager->fast_interrupt );
      free( interrupt_manager );
      return NULL;
    }
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "Initialized interrupt manager with address %p\r\n",
        ( void* )interrupt_manager );
    #endif
  }

  // set by type
  switch ( type ) {
    case INTERRUPT_NORMAL:
      return interrupt_manager->normal_interrupt;

    case INTERRUPT_FAST:
      return interrupt_manager->fast_interrupt;

    case INTERRUPT_SOFTWARE:
      return interrupt_manager->software_interrupt;
  }

  // invalid
  return NULL;
}

/**
 * @brief Unregister interrupt handler
 *
 * @param num interrupt to unbind
 * @param callback Callback to unbind
 * @param type interrupt type
 * @param post flag to bind as post callback
 * @return true
 * @return false
 */
bool interrupt_unregister_handler(
  size_t num,
  interrupt_callback_t callback,
  interrupt_type_t type,
  bool post
) {
  if ( ! heap_init_get() ) {
    return false;
  }
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT(
      "Called interrupt_unregister_handler( %zu, %p, %d, %s )\r\n",
      num, callback, type, post  ? "true" : "false" );
  #endif

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Try to unmap callback for interrupt %zu\r\n", num );
  #endif

  // validate interrupt number by vendor
  if (
    (
      type == INTERRUPT_NORMAL
      || type == INTERRUPT_FAST
    ) && ! interrupt_validate_number( num )
  ) {
    return false;
  }

  // get correct tree to use
  avl_tree_ptr_t tree = tree_by_type( type );
  // handle no tree
  if ( ! tree ) {
    return false;
  }
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT(
      "Using interrupt tree \"%p\" for lookup!\r\n", ( void* )tree );
  #endif

  // try to find node
  avl_node_ptr_t node = avl_find_by_data( tree, ( void* )num );
  interrupt_block_ptr_t block;
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Found node %p\r\n", ( void* )node );
  #endif
  // handle not yet added
  if ( ! node ) {
    return true;
  }
  // gather block
  block = INTERRUPT_GET_BLOCK( node );

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Checking for not bound interrupt callback\r\n" );
  #endif
  // get first element
  list_item_ptr_t match = NULL;
  list_manager_ptr_t list = true != post
    ? block->handler : block->post;
  list_item_ptr_t current = list->first;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Used first element for looping at %p\r\n", ( void* )current );
  #endif
  // loop through list for check callback
  while ( current ) {
    // get callback from data
    interrupt_callback_wrapper_ptr_t wrapper =
      ( interrupt_callback_wrapper_ptr_t )current->data;
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Check bound callback at %p\r\n", ( void* )wrapper );
    #endif
    // handle match
    if ( wrapper->callback == callback ) {
      match = current;
      break;
    }
    // get to next
    current = current->next;
  }

  // handle no match
  if ( ! match ) {
    return true;
  }
  // free allocated wrapper
  free( match->data );
  // remove element
  return list_remove( list, match );
}

/**
 * @brief Register interrupt handler
 *
 * @param num Interrupt to bind
 * @param callback Callback to bind
 * @param type interrupt type
 * @param post flag to bind as post callback
 * @return true
 * @return false
 */
bool interrupt_register_handler(
  size_t num,
  interrupt_callback_t callback,
  interrupt_type_t type,
  bool post
) {
  if ( ! heap_init_get() ) {
    return false;
  }
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT(
      "Called interrupt_register_handler( %zu, %p, %d, %s )\r\n",
      num, callback, type, post  ? "true" : "false" );
  #endif

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Try to map callback for interrupt %zu\r\n", num );
  #endif

  // validate interrupt number by vendor
  if (
    (
      type == INTERRUPT_NORMAL
      || type == INTERRUPT_FAST
    ) && ! interrupt_validate_number( num )
  ) {
    return false;
  }

  // get correct tree to use
  avl_tree_ptr_t tree = tree_by_type( type );
  // check tree
  if ( ! tree ) {
    return false;
  }
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT(
      "Using interrupt tree \"%p\" for lookup!\r\n", ( void* )tree );
  #endif

  // try to find node
  avl_node_ptr_t node = avl_find_by_data( tree, ( void* )num );
  interrupt_block_ptr_t block;
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Found node %p\r\n", ( void* )node );
  #endif
  // handle not yet added
  if ( ! node ) {
    // allocate block
    block = ( interrupt_block_ptr_t )malloc( sizeof( interrupt_block_t ) );
    // check allocation
    if ( ! block ) {
      return false;
    }
    // prepare memory
    memset( ( void* )block, 0, sizeof( interrupt_block_t ) );
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "Initialized new node at %p\r\n", ( void* )block );
    #endif
    // populate block
    block->interrupt = num;
    block->handler = list_construct( NULL, NULL );
    // check list
    if ( ! block->handler ) {
      free( block );
      return false;
    }
    block->post = list_construct( NULL, NULL );
    // check list
    if ( ! block->post ) {
      free( block->handler );
      free( block );
      return false;
    }
    // prepare and insert node
    avl_prepare_node( &block->node, ( void* )num );
    if ( ! avl_insert_by_node( tree, &block->node ) ) {
      free( block->post );
      free( block->handler );
      free( block );
      return false;
    }
  // existing? => gather block
  } else {
    block = INTERRUPT_GET_BLOCK( node );
  }

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Checking for already bound interrupt callback\r\n" );
  #endif
  // get first element
  list_manager_ptr_t list = true != post
    ? block->handler : block->post;
  list_item_ptr_t current = list->first;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Used first element for looping at %p\r\n", ( void* )current );
  #endif
  // loop through list for check callback
  while ( current ) {
    // get callback from data
    interrupt_callback_wrapper_ptr_t wrapper =
      ( interrupt_callback_wrapper_ptr_t )current->data;
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Check bound callback at %p\r\n", ( void* )wrapper );
    #endif
    // handle match
    if ( wrapper->callback == callback ) {
      return true;
    }
    // get to next
    current = current->next;
  }

  // create wrapper
  interrupt_callback_wrapper_ptr_t wrapper = ( interrupt_callback_wrapper_ptr_t )
    malloc( sizeof( interrupt_callback_wrapper_t ) );
  // check allocation
  if ( ! wrapper ) {
    return false;
  }
  // prepare memory
  memset( ( void* )wrapper, 0, sizeof( interrupt_callback_wrapper_t ) );
  // populate wrapper
  wrapper->callback = callback;
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Created wrapper container at %p\r\n", ( void* )wrapper );
  #endif

  // push to list
  return list_push_back( list, ( void* )wrapper );
}

/**
 * @brief Handle interrupt
 *
 * @param num interrupt number
 * @param type interrupt type
 * @param context interrupt context
 */
void interrupt_handle( size_t num, interrupt_type_t type, void* context ) {
  // handle no interrupt manager as not bound
  if ( ! interrupt_manager ) {
    return;
  }

  // validate interrupt number by vendor
  if (
    (
      type == INTERRUPT_NORMAL
      || type == INTERRUPT_FAST
    ) && ! interrupt_validate_number( num )
  ) {
    return;
  }

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Handle interrupt %zu\r\n", num );
  #endif

  // get correct tree to use
  avl_tree_ptr_t tree = tree_by_type( type );
  // check tree
  if ( ! tree ) {
    return;
  }

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Using interrupt tree \"%p\" for lookup!\r\n", ( void* )tree );
  #endif

  // try to get node by interrupt
  avl_node_ptr_t node = avl_find_by_data( tree, ( void* )num );
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Found node %p\r\n", ( void* )node );
  #endif

  // handle nothing found which means nothing bound
  if ( ! node ) {
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
  while ( current ) {
    // get callback from data
    interrupt_callback_wrapper_ptr_t wrapper =
      ( interrupt_callback_wrapper_ptr_t )current->data;
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "Handling wrapper container %p\r\n", ( void* )wrapper );
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
  while ( current ) {
    // get callback from data
    interrupt_callback_wrapper_ptr_t wrapper =
      ( interrupt_callback_wrapper_ptr_t )current->data;
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "Handling wrapper container %p\r\n", ( void* )wrapper );
    #endif
    // fire with data
    wrapper->callback( context );
    // step to next
    current = current->next;
  }
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Handling of callbacks finished!\r\n" );
  #endif
}

/**
 * @brief Generic interrupt init method
 */
void interrupt_init( void ) {
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Calling arch related interrupt init\r\n" );
  #endif
  // arch related init part
  interrupt_arch_init();

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Calling post interrupt init\r\n" );
  #endif
  // possible post init
  interrupt_post_init();

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Toggle interrupts\r\n" );
  #endif
  interrupt_toggle( INTERRUPT_TOGGLE_ON );
}

/**
 * @brief Toggle interrupt on / off
 *
 * @param state
 */
void interrupt_toggle( interrupt_toggle_state_t state ) {
  // static status flag
  static bool enabled = false;

  // handle off
  if ( INTERRUPT_TOGGLE_OFF == state ) {
    // set flag
    enabled = false;
    // disable
    interrupt_disable();
  // handle on
  } else if ( INTERRUPT_TOGGLE_ON == state ) {
    // set flag
    enabled = true;
    // enable
    interrupt_enable();
  } else {
    // toggle flag
    enabled = !enabled;

    // handle enable
    if ( enabled ) {
      // enable
      interrupt_enable();
    } else {
      // disable interrupts
      interrupt_disable();
    }
  }
}
