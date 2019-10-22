
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
#include <kernel/irq.h>

/**
 * @brief Interrupt management structure
 */
irq_manager_ptr_t irq_manager = NULL;

/**
 * @brief Compare irq callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_irq_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // get blocks
  irq_block_ptr_t block_a = IRQ_GET_BLOCK( a );
  irq_block_ptr_t block_b = IRQ_GET_BLOCK( b );

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
 * @brief Unregister IRQ handler
 *
 * @param num IRQ/FIQ to bind
 * @param func Callback to bind
 * @param fast flag to bind FIQ
 */
void irq_unregister_handler(
  __unused uint8_t num,
  __unused irq_callback_t func,
  __unused bool fast
) {
  PANIC( "Unregister handling is not yet implemented!" );
}

/**
 * @brief Register IRQ handler
 *
 * @param num IRQ/FIQ to bind
 * @param func Callback to bind
 * @param fast flag to bind FIQ
 */
void irq_register_handler( uint8_t num, irq_callback_t func, bool fast ) {
  // assert heap existance
  assert( true == heap_initialized_get() );

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Try to map callback for interrupt %d\r\n", num );
  #endif
  // setup irq manager if not done existing
  if ( NULL == irq_manager ) {
    // initialize irq manager
    irq_manager = ( irq_manager_ptr_t )malloc( sizeof( irq_manager_t ) );
    // assert initialization
    assert( NULL != irq_manager );
    // prepare memory
    memset( ( void* )irq_manager, 0, sizeof( irq_manager_t ) );
    // create trees for interrupt types
    irq_manager->normal_interrupt = avl_create_tree( compare_irq_callback );
    irq_manager->fast_interrupt = avl_create_tree( compare_irq_callback );
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT(
        "Initialized irq manager with address 0x%08p\r\n",
        irq_manager
      );
    #endif
  }

  // validate irq number by fendor
  assert( irq_validate_number( num ) );

  // get correct tree to use
  avl_tree_ptr_t tree = irq_manager->normal_interrupt;
  // use fast tree if set
  if ( true == fast ) {
    tree = irq_manager->fast_interrupt;
  }
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT(
      "Using irq tree \"%s\" for lookup!\r\n",
      ( true == fast ) ? "fast_interrupt" : "normal_interrupt"
    );
  #endif

  // build temporary block
  irq_block_t tmp_block;
  // prepare memory
  memset( ( void* )&tmp_block, 0, sizeof( irq_block_t ) );
  // prepare node
  avl_prepare_node( &tmp_block.node, NULL );
  // populate necessary attributes
  tmp_block.interrupt = num;

  // try to find node
  avl_node_ptr_t node = avl_find_by_node( tree, &tmp_block.node );
  irq_block_ptr_t block;
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Found node 0x%08p\r\n", node );
  #endif
  // handle not yet added
  if ( NULL == node ) {
    // allocate block
    block = ( irq_block_ptr_t )malloc( sizeof( irq_block_t ) );
    // assert initialization
    assert( NULL != block );
    // prepare memory
    memset( ( void* )block, 0, sizeof( irq_block_t ) );
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "Initialized new node at 0x%08p\r\n", block );
    #endif
    // populate block
    block->interrupt = num;
    block->callback_list = list_construct();
    // prepare and insert node
    avl_prepare_node( &block->node, NULL );
    avl_insert_by_node( tree, &block->node );
    // overwrite node
    node = &block->node;
  // existing? => gather block
  } else {
    block = IRQ_GET_BLOCK( node );
  }

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Checking for already bound irq callback\r\n" );
  #endif
  // get first element
  list_item_ptr_t current = block->callback_list->first;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Used first element for looping at 0x%08p\r\n", current );
  #endif
  // loop through list for check callback
  while ( NULL != current ) {
    // get callback from data
    irq_callback_wrapper_ptr_t wrapper =
      ( irq_callback_wrapper_ptr_t )current->data;
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Check bound callback at 0x%08p\r\n", wrapper );
    #endif
    // handle match
    if ( wrapper->callback == func ) {
      return;
    }
    // get to next
    current = current->next;
  }

  // create wrapper
  irq_callback_wrapper_ptr_t wrapper = ( irq_callback_wrapper_ptr_t )malloc(
    sizeof( irq_callback_wrapper_t ) );
  // assert initialization
  assert( NULL != wrapper );
  // prepare memory
  memset( ( void* )wrapper, 0, sizeof( irq_callback_wrapper_t ) );

  // populate wrapper
  wrapper->callback = func;

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Created wrapper container at 0x%08p\r\n", wrapper );
  #endif

  // push to list
  list_push( block->callback_list, ( void* )wrapper );
}

/**
 * @brief Handle interrupt
 *
 * @param num interrupt number
 * @param fast fast interrupt flag
 * @param context interrupt context
 */
void irq_handle( uint8_t num, bool fast, void** context ) {
  // handle no irq manager which means no irq bound
  if ( NULL == irq_manager ) {
    return;
  }

  // validate irq number by fendor
  assert( irq_validate_number( num ) );

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Handle interrupt %d\r\n", num );
  #endif

  // get correct tree to use
  avl_tree_ptr_t tree = irq_manager->normal_interrupt;
  // use fast tree if set
  if ( true == fast ) {
    tree = irq_manager->fast_interrupt;
  }

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT(
      "Using irq tree \"%s\" for lookup!\r\n",
      ( true == fast ) ? "fast_interrupt" : "normal_interrupt"
    );
  #endif

  // build temporary block
  irq_block_t tmp_block;
  // prepare memory
  memset( ( void* )&tmp_block, 0, sizeof( irq_block_t ) );
  // prepare node
  avl_prepare_node( &tmp_block.node, NULL );
  // populate necessary attributes
  tmp_block.interrupt = num;

  // try to get node by interrupt
  avl_node_ptr_t node = avl_find_by_node( tree, &tmp_block.node );
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Found node 0x%08p\r\n", node );
  #endif

  // handle nothing found which means nothing bound
  if ( NULL == node ) {
    return;
  }
  // get irq block
  irq_block_ptr_t block = IRQ_GET_BLOCK( node );

  // get list of bound handlers
  list_manager_ptr_t list = block->callback_list;
  // get first element
  list_item_ptr_t current = list->first;

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Looping through mapped callbacks and execute them\r\n" );
  #endif

  // loop through list
  while ( NULL != current ) {
    // get callback from data
    irq_callback_wrapper_ptr_t wrapper =
      ( irq_callback_wrapper_ptr_t )current->data;

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
