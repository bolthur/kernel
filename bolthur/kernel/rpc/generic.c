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

#include <inttypes.h>
#include <errno.h>
#include "../lib/string.h"
#include "../lib/stdlib.h"
#include "backup.h"
#include "data.h"
#include "queue.h"
#include "generic.h"
#include "../panic.h"
#if defined( PRINT_RPC )
  #include "../debug/debug.h"
#endif

static avl_tree_t* origin_tree = NULL;

/**
 * @fn int32_t compare_callback(const avl_node_t*, const avl_node_t*)
 * @brief Compare callback
 *
 * @param a
 * @param b
 * @return
 */
static int32_t compare_callback( const avl_node_t* a, const avl_node_t* b ) {
  // get blocks
  rpc_origin_source_t* block_a = RPC_GET_ORIGIN_SOURCE( a );
  rpc_origin_source_t* block_b = RPC_GET_ORIGIN_SOURCE( b );
  // -1 if address of a->type is greater than address of b->type
  if ( block_a->rpc_id > block_b->rpc_id ) {
    return -1;
  // 1 if address of b->type is greater than address of a->type
  } else if ( block_b->rpc_id > block_a->rpc_id ) {
    return 1;
  }
  // equal => return 0
  return 0;
}

/**
 * @fn int32_t lookup_callback(const avl_node_t*, const void*)
 * @brief Lookup callback helper
 *
 * @param a
 * @param b
 * @return
 */
static int32_t lookup_callback(
  const avl_node_t* a,
  const void* b
) {
  rpc_origin_source_t* block = RPC_GET_ORIGIN_SOURCE( a );
  // -1 if address of a->data is greater than address of b->data
  if ( block->rpc_id > ( size_t )b) {
    return -1;
  // 1 if address of b->data is greater than address of a->data
  } else if ( ( size_t )b > block->rpc_id ) {
    return 1;
  }
  // equal => return 0
  return 0;
}

/**
 * @fn void cleanup_callback(avl_node_t*)
 * @brief Cleanup callback helper
 *
 * @param a
 */
static void cleanup_callback( avl_node_t* a ) {
  rpc_origin_source_t* block = RPC_GET_ORIGIN_SOURCE( a );
  free( block );
}

/**
 * @fn rpc_origin_source_t rpc_generic_source_info*(size_t)
 * @brief Helper to get origin source information data
 *
 * @param id
 * @return
 */
rpc_origin_source_t* rpc_generic_source_info( size_t id ) {
  // try to find node by data
  avl_node_t* node = avl_find_by_data( origin_tree, ( void* )id );
  if ( ! node ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "origin information for id %d not found!\r\n", id )
    #endif
    // return null
    return NULL;
  }
  // return block
  return RPC_GET_ORIGIN_SOURCE( node );
}

/**
 * @fn void rpc_generic_remove_source_info(rpc_origin_source_t*)
 * @brief Helper to remove source info from tree
 *
 * @param info
 */
void rpc_generic_destroy_source_info( rpc_origin_source_t* info ) {
  if ( ! info || ! info->rpc_id ) {
    return;
  }
  // remove from tree
  avl_remove_by_data( origin_tree, ( void* )( info->rpc_id ) );
  // free info
  free( info );
}

/**
 * @fn rpc_backup_t* rpc_generic_raise(task_thread_t*, task_process_t*, size_t, void*, size_t, task_thread_t*, bool, size_t, bool)
 * @brief Raise an rpc in target from source
 *
 * @param source
 * @param target
 * @param type
 * @param data
 * @param length
 * @param target_thread
 * @param sync
 * @param origin_data_id
 * @param disable_data
 * @return
 */
rpc_backup_t* rpc_generic_raise(
  task_thread_t* source,
  task_process_t* target,
  size_t type,
  void* data,
  size_t length,
  task_thread_t* target_thread,
  bool sync,
  size_t origin_data_id,
  bool disable_data
) {
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT(
      "rpc_raise( %#p, %#p, %#p, %#zx, %#p )\r\n",
      source, target, data, length, target_thread )
  #endif
  // backup necessary stuff
  rpc_backup_t* backup = rpc_backup_create(
    source,
    target,
    type,
    data,
    length,
    target_thread,
    sync,
    origin_data_id,
    disable_data
  );
  if ( ! backup ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Error while creating backup for target %d\r\n", target->id )
    #endif
    // skip if backup could not be created
    return NULL;
  }
  // prepare thread
  if ( ! rpc_generic_prepare_invoke( backup ) ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Error while preparing target %d\r\n", target->id )
    #endif
    rpc_backup_destroy( backup );
    // skip if error occurred during rpc invoke
    return NULL;
  }
  // allocate new structure for tree
  if ( backup->data_id ) {
    rpc_origin_source_t* rpc_info = malloc( sizeof( *rpc_info ) );
    if ( ! rpc_info ) {
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT( "Error while allocating rpc info object\r\n" )
      #endif
      rpc_backup_destroy( backup );
      return NULL;
    }
    // clear out
    memset( rpc_info, 0, sizeof( *rpc_info ) );
    // prepare structure
    rpc_info->source_process = source->process->id;
    rpc_info->rpc_id = backup->data_id;
    rpc_info->origin_rpc_id = origin_data_id;
    rpc_info->sync = sync;
    // prepare node
    avl_prepare_node( &rpc_info->node, ( void* )backup->data_id );
    // add to tree
    if ( ! avl_insert_by_node( origin_tree, &rpc_info->node ) ) {
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT( "Error while adding information to origin tree\r\n" )
      #endif
      free( rpc_info );
      rpc_backup_destroy( backup );
      return NULL;
    }
  }
  // return created backup
  return backup;
}

/**
 * @fn bool rpc_generic_init(void)
 * @brief Setup necessary stuff for rpc handling
 *
 * @return
 */
bool rpc_generic_init( void ) {
  // create tree for origin source handling
  origin_tree = avl_create_tree(
    compare_callback,
    lookup_callback,
    cleanup_callback
  );
  return origin_tree;
}

/**
 * @fn bool rpc_generic_setup(task_process_t*)
 * @brief Setup rpc queue
 *
 * @param proc
 * @return
 */
bool rpc_generic_setup( task_process_t* proc ) {
  // setup data queue handling
  if ( ! rpc_data_queue_setup( proc ) ) {
    return false;
  }
  if ( ! rpc_queue_setup( proc ) ) {
    rpc_generic_destroy( proc );
    return false;
  }
  return true;
}

/**
 * @fn bool rpc_generic_ready(task_process_t*)
 * @brief Check if process is rpc ready
 *
 * @param proc
 * @return
 */
bool rpc_generic_ready( task_process_t* proc ) {
  return rpc_data_queue_ready( proc )
    && rpc_queue_ready( proc )
    && proc->rpc_handler
    && proc->rpc_ready;
}

/**
 * @fn void rpc_generic_destroy(task_process_t*)
 * @brief Destroy rpc queue
 *
 * @param proc
 */
void rpc_generic_destroy( task_process_t* proc ) {
  rpc_data_queue_destroy( proc );
  rpc_queue_destroy( proc );
}
