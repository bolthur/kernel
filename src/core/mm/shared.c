
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
#include <avl.h>
#include <string.h>
#include <assert.h>
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
  // assert existence
  assert( NULL != entry );
  // prepare area
  entry = memset( entry, 0, sizeof( shared_memory_entry_t ) );

  // allocate name array
  entry->name = ( char* )calloc( strlen( name ), sizeof( char ) );
  assert( NULL != entry->name );
  // allocate address list
  entry->address_list = ( uint64_t* )calloc( count, sizeof( uint64_t ) );
  assert( NULL != entry->address_list );

  // copy data
  entry->name = strcpy( entry->name, name );
  // allocate pages
  for ( size_t idx = 0; idx < count; idx++ ) {
    entry->address_list[ idx ] = phys_find_free_page( PAGE_SIZE );
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
  // assert existence
  assert( NULL != entry );
  // prepare area
  entry = memset( entry, 0, sizeof( shared_memory_entry_t ) );

  // allocate name array
  entry->name = ( char* )calloc( strlen( name ), sizeof( char ) );
  assert( NULL != entry->name );

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
 */
static void prepare_process( task_process_ptr_t process ) {
  // create tree if necessary
  if ( NULL == process->shared_memory_entry ) {
    process->shared_memory_entry = avl_create_tree( compare_entry );
  }
  if ( NULL == process->shared_memory_mapped ) {
    process->shared_memory_mapped = avl_create_tree( compare_mapped );
  }
}

/**
 * @brief Init shared memory
 */
void shared_init( void ) {
  shared_tree = avl_create_tree( compare_entry );
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
  avl_node_ptr_t node = avl_find_by_value(
    shared_tree,
    ( void* )name,
    lookup_entry
  );
  // skip if name is already in use
  if ( NULL != node ) {
    return false;
  }

  // create entry
  shared_memory_entry_ptr_t tmp = create_entry( name, size );
  // push back entry
  avl_insert_by_node( shared_tree, &tmp->node );
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
    return ( uintptr_t )NULL;
  }

  // prepare process
  prepare_process( process );

  // try to get item by name
  avl_node_ptr_t node = avl_find_by_value(
    shared_tree,
    ( void* )name,
    lookup_entry
  );
  // handle missing
  if ( NULL == node ) {
    return ( uintptr_t )NULL;
  }
  // try to get item by name from process
  avl_node_ptr_t process_node = avl_find_by_value(
    process->shared_memory_entry,
    ( void* )name,
    lookup_entry
  );
  // handle already attached
  if ( NULL != process_node ) {
    // get mapped entry
    avl_node_ptr_t mapped_node = avl_find_by_value(
      process->shared_memory_mapped,
      ( void* )name,
      lookup_mapped
    );
    // handle error
    if ( NULL == mapped_node ) {
      return ( uintptr_t )NULL;
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
    virt_map_address(
      process->virtual_context,
      start,
      tmp->address_list[ idx ],
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_NON_EXECUTABLE
    );
    // next one
    start += PAGE_SIZE;
    idx++;
  }

  // attach item to process list
  avl_insert_by_node( process->shared_memory_entry, node );
  // create mapped entry
  shared_memory_entry_mapped_ptr_t mapped = create_mapped(
    name, tmp->size, virt );
  // push back
  avl_insert_by_node( process->shared_memory_mapped, &mapped->node );

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
  prepare_process( process );

  // get item by name
  avl_node_ptr_t node = avl_find_by_value(
    shared_tree,
    ( void* )name,
    lookup_entry
  );
  // handle missing
  if ( NULL == node ) {
    return false;
  }

  // get mapped item
  avl_node_ptr_t mapped = avl_find_by_value(
    process->shared_memory_mapped,
    ( void* )name,
    lookup_mapped
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
  }

  // return success
  return true;
}
