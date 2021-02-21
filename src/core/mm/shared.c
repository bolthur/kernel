
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

/**
 * @brief Tree of shared memory items
 */
avl_tree_ptr_t shared_tree = NULL;

/**
 * @brief List of deleted items
 */
list_manager_ptr_t deleted = NULL;

/**
 * @brief Compare entry callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_entry( const avl_node_ptr_t a, const avl_node_ptr_t b ) {
  // get blocks
  shared_memory_entry_ptr_t block_a = SHARED_ENTRY_GET_BLOCK( a );
  shared_memory_entry_ptr_t block_b = SHARED_ENTRY_GET_BLOCK( b );
  // return string comparison
  return strcmp( block_a->name, block_b->name );
}

/**
 * @brief Lookup entry callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t lookup_entry( const avl_node_ptr_t a, const void* b ) {
  // get blocks
  shared_memory_entry_ptr_t block_a = SHARED_ENTRY_GET_BLOCK( a );
  // return string comparison
  return strcmp( block_a->name, ( char* )b );
}

/**
 * @brief Compare mapped callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_mapped( const avl_node_ptr_t a, const avl_node_ptr_t b ) {
  // get blocks
  shared_memory_entry_mapped_ptr_t block_a = SHARED_MAPPED_GET_BLOCK( a );
  shared_memory_entry_mapped_ptr_t block_b = SHARED_MAPPED_GET_BLOCK( b );
  // return string comparison
  return strcmp( block_a->name, block_b->name );
}

/**
 * @brief Lookup mapped callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t lookup_mapped( const avl_node_ptr_t a, const void* b ) {
  // get blocks
  shared_memory_entry_mapped_ptr_t block_a = SHARED_MAPPED_GET_BLOCK( a );
  // return string comparison
  return strcmp( block_a->name, ( char* )b );
}

/**
 * @brief Helper to destroy single entry completely
 *
 * @param entry
 */
