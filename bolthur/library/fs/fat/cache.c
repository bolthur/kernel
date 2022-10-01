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
#include "../fat.h"

__unused static void cleanup_cache_block( list_item_t* a ) {
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

cache_handle_t* fat_cache_construct(
  __unused void* fs,
  __unused uint32_t block_size
) {
  return NULL;
}

void fat_cache_sync( __unused cache_handle_t* handle ) {
}

cache_block_t* fat_cache_block_allocate(
  __unused cache_handle_t* handle,
  __unused uint32_t block,
  __unused bool read
) {
  return NULL;
}

bool fat_cache_block_free( __unused cache_block_t* cache, __unused bool dirty ) {
  return false;
}

bool fat_cache_block_dirty( __unused cache_block_t* cache ) {
  return false;
}
