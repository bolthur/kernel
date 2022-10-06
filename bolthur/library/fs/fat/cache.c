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
#include "internal.h"

/**
 * @fn bool fat_cache_sync(cache_handle_t*)
 * @brief Synchronize fat cache
 *
 * @param handle
 */
bool fat_cache_sync( cache_handle_t* handle ) {
  // get first item
  list_item_t* current = handle->list->first;
  // loop until end
  while ( current ) {
    // get cache block
    cache_block_t* cache = current->data;
    // write to storage
    if ( ! fat_cache_block_dirty( cache ) ) {
      return false;
    }
    // get next
    current = current->next;
  }
  // return success
  return true;
}

/**
 * @fn cache_block_t fat_cache_block_allocate*(cache_handle_t*, uint32_t, bool)
 * @brief Allocate cache block
 *
 * @param handle
 * @param block
 * @param read
 * @return
 */
cache_block_t* fat_cache_block_allocate(
  cache_handle_t* handle,
  uint32_t block,
  bool read
) {
  // allocate cache block
  cache_block_t* cache = malloc( sizeof( cache_block_t ) );
  if ( ! cache ) {
    return NULL;
  }
  // clear memory
  memset( cache, 0, sizeof( cache_block_t ) );
  // allocate data block
  cache->data = malloc( handle->block_size );
  if ( ! cache->data ) {
    free( cache );
    return NULL;
  }
  // clear memory
  memset( cache->data, 0, handle->block_size );
  // populate data
  cache->block_number = block;
  cache->block_size = handle->block_size;
  cache->handle = handle;
  // handle read
  if ( read ) {
    // cache fs
    fat_fs_t* fs = handle->fs;
    // caculate block
    uint32_t block_read_sector = fs->partition_sector_offset + (
      block * handle->block_size
    );
    // try to read
    if ( ! fs->dev_read(
      ( uint32_t* )cache->data,
      handle->block_size,
      block_read_sector
    ) ) {
      free( cache->data );
      free( cache );
      return NULL;
    }
  }
  // insert data
  if ( ! list_insert_data( handle->list, cache ) ) {
    free( cache->data );
    free( cache );
    return NULL;
  }
  // return cache block
  return cache;
}

/**
 * @fn bool fat_cache_block_release(cache_block_t*, bool)
 * @brief Free cache block
 *
 * @param cache
 * @param dirty
 * @return
 */
bool fat_cache_block_release( cache_block_t* cache, bool dirty ) {
  // handle possible write back
  if ( dirty && ! fat_cache_block_dirty( cache ) ) {
    return false;
  }
  // remove from list
  return list_remove_data( cache->handle->list, cache );
}

/**
 * @fn bool fat_cache_block_dirty(cache_block_t*)
 * @brief Write back dirty cache block
 *
 * @param block
 * @return
 */
bool fat_cache_block_dirty( cache_block_t* cache ) {
  // cache fs
  fat_fs_t* fs = cache->handle->fs;
  // caculate block
  uint32_t block_write_sector = fs->partition_sector_offset + (
    cache->block_number * cache->block_size
  );
  // try to write
  return fs->dev_write(
    ( uint32_t* )cache->data,
    cache->block_size,
    block_write_sector
  );
}
