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
#include "cache.h"
#include "device.h"
#include "general.h"
#include "fat/bpb.h"
#include "fat/fs.h"
#include "fat/directory.h"

#ifndef _FAT_H
#define _FAT_H

#define FAT_CLUSTER_COUNT(x, size) (((x) + (size) - 1) / (size))
#define FAT_MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct {
  uint32_t first_cluster;
  uint32_t attributes;
  uint32_t size;
  char name[ 256 ];
} fat_directory_entry_t;

typedef struct {
  fat_fs_t* fs;
  fat_directory_entry_raw_t* entry_list;
  uint32_t entry_count;
  uint32_t read_index;
} fat_directory_t;

typedef struct {
  fat_fs_t* fs;
  uint32_t size;
  uint32_t block_count;
  uint32_t* block_list;
  fat_directory_entry_t entry;
} fat_file_t;

// cache related functions
bool fat_cache_sync( cache_handle_t* );
cache_block_t* fat_cache_block_allocate( cache_handle_t*, uint32_t, bool );
bool fat_cache_block_release( cache_block_t*, bool );
bool fat_cache_block_dirty( cache_block_t* );

// cluster related functions
bool fat_cluster_next( fat_fs_t*, uint32_t, uint32_t* );
uint32_t fat_cluster_get_by_file( fat_file_t*, uint32_t );
bool fat_cluster_read( fat_fs_t*, uint8_t*, uint32_t, uint32_t, uint32_t );
uint32_t fat_cluster_get_offset( fat_fs_t*, uint32_t );

// generic related functions
fat_fs_t* fat_fs_init( dev_read_t, dev_write_t, uint32_t );
bool fat_fs_mount( fat_fs_t* );
bool fat_fs_unmount( fat_fs_t* );
bool fat_fs_sync( fat_fs_t* );

// directory related functions
uint32_t fat_directory_root_cluster( fat_fs_t* );
uint32_t fat_directory_files_per_sector( fat_fs_t* );
bool fat_directory_open_root_directory( fat_fs_t*, fat_directory_t* );
bool fat_directory_open_subdirectory( fat_fs_t*, fat_directory_t*, const char*, fat_directory_t* );
bool fat_directory_open_sub_directory_by_directory( fat_fs_t*, fat_directory_entry_t*, fat_directory_t* );
int fat_directory_extract( fat_directory_t*, fat_directory_entry_t*, uint32_t* );
bool fat_directory_open( fat_fs_t*, const char*, fat_directory_t* );
bool fat_directory_read( fat_directory_t* );
void fat_directory_close( fat_directory_t* );

// file related functions
bool fat_file_open_by_directory( fat_fs_t*, fat_directory_entry_t*, fat_file_t* );
bool fat_file_open( fat_fs_t*, const char*, fat_file_t* );
bool fat_file_read( fat_file_t*, size_t, uint8_t*, size_t );
void fat_file_close( fat_file_t* );

#endif