static void destroy_entry( shared_memory_entry_ptr_t entry ) {
  // handle invalid
  if ( ! entry ) {
    return;
  }
  // free name
  if ( entry->name ) {
    free( entry->name ) ;
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
  // free rest of structure
  free( entry );
}

/**
 * @brief list item cleanup
 * @param item
 */
static void deleted_cleanup( list_item_ptr_t item ) {
  // destroy entry
  destroy_entry( item->data );
  // delete item
  free( item );
}

/**
 * @brief Helper to destroy single mapped completely
 *
 * @param mapped
 */
static void destroy_mapped( shared_memory_entry_mapped_ptr_t mapped ) {
  // handle invalid
  if ( ! mapped ) {
    return;
  }
  // free rest of structure
  free( mapped );
}

/**
 * @brief initialize memory area
 *
 * @param name
 * @param size
 * @return
 */
bool shared_memory_initialize( const char* name, size_t size ) {
  // round up to full size
  ROUND_UP_TO_FULL_PAGE( size )
  // determine count
  size_t count = size / PAGE_SIZE;
  // get entry
  shared_memory_entry_ptr_t entry = shared_memory_retrieve_by_name( name );
  // treat non zero as initialized
  if ( ! entry || 0 < entry->size ) {
    return false;
  }
  // allocate address list
  uint64_t* addr = ( uint64_t* )calloc( count, sizeof( uint64_t ) );
  // check allocation
  if ( ! addr ) {
    return false;
  }
  // allocate pages
  for ( size_t idx = 0; idx < count; idx++ ) {
    addr[ idx ] = phys_find_free_page( PAGE_SIZE );
    // handle error
    if ( 0 == addr[ idx ] ) {
      // free pages allocated until here
      for ( size_t inner = 0; inner < idx; inner++ ) {
        phys_free_page( addr[ inner ] );
      }
      // cleanup allocated list
      free( addr );
      // return error
      return false;
    }
  }
  // populate new size and address list
  entry->size = size;
  entry->address = addr;
  // return success
  return true;
}

/**
 * @brief Create a entry object
 *
 * @param name
 * @param size
 * @return shared_memory_entry_ptr_t
 */
static shared_memory_entry_ptr_t create_entry(
  const char* name,
  uint32_t access
) {
  // create list entry
  shared_memory_entry_ptr_t entry = ( shared_memory_entry_ptr_t )malloc(
    sizeof( shared_memory_entry_t )
  );
  // check allocation
  if ( ! entry ) {
    destroy_entry( entry );
    return NULL;
  }
  // prepare area
  entry = memset( entry, 0, sizeof( shared_memory_entry_t ) );

  // allocate name array
  entry->name = ( char* )calloc( strlen( name ), sizeof( char ) );
  // check allocation
  if ( ! entry->name ) {
    destroy_entry( entry );
    return NULL;
  }

  // populate data
  entry->name = strcpy( entry->name, name );
  entry->size = 0;
  entry->use_count = 0;
  entry->address = NULL;
  entry->access = access;

  // return address
  return entry;
}

/**
 * @brief Create a mapped object
 *
 * @param name
 * @param size
 * @param address
 * @return shared_memory_entry_mapped_ptr_t
 */
static shared_memory_entry_mapped_ptr_t create_mapped(
  const char* name,
  size_t size,
  uintptr_t address,
  shared_memory_entry_ptr_t reference
) {
  // round up to full size
  ROUND_UP_TO_FULL_PAGE( size )
  // create list entry
  shared_memory_entry_mapped_ptr_t entry = ( shared_memory_entry_mapped_ptr_t )
    malloc( sizeof( shared_memory_entry_mapped_t ) );
  // check allocation
  if ( ! entry ) {
    destroy_mapped( entry );
    return NULL;
  }
  // prepare area
  entry = memset( entry, 0, sizeof( shared_memory_entry_t ) );
  // allocate name array
  entry->name = ( char* )calloc( strlen( name ), sizeof( char ) );
  // check allocation
  if ( ! entry->name ) {
    destroy_mapped( entry );
    return NULL;
  }
  // populate data
  entry->name = strcpy( entry->name, name );
  entry->start = address;
  entry->size = size;
  entry->reference = reference;
  // return created entry
  return entry;
}

/**
 * @brief Helper to setup process trees
 *
 * @param process
 * @return true
 * @return false
 */
static bool prepare_process( task_process_ptr_t process ) {
  // create tree if necessary
  if ( ! process->shared_memory_entry ) {
    process->shared_memory_entry = avl_create_tree(
      compare_entry,
      lookup_entry,
      NULL
    );
  }
  if ( ! process->shared_memory_mapped ) {
    process->shared_memory_mapped = avl_create_tree(
      compare_mapped,
      lookup_mapped,
      NULL
    );
  }
  // check for error
  if (
    ! process->shared_memory_entry
    || ! process->shared_memory_mapped
  ) {
    avl_destroy_tree( process->shared_memory_entry );
    avl_destroy_tree( process->shared_memory_mapped );
    return false;
  }
  // return success
  return true;
}

/**
 * @brief Init shared memory
 * @return true
 * @return false
 */
bool shared_init( void ) {
  // create tree
  shared_tree = avl_create_tree( compare_entry, lookup_entry, NULL );
  if ( ! shared_tree ) {
    return false;
  }
  // create deleted list
  deleted = list_construct( NULL, deleted_cleanup );
  if ( ! deleted ) {
    return false;
  }
  return true;
}

/**
 * @brief Create new shared memory area with name and size
 *
 * @param name
 * @param flags
 * @return true
 * @return false
 */
bool shared_memory_create( const char* name, uint32_t flags ) {
  // handle not initialized
  if ( ! shared_tree ) {
    return false;
  }
  // check for existence
  if ( shared_memory_retrieve_by_name( name ) ) {
    return false;
  }
  // check flags
  if (
    ! ( flags & SHARED_MEMORY_ACCESS_READ )
    && ! ( flags & SHARED_MEMORY_ACCESS_WRITE )
  ) {
    return false;
  }
  // create entry
  shared_memory_entry_ptr_t tmp = create_entry( name, flags );
  // handle error
  if ( ! tmp ) {
    return false;
  }
  // push back entry
  if ( ! avl_insert_by_node( shared_tree, &tmp->node ) ) {
    destroy_entry( tmp );
    return false;
  }
  // return success
  return true;
}

/**
 * Shared memory existing helper
 *
 * @param name
 * @return
 */
shared_memory_entry_ptr_t shared_memory_retrieve_by_name(
  const char* name
) {
  // try to get node by name
  avl_node_ptr_t node = avl_find_by_data( shared_tree, ( void* )name );
  // return node or null
  return ! node ? NULL : SHARED_ENTRY_GET_BLOCK( node );
}

/**
 * @brief Process acquire of shared memory
 *
 * @param process
 * @param name
 * @param virt_start optional start address
 * @param access
 * @return uintptr_t start address or NULL if not existing / already mapped
 */
uintptr_t shared_memory_acquire(
  task_process_ptr_t process,
  const char* name,
  uintptr_t virt_start,
  uint32_t access
) {
  // handle not initialized
  if ( ! shared_tree ) {
    return 0;
  }
  // try to get item by name
  avl_node_ptr_t node = avl_find_by_data(
    shared_tree,
    ( void* )name
  );
  // handle missing
  if ( ! node ) {
    return 0;
  }
  // cache area information
  shared_memory_entry_ptr_t area = SHARED_ENTRY_GET_BLOCK( node );
  // handle not initialized
  if ( 0 == area->size ) {
    return 0;
  }
  // prepare process structure
  if ( ! prepare_process( process ) ) {
    return 0;
  }
  // check mapping options
  if (
    (
      ( access & VIRT_PAGE_TYPE_WRITE )
      && !( area->access & SHARED_MEMORY_ACCESS_WRITE )
    ) || (
      ( access & VIRT_PAGE_TYPE_READ )
      && !( area->access & SHARED_MEMORY_ACCESS_READ )
    )
  ) {
    return 0;
  }
  // try to get item by name from process
  avl_node_ptr_t process_node = avl_find_by_data(
    process->shared_memory_entry,
    ( void* )name
  );
  // handle already attached
  if ( process_node ) {
    // get mapped entry
    avl_node_ptr_t mapped_node = avl_find_by_data(
      process->shared_memory_mapped,
      ( void* )name
    );
    // handle error
    if ( ! mapped_node ) {
      return 0;
    }
    // return start
    return ( ( shared_memory_entry_mapped_ptr_t )mapped_node->data )->start;
  }

  // find free page range
  uintptr_t virt;
  if ( 0 != virt_start ) {
    virt = virt_start;
  } else {
    virt = virt_find_free_page_range( process->virtual_context, area->size, 0 );
  }

  // determine end
  uintptr_t start = virt;
  uintptr_t end = start + area->size;
  size_t idx = 0;
  // map addresses
  while ( start < end ) {
    // map address
    if ( ! virt_map_address(
      process->virtual_context,
      start,
      area->address[ idx ],
      VIRT_MEMORY_TYPE_NORMAL,
      access
    ) ) {
      // unmap everything on error
      uintptr_t start_inner = virt;
      uintptr_t end_inner = start + area->size;
      while ( start_inner < end_inner ) {
        virt_unmap_address( process->virtual_context, start_inner, false );
        start_inner += PAGE_SIZE;
      }
      return false;
    }
    // next one
    start += PAGE_SIZE;
    idx++;
  }

  // attach item to process list
  if ( ! avl_insert_by_node( process->shared_memory_entry, node ) ) {
    start = virt;
    end = start + area->size;
    // unmap addresses
    while ( start < end ) {
      virt_unmap_address( process->virtual_context, start, false );
      start += PAGE_SIZE;
    }
  }
  // create mapped entry
  shared_memory_entry_mapped_ptr_t mapped = create_mapped(
    name, area->size, virt, area );
  // handle error
  if ( ! mapped ) {
    start = virt;
    end = start + area->size;
    // unmap addresses
    while ( start < end ) {
      virt_unmap_address( process->virtual_context, start, false );
      start += PAGE_SIZE;
    }
    // remove node
    avl_remove_by_node( process->shared_memory_entry, node );
  }
  // push back
  if ( ! avl_insert_by_node( process->shared_memory_mapped, &mapped->node ) ) {
    start = virt;
    end = start + area->size;
    // unmap addresses
    while ( start < end ) {
      virt_unmap_address( process->virtual_context, start, false );
      start += PAGE_SIZE;
    }
    // remove node
    avl_remove_by_node( process->shared_memory_entry, node );
    avl_remove_by_node( process->shared_memory_mapped, &mapped->node );
    // destroy mapped
    destroy_mapped( mapped );
  }

  // increment entry count
  area->use_count++;

  // return start address
  return virt;
}

/**
 * @brief Process release use of shared memory
 *
 * @param name name
 * @return true
 * @return false
 */
bool shared_memory_release( const char* name ) {
  // handle not initialized
  if ( ! shared_tree ) {
    return false;
  }

  // get item by name
  avl_node_ptr_t node = avl_find_by_data(
    shared_tree,
    ( void* )name
  );
  // handle missing
  if ( ! node ) {
    return false;
  }
  // get node to push to deleted list
  shared_memory_entry_ptr_t block = SHARED_ENTRY_GET_BLOCK( node );
  // insert to deleted
  if ( ! list_push_back( deleted, block ) ) {
    return false;
  }
  // remove node from tree
  avl_remove_by_node( shared_tree, node );
  // return success
  return true;
}

/**
 * Retrieve shared memory by address
 *
 * @param process
 * @param start
 * @return
 */
shared_memory_entry_mapped_ptr_t shared_memory_retrieve_by_address(
  task_process_ptr_t process,
  uintptr_t start
) {
  // get start node
  avl_node_ptr_t node = avl_iterate_first( process->shared_memory_mapped );
  // loop until end
  while ( NULL != node ) {
    // get mapped entry
    shared_memory_entry_mapped_ptr_t area =
      ( shared_memory_entry_mapped_ptr_t )node->data;
    // handle match
    if ( area->start == start ) {
      return area;
    }
    // get next
    node = avl_iterate_next( process->shared_memory_mapped, node );
  }
  // return NULL
  return NULL;
}

/**
 * @brief check whether item is in deleted
 *
 * @param reference
 * @return
 */
shared_memory_entry_ptr_t shared_memory_retrieve_deleted(
  shared_memory_entry_ptr_t reference
) {
  list_item_ptr_t item = list_lookup_data( deleted, reference );
  return ! item ? NULL : item->data;
}

/**
 * @brief remove from deleted list
 *
 * @param reference
 */
bool shared_memory_cleanup_deleted( shared_memory_entry_ptr_t reference ) {
  return list_remove_data( deleted, reference );
}
