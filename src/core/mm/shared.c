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
#include <collection/list.h>
#include <collection/avl.h>
#include <string.h>
#include <core/task/process.h>
#include <core/mm/phys.h>
#include <core/mm/shared.h>
#if defined( PRINT_MM_SHARED )
#include <core/debug/debug.h>
#endif

#include <inttypes.h>

/**
 * @brief Tree of shared memory items
 */
avl_tree_ptr_t shared_tree = NULL;

/**
 * @fn int32_t lookup_process(const list_item_ptr_t, const void*)
 * @brief Tree helper for process list lookup
 *
 * @param a
 * @param b
 * @return
 */
static int32_t lookup_process( const list_item_ptr_t a, const void* b ) {
  // get blocks
  shared_memory_entry_mapped_ptr_t item = ( shared_memory_entry_mapped_ptr_t )
    a->data;
  // compare process structures
  if ( item->process == ( task_process_ptr_t )b ) {
    return 0;
  }
  return 1;
}

/**
 * @fn void cleanup_process(const list_item_ptr_t)
 * @brief Helper to cleanup process list
 *
 * @param a
 */
static void cleanup_process( const list_item_ptr_t a ) {
  // get blocks
  shared_memory_entry_mapped_ptr_t item = ( shared_memory_entry_mapped_ptr_t )
    a->data;
  // set start and end
  uintptr_t start = item->start;
  uintptr_t end = start + item->size;
  // loop until end and unmap
  while ( start < end ) {
    // unmap
    virt_unmap_address( item->process->virtual_context, start, false );
    // get next page
    start += PAGE_SIZE;
  }
}

/**
 * @fn size_t generate_shared_memory_id(void)
 * @brief Helper to generate new shared memory id
 *
 * @return
 */
static size_t generate_shared_memory_id( void ) {
  static size_t id = 1;
  return id++;
}

/**
 * @fn void destroy_entry(shared_memory_entry_ptr_t)
 * @brief Helper to destroy single entry completely
 *
 * @param entry
 */
static void destroy_entry( shared_memory_entry_ptr_t entry ) {
  // handle invalid
  if ( ! entry ) {
    return;
  }
  // free address list
  if ( entry->address ) {
    // free pages again
    for (
      size_t count = entry->size / PAGE_SIZE, idx = 0;
      idx < count;
      idx++
    ) {
      if ( 0 != entry->address[ idx ] ) {
        phys_free_page( entry->address[ idx ] );
      }
    }
    // free array
    free( entry->address );
  }
  // free process mapping list if existing
  if ( entry->process_mapping ) {
    list_destruct( entry->process_mapping );
  }
  // free rest of structure
  free( entry );
}

/**
 * @fn shared_memory_entry_ptr_t create_entry(size_t)
 * @brief Helper to create a entry object
 *
 * @param size
 * @return
 */
static shared_memory_entry_ptr_t create_entry( size_t size ) {
  // create list entry
  shared_memory_entry_ptr_t entry = ( shared_memory_entry_ptr_t )malloc(
    sizeof( shared_memory_entry_t )
  );
  // check allocation
  if ( ! entry ) {
    return NULL;
  }
  // prepare area
  entry = memset( entry, 0, sizeof( shared_memory_entry_t ) );

  // round up to full size
  ROUND_UP_TO_FULL_PAGE( size )
  // determine count
  size_t count = size / PAGE_SIZE;
  // allocate address list
  entry->address = ( uint64_t* )calloc( count, sizeof( uint64_t ) );
  // check allocation
  if ( ! entry->address ) {
    destroy_entry( entry );
    return NULL;
  }
  // allocate pages
  for ( size_t idx = 0; idx < count; idx++ ) {
    entry->address[ idx ] = phys_find_free_page( PAGE_SIZE );
    // handle error
    if ( 0 == entry->address[ idx ] ) {
      destroy_entry( entry );
      return NULL;
    }
  }

  // generate process mapping list
  entry->process_mapping = list_construct( lookup_process, cleanup_process );
  if ( ! entry->process_mapping ) {
    destroy_entry( entry );
    return NULL;
  }

  // populate remaining data
  entry->id = generate_shared_memory_id();
  entry->size = size;
  entry->use_count = 0;
  entry->address = NULL;

  // return address
  return entry;
}

/**
 * @fn bool shared_memory_init(void)
 * @brief Initialize shared memory functionality
 *
 * @return
 */
bool shared_memory_init( void ) {
  // debug output
  #if defined( PRINT_MM_SHARED )
    DEBUG_OUTPUT( "shared_memory_init()\r\n" )
  #endif
  // create tree
  shared_tree = avl_create_tree( NULL, NULL, NULL );
  if ( ! shared_tree ) {
    return false;
  }
  // return success
  return true;
}

