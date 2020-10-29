
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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
#include <collection/avl.h>
#include <string.h>
#include <core/panic.h>
#include <core/mm/phys.h>
#include <core/mm/shared.h>

/**
 * @brief Tree of shared memory items
 */
avl_tree_ptr_t shared_tree = NULL;

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
  if ( NULL == entry ) {
    return;
  }
  // free name
  if ( NULL != entry->name ) {
    free( entry->name ) ;
  }
  // free address list
  if ( NULL != entry->address_list ) {
    // free pages again
    for (
      size_t count = entry->size / PAGE_SIZE, idx = 0;
      idx < count;
      idx++
    ) {
      if ( 0 != entry->address_list[ idx ] ) {
        phys_free_page( entry->address_list[ idx ] );
      }
    }
    // free array
    free( entry->address_list );
  }
  // free rest of structure
  free( entry );
}

/**
 * @brief Helper to destroy single mapped completely
 *
 * @param entry
 */
static void destroy_mapped( shared_memory_entry_mapped_ptr_t mapped ) {
  // handle invalid
  if ( NULL == mapped ) {
    return;
  }
  // free rest of structure
  free( mapped );
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
  size_t size
) {
  // round up to full size
  ROUND_UP_TO_FULL_PAGE( size )

  // determine count
  size_t count = size / PAGE_SIZE;

  // create list entry
  shared_memory_entry_ptr_t entry = ( shared_memory_entry_ptr_t )malloc(
    sizeof( shared_memory_entry_t )
  );
  // check allocation
  if ( NULL == entry ) {
    destroy_entry( entry );
    return NULL;
  }
  // prepare area
  entry = memset( entry, 0, sizeof( shared_memory_entry_t ) );

  // allocate name array
  entry->name = ( char* )calloc( strlen( name ), sizeof( char ) );
  // check allocation
  if ( NULL == entry->name ) {
    destroy_entry( entry );
    return NULL;
  }
  // allocate address list
  entry->address_list = ( uint64_t* )calloc( count, sizeof( uint64_t ) );
  // check allocation
  if ( NULL == entry->name ) {
    destroy_entry( entry );
    return NULL;
  }

  // copy data
  entry->name = strcpy( entry->name, name );
  // allocate pages
  for ( size_t idx = 0; idx < count; idx++ ) {
    entry->address_list[ idx ] = phys_find_free_page( PAGE_SIZE );
    // handle error
    if ( 0 == entry->address_list[ idx ] ) {
      destroy_entry( entry );
      return NULL;
    }
  }
  entry->size = size;
  entry->use_count = 0;

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
  uintptr_t address
) {
  // round up to full size
  ROUND_UP_TO_FULL_PAGE( size )

  // create list entry
  shared_memory_entry_mapped_ptr_t entry = ( shared_memory_entry_mapped_ptr_t )
    malloc( sizeof( shared_memory_entry_mapped_t ) );
  // check allocation
  if ( NULL == entry ) {
    destroy_mapped( entry );
    return NULL;
  }
  // prepare area
  entry = memset( entry, 0, sizeof( shared_memory_entry_t ) );

  // allocate name array
  entry->name = ( char* )calloc( strlen( name ), sizeof( char ) );
  // check allocation
  if ( NULL == entry->name ) {
    destroy_mapped( entry );
    return NULL;
  }

  // copy data
  entry->name = strcpy( entry->name, name );
  entry->start = address;
  entry->size = size;

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
  if ( NULL == process->shared_memory_entry ) {
    process->shared_memory_entry = avl_create_tree(
      compare_entry,
      lookup_entry,
      NULL
    );
  }
  if ( NULL == process->shared_memory_mapped ) {
    process->shared_memory_mapped = avl_create_tree(
      compare_mapped,
      lookup_mapped,
      NULL
    );
  }
  // check for error
  if (
    NULL == process->shared_memory_entry
    || NULL == process->shared_memory_mapped
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
  shared_tree = avl_create_tree(
    compare_entry,
    lookup_entry,
    NULL
  );
  return NULL != shared_tree;
}

/**
 * @brief Create new shared memory area with name and size
 *
 * @param name name of the area
 * @param size area size
 * @return true
 * @return false
 */
bool shared_memory_create( const char* name, size_t size ) {
  // handle not initialized
  if ( NULL == shared_tree ) {
    return false;
  }

  // try to find node
  avl_node_ptr_t node = avl_find_by_data(
    shared_tree,
    ( void* )name
  );
  // skip if name is already in use
  if ( NULL != node ) {
    return false;
  }

  // create entry
  shared_memory_entry_ptr_t tmp = create_entry( name, size );
  // handle error
  if ( NULL == tmp ) {
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
 * @brief Process acquire of shared memory
 *
 * @param process process
 * @param name name
 * @return uintptr_t start address or NULL if not existing / already mapped
 */
uintptr_t shared_memory_acquire( task_process_ptr_t process, const char* name ) {
  // handle not initialized
  if ( NULL == shared_tree ) {
    return 0;
  }

  // prepare process
  if ( ! prepare_process( process ) ) {
    return 0;
  }

  // try to get item by name
  avl_node_ptr_t node = avl_find_by_data(
    shared_tree,
    ( void* )name
  );
  // handle missing
  if ( NULL == node ) {
    return 0;
  }
  // try to get item by name from process
  avl_node_ptr_t process_node = avl_find_by_data(
    process->shared_memory_entry,
    ( void* )name
  );
  // handle already attached
  if ( NULL != process_node ) {
    // get mapped entry
    avl_node_ptr_t mapped_node = avl_find_by_data(
      process->shared_memory_mapped,
      ( void* )name
    );
    // handle error
    if ( NULL == mapped_node ) {
      return 0;
    }
    // return start
    return ( ( shared_memory_entry_mapped_ptr_t )mapped_node->data )->start;
  }

  shared_memory_entry_ptr_t tmp = ( shared_memory_entry_ptr_t )node->data;
  // find free page range
  uintptr_t virt = virt_find_free_page_range(
    process->virtual_context,
    tmp->size
  );

  // determine end
  uintptr_t start = virt;
  uintptr_t end = start + tmp->size;
  size_t idx = 0;
  // map addresses
  while ( start < end ) {
    // map address
    if ( ! virt_map_address(
      process->virtual_context,
      start,
      tmp->address_list[ idx ],
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_NON_EXECUTABLE
    ) ) {
      // unmap everything on error
      uintptr_t start_inner = virt;
      uintptr_t end_inner = start + tmp->size;
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
    end = start + tmp->size;
    // unmap addresses
    while ( start < end ) {
      virt_unmap_address( process->virtual_context, start, false );
      start += PAGE_SIZE;
    }
  }
  // create mapped entry
  shared_memory_entry_mapped_ptr_t mapped = create_mapped(
    name, tmp->size, virt );
  // handle error
  if ( NULL == mapped ) {
    start = virt;
    end = start + tmp->size;
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
    end = start + tmp->size;
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
  tmp->use_count++;

  // return start address
  return virt;
}

/**
 * @brief Process release use of shared memory
 *
 * @param process process
 * @param name name
 * @return true
 * @return false
 */
bool shared_memory_release( task_process_ptr_t process, const char* name ) {
  // handle not initialized
  if ( NULL == shared_tree ) {
    return false;
  }

  // prepare process
  if ( ! prepare_process( process ) ) {
    return false;
  }

  // get item by name
  avl_node_ptr_t node = avl_find_by_data(
    shared_tree,
    ( void* )name
  );
  // handle missing
  if ( NULL == node ) {
    return false;
  }

  // get mapped item
  avl_node_ptr_t mapped = avl_find_by_data(
    process->shared_memory_mapped,
    ( void* )name
  );
  if ( NULL == mapped ) {
    return false;
  }

  shared_memory_entry_mapped_ptr_t mapped_item = ( shared_memory_entry_mapped_ptr_t )
    mapped->data;
  shared_memory_entry_ptr_t node_item = ( shared_memory_entry_ptr_t )
    node->data;

  // determine start and end
  uintptr_t start = mapped_item->start;
  uintptr_t end = start + mapped_item->size;
  // loop until end
  while ( start < end ) {
    // unmap
    virt_unmap_address(
      process->virtual_context,
      start,
      false
    );
    // next page
    start += PAGE_SIZE;
  }

  // decrement entry count
  node_item->use_count--;
  // remove item
  avl_remove_by_node( process->shared_memory_entry, node );
  avl_remove_by_node( process->shared_memory_mapped, mapped );
  // cleanup if unused
  if ( 0 == node_item->use_count ) {
    avl_remove_by_node( shared_tree, node );
    free( node );
    destroy_entry( node_item );
  }
  free( mapped );
  destroy_mapped( mapped_item );

  // return success
  return true;
}
