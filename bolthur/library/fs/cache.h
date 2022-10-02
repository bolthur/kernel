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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../collection/list/list.h"

#ifndef _CACHE_H
#define _CACHE_H

typedef struct {
  uint32_t block_size;
  void* fs;
  list_manager_t* list;
} cache_handle_t;

typedef struct {
  uint32_t block_number;
  uint32_t block_size;
  uint8_t* data;
  cache_handle_t* handle;
} cache_block_t;

typedef cache_handle_t* ( *cache_construct_t )( void* fs, uint32_t block_size );
typedef void ( *cache_destruct_t )( cache_handle_t* handle );
typedef bool ( *cache_sync_t )( cache_handle_t* handle );
typedef cache_block_t* ( *cache_block_allocate_t )( cache_handle_t* handle, uint32_t block, bool read );
typedef bool ( *cache_block_free_t )( cache_block_t* block, bool dirty );
typedef bool ( *cache_block_dirty_t )( cache_block_t* block );

void cache_destruct( cache_handle_t* );

#endif
