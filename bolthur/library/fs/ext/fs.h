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
#include "superblock.h"
#include "../cache.h"
#include "../device.h"

#ifndef _EXT_FS_H
#define _EXT_FS_H

typedef struct {
  dev_read_t dev_read;
  dev_write_t dev_write;

  cache_construct_t cache_construct;
  cache_destruct_t cache_destruct;
  cache_sync_t cache_sync;
  cache_block_allocate_t cache_block_allocate;
  cache_block_release_t cache_block_release;
  cache_block_dirty_t cache_block_dirty;

  uint32_t partition_sector_offset;

  ext_superblock_t* superblock;

  uint8_t* boot_sector;
  cache_handle_t* handle;
} ext_fs_t;

#endif
