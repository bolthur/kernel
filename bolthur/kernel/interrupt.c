/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include "lib/string.h"
#include "lib/stdlib.h"
#include "lib/collection/avl.h"
#include "mm/heap.h"
#include "rpc/generic.h"
#include "interrupt.h"
#if defined( PRINT_EVENT )
  #include "debug/debug.h"
#endif

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

  // -1 if address of a->interrupt is greater than address of b->interrupt
  if ( block_a->interrupt > block_b->interrupt ) {
    return -1;
  // 1 if address of b->interrupt is greater than address of a->interrupt
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
    interrupt_manager = malloc( sizeof( interrupt_manager_t ) );
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
    // default: invalid
    default:
      return NULL;
  }
}

/**
 * @fn int32_t block_list_lookup(const list_item_ptr_t, const void*)
 * @brief interrupt block list cleanup helper
 * @param a
 */
static int32_t kernel_block_list_lookup( const list_item_ptr_t a, const void* data ) {
  // get callback from data
  interrupt_callback_wrapper_ptr_t wrapper = a->data;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Check bound callback at %p\r\n", ( void* )wrapper );
  #endif
  // handle match
  return ( uintptr_t )data == ( uintptr_t )wrapper->callback ? 0 : 1;
}

/**
 * @fn void block_list_cleanup(const list_item_ptr_t)
 * @brief interrupt block list cleanup helper
 * @param a
 */
static void kernel_block_list_cleanup( const list_item_ptr_t a ) {
  if ( a->data ) {
    free( a );
  }
  list_default_cleanup( a );
}

/**
 * @fn int32_t block_list_lookup(const list_item_ptr_t, const void*)
 * @brief interrupt block list cleanup helper
 * @param a
 */
static int32_t process_block_list_lookup( const list_item_ptr_t a, const void* data ) {
  // get callback from data
  task_process_ptr_t proc = a->data;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Check bound callback at %p\r\n", ( void* )wrapper );
  #endif
  // handle match
  return ( pid_t )data == proc->id ? 0 : 1;
}

/**
 * @fn bool interrupt_unregister_handler(size_t, interrupt_callback_t, task_process_ptr_t, interrupt_type_t, bool, bool)
 * @brief Unregister interrupt handler
 *
 * @param num interrupt to unbind
 * @param callback Callback to unbind
 * @param process optional process if user handler
 * @param type interrupt type
 * @param post flag to bind as post callback
 * @param disable disable interrupt if necessary
 * @return
 *
 * @todo Add removal of tree node, when all lists are empty
 */
bool interrupt_unregister_handler(
  size_t num,
  interrupt_callback_t callback,
  task_process_ptr_t process,
  interrupt_type_t type,
  bool post,
  bool disable
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
  list_manager_ptr_t list = NULL;
  list_item_ptr_t match = NULL;
  // get matching element
  if ( process ) {
    list = block->process;
    match = list_lookup_data( list, ( void* )process->id );
  } else {
    list = true != post ? block->handler : block->post;
    match = list_lookup_data( list, ( void* )( ( uintptr_t )callback ) );
  }
  // handle no match
  if ( ! match ) {
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Callback not bound, returning success\r\n" );
    #endif
    return true;
  }
  // disable interrupt in case no further handler is bound
  if (
    type == INTERRUPT_NORMAL
    && disable
    && list_empty( block->process )
    && list_empty( block->handler )
    && list_empty( block->post )
  ) {
    // enable interrupt
    interrupt_unmask_specific( ( int8_t )num );
  }
  // remove element
  return list_remove( list, match );
}

/**
 * @fn bool interrupt_register_handler(size_t, interrupt_callback_t, task_process_ptr_t, interrupt_type_t, bool, bool)
 * @brief Register interrupt handler
 *
 * @param num Interrupt to bind
 * @param callback Callback to bind
 * @param process optional process if user handler
 * @param type interrupt type
 * @param post flag to bind as post callback
 * @param enable enable interrupt if necessary
 * @return
 */
