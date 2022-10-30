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
#include <assert.h>
#include "../cache.h"
#include "../device.h"
#include "bpb.h"

#ifndef _FAT_FS_H
#define _FAT_FS_H

typedef enum {
  FAT_EXFAT = 0,
  FAT_FAT12 = 1,
  FAT_FAT16 = 2,
  FAT_FAT32 = 3,
} fat_type_t;

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

  fat_bpb_t* boot_sector;
  fat_type_t type;
  struct {
    uint32_t total_sectors;
    uint32_t fat_size;
    uint32_t root_dir_sectors;
    uint32_t first_data_sector;
    uint32_t first_fat_sector;
    uint32_t data_sectors;
    uint32_t total_clusters;
  } fat_data;

  cache_handle_t* handle;
} fat_fs_t;

#endif
