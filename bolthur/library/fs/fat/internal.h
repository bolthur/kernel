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
#include <stdbool.h>
#include "../cache.h"
#include "../fat.h"
#include "bpb.h"
#include "directory.h"
#include "file.h"
#include "fs.h"
#include "fsinfo.h"

#ifndef _FAT_INTERNAL_H
#define _FAT_INTERNAL_H

// cache related functions
bool fat_cache_sync( cache_handle_t* );
cache_block_t* fat_cache_block_allocate( cache_handle_t*, uint32_t, bool );
bool fat_cache_block_release( cache_block_t*, bool );
bool fat_cache_block_dirty( cache_block_t* );

// directory related stuff
bool fat_directory_open_root(fat_fs_t*, fat_directory_t* );
uint32_t fat_directory_files_per_sector( fat_fs_t* );
bool fat_find_directory_get_next_sector( fat_fs_t*, const char*, uint32_t, fat_directory_entry_raw_t* );

#endif