/**
 * @fn size_t shared_memory_create(size_t)
 * @brief Create new shared memory area
 *
 * @param len shared area size
 * @return
 */
size_t shared_memory_create( size_t len ) {
  // debug output
  #if defined( PRINT_MM_SHARED )
    DEBUG_OUTPUT( "shared_memory_create( %zu )\r\n", len )
  #endif
  // handle not initialized or invalid length
  if ( ! shared_tree || 0 == len ) {
    return 0;
  }
  // create new block
  shared_memory_entry_ptr_t entry = create_entry( len );
  if ( ! entry ) {
    return 0;
  }
  // prepare node
  avl_prepare_node( &entry->node, ( void* )entry->id );
  // add new item to tree
  if ( ! avl_insert_by_node( shared_tree, &entry->node ) ) {
    destroy_entry( entry );
    return 0;
  }
  // return id of new shared area
  return entry->id;
}

/**
 * @fn uintptr_t shared_memory_attach(task_process_ptr_t, size_t, uintptr_t)
 * @brief Attached shared memory area by id
 *
 * @param process
 * @param id
 * @param virt_start
 * @return
 *
 * @todo add support for start address usually passed by mmap as orientation
 */
uintptr_t shared_memory_attach(
  task_process_ptr_t process,
  size_t id,
  uintptr_t virt_start
) {
  // debug output
  #if defined( PRINT_MM_SHARED )
    DEBUG_OUTPUT(
      "shared_memory_attach( %d, %zu, %#"PRIxPTR" )\r\n",
      process->id, id, virt_start )
  #endif
  // handle not initialized
  if ( ! shared_tree ) {
    return 0;
  }
  // try to get node by id
  avl_node_ptr_t node = avl_find_by_data( shared_tree, ( void* )id );
  // handle not existing
  if ( ! node ) {
    return 0;
  }
  shared_memory_entry_ptr_t entry = SHARED_ENTRY_GET_BLOCK( node );
  // lookup process
  list_item_ptr_t process_list_item = list_lookup_data(
    entry->process_mapping, process );
  // handle already attached
  if ( process_list_item ) {
    // transform to mapped entry
    shared_memory_entry_mapped_ptr_t mapped = ( shared_memory_entry_mapped_ptr_t )
      process_list_item->data;
    // return start of mapping
    return mapped->start;
  }
  // create mapping structure
  shared_memory_entry_mapped_ptr_t mapped = ( shared_memory_entry_mapped_ptr_t )
    malloc( sizeof( shared_memory_entry_mapped_t) );
  if ( ! mapped ) {
    return 0;
  }
  // clear out
  memset( mapped, 0, sizeof( shared_memory_entry_mapped_t ) );
  // find free page range
  uintptr_t virt = virt_find_free_page_range(
    process->virtual_context,
    entry->size,
    virt_start
  );
  // handle error
  if ( 0 == virt ) {
    free( mapped );
    return 0;
  }

  // determine end
  uintptr_t start = virt;
  uintptr_t end = start + entry->size;
  size_t idx = 0;
  // map addresses
  while ( start < end ) {
    // map address
    if ( ! virt_map_address(
      process->virtual_context,
      start,
      entry->address[ idx ],
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
    ) ) {
      // unmap everything on error
      uintptr_t start_inner = virt;
      uintptr_t end_inner = start + entry->size;
      while ( start_inner < end_inner ) {
        virt_unmap_address( process->virtual_context, start_inner, false );
        start_inner += PAGE_SIZE;
      }
      return 0;
    }
    // next one
    start += PAGE_SIZE;
    idx++;
  }
  // push to entry list
  if ( ! list_push_back( entry->process_mapping, mapped ) ) {
    // unmap everything on error
     uintptr_t start_inner = virt;
     uintptr_t end_inner = start + entry->size;
     while ( start_inner < end_inner ) {
       virt_unmap_address( process->virtual_context, start_inner, false );
       start_inner += PAGE_SIZE;
     }
    free( mapped );
    return 0;
  }
  // return mapped address
  return mapped->start;
}

/**
 * @fn bool shared_memory_detach(task_process_ptr_t, size_t)
 * @brief Detach shared memory area by id
 *
 * @param process
 * @param id
 * @return
 */
