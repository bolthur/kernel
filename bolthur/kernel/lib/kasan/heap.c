/**
 * Copyright (C) 2018 - 2025 bolthur project.
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

#include "heap.h"
#include "list.h"

/**
 * @brief Memory start address
 */
static uint8_t* memory_start = NULL;

/**
 * @brief Memory size
 */
static size_t memory_size = 0;

/**
 * @brief Free list
 */
static kasan_list_t free_list[ HEAP_MEMORY_MAX_LEVELS ];

/**
 * @fn static void* _kasan_heap_alloc(int32_t)
 * @brief Helper to allocate block on index
 * @param index
 * @return
 */
static void* _kasan_heap_alloc( const int32_t index ) {
  uint8_t* memory;
  kasan_list_t* link;
  // maximum levels reached
  if ( index >= HEAP_MEMORY_MAX_LEVELS || 0 > index ) {
    return NULL;
  }
  // handle list is empty
  if ( kasan_list_empty( &free_list[ index ] ) ) {
    // try to allocate from index above
    memory = ( uint8_t* )_kasan_heap_alloc( index - 1 );
    // handle error
    if ( ! memory ) {
      return NULL;
    }
    // subtract offset
    memory -= sizeof(memory_block_t);
    // get size of blocks at level
    const size_t size = HEAP_MEMORY_SIZE_OF_BLOCKS_AT_LEVEL(
      index, memory_size );
    // split blocks into two
    kasan_list_t* left = (kasan_list_t*)memory;
    kasan_list_t* right = (kasan_list_t*)(memory + size);
    // initialize both
    kasan_list_init(left);
    kasan_list_init(right);
    // insert both to list
    kasan_list_insert_tail( &free_list[ index ], left );
    kasan_list_insert_tail( &free_list[ index ], right );
  }
  // get first item
  link = kasan_list_shift( &free_list[ index ] );
  // set memory
  memory = ( uint8_t* )link;
  // populate block
  memory_block_t* block = ( memory_block_t* )memory;
  block->size = HEAP_MEMORY_SIZE_OF_BLOCKS_AT_LEVEL( index, memory_size );
  // return memory
  return ( void* )( memory + sizeof( memory_block_t ) );
}

/**
 * @fn static void _kasan_heap_free(void*, int32_t)
 * @brief Internal heap free implementation
 * @param mem
 * @param level
 */
static void _kasan_heap_free( void* mem, const int32_t level ) {
  // list entries for link and buddy link
  kasan_list_t* link = NULL;
  kasan_list_t* buddy_link = NULL;
  // block to get real size
  const memory_block_t* block = ( memory_block_t* )mem;
  // cache block size
  const size_t size = block->size;
  // get memory index of pointer in level
  const int32_t index = ( int32_t )HEAP_MEMORY_INDEX_OF_POINTER_IN_LEVEL(
    mem, level, memory_start, memory_size );
  // variable for buddy address
  uintptr_t buddy;
  // determine buddy
  if ( ! ( index & 1 ) ) {
    buddy = ( uintptr_t )mem + size;
  } else {
    buddy = ( uintptr_t )mem - size;
  }
  // if not empty try to find buddy link
  if ( ! kasan_list_empty( &free_list[ level ] ) ) {
    // try to find buddy in free list of level
    buddy_link = kasan_list_find( &free_list[ level ], buddy );
  }
  link = ( kasan_list_t* )mem;
  // initialize link
  kasan_list_init( link );
  // insert into free list
  kasan_list_insert_tail( &free_list[ level ], link );
  // handle buddy link found
  if (buddy_link) {
    // remove link and buddy link
    kasan_list_remove( link );
    kasan_list_remove( buddy_link );
    // handle recursive free depending on index
    if ( ! ( index & 1 ) ) {
      _kasan_heap_free( link, level - 1 );
    } else {
      _kasan_heap_free( buddy_link, level - 1 );
    }
  }
}

/**
 * @fn int32_t kasan_heap_get_level(size_t)
 * @brief Helper to get heap level by size
 * @param size
 * @return
 */
__no_sanitize int32_t kasan_heap_get_level( const size_t size ) {
  int32_t level = 0;
  size_t heap_size = memory_size;
  while ( heap_size > size ) {
    heap_size /= 2;
    level++;
  }
  return level;
}

/**
 * @fn void kasan_heap_init(void*, size_t)
 * @brief Initialize kasan shadow heap
 */
__no_sanitize void kasan_heap_init( void* mem, size_t size ) {
  kasan_list_t* entry = ( kasan_list_t* )mem;
  memory_start = mem;
  memory_size = size;
  // setup free list
  for ( int i = 0; i < HEAP_MEMORY_MAX_LEVELS; i++ ) {
    kasan_list_init( &free_list[ i ] );
  }
  // setup list entry
  kasan_list_init( entry );
  // insert on top level
  kasan_list_insert_tail( &free_list[ 0 ], entry );
}

/**
 * @fn void* kasan_heap_alloc(size_t)
 * @brief Allocate something on kasan heap
 * @param size
 * @return
 */
__no_sanitize void* kasan_heap_alloc( size_t size ) {
  // adjust size by memory block
  size += sizeof( memory_block_t );
  // get level by size
  const int32_t level = kasan_heap_get_level( size );
  // allocate and return result
  return _kasan_heap_alloc( level );
}

/**
 * @fn void kasan_heap_free(void*)
 * @brief Free something on kasan heap
 * @param mem
 */
__no_sanitize void kasan_heap_free( void* mem ) {
  // convert to uint8_t
  uint8_t* memory = ( uint8_t* )mem;
  // assert memory within heap
  if ( memory <= memory_start || memory >= memory_start + memory_size ) {
    return;
  }
  // get management block
  memory_block_t* block = ( memory_block_t* )( memory - sizeof( memory_block_t ) );
  // get level by size
  const int32_t level = kasan_heap_get_level( block->size );
  // call recursive free
  _kasan_heap_free( ( void* )block, level );
}
