
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
#include <list.h>
#include <string.h>
#include <assert.h>
#include <core/panic.h>
#include <core/mm/phys.h>
#include <core/mm/shared.h>

/**
 * @brief list of shared memory entries
 */
list_manager_ptr_t shared_memory = NULL;

/**
 * @brief Helper to get item by name
 *
 * @param name name to lookup
 * @return list_item_ptr_t
 */
static list_item_ptr_t entry_by_name(
  const char* name,
  list_manager_ptr_t list
) {
  // check for name is already in use
  list_item_ptr_t item = list->first;
  // loop until end of list
  while ( item ) {
    // get entry
    shared_memory_entry_ptr_t entry = ( shared_memory_entry_ptr_t )item->data;
    // skip if length is not equal
    if ( strlen( name ) != strlen( entry->name ) ) {
      item = item->next;
      continue;
    }
    // break if name is equal
    if ( 0 == strncmp( entry->name, name, strlen( name ) ) ) {
      return item;
    }
    // next one
    item = item->next;
  }
  // not found
  return NULL;
}

/**
 * @brief Helper to get item by name
 *
 * @param name name to lookup
 * @return list_item_ptr_t
 */
static list_item_ptr_t mapped_by_name(
  const char* name,
  list_manager_ptr_t list
) {
  // check for name is already in use
  list_item_ptr_t item = list->first;
  // loop until end of list
  while ( item ) {
    // get entry
    shared_memory_entry_mapped_ptr_t entry = ( shared_memory_entry_mapped_ptr_t )
      item->data;
    // skip if length is not equal
    if ( strlen( name ) != strlen( entry->name ) ) {
      item = item->next;
      continue;
    }
    // break if name is equal
    if ( 0 == strncmp( entry->name, name, strlen( name ) ) ) {
      return item;
    }
    // next one
    item = item->next;
  }
  // not found
  return NULL;
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
 * @brief Init shared memory
 */
void shared_init( void ) {
  shared_memory = list_construct();
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
  if ( NULL == shared_memory ) {
    return false;
  }

  // entry variable
  list_item_ptr_t entry = entry_by_name( name, shared_memory );
  // skip if name is already in use
  if ( NULL != entry ) {
    return false;
  }

  // create entry
  shared_memory_entry_ptr_t tmp = create_entry( name, size );
  // push back entry
  list_push_back( shared_memory, tmp );
  // return success
  return true;
}

/**
 * @brief Extend shared memory area
 *
 * @param name name to extend
 * @return uintptr_t
 *
 * @todo implement logic
 */
uintptr_t shared_memory_extend( const char* name ) {
  // try to get item by name
  list_item_ptr_t entry = entry_by_name( name, shared_memory );
  // handle missing
  if ( NULL == entry ) {
    return ( uintptr_t )NULL;
  }

  // FIXME: ADD LOGIC
  PANIC( "Not implemented!" );

  return (uintptr_t)NULL;
}

/**
 * @brief Process acquire of shared memory
 *
 * @param process process
 * @param name name
 * @return uintptr_t start address or NULL if not existing / already mapped
 */
uintptr_t shared_memory_acquire( task_process_ptr_t process, const char* name ) {
  // try to get item by name
  list_item_ptr_t list_entry = entry_by_name( name, shared_memory );
  // handle missing
  if ( NULL == list_entry ) {
    return ( uintptr_t )NULL;
  }
  // try to get item by name from process
  list_item_ptr_t process_entry = entry_by_name(
    name, process->shared_memory_entry );
  // handle already attached
  if ( NULL != process_entry ) {
    // get mapped entry
    list_item_ptr_t mapped_entry = mapped_by_name(
      name, process->shared_memory_mapped );
    // handle error
    if ( NULL == mapped_entry ) {
      return ( uintptr_t )NULL;
    }
    // return start
    return ( ( shared_memory_entry_mapped_ptr_t )mapped_entry->data )->start;
  }

  shared_memory_entry_ptr_t tmp = ( shared_memory_entry_ptr_t )list_entry->data;
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
  list_push_back_node( process->shared_memory_entry, list_entry );
  // create mapped entry
  shared_memory_entry_mapped_ptr_t mapped = create_mapped(
    name, tmp->size, virt );
  // push back
  list_push_back( process->shared_memory_mapped, mapped );

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
bool shared_memory_release( __unused task_process_ptr_t process, const char* name ) {
  // get item by name
  list_item_ptr_t entry = entry_by_name( name, shared_memory );
  // handle missing
  if ( NULL == entry ) {
    return false;
  }

  // get mapped item
  list_item_ptr_t mapped = mapped_by_name(
    name, process->shared_memory_mapped );
  if ( NULL == mapped ) {
    return false;
  }

  // determine start and end
  uintptr_t start = ( ( shared_memory_entry_mapped_ptr_t )mapped->data )->start;
  uintptr_t end = start + ( ( shared_memory_entry_mapped_ptr_t )mapped->data )->size;
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
  ( ( shared_memory_entry_ptr_t )entry->data )->use_count--;
  // remove item
  list_remove_data_keep( process->shared_memory_entry, entry );
  list_remove_data( process->shared_memory_mapped, mapped );
  // cleanup if unused
  if ( 0 == ( ( shared_memory_entry_ptr_t )entry->data )->use_count ) {
    list_remove_data( shared_memory, entry );
  }

  // return success
  return true;
}