bool shared_memory_detach( task_process_ptr_t process, size_t id ) {
  // debug output
  #if defined( PRINT_MM_SHARED )
    DEBUG_OUTPUT(
      "shared_memory_detach( %d, %zu )\r\n",
      process->id, id )
  #endif
  // handle not initialized
  if ( ! shared_tree ) {
    return false;
  }
  // try to get node by id
  avl_node_ptr_t node = avl_find_by_data( shared_tree, ( void* )id );
  // handle not existing
  if ( ! node ) {
    return true;
  }
  shared_memory_entry_ptr_t entry = SHARED_ENTRY_GET_BLOCK( node );
  // lookup process
  list_item_ptr_t process_list_item = list_lookup_data(
    entry->process_mapping, process );
  // handle not attached
  if ( ! process_list_item ) {
    return true;
  }
  // remove from list with destruction of item
  if ( ! list_remove( entry->process_mapping, process_list_item ) ) {
    return false;
  }
  // handle empty ( delete shared area )
  if ( list_empty( entry->process_mapping ) ) {
    // remove node from tree
    avl_remove_by_node( shared_tree, &entry->node );
    // destroy entry itself
    destroy_entry( entry );
  }
  // return success
  return true;
}

/**
 * @fn bool shared_memory_address_is_shared(task_process_ptr_t, uintptr_t, size_t)
 * @brief Check if area is somehow in shared
 *
 * @param process
 * @param start
 * @param len
 * @return
 */
bool shared_memory_address_is_shared(
  task_process_ptr_t process,
  uintptr_t start,
  size_t len
) {
  // get start node
  avl_node_ptr_t node = avl_iterate_first( shared_tree );
  // loop until end
  while ( NULL != node ) {
    // get mapped entry
    shared_memory_entry_ptr_t entry = SHARED_ENTRY_GET_BLOCK( node );
    // lookup process
    list_item_ptr_t process_list_item = list_lookup_data(
      entry->process_mapping, process );
    // handle attached
    if ( process_list_item ) {
      // transform to mapped entry
      shared_memory_entry_mapped_ptr_t mapped = ( shared_memory_entry_mapped_ptr_t )
        process_list_item->data;
      // handle match
      if (
        start <= ( mapped->start + mapped->size )
        && ( start + len ) >= mapped->start
      ) {
        return true;
      }
    }
    // get next
    node = avl_iterate_next( shared_tree, node );
  }
  // return NULL
  return false;
}

/**
 * @fn bool shared_memory_fork(task_process_ptr_t, task_process_ptr_t)
* @brief Method to duplicate shared memory entries during fork
 *
 * @param process_to_fork
 * @param process_fork
 * @return
 */
bool shared_memory_fork(
  task_process_ptr_t process_to_fork,
  task_process_ptr_t process_fork
) {
  // get start node
  avl_node_ptr_t node = avl_iterate_first( shared_tree );
  // loop until end
  while ( NULL != node ) {
    // get mapped entry
    shared_memory_entry_ptr_t entry = SHARED_ENTRY_GET_BLOCK( node );
    // lookup process
    list_item_ptr_t process_list_item = list_lookup_data(
      entry->process_mapping, process_to_fork );
    // handle attached
    if ( process_list_item ) {
      // transform to mapped entry
      shared_memory_entry_mapped_ptr_t mapped_to_fork = ( shared_memory_entry_mapped_ptr_t )
        process_list_item->data;
      // allocate new entry
      shared_memory_entry_mapped_ptr_t mapped_fork = ( shared_memory_entry_mapped_ptr_t )
        malloc( sizeof( shared_memory_entry_mapped_t) );
      if ( ! mapped_fork ) {
        return false;
      }
      memset( mapped_fork, 0, sizeof( shared_memory_entry_mapped_t ) );
      // duplicate data
      mapped_fork->process = process_fork;
      mapped_fork->size = mapped_to_fork->size;
      mapped_fork->start = mapped_to_fork->start;
      // push to entry list
      if ( ! list_push_back( entry->process_mapping, mapped_fork ) ) {
        // free and return error
        free( mapped_fork );
        return false;
      }
    }
    // get next
    node = avl_iterate_next( shared_tree, node );
  }
  // return success
  return true;
}

/**
 * @fn bool shared_memory_cleanup_process(task_process_ptr_t)
 * @brief Method to cleanup all assigned shared memory areas
 *
 * @param proc
 * @return
 */
bool shared_memory_cleanup_process( task_process_ptr_t proc ) {
  // get start node
  avl_node_ptr_t node = avl_iterate_first( shared_tree );
  // loop until end
  while ( NULL != node ) {
    // get mapped entry
    shared_memory_entry_ptr_t entry = SHARED_ENTRY_GET_BLOCK( node );
    // detach shared memory
    if ( ! shared_memory_detach( proc, entry->id ) ) {
      return false;
    }
    // get next
    node = avl_iterate_next( shared_tree, node );
  }
  // return success
  return true;
}
