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

#include "../lib/stdlib.h"
#include "../lib/collection/list.h"
#include "../lib/collection/avl.h"
#include "../lib/string.h"
#include "../task/process.h"
#include "../mm/phys.h"
#include "../mm/shared.h"
#if defined( PRINT_MM_SHARED )
  #include "../debug/debug.h"
#endif

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
  size = ROUND_UP_TO_FULL_PAGE( size );
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
    entry->address[ idx ] = phys_find_free_page( PAGE_SIZE, PHYS_MEMORY_TYPE_NORMAL );
    // handle error
    if ( 0 == entry->address[ idx ] ) {
      destroy_entry( entry );
      return NULL;
    }
  }

  // generate process mapping list
  entry->process_mapping = list_construct(
    lookup_process,
    cleanup_process,
    NULL
  );
  if ( ! entry->process_mapping ) {
    destroy_entry( entry );
    return NULL;
  }

  // populate remaining data
  entry->id = generate_shared_memory_id();
  entry->size = size;
  entry->use_count = 0;

  // return address
  return entry;
}

/**
 * @fn int32_t thread_compare_id_callback(const avl_node_ptr_t, const avl_node_ptr_t)
 * @brief Helper necessary for avl thread manager tree
 *
 * @param a node a
 * @param b node b
 * @return
 */
static int32_t shared_compare_id_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // debug output
  #if defined( PRINT_MM_SHARED )
    DEBUG_OUTPUT( "a = %p, b = %p\r\n", ( void* )a, ( void* )b );
    DEBUG_OUTPUT( "a->data = %zu, b->data = %zu\r\n",
      ( size_t )a->data,
      ( size_t )b->data );
  #endif

  // -1 if address of a->data is greater than address of b->data
  if ( ( size_t )a->data > ( size_t )b->data ) {
    return -1;
  // 1 if address of b->data is greater than address of a->data
  } else if ( ( size_t )b->data > ( size_t )a->data ) {
    return 1;
  }

  // equal => return 0
  return 0;
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
  shared_tree = avl_create_tree( shared_compare_id_callback, NULL, NULL );
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
 * @fn uintptr_t shared_memory_attach(task_process_ptr_t, task_thread_ptr_t, size_t, uintptr_t)
 * @brief Attached shared memory area by id
 *
 * @param process
 * @param thread
 * @param id
 * @param virt_start
 * @return
 */
uintptr_t shared_memory_attach(
  task_process_ptr_t process,
  task_thread_ptr_t thread,
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
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "Not yet initialized\r\n" )
    #endif
    return 0;
  }
  // try to get node by id
  avl_node_ptr_t node = avl_find_by_data( shared_tree, ( void* )id );
  // handle not existing
  if ( ! node ) {
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "Not such shared area existing\r\n" )
    #endif
    return 0;
  }
  shared_memory_entry_ptr_t entry = SHARED_ENTRY_GET_BLOCK( node );
  // debug output
  #if defined( PRINT_MM_SHARED )
    DEBUG_OUTPUT( "node = %#x, entry = %#x\r\n", node, entry )
    DEBUG_OUTPUT( "looking up for mapping at %#p\r\n", entry->process_mapping )
  #endif
  // lookup process
  list_item_ptr_t process_list_item = list_lookup_data(
    entry->process_mapping, process );
  // debug output
  #if defined( PRINT_MM_SHARED )
    DEBUG_OUTPUT( "process_list_item = %#x\r\n", process_list_item )
  #endif
  // handle already attached
  if ( process_list_item ) {
    // transform to mapped entry
    shared_memory_entry_mapped_ptr_t mapped = ( shared_memory_entry_mapped_ptr_t )
      process_list_item->data;
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "Area %d already attached\r\n", id )
    #endif
    // return start of mapping
    return mapped->start;
  }
  // debug output
  #if defined( PRINT_MM_SHARED )
    DEBUG_OUTPUT( "Allocating new mapping\r\n" )
  #endif
  // create mapping structure
  shared_memory_entry_mapped_ptr_t mapped = ( shared_memory_entry_mapped_ptr_t )
    malloc( sizeof( shared_memory_entry_mapped_t) );
  if ( ! mapped ) {
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "Error while allocating map entry\r\n" )
    #endif
    return 0;
  }
  // clear out
  memset( mapped, 0, sizeof( shared_memory_entry_mapped_t ) );

  uintptr_t virt = 0;
  // fixed handling means take address as start
  if ( virt_start ) {
    virt = virt_start;
    // get min and max address of context
    uintptr_t min = virt_get_context_min_address( process->virtual_context );
    uintptr_t max = virt_get_context_max_address( process->virtual_context );
    // ensure that address is in context
    if ( min > virt || max <= virt || max <= virt + entry->size ) {
      free( mapped );
      return 0;
    }
  // find free page range starting after thread entry point
  } else {
    // set address
    virt = virt_find_free_page_range(
      process->virtual_context,
      entry->size,
      ROUND_UP_TO_FULL_PAGE( thread->entry ) );
  }
  // debug output
  #if defined( PRINT_MM_SHARED )
    DEBUG_OUTPUT( "Mapping start = %#x\r\n", virt )
  #endif
  // handle error
  if ( 0 == virt ) {
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "No free virtual page range found\r\n" )
    #endif
    free( mapped );
    return 0;
  }

  // determine end
  uintptr_t start = virt;
  uintptr_t end = start + entry->size;
  size_t idx = 0;
  // map addresses
  while ( start < end ) {
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "Mapping %#x to %#x\r\n", virt, entry->address[ idx ] )
    #endif
    // map address
    if ( ! virt_map_address(
      process->virtual_context,
      start,
      entry->address[ idx ],
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
    ) ) {
      // debug output
      #if defined( PRINT_MM_SHARED )
        DEBUG_OUTPUT( "Error while mapping shared area into process\r\n" )
      #endif
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
  // populate structure
  mapped->process = process;
  mapped->size = entry->size;
  mapped->start = virt;
  // push to entry list
  if ( ! list_push_back( entry->process_mapping, mapped ) ) {
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "Error while pushing mapping entry\r\n" )
    #endif
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
  // debug output
  #if defined( PRINT_MM_SHARED )
    DEBUG_OUTPUT( "Area %d successfully mapped to %#x\r\n", id, mapped->start )
  #endif
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
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "Not initialized\r\n" )
    #endif
    return false;
  }
  // try to get node by id
  avl_node_ptr_t node = avl_find_by_data( shared_tree, ( void* )id );
  // handle not existing
  if ( ! node ) {
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "No area with id %d\r\n", id )
    #endif
    return true;
  }
  shared_memory_entry_ptr_t entry = SHARED_ENTRY_GET_BLOCK( node );
  // lookup process
  list_item_ptr_t process_list_item = list_lookup_data(
    entry->process_mapping, process );
  // handle not attached
  if ( ! process_list_item ) {
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "Process has not mapped the area\r\n" )
    #endif
    return true;
  }
  // remove from list with destruction of item
  if ( ! list_remove( entry->process_mapping, process_list_item ) ) {
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "Remove of process from mapping list failed\r\n" )
    #endif
    return false;
  }
  // handle empty ( delete shared area )
  if ( list_empty( entry->process_mapping ) ) {
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "Remove area from available tree %#x, %#x\r\n",
        shared_tree, &entry->node )
    #endif
    // remove node from tree
    avl_remove_by_node( shared_tree, &entry->node );
    // debug output
    #if defined( PRINT_MM_SHARED )
      DEBUG_OUTPUT( "Remove area from available tree\r\n" )
    #endif
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
