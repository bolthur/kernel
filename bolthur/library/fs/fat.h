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
#include "fat/bpb.h"
#include "fat/fs.h"
#include "fat/fsinfo.h"
#include "fat/node.h"

#ifndef _FAT_H
#define _FAT_H

// generic related functions
fat_fs_t* fat_fs_init( dev_read_t, dev_write_t, uint32_t );
bool fat_fs_mount( fat_fs_t* );
bool fat_fs_unmount( fat_fs_t* );
bool fat_fs_sync( fat_fs_t* );

// cache related functions
cache_handle_t* fat_cache_construct( void*, uint32_t );
bool fat_cache_sync( cache_handle_t* );
cache_block_t* fat_cache_block_allocate( cache_handle_t*, uint32_t, bool );
bool fat_cache_block_release( cache_block_t*, bool );
bool fat_cache_block_dirty( cache_block_t* );

// FIXME: ADD FUNCTION PROTOTYPES HERE

#endif
