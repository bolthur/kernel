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

#include <stdbool.h>
#include <stdint.h>

#include "cache.h"
#include "device.h"
#include "ext/fs.h"
#include "ext/superblock.h"
#include "ext/blockgroup.h"
#include "ext/inode.h"
#include "ext/directory.h"

#ifndef _EXT_H
#define _EXT_H

// generic related functions
ext_fs_t* ext_fs_init( dev_read_t, dev_write_t, uint32_t );
bool ext_fs_mount( ext_fs_t* );
bool ext_fs_unmount( ext_fs_t* );
bool ext_fs_sync( ext_fs_t* );

// cache related functions
cache_handle_t* ext_cache_construct( void*, uint32_t );
bool ext_cache_sync( cache_handle_t* );
cache_block_t* ext_cache_block_allocate( cache_handle_t*, uint32_t, bool );
bool ext_cache_block_free( cache_block_t*, bool );
bool ext_cache_block_dirty( cache_block_t* );

// super block related functions
bool ext_superblock_read( ext_fs_t*, ext_superblock_t* );
bool ext_superblock_write( ext_fs_t*, ext_superblock_t* );
uint32_t ext_superblock_block_size( ext_superblock_t* );
uint32_t ext_superblock_frag_size( ext_superblock_t* );
uint32_t ext_superblock_total_group_by_blocks( ext_superblock_t* );
uint32_t ext_superblock_total_group_by_inode( ext_superblock_t* );
uint32_t ext_superblock_inode_size( ext_superblock_t* );
void ext_superblock_dump( ext_superblock_t* );

// block group related functions
bool ext_blockgroup_has_superblock( ext_superblock_t*, uint32_t );

// directory related functions

#endif