bool interrupt_register_handler(
  size_t num,
  interrupt_callback_t callback,
  task_process_ptr_t process,
  interrupt_type_t type,
  bool post,
  bool enable
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
    block = malloc( sizeof( interrupt_block_t ) );
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
    block->handler = list_construct(
      kernel_block_list_lookup,
      kernel_block_list_cleanup,
      NULL
    );
    // check list
    if ( ! block->handler ) {
      free( block );
      return false;
    }
    block->post = list_construct(
      kernel_block_list_lookup,
      kernel_block_list_cleanup,
      NULL
    );
    // check list
    if ( ! block->post ) {
      list_destruct( block->handler );
      free( block );
      return false;
    }
    block->process = list_construct( process_block_list_lookup, NULL, NULL );
    // check list
    if ( ! block->process ) {
      list_destruct( block->post );
      list_destruct( block->handler );
      free( block );
      return false;
    }
    // prepare and insert node
    avl_prepare_node( &block->node, ( void* )num );
    if ( ! avl_insert_by_node( tree, &block->node ) ) {
      list_destruct( block->process );
      list_destruct( block->post );
      list_destruct( block->handler );
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
  list_manager_ptr_t list = NULL;
  list_item_ptr_t match = NULL;
  // try to find matching element
  if ( process ) {
    list = block->process;
    match = list_lookup_data( list, ( void* )process->id );
  } else {
    list = true != post ? block->handler : block->post;
    match = list_lookup_data( list, ( void* )( ( uintptr_t )callback ) );
  }
  // if already existing, just return success
  if ( match ) {
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Callback already bound, returning success\r\n" );
    #endif
    return true;
  }
  void* data = NULL;
  if ( process ) {
    // set data to process
    data = process;
  } else {
    // create wrapper
    interrupt_callback_wrapper_ptr_t wrapper = malloc(
      sizeof( interrupt_callback_wrapper_t )
    );
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
    // set data to wrapper
    data = ( void* )wrapper;
  }
  // enable interrupt if set
  if ( type == INTERRUPT_NORMAL && enable ) {
    interrupt_mask_specific( ( int8_t )num );
  }
  // push to list
  return list_push_back( list, data );
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
    interrupt_callback_wrapper_ptr_t wrapper = current->data;
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "Handling wrapper container %p\r\n", ( void* )wrapper );
    #endif
    // fire with data
    wrapper->callback( context );
    // step to next
    current = current->next;
  }

  // get first element of process handlers
  current = block->process->first;
  while ( current ) {
    task_process_ptr_t process = current->data;
    // get first thread
    avl_node_ptr_t first = avl_iterate_first( process->thread_manager );
    // handle no thread with removal and skip
    if ( ! first ) {
      list_item_ptr_t next = current->next;
      list_remove( block->process, current );
      current = next;
      continue;
    }
    // get thread
    task_thread_ptr_t thread = TASK_THREAD_GET_BLOCK( first );
    // try to raise rpc without data
    rpc_backup_ptr_t rpc = rpc_generic_raise(
      thread,
      process,
      num,
      NULL,
      0,
      NULL,
      false,
      0,
      true
    );
    // handle error by skip
    if ( ! rpc ) {
      // debug output
      #if defined( PRINT_EVENT )
        DEBUG_OUTPUT( "Unable to raise rpc\r\n" )
      #endif
      current = current->next;
      continue;
    }
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
    interrupt_callback_wrapper_ptr_t wrapper = current->data;
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

/**
 * @fn void interrupt_handle_possible(void*, bool)
 * @brief Method to enqueue possible interrupt handler
 *
 * @param context
 * @param fast
 */
void interrupt_handle_possible( void* context, bool fast ) {
  int8_t interrupt_bit;
  // get pending interrupt
  while( -1 != ( interrupt_bit = interrupt_get_pending( fast ) ) ) {
    // transform bit to interrupt
    uint32_t interrupt = ( 1U << interrupt_bit );
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "pending interrupt: %#x\r\n", interrupt );
    #endif
    // call interrupt handler
    interrupt_handle(
      interrupt,
      fast ? INTERRUPT_FAST : INTERRUPT_NORMAL,
      context
    );
  }
}

/**
 * @fn void interrupt_unregister_process(task_process_ptr_t)
 * @brief Unregister process completely
 *
 * @param process
 */
void interrupt_unregister_process( task_process_ptr_t process ) {
  avl_tree_ptr_t tree = tree_by_type( INTERRUPT_NORMAL );
  // get first entry
  avl_node_ptr_t avl_list = avl_iterate_first( tree );
  while ( avl_list ) {
    interrupt_block_ptr_t block = INTERRUPT_GET_BLOCK( avl_list );
    // try to find process
    list_item_ptr_t match = list_lookup_data(
      block->process,
      ( void* )process->id
    );
    // remove if there is a match
    if ( match ) {
      list_remove( block->process, match );
    }
    // get next list
    avl_list = avl_iterate_next( tree, avl_list );
  }
}
