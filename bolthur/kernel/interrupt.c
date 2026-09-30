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

#include <stddef.h>
#include "lib/string.h"
#include "lib/stdlib.h"
#include "lib/inttypes.h"
#include "lib/assert.h"
#include "../library/collection/avl/avl.h"
#include "panic.h"
#include "mm/heap.h"
#include "rpc/generic.h"
#include "interrupt.h"
#include "debug/debug.h"
#if defined( PRINT_INTERRUPT )
  #include "debug/debug.h"
#endif

/**
 * @brief Interrupt management structure
 */
static interrupt_manager_t* interrupt_manager = nullptr;

/**
 * @fn int32_t compare_interrupt_callback(const avl_node_t*, const avl_node_t*)
 * @brief Compare interrupt callback necessary for avl tree
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_interrupt_callback(
  const avl_node_t* a,
  const avl_node_t* b
) {
  // get blocks
  auto const block_a = INTERRUPT_GET_BLOCK( a );
  auto const block_b = INTERRUPT_GET_BLOCK( b );
  // -1 if address of a->interrupt is greater than address of b->interrupt
  if ( block_a->interrupt > block_b->interrupt ) {
    return -1;
  }
  // 1 if address of b->interrupt is greater than address of a->interrupt
  if ( block_b->interrupt > block_a->interrupt ) {
    return 1;
  }
  // equal => return 0
  return 0;
}

/**
 * @fn avl_tree_t* tree_by_type(interrupt_type_t)
 * @brief Helper to get interrupt manager tree by type
 * @param type type to get tree from
 * @return avl_tree_t*
 */
static avl_tree_t* tree_by_type( const interrupt_type_t type ) {
  if ( ! interrupt_manager ) {
    return nullptr;
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
      return nullptr;
  }
}

/**
 * @fn bool interrupt_unregister_handler(size_t, interrupt_callback_t, const task_process_t*, interrupt_type_t, bool, bool)
 * @brief Unregister interrupt handler
 * @param num interrupt to unbind
 * @param callback Callback to unbind
 * @param process optional process if user handler
 * @param type interrupt type
 * @param post flag to bind as post callback
 * @param disable disable interrupt if necessary
 * @return
 *
 * @todo Add removal of tree node, when all lists are empty
 * @todo Shorten process array registered interrupts if existing
 */
bool interrupt_unregister_handler(
  const size_t num,
  const interrupt_callback_t callback,
  const task_process_t* process,
  const interrupt_type_t type,
  const bool post,
  const bool disable
) {
  if ( ! heap_init_get() ) {
    return false;
  }
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT(
      "Called interrupt_unregister_handler( %zu, %#"PRIxPTR", %d, %s )\r\n",
      num,
      ( uintptr_t )callback,
      type,
      post ? "true" : "false"
    )
  #endif

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Try to unmap callback for interrupt %zu\r\n", num )
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
  avl_tree_t* tree = tree_by_type( type );
  // handle no tree
  if ( ! tree ) {
    return false;
  }
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Using interrupt tree \"%p\" for lookup!\r\n", tree )
  #endif

  // try to find node
  avl_node_t* node = avl_find_by_data( tree, num );
  interrupt_block_t* block;
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Found node %p\r\n", node )
  #endif
  // handle not yet added
  if ( ! node ) {
    return true;
  }
  // gather block
  block = INTERRUPT_GET_BLOCK( node );

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Checking for not bound interrupt callback\r\n" )
  #endif
  // get matching element
  if ( process ) {
    if ( block->external == process ) {
      block->external = nullptr;
    }
  } else {
    if ( ! post && block->internal == callback ) {
      block->internal = nullptr;
    } else if ( post && block->post == callback ) {
      block->post = nullptr;
    }
  }
  // disable interrupt in case no further handler is bound
  if (
    type == INTERRUPT_NORMAL
    && disable
    && ! block->internal
    && ! block->external
    && ! block->post
  ) {
    // enable interrupt
    interrupt_unmask_specific( ( int8_t )num );
  }
  return true;
}

