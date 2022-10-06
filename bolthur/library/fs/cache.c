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

#include <string.h>
#include <stdlib.h>
#include "cache.h"
#include "../collection/list/list.h"


/**
 * @fn void cache_cleanup_block(list_item_t*)
 * @brief Cleanup cache block helper
 *
 * @param a
 */
void cache_cleanup_block( list_item_t* a ) {
  // get cache block
  cache_block_t* cache = a->data;
  // free data if necessary
  if ( cache->data ) {
    free( cache->data );
  }
  // free cache block
  free( cache );
  // default list cleanup
  list_default_cleanup( a );
}

/**
 * @fn bool cache_insert_block(list_manager_t*, void*)
 * @brief Insert cache block callback
 *
 * @param list
 * @param data
 * @return
 */
bool cache_insert_block( list_manager_t* list, void* data ) {
  // push back if empty
  if ( list_empty( list ) ) {
    return list_push_back_data( list, data );
  }
  // handle already added
  if ( list_lookup_data( list, data ) ) {
    return false;
  }
  // get entry to add
  cache_block_t* cache_to_add = ( cache_block_t* )data;
  list_item_t* item = list->first;
  while ( item ) {
    // get pointer to current entry
    cache_block_t* entry = ( cache_block_t* )item->data;
    // expire greater? => use previous one t
    if ( entry->block_number > cache_to_add->block_number ) {
      break;
    }
    // get to next item
    item = item->next;
  }
  // in case there is an item to insert before, insert it
  if ( item ) {
    return list_insert_data_before( list, item, data );
  }
  // there is no match, so insert it behind
  return list_push_back_data( list, data );
}

/**
 * @fn int32_t cache_lookup_block(const list_item_t*, const void*)
 * @brief Lookup cache block callback
 *
 * @param item
 * @param data
 * @return
 */
int32_t cache_lookup_block( const list_item_t* item, const void* data ) {
  cache_block_t* cache = item->data;
  return cache->block_number == ( uint32_t )data ? 0 : 1;
}

/**
 * @fn cache_handle_t ext_cache_construct*(void*, uint32_t)
 * @brief Generate a new cache handle
 *
 * @param fs
 * @param block_size
 * @return
 */
cache_handle_t* cache_construct( void* fs, uint32_t block_size ) {
  // allocate handle
  cache_handle_t* handle = malloc( sizeof( cache_handle_t ) );
  if ( ! handle ) {
    return NULL;
  }
  // clear memory
  memset( handle, 0, sizeof( cache_handle_t ) );
  // populate data
  handle->block_size = block_size;
  handle->fs = fs;
  // create management list
  handle->list = list_construct(
    cache_lookup_block,
    cache_cleanup_block,
    cache_insert_block
  );
  if ( ! handle->list ) {
    free( handle );
    return NULL;
  }
  return handle;
}

/**
 * @fn void cache_destruct(cache_handle_t*)
 * @brief Destruct cache handle list
 *
 * @param handle
 */
void cache_destruct( cache_handle_t* handle ) {
  if ( ! handle ) {
    return;
  }
  // destroy list
  if ( handle->list ) {
    list_destruct( handle->list );
  }
  // destroy handle
  free( handle );
}
