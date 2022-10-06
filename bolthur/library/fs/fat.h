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

#ifndef _FAT_H
#define _FAT_H

struct fat_fs_t;

typedef struct fat_fs fat_fs_t;

typedef struct {
  uint32_t attrib;
  uint32_t ctime;
  uint32_t atime;
  uint32_t mtime;
  uint32_t cluster;
  uint32_t size;
  char name[ 256 ];
} fat_entry_t;

typedef struct {
  fat_fs_t* fs;
  cache_block_t* cache;
  fat_entry_t entry;
} fat_file_t;

typedef struct {
  fat_fs_t* fs;
  fat_entry_t* entry_list;
  uint32_t entry_count;
  cache_block_t* cache;
  uint32_t first_block;
} fat_directory_t;

// generic related functions
fat_fs_t* fat_fs_init( dev_read_t, dev_write_t, uint32_t );
bool fat_fs_mount( fat_fs_t* );
bool fat_fs_unmount( fat_fs_t* );
bool fat_fs_sync( fat_fs_t* );

// directory related functions
bool fat_directory_open( fat_fs_t*, const char*, fat_directory_t* );
bool fat_directory_read( fat_directory_t* );
void fat_directory_close( fat_directory_t* );

// file related functions
bool fat_file_open( fat_fs_t*, const char*, fat_file_t* );
bool fat_file_read( fat_file_t*, size_t, uint8_t*, size_t );
void fat_file_close( fat_file_t* );

#endif