/**
 * @fn bool interrupt_register_handler(size_t, interrupt_callback_t, task_process_t*, interrupt_type_t, bool, bool)
 * @brief Register interrupt handler
 * @param num Interrupt to bind
 * @param callback Callback to bind
 * @param process optional process if user handler
 * @param type interrupt type
 * @param post flag to bind as post callback
 * @param enable enable interrupt if necessary
 * @return
 */
bool interrupt_register_handler(
  const size_t num,
  const interrupt_callback_t callback,
  task_process_t* process,
  const interrupt_type_t type,
  const bool post,
  const bool enable
) {
  if ( ! heap_init_get() ) {
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "No heap initialized!\r\n" )
    #endif
    return false;
  }
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT(
      "Called interrupt_register_handler( %zu, %#"PRIxPTR", %#"PRIxPTR", %d, %s, %s )\r\n",
      num,
      ( uintptr_t )callback,
      ( uintptr_t )process,
      type,
      post ? "true" : "false",
      enable ? "true" : "false"
    )
  #endif

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Try to map callback for interrupt %zu\r\n", num )
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
  avl_tree_t* tree = tree_by_type( type );
  // check tree
  if ( ! tree ) {
    return false;
  }
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Using interrupt tree \"%p\" for lookup!\r\n", tree )
  #endif

  // try to find node
  avl_node_t* node = avl_find_by_data( tree, num );
  interrupt_block_t* block;
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Found node %p\r\n", node )
  #endif
  // handle not yet added
  if ( ! node ) {
    // reserve block
    block = malloc( sizeof( *block ) );
    // check
    if ( ! block ) {
      return false;
    }
    // prepare memory
    memset( block, 0, sizeof( *block ) );
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "Initialized new node at %p\r\n", block )
    #endif
    // populate block
    block->interrupt = num;
    // prepare and insert node
    avl_prepare_node( &block->node, num );
    if ( ! avl_insert_by_node( tree, &block->node ) ) {
      free( block );
      return false;
    }
  // existing? => gather block
  } else {
    block = INTERRUPT_GET_BLOCK( node );
  }

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Checking for already bound interrupt callback\r\n" )
  #endif
  // try to find matching element
  if ( process ) {
    if ( ! block->external ) {
      bool found = false;
      for ( size_t i = 0; i < process->registered_interrupts_size; i++ ) {
        if ( process->registered_interrupts[ i ] == num ) {
          found = true;
          break;
        }
      }
      if ( ! found ) {
        uint32_t* tmp = realloc( process->registered_interrupts, sizeof( uint32_t ) * ( process->registered_interrupts_size + 1 ) );
        if ( ! tmp ) {
          return false;
        }
        process->registered_interrupts = tmp;
        process->registered_interrupts[ process->registered_interrupts_size ] = num;
        process->registered_interrupts_size++;
      }
      block->external = process;
    }
  } else {
    if ( ! post && ! block->internal ) {
      block->internal = callback;
    } else if ( post && ! block->post ) {
      block->post = callback;
    }
  }
  // enable interrupt if set
  if ( type == INTERRUPT_NORMAL && enable ) {
    interrupt_mask_specific( ( int8_t )num );
  }
  return true;
}

/**
 * @fn void interrupt_handle(size_t, interrupt_type_t, void*, bool)
 * @brief Handle interrupt
 * @param num interrupt number
 * @param type interrupt type
 * @param context interrupt context
 * @param disable disable pending interrupt
 */
void interrupt_handle( const size_t num, const interrupt_type_t type, void* context, const bool disable ) {
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
    DEBUG_OUTPUT( "Handle interrupt %zu\r\n", num )
  #endif

  // get correct tree to use
  avl_tree_t* tree = tree_by_type( type );
  // check tree
  if ( ! tree ) {
    return;
  }

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Using interrupt tree \"%p\" for lookup!\r\n", tree )
  #endif

  // try to get node by interrupt
  avl_node_t* node = avl_find_by_data( tree, num );
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Found node %p\r\n", node )
  #endif

  // handle nothing found which means nothing bound
  if ( ! node ) {
    return;
  }
  // get interrupt block
  auto const block = INTERRUPT_GET_BLOCK( node );

  // first level handler
  if ( block->internal ) {
    // fire with data
    block->internal( context );
  }
  // bound external handler
  if ( block->external ) {
    // take first thread
    task_thread_t* thread = list_peek_front_data( block->external->thread_list );
    // debug output
    #if defined( PRINT_INTERRUPT )
      DEBUG_OUTPUT( "Raising interrupt handler %zu for %d\r\n",
        num, thread->process->id )
    #endif
    // try to raise rpc without data
    const rpc_backup_t* rpc = rpc_generic_raise(
      thread,
      block->external,
      num,
      nullptr,
      0,
      thread,
      false,
      0,
      true,
      true,
      false
    );
    // handle error by skip
    if ( rpc ) {
      // get origin
      const event_origin_t origin = event_determine_origin( context );
      // set task to switch to in case it's a different thread
      if ( task_thread_current_thread != rpc->thread ) {
        task_thread_try_switch_to = rpc->thread;
      }
      // in case we're executed from user and thread is not current thread trigger process
      if ( EVENT_ORIGIN_USER == origin && rpc->thread != task_thread_current_thread ) {
        event_enqueue( EVENT_PROCESS );
      }
    }
  }
  // post handler
  if ( block->post ) {
    block->post( context );
  }

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Handling of callbacks finished!\r\n" )
  #endif
  if ( disable ) {
    // disable interrupt to prevent it firing all along
    interrupt_disable_after_handling( ( int8_t )num );
  }
}

/**
 * @fn void interrupt_init(void)
 * @brief Generic interrupt init method
 */
void interrupt_init( void ) {
  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Calling arch related interrupt init\r\n" )
  #endif
  // arch related init part
  interrupt_arch_init();

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Calling platform interrupt init\r\n" )
  #endif
  // possible post init
  interrupt_platform_init();

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Calling post interrupt init\r\n" )
  #endif
  // possible post init
  interrupt_post_init();

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Setup interrupt trees\r\n" )
  #endif

  // initialize interrupt manager
  interrupt_manager = malloc( sizeof( *interrupt_manager ) );
  // check
  assert( interrupt_manager );
  // create trees for interrupt types
  interrupt_manager->normal_interrupt = avl_create_tree( compare_interrupt_callback, nullptr, nullptr );
  assert( interrupt_manager->normal_interrupt );
  interrupt_manager->fast_interrupt = avl_create_tree( compare_interrupt_callback, nullptr, nullptr );
  assert( interrupt_manager->fast_interrupt );
  interrupt_manager->software_interrupt = avl_create_tree( compare_interrupt_callback, nullptr, nullptr );
  assert( interrupt_manager->software_interrupt );

  // debug output
  #if defined( PRINT_INTERRUPT )
    DEBUG_OUTPUT( "Toggle on interrupts\r\n" )
  #endif
  // possible post init
  interrupt_toggle( INTERRUPT_TOGGLE_ON );
}

/**
 * @fn void interrupt_toggle(interrupt_toggle_state_t)
 * @brief Toggle interrupt on / off
 * @param state
 */
void interrupt_toggle( const interrupt_toggle_state_t state ) {
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
 * @fn void interrupt_unregister_process(const task_process_t*)
 * @brief Unregister process completely
 * @param process
 */
void interrupt_unregister_process( const task_process_t* process ) {
  auto const tree = tree_by_type( INTERRUPT_NORMAL );
  for ( size_t i = 0; i < process->registered_interrupts_size; i++ ) {
    avl_node_t* node = avl_find_by_data( tree, process->registered_interrupts[ i ] );
    if ( ! node ) {
      continue;
    }
    // get block
    auto const block = INTERRUPT_GET_BLOCK( node );
    // handle process
    if ( block->external == process ) {
      block->external = nullptr;
    }
  }
}

/**
 * @fn void interrupt_get_context*(void*)
 * @brief Method to get interrupt context
 * @param context
 */
void* interrupt_get_context( void* context ) {
  // return context if set
  if ( context ) {
    return context;
  }
  // return from task if set
  if ( task_thread_current_thread ) {
    return task_thread_current_thread->current_context;
  }
  // no context possible, panic
  PANIC( "No valid context found!" )
}
